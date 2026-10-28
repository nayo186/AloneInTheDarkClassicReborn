///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// In-game advanced options (F1 or Home): the same options as the launcher
// window (FPS, VSync, screen effects, animation smoothing), applied live and
// saved to aitd_remaster.cfg when the window closes. The game waits while it
// is open (mainLoop.cpp asks through g_isOptionsWindowOpen).
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Thread-safe: F1 arrives on SDL's event thread, the window is drawn on the
// game/render thread
bool advancedOptionsIsOpen();

// The game waits (g_isOptionsWindowOpen): the window is open or an export
// started from it is still running
bool advancedOptionsPausesGame();
// Opens or closes the F1 window
void advancedOptionsToggle();

// The controls window is waiting for a new input (thread-safe)
bool advancedOptionsCapturingInput();

// Called inside the active ImGui frame, just before imguiEndFrame()
void advancedOptionsDraw();
