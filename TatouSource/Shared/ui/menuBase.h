///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Base of every menu (launcher, F1 window, controls, audio), shared by the
// launcher and the game and ready for other systems (Android):
//  - one palette (dark charcoal panels, gold accents);
//  - metrics: the scale of the interface from the screen size and its
//    density, the layout (landscape or portrait) and the minimum size of
//    anything pressed (bigger after a touch);
//  - widgets that work the same with mouse, touch, keyboard and controller:
//    gold and dark buttons, "(?)" help (hover, focus or tap), text that fits.
// Sizes are given in interface units: Metrics::px() turns them into pixels.
// Header-only, like the rest of Shared/ (both programs use the same ImGui).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <imgui.h>
#include <imgui_internal.h>   // GetFocusID: the item the cursor is on; ClearActiveID

#include <algorithm>
#include <cfloat>
#include <string>

namespace menu
{

// ── Palette ─────────────────────────────────────────────────────────────────
// Charcoal and gold; the menus change them to the colour of the game
// (setGamePalette, below). The names stay the gold ones.
inline ImU32 kBackground   = IM_COL32(27, 26, 30, 255);
inline ImU32 kPanel        = IM_COL32(34, 33, 38, 255);
inline ImU32 kPanelBorder  = IM_COL32(74, 64, 46, 255);
inline ImU32 kGold         = IM_COL32(201, 164, 92, 255);
inline ImU32 kGoldLight    = IM_COL32(236, 213, 154, 255);
inline ImU32 kGoldTop      = IM_COL32(243, 226, 174, 255);
inline ImU32 kGoldBottom   = IM_COL32(184, 145, 77, 255);
inline ImU32 kGoldHoverTop    = IM_COL32(250, 236, 192, 255);
inline ImU32 kGoldHoverBottom = IM_COL32(204, 165, 92, 255);
inline ImU32 kGoldEdge     = IM_COL32(248, 234, 190, 255);
inline ImU32 kGoldText     = IM_COL32(42, 33, 22, 255);   // on the primary buttons
inline ImU32 kText         = IM_COL32(232, 223, 204, 255);
inline ImU32 kTextDim      = IM_COL32(154, 146, 130, 255);
inline ImU32 kTextDisabled = IM_COL32(107, 101, 90, 255);
inline ImU32 kButtonDark   = IM_COL32(38, 36, 41, 255);
inline ImU32 kButtonHover  = IM_COL32(48, 45, 51, 255);
inline ImU32 kButtonSelected = IM_COL32(70, 58, 38, 255);
inline ImU32 kHeader       = IM_COL32(107, 87, 51, 255);
inline ImU32 kHeaderHovered = IM_COL32(133, 107, 61, 255);
inline ImU32 kHeaderActive = IM_COL32(153, 122, 71, 255);
inline ImU32 kButtonActive = IM_COL32(77, 64, 43, 255);
inline ImU32 kError        = IM_COL32(222, 120, 96, 255);

inline ImVec4 color(ImU32 value, float alpha = 1.0f)
{
    ImVec4 result = ImGui::ColorConvertU32ToFloat4(value);
    result.w *= alpha;
    return result;
}

// ── Metrics ─────────────────────────────────────────────────────────────────
enum class Layout
{
    Landscape,   // wider than tall: side by side (a PC window, a phone on its side)
    Portrait     // taller than wide: one column that scrolls (a phone upright)
};

// Smallest thing a finger can press reliably, in interface units
static const float kTouchTarget = 44.0f;

struct Metrics
{
    float scale = 1.0f;          // pixels per interface unit
    Layout layout = Layout::Landscape;
    ImVec2 size;                 // screen or window, in pixels
    ImVec2 units;                // the same in interface units
    bool touch = false;          // the last pointer input was a touch

