///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted - Launcher
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Launcher settings, game config edits and game start
///////////////////////////////////////////////////////////////////////////////

#include "launcherSettings.h"
#include "userFolder.h"
#include "narrators.h"

#include <SDL3/SDL.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

static FILE* s_log = nullptr;

static fs::path toPath(const std::string& utf8)
{
    return fs::u8path(utf8);
}

static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string unquote(const std::string& s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

// Splits "key = value" (value may be quoted); returns false for other lines
static bool parseLine(const std::string& line, std::string& key, std::string& value)
{
    std::string t = trim(line);
    if (t.empty() || t[0] == '#')
        return false;
    size_t eq = t.find('=');
    if (eq == std::string::npos)
        return false;
    key = trim(t.substr(0, eq));
    value = unquote(trim(t.substr(eq + 1)));
    return true;
}

std::string launcherFolder()
{
    const char* base = SDL_GetBasePath();
    return base ? std::string(base) : std::string();
}

void logOpen()
{
    fs::path path = toPath(launcherFolder() + "launcher.log");
#ifdef _WIN32
    s_log = _wfopen(path.wstring().c_str(), L"w");
#else
    s_log = fopen(path.string().c_str(), "w");
#endif
    time_t now = time(nullptr);
    char stamp[64];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    logLine("=== Launcher started %s", stamp);
}

void logLine(const char* format, ...)
{
    if (!s_log)
        return;
    va_list args;
    va_start(args, format);
    vfprintf(s_log, format, args);
    va_end(args);
    fputc('\n', s_log);
    fflush(s_log);
}

static fs::path launcherConfigPath()
{
    return toPath(launcherFolder() + "launcher.cfg");
}

void loadLauncherSettings(LauncherSettings& settings)
{
    std::ifstream file(launcherConfigPath());
    if (!file)
    {
        logLine("launcher.cfg not found, using defaults");
        return;
    }
    std::string line, key, value;
    while (std::getline(file, line))
    {
        if (!parseLine(line, key, value))
            continue;
        if (key == "ui.language")
            settings.uiLanguage = value;
        else if (key == "aitd1.dataFolder")
            settings.aitd1DataFolder = value;
        else if (key == "debugMode")
            settings.debugMode = (value == "true" || value == "1");
        else if (key == "window.width")
            settings.windowWidth = atoi(value.c_str());
        else if (key == "window.height")
            settings.windowHeight = atoi(value.c_str());
        else if (key == "window.maximized")
            settings.windowMaximized = (value == "true" || value == "1");
    }
    logLine("Loaded launcher.cfg: ui.language=%s aitd1.dataFolder=%s",
        settings.uiLanguage.c_str(), settings.aitd1DataFolder.c_str());
}

void saveLauncherSettings(const LauncherSettings& settings)
{
    std::ofstream file(launcherConfigPath());
    if (!file)
    {
        logLine("ERROR: could not write launcher.cfg");
        return;
    }
    file << "# Alone in the Dark: Classic Reborn - launcher settings\n";
    file << "ui.language = " << settings.uiLanguage << "\n";
    file << "aitd1.dataFolder = \"" << settings.aitd1DataFolder << "\"\n";
    // Only changed by hand (0 or 1): no box in the window
    file << "debugMode = " << (settings.debugMode ? 1 : 0) << "\n";
    // The window as it was left
    file << "window.width = " << settings.windowWidth << "\n";
    file << "window.height = " << settings.windowHeight << "\n";
    file << "window.maximized = " << (settings.windowMaximized ? 1 : 0) << "\n";
}

// Where each part of the chosen folder is (see launcherSettings.h)
struct GameInstall
{
    bool valid = false;
    bool gog = false;
    fs::path root;          // the folder Tatou starts in
    fs::path aitd1Data;     // LISTBOD2.PAK and the language PAKs
    fs::path jackData;      // PERE.PAK
    fs::path configFolder;  // aitd_remaster.cfg
};

static bool pathExists(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec);
}

