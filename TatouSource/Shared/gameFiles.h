///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The check of a data folder's game files against the editions we know
// (knownEditions.h): a copy, a GOG or Steam installation or the player's CD
// image (gameDisc.h). Each .PAK and .ITD (not the saves, SAVEn.ITD) and each
// .EXE a known edition has is read and its size and CRC-32 compared; a file
// in no known edition is noted ("NAME size crc", the line to add once that
// edition works), and so is a needed file that is not there (the languages
// and the .EXE are not needed). What was read is kept in a small file
// (GameFilesCache) by size and date: unchanged files are not read again.
// Used by the launcher (in a thread, with its progress) and the game (only
// noted in tatou.log). Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cdImage.h"
#include "knownEditions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

// The game whose files are checked: its needed files
enum class GameFilesOf { Aitd1, Jack, Aitd2, Aitd3 };

// Where they are: a folder of loose files, or a folder of a CD image
struct GameFilesSource
{
    GameFilesOf game = GameFilesOf::Aitd1;
    std::filesystem::path folder;           // the loose files' folder, or the image file
    std::shared_ptr<const CdImage> image;   // the CD image, null for loose files
    std::string imageFolder;                // its folder (INDARK, INDARK2, INDARK3, JACK)
    std::string prefix;                     // before each name in the lines ("JACK/" with AITD1)
};

// What the check found
struct GameFilesResult
{
    std::vector<std::string> unknown;   // in no known edition: "NAME size 0xCRC"
    std::vector<std::string> missing;   // needed and not there: "NAME"
};

// CRC-32 (the one of zip and PNG) of some bytes
inline unsigned int gameFileCrc32(const std::vector<unsigned char>& data)
{
    // Built once (thread-safe: a static initialised by a lambda)
    static const std::array<unsigned int, 256> table = []
    {
        std::array<unsigned int, 256> built{};
        for (unsigned int i = 0; i < 256; i++)
        {
            unsigned int c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            built[i] = c;
        }
        return built;
    }();
    unsigned int crc = 0xFFFFFFFFu;
    for (unsigned char byte : data)
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

namespace gameFilesDetail
{
    inline std::string upper(std::string text)
    {
        for (char& c : text)
            c = (char)toupper((unsigned char)c);
        return text;
    }

    // A path as UTF-8 text (C++17 and C++20 alike)
    inline std::string utf8(const std::filesystem::path& path)
    {
        const auto text = path.u8string();
        return std::string(text.begin(), text.end());
    }

    // A language of the game: not needed (an edition has only some)
    inline bool isLanguage(const std::string& name)
    {
        static const char* const languages[] = { "ENGLISH.PAK", "USA.PAK", "FRANCAIS.PAK", "DEUTSCH.PAK",
                                                  "ESPAGNOL.PAK", "ITALIANO.PAK", "KOREAN.PAK" };
        for (const char* language : languages)
            if (name == language)
                return true;
        return false;
    }

    // A file to check (upper-case name): .PAK, .ITD but the saves (SAVEn.ITD),
    // and an .EXE a known edition has (not an installer)
    inline bool isGameFile(const std::string& name)
    {
        const size_t dot = name.rfind('.');
        const std::string ext = dot == std::string::npos ? std::string() : name.substr(dot);
        if (ext == ".PAK")
            return true;
        if (ext == ".ITD")
            return !(name.compare(0, 4, "SAVE") == 0 && dot > 4
                && std::all_of(name.begin() + 4, name.begin() + dot, [](char c) { return isdigit((unsigned char)c) != 0; }));
        return ext == ".EXE" && isKnownGameFileName(name);
    }

    // The known edition whose names say which files a game needs
    inline KnownEdition neededFiles(GameFilesOf game)
    {
        const std::vector<KnownEdition>& editions = knownEditions();
        // knownEditions(): AITD1, Jack, AITD2, AITD2 (GOG's languages), AITD3, AITD3 (Spanish)
        return editions[game == GameFilesOf::Aitd1 ? 0 : game == GameFilesOf::Jack ? 1 : game == GameFilesOf::Aitd2 ? 2 : 4];
    }

    // Size and date of a file: what tells that it changed
    inline std::string fileStamp(const std::filesystem::path& file)
    {
        std::error_code ec;
        const unsigned long long size = (unsigned long long)std::filesystem::file_size(file, ec);
        if (ec)
            return std::string();
        const long long date = (long long)std::filesystem::last_write_time(file, ec).time_since_epoch().count();
        if (ec)
            return std::string();
        return std::to_string(size) + ":" + std::to_string(date);
    }
}

// What the check read before (UserData\<game>\fileCheck.cfg): each file's
// stamp (size and date of the file, or of the CD image it is in) with its size
// and CRC-32, and the problems the player chose not to be
// warned about again (the launcher's "Do not warn again")
class GameFilesCache
{
public:
    std::string accepted;   // the signature of the accepted problems (gameFilesSignature)

    void load(const std::filesystem::path& file)
    {
        std::ifstream in(file);
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.compare(0, 11, "accepted = ") == 0)
                accepted = line.substr(11);
            else if (line.compare(0, 7, "file = ") == 0)
            {
                // file = <stamp> <crc> <size> <key>
                const size_t a = line.find(' ', 7);
                const size_t b = a == std::string::npos ? a : line.find(' ', a + 1);
                const size_t c = b == std::string::npos ? b : line.find(' ', b + 1);
                if (c == std::string::npos)
                    continue;
                Entry entry;
                entry.stamp = line.substr(7, a - 7);
                entry.crc = (unsigned int)strtoul(line.substr(a + 1, b - a - 1).c_str(), nullptr, 16);
                entry.size = (unsigned int)strtoul(line.substr(b + 1, c - b - 1).c_str(), nullptr, 10);
                m_files[line.substr(c + 1)] = entry;
            }
        }
    }

    void save(const std::filesystem::path& file) const
    {
        std::ofstream out(file);
        if (!out)
            return;
        out << "# Alone in the Dark: Classic Reborn - what the check of the game files read\n";
        if (!accepted.empty())
            out << "accepted = " << accepted << "\n";
        for (const auto& entry : m_files)
        {
            char crc[16];
            snprintf(crc, sizeof(crc), "%08X", entry.second.crc);
            out << "file = " << entry.second.stamp << " " << crc << " " << entry.second.size << " " << entry.first << "\n";
        }
    }

    // The size and CRC-32 kept for this file with this stamp; false if none or it changed
    bool find(const std::string& key, const std::string& stamp, unsigned int& size, unsigned int& crc) const
    {
        const auto found = m_files.find(key);
        if (found == m_files.end() || stamp.empty() || found->second.stamp != stamp)
            return false;
        size = found->second.size;
        crc = found->second.crc;
        return true;
    }

    void store(const std::string& key, const std::string& stamp, unsigned int size, unsigned int crc)
    {
        if (!stamp.empty())
            m_files[key] = { stamp, crc, size };
    }

