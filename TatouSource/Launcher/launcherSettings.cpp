///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted - Launcher
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Launcher settings, game config edits and game start
///////////////////////////////////////////////////////////////////////////////

#include "launcherSettings.h"
#include "userFolder.h"
#include "narrators.h"
#include "languageFolder.h"
#include "gameDisc.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#ifdef __ANDROID__
    // Android: the app's own folder in the shared storage, as the game's
    // (platform/gameFolder.cpp)
    if (const char* external = SDL_GetAndroidExternalStoragePath())
        return std::string(external) + "/";
#endif
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
        else if (key == "aitd2.dataFolder")
            settings.aitd2DataFolder = value;
        else if (key == "aitd3.dataFolder")
            settings.aitd3DataFolder = value;
        else if (key == "debugMode")
            settings.debugMode = (value == "true" || value == "1");
        else if (key == "window.width")
            settings.windowWidth = atoi(value.c_str());
        else if (key == "window.height")
            settings.windowHeight = atoi(value.c_str());
        else if (key == "window.maximized")
            settings.windowMaximized = (value == "true" || value == "1");
    }
    logLine("Loaded launcher.cfg: ui.language=%s aitd1.dataFolder=%s aitd2.dataFolder=%s aitd3.dataFolder=%s",
        settings.uiLanguage.c_str(), settings.aitd1DataFolder.c_str(), settings.aitd2DataFolder.c_str(),
        settings.aitd3DataFolder.c_str());
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
    file << "aitd2.dataFolder = \"" << settings.aitd2DataFolder << "\"\n";
    file << "aitd3.dataFolder = \"" << settings.aitd3DataFolder << "\"\n";
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
    bool disc = false;      // the player's CD as an image (Shared/gameDisc.h)
    int game = -1;          // 0 AITD1 (with Jack), 1 AITD2, 2 AITD3
    fs::path root;          // the folder Tatou starts in
    fs::path gameData;      // the game's PAKs, with the language PAKs
    fs::path jackData;      // PERE.PAK (AITD1 only)
    fs::path configFolder;  // aitd_remaster.cfg
};

// Each game's folder in a GOG installation and a file only it has
static const int kGames = 3;
static const char* const kGogDataFolder[kGames] = { "INDARK", "INDARK2", "INDARK3" };
static const char* const kGameMarker[kGames] = { "LISTBOD2.PAK", "MER.PAK", "AN1.PAK" };

static bool pathExists(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec);
}

// GOG (GAME.INS) or Steam (GAME.INST): the same edition and CD image; the
// game installed there, or -1
static int gogRootGame(const fs::path& folder)
{
    const bool image = pathExists(folder / "GAME.GOG") && (pathExists(folder / "GAME.INS") || pathExists(folder / "GAME.INST"));
    for (int game = 0; game < kGames; game++)
    {
        // AITD3 reads its music from MUSIC, not from its CD image: a copy
        // of the installation without GAME.GOG also works
        const bool music = game == 2 && pathExists(folder / "MUSIC");
        if ((image || music) && pathExists(folder / kGogDataFolder[game] / kGameMarker[game]))
            return game;
    }
    return -1;
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
    if (gogRootGame(path) < 0 && gogRootGame(path.parent_path()) >= 0)
        path = path.parent_path();   // GOG's INDARK (INDARK2, INDARK3) folder was chosen
    const int gogGame = gogRootGame(path);
    if (gogGame >= 0)
    {
        install.configFolder = userDataFolder();
        install.valid = install.gog = true;
        install.game = gogGame;
        install.root = path;
        install.gameData = path / kGogDataFolder[gogGame];
        install.jackData = path / "JACK";
        return install;
    }
    for (int game = 0; game < kGames; game++)
    {
        if (!pathExists(path / kGameMarker[game]))
            continue;
        install.valid = true;
        install.game = game;
        install.root = install.gameData = path;
        install.jackData = path / "JACK";
        // Debug mode keeps a copy's options inside it; otherwise UserData
        install.configFolder = s_debugMode ? path : userDataFolder();
        return install;
    }
    // The player's CD: everything is read from the image, nothing written there
    CdImage image;
    const GameDisc disc = findGameDisc(path, image);
    if (disc.game >= 0)
    {
        install.valid = install.disc = true;
        install.game = disc.game;
        install.root = path;
        install.configFolder = userDataFolder();
    }
    return install;
}

// The player's CD image of a folder, opened; false if it has none
static bool openFolderDisc(const GameInstall& install, CdImage& image)
{
    return install.disc && findGameDisc(install.root, image).game >= 0;
}

int dataFolderGame(const std::string& folder)
{
    return findGameInstall(folder).game;
}

