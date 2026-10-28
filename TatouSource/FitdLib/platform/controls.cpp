///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game controls (see controls.h)
///////////////////////////////////////////////////////////////////////////////

#include "controls.h"
#include "common.h"

#include <cmath>

// A trigger counts as pressed past half its travel; a stick direction past
// this much after the dead zone (as before the rebuild)
static const float kTriggerPressed = 0.5f;
static const float kStickPressed = 0.3f;

static std::vector<std::vector<InputBinding>>& bindingTable()
{
    static std::vector<std::vector<InputBinding>> table;
    if (table.empty())
    {
        table.resize(kGameActionCount);
        for (int i = 0; i < kGameActionCount; i++)
            table[i] = defaultBindings((GameAction)i);
    }
    return table;
}

const std::vector<InputBinding>& controlsBindings(GameAction action)
{
    return bindingTable()[(int)action];
}

void controlsSetBindings(GameAction action, const std::vector<InputBinding>& bindings)
{
    bindingTable()[(int)action] = bindings;
}

void controlsResetDefaults()
{
    for (int i = 0; i < kGameActionCount; i++)
        bindingTable()[i] = defaultBindings((GameAction)i);
}

ControlsAnalog& controlsAnalog()
{
    static ControlsAnalog analog;
    return analog;
}

static bool isStickAxis(int axis)
{
    return axis == SDL_GAMEPAD_AXIS_LEFTX || axis == SDL_GAMEPAD_AXIS_LEFTY ||
           axis == SDL_GAMEPAD_AXIS_RIGHTX || axis == SDL_GAMEPAD_AXIS_RIGHTY;
}

// A stick axis from -1 to 1 after the dead zone, sensitivity and inversion
static float stickValue(SDL_Gamepad* gamepad, SDL_GamepadAxis axis)
{
    const ControlsAnalog& analog = controlsAnalog();
    float value = SDL_GetGamepadAxis(gamepad, axis) / 32768.0f;
    if (std::fabs(value) < analog.deadzone)
        return 0.0f;
    const float sign = (value > 0.0f) ? 1.0f : -1.0f;
    value = sign * (std::fabs(value) - analog.deadzone) / (1.0f - analog.deadzone);
    value *= analog.sensitivity;
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    if (analog.invertY && (axis == SDL_GAMEPAD_AXIS_LEFTY || axis == SDL_GAMEPAD_AXIS_RIGHTY))
        value = -value;
    return value;
}

static bool isDirection(GameAction action)
{
    return action == GameAction::Up || action == GameAction::Down ||
           action == GameAction::Left || action == GameAction::Right;
}

static bool bindingHeld(const InputBinding& binding, GameAction action, const bool* keyboard, int keyCount, SDL_Gamepad* gamepad)
{
    switch (binding.kind)
    {
    case InputBinding::Kind::Key:
        return binding.code >= 0 && binding.code < keyCount && keyboard[binding.code];
    case InputBinding::Kind::PadButton:
        return gamepad && SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)binding.code);
    case InputBinding::Kind::PadAxis:
    {
        if (!gamepad)
            return false;
        if (isStickAxis(binding.code))
        {
            if (isDirection(action) && !controlsAnalog().stickMovement)
                return false;
            const float value = stickValue(gamepad, (SDL_GamepadAxis)binding.code);
            return binding.positive ? (value > kStickPressed) : (value < -kStickPressed);
        }
        // Trigger: 0 released to 32767 fully pressed
        const float value = SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)binding.code) / 32767.0f;
        return binding.positive ? (value > kTriggerPressed) : (value < -kTriggerPressed);
    }
    }
    return false;
}

static bool actionHeld(GameAction action, const bool* keyboard, int keyCount, SDL_Gamepad* gamepad)
{
    for (const InputBinding& binding : controlsBindings(action))
        if (bindingHeld(binding, action, keyboard, keyCount, gamepad))
            return true;
    return false;
}

bool controlsActionHeld(GameAction action, const bool* keyboard, int keyCount, SDL_Gamepad* gamepad)
{
    return actionHeld(action, keyboard, keyCount, gamepad);
}

void controlsUpdate(const bool* keyboard, int keyCount, SDL_Gamepad* gamepad)
{
    auto held = [&](GameAction action) { return actionHeld(action, keyboard, keyCount, gamepad); };

    // JoyD bits: 1 up, 2 down, 4 left, 8 right, 0x40 run (core/track.cpp)
    if (held(GameAction::Up))             JoyD |= 0x01;
    if (held(GameAction::Down))           JoyD |= 0x02;
    if (held(GameAction::Left))           JoyD |= 0x04;
    if (held(GameAction::Right))          JoyD |= 0x08;
    if (held(GameAction::Run))            JoyD |= 0x40;

    if (held(GameAction::Action))
        Click = 1;

    // The 1992 key codes: Esc, Enter (Enter wins over Esc, as before)
    if (held(GameAction::Cancel))  key = 0x1B;
    if (held(GameAction::Confirm)) key = 0x1C;
}
