///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game controls: the bindings in use (actions and inputs: Shared/
// controlsBindings.h) and, each frame, the keyboard and controller turned
// into the game's 1992 inputs (JoyD, key, Click).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "controlsBindings.h"

// The inputs of an action (starts with the defaults)
const std::vector<InputBinding>& controlsBindings(GameAction action);
// Replaces the inputs of an action
void controlsSetBindings(GameAction action, const std::vector<InputBinding>& bindings);
// Puts every action back to its default inputs
void controlsResetDefaults();

// Analog stick and trigger settings (aitd_remaster.cfg controller.*)
struct ControlsAnalog
{
    float deadzone = 0.15f;      // stick values below it count as centred
    float sensitivity = 1.0f;
    bool invertY = false;
    bool modernStick = true;     // the left stick walks where it points, seen from the camera (controller.modernStick)
};
// The sticks' settings (dead zone, sensitivity, inverted Y, modern left stick)
ControlsAnalog& controlsAnalog();

// The left stick for the modern movement (core/track.cpp), read each frame.
// The stick still sets JoyD's tank directions too, for the menus and for
// fighting; the core replaces them while it walks the character.
struct ControlsLeftStick
{
    bool active = false;      // modern stick on, a controller and the stick pushed
    float x = 0.0f;           // -1 left .. 1 right
    float y = 0.0f;           // -1 up .. 1 down
    int otherDirections = 0;  // JoyD direction bits from every other input (keys, d-pad...)
};
// The left stick as read at the last controlsUpdate
const ControlsLeftStick& controlsLeftStick();

// Each frame (input.cpp): sets JoyD, key and Click from the keyboard state and
// the controller (null when none is connected)
void controlsUpdate(const bool* keyboard, int keyCount, SDL_Gamepad* gamepad);

// Whether any input of the action is held now (GameAction::Options: input.cpp)
bool controlsActionHeld(GameAction action, const bool* keyboard, int keyCount, SDL_Gamepad* gamepad);
