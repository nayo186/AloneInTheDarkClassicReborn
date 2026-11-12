///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controls window, shared by the game (F1 window) and the launcher
// (advanced options): each action with its inputs; click one to remove it,
// "+ Add" then press a key, a controller button, a trigger or a stick
// direction; restore the defaults. Two tabs: the keyboard (each action and
// its keys) and the controller, a drawing of it where each part says what it
// does and a press on it changes that (AITD3's map is one of its jobs); on
// Android, a list like the keyboard's (the drawing is too small to touch on a
// phone). Header-only (both use
// ImGui and SDL).
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
    int padChoice = -1;          // the part of the controller whose list is open
    int tab = 0;                 // the tab shown: 0 keyboard, 1 controller
    int tabRequest = -1;         // the tab LB / RB asked for, -1 none
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
        AdvancedText::ActionRun, AdvancedText::ActionMap, AdvancedText::ActionOptions };
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
    ImTextureID padPicture = 0;      // the controller drawing (ui/controllerPicture.h), as a texture of the host
    float padPictureAspect = 0.0f;   // its height / width
};

// ── The controller drawing ───────────────────────────────────────
// Each part of the controller in the picture (ui/controllerPicture.h): what
// it is, where it is (0-1 of the picture's width and height, measured once),
// the size of its ring and the side of its text. A button, trigger or stick
// press does one action; a stick or the D-pad moves (its four directions).
struct PadPart
{
    AdvancedText name;
    const char* letter;          // drawn on the face buttons, else nullptr
    bool movement;               // stick or D-pad: its four directions
    InputBinding input;          // a button or a trigger
    InputBinding directions[4];  // up, down, left, right (movement)
    float x, y;
    float ring;                  // radius, in widths of the picture
    bool pictureButton;          // pressed on the drawing too (not a stick's press: same place)
};

// The parts in the order of their texts: the left column, then the right one
static const int kPadPartsLeft = 6;
inline const std::vector<PadPart>& padParts()
{
    static const std::vector<PadPart> parts = {
        // Left column, from the top
        { AdvancedText::PadNone, "LT", false, padAxisBinding(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, true), {}, 0.196f, 0.073f, 0.040f, true },
        { AdvancedText::PadNone, "LB", false, padButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER), {}, 0.219f, 0.202f, 0.045f, true },
        { AdvancedText::PadLeftStick, nullptr, true, {},
          { padAxisBinding(SDL_GAMEPAD_AXIS_LEFTY, false), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTY, true),
            padAxisBinding(SDL_GAMEPAD_AXIS_LEFTX, false), padAxisBinding(SDL_GAMEPAD_AXIS_LEFTX, true) }, 0.208f, 0.518f, 0.064f, true },
        { AdvancedText::PadLeftStickPress, nullptr, false, padButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_STICK), {}, 0.208f, 0.518f, 0.040f, false },
        { AdvancedText::PadView, nullptr, false, padButtonBinding(SDL_GAMEPAD_BUTTON_BACK), {}, 0.418f, 0.513f, 0.024f, true },
        { AdvancedText::PadDpad, nullptr, true, {},
          { padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_UP), padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_DOWN),
            padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_LEFT), padButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) }, 0.358f, 0.720f, 0.073f, true },
        // Right column, from the top
        { AdvancedText::PadNone, "RT", false, padAxisBinding(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, true), {}, 0.790f, 0.073f, 0.040f, true },
        { AdvancedText::PadNone, "RB", false, padButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER), {}, 0.773f, 0.202f, 0.045f, true },
        { AdvancedText::PadNone, "Y", false, padButtonBinding(SDL_GAMEPAD_BUTTON_NORTH), {}, 0.790f, 0.404f, 0.036f, true },
        { AdvancedText::PadNone, "B", false, padButtonBinding(SDL_GAMEPAD_BUTTON_EAST), {}, 0.862f, 0.503f, 0.036f, true },
        { AdvancedText::PadNone, "X", false, padButtonBinding(SDL_GAMEPAD_BUTTON_WEST), {}, 0.710f, 0.507f, 0.036f, true },
        { AdvancedText::PadNone, "A", false, padButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH), {}, 0.783f, 0.610f, 0.036f, true },
        { AdvancedText::PadMenu, nullptr, false, padButtonBinding(SDL_GAMEPAD_BUTTON_START), {}, 0.581f, 0.513f, 0.024f, true },
        { AdvancedText::PadRightStick, nullptr, true, {},
          { padAxisBinding(SDL_GAMEPAD_AXIS_RIGHTY, false), padAxisBinding(SDL_GAMEPAD_AXIS_RIGHTY, true),
            padAxisBinding(SDL_GAMEPAD_AXIS_RIGHTX, false), padAxisBinding(SDL_GAMEPAD_AXIS_RIGHTX, true) }, 0.642f, 0.774f, 0.064f, true },
        { AdvancedText::PadRightStickPress, nullptr, false, padButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_STICK), {}, 0.642f, 0.774f, 0.040f, false },
    };
    return parts;
}

