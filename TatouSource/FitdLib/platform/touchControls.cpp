///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// On-screen controls for touch screens: a virtual Xbox controller
// (touchControls.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "touchControls.h"
#include "controls.h"
#include "bgfxGlue.h"
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <mutex>

namespace
{

// Sizes in density units (1 = 1/160 inch on a phone: SDL_GetWindowDisplayScale)
const float kMargin = 16.0f;
const float kStickRadius = 50.0f;         // the ring the finger moves in
const float kKnobRadius = 21.0f;
const float kPadCell = 36.0f;             // each arm of the D-pad
const float kButtonRadius = 26.0f;        // A B X Y
const float kButtonSpacing = 54.0f;       // from the centre of the diamond to each one
const float kPillWidth = 92.0f, kPillHeight = 32.0f;   // Back, Start, RT
const float kHitGrow = 1.3f;              // a touch near a button still presses it

// The buttons drawn round (A B X Y) and as pills (Back, Start, RT)
enum Button { ButtonA, ButtonB, ButtonX, ButtonY, ButtonBack, ButtonStart, ButtonRT, ButtonCount };

struct ButtonSpec
{
    const char* name;
    int padButton;      // SDL_GamepadButton, or -1 for RT (an axis)
    ImU32 colour;
};

const ButtonSpec kButtons[ButtonCount] = {
    { "A",     SDL_GAMEPAD_BUTTON_SOUTH, IM_COL32(16, 124, 16, 255) },
    { "B",     SDL_GAMEPAD_BUTTON_EAST,  IM_COL32(200, 40, 40, 255) },
    { "X",     SDL_GAMEPAD_BUTTON_WEST,  IM_COL32(30, 90, 200, 255) },
    { "Y",     SDL_GAMEPAD_BUTTON_NORTH, IM_COL32(230, 180, 0, 255) },
    { "Back",  SDL_GAMEPAD_BUTTON_BACK,  IM_COL32(0, 0, 0, 255) },
    { "Start", SDL_GAMEPAD_BUTTON_START, IM_COL32(0, 0, 0, 255) },
    { "RT",    -1,                       IM_COL32(0, 0, 0, 255) },
};

const int kMaxFingers = 10;

struct Finger
{
    SDL_FingerID id = 0;
    bool used = false;
    int button = -1;      // the button it holds, or -1
    bool stick = false;   // it moves the stick
    bool pad = false;     // it is on the D-pad
    int padDirections = 0;
};

// Where everything is, in window pixels, and what is held. Written by the
// window thread, copied by the game thread to draw
struct State
{
    float width = 0.0f, height = 0.0f, unit = 1.0f;
    ImVec2 buttonCentre[ButtonCount];
    bool shown[ButtonCount] = {};   // something is assigned to it (the Controls window)
    float buttonRadius = 0.0f;
    ImVec2 pillSize;
    ImVec2 stickHome;            // the ring when no finger is on it
    ImVec2 stickCentre, knob;    // where the finger put it
    bool stickActive = false;
    float stickRadius = 0.0f;
    ImVec2 padCentre;
    float padCell = 0.0f;
    int padDirections = 0;       // 1 up, 2 down, 4 left, 8 right
    bool held[ButtonCount] = {};
    bool visible = false;
    bool menuOpen = false;
    bool back = false;           // Android's Back key (B)
};

std::mutex s_mutex;
State s_state;
Finger s_fingers[kMaxFingers];

#ifdef __ANDROID__
const bool kVisibleAtStart = true;
#else
const bool kVisibleAtStart = false;
#endif
bool s_initialised = false;

void releaseAll(State& state)
{
    for (Finger& finger : s_fingers)
        finger = Finger();
    for (bool& held : state.held)
        held = false;
    state.stickActive = false;
    state.padDirections = 0;
}

// Something is assigned to the button (the Controls window); the map only
// counts in AITD3, the only game that has one
bool assigned(int b)
{
    for (int a = 0; a < kGameActionCount; a++)
    {
        if ((GameAction)a == GameAction::Map && g_gameId != AITD3)
            continue;
        for (const InputBinding& binding : controlsBindings((GameAction)a))
        {
            const bool match = (kButtons[b].padButton >= 0)
                ? (binding.kind == InputBinding::Kind::PadButton && binding.code == kButtons[b].padButton)
                : (binding.kind == InputBinding::Kind::PadAxis && binding.code == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER && binding.positive);
            if (match)
                return true;
        }
    }
    return false;
}

// Places the controls for a window of this size: each side centred in its
// black bar when it fits there, otherwise against the edge of the screen.
// Left: Back, the stick, the D-pad; right: Start, RT, the A B X Y diamond.
void layout(State& state, float width, float height, float unit, const int sceneRect[4])
{
    state.width = width;
    state.height = height;
    state.unit = unit;
    state.buttonRadius = kButtonRadius * unit;
    state.stickRadius = kStickRadius * unit;
    state.padCell = kPadCell * unit;
    state.pillSize = ImVec2(kPillWidth * unit, kPillHeight * unit);
    for (int b = 0; b < ButtonCount; b++)
        state.shown[b] = assigned(b);

    const float margin = kMargin * unit;
    const float leftBar = (float)sceneRect[0];
    const float rightBar = width - (float)(sceneRect[0] + sceneRect[2]);
    const float pillStep = state.pillSize.y + margin * 0.75f;

    // Right side
    const float diamond = (kButtonSpacing + kButtonRadius) * unit;
    const float rightX = (rightBar >= diamond * 2.0f + margin) ? width - rightBar * 0.5f : width - margin - diamond;
    const float cy = height - margin - diamond;
    const float s = kButtonSpacing * unit;
    state.buttonCentre[ButtonA] = ImVec2(rightX, cy + s);
    state.buttonCentre[ButtonB] = ImVec2(rightX + s, cy);
    state.buttonCentre[ButtonX] = ImVec2(rightX - s, cy);
    state.buttonCentre[ButtonY] = ImVec2(rightX, cy - s);
    const float pillX = (std::min)(rightX, width - margin - state.pillSize.x * 0.5f);
    state.buttonCentre[ButtonStart] = ImVec2(pillX, margin + state.pillSize.y * 0.5f);
    state.buttonCentre[ButtonRT] = ImVec2(pillX, margin + state.pillSize.y * 0.5f + pillStep);

    // Left side
    const float pad = state.padCell * 1.5f;
    const float leftWidest = (std::max)(state.stickRadius, pad);
    const float leftX = (leftBar >= leftWidest * 2.0f + margin * 2.0f) ? leftBar * 0.5f : margin + leftWidest;
    state.padCentre = ImVec2(leftX, height - margin - pad);
    state.stickHome = ImVec2(leftX, state.padCentre.y - pad - margin - state.stickRadius);
    state.buttonCentre[ButtonBack] = ImVec2((std::max)(leftX, margin + state.pillSize.x * 0.5f),
        margin + state.pillSize.y * 0.5f);
}

bool isPill(int b)
{
    return b == ButtonBack || b == ButtonStart || b == ButtonRT;
}

int buttonAt(const State& state, ImVec2 pos)
{
    int best = -1;
    float bestDistance = 0.0f;
    for (int b = 0; b < ButtonCount; b++)
    {
        if (!state.shown[b])
            continue;
        const ImVec2 c = state.buttonCentre[b];
        if (isPill(b))
        {
            const float grow = state.unit * 8.0f;
            if (std::fabs(pos.x - c.x) <= state.pillSize.x * 0.5f + grow && std::fabs(pos.y - c.y) <= state.pillSize.y * 0.5f + grow)
                return b;
            continue;
        }
        const float dx = pos.x - c.x, dy = pos.y - c.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= state.buttonRadius * kHitGrow && (best < 0 || distance < bestDistance))
        {
            best = b;
            bestDistance = distance;
        }
    }
    return best;
}

