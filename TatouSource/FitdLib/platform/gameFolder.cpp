///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game folders (see gameFolder.h)
///////////////////////////////////////////////////////////////////////////////

#include "gameFolder.h"
#include "consoleLog.h"
#include "userFolder.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

extern "C" { extern char homePath[512]; }

static bool s_debug = false;
static bool s_portable = false;
static std::string s_configFolder;
static std::string s_gogImage;
static std::string s_gogCue;
static std::string s_message;   // logged by logGameFolder

bool isDebugMode() { return s_debug; }
bool usesPortableFolders() { return s_portable; }
const std::string& configFolder() { return s_configFolder; }
bool isGogInstall() { return !s_gogImage.empty(); }
const std::string& gogImagePath() { return s_gogImage; }
const std::string& gogCuePath() { return s_gogCue; }

static std::string s_picturesFolder = "backgrounds_custom/";
static std::string s_masksFolder = "backgrounds_custom/";
static std::string s_exportsFolder = "backgrounds_export/";
static std::string s_savesFolder;
const char* customPicturesFolder() { return s_picturesFolder.c_str(); }
const char* customMasksFolder() { return s_masksFolder.c_str(); }
const char* exportsFolder() { return s_exportsFolder.c_str(); }
const char* savesFolder() { return s_savesFolder.c_str(); }

static std::string lower(std::string text)
{
    for (char& c : text)
        c = (char)tolower((unsigned char)c);
    return text;
}

static bool pathExists(const fs::path& path)
{
    std::error_code error;
    return fs::exists(path, error);
}

// The folder of Tatou.exe (and of the launcher), UTF-8
static fs::path exeFolder()
{
    const char* base = SDL_GetBasePath();
    return base ? fs::u8path(base) : fs::path();
}

// A value of launcher.cfg, next to the exe (empty if it is not there)
static std::string launcherSetting(const char* wanted)
{
    std::ifstream file(exeFolder() / "launcher.cfg");
    std::string line;
    while (std::getline(file, line))
    {
        const size_t equals = line.find('=');
        if (equals == std::string::npos)
            continue;
        std::string key = line.substr(0, equals), value = line.substr(equals + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t\""));
        value.erase(value.find_last_not_of(" \t\"\r") + 1);
        if (key == wanted)
            return value;
    }
    return "";
}

// "debugMode = true" in launcher.cfg (used when no --debug / --final)
static bool launcherDebugMode()
{
    const std::string value = launcherSetting("debugMode");
    return value == "true" || value == "1";
}

// The folder after --data, as typed (on Windows read again from the wide
// command line, so any letter in the path works)
static fs::path dataArgument(int argc, char* argv[])
{
    for (int i = 1; i + 1 < argc; i++)
    {
        if (strcmp(argv[i], "--data") != 0)
            continue;
#ifdef _WIN32
        int count = 0;
        wchar_t** wide = CommandLineToArgvW(GetCommandLineW(), &count);
        const fs::path folder = (wide && count == argc) ? fs::path(wide[i + 1]) : fs::path(argv[i + 1]);
        LocalFree(wide);
        return folder;
#else
        return fs::u8path(argv[i + 1]);
#endif
    }
    return fs::path();
}

// The track list of the CD image: GAME.INS (GOG) or GAME.INST (Steam, same
// edition and same contents); empty if neither
static fs::path cueFile(const fs::path& folder)
{
    for (const char* name : { "GAME.INS", "GAME.INST" })
        if (pathExists(folder / name))
            return folder / name;
    return fs::path();
}

// The games of the trilogy (Jack goes with AITD1: its folder is inside AITD1's)
enum class Trilogy { AITD1, AITD2, AITD3 };

// The game's folder in a GOG installation, a file only that game has and the
// name of its folder in UserData and of its data folder in launcher.cfg
static const char* gogDataFolder(Trilogy game) { return game == Trilogy::AITD3 ? "INDARK3" : game == Trilogy::AITD2 ? "INDARK2" : "INDARK"; }
static const char* gameMarker(Trilogy game) { return game == Trilogy::AITD3 ? "AN1.PAK" : game == Trilogy::AITD2 ? "MER.PAK" : "LISTBOD2.PAK"; }
static const char* gameKey(Trilogy game) { return game == Trilogy::AITD3 ? "aitd3" : game == Trilogy::AITD2 ? "aitd2" : "aitd1"; }

// A GOG or Steam installation: GAME.GOG and its track list next to INDARK
// (INDARK2 for AITD2, INDARK3 for AITD3)
static bool isGogRoot(const fs::path& folder, Trilogy game)
{
    // AITD3 reads its music from MUSIC, not from its CD image: a copy of
    // the installation without GAME.GOG also works (UserData\AITD3\GAME)
    const bool image = pathExists(folder / "GAME.GOG") && !cueFile(folder).empty();
    const bool music = game == Trilogy::AITD3 && pathExists(folder / "MUSIC");
    return (image || music) && pathExists(folder / gogDataFolder(game) / gameMarker(game));
}