// GOG (GAME.INS) or Steam (GAME.INST): the same edition and CD image
static bool isGogRoot(const fs::path& folder)
{
    return pathExists(folder / "GAME.GOG") && (pathExists(folder / "GAME.INS") || pathExists(folder / "GAME.INST")) &&
        pathExists(folder / "INDARK" / "LISTBOD2.PAK");
}

static bool s_debugMode = false;

void setLauncherDebugMode(bool debugMode)
{
    s_debugMode = debugMode;
}

static fs::path userDataFolder()
{
    return toPath(launcherFolder()) / kUserDataFolder;
}

static GameInstall findGameInstall(const std::string& folder)
{
    GameInstall install;
    if (folder.empty())
        return install;
    fs::path path = toPath(folder);
    if (!isGogRoot(path) && isGogRoot(path.parent_path()))
        path = path.parent_path();   // GOG's INDARK folder was chosen
    if (isGogRoot(path))
    {
        install.configFolder = userDataFolder();
        install.valid = install.gog = true;
        install.root = path;
        install.aitd1Data = path / "INDARK";
        install.jackData = path / "JACK";
        return install;
    }
    if (pathExists(path / "LISTBOD2.PAK"))
    {
        install.valid = true;
        install.root = install.aitd1Data = path;
        install.jackData = path / "JACK";
        // Debug mode keeps a copy's options inside it; otherwise UserData
        install.configFolder = s_debugMode ? path : userDataFolder();
    }
    return install;
}

bool isAitd1DataFolder(const std::string& folder)
{
    return findGameInstall(folder).valid;
}

bool isGogInstallFolder(const std::string& folder)
{
    return findGameInstall(folder).gog;
}

bool hasJackData(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    return install.valid && pathExists(install.jackData / "PERE.PAK");
}

std::vector<std::string> findAitd1Narrators(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    if (!install.valid)
        return {};
    const fs::path workFolder = (!install.gog && s_debugMode) ? install.aitd1Data : userDataFolder() / "AITD1";
    return findNarrators(workFolder / kVoicesFolder);
}

void prepareUserData()
{
    for (const char* game : { "AITD1", "JACK" })
    {
        const fs::path gameFolder = userDataFolder() / game;
        std::error_code ec;
        for (const char* name : { kSavesFolder, kPicturesFolder, kMasksFolder, kExportsFolder })
            fs::create_directories(gameFolder / name, ec);
    }
    std::error_code ec;
    fs::create_directories(userDataFolder() / "AITD1" / kVoicesFolder, ec);
    logLine("UserData ready: %s", userDataFolder().u8string().c_str());
}

std::vector<GameLanguage> findGameLanguages(const std::string& folder)
{
    static const GameLanguage all[] = {
        { "ENGLISH", "English" },
        { "FRANCAIS", "Français" },
        { "ITALIANO", "Italiano" },
        { "ESPAGNOL", "Español" },
        { "DEUTSCH", "Deutsch" },
    };
    std::vector<GameLanguage> found;
    const GameInstall install = findGameInstall(folder);
    if (!install.valid)
        return found;
    for (const GameLanguage& language : all)
    {
        if (pathExists(install.aitd1Data / (language.pakName + ".PAK")))
            found.push_back(language);
    }
    return found;
}

static bool isTrue(const std::string& value)
{
    return value == "true" || value == "1";
}

static const char* boolText(bool value)
{
    return value ? "true" : "false";
}

// The copy's folder, or the user folder for a GOG installation
static fs::path gameConfigPath(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    if (install.valid)
    {
        std::error_code ec;
        fs::create_directories(install.configFolder, ec);
        return install.configFolder / "aitd_remaster.cfg";
    }
    return toPath(folder) / "aitd_remaster.cfg";
}

