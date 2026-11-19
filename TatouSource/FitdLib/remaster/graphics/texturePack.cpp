///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Texture packs (texturePack.h)
///////////////////////////////////////////////////////////////////////////////

#include "texturePack.h"
#include "common.h"
#include "consoleLog.h"
#include "gameFolder.h"

#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>

namespace fs = std::filesystem;

namespace
{

// One file inside a pack
struct PackEntry
{
    size_t pack = 0;          // index in s_packs
    uint64_t headerOffset = 0;// of its local header
    uint32_t method = 0;      // 0 stored, 8 deflated
    uint32_t packedSize = 0;
    uint32_t size = 0;
};

struct Pack
{
    fs::path path;
    fs::file_time_type time;
};

std::once_flag s_opened;
std::vector<Pack> s_packs;
std::map<std::string, PackEntry> s_entries;   // by lower-case name, '/' separators

std::string entryKey(std::string name)
{
    for (char& c : name)
        c = (c == '\\') ? '/' : (char)tolower((unsigned char)c);
    return name;
}

uint16_t get16(const unsigned char* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
uint32_t get32(const unsigned char* p) { return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24)); }

// Reads the central directory of one ZIP; its files are added unless an
// earlier pack has them (packs are read in name order)
bool readPack(const fs::path& path, size_t packIndex, int& files)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.seekg(0, std::ios::end);
    const uint64_t length = (uint64_t)file.tellg();
    if (length < 22)
        return false;

    // The end of central directory record, within the last 64 KB (its comment)
    const uint64_t tail = (std::min)(length, (uint64_t)(65535 + 22));
    std::vector<unsigned char> end((size_t)tail);
    file.seekg((std::streamoff)(length - tail));
    file.read((char*)end.data(), (std::streamsize)tail);
    int found = -1;
    for (int i = (int)tail - 22; i >= 0; i--)
        if (get32(&end[i]) == 0x06054b50) { found = i; break; }
    if (found < 0)
        return false;
    const uint32_t count = get16(&end[found + 10]);
    const uint32_t directorySize = get32(&end[found + 12]);
    const uint32_t directoryOffset = get32(&end[found + 16]);
    if ((uint64_t)directoryOffset + directorySize > length)
        return false;

    std::vector<unsigned char> directory(directorySize);
    file.seekg(directoryOffset);
    file.read((char*)directory.data(), directorySize);
    if (!file)
        return false;
    size_t pos = 0;
    for (uint32_t i = 0; i < count && pos + 46 <= directory.size(); i++)
    {
        const unsigned char* entry = &directory[pos];
        if (get32(entry) != 0x02014b50)
            return false;
        const uint16_t nameLength = get16(entry + 28);
        const uint16_t extraLength = get16(entry + 30);
        const uint16_t commentLength = get16(entry + 32);
        if (pos + 46 + nameLength > directory.size())
            return false;
        PackEntry packEntry;
        packEntry.pack = packIndex;
        packEntry.method = get16(entry + 10);
        packEntry.packedSize = get32(entry + 20);
        packEntry.size = get32(entry + 24);
        packEntry.headerOffset = get32(entry + 42);
        const std::string name((const char*)entry + 46, nameLength);
        if (!name.empty() && name.back() != '/' && (packEntry.method == 0 || packEntry.method == 8))
        {
            if (s_entries.emplace(entryKey(name), packEntry).second)
                files++;
        }
        pos += 46 + nameLength + extraLength + commentLength;
    }
    return true;
}

// The .texture files of UserData, once (a pack added while playing is seen
// the next time the game opens)
void openPacks()
{
    std::error_code error;
    const fs::path folder = userDataFolder();
    std::vector<fs::path> paths;
    for (fs::directory_iterator it(folder, error), end; !error && it != end; it.increment(error))
    {
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)tolower(c); });
        if (extension == ".texture" && it->is_regular_file(error))
            paths.push_back(it->path());
    }
    std::sort(paths.begin(), paths.end());
    for (const fs::path& path : paths)
    {
        int files = 0;
        const size_t index = s_packs.size();
        if (!readPack(path, index, files))
        {
            printf(MAIN_WARN "Texture pack %s: not a ZIP file, not used" CON_RESET "\n", path.filename().string().c_str());
            continue;
        }
        s_packs.push_back({ path, fs::last_write_time(path, error) });
        printf(MAIN_OK "Texture pack %s: %d pictures and masks" CON_RESET "\n", path.filename().string().c_str(), files);
    }
}

} // namespace

const char* texturePackGameFolder()
{
    switch (g_gameId)
    {
    case JACK: return "JACK";
    case AITD2: return "AITD2";
    case AITD3: return "AITD3";
    case TIMEGATE: return "TIMEGATE";
    default: return "AITD1";
    }
}

bool texturePackHas(const std::string& name, fs::file_time_type* time)
{
    std::call_once(s_opened, openPacks);
    const auto found = s_entries.find(entryKey(name));
    if (found == s_entries.end())
        return false;
    if (time)
        *time = s_packs[found->second.pack].time;
    return true;
}

bool texturePackRead(const std::string& name, std::vector<unsigned char>& data)
{
    std::call_once(s_opened, openPacks);
    const auto found = s_entries.find(entryKey(name));
    if (found == s_entries.end())
        return false;
    const PackEntry& entry = found->second;

    // Each read opens the pack on its own: several job threads read at once
    std::ifstream file(s_packs[entry.pack].path, std::ios::binary);
    unsigned char header[30];
    file.seekg((std::streamoff)entry.headerOffset);
    file.read((char*)header, sizeof(header));
    if (!file || get32(header) != 0x04034b50)
        return false;
    file.seekg((std::streamoff)(entry.headerOffset + 30 + get16(header + 26) + get16(header + 28)));

    std::vector<unsigned char> packed(entry.packedSize);
    file.read((char*)packed.data(), entry.packedSize);
    if (!file)
        return false;
    if (entry.method == 0)
    {
        data.swap(packed);
        return true;
    }

    // Deflated (a ZIP made with any tool)
    data.resize(entry.size);
    z_stream stream{};
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        return false;
    stream.next_in = packed.data();
    stream.avail_in = (uInt)packed.size();
    stream.next_out = data.data();
    stream.avail_out = (uInt)data.size();
    const int result = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    return result == Z_STREAM_END;
}
