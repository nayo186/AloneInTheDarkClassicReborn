///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Debug cheats, for testing only: only in debug mode (launcher.cfg
// debugMode = 1) and only in development builds (CMake option
// CLASSIC_REBORN_DEV_TOOLS; players' builds get empty functions).
//   F9: a window with every item that can be picked up; "Give" puts it in
//       the inventory (given when the window closes, on the game thread)
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Window thread (input.cpp): F9 opens or closes the item window
void debugCheatsToggle();

// The item window is open: the game waits and gets no input
bool debugCheatsWindowOpen();

// Inside the active ImGui frame (bgfxGlue.cpp, next to the F1 window)
void debugCheatsDraw();

// Registers the game loop hook (remasterHooks.cpp)
void debugCheatsRegister();
