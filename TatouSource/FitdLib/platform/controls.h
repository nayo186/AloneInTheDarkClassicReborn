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
    bool stickMovement = true;   // stick directions move the character
};
// The sticks' settings (dead zone, sensitivity, inverted Y, stick movement)
ControlsAnalog& controlsAnalog();

// Each frame (input.cpp): sets JoyD, key and Click from the keyboard state and
// the controller (null when none is connected)
void controlsUpdate(const bool* keyboard, int keyCount, SDL_Gamepad* gamepad);

// Whether any input of the action is held now (GameAction::Options: input.cpp)
bool controlsActionHeld(GameAction action, const bool* keyboard, int keyCount, SDL_Gamepad* gamepad);
