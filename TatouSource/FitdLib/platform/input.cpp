///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Keyboard, mouse, and gamepad input handling via SDL
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include <SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <atomic>
#include <cmath>
#include "settings.h"
#include "controls.h"
#include "bgfxGlue.h"
#include "platformHooks.h"

extern float nearVal;
extern float farVal;
extern float cameraZoom;
extern float fov;

// Controller state - exposed in input.h
ControllerState g_controllerState = {
    nullptr, // gamepad
    false,   // connected
};

// Window resize flag - set when window dimensions change
bool g_windowWasResized = false;

void resetWindowResizeFlag()
{
    g_windowWasResized = false;
}

static std::atomic_bool s_inputBlocked(false);
static std::atomic_bool s_controllerInUse(false);

bool controllerInUse()
{
    return s_controllerInUse.load(std::memory_order_acquire);
}

// The controller hides the mouse cursor; moving the mouse shows it again
static void setControllerInUse(bool inUse)
{
    if (s_controllerInUse.exchange(inUse) == inUse)
        return;
    if (inUse)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
}

bool gameInputBlocked()
{
    return s_inputBlocked.load(std::memory_order_acquire);
}

void handleKeyDown(SDL_Event& event)
{
    // Ignore key-repeat so holding F1 cannot rapidly open and close the dialog.
    if (event.key.repeat)
        return;

    // SDL3 exposes the physical key in `scancode` and the layout-dependent
    // keycode in `key`.  All cases below use SDL_SCANCODE_* constants, so they
    // must be compared with the scancode field.
    switch (event.key.scancode)
    {
#ifdef FITD_DEBUGGER
    case SDL_SCANCODE_GRAVE:
        debuggerVar_debugMenuDisplayed ^= 1;
        break;
#endif
    case SDL_SCANCODE_RETURN:
        // ALT+ENTER toggles fullscreen
        if (event.key.mod & SDL_KMOD_ALT)
        {
            toggleFullscreen();
        }
        break;
    case SDL_SCANCODE_F11:
        toggleFullscreen();
        break;
    default:
        // F9, F10...: the layers above decide (platformHooks.h)
        if (g_onKeyPressed)
            g_onKeyPressed(event.key.scancode);
        break;
    }
}

void readKeyboard(void)
{
    SDL_Event event;
    int size;
    int j;
    const bool *keyboard;

    JoyD = 0;
    Click = 0;
    key = 0;

    while (SDL_PollEvent(&event)) {

        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            handleKeyDown(event);
            break;
        case SDL_EVENT_QUIT:
            cleanupAndExit();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            // Mark that the window was resized so menus can redraw
            g_windowWasResized = true;
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
            handleControllerDeviceEvent(event);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            setControllerInUse(true);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            // Past a dead zone: a resting stick drifts a little
            if (event.gaxis.value > 8000 || event.gaxis.value < -8000)
                setControllerInUse(true);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            setControllerInUse(false);
            break;
        }

    }

#ifdef FITD_DEBUGGER
    debuggerVar_fastForward = false;
#endif

    // The settings window (and the debug item window) owns keyboard, mouse and
    // controller focus while open. Events still reach ImGui above, but none
    // leak into the legacy game inputs.
    // The F1 window opens and closes with the "Options window" controls (F1,
    // Home and Back by default, editable in the controls window), when
    // pressed; not while the controls window waits for a new input
    keyboard = SDL_GetKeyboardState(&size);
    SDL_Gamepad* gamepad = g_controllerState.connected ? g_controllerState.gamepad : nullptr;
    {
        static bool optionsWasHeld = false;
        const bool optionsHeld = controlsActionHeld(GameAction::Options, keyboard, size, gamepad);
        if (optionsHeld && !optionsWasHeld && g_onOptionsAction)
            g_onOptionsAction();
        optionsWasHeld = optionsHeld;
    }

    // The object editor freezes the game but keeps drawing the scene; like
    // the other windows, it keeps every input from the game
    static bool waitRelease = false;
    if (g_isMenuCapturingInput && g_isMenuCapturingInput())
    {
        JoyD = 0;
        Click = 0;
        key = 0;
        waitRelease = true;
        s_inputBlocked.store(true, std::memory_order_release);
        return;
    }

    // The keyboard and the controller as the game's inputs (controls.cpp)
    controlsUpdate(keyboard, size, gamepad);

    // Just closed: nothing reaches the game until everything is released
    if (waitRelease)
    {
        const bool mouseHeld = SDL_GetMouseState(nullptr, nullptr) != 0;
        if (JoyD || key || Click || mouseHeld)
        {
            JoyD = 0;
            Click = 0;
            key = 0;
            return;
        }
        waitRelease = false;
    }
    s_inputBlocked.store(false, std::memory_order_release);

