///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game controls shared by the game and the launcher: the actions, the inputs
// that trigger them (several per action: keys, controller buttons, triggers,
// stick directions), their defaults and their text form in aitd_remaster.cfg,
// e.g. "controls.run = key:Left Shift, pad:leftstick, axis:righttrigger+".
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

enum class GameAction
{
    Up,
    Down,
    Left,
    Right,
    Action,          // use, fight, take
    Confirm,         // Enter
    Cancel,          // Esc: pause menu
    Run,
    Options,         // opens and closes the F1 window (not a game input)
    Count
};

static const int kGameActionCount = (int)GameAction::Count;

struct InputBinding
{
    enum class Kind { Key, PadButton, PadAxis };
    Kind kind = Kind::Key;
    int code = 0;          // SDL_Scancode, SDL_GamepadButton or SDL_GamepadAxis
    bool positive = true;  // axes: which direction (triggers only go positive)

    bool operator==(const InputBinding& other) const
    {
        return kind == other.kind && code == other.code && (kind != Kind::PadAxis || positive == other.positive);
    }
};

// Name of the action in aitd_remaster.cfg ("controls.<id>")
inline const char* gameActionId(GameAction action)
{
    static const char* const ids[kGameActionCount] = {
        "up", "down", "left", "right", "action", "confirm", "cancel", "run", "options" };
    return ids[(int)action];
}

inline InputBinding keyBinding(SDL_Scancode key)
{
    InputBinding b; b.kind = InputBinding::Kind::Key; b.code = key; return b;
}
inline InputBinding padButtonBinding(SDL_GamepadButton button)
{
    InputBinding b; b.kind = InputBinding::Kind::PadButton; b.code = button; return b;
}
inline InputBinding padAxisBinding(SDL_GamepadAxis axis, bool positive)
{
    InputBinding b; b.kind = InputBinding::Kind::PadAxis; b.code = axis; b.positive = positive; return b;
}

// The default controls (the same as before the rebuild, plus RT to run)
inline std::vector<InputBinding> defaultBindings(GameAction action)
{
    switch (action)
    {
    case GameAction::Up:
        return { keyBinding(SDL_SCANCODE_UP), keyBinding(SDL_SCANCODE_W),
                 padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_UP), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTY, false) };
    case GameAction::Down:
        return { keyBinding(SDL_SCANCODE_DOWN), keyBinding(SDL_SCANCODE_S),
                 padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_DOWN), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTY, true) };
    case GameAction::Left:
        return { keyBinding(SDL_SCANCODE_LEFT), keyBinding(SDL_SCANCODE_A),
                 padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_LEFT), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTX, false) };
    case GameAction::Right:
        return { keyBinding(SDL_SCANCODE_RIGHT), keyBinding(SDL_SCANCODE_D),
                 padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_RIGHT), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTX, true) };
    case GameAction::Action:
        return { keyBinding(SDL_SCANCODE_SPACE), padButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH) };
    case GameAction::Confirm:
        return { keyBinding(SDL_SCANCODE_RETURN), padButtonBinding(SDL_GAMEPAD_BUTTON_START) };
    case GameAction::Cancel:
        return { keyBinding(SDL_SCANCODE_ESCAPE), padButtonBinding(SDL_GAMEPAD_BUTTON_EAST) };
    case GameAction::Run:
        return { keyBinding(SDL_SCANCODE_LSHIFT), padButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_STICK),
                 padAxisBinding(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, true) };
    case GameAction::Options:
        return { keyBinding(SDL_SCANCODE_F1), keyBinding(SDL_SCANCODE_HOME), padButtonBinding(SDL_GAMEPAD_BUTTON_BACK) };
    default:
        return {};
    }
}

// "key:Left Shift", "pad:leftstick", "axis:righttrigger+" (SDL's own names)
inline std::string bindingToText(const InputBinding& binding)
{
    switch (binding.kind)
    {
    case InputBinding::Kind::Key:
    {
        const char* name = SDL_GetScancodeName((SDL_Scancode)binding.code);
        return std::string("key:") + ((name && name[0]) ? name : std::to_string(binding.code));
    }
    case InputBinding::Kind::PadButton:
    {
        const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton)binding.code);
        return std::string("pad:") + (name ? name : std::to_string(binding.code));
    }
    case InputBinding::Kind::PadAxis:
    {
        const char* name = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)binding.code);
        return std::string("axis:") + (name ? name : std::to_string(binding.code)) + (binding.positive ? "+" : "-");
    }
    }
    return std::string();
}

inline bool bindingFromText(std::string text, InputBinding& binding)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.erase(text.begin());
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.pop_back();
    const size_t colon = text.find(':');
    if (colon == std::string::npos)
        return false;
    const std::string kind = text.substr(0, colon);
    std::string name = text.substr(colon + 1);
    if (kind == "key")
    {
        const SDL_Scancode key = SDL_GetScancodeFromName(name.c_str());
        if (key == SDL_SCANCODE_UNKNOWN)
            return false;
        binding = keyBinding(key);
        return true;
    }
    if (kind == "pad")
    {
        const SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name.c_str());
        if (button == SDL_GAMEPAD_BUTTON_INVALID)
            return false;
        binding = padButtonBinding(button);
        return true;
    }
    if (kind == "axis" && !name.empty())
    {
        const bool positive = (name.back() != '-');
        if (name.back() == '+' || name.back() == '-')
            name.pop_back();
        const SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(name.c_str());
        if (axis == SDL_GAMEPAD_AXIS_INVALID)
            return false;
        binding = padAxisBinding(axis, positive);
        return true;
    }
    return false;
}

inline std::string bindingsToText(const std::vector<InputBinding>& bindings)
{
    std::string text;
    for (const InputBinding& binding : bindings)
    {
        if (!text.empty())
            text += ", ";
        text += bindingToText(binding);
    }
    return text;
}

inline std::vector<InputBinding> bindingsFromText(const std::string& text)
{
    std::vector<InputBinding> bindings;
    size_t start = 0;
    while (start <= text.size())
    {
        size_t comma = text.find(',', start);
        if (comma == std::string::npos)
            comma = text.size();
        InputBinding binding;
        if (bindingFromText(text.substr(start, comma - start), binding))
            bindings.push_back(binding);
        start = comma + 1;
    }
    return bindings;
}
