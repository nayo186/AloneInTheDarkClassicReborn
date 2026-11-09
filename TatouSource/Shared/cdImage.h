///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// A CD image, only read: an .iso (2048-byte sectors) or a .bin / GOG's
// GAME.GOG (raw 2352-byte sectors, the kind is found from the image itself).
// The files of the first level of folders of the CD (ISO 9660, e.g.
// INDARK/000123.VOC) and, when a track list (.cue, GOG's GAME.INS) is given,
// its tracks (the music): in the image itself or, as CD rips often are, one
// .bin per track named in the .cue. Used by the game (GOG installations, the
// player's CD, narrators' voices) and by the launcher (finding games and
// narrators). Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

struct CdTrack
{
    int number = 0;
    bool audio = false;
    std::filesystem::path file;   // the file the track is in (empty: not a binary file)
    long long start = 0;          // first byte of the track in that file
    long long end = 0;            // first byte after it
};

class CdImage
{
public:
    // Reads the image's file list (and the track list, if a cue file is
    // given); false if it is not a CD image with files
    bool open(const std::filesystem::path& image, const std::filesystem::path& cue = std::filesystem::path())
    {
        m_path = image;
        m_files.clear();
        m_tracks.clear();
        std::ifstream file(image, std::ios::binary);
        if (!file)
            return false;
        unsigned char descriptor[kDataSize];
        // Raw sectors first (sync and header before the data: 16 bytes in
        // MODE1, 24 in MODE2), then plain ones
        bool found = false;
        static const int kLayouts[3][2] = { { 2352, 16 }, { 2352, 24 }, { kDataSize, 0 } };
        for (const auto& layout : kLayouts)
        {
            m_sectorSize = layout[0];
            m_dataOffset = layout[1];
            if (readSector(file, 16, descriptor) && memcmp(descriptor + 1, "CD001", 5) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;

        // The root folder, then each folder in it
        std::vector<unsigned char> root;
        if (!readExtent(file, get32(descriptor + 158), get32(descriptor + 166), root))
            return false;
        forEachRecord(root, [&](const unsigned char* record, const std::string& name, bool folder)
        {
            std::vector<unsigned char> data;
            if (folder && readExtent(file, get32(record + 2), get32(record + 10), data))
            {
                forEachRecord(data, [&](const unsigned char* entry, const std::string& entryName, bool entryFolder)
                {
                    if (!entryFolder)
                        m_files[name + "/" + entryName] = { get32(entry + 2), get32(entry + 10) };
                });
            }
        });
        if (!cue.empty())
            readTrackList(cue);
        return !m_files.empty();
    }

    // The image file
    const std::filesystem::path& path() const { return m_path; }
    // How many files its first level of folders has
    int fileCount() const { return (int)m_files.size(); }
    // Its tracks, from the track list (empty without one)
    const std::vector<CdTrack>& tracks() const { return m_tracks; }

    bool hasFile(const std::string& folder, const std::string& name) const
    {
        return m_files.count(upper(folder) + "/" + upper(name)) != 0;
    }

    // The names of the files in a folder of the CD (upper case)
    std::vector<std::string> filesIn(const std::string& folder) const
    {
        std::vector<std::string> names;
        const std::string prefix = upper(folder) + "/";
        for (const auto& entry : m_files)
            if (entry.first.compare(0, prefix.size(), prefix) == 0)
                names.push_back(entry.first.substr(prefix.size()));
        return names;
    }

    // A file of the CD, e.g. ("INDARK", "000123.VOC"); false if not there.
    // Opens the image each time: safe from several threads.
    bool readFile(const std::string& folder, const std::string& name, std::vector<unsigned char>& data) const
    {
        auto found = m_files.find(upper(folder) + "/" + upper(name));
        if (found == m_files.end())
            return false;
        std::ifstream file(m_path, std::ios::binary);
        return file && readExtent(file, found->second.sector, found->second.size, data);
    }

    // The image file a .cue names first (its data track), found whatever the
    // case of its name; empty if none is there
    static std::filesystem::path cueImageFile(const std::filesystem::path& cue)
    {
        std::ifstream list(cue);
        std::string line;
        while (std::getline(list, line))
        {
            std::string name, type;
            if (readFileLine(line, name, type))
                return findFile(cue.parent_path(), name);
        }
        return std::filesystem::path();
    }

private:
    static const int kDataSize = 2048;
    struct Entry { unsigned int sector; unsigned int size; };

    static unsigned int get32(const unsigned char* p)
    {
        return p[0] | p[1] << 8 | p[2] << 16 | (unsigned int)p[3] << 24;
    }

    static std::string upper(std::string text)
    {
        for (char& c : text)
            c = (char)toupper((unsigned char)c);
        return text;
    }

    // A path as UTF-8 text (u8string is of char8_t in C++20)
    static std::string utf8(const std::filesystem::path& path)
    {
        const auto text = path.u8string();
        return std::string(text.begin(), text.end());
    }

    // A file of a folder by its name in any case (a .cue written on Windows
    // read on Linux); empty if not there
    static std::filesystem::path findFile(const std::filesystem::path& folder, const std::string& name)
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path exact = folder / fs::u8path(name);
        if (fs::exists(exact, ec))
            return exact;
        const std::string wanted = upper(utf8(fs::u8path(name).filename()));
        for (const fs::directory_entry& entry : fs::directory_iterator(exact.parent_path(), ec))
            if (upper(utf8(entry.path().filename())) == wanted)
                return entry.path();
        return fs::path();
    }

    // FILE "name" TYPE (the name may have no quotes)
    static bool readFileLine(const std::string& line, std::string& name, std::string& type)
    {
        const size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line.compare(start, 5, "FILE ") != 0)
            return false;
        std::string rest = line.substr(start + 5);
        rest.erase(0, rest.find_first_not_of(" \t"));
        size_t nameEnd;
        if (!rest.empty() && rest[0] == '"')
        {
            nameEnd = rest.find('"', 1);
            if (nameEnd == std::string::npos)
                return false;
            name = rest.substr(1, nameEnd - 1);
            nameEnd++;
        }
        else
        {
            nameEnd = rest.find_last_of(" \t");
            if (nameEnd == std::string::npos)
                return false;
            name = rest.substr(0, nameEnd);
        }
        std::istringstream words(rest.substr(nameEnd));
        words >> type;
        type = upper(type);
        // Windows paths in the .cue
        std::replace(name.begin(), name.end(), '\\', '/');
        return !name.empty();
    }

