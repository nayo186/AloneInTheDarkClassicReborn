///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Updates from GitHub Releases (see updater.h)
///////////////////////////////////////////////////////////////////////////////

#include "updater.h"
#include "launcherSettings.h"
#include "userFolder.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;

#ifndef CLASSIC_REBORN_VERSION
#define CLASSIC_REBORN_VERSION "dev"
#endif

static const char* const kLatestReleaseUrl = "https://api.github.com/repos/nayo186/AloneInTheDarkClassicReborn/releases/latest";
static const char* const kDownloadUrl = "https://github.com/nayo186/AloneInTheDarkClassicReborn/releases/download/";
static const char* const kUpdateFolder = "Update";

#ifdef _WIN32
static const char* const kPackageEnd = "-windows-x64.zip";
static const char* const kLauncherFile = "Launcher.exe";
static const char* const kGameFile = "Tatou.exe";
#else
static const char* const kPackageEnd = "-linux-x64.tar.gz";
static const char* const kLauncherFile = "Launcher";
static const char* const kGameFile = "Tatou";
#endif

static std::atomic<int> s_state{ (int)UpdateState::Idle };
static std::atomic<bool> s_replacing{ false };
static std::mutex s_mutex;
static std::string s_latestVersion;   // under s_mutex

const char* launcherVersion()
{
    return CLASSIC_REBORN_VERSION;
}

bool updatesEnabled()
{
    return strcmp(CLASSIC_REBORN_VERSION, "dev") != 0;
}

UpdateState updaterState()
{
    return (UpdateState)s_state.load();
}

std::string updaterLatestVersion()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_latestVersion;
}

static fs::path updateFolder()
{
    return fs::u8path(launcherFolder()) / kUpdateFolder;
}

void updaterCleanup()
{
    std::error_code ec;
    if (fs::exists(updateFolder(), ec))
    {
        fs::remove_all(updateFolder(), ec);
        logLine("Update: leftovers of the last update %s", ec ? "could not be removed" : "removed");
    }
}

// ── Versions: "1.0.0-beta.7" < "1.0.0-beta.8" < "1.0.0" < "1.0.1" ─────────
static std::vector<std::string> splitDots(const std::string& text)
{
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, '.'))
        parts.push_back(part);
    return parts;
}

static bool isNumber(const std::string& text)
{
    if (text.empty())
        return false;
    for (char c : text)
        if (!isdigit((unsigned char)c))
            return false;
    return true;
}

// <0, 0 or >0 as a is older, the same or newer than b; parts by dots,
// numbers as numbers, words as words
static int compareParts(const std::string& a, const std::string& b)
{
    const std::vector<std::string> partsA = splitDots(a), partsB = splitDots(b);
    for (size_t i = 0; i < partsA.size() && i < partsB.size(); i++)
    {
        if (isNumber(partsA[i]) && isNumber(partsB[i]))
        {
            const long numberA = atol(partsA[i].c_str()), numberB = atol(partsB[i].c_str());
            if (numberA != numberB)
                return numberA < numberB ? -1 : 1;
        }
        else if (partsA[i] != partsB[i])
            return partsA[i] < partsB[i] ? -1 : 1;
    }
    return (int)partsA.size() - (int)partsB.size();
}

// A version without "-beta.N" is newer than its betas
static int compareVersions(const std::string& a, const std::string& b)
{
    const size_t dashA = a.find('-'), dashB = b.find('-');
    const int numbers = compareParts(a.substr(0, dashA), b.substr(0, dashB));
    if (numbers != 0)
        return numbers;
    if (dashA == std::string::npos || dashB == std::string::npos)
        return (dashA == std::string::npos ? 1 : 0) - (dashB == std::string::npos ? 1 : 0);
    return compareParts(a.substr(dashA + 1), b.substr(dashB + 1));
}