static void setHomePath(const fs::path& dataFolder)
{
    const std::string path = (dataFolder / "").string();
    snprintf(homePath, 512, "%s", path.c_str());
}

void selectGameFolder(int argc, char* argv[])
{
    bool jack = false;
    Trilogy game = Trilogy::AITD1;
    bool debugGiven = false;
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--game" && i + 1 < argc)
        {
            const std::string name = lower(argv[i + 1]);
            jack = (name == "jack");
            game = name == "aitd3" ? Trilogy::AITD3 : name == "aitd2" ? Trilogy::AITD2 : Trilogy::AITD1;
        }
        else if (arg == "--debug")
            s_debug = debugGiven = true;
        else if (arg == "--final")
        {
            s_debug = false;
            debugGiven = true;
        }
    }
    if (!debugGiven)
        s_debug = launcherDebugMode();

    // The data folder: --data, or the folder Tatou was started in (the
    // launcher starts it there), or else the one chosen in the launcher
    // (launcher.cfg): other programs (a frontend) can start Tatou directly
    std::error_code error;
    fs::path start = dataArgument(argc, argv);
    if (start.empty())
    {
        start = fs::current_path(error);
        if (error)
            return;
        const bool known = pathExists(start / gameMarker(game)) || isGogRoot(start, game) || isGogRoot(start.parent_path(), game);
        const std::string chosen = launcherSetting((std::string(gameKey(game)) + ".dataFolder").c_str());
        // The game's portable copy (UserData\<game>\GAME; Jack's is AITD1's),
        // when none is chosen or the chosen one is gone, as the launcher does
        const fs::path portable = exeFolder() / kUserDataFolder
            / (game == Trilogy::AITD3 ? "AITD3" : game == Trilogy::AITD2 ? "AITD2" : "AITD1") / kPortableGameFolder;
        if (!known && !chosen.empty() && pathExists(fs::u8path(chosen)))
            start = fs::u8path(chosen);
        else if (!known && pathExists(portable))
            start = portable;
    }
    fs::current_path(start, error);
    if (error)
    {
        s_message = std::string(MAIN_WARN "Data folder not found: ") + start.string() + CON_RESET "\n";
        return;
    }

    // Started inside the GOG INDARK (INDARK2, INDARK3) folder: its installation is the parent
    if (!isGogRoot(start, game) && isGogRoot(start.parent_path(), game))
        start = start.parent_path();
    const bool gog = isGogRoot(start, game);

    // Where the game data is
    fs::path dataFolder = gog ? start / (jack ? "JACK" : gogDataFolder(game)) : (jack ? start / "JACK" : start);
    if (jack && !pathExists(dataFolder / "PERE.PAK"))
    {
        s_message = MAIN_WARN "Jack in the Dark: no JACK\\PERE.PAK in the data folder" CON_RESET "\n";
        return;
    }
    if (!jack && !pathExists(dataFolder / gameMarker(game)))
        return;   // not a known layout: the working directory as it is

    if (s_debug && !gog)
    {
        // Debug mode with a copy: everything inside the copy, as before
        if (jack)
        {
            fs::current_path(dataFolder, error);
            s_configFolder = (start / "").string();
        }
        s_message = std::string(MAIN_OK "Debug mode, copy of the data: everything in ") + dataFolder.string() + "\n";
        return;
    }

    // The portable folder next to the exe; the data is only read
    const fs::path userRoot = exeFolder() / kUserDataFolder;
    const fs::path userFolder = userRoot / (jack ? "JACK" : game == Trilogy::AITD3 ? "AITD3" : game == Trilogy::AITD2 ? "AITD2" : "AITD1");
    fs::create_directories(userFolder, error);
    fs::current_path(userFolder, error);
    if (error)
    {
        s_message = std::string(MAIN_WARN "Cannot use the user folder ") + userFolder.string() + ": data folder used" CON_RESET "\n";
        return;
    }
    setHomePath(dataFolder);
    s_portable = true;
    s_configFolder = (userRoot / "").string();
    s_picturesFolder = std::string(kPicturesFolder) + "/";
    s_masksFolder = std::string(kMasksFolder) + "/";
    s_exportsFolder = std::string(kExportsFolder) + "/";
    s_savesFolder = std::string(kSavesFolder) + "/";
    fs::create_directories(userFolder / kSavesFolder, error);
    if (gog)
    {
        s_gogImage = (start / "GAME.GOG").string();
        s_gogCue = cueFile(start).string();
    }
    // AITD3's music is in MUSIC; its CD image may be left out of a copy
    const std::string audioFrom = !gog ? std::string()
        : pathExists(start / "GAME.GOG") ? ", music and voices from " + s_gogImage
        : ", music from " + (start / "MUSIC").string();
    s_message = std::string(MAIN_OK) + (gog ? "GOG/Steam installation" : "Copy of the data") + (s_debug ? " (debug mode)" : "") +
        ": data in " + dataFolder.string() + " (read only), everything written in " + userFolder.string() +
        ", configuration in " + s_configFolder + audioFrom + "\n";
}

void logGameFolder()
{
    if (!s_message.empty())
        printf("%s", s_message.c_str());
}
