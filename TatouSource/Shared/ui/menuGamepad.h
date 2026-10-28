///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controller in the menus, shared by the launcher and the game. ImGui's
// SDL3 backend is set to manual mode (no controller of its own) and
// feedGamepad() gives ImGui the controller every frame, translated as in
// other games' menus:
//  - left stick: moves the cursor, like the d-pad;
//  - right stick: scrolls the window under the cursor (ImGui scrolls with
//    what it calls the left stick, so the right one is given as that);
//  - A, B, the rest of the buttons and the triggers as they are (not X and
//    Y: ImGui would open its window switcher or text input with them).
// Also: B as "back" (BackButton) and whether a list or a help popup is open.
// Header-only; needs imgui_internal.h for the popup stack.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>

namespace menu
{

// Call after the backend's NewFrame and before ImGui::NewFrame. gamepad: the
// controller in use, or nullptr (then ImGui has none)
inline void feedGamepad(SDL_Gamepad* gamepad)
{
    ImGuiIO& io = ImGui::GetIO();
    if (!gamepad)
        return;   // the backend already told ImGui there is none
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    auto axis = [gamepad](SDL_GamepadAxis a) { return SDL_GetGamepadAxis(gamepad, a) / 32767.0f; };
    auto pressed = [gamepad](SDL_GamepadButton b) { return SDL_GetGamepadButton(gamepad, b); };

    // Left stick as the d-pad, with a little hysteresis so it does not flicker
    static bool stickHeld[4] = {};
    auto stickDirection = [](int index, float value)
    {
        stickHeld[index] = stickHeld[index] ? (value > 0.35f) : (value > 0.55f);
        return stickHeld[index];
    };
    const float lx = axis(SDL_GAMEPAD_AXIS_LEFTX), ly = axis(SDL_GAMEPAD_AXIS_LEFTY);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft, pressed(SDL_GAMEPAD_BUTTON_DPAD_LEFT) || stickDirection(0, -lx));
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight, pressed(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || stickDirection(1, lx));
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp, pressed(SDL_GAMEPAD_BUTTON_DPAD_UP) || stickDirection(2, -ly));
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown, pressed(SDL_GAMEPAD_BUTTON_DPAD_DOWN) || stickDirection(3, ly));

    io.AddKeyEvent(ImGuiKey_GamepadStart, pressed(SDL_GAMEPAD_BUTTON_START));
    io.AddKeyEvent(ImGuiKey_GamepadBack, pressed(SDL_GAMEPAD_BUTTON_BACK));
    // X and Y are not given: ImGui keeps them for its window switcher (held X
    // showed an empty "Untitled" list) and for typing text, neither used here
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight, pressed(SDL_GAMEPAD_BUTTON_EAST));
    io.AddKeyEvent(ImGuiKey_GamepadFaceDown, pressed(SDL_GAMEPAD_BUTTON_SOUTH));
    io.AddKeyEvent(ImGuiKey_GamepadL1, pressed(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
    io.AddKeyEvent(ImGuiKey_GamepadR1, pressed(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    io.AddKeyEvent(ImGuiKey_GamepadL3, pressed(SDL_GAMEPAD_BUTTON_LEFT_STICK));
    io.AddKeyEvent(ImGuiKey_GamepadR3, pressed(SDL_GAMEPAD_BUTTON_RIGHT_STICK));
    const float l2 = axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER), r2 = axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadL2, l2 > 0.1f, l2);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadR2, r2 > 0.1f, r2);

    // Right stick: scrolling (outside a dead zone, faster the further it goes)
    auto scroll = [&io](ImGuiKey key, float value)
    {
        const float deadZone = 0.25f;
        const float amount = value > deadZone ? (value - deadZone) / (1.0f - deadZone) : 0.0f;
        io.AddKeyAnalogEvent(key, amount > 0.0f, amount);
    };
    const float rx = axis(SDL_GAMEPAD_AXIS_RIGHTX), ry = axis(SDL_GAMEPAD_AXIS_RIGHTY);
    scroll(ImGuiKey_GamepadLStickLeft, -rx);
    scroll(ImGuiKey_GamepadLStickRight, rx);
    scroll(ImGuiKey_GamepadLStickUp, -ry);
    scroll(ImGuiKey_GamepadLStickDown, ry);
}

// A drop-down list or a help popup is open on top (not a modal window, such
// as the launcher's advanced options): B then only closes it
inline bool listOrHelpOpen()
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.empty())
        return false;
    ImGuiWindow* top = g.OpenPopupStack.back().Window;
    return !top || !(top->Flags & ImGuiWindowFlags_Modal);
}

// B as "back": true on the frame it is released after a press that did not
// only close a list or a help popup (ImGui closes those on the press). On
// release, so whatever B means behind the menu never sees it held.
struct BackButton
{
    bool pending = false;
    bool listWasOpen = false;

    // Call once per frame, inside the ImGui frame. enabled: the menu is open
    // and B may go back now (not while waiting for a new input)
    bool released(bool enabled)
    {
        if (enabled && !listWasOpen && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
            pending = true;
        bool result = false;
        if (pending && !ImGui::IsKeyDown(ImGuiKey_GamepadFaceRight))
        {
            pending = false;
            result = enabled;
        }
        listWasOpen = listOrHelpOpen();
        return result;
    }
};

} // namespace menu