    // Size in pixels of interface units (scaled to the screen)
    float px(float interfaceUnits) const { return interfaceUnits * scale; }
    // Size in pixels of interface units (scaled to the screen)
    ImVec2 px(float x, float y) const { return ImVec2(x * scale, y * scale); }
    // True for the vertical layout (a tall window or a phone)
    bool portrait() const { return layout == Layout::Portrait; }
    // Height of anything pressed: the given one, never smaller than a finger after a touch
    float pressHeight(float interfaceUnits) const { return px(touch ? (std::max)(interfaceUnits, kTouchTarget) : interfaceUnits); }
};

// The screen (or window) in pixels and its density (1 = 96 dpi on Windows,
// SDL_GetWindowDisplayScale; about 2.5-3.5 on phones). design: the size, in
// units, the menu was drawn for in each layout. The scale makes that design
// fit, but never goes under 80% of the density (text stays readable: the
// menu scrolls instead) nor over 3 times it.
inline Metrics computeMetrics(ImVec2 sizePixels, float density, ImVec2 designLandscape, ImVec2 designPortrait)
{
    Metrics metrics;
    metrics.size = sizePixels;
    density = (density > 0.1f) ? density : 1.0f;
    metrics.layout = (sizePixels.y > sizePixels.x * 1.05f) ? Layout::Portrait : Layout::Landscape;
    const ImVec2 design = metrics.portrait() ? designPortrait : designLandscape;
    const float fit = (std::min)(sizePixels.x / design.x, sizePixels.y / design.y);
    metrics.scale = (std::max)(density * 0.8f, (std::min)(fit, density * 3.0f));
    metrics.units = ImVec2(sizePixels.x / metrics.scale, sizePixels.y / metrics.scale);
    metrics.touch = ImGui::GetIO().MouseSource == ImGuiMouseSource_TouchScreen;
    return metrics;
}

// Fonts are drawn at a fixed size: rebuild them when the scale moved more than
// 10% from the one they were made for (sharp letters at any size)
inline bool fontsNeedRebuild(float fontScale, float scale)
{
    return fontScale <= 0.0f || scale > fontScale * 1.1f || scale < fontScale / 1.1f;
}

// ImGui's style for the menus at this scale (from scratch every time, so it
// never grows on itself). panelAlpha: < 1 lets the game show through.
inline void applyStyle(const Metrics& metrics, float panelAlpha = 1.0f)
{
    ImGuiStyle style;
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding = ImVec2(22.0f, 18.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ScrollbarSize = 16.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    if (metrics.touch)
    {
        // Wider grips and slider handles for fingers
        style.ScrollbarSize = 24.0f;
        style.GrabMinSize = 24.0f;
        style.TouchExtraPadding = ImVec2(4.0f, 4.0f);
    }
    // Round corners, as the launcher screen
    style.WindowRounding = 14.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.ScaleAllSizes(metrics.scale);
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = color(kText);
    c[ImGuiCol_TextDisabled] = color(kTextDisabled);
    c[ImGuiCol_WindowBg] = color(kPanel, panelAlpha);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = color(kPanel, (std::max)(panelAlpha, 0.97f));
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
    c[ImGuiCol_Border] = color(kGold);
    c[ImGuiCol_FrameBg] = color(kButtonDark);
    c[ImGuiCol_FrameBgHovered] = color(kButtonHover);
    c[ImGuiCol_FrameBgActive] = color(kButtonHover);
    c[ImGuiCol_CheckMark] = color(kGoldLight);
    c[ImGuiCol_SliderGrab] = color(kGold);
    c[ImGuiCol_SliderGrabActive] = color(kGoldLight);
    c[ImGuiCol_Header] = color(kHeader);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.52f, 0.42f, 0.24f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.48f, 0.28f, 1.0f);
    c[ImGuiCol_Button] = color(kButtonDark);
    c[ImGuiCol_ButtonHovered] = color(kButtonHover);
    c[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.25f, 0.17f, 1.0f);
    c[ImGuiCol_Separator] = color(kGold);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0.25f);
    c[ImGuiCol_ScrollbarGrab] = color(kPanelBorder);
    c[ImGuiCol_ScrollbarGrabHovered] = color(kGold);
    c[ImGuiCol_ScrollbarGrabActive] = color(kGoldLight);
    c[ImGuiCol_NavCursor] = color(kGoldLight);   // the controller / keyboard cursor
    // Tabs (the controls window): dark, the chosen one in the accent
    c[ImGuiCol_Tab] = color(kButtonDark);
    c[ImGuiCol_TabHovered] = color(kButtonHover);
    c[ImGuiCol_TabSelected] = color(kButtonSelected);
    c[ImGuiCol_TabSelectedOverline] = color(kGold);
    c[ImGuiCol_TabDimmed] = color(kButtonDark);
    c[ImGuiCol_TabDimmedSelected] = color(kButtonSelected);
    c[ImGuiCol_TabDimmedSelectedOverline] = color(kGold);
    // Soft edges in the game's colour, not a strong line
    c[ImGuiCol_Border] = color(kGold, 0.40f);
    c[ImGuiCol_Separator] = color(kPanelBorder);
    ImGui::GetStyle() = style;
}

