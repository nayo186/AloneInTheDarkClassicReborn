///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// "Return to the launcher" (issue #13, BUGS.md C94). The launcher starts the
// game with --from-launcher; then the last entry of the game's menu and a
// button of F1 close the game and open the launcher again, which shows the
// game just left. Started without it (a frontend), the game closes as always.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Notes whether the launcher started this game (selectGameFolder, --from-launcher)
void setStartedByLauncher(bool started);

// True if the launcher started the game: its menus offer to go back to it
bool startedByLauncher();

// Opens the launcher again: on a PC the Launcher next to Tatou, on Android
// the app starts anew on the launcher's screen once the game has ended.
// False if it could not
bool openLauncher();

// From F1 (the window thread's frame): the window thread opens the launcher
// and closes the game at the start of its next frame (osystemSDL.cpp)
void requestReturnToLauncher();

// True once requestReturnToLauncher was called
bool returnToLauncherRequested();
