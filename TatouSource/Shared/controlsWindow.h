///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controls window, shared by the game (F1 window) and the launcher
// (advanced options): each action with its inputs; click one to remove it,
// "+ Add" then press a key, a controller button, a trigger or a stick
// direction; restore the defaults. Header-only (both use ImGui and SDL).
// While waiting for an input, ImGui's keyboard and controller navigation is
// off, so Space, Enter or A do not also press the focused button.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "advancedOptionsText.h"
#include "controlsBindings.h"
#include "ui/menuBase.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct ControlsWindowState
{
    bool open = false;
    bool justOpened = false;
    int capturing = -1;          // action waiting for a new input, -1 none
    bool capturePad = false;     // waiting for a controller input (else a key)
    bool waitRelease = false;    // after a capture: everything must be released first
    std::vector<bool> prevKeys;
    bool prevButtons[SDL_GAMEPAD_BUTTON_COUNT] = {};
    float prevAxes[SDL_GAMEPAD_AXIS_COUNT] = {};
    ImGuiConfigFlags savedNavFlags = 0;
    bool navSuppressed = false;
    bool requestClose = false;   // close at the next frame (B in the launcher)
};

// Short name of an input for the window
inline std::string bindingDisplayName(const InputBinding& binding)
{
    switch (binding.kind)
    {
    case InputBinding::Kind::Key:
    {
        const char* name = SDL_GetScancodeName((SDL_Scancode)binding.code);
        return (name && name[0]) ? name : "?";
    }
    case InputBinding::Kind::PadButton:
        switch (binding.code)
        {
        case SDL_GAMEPAD_BUTTON_SOUTH: return "A";
        case SDL_GAMEPAD_BUTTON_EAST: return "B";
        case SDL_GAMEPAD_BUTTON_WEST: return "X";
        case SDL_GAMEPAD_BUTTON_NORTH: return "Y";
        case SDL_GAMEPAD_BUTTON_BACK: return "Back";
        case SDL_GAMEPAD_BUTTON_GUIDE: return "Guide";
        case SDL_GAMEPAD_BUTTON_START: return "Start";
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return "L3";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return "R3";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "LB";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "RB";
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return "D-pad Up";
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return "D-pad Down";
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return "D-pad Left";
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return "D-pad Right";
        default:
        {
            const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton)binding.code);
            return name ? name : "?";
        }
        }
    case InputBinding::Kind::PadAxis:
        switch (binding.code)
        {
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return "LT";
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return "RT";
        case SDL_GAMEPAD_AXIS_LEFTX: return binding.positive ? "Left stick Right" : "Left stick Left";
        case SDL_GAMEPAD_AXIS_LEFTY: return binding.positive ? "Left stick Down" : "Left stick Up";
        case SDL_GAMEPAD_AXIS_RIGHTX: return binding.positive ? "Right stick Right" : "Right stick Left";
        case SDL_GAMEPAD_AXIS_RIGHTY: return binding.positive ? "Right stick Down" : "Right stick Up";
        default: return "?";
        }
    }
    return "?";
}

inline AdvancedText controlsActionText(GameAction action)
{
    static const AdvancedText texts[kGameActionCount] = {
        AdvancedText::ActionUp, AdvancedText::ActionDown, AdvancedText::ActionLeft, AdvancedText::ActionRight,
        AdvancedText::ActionAction, AdvancedText::ActionConfirm, AdvancedText::ActionCancel,
        AdvancedText::ActionRun, AdvancedText::ActionOptions };
    return texts[(int)action];
}

// Current input state, for detecting a new press
inline void controlsReadInputs(SDL_Gamepad* gamepad, std::vector<bool>& keys, bool* buttons, float* axes)
{
    int count = 0;
    const bool* keyboard = SDL_GetKeyboardState(&count);
    keys.assign(count, false);
    for (int i = 0; i < count; i++)
        keys[i] = keyboard[i];
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
        buttons[b] = gamepad && SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)b);
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++)
        axes[a] = gamepad ? SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)a) / 32767.0f : 0.0f;
}

inline bool controlsAnythingHeld(const std::vector<bool>& keys, const bool* buttons, const float* axes)
{
    for (bool k : keys)
        if (k)
            return true;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
        if (buttons[b])
            return true;
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++)
        if (std::fabs(axes[a]) > 0.5f)
            return true;
    return false;
}

// Looks of the window (the caller pushes its own colours and paddings)
struct ControlsWindowStyle
{
    float scale = 1.0f;             // size scale (the game's F1 window grows with the screen)
    float fontScale = 0.0f;         // font scale, 0 = the same (the launcher makes its fonts at the scale)
    ImVec4 titleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImFont* titleFont = nullptr;
};