// The actions a button can do, as the list offers them (-1: none)
static const int kPadChoices[] = { (int)GameAction::Action, (int)GameAction::Confirm, (int)GameAction::Cancel,
                                   (int)GameAction::Run, (int)GameAction::Map, (int)GameAction::Options, -1 };

// What an action is called on the drawing (-1: not used)
inline AdvancedText padActionText(int action)
{
    switch (action)
    {
    case (int)GameAction::Action: return AdvancedText::PadAction;
    case (int)GameAction::Confirm: return AdvancedText::PadConfirm;
    case (int)GameAction::Cancel: return AdvancedText::PadCancel;
    case (int)GameAction::Run: return AdvancedText::ActionRun;
    case (int)GameAction::Options: return AdvancedText::PadOptions;
    case (int)GameAction::Map: return AdvancedText::ActionMap;
    case (int)GameAction::Up: return AdvancedText::ActionUp;
    case (int)GameAction::Down: return AdvancedText::ActionDown;
    case (int)GameAction::Left: return AdvancedText::ActionLeft;
    case (int)GameAction::Right: return AdvancedText::ActionRight;
    default: return AdvancedText::PadNone;
    }
}

inline bool padHas(const std::vector<InputBinding>& list, const InputBinding& input)
{
    return std::find(list.begin(), list.end(), input) != list.end();
}

// The action of a button (the first that has it), -1 none
inline int padActionOf(const std::vector<std::vector<InputBinding>>& bindings, const InputBinding& input)
{
    for (int a = 0; a < kGameActionCount; a++)
    {
        if (padHas(bindings[a], input))
            return a;
    }
    return -1;
}

// A stick or the D-pad moves when its four directions are the four of moving
inline bool padMoves(const std::vector<std::vector<InputBinding>>& bindings, const PadPart& part)
{
    for (int d = 0; d < 4; d++)
        if (!padHas(bindings[(int)GameAction::Up + d], part.directions[d]))
            return false;
    return true;
}

inline bool padUsed(const std::vector<std::vector<InputBinding>>& bindings, const PadPart& part)
{
    return part.movement ? padMoves(bindings, part) : padActionOf(bindings, part.input) >= 0;
}

inline void padRemove(std::vector<std::vector<InputBinding>>& bindings, const InputBinding& input)
{
    for (std::vector<InputBinding>& list : bindings)
        list.erase(std::remove(list.begin(), list.end(), input), list.end());
}

// Gives a part its new job: an action (or none) to a button; moving (or
// not) to a stick or the D-pad. What it did before is taken away.
inline void padAssign(std::vector<std::vector<InputBinding>>& bindings, const PadPart& part, int choice)
{
    if (part.movement)
    {
        for (int d = 0; d < 4; d++)
        {
            padRemove(bindings, part.directions[d]);
            if (choice >= 0)
                bindings[(int)GameAction::Up + d].push_back(part.directions[d]);
        }
        return;
    }
    padRemove(bindings, part.input);
    if (choice >= 0)
        bindings[choice].push_back(part.input);
}

// The drawing is one controller (Xbox's layout); a PlayStation or Nintendo
// controller connected gets its own names on it. The buttons are the same
// by place (SDL calls them south, east...), so the controls do not change.
enum class PadFamily { Xbox, PlayStation, Nintendo };

