///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted - Launcher
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Launcher settings and game start:
// - launcher.cfg (next to Launcher.exe): interface language, data folder,
//   debug mode and the window
// - aitd_remaster.cfg (configFolder of the install): gamedata.language, the
//   advanced options, the volumes and the controls; only these lines are
//   changed
// - starts Tatou.exe with --game, --lang and --debug / --final
// Everything is logged to launcher.log next to Launcher.exe.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "controlsBindings.h"
#include "audioVolume.h"

#include <string>
#include <vector>

struct GameLanguage
{
    std::string pakName;      // "ESPAGNOL"
    std::string displayName;  // "Español"
};

struct LauncherSettings
{
    std::string uiLanguage = "en";
    std::string aitd1DataFolder;   // UTF-8 path
    bool debugMode = false;        // test tools; a copy of the data keeps everything inside it
    int windowWidth = 0;           // the launcher window as it was left (0: not saved yet)
    int windowHeight = 0;
    bool windowMaximized = false;

    // Read from / written to the data folder's aitd_remaster.cfg
    std::string gameLanguage;      // language PAK name, empty = first available

    // Advanced options (all at their default value = "default settings")
    int frameLimit = kDefaultFrameLimit;   // frames per second, 0 = no limit
    bool vsync = true;
    int aspect = 0;            // 0 = 4:3 (original), 1 = 16:10, 2 = stretched
    int resolution = 0;        // scene image width (4:3), 0 = automatic
    bool fullscreen = false;
    bool filmGrain = false;
    bool bloom = false;
    bool ssao = false;
    int antialiasing = 0;      // 0 off, 1 FXAA
    bool animationSmoothing = false;
    bool customBackgrounds = false;
    bool enhancedText = false;

    // Volumes 0-100 (audio.musicVolume, audio.soundVolume); like the controls,
    // not part of the default settings button
    int musicVolume = kDefaultAudioVolume;
    int soundVolume = kDefaultAudioVolume;
    std::string voices;   // narrator (audio.voices): a folder of Voices, empty = the game's (English)

    // Game controls ("controls.<action>" lines; not part of the default
    // settings button: they are reset in their own window)
    std::vector<std::vector<InputBinding>> controls;
    void restoreDefaultControls()
    {
        controls.assign(kGameActionCount, {});
        for (int a = 0; a < kGameActionCount; a++)
            controls[a] = defaultBindings((GameAction)a);
    }

    static const int kDefaultFrameLimit = 60;
    bool isDefault() const
    {
        return frameLimit == kDefaultFrameLimit && vsync && aspect == 0 && resolution == 0 && !fullscreen &&
            !filmGrain && !bloom && !ssao && antialiasing == 0 && !animationSmoothing && !customBackgrounds &&
            !enhancedText;
    }
    void restoreDefaults()
    {
        frameLimit = kDefaultFrameLimit;
        vsync = true;
        aspect = 0;
        resolution = 0;
        fullscreen = false;
        filmGrain = bloom = ssao = animationSmoothing = customBackgrounds = false;
        enhancedText = false;
        antialiasing = 0;
    }
};

// Folder that contains Launcher.exe (and Tatou.exe), UTF-8, with trailing separator
std::string launcherFolder();

// Opens launcher.log next to Launcher.exe
void logOpen();
// Writes a line to launcher.log
void logLine(const char* format, ...);

// Reads launcher.cfg
void loadLauncherSettings(LauncherSettings& settings);
// Writes launcher.cfg
void saveLauncherSettings(const LauncherSettings& settings);

// The chosen folder can be a copy of the data (LISTBOD2.PAK in it, Jack in
// its JACK subfolder) or a GOG or Steam installation (GAME.GOG and GAME.INS,
// or GAME.INST on Steam; AITD1 in INDARK, Jack in JACK; the INDARK folder
// itself may be chosen too). A GOG
// installation is never written to: aitd_remaster.cfg goes to the user folder
// (Shared/userFolder.h), as the game's saves and log.
bool isAitd1DataFolder(const std::string& folder);
// True if the folder is a GOG or Steam installation
bool isGogInstallFolder(const std::string& folder);

// True if Jack in the Dark is there too (PERE.PAK)
bool hasJackData(const std::string& folder);
// The narrators with voices in AITD1's working folder (Voices: in
// UserData\AITD1, or in the copy itself in debug mode; Shared/narrators.h)
std::vector<std::string> findAitd1Narrators(const std::string& folder);

// Debug mode (launcher.cfg "debugMode = 1", only by hand): with a copy of the data, the game
// writes inside the copy; otherwise everything goes to UserData next to the
// launcher (Shared/userFolder.h)
void setLauncherDebugMode(bool debugMode);

// Creates UserData with each game's Saves, Backgrounds, Masks and Exports folders
// (and AITD1's Voices); called when the launcher opens
void prepareUserData();

// Language PAKs present in the data folder, in a fixed order
std::vector<GameLanguage> findGameLanguages(const std::string& folder);

// Language and advanced options of the data folder's aitd_remaster.cfg
void readGameConfig(const std::string& folder, LauncherSettings& settings);
// Writes the launcher's options in the game's aitd_remaster.cfg, keeping every other line
bool writeGameConfig(const std::string& folder, const LauncherSettings& settings);

enum class LaunchResult
{
    Started,
    TatouMissing,
    Failed,
};

// Starts Tatou.exe with the default settings and the chosen language, in the
// AITD1 data folder: game "aitd1", or "jack" (Tatou enters the JACK subfolder
// and shares the AITD1 configuration)
LaunchResult launchGame(const LauncherSettings& settings, const char* game);