// Draws the window while state.open (call every frame). bindings: one list
// per action, edited in place; modernStick: the left stick walks where it
// points (controller.modernStick). Returns true when either changed.
// asPopup: a modal popup inside the caller's popup (the launcher's advanced
// options); otherwise a window of its own (the game's F1 window, hidden
// meanwhile so they never overlap). Never larger than the screen: the list
// scrolls inside.
inline bool drawControlsWindow(ControlsWindowState& state, std::vector<std::vector<InputBinding>>& bindings,
                               bool& modernStick, int language, SDL_Gamepad* gamepad, bool asPopup,
                               const ControlsWindowStyle& style = ControlsWindowStyle())
{
    auto text = [language](AdvancedText t) { return advancedText(t, language); };
    bool changed = false;
    if (bindings.size() != (size_t)kGameActionCount)
        bindings.resize(kGameActionCount);

    ImGuiIO& io = ImGui::GetIO();
    const char* popupId = "##controlsWindow";
    if (state.open && state.justOpened && asPopup)
        ImGui::OpenPopup(popupId);

    // Waiting for an input: no ImGui keyboard/controller navigation
    const bool wantSuppress = state.open && (state.capturing >= 0 || state.waitRelease);
    if (wantSuppress && !state.navSuppressed)
    {
        state.savedNavFlags = io.ConfigFlags & (ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
        io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
        state.navSuppressed = true;
    }
    else if (!wantSuppress && state.navSuppressed)
    {
        io.ConfigFlags |= state.savedNavFlags;
        state.navSuppressed = false;
    }
    if (!state.open)
        return false;

    // Capture: the first key, button, trigger or stick direction pressed
    std::vector<bool> keys;
    bool buttons[SDL_GAMEPAD_BUTTON_COUNT];
    float axes[SDL_GAMEPAD_AXIS_COUNT];
    controlsReadInputs(gamepad, keys, buttons, axes);
    if (state.waitRelease && !controlsAnythingHeld(keys, buttons, axes))
        state.waitRelease = false;
    if (state.capturing >= 0)
    {
        bool done = false;
        InputBinding captured;
        bool haveCapture = false;
        for (size_t k = 0; k < keys.size() && !done; k++)
        {
            const bool wasHeld = (k < state.prevKeys.size()) && state.prevKeys[k];
            if (keys[k] && !wasHeld && (k == SDL_SCANCODE_ESCAPE || !state.capturePad))
            {
                done = true;
                if (k != SDL_SCANCODE_ESCAPE)   // Esc cancels
                {
                    captured = keyBinding((SDL_Scancode)k);
                    haveCapture = true;
                }
            }
        }
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT && !done && state.capturePad; b++)
            if (buttons[b] && !state.prevButtons[b])
            {
                done = true;
                captured = padButtonBinding((SDL_GamepadButton)b);
                haveCapture = true;
            }
        for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT && !done && state.capturePad; a++)
        {
            const bool isTrigger = (a == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || a == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
            const float press = isTrigger ? 0.5f : 0.6f;
            if (std::fabs(axes[a]) > press && std::fabs(state.prevAxes[a]) <= press)
            {
                done = true;
                captured = padAxisBinding((SDL_GamepadAxis)a, isTrigger ? true : axes[a] > 0.0f);
                haveCapture = true;
            }
        }
        if (done)
        {
            if (haveCapture)
            {
                std::vector<InputBinding>& list = bindings[state.capturing];
                if (std::find(list.begin(), list.end(), captured) == list.end())
                {
                    list.push_back(captured);
                    changed = true;
                }
            }
            state.capturing = -1;
            state.waitRelease = true;
        }
    }
    state.prevKeys = keys;
    std::copy(buttons, buttons + SDL_GAMEPAD_BUTTON_COUNT, state.prevButtons);
    std::copy(axes, axes + SDL_GAMEPAD_AXIS_COUNT, state.prevAxes);
    const bool busy = (state.capturing >= 0 || state.waitRelease);

    // Centred, never larger than the screen
    const float width = (std::min)(600.0f * style.scale, io.DisplaySize.x * 0.94f);
    const float height = (std::min)(540.0f * style.scale, io.DisplaySize.y * 0.90f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    if (!asPopup && state.justOpened)
        ImGui::SetNextWindowFocus();
    state.justOpened = false;

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (asPopup)
    {
        if (!ImGui::BeginPopupModal(popupId, nullptr, flags))
        {
            state.open = false;
            return changed;
        }
    }
    else if (!ImGui::Begin(popupId, nullptr, flags))
    {
        ImGui::End();
        return changed;
    }
    ImGui::SetWindowFontScale(style.fontScale > 0.0f ? style.fontScale : style.scale);

    if (style.titleFont)
        ImGui::PushFont(style.titleFont);
    ImGui::TextColored(style.titleColor, "%s", text(AdvancedText::Controls));
    if (style.titleFont)
        ImGui::PopFont();
    ImGui::BeginDisabled(state.capturing >= 0 || state.waitRelease);
    if (ImGui::Checkbox(text(AdvancedText::ModernStick), &modernStick))
        changed = true;
    menu::helpWhenFocused(text(AdvancedText::ModernStickHelp));
    menu::helpMarker(text(AdvancedText::ModernStickHelp));
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", text(AdvancedText::ControlsHelp));
    ImGui::Separator();
    if (state.capturing >= 0)
    {
        ImGui::TextColored(style.titleColor, text(state.capturePad ? AdvancedText::ControlsPressPad : AdvancedText::ControlsPressKey),
                           text(controlsActionText((GameAction)state.capturing)));
        ImGui::Separator();
    }

    // Action list in three columns: the action, its keys and its controller
    // inputs (a click removes one; each column has its own "+ Add", so a key
    // A and the controller's A are never confused). Inputs wrap inside their
    // column when it is full.
    const float footer = ImGui::GetFrameHeightWithSpacing() * 2.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;
    ImGui::BeginChild("##controlsList", ImVec2(0.0f, -footer));
    ImGui::BeginDisabled(busy);
    if (ImGui::BeginTable("##controlsTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn("##action", ImGuiTableColumnFlags_WidthStretch, 0.30f);
        ImGui::TableSetupColumn(text(AdvancedText::ControlsKeyboard), ImGuiTableColumnFlags_WidthStretch, 0.32f);
        ImGui::TableSetupColumn(text(AdvancedText::ControlsController), ImGuiTableColumnFlags_WidthStretch, 0.38f);
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(style.titleColor, "%s", text(AdvancedText::ControlsKeyboard));
        ImGui::TableSetColumnIndex(2);
        ImGui::TextColored(style.titleColor, "%s", text(AdvancedText::ControlsController));

        // A button on the same line if it fits in the column, else below
        auto placeButton = [](const char* label, bool first)
        {
            if (first)
                return;
            const float buttonWidth = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SameLine();
            if (ImGui::GetContentRegionAvail().x < buttonWidth)
                ImGui::NewLine();
        };

        for (int a = 0; a < kGameActionCount; a++)
        {
            ImGui::PushID(a);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(text(controlsActionText((GameAction)a)));

            std::vector<InputBinding>& list = bindings[a];
            int removeIndex = -1;
            for (int column = 0; column < 2; column++)
            {
                const bool padColumn = (column == 1);
                ImGui::TableSetColumnIndex(1 + column);
                ImGui::PushID(column);
                bool first = true;
                for (size_t i = 0; i < list.size(); i++)
                {
                    if ((list[i].kind != InputBinding::Kind::Key) != padColumn)
                        continue;
                    const std::string name = bindingDisplayName(list[i]);
                    placeButton(name.c_str(), first);
                    first = false;
                    ImGui::PushID((int)i);
                    if (ImGui::SmallButton(name.c_str()))
                        removeIndex = (int)i;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", text(AdvancedText::ControlsRemove));
                    ImGui::PopID();
                }
                placeButton(text(AdvancedText::ControlsAdd), first);
                if (ImGui::SmallButton(text(AdvancedText::ControlsAdd)))
                {
                    state.capturing = a;
                    state.capturePad = padColumn;
                    controlsReadInputs(gamepad, state.prevKeys, state.prevButtons, state.prevAxes);
                }
                ImGui::PopID();
            }
            if (removeIndex >= 0)
            {
                list.erase(list.begin() + removeIndex);
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::BeginDisabled(busy);
    if (ImGui::Button(text(AdvancedText::ControlsDefaults), ImVec2(-1.0f, 0.0f)))
    {
        for (int a = 0; a < kGameActionCount; a++)
            bindings[a] = defaultBindings((GameAction)a);
        modernStick = true;
        changed = true;
    }
    if (ImGui::Button(text(AdvancedText::Close), ImVec2(-1.0f, 0.0f))
        || (!busy && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) || state.requestClose)
    {
        state.requestClose = false;
        state.open = false;
        if (asPopup)
            ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    if (asPopup)
        ImGui::EndPopup();
    else
        ImGui::End();
    return changed;
}