private:
    struct Entry
    {
        std::string stamp;
        unsigned int crc = 0;
        unsigned int size = 0;
    };
    std::map<std::string, Entry> m_files;
};

// The files of a source to check (their names as they are)
inline std::vector<std::string> gameFilesToCheck(const GameFilesSource& source)
{
    std::vector<std::string> names;
    if (source.image)
    {
        for (const std::string& name : source.image->filesIn(source.imageFolder))
            if (gameFilesDetail::isGameFile(gameFilesDetail::upper(name)))
                names.push_back(name);
        return names;
    }
    std::error_code ec;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(source.folder, ec))
    {
        const std::string name = gameFilesDetail::utf8(entry.path().filename());
        if (entry.is_regular_file(ec) && gameFilesDetail::isGameFile(gameFilesDetail::upper(name)))
            names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// Checks one file of a source (a name from gameFilesToCheck); a file in no
// known edition goes to result.unknown
inline void checkGameFile(const GameFilesSource& source, const std::string& name, GameFilesCache& cache, GameFilesResult& result)
{
    namespace fs = std::filesystem;
    const fs::path file = source.image ? source.folder : source.folder / fs::u8path(name);
    const std::string key = gameFilesDetail::utf8(source.folder) + "|" + source.imageFolder + "|" + name;
    const std::string stamp = gameFilesDetail::fileStamp(file);
    unsigned int crc = 0;
    unsigned int size = 0;
    if (!cache.find(key, stamp, size, crc))
    {
        std::vector<unsigned char> data;
        if (source.image)
        {
            if (!source.image->readFile(source.imageFolder, name, data))
                return;
        }
        else
        {
            std::ifstream in(file, std::ios::binary);
            data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        }
        size = (unsigned int)data.size();
        crc = gameFileCrc32(data);
        cache.store(key, stamp, size, crc);
    }
    const std::string upperName = gameFilesDetail::upper(name);
    if (isKnownGameFile(upperName, size, crc))
        return;
    char line[160];
    snprintf(line, sizeof(line), "%s%s %u 0x%08X", source.prefix.c_str(), upperName.c_str(), size, crc);
    result.unknown.push_back(line);
}

// The files the game needs that are not among `present` (gameFilesToCheck)
// go to result.missing
inline void findMissingGameFiles(const GameFilesSource& source, const std::vector<std::string>& present, GameFilesResult& result)
{
    std::vector<std::string> names;
    for (const std::string& name : present)
        names.push_back(gameFilesDetail::upper(name));
    const KnownEdition needed = gameFilesDetail::neededFiles(source.game);
    for (size_t i = 0; i < needed.count; i++)
    {
        const std::string name = needed.files[i].name;
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".EXE") == 0)
            continue;
        if (!gameFilesDetail::isLanguage(name) && std::find(names.begin(), names.end(), name) == names.end())
            result.missing.push_back(source.prefix + name);
    }
}

// Checks every file of some sources at once (the game; the launcher uses the
// steps above, to show its progress)
inline GameFilesResult checkGameFiles(const std::vector<GameFilesSource>& sources, GameFilesCache& cache)
{
    GameFilesResult result;
    for (const GameFilesSource& source : sources)
    {
        const std::vector<std::string> names = gameFilesToCheck(source);
        for (const std::string& name : names)
            checkGameFile(source, name, cache, result);
        findMissingGameFiles(source, names, result);
    }
    return result;
}

// One text for a result's problems, to know whether they are the ones the
// player accepted (GameFilesCache::accepted); empty: no problems
inline std::string gameFilesSignature(const GameFilesResult& result)
{
    std::string all;
    for (const std::string& line : result.unknown)
        all += line + "\n";
    for (const std::string& name : result.missing)
        all += "-" + name + "\n";
    if (all.empty())
        return all;
    char signature[16];
    snprintf(signature, sizeof(signature), "%08X", gameFileCrc32(std::vector<unsigned char>(all.begin(), all.end())));
    return signature;
}