// Fonts of the menus, made at the current scale (fontsNeedRebuild)
struct Fonts
{
    ImFont* text = nullptr;       // 15 units
    ImFont* button = nullptr;     // 16 units, semibold
    ImFont* header = nullptr;     // 20 units, semibold
    ImFont* title = nullptr;      // the launcher's game name, 34 units, semibold
    ImFont* smallText = nullptr;  // smaller labels, 13 units
};

// A fixed panel without title bar; its scroll bar appears only when the
// content does not fit (NoDecoration would hide it)
constexpr ImGuiWindowFlags kPanelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

// ── Widgets ─────────────────────────────────────────────────────────────────

// Touch screens: dragging the current scrolling area moves it (phones have no
// wheel). Call inside the area, after its items. A finger moving up or down
// scrolls even when it started on a button, which then is not pressed (on a
// phone almost all the screen is buttons); sideways drags stay with their
// item (sliders).
inline void dragScroll()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseSource != ImGuiMouseSource_TouchScreen || !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        return;
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        return;
    if (ImGui::IsAnyItemActive())
    {
        const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if (drag.y * drag.y <= drag.x * drag.x)
            return;
        ImGui::ClearActiveID();
    }
    ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
}

// The controller / keyboard cursor on the last item, drawn by hand for the
// buttons drawn by hand
inline void navCursor(ImVec2 origin, ImVec2 corner, float rounding)
{
    if (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible)
        ImGui::GetWindowDrawList()->AddRect(ImVec2(origin.x - 3, origin.y - 3), ImVec2(corner.x + 3, corner.y + 3), kGoldLight, rounding + 2.0f, 0, 2.0f);
}

inline void textCentered(ImDrawList* draw, ImFont* font, float centerX, float y, ImU32 textColor, const char* text)
{
    ImVec2 size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, text);
    draw->AddText(font, font->FontSize, ImVec2(centerX - size.x * 0.5f, y), textColor, text);
}

// Corners of the menus' buttons (round, as the launcher's)
static const float kButtonRounding = 10.0f;

// Primary action: the game's colour with a sheen on its top half and dark text
inline bool goldButton(const char* id, const char* label, ImVec2 pos, ImVec2 size, ImFont* font, bool enabled)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::BeginDisabled(!enabled);
    const bool clicked = ImGui::InvisibleButton(id, size);
    ImGui::EndDisabled();
    const bool hovered = enabled && (ImGui::IsItemHovered() || ImGui::IsItemFocused());

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 corner(pos.x + size.x, pos.y + size.y);
    if (enabled)
    {
        draw->AddRectFilled(pos, corner, hovered ? kGoldHoverTop : kGoldTop, kButtonRounding);
        draw->AddRectFilled(pos, ImVec2(corner.x, pos.y + size.y * 0.5f), IM_COL32(255, 255, 255, hovered ? 40 : 24), kButtonRounding,
            ImDrawFlags_RoundCornersTop);
        draw->AddRect(pos, corner, kGoldBottom, kButtonRounding, 0, 1.0f);
        if (hovered)
            draw->AddRect(ImVec2(pos.x - 3, pos.y - 3), ImVec2(corner.x + 3, corner.y + 3), (kGoldLight & 0x00FFFFFF) | (90u << 24),
                kButtonRounding + 2.0f, 0, 2.0f);
    }
    else
    {
        draw->AddRectFilled(pos, corner, kButtonDark, kButtonRounding);
        draw->AddRect(pos, corner, kPanelBorder, kButtonRounding);
    }
    navCursor(pos, corner, kButtonRounding);
    textCentered(draw, font, pos.x + size.x * 0.5f, pos.y + (size.y - font->FontSize) * 0.5f,
        enabled ? IM_COL32(42, 33, 22, 255) : kTextDisabled, label);
    return clicked && enabled;
}