// Each game's portable copy (UserData\<game>\GAME): used when no data folder
// is chosen, or the chosen one is gone; a chosen one gone with no copy there
// is cleared, for the player to choose another
std::string portableGameFolder(int game)
{
    static const char* const games[kGames] = { "AITD1", "AITD2", "AITD3" };
    return (userDataFolder() / games[game] / kPortableGameFolder).u8string();
}

void findPortableGames(LauncherSettings& settings)
{
    static const char* const games[kGames] = { "AITD1", "AITD2", "AITD3" };
    std::string* folders[kGames] = { &settings.aitd1DataFolder, &settings.aitd2DataFolder, &settings.aitd3DataFolder };
    for (int game = 0; game < kGames; game++)
    {
        std::string& folder = *folders[game];
        if (!folder.empty() && dataFolderGame(folder) == game)
            continue;
        const std::string portable = portableGameFolder(game);
        if (dataFolderGame(portable) == game)
        {
            logLine("%s: data folder %s (%s)", games[game], portable.c_str(),
                folder.empty() ? "portable copy found" : "the chosen one is gone or has not the game");
            folder = portable;
        }
        else if (!folder.empty() && !pathExists(fs::u8path(folder)))
        {
            logLine("%s: data folder %s is gone and no portable copy: cleared", games[game], folder.c_str());
            folder.clear();
        }
    }
}

bool isGogInstallFolder(const std::string& folder)
{
    return findGameInstall(folder).gog;
}

bool isGameDiscFolder(const std::string& folder)
{
    return findGameInstall(folder).disc;
}

std::vector<GameFilesSource> gameFilesSources(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    if (!install.valid)
        return {};
    static const GameFilesOf games[kGames] = { GameFilesOf::Aitd1, GameFilesOf::Aitd2, GameFilesOf::Aitd3 };
    GameFilesSource game, jack;
    game.game = games[install.game];
    jack.game = GameFilesOf::Jack;
    jack.prefix = "JACK/";
    bool hasJack = false;
    if (install.disc)
    {
        auto image = std::make_shared<CdImage>();
        const GameDisc disc = findGameDisc(install.root, *image);
        if (disc.game < 0)
            return {};
        game.folder = jack.folder = disc.image;
        game.image = jack.image = image;
        game.imageFolder = kGogDataFolder[install.game];
        jack.imageFolder = "JACK";
        hasJack = image->hasFile("JACK", "PERE.PAK");
    }
    else
    {
        game.folder = install.gameData;
        jack.folder = install.jackData;
        hasJack = pathExists(install.jackData / "PERE.PAK");
    }
    if (install.game == 0 && hasJack)
        return { game, jack };
    return { game };
}

std::string gameFilesCacheFile(int game)
{
    static const char* const games[kGames] = { "AITD1", "AITD2", "AITD3" };
    return (userDataFolder() / games[game] / "fileCheck.cfg").u8string();
}

bool hasJackData(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    if (install.game != 0)
        return false;
    CdImage image;
    if (install.disc)
        return openFolderDisc(install, image) && image.hasFile("JACK", "PERE.PAK");
    return pathExists(install.jackData / "PERE.PAK");
}

std::vector<std::string> findAitd1Narrators(const std::string& folder)
{
    const GameInstall install = findGameInstall(folder);
    if (install.game != 0)
        return {};
    const fs::path workFolder = (!install.gog && !install.disc && s_debugMode) ? install.gameData : userDataFolder() / "AITD1";
    return findNarrators(workFolder / kVoicesFolder);
}