// ── Running curl and tar without a window ───────────────────────────────────
static int runTool(const char* tool, const std::vector<std::string>& args)
{
#ifdef _WIN32
    // From System32, never a curl.exe or tar.exe placed next to the launcher
    wchar_t system[MAX_PATH] = {};
    GetSystemDirectoryW(system, MAX_PATH);
    const std::wstring program = std::wstring(system) + L"\\" + fs::u8path(tool).wstring() + L".exe";
    std::wstring commandLine = L"\"" + program + L"\"";
    for (const std::string& arg : args)
        commandLine += L" \"" + fs::u8path(arg).wstring() + L"\"";

    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(program.c_str(), &commandLine[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
        &startup, &process))
    {
        logLine("Update: ERROR: %s could not be started (error %lu)", tool, GetLastError());
        return -1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return (int)exitCode;
#else
    std::vector<const char*> argv = { tool };
    for (const std::string& arg : args)
        argv.push_back(arg.c_str());
    argv.push_back(nullptr);
    SDL_Process* process = SDL_CreateProcess(argv.data(), false);
    if (!process)
    {
        logLine("Update: ERROR: %s could not be started: %s", tool, SDL_GetError());
        return -1;
    }
    int exitCode = 1;
    SDL_WaitProcess(process, true, &exitCode);
    SDL_DestroyProcess(process);
    return exitCode;
#endif
}

static bool download(const std::string& url, const fs::path& file, int seconds)
{
    const int result = runTool("curl", { "-fsSL", "--max-time", std::to_string(seconds), "-o", file.u8string(), url });
    if (result != 0)
        logLine("Update: ERROR: download of %s failed (curl %d)", url.c_str(), result);
    return result == 0;
}

static std::string readFile(const fs::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    std::stringstream text;
    text << stream.rdbuf();
    return text.str();
}

// The string value of "key" in GitHub's answer (enough for "tag_name")
static std::string jsonString(const std::string& json, const char* key)
{
    const std::string quoted = std::string("\"") + key + "\"";
    size_t at = json.find(quoted);
    if (at == std::string::npos)
        return "";
    at = json.find(':', at + quoted.size());
    at = (at == std::string::npos) ? at : json.find('"', at);
    if (at == std::string::npos)
        return "";
    const size_t end = json.find('"', at + 1);
    return end == std::string::npos ? "" : json.substr(at + 1, end - at - 1);
}

static std::string packageName(const std::string& version)
{
    return "ClassicReborn-" + version + kPackageEnd;
}

// ── Check ───────────────────────────────────────────────────────────────────
static void checkThread()
{
    std::error_code ec;
    fs::create_directories(updateFolder(), ec);
    const fs::path answer = updateFolder() / "latest.json";
    logLine("Update: looking for a newer version than %s", launcherVersion());
    if (!download(kLatestReleaseUrl, answer, 30))
    {
        s_state = (int)UpdateState::CheckFailed;
        return;
    }
    const std::string json = readFile(answer);
    fs::remove(answer, ec);
    fs::remove(updateFolder(), ec);   // only if empty
    std::string tag = jsonString(json, "tag_name");
    const std::string version = (!tag.empty() && tag[0] == 'v') ? tag.substr(1) : tag;
    if (version.empty())
    {
        logLine("Update: ERROR: GitHub's answer has no tag_name");
        s_state = (int)UpdateState::CheckFailed;
        return;
    }
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_latestVersion = version;
    }
    // A release is published some minutes before its packages are built
    const bool newer = compareVersions(version, launcherVersion()) > 0;
    const bool packaged = json.find(packageName(version)) != std::string::npos;
    logLine("Update: latest release %s (%s%s)", tag.c_str(), newer ? "newer" : "not newer",
        newer && !packaged ? ", its package is not ready yet" : "");
    s_state = (int)(newer && packaged ? UpdateState::Available : UpdateState::UpToDate);
}

void updaterStartCheck()
{
    const UpdateState state = updaterState();
    if (!updatesEnabled() || state == UpdateState::Checking || state == UpdateState::Installing ||
        state == UpdateState::Installed)
        return;
    s_state = (int)UpdateState::Checking;
    std::thread(checkThread).detach();
}

