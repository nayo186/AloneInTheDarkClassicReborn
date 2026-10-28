///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Input handling declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _INPUT_H_
#define _INPUT_H_

#include <SDL.h>

// The connected controller (its buttons are read by controls.cpp)
struct ControllerState
{
    SDL_Gamepad* gamepad;
    bool connected;
};

extern ControllerState g_controllerState;

// Window resize flag - set when window is resized/maximized/fullscreened
extern bool g_windowWasResized;

extern "C" {
// Reads keyboard, mouse and controller each frame and turns them into the game's inputs
void readKeyboard(void);
}

// The controller is the last thing used (a button, or a stick or trigger
// pushed past its dead zone): the mouse cursor is hidden until the mouse
// moves again (thread-safe)
bool controllerInUse();

// True while the game must not take any input: the F1 window (or the debug
// item window) is open, or it just closed and something that was held (the
// Esc, Enter, click or B that closed it) has not been released yet
bool gameInputBlocked();

// Reset the window resize flag (call after handling resize)
void resetWindowResizeFlag();

// Opens the first controller connected (and the ones connected later)
void initController();
// Closes the controller
void shutdownController();
// A controller was connected or disconnected
void handleControllerDeviceEvent(SDL_Event& event);

#endif