#ifdef FITD_DEBUGGER
    for (j = 0; j < size; j++)
    {
        if (keyboard[j])
        {
            switch (j)
            {
            case SDL_SCANCODE_O:
                debufferVar_topCameraZoom += 100;
                break;
            case SDL_SCANCODE_P:
                debufferVar_topCameraZoom -= 100;
                break;
            case SDL_SCANCODE_T:
                debuggerVar_topCamera = true;
                backgroundMode = backgroundModeEnum_3D;
                break;
            case SDL_SCANCODE_Y:
                debuggerVar_topCamera = false;
                backgroundMode = backgroundModeEnum_2D;
                FlagInitView = 1;
                break;
            case SDL_SCANCODE_C:
                debuggerVar_noHardClip = !debuggerVar_noHardClip;
                break;
            case SDL_SCANCODE_B:
                backgroundMode = backgroundModeEnum_3D;
                break;
            case SDL_SCANCODE_N:
                backgroundMode = backgroundModeEnum_2D;
                break;
            case SDL_SCANCODE_F:
                debuggerVar_fastForward = true;
                break;
            }
        }
    }
    if (debuggerVar_topCamera)
    {
        backgroundMode = backgroundModeEnum_3D;
    }
#endif
}

// ============================================================================
// Controller Support Functions
// ============================================================================

void initController()
{
    // The settings are already loaded (FitdMain, before the
    // launch options); reloading it here would undo them.

    // Initialize SDL gamepad subsystem
    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) < 0)
    {
        printf(INP_ERR "Failed to initialize SDL gamepad subsystem: %s" CON_RESET "\n", SDL_GetError());
        return;
    }

    // Open the first available gamepad
    int numGamepads = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);

    if (gamepads && numGamepads > 0)
    {
        g_controllerState.gamepad = SDL_OpenGamepad(gamepads[0]);
        if (g_controllerState.gamepad)
        {
            g_controllerState.connected = true;
            printf(INP_OK "Controller connected: %s\n", SDL_GetGamepadName(g_controllerState.gamepad));
        }
    }

    if (gamepads)
    {
        SDL_free(gamepads);
    }
}

void shutdownController()
{
    if (g_controllerState.gamepad)
    {
        SDL_CloseGamepad(g_controllerState.gamepad);
        g_controllerState.gamepad = nullptr;
        g_controllerState.connected = false;
    }
}

void handleControllerDeviceEvent(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!g_controllerState.connected)
        {
            g_controllerState.gamepad = SDL_OpenGamepad(event.gdevice.which);
            if (g_controllerState.gamepad)
            {
                g_controllerState.connected = true;
                printf(INP_OK "Controller connected: %s\n", SDL_GetGamepadName(g_controllerState.gamepad));
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_REMOVED:
        if (g_controllerState.gamepad)
        {
            SDL_CloseGamepad(g_controllerState.gamepad);
            g_controllerState.gamepad = nullptr;
            g_controllerState.connected = false;
            printf(INP_TAG "Controller disconnected\n");
        }
        break;
    }
}