    bool readSector(std::ifstream& file, long long sector, unsigned char* out) const
    {
        file.clear();
        file.seekg(sector * m_sectorSize + m_dataOffset);
        file.read((char*)out, kDataSize);
        return file.gcount() == kDataSize;
    }

    bool readExtent(std::ifstream& file, unsigned int sector, unsigned int size, std::vector<unsigned char>& out) const
    {
        out.resize(size);
        unsigned char buffer[kDataSize];
        for (size_t done = 0, i = 0; done < size; i++)
        {
            if (!readSector(file, sector + i, buffer))
                return false;
            const size_t part = (std::min)((size_t)size - done, (size_t)kDataSize);
            memcpy(&out[done], buffer, part);
            done += part;
        }
        return true;
    }

    // ISO 9660 directory records; "." and ".." skipped, names without ";1"
    template <class Visit>
    static void forEachRecord(const std::vector<unsigned char>& data, Visit visit)
    {
        size_t pos = 0;
        while (pos < data.size())
        {
            const unsigned char length = data[pos];
            if (length == 0)
            {
                pos = (pos / kDataSize + 1) * kDataSize;   // records do not cross sectors
                continue;
            }
            if (pos + length > data.size())
                return;
            const unsigned char* record = &data[pos];
            pos += length;
            const unsigned char nameLength = record[32];
            if (nameLength == 0 || (nameLength == 1 && record[33] <= 1))
                continue;
            std::string name((const char*)record + 33, nameLength);
            visit(record, upper(name.substr(0, name.find(';'))), (record[25] & 2) != 0);
        }
    }

    // FILE "x.bin" BINARY, then "TRACK 02 AUDIO" and "INDEX 01 05:31:19"
    // (minutes, seconds, frames from the start of that file). The first
    // FILE is the image itself; a track in a file that is not binary (GOG's
    // AITD3 names .ogg files) has no file.
    void readTrackList(const std::filesystem::path& cue)
    {
        namespace fs = std::filesystem;
        std::ifstream list(cue);
        std::string line;
        fs::path currentFile = m_path;
        bool firstFile = true;
        int trackSectorSize = 2352;
        while (std::getline(list, line))
        {
            std::string name, type;
            if (readFileLine(line, name, type))
            {
                currentFile = firstFile ? m_path : (type == "BINARY" ? findFile(cue.parent_path(), name) : fs::path());
                firstFile = false;
                continue;
            }
            std::istringstream words(line);
            std::string word;
            words >> word;
            if (word == "TRACK")
            {
                CdTrack track;
                std::string mode;
                words >> track.number >> mode;
                mode = upper(mode);
                track.audio = (mode == "AUDIO");
                track.file = currentFile;
                trackSectorSize = mode.find("/2048") != std::string::npos ? 2048 : 2352;
                m_tracks.push_back(track);
            }
            else if (word == "INDEX" && !m_tracks.empty())
            {
                int index = 0, minutes = 0, seconds = 0, frames = 0;
                std::string time;
                words >> index >> time;
                if (index == 1 && sscanf(time.c_str(), "%d:%d:%d", &minutes, &seconds, &frames) == 3)
                    m_tracks.back().start = (((long long)minutes * 60 + seconds) * 75 + frames) * trackSectorSize;
            }
        }
        // A track ends where the next one in its file starts, else at the end of the file
        for (size_t i = 0; i < m_tracks.size(); i++)
        {
            CdTrack& track = m_tracks[i];
            if (i + 1 < m_tracks.size() && m_tracks[i + 1].file == track.file)
                track.end = m_tracks[i + 1].start;
            else if (!track.file.empty())
            {
                std::error_code ec;
                const auto size = fs::file_size(track.file, ec);
                track.end = ec ? 0 : (long long)size;
            }
        }
    }

    std::filesystem::path m_path;
    int m_sectorSize = 2352;
    int m_dataOffset = 16;
    std::map<std::string, Entry> m_files;   // "INDARK/000123.VOC"
    std::vector<CdTrack> m_tracks;
};
