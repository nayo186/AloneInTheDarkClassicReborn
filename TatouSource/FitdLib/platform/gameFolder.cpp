///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game folders (see gameFolder.h)
///////////////////////////////////////////////////////////////////////////////

#include "gameFolder.h"
#include "consoleLog.h"
#include "userFolder.h"
#include "gameDisc.h"
#include "gameFiles.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

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
static std::string s_image;          // GAME.GOG or the player's CD image
static std::string s_cue;
static std::string s_imageFolder;    // the player's CD: its folder with the game
static std::string s_gogFolder;
static std::string s_message;   // logged by logGameFolder
static bool s_checkFiles = false;           // startGameFileCheck: the data folder is known
static GameFilesSource s_checkSource;       // its game files
static fs::path s_checkCache;               // what the check read (fileCheck.cfg)

bool isDebugMode() { return s_debug; }
bool usesPortableFolders() { return s_portable; }
const std::string& configFolder() { return s_configFolder; }
const std::string& gameImagePath() { return s_image; }
const std::string& gameCuePath() { return s_cue; }
const std::string& gameImageFolder() { return s_imageFolder; }
const std::string& gogInstallFolder() { return s_gogFolder; }

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
#ifdef __ANDROID__
    // Android: the app's own folder in the shared storage
    // (Android/data/com.classicreborn.game/files): UserData, with each
    // game's GAME folder, goes there
    const char* external = SDL_GetAndroidExternalStoragePath();
    if (external)
        return fs::u8path(external);
#endif
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

// The player's CD image of this game in the folder, opened in `image`
static bool hasGameDisc(const fs::path& folder, Trilogy game, CdImage& image, GameDisc& disc)
{
    disc = findGameDisc(folder, image);
    return disc.game == (int)game;
}

static bool hasGameDisc(const fs::path& folder, Trilogy game)
{
    CdImage image;
    GameDisc disc;
    return hasGameDisc(folder, game, image, disc);
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
        const bool known = pathExists(start / gameMarker(game)) || isGogRoot(start, game) || isGogRoot(start.parent_path(), game)
            || hasGameDisc(start, game);
        const std::string chosen = launcherSetting((std::string(gameKey(game)) + ".dataFolder").c_str());
        // The game's portable copy (UserData\<game>\GAME; Jack's is AITD1's),
        // when none is chosen or the chosen one is gone, as the launcher does
        const fs::path portable = exeFolder() / kUserDataFolder
            / (game == Trilogy::AITD3 ? "AITD3" : game == Trilogy::AITD2 ? "AITD2" : "AITD1") / kPortableGameFolder;
        if (!known && !chosen.empty() && pathExists(fs::u8path(chosen)))
            start = fs::u8path(chosen);
        // Only if the game is in it: an empty GAME (the launcher creates them)
        // took the log and the rest, written into the data folder
        else if (!known && (isGogRoot(portable, game) || pathExists(portable / gameMarker(game)) || hasGameDisc(portable, game)))
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
    // The player's CD as an image (only where there are no loose files)
    CdImage discImage;
    GameDisc disc;
    const bool cd = !gog && !pathExists(start / gameMarker(game)) && hasGameDisc(start, game, discImage, disc);

    // Where the game data is (with the CD, in the image: homePath is its
    // folder, which has no game files)
    fs::path dataFolder = gog ? start / (jack ? "JACK" : gogDataFolder(game)) : (jack && !cd ? start / "JACK" : start);
    if (jack && !(cd ? discImage.hasFile("JACK", "PERE.PAK") : pathExists(dataFolder / "PERE.PAK")))
    {
        s_message = MAIN_WARN "Jack in the Dark: no JACK\\PERE.PAK in the data folder" CON_RESET "\n";
        return;
    }
    if (!jack && !cd && !pathExists(dataFolder / gameMarker(game)))
        return;   // not a known layout: the working directory as it is

    // The game files to check (startGameFileCheck)
    s_checkFiles = true;
    s_checkSource.game = jack ? GameFilesOf::Jack : game == Trilogy::AITD3 ? GameFilesOf::Aitd3
        : game == Trilogy::AITD2 ? GameFilesOf::Aitd2 : GameFilesOf::Aitd1;
    s_checkSource.folder = cd ? disc.image : dataFolder;
    if (cd)
    {
        s_checkSource.image = std::make_shared<CdImage>(discImage);
        s_checkSource.imageFolder = jack ? "JACK" : kGameDiscFolder[(int)game];
    }
    s_checkCache = dataFolder / "fileCheck.cfg";

    if (s_debug && !gog && !cd)
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
    s_checkCache = userFolder / "fileCheck.cfg";
    s_configFolder = (userRoot / "").string();
    s_picturesFolder = std::string(kPicturesFolder) + "/";
    s_masksFolder = std::string(kMasksFolder) + "/";
    s_exportsFolder = std::string(kExportsFolder) + "/";
    s_savesFolder = std::string(kSavesFolder) + "/";
    fs::create_directories(userFolder / kSavesFolder, error);
    if (gog)
    {
        s_gogFolder = start.string();
        if (pathExists(start / "GAME.GOG"))
            s_image = (start / "GAME.GOG").string();
        s_cue = cueFile(start).string();
    }
    if (cd)
    {
        s_image = disc.image.string();
        s_cue = disc.cue.string();
        s_imageFolder = jack ? "JACK" : kGameDiscFolder[(int)game];
    }
    // AITD3's music is in MUSIC; its CD image may be left out of a copy
    const std::string audioFrom = cd ? ", everything from the CD image " + s_image + " (" + s_imageFolder + ")"
        : !gog ? std::string()
        : !s_image.empty() ? ", music and voices from " + s_image
        : ", music from " + (start / "MUSIC").string();
    s_message = std::string(MAIN_OK) + (cd ? "Player's CD" : gog ? "GOG/Steam installation" : "Copy of the data") + (s_debug ? " (debug mode)" : "") +
        ": data in " + dataFolder.string() + " (read only), everything written in " + userFolder.string() +
        ", configuration in " + s_configFolder + audioFrom + "\n";
}

void logGameFolder()
{
    if (!s_message.empty())
        printf("%s", s_message.c_str());
}

void startGameFileCheck()
{
    if (!s_checkFiles)
        return;
    // A thread of its own, so the game does not wait; unchanged files are not
    // read again (fileCheck.cfg, shared with the launcher)
    std::thread([]
    {
        GameFilesCache cache;
        cache.load(s_checkCache);
        const GameFilesResult result = checkGameFiles({ s_checkSource }, cache);
        cache.save(s_checkCache);
        if (result.unknown.empty() && result.missing.empty())
            printf(MAIN_OK "Game files: a known edition\n");
        if (!result.missing.empty())
        {
            printf(MAIN_WARN "Game files: %d needed files are missing (the game will close or freeze when it needs them):" CON_RESET "\n",
                (int)result.missing.size());
            for (const std::string& name : result.missing)
                printf(MAIN_WARN "  %s" CON_RESET "\n", name.c_str());
        }
        if (!result.unknown.empty())
        {
            printf(MAIN_WARN "Game files: %d are in no edition we know (another edition, changed or damaged; it may not work well):" CON_RESET "\n",
                (int)result.unknown.size());
            for (const std::string& line : result.unknown)
                printf(MAIN_WARN "  %s" CON_RESET "\n", line.c_str());
        }
    }).detach();
}