inline PadFamily padFamily(SDL_Gamepad* gamepad)
{
    switch (gamepad ? SDL_GetGamepadType(gamepad) : SDL_GAMEPAD_TYPE_UNKNOWN)
    {
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5:
        return PadFamily::PlayStation;
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return PadFamily::Nintendo;
    default:
        return PadFamily::Xbox;
    }
}

// The name of a part on that controller: its letters (LT, A...) or its name
inline const char* padPartName(const PadPart& part, int language, PadFamily family, SDL_Gamepad* gamepad)
{
    if (part.movement)
        return advancedText(part.name, language);
    const bool button = part.input.kind == InputBinding::Kind::PadButton;
    const bool trigger = part.input.kind == InputBinding::Kind::PadAxis;
    const int code = part.input.code;
    if (family == PadFamily::PlayStation)
    {
        if (trigger)
            return code == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? "L2" : "R2";
        switch (button ? code : -1)
        {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "L1";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "R1";
        case SDL_GAMEPAD_BUTTON_SOUTH: return advancedText(AdvancedText::PadCross, language);
        case SDL_GAMEPAD_BUTTON_EAST: return advancedText(AdvancedText::PadCircle, language);
        case SDL_GAMEPAD_BUTTON_WEST: return advancedText(AdvancedText::PadSquare, language);
        case SDL_GAMEPAD_BUTTON_NORTH: return advancedText(AdvancedText::PadTriangle, language);
        case SDL_GAMEPAD_BUTTON_BACK: return SDL_GetGamepadType(gamepad) == SDL_GAMEPAD_TYPE_PS5 ? "Create" : "Share";
        case SDL_GAMEPAD_BUTTON_START: return "Options";
        }
    }
    else if (family == PadFamily::Nintendo)
    {
        if (trigger)
            return code == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? "ZL" : "ZR";
        switch (button ? code : -1)
        {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "L";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "R";
        case SDL_GAMEPAD_BUTTON_SOUTH: return "B";
        case SDL_GAMEPAD_BUTTON_EAST: return "A";
        case SDL_GAMEPAD_BUTTON_WEST: return "Y";
        case SDL_GAMEPAD_BUTTON_NORTH: return "X";
        case SDL_GAMEPAD_BUTTON_BACK: return "-";
        case SDL_GAMEPAD_BUTTON_START: return "+";
        }
    }
    return part.name == AdvancedText::PadNone ? part.letter : advancedText(part.name, language);
}

// The mark on a face button over the drawing: its letter, or PlayStation's
// shapes (cross, circle, square, triangle), drawn in white
inline void padFaceMark(ImDrawList* draw, ImVec2 centre, float size, const PadPart& part, PadFamily family)
{
    const ImU32 white = IM_COL32(255, 255, 255, 235);
    const int code = part.input.code;
    if (family == PadFamily::PlayStation)
    {
        const float r = size * 0.42f;
        const float thickness = (std::max)(1.5f, size * 0.12f);
        if (code == SDL_GAMEPAD_BUTTON_SOUTH)
        {
            draw->AddLine(ImVec2(centre.x - r, centre.y - r), ImVec2(centre.x + r, centre.y + r), white, thickness);
            draw->AddLine(ImVec2(centre.x - r, centre.y + r), ImVec2(centre.x + r, centre.y - r), white, thickness);
        }
        else if (code == SDL_GAMEPAD_BUTTON_EAST)
            draw->AddCircle(centre, r, white, 0, thickness);
        else if (code == SDL_GAMEPAD_BUTTON_WEST)
            draw->AddRect(ImVec2(centre.x - r * 0.85f, centre.y - r * 0.85f), ImVec2(centre.x + r * 0.85f, centre.y + r * 0.85f), white, 0.0f, 0, thickness);
        else
            draw->AddTriangle(ImVec2(centre.x, centre.y - r), ImVec2(centre.x + r, centre.y + r * 0.75f),
                ImVec2(centre.x - r, centre.y + r * 0.75f), white, thickness);
        return;
    }
    static const char* const xbox[] = { "A", "B", "X", "Y" };
    static const char* const nintendo[] = { "B", "A", "Y", "X" };
    const int index = code == SDL_GAMEPAD_BUTTON_SOUTH ? 0 : code == SDL_GAMEPAD_BUTTON_EAST ? 1 : code == SDL_GAMEPAD_BUTTON_WEST ? 2 : 3;
    const char* letter = family == PadFamily::Nintendo ? nintendo[index] : xbox[index];
    const ImVec2 textSize = ImGui::CalcTextSize(letter);
    draw->AddText(ImVec2(centre.x - textSize.x * 0.5f, centre.y - textSize.y * 0.5f), white, letter);
}