bool onPad(const State& state, ImVec2 pos)
{
    const float half = state.padCell * 1.5f * 1.15f;
    return std::fabs(pos.x - state.padCentre.x) <= half && std::fabs(pos.y - state.padCentre.y) <= half;
}

// The D-pad's directions where the finger is: the arms, and the corners
// between them as diagonals
int padDirections(const State& state, ImVec2 pos)
{
    const float dx = pos.x - state.padCentre.x, dy = pos.y - state.padCentre.y;
    const float edge = state.padCell * 0.5f;
    int bits = 0;
    if (dy < -edge) bits |= 1;
    if (dy > edge)  bits |= 2;
    if (dx < -edge) bits |= 4;
    if (dx > edge)  bits |= 8;
    return bits;
}

void moveKnob(State& state, ImVec2 pos)
{
    float dx = pos.x - state.stickCentre.x, dy = pos.y - state.stickCentre.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length > state.stickRadius)
    {
        dx *= state.stickRadius / length;
        dy *= state.stickRadius / length;
    }
    state.knob = ImVec2(state.stickCentre.x + dx, state.stickCentre.y + dy);
}

Finger* findFinger(SDL_FingerID id)
{
    for (Finger& finger : s_fingers)
        if (finger.used && finger.id == id)
            return &finger;
    return nullptr;
}

