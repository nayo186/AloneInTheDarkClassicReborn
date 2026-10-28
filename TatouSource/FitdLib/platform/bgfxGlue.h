///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// BGFX initialization and window management declarations
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL.h>

// Creates the window and starts bgfx, ImGui and the screen effects
int initBgfxGlue(int argc, char* argv[]);

// Start of a frame: window size, scene rectangle, ImGui frame, controller in the menus
void StartFrame();
// End of a frame: draws the menus and the effects and shows the frame
void EndFrame();

extern bool gCloseApp;

extern SDL_Window* gWindowBGFX;

extern bool gIsFullscreen;
extern bool g_pendingFullscreenToggle;
// Window mode: keeps the window in the shape of the aspect ratio (window thread)
void applyWindowAspectRatio();
// Switches between window and fullscreen
void toggleFullscreen();