void readGameConfig(const std::string& folder, LauncherSettings& settings)
{
    settings.gameLanguage.clear();
    settings.restoreDefaults();
    settings.restoreDefaultControls();
    settings.musicVolume = settings.soundVolume = kDefaultAudioVolume;
    settings.voices.clear();

    std::ifstream file(gameConfigPath(folder));
    if (!file)
    {
        logLine("No aitd_remaster.cfg in the data folder yet (default settings, first language)");
        return;
    }
    std::string line, key, value;
    while (std::getline(file, line))
    {
        if (!parseLine(line, key, value))
            continue;
        if (key == "gamedata.language")
            settings.gameLanguage = value;
        else if (key == "graphics.frameLimit")
            settings.frameLimit = atoi(value.c_str());
        else if (key == "graphics.vsync")
            settings.vsync = isTrue(value);
        else if (key == "graphics.aspect")
            settings.aspect = atoi(value.c_str());
        else if (key == "graphics.resolution")
            settings.resolution = atoi(value.c_str());
        else if (key == "graphics.fullscreen")
            settings.fullscreen = isTrue(value);
        else if (key == "postprocessing.filmGrain")
            settings.filmGrain = isTrue(value);
        else if (key == "postprocessing.bloom")
            settings.bloom = isTrue(value);
        else if (key == "postprocessing.ssao")
            settings.ssao = isTrue(value);
        else if (key == "postprocessing.antialiasing")
            settings.antialiasing = atoi(value.c_str());
        else if (key == "animation.poseSmoothing")
            settings.animationSmoothing = isTrue(value);
        else if (key == "graphics.customBackgrounds")
            settings.customBackgrounds = isTrue(value);
        else if (key == "graphics.enhancedText")
            settings.enhancedText = isTrue(value);
        else if (key == "audio.musicVolume")
            settings.musicVolume = std::clamp(atoi(value.c_str()), 0, 100);
        else if (key == "audio.soundVolume")
            settings.soundVolume = std::clamp(atoi(value.c_str()), 0, 100);
        else if (key == "audio.voices")
            settings.voices = value;
        else if (key == "controller.modernStick")
            settings.modernStick = isTrue(value);
        else if (key.compare(0, 9, "controls.") == 0)
        {
            for (int a = 0; a < kGameActionCount; a++)
                if (key.compare(9, std::string::npos, gameActionId((GameAction)a)) == 0)
                    settings.controls[a] = bindingsFromText(value);
        }
    }
    logLine("Game config: language=%s frameLimit=%d vsync=%d filmGrain=%d bloom=%d ssao=%d animationSmoothing=%d",
        settings.gameLanguage.empty() ? "(first available)" : settings.gameLanguage.c_str(),
        settings.frameLimit, settings.vsync, settings.filmGrain, settings.bloom, settings.ssao,
        settings.animationSmoothing);
}

bool writeGameConfig(const std::string& folder, const LauncherSettings& settings)
{
    // Keep every other line of the game's config as it is
    std::vector<std::string> lines;
    {
        std::ifstream in(gameConfigPath(folder));
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }
    }

    // The launcher always starts the default settings (the engine's Original
    // mode); the advanced options are written on top of them
    struct ConfigLine { std::string key; std::string text; bool found; };
    std::vector<ConfigLine> wanted = {
        { "gamedata.language", "gamedata.language = \"" + settings.gameLanguage + "\"", false },
        { "graphics.frameLimit", "graphics.frameLimit = " + std::to_string(settings.frameLimit), false },
        { "graphics.vsync", std::string("graphics.vsync = ") + boolText(settings.vsync), false },
        { "graphics.aspect", "graphics.aspect = " + std::to_string(settings.aspect), false },
        { "graphics.resolution", "graphics.resolution = " + std::to_string(settings.resolution), false },
        { "graphics.fullscreen", std::string("graphics.fullscreen = ") + boolText(settings.fullscreen), false },
        { "postprocessing.filmGrain", std::string("postprocessing.filmGrain = ") + boolText(settings.filmGrain), false },
        { "postprocessing.bloom", std::string("postprocessing.bloom = ") + boolText(settings.bloom), false },
        { "postprocessing.ssao", std::string("postprocessing.ssao = ") + boolText(settings.ssao), false },
        { "postprocessing.antialiasing", "postprocessing.antialiasing = " + std::to_string(settings.antialiasing), false },
        { "animation.poseSmoothing", std::string("animation.poseSmoothing = ") + boolText(settings.animationSmoothing), false },
        { "graphics.customBackgrounds", std::string("graphics.customBackgrounds = ") + boolText(settings.customBackgrounds), false },
        { "graphics.enhancedText", std::string("graphics.enhancedText = ") + boolText(settings.enhancedText), false },
    };
    wanted.push_back({ "audio.musicVolume", "audio.musicVolume = " + std::to_string(settings.musicVolume), false });
    wanted.push_back({ "audio.soundVolume", "audio.soundVolume = " + std::to_string(settings.soundVolume), false });
    wanted.push_back({ "audio.voices", "audio.voices = \"" + settings.voices + "\"", false });
    wanted.push_back({ "controller.modernStick", std::string("controller.modernStick = ") + boolText(settings.modernStick), false });
    if (settings.controls.size() == (size_t)kGameActionCount)
    {
        for (int a = 0; a < kGameActionCount; a++)
        {
            const std::string key = std::string("controls.") + gameActionId((GameAction)a);
            wanted.push_back({ key, key + " = " + bindingsToText(settings.controls[a]), false });
        }
    }
    for (std::string& line : lines)
    {
        std::string key, value;
        if (!parseLine(line, key, value))
            continue;
        for (ConfigLine& w : wanted)
        {
            if (key == w.key)
            {
                line = w.text;
                w.found = true;
            }
        }
    }
    for (const ConfigLine& w : wanted)
    {
        if (!w.found)
            lines.push_back(w.text);
    }

    std::ofstream out(gameConfigPath(folder));
    if (!out)
    {
        logLine("ERROR: could not write aitd_remaster.cfg in %s", folder.c_str());
        return false;
    }
    for (const std::string& line : lines)
        out << line << "\n";
    for (const ConfigLine& w : wanted)
        logLine("Wrote to aitd_remaster.cfg: %s", w.text.c_str());
    return true;
}

