///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game folders. The launcher starts Tatou in the chosen folder: a copy of the
// data (AITD1 there, Jack in JACK) or a GOG or Steam installation (the same
// edition: GAME.GOG and its track list GAME.INS, or GAME.INST on Steam;
// AITD1 in INDARK, Jack in JACK). The game data is read through
// homePath; the working directory is the folder written to:
// - normally the portable UserData folder next to Tatou.exe
//   (Shared/userFolder.h): the data is never written to;
// - in debug mode with a copy of the data, the copy itself, as before.
// The debug mode comes from --debug / --final (the launcher passes one), or
// else from launcher.cfg next to the exe. It also enables test tools (F9).
// Music and voices of a GOG installation come from GAME.GOG (gogImage.cpp).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// Called first, before the log file opens (reads --game aitd1|jack and
// --debug / --final)
void selectGameFolder(int argc, char* argv[]);

// Writes to the log what selectGameFolder found (once the log file is open)
void logGameFolder();

// Debug mode: test tools on; with a copy of the data, written into the copy
bool isDebugMode();

// Folder of aitd_remaster.cfg, with a trailing separator (empty: the working
// directory)
const std::string& configFolder();

// The player's pictures, masks and the exports, relative to the working
// directory, with a trailing slash: Backgrounds/, Masks/, Exports/ in the
// portable folder, or backgrounds_custom/ (both) and backgrounds_export/
// inside a copy in debug mode
const char* customPicturesFolder();
// Folder of the saves, relative to the working directory, with a trailing
// slash: Saves/, or empty (the copy itself) in debug mode with a copy
const char* savesFolder();
// Folder of the player's masks (Masks, or backgrounds_custom in debug mode)
const char* customMasksFolder();
// Folder of the exported backgrounds and masks
const char* exportsFolder();
// True when the working folder is UserData (outside the debug mode or with GOG/Steam)
bool usesPortableFolders();

// GOG installation: the CD image and its track list (empty for a copy)
bool isGogInstall();
const std::string& gogImagePath();   // Path of the CD image (...\GAME.GOG)
const std::string& gogCuePath();     // Path of the track list (...\GAME.INS on GOG, GAME.INST on Steam)
