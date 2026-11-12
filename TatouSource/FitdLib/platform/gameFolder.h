///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game folders. The data folder is the one after --data, else the folder
// Tatou is started in (the launcher starts it in the chosen one), else the
// one chosen in the launcher (aitd1.dataFolder or aitd2.dataFolder in
// launcher.cfg), so other
// programs (a frontend) can start each game directly. It can be a copy of the
// data (AITD1 or AITD2 there, Jack in JACK) or a GOG or Steam installation (the same
// edition: GAME.GOG and its track list GAME.INS, or GAME.INST on Steam;
// AITD1 in INDARK, Jack in JACK, AITD2 in INDARK2). The game data is read through
// homePath; the working directory is the folder written to:
// - normally the portable UserData folder next to Tatou.exe
//   (Shared/userFolder.h): the data is never written to;
// - in debug mode with a copy of the data, the copy itself, as before.
// The debug mode comes from --debug / --final (the launcher passes one), or
// else from launcher.cfg next to the exe. It also enables test tools (F9).
// Music and voices of a GOG installation come from GAME.GOG (gameImage.cpp).
// The folder can also hold the player's own CD as an image (.cue with its
// .bin, or an .iso: Shared/gameDisc.h): everything is read from it.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// Called first, before the log file opens (reads --game aitd1|jack|aitd2, --data
// <folder> (or --data=<folder>) and --debug / --final)
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

// The CD image of a GOG installation (GAME.GOG) or the player's CD; empty for a copy
const std::string& gameImagePath();
// Its track list (GAME.INS on GOG, GAME.INST on Steam, the player's .cue); empty if none
const std::string& gameCuePath();
// The player's CD: the folder of the CD with the game's files ("INDARK",
// "INDARK2", "INDARK3", "JACK"), read from the image; empty otherwise
const std::string& gameImageFolder();
// The GOG or Steam installation's folder (empty for a copy or a CD image)
const std::string& gogInstallFolder();
// Checks the game's files against the editions we know (Shared/gameFiles.h)
// in a thread of its own and notes in tatou.log what differs or is missing
void startGameFileCheck();