LaunchResult launchGame(const LauncherSettings& settings, const char* game)
{
    std::error_code ec;
    fs::path tatou = toPath(launcherFolder() + "Tatou.exe");
    if (!fs::exists(tatou, ec))
    {
        logLine("ERROR: Tatou.exe not found in %s", launcherFolder().c_str());
        return LaunchResult::TatouMissing;
    }

    std::string args = std::string("--game ") + game + (settings.debugMode ? " --debug" : " --final");
    if (!settings.gameLanguage.empty())
        args += " --lang " + settings.gameLanguage;
    // Tatou starts in the copy's folder or the GOG installation and finds the
    // rest itself (platform/gameFolder.cpp)
    const GameInstall install = findGameInstall(settings.aitd1DataFolder);
    const fs::path startFolder = install.valid ? install.root : toPath(settings.aitd1DataFolder);
    logLine("Starting Tatou.exe %s (working folder %s%s)", args.c_str(), startFolder.u8string().c_str(),
        install.gog ? ", GOG installation" : "");

#ifdef _WIN32
    std::wstring commandLine = L"\"" + tatou.wstring() + L"\" " + toPath(args).wstring();
    std::wstring workingFolder = startFolder.wstring();

    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(tatou.wstring().c_str(), &commandLine[0], nullptr, nullptr, FALSE, 0, nullptr,
        workingFolder.c_str(), &startup, &process))
    {
        logLine("ERROR: CreateProcess failed (error %lu)", GetLastError());
        return LaunchResult::Failed;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
#else
    fs::current_path(startFolder, ec);
    std::string tatouPath = tatou.string();
    std::vector<std::string> parts = { tatouPath, "--game", game, settings.debugMode ? "--debug" : "--final" };
    if (!settings.gameLanguage.empty())
    {
        parts.push_back("--lang");
        parts.push_back(settings.gameLanguage);
    }
    std::vector<const char*> argv;
    for (const std::string& part : parts)
        argv.push_back(part.c_str());
    argv.push_back(nullptr);
    SDL_Process* process = SDL_CreateProcess(argv.data(), false);
    if (!process)
    {
        logLine("ERROR: could not start Tatou: %s", SDL_GetError());
        return LaunchResult::Failed;
    }
    SDL_DestroyProcess(process);  // does not stop the game
#endif

    logLine("Tatou.exe started");
    return LaunchResult::Started;
}
