///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The check of the chosen data folder's game files against the editions we
// know (Shared/gameFiles.h), in a thread of its own so the launcher never
// stops while the files are read; the launcher shows its progress and, when
// it ends, what differs or is missing.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "gameFiles.h"

#include <string>
#include <vector>

struct FileCheckState
{
    bool finished = false;      // the check ended (or none was started)
    int done = 0;               // files checked so far
    int total = 0;              // files to check (0 while they are listed)
    GameFilesResult result;     // once finished: what differs or is missing
    bool accepted = false;      // those problems are the ones the player chose not to be warned about
};

// Starts checking these files (gameFilesSources) in a thread, keeping what it
// reads in cacheFile; a check still running is stopped first
void fileCheckStart(const std::vector<GameFilesSource>& sources, const std::string& cacheFile);
// Stops the check if it is running (on leaving, or when there is no game)
void fileCheckStop();
// How the check goes; its result once finished
FileCheckState fileCheckState();
// The player chose not to be warned again about the problems found: kept
// with what the check read (they are warned again if the files change)
void fileCheckAccept();