// Secondary action: dark with a gold border
inline bool darkButton(const char* id, const char* label, ImVec2 pos, ImVec2 size, ImFont* font, bool enabled, bool selected = false)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::BeginDisabled(!enabled);
    const bool clicked = ImGui::InvisibleButton(id, size);
    ImGui::EndDisabled();
    const bool hovered = enabled && (ImGui::IsItemHovered() || ImGui::IsItemFocused());

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 corner(pos.x + size.x, pos.y + size.y);
    const ImU32 fill = selected ? IM_COL32(70, 58, 38, 255) : (hovered ? kButtonHover : kButtonDark);
    const ImU32 border = !enabled ? kPanelBorder : (selected || hovered ? kGoldLight : kGold);
    const ImU32 textColor = !enabled ? kTextDisabled : (selected ? kGoldLight : kText);
    draw->AddRectFilled(pos, corner, fill, kButtonRounding);
    draw->AddRect(pos, corner, border, kButtonRounding, 0, selected ? 2.0f : 1.2f);
    navCursor(pos, corner, kButtonRounding);
    textCentered(draw, font, pos.x + size.x * 0.5f, pos.y + (size.y - font->FontSize) * 0.5f, textColor, label);
    return clicked && enabled;
}

// Help boxes stand out from the window: lighter, warmer and with a light gold border
inline ImU32 kHelpBackground = IM_COL32(58, 50, 38, 250);

// A colour between a (t = 0) and b (t = 1), opaque
inline ImU32 mixColour(ImU32 a, ImU32 b, float t)
{
    const ImVec4 x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, 1.0f));
}

// The menus (the launcher's windows, the game's F1) take the chosen game's
// colour, from its light and dark ends, over dark glass
// like the launcher screen; call applyStyle afterwards
inline void setAccentPalette(ImU32 light, ImU32 dark)
{
    const ImU32 white = IM_COL32(255, 255, 255, 255);
    kBackground = IM_COL32(11, 12, 16, 255);
    kPanel = IM_COL32(20, 21, 27, 255);
    kPanelBorder = IM_COL32(52, 54, 62, 255);
    kGold = light;
    kGoldLight = mixColour(light, white, 0.35f);
    kGoldTop = light;
    kGoldBottom = mixColour(dark, light, 0.25f);
    kGoldHoverTop = mixColour(light, white, 0.18f);
    kGoldHoverBottom = mixColour(dark, light, 0.45f);
    kGoldEdge = mixColour(light, white, 0.3f);
    kGoldText = IM_COL32(11, 12, 16, 255);
    kText = IM_COL32(233, 230, 223, 255);
    kTextDim = IM_COL32(152, 148, 140, 255);
    kTextDisabled = IM_COL32(98, 96, 92, 255);
    kButtonDark = IM_COL32(30, 31, 38, 255);
    kButtonHover = IM_COL32(42, 43, 52, 255);
    kButtonSelected = mixColour(kButtonDark, dark, 0.45f);
    kHeader = mixColour(kButtonDark, dark, 0.55f);
    kHeaderHovered = mixColour(kButtonDark, dark, 0.75f);
    kHeaderActive = dark;
    kButtonActive = mixColour(kButtonDark, dark, 0.4f);
    kHelpBackground = IM_COL32(34, 36, 44, 250);
}

// Each game's colour, taken from its cover (light and dark ends): the
// ghost's green (0, AITD1 and Jack), the sea's blue (1, AITD2), the fire's
// orange (2, AITD3)
struct GameAccent
{
    ImU32 light;
    ImU32 dark;
};

inline GameAccent gameAccent(int game)
{
    static const GameAccent accents[3] = {
        { IM_COL32(143, 209, 106, 255), IM_COL32(63, 138, 58, 255) },
        { IM_COL32(74, 156, 240, 255), IM_COL32(26, 86, 184, 255) },
        { IM_COL32(240, 122, 60, 255), IM_COL32(178, 58, 34, 255) },
    };
    return accents[(game >= 0 && game < 3) ? game : 0];
}

// The palette of a game (setAccentPalette with its colour)
inline void setGamePalette(int game)
{
    const GameAccent accent = gameAccent(game);
    setAccentPalette(accent.light, accent.dark);
}

// With the controller or the keyboard the help waits until the cursor has
// stayed this long on the same item, so it never covers the options while
// moving through them
static const double kHelpDelaySeconds = 4.0;

// Text of a help box, wrapped at a readable width
inline void wrappedHelpText(const char* text)
{
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
}

