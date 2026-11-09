///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The check of the game files in a thread (fileCheck.h)
///////////////////////////////////////////////////////////////////////////////

#include "fileCheck.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>

static std::thread s_thread;
static std::atomic<bool> s_stop{ false };
static std::atomic<bool> s_finished{ true };
static std::atomic<int> s_done{ 0 };
static std::atomic<int> s_total{ 0 };
static std::mutex s_mutex;              // guards the two below
static GameFilesResult s_result;
static bool s_accepted = false;
static std::string s_cacheFile;         // of the last check (fileCheckAccept)

void fileCheckStop()
{
    s_stop = true;
    if (s_thread.joinable())
        s_thread.join();
    s_finished = true;
}

void fileCheckStart(const std::vector<GameFilesSource>& sources, const std::string& cacheFile)
{
    fileCheckStop();
    s_stop = false;
    s_finished = false;
    s_done = 0;
    s_total = 0;
    s_cacheFile = cacheFile;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_result = GameFilesResult();
        s_accepted = false;
    }
    s_thread = std::thread([sources, cacheFile]
    {
        const std::filesystem::path cachePath = std::filesystem::u8path(cacheFile);
        GameFilesCache cache;
        cache.load(cachePath);
        // First the files of every source, so the progress knows the total
        std::vector<std::vector<std::string>> names;
        int total = 0;
        for (const GameFilesSource& source : sources)
        {
            names.push_back(gameFilesToCheck(source));
            total += (int)names.back().size();
        }
        s_total = total;
        GameFilesResult result;
        for (size_t i = 0; i < sources.size(); i++)
        {
            for (const std::string& name : names[i])
            {
                if (s_stop)
                    return;
                checkGameFile(sources[i], name, cache, result);
                s_done++;
            }
            findMissingGameFiles(sources[i], names[i], result);
        }
        cache.save(cachePath);
        const bool accepted = !cache.accepted.empty() && cache.accepted == gameFilesSignature(result);
        std::lock_guard<std::mutex> lock(s_mutex);
        s_result = result;
        s_accepted = accepted;
        s_finished = true;
    });
}

FileCheckState fileCheckState()
{
    FileCheckState state;
    state.finished = s_finished;
    state.done = s_done;
    state.total = s_total;
    if (state.finished)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        state.result = s_result;
        state.accepted = s_accepted;
    }
    return state;
}

void fileCheckAccept()
{
    if (!s_finished || s_cacheFile.empty())
        return;
    const std::filesystem::path cachePath = std::filesystem::u8path(s_cacheFile);
    GameFilesCache cache;
    cache.load(cachePath);
    std::lock_guard<std::mutex> lock(s_mutex);
    cache.accepted = gameFilesSignature(s_result);
    cache.save(cachePath);
    s_accepted = true;
}