// ── Install ─────────────────────────────────────────────────────────────────
// The unpacked folder that holds the programs: the package itself (zip) or
// its only folder (tar.gz)
static fs::path findPackageRoot(const fs::path& unpacked)
{
    std::error_code ec;
    if (fs::exists(unpacked / kLauncherFile, ec) && fs::exists(unpacked / kGameFile, ec))
        return unpacked;
    for (const fs::directory_entry& entry : fs::directory_iterator(unpacked, ec))
        if (entry.is_directory(ec) && fs::exists(entry.path() / kLauncherFile, ec) && fs::exists(entry.path() / kGameFile, ec))
            return entry.path();
    return fs::path();
}

// The package's files (never the user's: launcher.cfg, UserData) take the
// place of the old ones, which go to Update/old; all or nothing
static bool replaceFiles(const fs::path& root, const fs::path& oldFolder)
{
    const fs::path target = fs::u8path(launcherFolder());
    std::error_code ec;
    fs::create_directories(oldFolder, ec);
    std::vector<std::pair<fs::path, bool>> done;   // file name, an old one was moved
    bool ok = true;
    for (const fs::directory_entry& entry : fs::directory_iterator(root, ec))
    {
        const fs::path name = entry.path().filename();
        if (!entry.is_regular_file(ec) || name == "launcher.cfg" || name == kUserDataFolder)
            continue;
        const bool hadOld = fs::exists(target / name, ec);
        if (hadOld)
        {
            fs::rename(target / name, oldFolder / name, ec);
            if (ec)
            {
                logLine("Update: ERROR: %s could not be moved aside: %s", name.u8string().c_str(), ec.message().c_str());
                ok = false;
                break;
            }
        }
        fs::rename(entry.path(), target / name, ec);
        if (ec)
        {
            logLine("Update: ERROR: %s could not be replaced: %s", name.u8string().c_str(), ec.message().c_str());
            if (hadOld)
                fs::rename(oldFolder / name, target / name, ec);
            ok = false;
            break;
        }
        done.push_back({ name, hadOld });
        logLine("Update: %s replaced", name.u8string().c_str());
    }
    if (ok)
        return true;

    // Back as it was
    for (const auto& [name, hadOld] : done)
    {
        fs::remove(target / name, ec);
        if (hadOld)
            fs::rename(oldFolder / name, target / name, ec);
        logLine("Update: %s %s", name.u8string().c_str(), ec ? "COULD NOT BE RESTORED" : "restored");
    }
    return false;
}

static void installThread(std::string version)
{
    const fs::path folder = updateFolder();
    const fs::path package = folder / packageName(version);
    const fs::path unpacked = folder / "new";
    std::error_code ec;
    fs::remove_all(unpacked, ec);
    fs::create_directories(unpacked, ec);
    logLine("Update: installing %s", version.c_str());

    bool ok = download(kDownloadUrl + std::string("v") + version + "/" + packageName(version), package, 600);
    if (ok)
    {
        const int result = runTool("tar", { "-xf", package.u8string(), "-C", unpacked.u8string() });
        ok = (result == 0);
        if (!ok)
            logLine("Update: ERROR: the package could not be unpacked (tar %d)", result);
    }
    fs::path root;
    if (ok)
    {
        root = findPackageRoot(unpacked);
        ok = !root.empty();
        if (!ok)
            logLine("Update: ERROR: the package has no %s and %s", kLauncherFile, kGameFile);
    }
    if (ok)
    {
        s_replacing = true;
        ok = replaceFiles(root, folder / "old");
        s_replacing = false;
    }
    // The old files stay in Update/old until the next start (the running
    // launcher is one of them)
    fs::remove(package, ec);
    fs::remove_all(unpacked, ec);
    logLine("Update: %s", ok ? "done, the launcher must be opened again" : "failed, nothing was changed");
    s_state = (int)(ok ? UpdateState::Installed : UpdateState::InstallFailed);
}

void updaterStartInstall()
{
    if (updaterState() != UpdateState::Available && updaterState() != UpdateState::InstallFailed)
        return;
    s_state = (int)UpdateState::Installing;
    std::thread(installThread, updaterLatestVersion()).detach();
}

void updaterShutdown()
{
    while (s_replacing)
        SDL_Delay(10);
}
