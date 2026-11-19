///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Narrators: besides the game's own (English), each folder in AITD1's
// Voices folder (Shared/userFolder.h) is a narrator, e.g. Voices\French with
// the voices of the player's French CD, or a fans' dubbing. A folder holds
// either loose voice files or one CD image (.iso, or .bin with or without
// its .cue) with them in its INDARK folder. A narrator can also be a single
// file, a narrator pack: Voices\Spanish.voice, the voice files gathered one
// after the other without compression (zipArchive.h; tools/makeVoicePack.py),
// named as the folder would be; a folder of the same name comes first.
//
// Voice files: BBSSNN with the extension .VOC, .WAV or .OGG: BB the book
// (00-19), SS its page, NN a piece of that page's narration; a page plays
// its pieces in order until the first missing one, so a single file per
// page (BBSS00) is enough. Single readings use their whole number (e.g.
// 001006). A folder counts as a narrator when it has 000000 (the first page
// of the first book). Used by the game and the launcher. Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cdImage.h"
#include "zipArchive.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

// A path as UTF-8 text (C++17 and C++20 alike)
inline std::string narratorPathText(const std::filesystem::path& path)
{
    const auto text = path.u8string();
    return std::string(text.begin(), text.end());
}

class NarratorVoices
{
public:
    // The voices of a folder, or of a narrator pack (the path with or without
    // .voice); false if it has none (no 000000 file or image with it)
    bool open(const std::filesystem::path& folder)
    {
        namespace fs = std::filesystem;
        m_folder = folder;
        m_name = narratorPathText(folder.filename());
        m_image.reset();
        m_pack.reset();
        std::error_code ec;
        if (!fs::is_directory(folder, ec))
        {
            fs::path packPath = folder;
            if (!isVoicePack(packPath))
                packPath += ".voice";
            if (!fs::is_regular_file(packPath, ec))
                return false;
            auto pack = std::make_unique<ZipArchive>();
            if (!pack->open(packPath))
                return false;
            m_pack = std::move(pack);
            m_name = narratorPathText(packPath.stem());
            std::vector<unsigned char> probe;
            std::string ext;
            return readPiece(0, probe, ext);
        }
        for (const fs::directory_entry& entry : fs::directory_iterator(folder, ec))
        {
            std::string ext = narratorPathText(entry.path().extension());
            for (char& c : ext)
                c = (char)tolower((unsigned char)c);
            if (ext != ".iso" && ext != ".bin" && ext != ".img")
                continue;
            auto image = std::make_unique<CdImage>();
            if (image->open(entry.path()) && image->hasFile("INDARK", "000000.VOC"))
            {
                m_image = std::move(image);
                break;
            }
        }
        std::vector<unsigned char> probe;
        std::string ext;
        return readPiece(0, probe, ext);
    }

    const std::string& name() const { return m_name; }
    // The image the voices come from, or empty (loose files)
    std::filesystem::path imagePath() const { return m_image ? m_image->path() : std::filesystem::path(); }
    // The narrator pack the voices come from, or empty
    std::filesystem::path packPath() const { return m_pack ? m_pack->path() : std::filesystem::path(); }

    // A narrator pack's file name (Spanish.voice, any case)
    static bool isVoicePack(const std::filesystem::path& path)
    {
        std::string ext = narratorPathText(path.extension());
        for (char& c : ext)
            c = (char)tolower((unsigned char)c);
        return ext == ".voice";
    }

    // Voice number (e.g. 10203 = book 1, page 2, piece 3) as stored, with its
    // extension (".voc", ".wav" or ".ogg"); false if there is none
    bool readPiece(int number, std::vector<unsigned char>& data, std::string& ext) const
    {
        char name[16];
        snprintf(name, sizeof(name), "%06d", number);
        if (m_image)
        {
            ext = ".voc";
            return m_image->readFile("INDARK", std::string(name) + ".VOC", data);
        }
        for (const char* candidate : { ".voc", ".wav", ".ogg" })
        {
            if (m_pack ? m_pack->read(std::string(name) + candidate, data) && !data.empty() : readWhole(m_folder / (std::string(name) + candidate), data))
            {
                ext = candidate;
                return true;
            }
        }
        return false;
    }

    bool hasPiece(int number) const
    {
        char name[16];
        snprintf(name, sizeof(name), "%06d", number);
        if (m_image)
            return m_image->hasFile("INDARK", std::string(name) + ".VOC");
        if (m_pack)
        {
            for (const char* candidate : { ".voc", ".wav", ".ogg" })
            {
                const ZipArchive::Entry* entry = m_pack->find(std::string(name) + candidate);
                if (entry && entry->method == 0)
                    return true;
            }
            return false;
        }
        std::error_code ec;
        for (const char* candidate : { ".voc", ".wav", ".ogg" })
            if (std::filesystem::exists(m_folder / (std::string(name) + candidate), ec))
                return true;
        return false;
    }

private:
    static bool readWhole(const std::filesystem::path& path, std::vector<unsigned char>& data)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return !data.empty();
    }

    std::filesystem::path m_folder;
    std::string m_name;
    std::unique_ptr<CdImage> m_image;
    std::unique_ptr<ZipArchive> m_pack;
};

// The narrators with voices inside a Voices folder (folders and narrator
// packs), by name; a name only once
inline std::vector<std::string> findNarrators(const std::filesystem::path& voicesFolder)
{
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(voicesFolder, ec))
    {
        NarratorVoices voices;
        const bool candidate = entry.is_directory(ec) || (entry.is_regular_file(ec) && NarratorVoices::isVoicePack(entry.path()));
        if (candidate && voices.open(entry.path()) &&
            std::find(names.begin(), names.end(), voices.name()) == names.end())
            names.push_back(voices.name());
    }
    std::sort(names.begin(), names.end());
    return names;
}
