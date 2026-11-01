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

// The game's folder in a GOG installation and a file only that game has
// (Jack goes in AITD1's installation)
static const char* gogDataFolder(bool aitd2) { return aitd2 ? "INDARK2" : "INDARK"; }
static const char* gameMarker(bool aitd2) { return aitd2 ? "MER.PAK" : "LISTBOD2.PAK"; }

// A GOG or Steam installation: GAME.GOG and its track list next to INDARK
// (INDARK2 for AITD2)
static bool isGogRoot(const fs::path& folder, bool aitd2)
{
    return pathExists(folder / "GAME.GOG") && !cueFile(folder).empty() &&
        pathExists(folder / gogDataFolder(aitd2) / gameMarker(aitd2));
}

static void setHomePath(const fs::path& dataFolder)
{
    const std::string path = (dataFolder / "").string();
    snprintf(homePath, 512, "%s", path.c_str());
}

void selectGameFolder(int argc, char* argv[])
{
    bool jack = false;
    bool aitd2 = false;
    bool debugGiven = false;
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--game" && i + 1 < argc)
        {
            jack = (lower(argv[i + 1]) == "jack");
            aitd2 = (lower(argv[i + 1]) == "aitd2");
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
        const bool known = pathExists(start / gameMarker(aitd2)) || isGogRoot(start, aitd2) || isGogRoot(start.parent_path(), aitd2);
        const std::string chosen = launcherSetting(aitd2 ? "aitd2.dataFolder" : "aitd1.dataFolder");
        if (!known && !chosen.empty())
            start = fs::u8path(chosen);
    }
    fs::current_path(start, error);
    if (error)
    {
        s_message = std::string(MAIN_WARN "Data folder not found: ") + start.string() + CON_RESET "\n";
        return;
    }

    // Started inside the GOG INDARK (INDARK2) folder: its installation is the parent
    if (!isGogRoot(start, aitd2) && isGogRoot(start.parent_path(), aitd2))
        start = start.parent_path();
    const bool gog = isGogRoot(start, aitd2);

    // Where the game data is
    fs::path dataFolder = gog ? start / (jack ? "JACK" : gogDataFolder(aitd2)) : (jack ? start / "JACK" : start);
    if (jack && !pathExists(dataFolder / "PERE.PAK"))
    {
        s_message = MAIN_WARN "Jack in the Dark: no JACK\\PERE.PAK in the data folder" CON_RESET "\n";
        return;
    }
    if (!jack && !pathExists(dataFolder / gameMarker(aitd2)))
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
    const fs::path userFolder = userRoot / (jack ? "JACK" : aitd2 ? "AITD2" : "AITD1");
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
    s_message = std::string(MAIN_OK) + (gog ? "GOG/Steam installation" : "Copy of the data") + (s_debug ? " (debug mode)" : "") +
        ": data in " + dataFolder.string() + " (read only), everything written in " + userFolder.string() +
        ", configuration in " + s_configFolder + (gog ? ", music and voices from " + s_gogImage : std::string()) + "\n";
}

void logGameFolder()
{
    if (!s_message.empty())
        printf("%s", s_message.c_str());
}