inline void pushHelpColors()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, color(kHelpBackground));
    ImGui::PushStyleColor(ImGuiCol_Border, color(kGoldLight));
    ImGui::PushStyleColor(ImGuiCol_Text, color(kGoldTop));
}

inline void helpTooltip(const char* text)
{
    pushHelpColors();
    ImGui::BeginTooltip();
    wrappedHelpText(text);
    ImGui::EndTooltip();
    ImGui::PopStyleColor(3);
}

// The last item has had the controller / keyboard cursor for kHelpDelaySeconds
inline bool focusedLongEnough()
{
    // The time counts from when the cursor reached the item it is on now.
    // Only while the controller or the keyboard is in use: once the mouse
    // moves, ImGui stops highlighting the cursor's item and so does this
    // (otherwise the first option's help stayed up and mixed with the "(?)"
    // under the mouse)
    static ImGuiID focusedId = 0;
    static double since = 0.0;
    const ImGuiID current = GImGui->NavHighlightItemUnderNav ? ImGui::GetFocusID() : 0;
    if (current != focusedId)
    {
        focusedId = current;
        since = ImGui::GetTime();
    }
    return current != 0 && ImGui::IsItemFocused() && ImGui::GetIO().NavVisible && ImGui::GetTime() - since >= kHelpDelaySeconds;
}

// The explanation of the last item once the controller / keyboard cursor has
// stayed on it (console menus show it by themselves; a drop-down list calls
// this after it, since its "(?)" belongs to the label before it)
inline void helpWhenFocused(const char* text)
{
    if (focusedLongEnough() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        helpTooltip(text);
}

// "(?)" after the last item. Its explanation shows at once with the mouse
// over it, after a while with the controller / keyboard cursor on the item or
// on the "(?)", and stays open after a click, a tap or A until something else
// is pressed (phones have no hover).
inline void helpMarker(const char* text)
{
    const bool itemFocused = focusedLongEnough();
    ImGui::SameLine();
    ImGui::PushID(text);
    const char* label = "(?)";
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImGuiIO& io = ImGui::GetIO();
    const bool touch = io.MouseSource == ImGuiMouseSource_TouchScreen;
    // A finger needs a bigger target than the three letters
    const float side = touch ? (std::max)(textSize.y * 2.0f, textSize.x + ImGui::GetStyle().FramePadding.x * 2.0f) : 0.0f;
    const ImVec2 size(touch ? side : textSize.x, touch ? side : ImGui::GetFrameHeight());
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##help", size);
    const bool hovered = ImGui::IsItemHovered() && !touch;
    const bool markerFocused = ImGui::IsItemFocused() && io.NavVisible;
    const bool markerWaited = focusedLongEnough();
    const ImVec2 corner(pos.x + size.x, pos.y + size.y);
    ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f),
        (hovered || markerFocused) ? kGoldLight : kTextDisabled, label);
    navCursor(pos, corner, 4.0f);
    if (pressed)
        ImGui::OpenPopup("##helpPopup");
    pushHelpColors();
    const bool popupOpen = ImGui::BeginPopup("##helpPopup");
    if (popupOpen)
    {
        wrappedHelpText(text);
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    if (!popupOpen && (hovered || markerWaited || itemFocused))
        helpTooltip(text);
    ImGui::PopID();
}

// Text that fits in maxWidth: a longer one keeps its start and end with
// "..." in the middle (UTF-8 safe), and shows whole with the mouse over it
inline void fittedText(ImU32 textColor, const std::string& text, float maxWidth)
{
    std::string shown = text;
    if (ImGui::CalcTextSize(text.c_str()).x > maxWidth)
    {
        size_t keep = text.size();
        while (keep > 2)
        {
            keep--;
            size_t head = keep / 2, tail = text.size() - (keep - head);
            while (head > 0 && (text[head] & 0xC0) == 0x80)
                head--;
            while (tail < text.size() && (text[tail] & 0xC0) == 0x80)
                tail++;
            shown = text.substr(0, head) + "..." + text.substr(tail);
            if (ImGui::CalcTextSize(shown.c_str()).x <= maxWidth)
                break;
        }
    }
    ImGui::TextColored(color(textColor), "%s", shown.c_str());
    if (shown != text && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", text.c_str());
}

} // namespace menu
