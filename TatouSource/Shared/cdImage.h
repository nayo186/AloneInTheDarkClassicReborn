///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// A CD image, only read: an .iso (2048-byte sectors) or a .bin / GOG's
// GAME.GOG (raw 2352-byte sectors, the kind is found from the image itself).
// The files of the first level of folders of the CD (ISO 9660, e.g.
// INDARK/000123.VOC) and, when a track list (.cue, GOG's GAME.INS) is given,
// its tracks (the music). Used by the game (GOG installations, narrators'
// voices) and by the launcher (finding narrators). Header-only.
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
#include <vector>

struct CdTrack
{
    int number = 0;
    bool audio = false;
    long long startSector = 0;
    long long endSector = 0;   // first sector after the track
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
        // Raw sectors first (sync and header before the data), then plain ones
        m_sectorSize = 2352;
        m_dataOffset = 16;
        if (!readSector(file, 16, descriptor) || memcmp(descriptor + 1, "CD001", 5) != 0)
        {
            m_sectorSize = kDataSize;
            m_dataOffset = 0;
            if (!readSector(file, 16, descriptor) || memcmp(descriptor + 1, "CD001", 5) != 0)
                return false;
        }
        file.clear();
        file.seekg(0, std::ios::end);
        const long long sectors = (long long)file.tellg() / m_sectorSize;

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
            readTrackList(cue, sectors);
        return !m_files.empty();
    }

    const std::filesystem::path& path() const { return m_path; }
    int fileCount() const { return (int)m_files.size(); }
    const std::vector<CdTrack>& tracks() const { return m_tracks; }
    // Bytes of one raw sector (2352) or 2048 for an .iso: where a track starts
    int sectorSize() const { return m_sectorSize; }

    bool hasFile(const std::string& folder, const std::string& name) const
    {
        return m_files.count(upper(folder) + "/" + upper(name)) != 0;
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

    // "TRACK 02 AUDIO" then "INDEX 01 05:31:19" (minutes, seconds, frames)
    void readTrackList(const std::filesystem::path& cue, long long imageSectors)
    {
        std::ifstream list(cue);
        std::string line;
        while (std::getline(list, line))
        {
            std::istringstream words(line);
            std::string word;
            words >> word;
            if (word == "TRACK")
            {
                CdTrack track;
                std::string type;
                words >> track.number >> type;
                track.audio = (upper(type) == "AUDIO");
                m_tracks.push_back(track);
            }
            else if (word == "INDEX" && !m_tracks.empty())
            {
                int index = 0, minutes = 0, seconds = 0, frames = 0;
                std::string time;
                words >> index >> time;
                if (index == 1 && sscanf(time.c_str(), "%d:%d:%d", &minutes, &seconds, &frames) == 3)
                    m_tracks.back().startSector = ((long long)minutes * 60 + seconds) * 75 + frames;
            }
        }
        for (size_t i = 0; i < m_tracks.size(); i++)
            m_tracks[i].endSector = (i + 1 < m_tracks.size()) ? m_tracks[i + 1].startSector : imageSectors;
    }

    std::filesystem::path m_path;
    int m_sectorSize = 2352;
    int m_dataOffset = 16;
    std::map<std::string, Entry> m_files;   // "INDARK/000123.VOC"
    std::vector<CdTrack> m_tracks;
};
