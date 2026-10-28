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
#include <imgui_internal.h>   // GetFocusID: the item the cursor is on

#include <algorithm>
#include <cfloat>
#include <string>

namespace menu
{

// ── Palette ─────────────────────────────────────────────────────────────────
static const ImU32 kBackground   = IM_COL32(27, 26, 30, 255);
static const ImU32 kPanel        = IM_COL32(34, 33, 38, 255);
static const ImU32 kPanelBorder  = IM_COL32(74, 64, 46, 255);
static const ImU32 kGold         = IM_COL32(201, 164, 92, 255);
static const ImU32 kGoldLight    = IM_COL32(236, 213, 154, 255);
static const ImU32 kGoldTop      = IM_COL32(243, 226, 174, 255);
static const ImU32 kGoldBottom   = IM_COL32(184, 145, 77, 255);
static const ImU32 kText         = IM_COL32(232, 223, 204, 255);
static const ImU32 kTextDim      = IM_COL32(154, 146, 130, 255);
static const ImU32 kTextDisabled = IM_COL32(107, 101, 90, 255);
static const ImU32 kButtonDark   = IM_COL32(38, 36, 41, 255);
static const ImU32 kButtonHover  = IM_COL32(48, 45, 51, 255);
static const ImU32 kHeader       = IM_COL32(107, 87, 51, 255);
static const ImU32 kError        = IM_COL32(222, 120, 96, 255);

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
    ImGui::GetStyle() = style;
}

// Fonts of the menus, made at the current scale (fontsNeedRebuild)
struct Fonts
{
    ImFont* text = nullptr;     // 17 units
    ImFont* button = nullptr;   // 21 units, bold
    ImFont* header = nullptr;   // 30 units
    ImFont* title = nullptr;    // 40 units, bold
};

// ── Widgets ─────────────────────────────────────────────────────────────────

// Touch screens: dragging the current scrolling area moves it (phones have no
// wheel). Call inside the area, after its items.
inline void dragScroll()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseSource != ImGuiMouseSource_TouchScreen || !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        return;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && !ImGui::IsAnyItemActive())
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

// Primary action: gold gradient with dark text
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
        const ImU32 top = hovered ? IM_COL32(250, 236, 192, 255) : kGoldTop;
        const ImU32 bottom = hovered ? IM_COL32(204, 165, 92, 255) : kGoldBottom;
        draw->AddRectFilledMultiColor(pos, corner, top, top, bottom, bottom);
        draw->AddRect(pos, corner, IM_COL32(248, 234, 190, 255), 6.0f, 0, 1.5f);
        if (hovered)
            draw->AddRect(ImVec2(pos.x - 3, pos.y - 3), ImVec2(corner.x + 3, corner.y + 3), IM_COL32(236, 213, 154, 90), 8.0f, 0, 2.0f);
    }
    else
    {
        draw->AddRectFilled(pos, corner, kButtonDark, 6.0f);
        draw->AddRect(pos, corner, kPanelBorder, 6.0f);
    }
    navCursor(pos, corner, 6.0f);
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
    draw->AddRectFilled(pos, corner, fill, 6.0f);
    draw->AddRect(pos, corner, border, 6.0f, 0, selected ? 2.0f : 1.2f);
    navCursor(pos, corner, 6.0f);
    textCentered(draw, font, pos.x + size.x * 0.5f, pos.y + (size.y - font->FontSize) * 0.5f, textColor, label);
    return clicked && enabled;
}

// Help boxes stand out from the window: lighter, warmer and with a light gold border
static const ImU32 kHelpBackground = IM_COL32(58, 50, 38, 250);

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
