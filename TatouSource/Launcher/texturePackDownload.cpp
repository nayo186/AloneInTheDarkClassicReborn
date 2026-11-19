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

// The pack published now: its file name (game and version), where it is and
// its exact size. A new version: change these three; the old file is removed
// when the new one arrives (every "nayo_AITD1*.texture")
static const char* const kPackName = "nayo_AITD1V2.texture";
static const char* const kPackPrefix = "nayo_AITD1";
static const char* const kPackUrl =
    "https://drive.usercontent.google.com/download?id=1Je5oqfYU6c91gWDQOp5P-aOBK3C4XUPW&export=download&confirm=t";
static const uint64_t kPackSize = 130644681;

static std::atomic<int> s_state{ -1 };   // -1: look at UserData
static std::atomic<bool> s_running{ false };

static fs::path userData()
{
    return fs::u8path(launcherFolder()) / kUserDataFolder;
}

static fs::path packPath()
{
    return userData() / kPackName;
}

static fs::path partPath()
{
    return userData() / (std::string(kPackName) + ".part");
}

// The file arrived whole: its size, and a ZIP (a page of the server would not be)
static bool wholePack(const fs::path& path)
{
    std::error_code error;
    if (fs::file_size(path, error) != kPackSize || error)
        return false;
    std::ifstream file(path, std::ios::binary);
    char magic[4] = {};
    file.read(magic, 4);
    return file && magic[0] == 'P' && magic[1] == 'K' && magic[2] == 3 && magic[3] == 4;
}

static void downloadThread()
{
    std::error_code error;
    fs::create_directories(userData(), error);
    fs::remove(partPath(), error);
    logLine("Texture pack: downloading %s", kPackName);
    const bool downloaded = launcherDownload(kPackUrl, partPath(), 3600);
    if (!downloaded || !wholePack(partPath()))
    {
        logLine("Texture pack: ERROR: the download failed or is not whole (%llu of %llu bytes)",
            (unsigned long long)fs::file_size(partPath(), error), (unsigned long long)kPackSize);
        fs::remove(partPath(), error);
        s_state = (int)PackState::Failed;
        s_running = false;
        return;
    }
    fs::rename(partPath(), packPath(), error);
    if (error)
    {
        logLine("Texture pack: ERROR: %s could not be renamed: %s", kPackName, error.message().c_str());
        fs::remove(partPath(), error);
        s_state = (int)PackState::Failed;
        s_running = false;
        return;
    }
    // Older versions of the pack
    for (fs::directory_iterator it(userData(), error), end; !error && it != end; it.increment(error))
    {
        const std::string name = it->path().filename().u8string();
        if (name != kPackName && name.rfind(kPackPrefix, 0) == 0 && it->path().extension() == ".texture")
        {
            std::error_code removeError;
            fs::remove(it->path(), removeError);
            logLine("Texture pack: older version %s removed", name.c_str());
        }
    }
    logLine("Texture pack: %s installed", kPackName);
    s_state = (int)PackState::Installed;
    s_running = false;
}

PackState texturePackState()
{
    if (s_running)
        return PackState::Downloading;
    if (s_state == (int)PackState::Failed)
        return PackState::Failed;
    std::error_code error;
    return fs::exists(packPath(), error) ? PackState::Installed : PackState::Missing;
}

int texturePackProgress()
{
    std::error_code error;
    const uint64_t size = fs::file_size(partPath(), error);
    if (error)
        return 0;
    return (int)((size * 100) / kPackSize);
}

int texturePackMegabytes()
{
    return (int)((kPackSize + 500000) / 1000000);
}

void texturePackStartDownload()
{
    bool expected = false;
    if (!s_running.compare_exchange_strong(expected, true))
        return;
    s_state = (int)PackState::Downloading;
    std::thread(downloadThread).detach();
}

void texturePackRemove()
{
    if (s_running)
        return;
    std::error_code error;
    fs::remove(packPath(), error);
    s_state = -1;
    logLine("Texture pack: %s removed", kPackName);
}

void texturePackShutdown()
{
    // curl (or the app's download) cannot be stopped halfway: the .part left
    // is removed by the next download
}