// What the part does now, as its text says it
inline const char* padPartAction(const std::vector<std::vector<InputBinding>>& bindings, const PadPart& part, int language)
{
    if (part.movement)
        return advancedText(padMoves(bindings, part) ? AdvancedText::PadMove : AdvancedText::PadNone, language);
    return advancedText(padActionText(padActionOf(bindings, part.input)), language);
}

// The drawing with its two columns of texts, in the space left in the window.
// A press on a part or on its text opens the list of what it can do.
inline bool drawPadTab(ControlsWindowState& state, std::vector<std::vector<InputBinding>>& bindings,
                       const ControlsWindowStyle& style, int language, SDL_Gamepad* gamepad, ImVec2 size)
{
    bool changed = false;
    const PadFamily family = padFamily(gamepad);
    const std::vector<PadPart>& parts = padParts();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float lineH = ImGui::GetTextLineHeight();
    const float labelH = lineH * 2.0f + 2.0f;

    // The picture in the middle, as big as fits next to the two columns
    float longest = 0.0f;
    for (int choice : kPadChoices)
        longest = (std::max)(longest, ImGui::CalcTextSize(advancedText(padActionText(choice), language)).x);
    for (AdvancedText name : { AdvancedText::PadMove, AdvancedText::PadLeftStickPress, AdvancedText::PadRightStickPress })
        longest = (std::max)(longest, ImGui::CalcTextSize(advancedText(name, language)).x);
    const float columnW = (std::min)(longest + lineH, size.x * 0.3f);
    float pictureW = (std::max)(size.x - columnW * 2.0f - lineH * 2.0f, size.x * 0.3f);
    const float aspect = style.padPictureAspect > 0.0f ? style.padPictureAspect : 0.616f;
    pictureW = (std::min)(pictureW, size.y / aspect);
    const float pictureH = pictureW * aspect;
    const ImVec2 pictureMin(origin.x + (size.x - pictureW) * 0.5f, origin.y + (size.y - pictureH) * 0.5f);
    if (style.padPicture)
        draw->AddImage(style.padPicture, pictureMin, ImVec2(pictureMin.x + pictureW, pictureMin.y + pictureH));
    auto at = [&](const PadPart& part) { return ImVec2(pictureMin.x + part.x * pictureW, pictureMin.y + part.y * pictureH); };

    // The texts: each column spread over the height, top to bottom
    const int rightCount = (int)parts.size() - kPadPartsLeft;
    const float top = origin.y + labelH * 0.5f;
    const float bottom = origin.y + size.y - labelH * 0.5f;
    const ImU32 used = menu::kGold, unused = (menu::kTextDisabled & 0x00FFFFFF) | (200u << 24);
    int pressed = -1;
    for (int i = 0; i < (int)parts.size(); i++)
    {
        const PadPart& part = parts[i];
        const bool right = i >= kPadPartsLeft;
        const int index = right ? i - kPadPartsLeft : i;
        const int count = right ? rightCount : kPadPartsLeft;
        const float y = count > 1 ? top + (bottom - top) * index / (count - 1) : (top + bottom) * 0.5f;
        const bool isUsed = padUsed(bindings, part);
        const ImU32 lineColour = isUsed ? used : unused;

        // The text, its line to the part and the part's ring
        const float textX = right ? origin.x + size.x - columnW : origin.x;
        const float elbowX = right ? textX - lineH * 0.6f : textX + columnW + lineH * 0.6f;
        const ImVec2 point = at(part);
        draw->AddLine(point, ImVec2(elbowX, y), lineColour, 1.0f);
        draw->AddLine(ImVec2(elbowX, y), ImVec2(right ? textX - 2.0f : textX + columnW + 2.0f, y), lineColour, 1.0f);
        draw->AddCircleFilled(point, 3.0f, lineColour);
        if (isUsed && part.ring > 0.0f)
            draw->AddCircle(point, part.ring * pictureW, (used & 0x00FFFFFF) | (230u << 24), 0, 2.0f);

        ImGui::SetCursorScreenPos(ImVec2(textX, y - labelH * 0.5f));
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##padText", ImVec2(columnW, labelH)))
            pressed = i;
        const bool hovered = ImGui::IsItemHovered() || (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible);
        menu::navCursor(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 4.0f);
        const char* name = padPartName(part, language, family, gamepad);
        const char* action = padPartAction(bindings, part, language);
        const float nameW = ImGui::CalcTextSize(name).x, actionW = ImGui::CalcTextSize(action).x;
        draw->AddText(ImVec2(right ? textX : textX + columnW - nameW, y - lineH), hovered ? menu::kGoldLight : menu::kTextDim, name);
        draw->AddText(ImVec2(right ? textX : textX + columnW - actionW, y + 1.0f),
            hovered ? menu::kGoldLight : (isUsed ? menu::kText : menu::kTextDisabled), action);
        if (part.pictureButton)
        {
            const float r = (std::max)(part.ring * pictureW, lineH * 0.6f);
            ImGui::SetCursorScreenPos(ImVec2(point.x - r, point.y - r));
            if (ImGui::InvisibleButton("##padPart", ImVec2(r * 2.0f, r * 2.0f), ImGuiButtonFlags_None))
                pressed = i;
            if (ImGui::IsItemHovered())
                draw->AddCircle(point, r, menu::kGoldLight, 0, 1.5f);
        }
        ImGui::PopID();
    }
    // The marks of the face buttons, over the picture
    for (const PadPart& part : parts)
        if (part.letter && part.letter[1] == 0)
            padFaceMark(draw, at(part), ImGui::GetTextLineHeight(), part, family);

    if (pressed >= 0)
    {
        state.padChoice = pressed;
        ImGui::OpenPopup("##padChoice");
    }
    if (ImGui::BeginPopup("##padChoice"))
    {
        const PadPart& part = parts[(std::max)(0, (std::min)(state.padChoice, (int)parts.size() - 1))];
        ImGui::TextColored(style.titleColor, "%s", padPartName(part, language, family, gamepad));
        ImGui::Separator();
        if (part.movement)
        {
            const bool moves = padMoves(bindings, part);
            if (ImGui::Selectable(advancedText(AdvancedText::PadMove, language), moves) && !moves)
            {
                padAssign(bindings, part, 0);
                changed = true;
            }
            if (ImGui::Selectable(advancedText(AdvancedText::PadNone, language), !moves) && moves)
            {
                padAssign(bindings, part, -1);
                changed = true;
            }
        }
        else
        {
            const int current = padActionOf(bindings, part.input);
            for (int choice : kPadChoices)
                if (ImGui::Selectable(advancedText(padActionText(choice), language), current == choice) && current != choice)
                {
                    padAssign(bindings, part, choice);
                    changed = true;
                }
        }
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(size);
    return changed;
}

// The list of each action with its keys (pad false) or its controller
// inputs (pad true): a press on one removes it, "+ Add" waits for a new one.
// The controller tab on Android (the drawing is too small on a phone).
inline bool drawBindingList(ControlsWindowState& state, std::vector<std::vector<InputBinding>>& bindings,
                            const ControlsWindowStyle& style, int language, SDL_Gamepad* gamepad, bool pad,
                            bool busy, float footer)
{
    auto text = [language](AdvancedText t) { return advancedText(t, language); };
    bool changed = false;
    ImGui::TextWrapped("%s", text(AdvancedText::ControlsHelp));
    if (state.capturing >= 0)
        ImGui::TextColored(style.titleColor, text(state.capturePad ? AdvancedText::ControlsPressPad : AdvancedText::ControlsPressKey),
                           text(controlsActionText((GameAction)state.capturing)));
    ImGui::Separator();
    ImGui::BeginChild(pad ? "##padList" : "##controlsList", ImVec2(0.0f, -footer));
    ImGui::BeginDisabled(busy);
    if (ImGui::BeginTable("##keysTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn("##action", ImGuiTableColumnFlags_WidthStretch, 0.4f);
        ImGui::TableSetupColumn("##keys", ImGuiTableColumnFlags_WidthStretch, 0.6f);
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
            ImGui::TableSetColumnIndex(1);
            std::vector<InputBinding>& list = bindings[a];
            int removeIndex = -1;
            bool first = true;
            for (size_t i = 0; i < list.size(); i++)
            {
                if ((list[i].kind != InputBinding::Kind::Key) != pad)
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
                state.capturePad = pad;
                controlsReadInputs(gamepad, state.prevKeys, state.prevButtons, state.prevAxes);
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
    return changed;
}

// Draws the window while state.open (call every frame). bindings: one list
// per action, edited in place; modernStick: the left stick walks where it
// points (controller.modernStick); vibration: the controller vibrates when
// hit, dying or shooting (controller.vibration). Returns true when any changed.
// asPopup: a modal popup inside the caller's popup (the launcher's advanced
// options); otherwise a window of its own (the game's F1 window, hidden
// meanwhile so they never overlap). Never larger than the screen: the list
// scrolls inside.
inline bool drawControlsWindow(ControlsWindowState& state, std::vector<std::vector<InputBinding>>& bindings,
                               bool& modernStick, bool& vibration, int language, SDL_Gamepad* gamepad, bool asPopup,
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

    // Centred, never larger than the screen (wide, for the controller drawing)
    const float width = (std::min)(960.0f * style.scale, io.DisplaySize.x * 0.96f);
    const float height = (std::min)(640.0f * style.scale, io.DisplaySize.y * 0.92f);
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
    const float footer = ImGui::GetFrameHeightWithSpacing() * 2.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;
    // LB / RB switch between the tabs, as the launcher's games (not while
    // an input is awaited or a button's list is open)
    if (!busy && state.padChoice < 0
        && (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)))
        state.tabRequest = 1 - state.tab;
    if (ImGui::BeginTabBar("##controlsTabs"))
    {
        if (ImGui::BeginTabItem(text(AdvancedText::KeyboardTab), nullptr,
            state.tabRequest == 0 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            state.tab = 0;
            if (drawBindingList(state, bindings, style, language, gamepad, false, busy, footer))
                changed = true;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(text(AdvancedText::ControllerTab), nullptr,
            state.tabRequest == 1 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            state.tab = 1;
            ImGui::BeginDisabled(busy);
            if (ImGui::Checkbox(text(AdvancedText::ModernStick), &modernStick))
                changed = true;
            menu::helpMarker(text(AdvancedText::ModernStickHelp));
#ifndef __ANDROID__
            ImGui::SameLine(0.0f, ImGui::GetFontSize() * 2.0f);   // on a phone, one under the other
#endif
            if (ImGui::Checkbox(text(AdvancedText::Vibration), &vibration))
                changed = true;
            menu::helpMarker(text(AdvancedText::VibrationHelp));
#ifdef __ANDROID__
            ImGui::EndDisabled();
            // On a phone the drawing is too small to touch: the list of each
            // action with its buttons, as before C73
            if (drawBindingList(state, bindings, style, language, gamepad, true, busy, footer))
                changed = true;
#else
            ImGui::TextDisabled("%s", text(AdvancedText::PadHelp));
            ImGui::EndDisabled();
            ImVec2 area = ImGui::GetContentRegionAvail();
            area.y -= footer;
            ImGui::BeginChild("##padArea", area, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (drawPadTab(state, bindings, style, language, gamepad, ImGui::GetContentRegionAvail()))
                changed = true;
            ImGui::EndChild();
#endif
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    state.tabRequest = -1;

    ImGui::Separator();
    ImGui::BeginDisabled(busy);
    if (ImGui::Button(text(AdvancedText::ControlsDefaults), ImVec2(-1.0f, 0.0f)))
    {
        for (int a = 0; a < kGameActionCount; a++)
            bindings[a] = defaultBindings((GameAction)a);
        modernStick = true;
        vibration = true;
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
