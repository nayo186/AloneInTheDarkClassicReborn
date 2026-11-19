///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The improved backgrounds' texture pack (texturePackDownload.h)
///////////////////////////////////////////////////////////////////////////////

#include "texturePackDownload.h"
#include "launcherSettings.h"
#include "updater.h"
#include "userFolder.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// The packs published: file name (game and version), where it is and its
// exact size. A new version: change these three; the old file is removed when
// the new one arrives (every file of the same prefix, "nayo_AITD1*.texture")
struct PackInfo
{
    const char* name;
    const char* prefix;
    const char* url;
    uint64_t size;
};

static const PackInfo kPacks[] = {
    { "nayo_AITD1V2.texture", "nayo_AITD1",
      "https://drive.usercontent.google.com/download?id=1Je5oqfYU6c91gWDQOp5P-aOBK3C4XUPW&export=download&confirm=t",
      130644681 },
    { "nayo_AITD2V2.texture", "nayo_AITD2",
      "https://drive.usercontent.google.com/download?id=1_8RuWXrnhpJeeXw6XhWE4HvghVTtQjcN&export=download&confirm=t",
      234324370 },
};
static const int kPackCount = (int)(sizeof(kPacks) / sizeof(kPacks[0]));

static std::atomic<int> s_state[kPackCount] = { { -1 }, { -1 } };   // -1: look at UserData
static std::atomic<bool> s_running[kPackCount] = { { false }, { false } };

static const PackInfo& info(TexturePack pack)
{
    return kPacks[(int)pack];
}

static fs::path userData()
{
    return fs::u8path(launcherFolder()) / kUserDataFolder;
}

static fs::path packPath(TexturePack pack)
{
    return userData() / info(pack).name;
}

static fs::path partPath(TexturePack pack)
{
    return userData() / (std::string(info(pack).name) + ".part");
}

// The file arrived whole: its size, and a ZIP (a page of the server would not be)
static bool wholePack(TexturePack pack, const fs::path& path)
{
    std::error_code error;
    if (fs::file_size(path, error) != info(pack).size || error)
        return false;
    std::ifstream file(path, std::ios::binary);
    char magic[4] = {};
    file.read(magic, 4);
    return file && magic[0] == 'P' && magic[1] == 'K' && magic[2] == 3 && magic[3] == 4;
}

static void finish(TexturePack pack, PackState state)
{
    s_state[(int)pack] = (int)state;
    s_running[(int)pack] = false;
}

static void downloadThread(TexturePack pack)
{
    const PackInfo& p = info(pack);
    std::error_code error;
    fs::create_directories(userData(), error);
    fs::remove(partPath(pack), error);
    logLine("Texture pack: downloading %s", p.name);
    const bool downloaded = launcherDownload(p.url, partPath(pack), 3600);
    if (!downloaded || !wholePack(pack, partPath(pack)))
    {
        logLine("Texture pack: ERROR: %s failed or is not whole (%llu of %llu bytes)", p.name,
            (unsigned long long)fs::file_size(partPath(pack), error), (unsigned long long)p.size);
        fs::remove(partPath(pack), error);
        finish(pack, PackState::Failed);
        return;
    }
    fs::rename(partPath(pack), packPath(pack), error);
    if (error)
    {
        logLine("Texture pack: ERROR: %s could not be renamed: %s", p.name, error.message().c_str());
        fs::remove(partPath(pack), error);
        finish(pack, PackState::Failed);
        return;
    }
    // Older versions of the pack
    for (fs::directory_iterator it(userData(), error), end; !error && it != end; it.increment(error))
    {
        const std::string name = it->path().filename().u8string();
        if (name != p.name && name.rfind(p.prefix, 0) == 0 && it->path().extension() == ".texture")
        {
            std::error_code removeError;
            fs::remove(it->path(), removeError);
            logLine("Texture pack: older version %s removed", name.c_str());
        }
    }
    logLine("Texture pack: %s installed", p.name);
    finish(pack, PackState::Installed);
}

bool texturePackForGame(int selectedGame, TexturePack* pack)
{
    if (selectedGame < 0 || selectedGame >= kPackCount)
        return false;
    *pack = (TexturePack)selectedGame;
    return true;
}

PackState texturePackState(TexturePack pack)
{
    if (s_running[(int)pack])
        return PackState::Downloading;
    if (s_state[(int)pack] == (int)PackState::Failed)
        return PackState::Failed;
    std::error_code error;
    return fs::exists(packPath(pack), error) ? PackState::Installed : PackState::Missing;
}

int texturePackProgress(TexturePack pack)
{
    std::error_code error;
    const uint64_t size = fs::file_size(partPath(pack), error);
    if (error)
        return 0;
    return (int)((size * 100) / info(pack).size);
}

int texturePackMegabytes(TexturePack pack)
{
    return (int)((info(pack).size + 500000) / 1000000);
}

void texturePackStartDownload(TexturePack pack)
{
    bool expected = false;
    if (!s_running[(int)pack].compare_exchange_strong(expected, true))
        return;
    s_state[(int)pack] = (int)PackState::Downloading;
    std::thread(downloadThread, pack).detach();
}

void texturePackRemove(TexturePack pack)
{
    if (s_running[(int)pack])
        return;
    std::error_code error;
    fs::remove(packPath(pack), error);
    s_state[(int)pack] = -1;
    logLine("Texture pack: %s removed", info(pack).name);
}

void texturePackShutdown()
{
    // curl (or the app's download) cannot be stopped halfway: the .part left
    // is removed by the next download
}
