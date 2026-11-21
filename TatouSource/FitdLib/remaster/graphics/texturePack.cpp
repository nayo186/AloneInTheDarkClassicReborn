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
#include "zipArchive.h"

#include <zlib.h>

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>

namespace fs = std::filesystem;

namespace
{

struct Pack
{
    ZipArchive archive;
    fs::file_time_type time;
};

// Where each file is: its pack and its place in it (the first pack that has it)
struct Found
{
    const Pack* pack = nullptr;
    const ZipArchive::Entry* entry = nullptr;
};

std::once_flag s_opened;
std::vector<std::unique_ptr<Pack>> s_packs;
std::map<std::string, Found> s_files;   // by ZipArchive::key

// The .texture files of UserData, once (a pack added while playing is seen
// the next time the game opens)
void openPacks()
{
    std::error_code error;
    std::vector<fs::path> paths;
    for (fs::directory_iterator it(userDataFolder(), error), end; !error && it != end; it.increment(error))
    {
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)tolower(c); });
        if (extension == ".texture" && it->is_regular_file(error))
            paths.push_back(it->path());
    }
    std::sort(paths.begin(), paths.end());
    for (const fs::path& path : paths)
    {
        auto pack = std::make_unique<Pack>();
        if (!pack->archive.open(path))
        {
            printf(MAIN_WARN "Texture pack %s: not a ZIP file, not used" CON_RESET "\n", path.filename().string().c_str());
            continue;
        }
        pack->time = fs::last_write_time(path, error);
        int files = 0;
        for (const auto& [name, entry] : pack->archive.entries())
            if ((entry.method == 0 || entry.method == 8) && s_files.emplace(name, Found{ pack.get(), &entry }).second)
                files++;
        printf(MAIN_OK "Texture pack %s: %d pictures and masks" CON_RESET "\n", path.filename().string().c_str(), files);
        s_packs.push_back(std::move(pack));
    }
}

const Found* find(const std::string& name)
{
    std::call_once(s_opened, openPacks);
    const auto found = s_files.find(ZipArchive::key(name));
    return found == s_files.end() ? nullptr : &found->second;
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

void texturePackList(const std::string& prefix, std::vector<std::string>& names)
{
    std::call_once(s_opened, openPacks);
    const std::string key = ZipArchive::key(prefix);
    for (auto it = s_files.lower_bound(key); it != s_files.end() && it->first.compare(0, key.size(), key) == 0; ++it)
        names.push_back(it->first);
}

bool texturePackHas(const std::string& name, fs::file_time_type* time)
{
    const Found* found = find(name);
    if (!found)
        return false;
    if (time)
        *time = found->pack->time;
    return true;
}

bool texturePackRead(const std::string& name, std::vector<unsigned char>& data)
{
    const Found* found = find(name);
    if (!found)
        return false;
    const ZipArchive::Entry& entry = *found->entry;
    if (entry.method == 0)
        return found->pack->archive.readStored(entry, data);

    // Deflated (a ZIP made with any tool)
    std::vector<unsigned char> packed;
    if (!found->pack->archive.readStored(entry, packed))
        return false;
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