void updateHeld(State& state)
{
    for (int b = 0; b < ButtonCount; b++)
        state.held[b] = false;
    state.padDirections = 0;
    for (const Finger& finger : s_fingers)
    {
        if (!finger.used)
            continue;
        if (finger.button >= 0)
            state.held[finger.button] = true;
        if (finger.pad)
            state.padDirections |= finger.padDirections;
    }
}

} // namespace

void touchControlsEvent(const SDL_Event& event)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    State& state = s_state;

    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
    {
        if (event.key.scancode == SDL_SCANCODE_AC_BACK)
            state.back = (event.type == SDL_EVENT_KEY_DOWN);
        return;
    }
    if (state.menuOpen || state.width <= 0.0f)
        return;

    const ImVec2 pos(event.tfinger.x * state.width, event.tfinger.y * state.height);
    switch (event.type)
    {
    case SDL_EVENT_FINGER_DOWN:
    {
        state.visible = true;
        Finger* finger = nullptr;
        for (Finger& free : s_fingers)
            if (!free.used)
            {
                finger = &free;
                break;
            }
        if (!finger)
            return;
        *finger = Finger();
        finger->id = event.tfinger.fingerID;
        const int button = buttonAt(state, pos);
        if (button >= 0)
        {
            finger->used = true;
            finger->button = button;
        }
        else if (onPad(state, pos))
        {
            finger->used = true;
            finger->pad = true;
            finger->padDirections = padDirections(state, pos);
        }
        else if (pos.x < state.width * 0.45f && !state.stickActive)
        {
            // The ring goes where the finger lands, kept on the screen
            finger->used = true;
            finger->stick = true;
            const float ring = state.stickRadius;
            state.stickCentre = ImVec2((std::max)(pos.x, ring), (std::min)((std::max)(pos.y, ring), state.height - ring));
            state.stickActive = true;
            moveKnob(state, pos);
        }
        break;
    }
    case SDL_EVENT_FINGER_MOTION:
    {
        Finger* finger = findFinger(event.tfinger.fingerID);
        if (finger && finger->stick)
            moveKnob(state, pos);
        else if (finger && finger->pad)
            finger->padDirections = padDirections(state, pos);   // sliding round the cross
        break;
    }
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
    {
        Finger* finger = findFinger(event.tfinger.fingerID);
        if (!finger)
            break;
        if (finger->stick)
            state.stickActive = false;
        *finger = Finger();
        break;
    }
    default:
        break;
    }
    updateHeld(state);
}

void touchControlsFrame(bool menuOpen)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    State& state = s_state;
    if (!s_initialised)
    {
        state.visible = kVisibleAtStart;
        s_initialised = true;
    }
    if (menuOpen && !state.menuOpen)
        releaseAll(state);
    state.menuOpen = menuOpen;

    int width = 0, height = 0;
    if (gWindowBGFX)
        SDL_GetWindowSizeInPixels(gWindowBGFX, &width, &height);
    float unit = gWindowBGFX ? SDL_GetWindowDisplayScale(gWindowBGFX) : 1.0f;
    if (unit <= 0.1f)
        unit = 1.0f;
    int sceneRect[4];
    osystem_getSceneRect(sceneRect);
    if (width > 0 && height > 0)
        layout(state, (float)width, (float)height, unit, sceneRect);
    if (!state.stickActive)
        state.stickCentre = state.knob = state.stickHome;
}

bool touchPadButton(int button)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const State& state = s_state;
    if (button == SDL_GAMEPAD_BUTTON_EAST && state.back)
        return true;
    switch (button)
    {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:    return (state.padDirections & 1) != 0;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:  return (state.padDirections & 2) != 0;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:  return (state.padDirections & 4) != 0;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return (state.padDirections & 8) != 0;
    default:
        break;
    }
    for (int b = 0; b < ButtonCount; b++)
        if (kButtons[b].padButton == button && state.held[b])
            return true;
    return false;
}

float touchPadAxis(int axis)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const State& state = s_state;
    if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
        return state.held[ButtonRT] ? 1.0f : 0.0f;
    if (!state.stickActive || state.stickRadius <= 0.0f)
        return 0.0f;
    if (axis == SDL_GAMEPAD_AXIS_LEFTX)
        return (state.knob.x - state.stickCentre.x) / state.stickRadius;
    if (axis == SDL_GAMEPAD_AXIS_LEFTY)
        return (state.knob.y - state.stickCentre.y) / state.stickRadius;
    return 0.0f;
}

void touchControlsHide()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_state.visible = false;
}

