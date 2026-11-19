///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// A file that gathers others one after the other: a ZIP, read without
// extracting anything. Its index (the central directory, at the end) tells
// where each file starts; a stored file (no compression) is read by copying
// its bytes from there. Used by texture packs (.texture, FitdLib/remaster/
// graphics/texturePack.h) and narrator packs (.voice, narrators.h). Files
// compressed with deflate are listed with their method: whoever reads them
// inflates them (the game does; the launcher only needs the index). No
// ZIP64 (files under 4 GB). Thread-safe once open: each read opens the file
// on its own. Used by the game and the launcher. Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

class ZipArchive
{
public:
    struct Entry
    {
        uint64_t headerOffset = 0;   // of its local header
        uint32_t method = 0;         // 0 stored, 8 deflated
        uint32_t packedSize = 0;
        uint32_t size = 0;
    };

    // Reads the index; false if it is not a ZIP
    bool open(const std::filesystem::path& path)
    {
        m_path = path;
        m_entries.clear();
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        file.seekg(0, std::ios::end);
        const uint64_t length = (uint64_t)file.tellg();
        if (length < 22)
            return false;

        // The end of the index, within the last 64 KB (its comment)
        const uint64_t tail = length < 65535 + 22 ? length : 65535 + 22;
        std::vector<unsigned char> end((size_t)tail);
        file.seekg((std::streamoff)(length - tail));
        file.read((char*)end.data(), (std::streamsize)tail);
        int found = -1;
        for (int i = (int)tail - 22; i >= 0; i--)
            if (get32(&end[i]) == 0x06054b50) { found = i; break; }
        if (found < 0)
            return false;
        const uint32_t count = get16(&end[found + 10]);
        const uint32_t indexSize = get32(&end[found + 12]);
        const uint32_t indexOffset = get32(&end[found + 16]);
        if ((uint64_t)indexOffset + indexSize > length)
            return false;

        std::vector<unsigned char> index(indexSize);
        file.seekg(indexOffset);
        file.read((char*)index.data(), indexSize);
        if (!file)
            return false;
        size_t pos = 0;
        for (uint32_t i = 0; i < count && pos + 46 <= index.size(); i++)
        {
            const unsigned char* item = &index[pos];
            if (get32(item) != 0x02014b50)
                return false;
            const uint16_t nameLength = get16(item + 28);
            const uint16_t extraLength = get16(item + 30);
            const uint16_t commentLength = get16(item + 32);
            if (pos + 46 + nameLength > index.size())
                return false;
            Entry entry;
            entry.method = get16(item + 10);
            entry.packedSize = get32(item + 20);
            entry.size = get32(item + 24);
            entry.headerOffset = get32(item + 42);
            const std::string name((const char*)item + 46, nameLength);
            if (!name.empty() && name.back() != '/')
                m_entries.emplace(key(name), entry);
            pos += 46 + nameLength + extraLength + commentLength;
        }
        return true;
    }

    const std::filesystem::path& path() const { return m_path; }
    // Every file, by lower-case name with '/' separators
    const std::map<std::string, Entry>& entries() const { return m_entries; }

    // The file with that name (any case), or null
    const Entry* find(const std::string& name) const
    {
        const auto found = m_entries.find(key(name));
        return found == m_entries.end() ? nullptr : &found->second;
    }

    // The bytes of a file as stored (compressed if its method is not 0)
    bool readStored(const Entry& entry, std::vector<unsigned char>& data) const
    {
        std::ifstream file(m_path, std::ios::binary);
        unsigned char header[30];
        file.seekg((std::streamoff)entry.headerOffset);
        file.read((char*)header, sizeof(header));
        if (!file || get32(header) != 0x04034b50)
            return false;
        file.seekg((std::streamoff)(entry.headerOffset + 30 + get16(header + 26) + get16(header + 28)));
        data.resize(entry.packedSize);
        file.read((char*)data.data(), entry.packedSize);
        return (bool)file;
    }

    // The bytes of a file kept without compression; false for a compressed one
    bool read(const std::string& name, std::vector<unsigned char>& data) const
    {
        const Entry* entry = find(name);
        return entry && entry->method == 0 && readStored(*entry, data);
    }

    static std::string key(std::string name)
    {
        for (char& c : name)
            c = (c == '\\') ? '/' : (char)tolower((unsigned char)c);
        return name;
    }

private:
    static uint16_t get16(const unsigned char* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
    static uint32_t get32(const unsigned char* p) { return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24)); }

    std::filesystem::path m_path;
    std::map<std::string, Entry> m_entries;
};