void prepareUserData()
{
    for (const char* game : { "AITD1", "JACK", "AITD2", "AITD3" })
    {
        const fs::path gameFolder = userDataFolder() / game;
        std::error_code ec;
        for (const char* name : { kSavesFolder, kPicturesFolder, kMasksFolder, kExportsFolder })
            fs::create_directories(gameFolder / name, ec);
    }
    std::error_code ec;
    fs::create_directories(userDataFolder() / "AITD1" / kVoicesFolder, ec);
    fs::create_directories(userDataFolder() / "AITD3" / kLanguagesFolder, ec);
    for (const char* game : { "AITD1", "AITD2", "AITD3" })
        fs::create_directories(userDataFolder() / game / kPortableGameFolder, ec);
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
    // AITD3: also a language from the player's CD, in Languages\<name> of
    // the working folder (the copy in debug mode, else UserData\AITD3), with
    // its files or its CD image (Shared/languageFolder.h)
    const fs::path languages = ((!install.gog && !install.disc && s_debugMode) ? install.gameData : userDataFolder() / "AITD3") / kLanguagesFolder;
    // The player's CD: the languages on it
    CdImage discImage;
    const bool disc = openFolderDisc(install, discImage);
    std::string installed = (install.game == 2 && !disc) ? installedLanguageAITD3(install.gameData) : std::string();
    if (!installed.empty() && !pathExists(install.gameData / (installed + ".PAK")))
        installed.clear();
    for (const GameLanguage& language : all)
    {
        const std::string pak = language.pakName + ".PAK";
        // AITD3 is sold one language at a time and changing it leaves the
        // other's PAK behind: only the installed one, whose voices and
        // tracks are there (BUGS.md T46)
        bool has = disc ? discImage.hasFile(kGogDataFolder[install.game], pak)
            : pathExists(install.gameData / pak) && (installed.empty() || installed == language.pakName);
        if (!has && install.game == 2)
        {
            // A language folder: its CD image, or its loose files if none of
            // the ones it needs is missing (BUGS.md T48)
            const fs::path folder = languages / languagePackName(language.pakName.c_str());
            CdImage image;
            if (openLanguageImage(folder, pak, image))
                has = true;
            else if (languageFileExists(folder, pak.c_str()))
            {
                const std::string missing = missingLanguageFiles(folder, language.pakName, install.gameData);
                has = missing.empty();
                if (!has)
                    logLine("Language folder %s not used: it lacks %s", folder.u8string().c_str(), missing.c_str());
            }
        }
        if (has)
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
        else if (key == "postprocessing.scanlines")
            settings.scanlines = isTrue(value);
        else if (key == "postprocessing.scanlineLevel")
            settings.scanlineLevel = std::clamp(atoi(value.c_str()), 1, 4);
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
        else if (key == "controller.vibration")
            settings.vibration = isTrue(value);
        else if (key.compare(0, 9, "controls.") == 0)
        {
            for (int a = 0; a < kGameActionCount; a++)
                if (key.compare(9, std::string::npos, gameActionId((GameAction)a)) == 0)
                    settings.controls[a] = bindingsFromText(value);
        }
    }
    logLine("Game config: language=%s frameLimit=%d vsync=%d filmGrain=%d scanlines=%d bloom=%d ssao=%d animationSmoothing=%d",
        settings.gameLanguage.empty() ? "(first available)" : settings.gameLanguage.c_str(),
        settings.frameLimit, settings.vsync, settings.filmGrain, settings.scanlines, settings.bloom, settings.ssao,
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
        { "postprocessing.scanlines", std::string("postprocessing.scanlines = ") + boolText(settings.scanlines), false },
        { "postprocessing.scanlineLevel", "postprocessing.scanlineLevel = " + std::to_string(settings.scanlineLevel), false },
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
    wanted.push_back({ "controller.vibration", std::string("controller.vibration = ") + boolText(settings.vibration), false });
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

// The game's executable, next to the launcher
#ifdef _WIN32
static const char* const kTatouFile = "Tatou.exe";
#else
static const char* const kTatouFile = "Tatou";
#endif

#ifdef __ANDROID__
static std::vector<std::string> s_gameArguments;

const std::vector<std::string>& launchedGameArguments()
{
    return s_gameArguments;
}
#endif

LaunchResult launchGame(const LauncherSettings& settings, const char* game)
{
    std::error_code ec;
#ifndef __ANDROID__
    fs::path tatou = toPath(launcherFolder() + kTatouFile);
    if (!fs::exists(tatou, ec))
    {
        logLine("ERROR: %s not found in %s", kTatouFile, launcherFolder().c_str());
        return LaunchResult::TatouMissing;
    }
#endif

    std::string args = std::string("--game ") + game + (settings.debugMode ? " --debug" : " --final");
    if (!settings.gameLanguage.empty())
        args += " --lang " + settings.gameLanguage;
    // Tatou starts in the copy's folder or the GOG installation and finds the
    // rest itself (platform/gameFolder.cpp)
    const std::string& folder = (strcmp(game, "aitd3") == 0) ? settings.aitd3DataFolder
        : (strcmp(game, "aitd2") == 0) ? settings.aitd2DataFolder : settings.aitd1DataFolder;
    const GameInstall install = findGameInstall(folder);
    const fs::path startFolder = install.valid ? install.root : toPath(folder);
    logLine("Starting %s %s (working folder %s%s)", kTatouFile, args.c_str(), startFolder.u8string().c_str(),
        install.gog ? ", GOG installation" : install.disc ? ", the player's CD image" : "");

#if defined(__ANDROID__)
    // The game runs in this same program once the launcher closes (Fitd/fitd.cpp)
    fs::current_path(startFolder, ec);
    s_gameArguments = { "Tatou", "--game", game, settings.debugMode ? "--debug" : "--final" };
    if (!settings.gameLanguage.empty())
    {
        s_gameArguments.push_back("--lang");
        s_gameArguments.push_back(settings.gameLanguage);
    }
#elif defined(_WIN32)
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

    logLine("%s started", kTatouFile);
    return LaunchResult::Started;
}