void touchControlsDraw()
{
    State state;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        state = s_state;
    }
    if (!state.visible || state.menuOpen || state.width <= 0.0f)
        return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float u = state.unit;
    const ImU32 outline = IM_COL32(255, 255, 255, 110);
    const ImU32 letterColour = IM_COL32(255, 255, 255, 210);
    const ImU32 dark = IM_COL32(0, 0, 0, 80), darkHeld = IM_COL32(255, 255, 255, 110);

    auto centredText = [&](ImVec2 centre, float size, ImU32 colour, const char* text)
    {
        const ImVec2 extent = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
        draw->AddText(font, size, ImVec2(centre.x - extent.x * 0.5f, centre.y - extent.y * 0.5f), colour, text);
    };
    auto withAlpha = [](ImU32 colour, int alpha) { return (colour & 0x00FFFFFF) | ((ImU32)alpha << IM_COL32_A_SHIFT); };

    // Stick
    draw->AddCircleFilled(state.stickCentre, state.stickRadius, dark, 48);
    draw->AddCircle(state.stickCentre, state.stickRadius, outline, 48, 2.0f * u);
    draw->AddCircleFilled(state.knob, kKnobRadius * u, IM_COL32(255, 255, 255, state.stickActive ? 150 : 90), 32);

    // D-pad: a cross, the arms pressed lit, with little arrows
    const float c = state.padCell, h = c * 0.5f;
    const ImVec2 p = state.padCentre;
    const ImVec2 arms[4][2] = {
        { ImVec2(p.x - h, p.y - h - c), ImVec2(p.x + h, p.y - h) },   // up
        { ImVec2(p.x - h, p.y + h), ImVec2(p.x + h, p.y + h + c) },   // down
        { ImVec2(p.x - h - c, p.y - h), ImVec2(p.x - h, p.y + h) },   // left
        { ImVec2(p.x + h, p.y - h), ImVec2(p.x + h + c, p.y + h) },   // right
    };
    draw->AddRectFilled(ImVec2(p.x - h, p.y - h), ImVec2(p.x + h, p.y + h), dark);
    for (int d = 0; d < 4; d++)
    {
        draw->AddRectFilled(arms[d][0], arms[d][1], (state.padDirections & (1 << d)) ? darkHeld : dark, 4.0f * u);
        draw->AddRect(arms[d][0], arms[d][1], outline, 4.0f * u, 0, 1.5f * u);
    }
    const float a = c * 0.22f;
    const ImU32 arrow = IM_COL32(255, 255, 255, 170);
    draw->AddTriangleFilled(ImVec2(p.x, p.y - h - c * 0.75f), ImVec2(p.x - a, p.y - h - c * 0.3f), ImVec2(p.x + a, p.y - h - c * 0.3f), arrow);
    draw->AddTriangleFilled(ImVec2(p.x, p.y + h + c * 0.75f), ImVec2(p.x + a, p.y + h + c * 0.3f), ImVec2(p.x - a, p.y + h + c * 0.3f), arrow);
    draw->AddTriangleFilled(ImVec2(p.x - h - c * 0.75f, p.y), ImVec2(p.x - h - c * 0.3f, p.y + a), ImVec2(p.x - h - c * 0.3f, p.y - a), arrow);
    draw->AddTriangleFilled(ImVec2(p.x + h + c * 0.75f, p.y), ImVec2(p.x + h + c * 0.3f, p.y - a), ImVec2(p.x + h + c * 0.3f, p.y + a), arrow);

    // The buttons with something assigned
    for (int b = 0; b < ButtonCount; b++)
    {
        if (!state.shown[b])
            continue;
        const ImVec2 centre = state.buttonCentre[b];
        if (isPill(b))
        {
            const ImVec2 lo(centre.x - state.pillSize.x * 0.5f, centre.y - state.pillSize.y * 0.5f);
            const ImVec2 hi(centre.x + state.pillSize.x * 0.5f, centre.y + state.pillSize.y * 0.5f);
            draw->AddRectFilled(lo, hi, state.held[b] ? darkHeld : dark, state.pillSize.y * 0.5f);
            draw->AddRect(lo, hi, outline, state.pillSize.y * 0.5f, 0, 1.5f * u);
            centredText(centre, state.pillSize.y * 0.45f, letterColour, kButtons[b].name);
        }
        else
        {
            const float radius = state.buttonRadius;
            draw->AddCircleFilled(centre, radius, withAlpha(kButtons[b].colour, state.held[b] ? 170 : 75), 40);
            draw->AddCircle(centre, radius, outline, 40, 2.0f * u);
            centredText(centre, radius, letterColour, kButtons[b].name);
        }
    }
}
