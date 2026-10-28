///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Launch options: command-line parameters passed by the launcher
//   --game aitd1|jack   --lang ENGLISH|FRANCAIS|ITALIANO|ESPAGNOL|DEUTSCH
// Anything missing falls back to aitd_remaster.cfg.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

struct LaunchOptions
{
    std::string game;      // "aitd1", "jack" or empty
    std::string language;  // language PAK name (e.g. "ESPAGNOL") or empty
};

extern LaunchOptions g_launchOptions;

// Reads the parameters above; unknown parameters are left to other parsers.
void parseLaunchOptions(int argc, char* argv[]);

// Applies the options: game selection and language. Call after
// loadSettings() and before the graphics and game data are set up.
void applyLaunchOptions();
