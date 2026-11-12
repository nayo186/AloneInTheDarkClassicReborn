///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The launcher screen: a library on the left (the three
// games with their covers and whether their folder is ready) and the chosen
// game on the right, over its own cover blurred: the cover, its story in one
// line, Play, Jack in the Dark (AITD1), the game language, three cards
// (Options, Controls, Audio) and a bar with the game folder. Each game has
// the colour of its cover (green, blue, orange). Only layout and drawing: the
// host (Launcher.exe, the game itself on Android) fills a LauncherView and
// acts on the LauncherIntent it gets back. Two layouts (menuBase.h metrics):
//  - landscape (a PC window, a phone on its side): library and game side by side;
//  - portrait (a narrow window, a phone upright): one column that scrolls,
//    the games in a row at the top.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "menuBase.h"
#include "launcherText.h"
#include "advancedOptionsText.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

// A game's cover, from any renderer (SDL renderer in Launcher.exe)
struct LauncherCover
{
    ImTextureID colour = 0;
    ImTextureID grey = 0;         // shown while the game has no data folder
    ImTextureID background = 0;   // blurred and darkened, behind the window
    int width = 0;
    int height = 0;
};

// What the screen shows; the host fills it every frame
struct LauncherView
{
    int selectedGame = 0;              // 0 AITD1, 1 AITD2, 2 AITD3
    bool dataFolderValid = false;      // the selected game's data folder is usable
    bool jackFound = false;            // Jack in the Dark is in it (JACK\PERE.PAK)
    bool isDefault = true;             // no advanced option changed
    bool debugMode = false;            // noted at the bottom (launcher.cfg debugMode = 1)
    bool canBrowse = true;             // a folder picker exists (not on every system)
    bool importGame = false;           // Android: the button imports (copies) the game instead
    std::string version;               // this build's version, shown at the bottom right
    bool canUpdate = false;            // an Update button (Launcher.exe of a published version)
    bool updateAvailable = false;      // a newer version was found: the button is gold
    std::string dataFolder;            // UTF-8, empty: none chosen
    std::string message;               // state of the folder (files checked, missing...), empty: none
    ImU32 messageColor = menu::kTextDim;
    std::string source;                // what the folder is (GOG / Steam, CD image, copy), empty: none
    std::vector<std::string> gameLanguages;   // display names
    int gameLanguage = -1;             // index in gameLanguages
    const LauncherCover* cover = nullptr;     // the selected game's
    const LauncherCover* covers[3] = {};      // every game's, for the library
    bool gameReady[3] = {};                   // each game's folder is chosen and valid
};

// What the user did this frame
struct LauncherIntent
{
    int selectGame = -1;
    int uiLanguage = -1;
    int gameLanguage = -1;
    bool play = false;          // the selected game
    bool playJack = false;
    bool restoreDefaults = false;
    bool openAdvanced = false;
    bool openControls = false;
    bool openAudio = false;
    bool browse = false;
    bool update = false;
    bool about = false;
    bool exit = false;
};

namespace launcherScreen
{

inline void tooltipWhenHovered(const char* text)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", text);
}

// Centred text that shrinks to fit maxWidth (long titles on a narrow screen)
inline void fittedCentredText(ImDrawList* draw, ImFont* font, float centerX, float y, float maxWidth, ImU32 color, const char* text)
{
    float size = font->FontSize;
    const float width = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
    if (width > maxWidth && width > 0.0f)
        size *= maxWidth / width;
    const float shown = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
    draw->AddText(font, size, ImVec2(centerX - shown * 0.5f, y), color, text);
}

// A drop-down list at a position and width; returns the chosen index or -1
inline int comboAt(const char* id, ImVec2 pos, float width, const char* current, const std::vector<std::string>& items,
                   int selected, bool enabled)
{
    int chosen = -1;
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(width);
    ImGui::BeginDisabled(!enabled);
    if (ImGui::BeginCombo(id, current))
    {
        for (int i = 0; i < (int)items.size(); i++)
            if (ImGui::Selectable(items[i].c_str(), i == selected))
                chosen = i;
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    return chosen;
}

inline std::vector<std::string> uiLanguageNames()
{
    std::vector<std::string> names;
    for (int i = 0; i < (int)UiLanguage::Count; i++)
        names.push_back(uiLanguageName((UiLanguage)i));
    return names;
}


// The size, in interface units, each layout was drawn for
static const ImVec2 kDesignLandscape(980.0f, 610.0f);
static const ImVec2 kDesignPortrait(480.0f, 900.0f);

// The screen's own colours (the windows it opens keep menuBase.h's palette)
static const ImU32 kBg        = IM_COL32(11, 12, 16, 255);
static const ImU32 kSide      = IM_COL32(14, 15, 20, 248);
static const ImU32 kBrand     = IM_COL32(232, 193, 90, 255);    // the gold of the covers' titles
static const ImU32 kWhite     = IM_COL32(255, 255, 255, 255);
static const ImU32 kTextMain  = IM_COL32(233, 230, 223, 255);
static const ImU32 kTextSoft  = IM_COL32(180, 176, 167, 255);
static const ImU32 kTextFaint = IM_COL32(119, 115, 107, 255);
static const ImU32 kReady     = IM_COL32(111, 207, 111, 255);
static const ImU32 kOnAccent  = IM_COL32(11, 12, 16, 255);       // text on the Play button

// A colour with another opacity (0-255)
inline ImU32 alpha(ImU32 colour, int value)
{
    return (colour & 0x00FFFFFF) | ((ImU32)value << 24);
}

// Each game's colour: menu::gameAccent (menuBase.h)
using Accent = menu::GameAccent;
using menu::gameAccent;

// A picture filling its frame (cropped, never stretched), rounded
inline void fillImage(ImDrawList* draw, ImTextureID picture, int width, int height, ImVec2 min, ImVec2 max, float rounding)
{
    if (!picture || width <= 0 || height <= 0)
        return;
    const float frameW = max.x - min.x, frameH = max.y - min.y;
    const float scale = (std::max)(frameW / width, frameH / height);
    const float u = frameW / (width * scale), v = frameH / (height * scale);
    draw->AddImageRounded(picture, min, max, ImVec2((1.0f - u) * 0.5f, (1.0f - v) * 0.5f),
        ImVec2((1.0f + u) * 0.5f, (1.0f + v) * 0.5f), IM_COL32_WHITE, rounding);
}

// Width of a text in a font at a size
inline float textWidth(ImFont* font, float size, const char* text)
{
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
}

static const char* const kGameNames[3] = { "Alone in the Dark", "Alone in the Dark 2", "Alone in the Dark 3" };
static const char* const kShortNames[3] = { "AITD", "AITD 2", "AITD 3" };
static const char* const kYears[3] = { "1992", "1993", "1994" };

// Places the parts of the screen; u() turns interface units into pixels
struct Parts
{
    const LauncherView& view;
    const menu::Metrics& m;
    const menu::Fonts& fonts;
    LauncherIntent& intent;
    ImDrawList* draw;
    bool linked;      // the selected game's folder is ready
    Accent accent;    // the selected game's colour

    // Size in pixels of `units` of the screen design
    float u(float units) const { return m.px(units); }

    void text(ImFont* font, float size, ImVec2 pos, ImU32 colour, const char* value)
    {
        draw->AddText(font, u(size), pos, colour, value);
    }

    // A clickable area drawn by hand (mouse, touch, keyboard, controller); true when pressed
    bool area(const char* id, ImVec2 pos, ImVec2 size, bool enabled, bool& hovered)
    {
        ImGui::SetCursorScreenPos(pos);
        ImGui::BeginDisabled(!enabled);
        const bool clicked = ImGui::InvisibleButton(id, size);
        ImGui::EndDisabled();
        hovered = enabled && (ImGui::IsItemHovered() || ImGui::IsItemFocused());
        return clicked && enabled;
    }

    // ---- Library ----------------------------------------------------------

    // "ALONE IN THE DARK" in the covers' gold and "CLASSIC REBORN" under it
    void brand(ImVec2 pos)
    {
        text(fonts.button, 16.0f, pos, kBrand, "ALONE IN THE DARK");
        text(fonts.smallText, 12.5f, ImVec2(pos.x, pos.y + u(21.0f)), kTextFaint, "CLASSIC REBORN");
    }

    // A game of the library: its cover small, name, year and a dot (green: ready)
    void libraryRow(int game, ImVec2 pos, float width, float barX)
    {
        const float height = m.pressHeight(64.0f);
        const ImVec2 corner(pos.x + width, pos.y + height);
        const bool selected = (view.selectedGame == game);
        const bool ready = view.gameReady[game];
        ImGui::PushID(game);
        bool hovered = false;
        if (area("##game", pos, ImVec2(width, height), true, hovered) && !selected)
            intent.selectGame = game;
        ImGui::PopID();
        if (selected || hovered)
            draw->AddRectFilled(pos, corner, alpha(kWhite, selected ? 18 : 10), u(10.0f));
        if (selected)
            draw->AddRectFilled(ImVec2(barX, pos.y + u(12.0f)), ImVec2(barX + u(3.0f), corner.y - u(12.0f)),
                gameAccent(game).light, u(2.0f));
        menu::navCursor(pos, corner, u(10.0f));

        const LauncherCover* cover = view.covers[game];
        const ImVec2 thumb(pos.x + u(8.0f), pos.y + (height - u(50.0f)) * 0.5f);
        draw->AddRectFilled(thumb, ImVec2(thumb.x + u(36.0f), thumb.y + u(50.0f)), alpha(kWhite, 12), u(5.0f));
        if (cover)
            fillImage(draw, ready ? cover->colour : cover->grey, cover->width, cover->height, thumb,
                ImVec2(thumb.x + u(36.0f), thumb.y + u(50.0f)), u(5.0f));

        const float textX = thumb.x + u(47.0f);
        const float dotX = corner.x - u(12.0f);
        draw->PushClipRect(ImVec2(textX, pos.y), ImVec2(dotX - u(8.0f), corner.y), true);
        const float nameY = pos.y + height * 0.5f - u(17.0f);
        text(fonts.button, 14.5f, ImVec2(textX, nameY), selected ? kWhite : kTextMain, kGameNames[game]);
        std::string detail = kYears[game];
        if (!ready)
            detail += std::string(" \xC2\xB7 ") + tr(UiText::NoFolderShort);
        text(fonts.smallText, 12.5f, ImVec2(textX, nameY + u(19.0f)), kTextFaint, detail.c_str());
        draw->PopClipRect();

        const ImVec2 dot(dotX, pos.y + height * 0.5f);
        if (ready)
            draw->AddCircleFilled(dot, u(3.5f), kReady);
        else
            draw->AddCircle(dot, u(3.5f), kTextFaint, 0, u(1.0f));
    }

    // A text link (About, Exit); returns its width
    float link(const char* id, const char* label, ImVec2 pos, bool& pressed)
    {
        const float width = textWidth(fonts.smallText, u(13.0f), label);
        bool hovered = false;
        if (area(id, ImVec2(pos.x - u(4.0f), pos.y - u(4.0f)), ImVec2(width + u(8.0f), m.pressHeight(22.0f)), true, hovered))
            pressed = true;
        menu::navCursor(ImVec2(pos.x - u(4.0f), pos.y - u(4.0f)), ImVec2(pos.x + width + u(4.0f), pos.y + m.pressHeight(22.0f) - u(4.0f)), u(4.0f));
        text(fonts.smallText, 13.0f, pos, hovered ? kTextMain : kTextFaint, label);
        return width;
    }

    // The gold Update button (only when a newer version was found); returns the y after it
    float updateButton(ImVec2 pos, float width)
    {
        if (!(view.canUpdate && view.updateAvailable))
            return pos.y;
        const float height = m.pressHeight(30.0f);
        bool hovered = false;
        if (area("##update", pos, ImVec2(width, height), true, hovered))
            intent.update = true;
        const ImVec2 corner(pos.x + width, pos.y + height);
        draw->AddRectFilled(pos, corner, alpha(kBrand, hovered ? 255 : 220), u(8.0f));
        menu::navCursor(pos, corner, u(8.0f));
        const char* label = tr(UiText::Update);
        text(fonts.button, 14.0f, ImVec2(pos.x + (width - textWidth(fonts.button, u(14.0f), label)) * 0.5f, pos.y + (height - u(13.0f)) * 0.5f - u(1.0f)),
            kOnAccent, label);
        return corner.y + u(10.0f);
    }

    // "Interface [English v]" at a width
    void interfaceLanguage(ImVec2 pos, float width)
    {
        const char* label = tr(UiText::InterfaceLanguage);
        ImGui::PushFont(fonts.smallText);
        const float labelW = ImGui::CalcTextSize(label).x;
        const float comboW = (std::max)(u(90.0f), width - labelW - u(10.0f));
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ImGui::GetStyle().FramePadding.y));
        ImGui::TextColored(menu::color(kTextFaint), "%s", label);
        const int chosen = comboAt("##uilang", ImVec2(pos.x + width - comboW, pos.y), comboW, uiLanguageName(uiLanguage()),
            uiLanguageNames(), (int)uiLanguage(), true);
        ImGui::PopFont();
        if (chosen >= 0)
            intent.uiLanguage = chosen;
    }

    void library(ImVec2 min, ImVec2 max)
    {
        draw->AddRectFilled(min, max, kSide);
        draw->AddLine(ImVec2(max.x - 1.0f, min.y), ImVec2(max.x - 1.0f, max.y), alpha(kWhite, 15));
        const float x = min.x + u(14.0f), width = max.x - min.x - u(28.0f);
        brand(ImVec2(x + u(8.0f), min.y + u(24.0f)));
        text(fonts.smallText, 12.0f, ImVec2(x + u(8.0f), min.y + u(84.0f)), kTextFaint, tr(UiText::Library));
        float y = min.y + u(104.0f);
        for (int game = 0; game < 3; game++)
        {
            libraryRow(game, ImVec2(x, y), width, min.x);
            y += m.pressHeight(64.0f) + u(4.0f);
        }

        // At the bottom: Update (when there is one), the interface language, About and Exit
        const float linksY = max.y - u(30.0f);
        ImGui::PushFont(fonts.smallText);
        const float comboY = linksY - u(16.0f) - ImGui::GetFrameHeight();
        ImGui::PopFont();
        const float updateY = comboY - u(10.0f) - (view.canUpdate && view.updateAvailable ? m.pressHeight(30.0f) : 0.0f);
        updateButton(ImVec2(x + u(8.0f), updateY), width - u(16.0f));
        interfaceLanguage(ImVec2(x + u(8.0f), comboY), width - u(16.0f));
        link("##about", tr(UiText::About), ImVec2(x + u(8.0f), linksY), intent.about);
        const char* exitLabel = tr(UiText::ExitShort);
        link("##exit", exitLabel, ImVec2(x + width - u(8.0f) - textWidth(fonts.smallText, u(13.0f), exitLabel), linksY), intent.exit);
    }

    // ---- The chosen game ---------------------------------------------------

    // The game's cover blurred behind, its colour in a corner and darker
    // towards the bottom
    void backdrop(ImVec2 min, ImVec2 max)
    {
        draw->AddRectFilled(min, max, kBg);
        if (view.cover && view.cover->background)
        {
            draw->PushClipRect(min, max, true);
            fillImage(draw, view.cover->background, view.cover->width, view.cover->height, min, max, 0.0f);
            draw->PopClipRect();
        }
        const ImU32 tint = linked ? accent.dark : IM_COL32(60, 60, 64, 255);
        draw->AddRectFilledMultiColor(min, max, alpha(tint, 70), alpha(tint, 10), alpha(tint, 0), alpha(tint, 24));
        const float middle = min.y + (max.y - min.y) * 0.45f;
        draw->AddRectFilledMultiColor(min, ImVec2(max.x, middle), alpha(kBg, 50), alpha(kBg, 50), alpha(kBg, 120), alpha(kBg, 120));
        draw->AddRectFilledMultiColor(ImVec2(min.x, middle), max, alpha(kBg, 120), alpha(kBg, 120), alpha(kBg, 245), alpha(kBg, 245));
    }

    // The cover with a glow of the game's colour and a soft shadow
    void cover(ImVec2 min, ImVec2 max)
    {
        const float rounding = u(14.0f);
        const ImU32 glow = linked ? accent.light : IM_COL32(120, 120, 124, 255);
        // A soft glow: many thin layers, each a little wider and rounder
        for (int i = 16; i >= 1; i--)
        {
            const float grow = u(2.5f * i);
            draw->AddRectFilled(ImVec2(min.x - grow, min.y - grow), ImVec2(max.x + grow, max.y + grow), alpha(glow, 4),
                rounding + grow * 1.6f);
        }
        for (int i = 4; i >= 1; i--)
        {
            const float grow = u(4.0f * i);
            draw->AddRectFilled(ImVec2(min.x - grow * 0.5f, min.y + u(10.0f)), ImVec2(max.x + grow * 0.5f, max.y + grow + u(12.0f)),
                IM_COL32(0, 0, 0, 40), rounding + grow);
        }
        draw->AddRectFilled(min, max, IM_COL32(24, 25, 30, 255), rounding);
        if (view.cover)
            fillImage(draw, linked ? view.cover->colour : view.cover->grey, view.cover->width, view.cover->height, min, max, rounding);
        draw->AddRect(min, max, alpha(kWhite, 26), rounding, 0, 1.0f);
    }

    // The cover's shape (height / width), 4:3 when there is none
    float coverAspect() const
    {
        if (view.cover && view.cover->width > 0)
            return (float)view.cover->height / (float)view.cover->width;
        return 4.0f / 3.0f;
    }

    // A rounded pill with an optional dot; returns its width
    float chip(ImVec2 pos, const char* label, ImU32 dot)
    {
        const float size = 12.5f;
        const float dotW = dot ? u(13.0f) : 0.0f;
        const float width = u(20.0f) + dotW + textWidth(fonts.smallText, u(size), label);
        const ImVec2 corner(pos.x + width, pos.y + u(26.0f));
        draw->AddRectFilled(pos, corner, alpha(kWhite, 18), u(13.0f));
        draw->AddRect(pos, corner, alpha(kWhite, 20), u(13.0f));
        if (dot)
            draw->AddCircleFilled(ImVec2(pos.x + u(14.0f), pos.y + u(13.0f)), u(3.5f), dot);
        text(fonts.smallText, size, ImVec2(pos.x + u(10.0f) + dotW, pos.y + u(13.0f) - u(size) * 0.55f), kTextMain, label);
        return width;
    }

    // What the folder is and whether Jack is in it; returns the y after them
    float chips(float x, float y)
    {
        if (!linked)
        {
            chip(ImVec2(x, y), tr(view.dataFolder.empty() ? UiText::NoFolderShort : UiText::DataFolderInvalid), kTextFaint);
            return y + u(26.0f);
        }
        float cx = x;
        if (!view.source.empty())
            cx += chip(ImVec2(cx, y), view.source.c_str(), kReady) + u(8.0f);
        if (view.selectedGame == 0 && view.jackFound)
            chip(ImVec2(cx, y), "+ Jack in the Dark", 0);
        return y + u(26.0f);
    }

    // Play in the game's colour, with its triangle
    void playButton(ImVec2 pos, ImVec2 size)
    {
        bool hovered = false;
        if (area("##play", pos, size, linked, hovered))
            intent.play = true;
        if (!linked)
            tooltipWhenHovered(tr(UiText::DataFolderMissing));
        const ImVec2 corner(pos.x + size.x, pos.y + size.y);
        const float rounding = u(12.0f);
        if (linked)
        {
            for (int i = 3; i >= 1; i--)
                draw->AddRectFilled(ImVec2(pos.x - u(2.0f * i), pos.y + u(4.0f)), ImVec2(corner.x + u(2.0f * i), corner.y + u(4.0f * i)),
                    alpha(accent.light, hovered ? 30 : 18), rounding + u(2.0f * i));
            // The light end, a sheen on its top half and a thin darker edge
            draw->AddRectFilled(pos, corner, alpha(accent.light, hovered ? 255 : 230), rounding);
            draw->AddRectFilled(pos, ImVec2(corner.x, pos.y + size.y * 0.5f), IM_COL32(255, 255, 255, hovered ? 40 : 26), rounding,
                ImDrawFlags_RoundCornersTop);
            draw->AddRect(pos, corner, alpha(accent.dark, 200), rounding, 0, u(1.0f));
        }
        else
        {
            draw->AddRectFilled(pos, corner, alpha(kWhite, 14), rounding);
            draw->AddRect(pos, corner, alpha(kWhite, 22), rounding);
        }
        menu::navCursor(pos, corner, rounding);
        const char* label = tr(UiText::PlayShort);
        const float textW = textWidth(fonts.button, u(16.0f), label);
        const float side = u(11.0f), gap = u(10.0f);
        const float startX = pos.x + (size.x - (side + gap + textW)) * 0.5f;
        const float midY = pos.y + size.y * 0.5f;
        const ImU32 colour = linked ? kOnAccent : kTextFaint;
        draw->AddTriangleFilled(ImVec2(startX, midY - side * 0.62f), ImVec2(startX, midY + side * 0.62f), ImVec2(startX + side, midY), colour);
        text(fonts.button, 16.0f, ImVec2(startX + side + gap, midY - u(15.0f) * 0.55f), colour, label);
    }

    // Width of a two-line button (a name and a small line under it)
    float twoLineWidth(const char* name, const char* detail, bool arrow) const
    {
        return (std::max)(textWidth(fonts.text, m.px(14.5f), name) + (arrow ? m.px(16.0f) : 0.0f),
                   textWidth(fonts.smallText, m.px(12.0f), detail)) + m.px(32.0f);
    }

    // A glass button with a name and a small line under it (arrow: it opens a list)
    bool twoLineButton(const char* id, const char* name, const char* detail, ImVec2 pos, ImVec2 size, bool enabled, bool arrow)
    {
        bool hovered = false;
        const bool pressed = area(id, pos, size, enabled, hovered);
        const ImVec2 corner(pos.x + size.x, pos.y + size.y);
        draw->AddRectFilled(pos, corner, alpha(kWhite, hovered ? 28 : 18), u(12.0f));
        draw->AddRect(pos, corner, hovered ? alpha(accent.light, 160) : alpha(kWhite, 26), u(12.0f));
        menu::navCursor(pos, corner, u(12.0f));
        const float x = pos.x + u(16.0f);
        const float top = pos.y + (size.y - u(14.5f) - u(3.0f) - u(12.0f)) * 0.5f - u(1.0f);
        text(fonts.text, 14.5f, ImVec2(x, top), enabled ? kTextMain : kTextFaint, name);
        if (arrow)
        {
            const float ax = x + textWidth(fonts.text, u(14.5f), name) + u(8.0f), ay = top + u(8.0f);
            draw->AddTriangleFilled(ImVec2(ax, ay - u(2.0f)), ImVec2(ax + u(7.0f), ay - u(2.0f)), ImVec2(ax + u(3.5f), ay + u(2.0f)),
                enabled ? kTextSoft : kTextFaint);
        }
        text(fonts.smallText, 12.0f, ImVec2(x, top + u(17.5f)), kTextFaint, detail);
        return pressed;
    }

    // The current game language's name
    const char* currentLanguage() const
    {
        return (view.gameLanguage >= 0 && view.gameLanguage < (int)view.gameLanguages.size())
            ? view.gameLanguages[view.gameLanguage].c_str() : tr(UiText::NoLanguageFound);
    }

    // The buttons next to Play: Jack in the Dark (AITD1) and the game
    // language; their widths (all the same when fullWidth)
    std::vector<float> sideButtons(float width, bool fullWidth) const
    {
        std::vector<float> widths;
        if (view.selectedGame == 0)
            widths.push_back(twoLineWidth("Jack in the Dark", tr(UiText::JackDetail), false));
        if (linked && !view.gameLanguages.empty())
            widths.push_back(twoLineWidth(currentLanguage(), tr(UiText::GameLanguage), true));
        if (fullWidth && !widths.empty())
        {
            const float each = (width - m.px(10.0f) * (widths.size() - 1)) / widths.size();
            for (float& w : widths)
                w = each;
        }
        return widths;
    }

    float playWidth() const
    {
        return (std::max)(m.px(120.0f), textWidth(fonts.button, m.px(16.0f), tr(UiText::PlayShort)) + m.px(64.0f));
    }

    // Rows of buttons the actions take (they wrap when they do not fit)
    int actionRows(float width, bool fullWidth) const
    {
        const std::vector<float> widths = sideButtons(width, fullWidth);
        if (fullWidth)
            return widths.empty() ? 1 : 2;
        const float gap = m.px(10.0f);
        float cx = playWidth() + gap;
        int rows = 1;
        for (float w : widths)
        {
            if (cx + w > width + 0.5f && cx > 0.0f)
            {
                cx = 0.0f;
                rows++;
            }
            cx += w + gap;
        }
        return rows;
    }

    // Play, then Jack in the Dark and the game language (wrapping to
    // another line when they do not fit); returns the y after them
    float actions(float x, float y, float width, bool fullWidth)
    {
        const float height = m.pressHeight(48.0f);
        const float gap = u(10.0f);
        const float playW = fullWidth ? width : playWidth();
        playButton(ImVec2(x, y), ImVec2(playW, height));
        float cx = fullWidth ? x : x + playW + gap;
        float cy = fullWidth ? y + height + gap : y;

        const std::vector<float> widths = sideButtons(width, fullWidth);
        const bool jack = (view.selectedGame == 0);
        for (size_t i = 0; i < widths.size(); i++)
        {
            if (cx + widths[i] > x + width + 0.5f && cx > x)
            {
                cx = x;
                cy += height + gap;
            }
            const ImVec2 pos(cx, cy), size(widths[i], height);
            if (jack && i == 0)
            {
                const bool jackReady = linked && view.jackFound;
                if (twoLineButton("##jack", "Jack in the Dark", tr(UiText::JackDetail), pos, size, jackReady, false))
                    intent.playJack = true;
                if (!jackReady)
                    tooltipWhenHovered(tr(linked ? UiText::JackMissing : UiText::DataFolderMissing));
            }
            else
            {
                const bool several = view.gameLanguages.size() > 1;
                if (twoLineButton("##gamelang", currentLanguage(), tr(UiText::GameLanguage), pos, size, several, several))
                    ImGui::OpenPopup("##gameLanguages");
                ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + size.y + u(4.0f)));
                ImGui::SetNextWindowSizeConstraints(ImVec2(size.x, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
                if (ImGui::BeginPopup("##gameLanguages"))
                {
                    for (int l = 0; l < (int)view.gameLanguages.size(); l++)
                        if (ImGui::Selectable(view.gameLanguages[l].c_str(), l == view.gameLanguage))
                            intent.gameLanguage = l;
                    ImGui::EndPopup();
                }
            }
            cx += widths[i] + gap;
        }
        if (fullWidth && widths.empty())
            cy -= height + gap;
        return cy + height;
    }

    // The little pictures of the cards, in the game's colour
    void cardIcon(int which, ImVec2 centre, ImU32 colour)
    {
        const float r = u(7.0f);
        if (which == 0)
        {
            // Options: a circle half filled (picture and effects)
            draw->AddCircle(centre, r, colour, 0, u(1.6f));
            draw->PathArcTo(centre, r, IM_PI * 0.5f, IM_PI * 1.5f);
            draw->PathFillConvex(colour);
        }
        else if (which == 1)
        {
            // Controls: a controller's outline and its two sticks
            const ImVec2 a(centre.x - u(9.0f), centre.y - u(5.0f)), b(centre.x + u(9.0f), centre.y + u(6.0f));
            draw->AddRect(a, b, colour, u(5.0f), 0, u(1.6f));
            draw->AddCircleFilled(ImVec2(centre.x - u(4.0f), centre.y + u(0.5f)), u(1.8f), colour);
            draw->AddCircleFilled(ImVec2(centre.x + u(4.0f), centre.y + u(0.5f)), u(1.8f), colour);
        }
        else
        {
            // Audio: a note
            draw->AddCircleFilled(ImVec2(centre.x - u(3.0f), centre.y + u(5.0f)), u(3.2f), colour);
            draw->AddLine(ImVec2(centre.x - u(0.2f), centre.y + u(5.0f)), ImVec2(centre.x - u(0.2f), centre.y - u(8.0f)), colour, u(1.6f));
            draw->AddLine(ImVec2(centre.x - u(0.2f), centre.y - u(8.0f)), ImVec2(centre.x + u(6.0f), centre.y - u(4.5f)), colour, u(1.6f));
        }
    }

    // A card: an icon, its name, a line of what it holds and an arrow
    bool card(int which, const char* name, const char* detail, ImVec2 pos, ImVec2 size)
    {
        bool hovered = false;
        ImGui::PushID(which);
        const bool pressed = area("##card", pos, size, linked, hovered);
        ImGui::PopID();
        const ImVec2 corner(pos.x + size.x, pos.y + size.y);
        const float rounding = u(12.0f);
        draw->AddRectFilled(pos, corner, hovered ? IM_COL32(30, 31, 38, 215) : IM_COL32(20, 21, 27, 190), rounding);
        draw->AddRect(pos, corner, hovered ? alpha(accent.light, 170) : alpha(kWhite, 20), rounding);
        menu::navCursor(pos, corner, rounding);
        const ImVec2 box(pos.x + u(14.0f), pos.y + (size.y - u(34.0f)) * 0.5f);
        draw->AddRectFilled(box, ImVec2(box.x + u(34.0f), box.y + u(34.0f)), alpha(kWhite, 16), u(9.0f));
        cardIcon(which, ImVec2(box.x + u(17.0f), box.y + u(17.0f)), linked ? accent.light : kTextFaint);
        const float textX = box.x + u(46.0f);
        const float arrowX = corner.x - u(16.0f);
        draw->PushClipRect(ImVec2(textX, pos.y), ImVec2(arrowX - u(6.0f), corner.y), true);
        const float nameY = pos.y + size.y * 0.5f - u(16.0f);
        text(fonts.button, 14.0f, ImVec2(textX, nameY), linked ? kTextMain : kTextFaint, name);
        text(fonts.smallText, 12.5f, ImVec2(textX, nameY + u(18.0f)), kTextFaint, detail);
        draw->PopClipRect();
        const float midY = pos.y + size.y * 0.5f;
        draw->AddLine(ImVec2(arrowX - u(3.0f), midY - u(5.0f)), ImVec2(arrowX + u(2.0f), midY), hovered ? kTextMain : kTextFaint, u(1.5f));
        draw->AddLine(ImVec2(arrowX + u(2.0f), midY), ImVec2(arrowX - u(3.0f), midY + u(5.0f)), hovered ? kTextMain : kTextFaint, u(1.5f));
        return pressed;
    }

    // "Settings" and Restore defaults over the three cards (a row, or a
    // column); returns the y after them
    float cards(float x, float y, float width, bool column)
    {
        text(fonts.smallText, 12.0f, ImVec2(x + u(2.0f), y + u(4.0f)), kTextFaint, tr(UiText::Settings));
        const char* restore = tr(UiText::RestoreDefaults);
        const float restoreW = textWidth(fonts.smallText, u(12.5f), restore);
        const ImVec2 restorePos(x + width - restoreW - u(2.0f), y + u(3.0f));
        bool hovered = false;
        const bool canRestore = linked && !view.isDefault;
        if (area("##defaults", ImVec2(restorePos.x - u(6.0f), restorePos.y - u(4.0f)), ImVec2(restoreW + u(12.0f), m.pressHeight(22.0f)),
            canRestore, hovered))
            intent.restoreDefaults = true;
        tooltipWhenHovered(tr(view.isDefault ? UiText::DefaultsHelp : UiText::DefaultsChanged));
        menu::navCursor(ImVec2(restorePos.x - u(6.0f), restorePos.y - u(4.0f)),
            ImVec2(restorePos.x + restoreW + u(6.0f), restorePos.y - u(4.0f) + m.pressHeight(22.0f)), u(4.0f));
        text(fonts.smallText, 12.5f, restorePos, canRestore ? (hovered ? kWhite : accent.light) : alpha(kTextFaint, 150), restore);
        y += u(26.0f);

        const char* names[3] = { tr(UiText::OptionsCard), tr(UiText::ControlsCard), tr(UiText::AudioCard) };
        const char* details[3] = { tr(UiText::OptionsDetail), tr(UiText::ControlsDetail), tr(UiText::AudioDetailAlone) };
        bool* actions[3] = { &intent.openAdvanced, &intent.openControls, &intent.openAudio };
        const float gap = u(12.0f);
        const float cardH = m.pressHeight(62.0f);
        const float cardW = column ? width : (width - gap * 2.0f) / 3.0f;
        for (int i = 0; i < 3; i++)
        {
            const ImVec2 pos = column ? ImVec2(x, y + (cardH + gap) * i) : ImVec2(x + (cardW + gap) * i, y);
            if (card(i, names[i], details[i], pos, ImVec2(cardW, cardH)))
                *actions[i] = true;
        }
        return y + (column ? (cardH + gap) * 3.0f - gap : cardH);
    }

    // The name's size: the title font, smaller when it does not fit
    float nameSize(float width) const
    {
        float size = fonts.title->FontSize;
        const float nameW = textWidth(fonts.title, size, kGameNames[view.selectedGame]);
        if (nameW > width)
            size *= width / nameW;
        return size;
    }

    const char* story() const
    {
        static const UiText stories[3] = { UiText::StoryAitd1, UiText::StoryAitd2, UiText::StoryAitd3 };
        return tr(stories[view.selectedGame]);
    }

    // Height of info() for a width, measured without drawing
    float infoHeight(float width, bool fullWidthButtons) const
    {
        const float wrap = (std::min)(width, m.px(470.0f));
        const float storyH = fonts.text->CalcTextSizeA(m.px(14.5f), FLT_MAX, wrap, story()).y;
        const int rows = actionRows(width, fullWidthButtons);
        return m.px(22.0f) + nameSize(width) + m.px(10.0f) + storyH + m.px(16.0f) + m.px(26.0f) + m.px(22.0f)
            + m.pressHeight(48.0f) * rows + m.px(10.0f) * (rows - 1);
    }

    // The game's maker and year, name, story, what its folder is, and the
    // buttons; returns the y after them
    float info(float x, float y, float width, bool fullWidthButtons)
    {
        const int game = view.selectedGame;
        char eyebrow[64];
        snprintf(eyebrow, sizeof(eyebrow), "INFOGRAMES \xC2\xB7 %s", kYears[game]);
        text(fonts.button, 12.5f, ImVec2(x, y), linked ? accent.light : kTextFaint, eyebrow);
        y += u(22.0f);
        const float size = nameSize(width);
        draw->AddText(fonts.title, size, ImVec2(x, y), kWhite, kGameNames[game]);
        y += size + u(10.0f);
        const float wrap = (std::min)(width, u(470.0f));
        draw->AddText(fonts.text, u(14.5f), ImVec2(x, y), kTextSoft, story(), nullptr, wrap);
        y += fonts.text->CalcTextSizeA(u(14.5f), FLT_MAX, wrap, story()).y + u(16.0f);
        y = chips(x, y) + u(22.0f);
        return actions(x, y, width, fullWidthButtons);
    }

    // ---- Bottom bar --------------------------------------------------------

    // A small glass button at its text's width; returns its width
    float smallButton(const char* id, const char* label, ImVec2 pos, float height, bool& pressed)
    {
        const float width = textWidth(fonts.text, u(14.0f), label) + u(28.0f);
        bool hovered = false;
        if (area(id, pos, ImVec2(width, height), true, hovered))
            pressed = true;
        const ImVec2 corner(pos.x + width, pos.y + height);
        draw->AddRectFilled(pos, corner, alpha(kWhite, hovered ? 30 : 18), u(8.0f));
        draw->AddRect(pos, corner, hovered ? alpha(accent.light, 170) : alpha(kWhite, 26), u(8.0f));
        menu::navCursor(pos, corner, u(8.0f));
        text(fonts.text, 14.0f, ImVec2(pos.x + u(14.0f), pos.y + (height - u(13.0f)) * 0.5f - u(1.0f)), kTextMain, label);
        return width;
    }

    // Text cut with "..." to a width, drawn in the small font
    void smallFitted(ImVec2 pos, ImU32 colour, const std::string& value, float width)
    {
        ImGui::PushFont(fonts.smallText);
        ImGui::SetCursorScreenPos(pos);
        menu::fittedText(colour, value, (std::max)(0.0f, width));
        ImGui::PopFont();
    }

    // The game folder, its state, the debug note, the version and Browse.
    // stacked: one under another (portrait). Returns the y after it.
    float statusBar(float x, float y, float width, bool stacked)
    {
        const float buttonH = m.pressHeight(30.0f);
        const char* label = tr(UiText::GameData);
        const std::string path = view.dataFolder.empty() ? std::string(tr(UiText::DataFolderMissing)) : view.dataFolder;
        const char* browse = tr(view.importGame ? UiText::ImportGame : UiText::Browse);
        const float lineH = fonts.smallText->FontSize;

        if (stacked)
        {
            text(fonts.smallText, 12.5f, ImVec2(x, y), kTextFaint, label);
            y += u(18.0f);
            smallFitted(ImVec2(x, y), linked ? kTextMain : menu::kError, path, width);
            y += u(20.0f);
            if (!view.message.empty())
            {
                draw->AddCircleFilled(ImVec2(x + u(4.0f), y + lineH * 0.5f), u(3.5f), view.messageColor);
                smallFitted(ImVec2(x + u(14.0f), y), view.messageColor, view.message, width - u(14.0f));
                y += u(22.0f);
            }
            if (view.debugMode)
            {
                text(fonts.smallText, 12.5f, ImVec2(x, y), menu::kError, tr(UiText::DebugMode));
                y += u(20.0f);
            }
            if (view.canBrowse)
            {
                const ImVec2 pos(x, y + u(4.0f)), corner(x + width, y + u(4.0f) + buttonH);
                bool hovered = false;
                if (area("##browse", pos, ImVec2(width, buttonH), true, hovered))
                    intent.browse = true;
                draw->AddRectFilled(pos, corner, alpha(kWhite, hovered ? 30 : 18), u(8.0f));
                draw->AddRect(pos, corner, alpha(kWhite, 26), u(8.0f));
                menu::navCursor(pos, corner, u(8.0f));
                text(fonts.text, 14.0f, ImVec2(x + (width - textWidth(fonts.text, u(14.0f), browse)) * 0.5f, pos.y + (buttonH - u(13.0f)) * 0.5f - u(1.0f)),
                    kTextMain, browse);
                y = corner.y;
            }
            return y;
        }

        const float barH = u(52.0f);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + barH), alpha(kBg, 235));
        draw->AddLine(ImVec2(x, y), ImVec2(x + width, y), alpha(kWhite, 15));
        const float padding = u(40.0f);
        const float midY = y + barH * 0.5f;
        float right = x + width - padding;
        if (view.canBrowse)
        {
            right -= textWidth(fonts.text, u(14.0f), browse) + u(28.0f);
            smallButton("##browse", browse, ImVec2(right, midY - buttonH * 0.5f), buttonH, intent.browse);
            right -= u(16.0f);
        }
        if (!view.version.empty())
        {
            const std::string version = "v" + view.version;
            right -= textWidth(fonts.smallText, u(12.5f), version.c_str());
            text(fonts.smallText, 12.5f, ImVec2(right, midY - u(6.0f)), kTextFaint, version.c_str());
            right -= u(16.0f);
        }
        if (view.debugMode)
        {
            const char* debug = tr(UiText::DebugMode);
            right -= textWidth(fonts.smallText, u(12.5f), debug);
            text(fonts.smallText, 12.5f, ImVec2(right, midY - u(6.5f)), menu::kError, debug);
            right -= u(16.0f);
        }

        float cx = x + padding;
        text(fonts.smallText, 13.0f, ImVec2(cx, midY - u(7.0f)), kTextFaint, label);
        cx += textWidth(fonts.smallText, u(13.0f), label) + u(12.0f);
        // The message takes what it needs (up to half of the room), the path the rest
        float messageW = 0.0f;
        if (!view.message.empty())
            messageW = (std::min)(textWidth(fonts.smallText, fonts.smallText->FontSize, view.message.c_str()) + u(20.0f), (right - cx) * 0.5f);
        const float pathRoom = right - cx - (messageW > 0.0f ? messageW + u(16.0f) : 0.0f);
        const float pathW = (std::min)(textWidth(fonts.smallText, fonts.smallText->FontSize, path.c_str()), pathRoom);
        smallFitted(ImVec2(cx, midY - lineH * 0.5f), linked ? kTextMain : menu::kError, path, pathRoom);
        if (messageW > 0.0f)
        {
            const float mx = cx + pathW + u(16.0f);
            draw->AddCircleFilled(ImVec2(mx + u(4.0f), midY), u(3.5f), view.messageColor);
            smallFitted(ImVec2(mx + u(14.0f), midY - lineH * 0.5f), view.messageColor, view.message, messageW - u(14.0f));
        }
        return y + barH;
    }

    // Version, Update, About and Exit (portrait, under everything)
    float footer(float x, float y, float width)
    {
        y = updateButton(ImVec2(x, y), width);
        if (!view.version.empty())
            text(fonts.smallText, 12.5f, ImVec2(x, y + u(2.0f)), kTextFaint, ("v" + view.version).c_str());
        const char* exitLabel = tr(UiText::ExitShort);
        const float exitW = textWidth(fonts.smallText, u(13.0f), exitLabel);
        link("##exit", exitLabel, ImVec2(x + width - exitW, y), intent.exit);
        const char* aboutLabel = tr(UiText::About);
        link("##about", aboutLabel, ImVec2(x + width - exitW - u(24.0f) - textWidth(fonts.smallText, u(13.0f), aboutLabel), y), intent.about);
        return y + u(22.0f);
    }
};

inline void landscape(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float sideW = m.px(216.0f);
    const ImVec2 size((std::max)(area.x, sideW + m.px(560.0f)), (std::max)(area.y, m.px(560.0f)));
    const ImVec2 mainMin(origin.x + sideW, origin.y), end(origin.x + size.x, origin.y + size.y);

    p.backdrop(mainMin, end);
    p.library(origin, ImVec2(mainMin.x, end.y));

    // The game: cover and info side by side, the cards under them, the bar
    // at the bottom; centred in the height above the bar
    const float pad = m.px(36.0f);
    const float barH = m.px(52.0f);
    const float x = mainMin.x + pad, width = end.x - pad - x;
    const float coverW = (std::min)(m.px(262.0f), width * 0.36f);
    const float coverH = coverW * p.coverAspect();
    const float infoX = x + coverW + m.px(34.0f), infoW = x + width - infoX;
    const float infoH = p.infoHeight(infoW, false);
    const float heroH = (std::max)(coverH, infoH);
    const float cardsH = m.px(26.0f) + m.pressHeight(62.0f);
    const float contentH = heroH + m.px(30.0f) + cardsH;
    const float freeH = (end.y - barH) - origin.y;
    const float top = origin.y + (std::max)(m.px(28.0f), (freeH - contentH) * 0.5f);

    p.cover(ImVec2(x, top + heroH - coverH), ImVec2(x + coverW, top + heroH));
    p.info(infoX, top + heroH - infoH - m.px(4.0f), infoW, false);
    p.cards(x, top + heroH + m.px(30.0f), width, false);
    p.statusBar(mainMin.x, end.y - barH, end.x - mainMin.x, false);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, end.y - 1.0f));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}

// One column: brand and interface language, the games in a row, cover, info,
// cards, folder and the links
inline void portrait(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float colW = (std::min)(area.x - m.px(32.0f), m.px(440.0f));
    const float x = origin.x + (area.x - colW) * 0.5f;
    float y = origin.y + m.px(20.0f);

    p.brand(ImVec2(x, y));
    p.interfaceLanguage(ImVec2(x + colW - m.px(190.0f), y + m.px(2.0f)), m.px(190.0f));
    y += m.px(54.0f);

    // The three games, side by side: cover small and short name
    const float gap = m.px(8.0f);
    const float tileW = (colW - gap * 2.0f) / 3.0f, tileH = m.pressHeight(56.0f);
    for (int game = 0; game < 3; game++)
    {
        const ImVec2 pos(x + (tileW + gap) * game, y), corner(pos.x + tileW, y + tileH);
        const bool selected = (p.view.selectedGame == game);
        ImGui::PushID(game);
        bool hovered = false;
        if (p.area("##game", pos, ImVec2(tileW, tileH), true, hovered) && !selected)
            p.intent.selectGame = game;
        ImGui::PopID();
        p.draw->AddRectFilled(pos, corner, alpha(kWhite, selected ? 22 : (hovered ? 14 : 8)), m.px(10.0f));
        if (selected)
            p.draw->AddRectFilled(ImVec2(pos.x + m.px(10.0f), corner.y - m.px(3.0f)), ImVec2(corner.x - m.px(10.0f), corner.y),
                gameAccent(game).light, m.px(2.0f));
        menu::navCursor(pos, corner, m.px(10.0f));
        const LauncherCover* cover = p.view.covers[game];
        const ImVec2 thumb(pos.x + m.px(8.0f), pos.y + (tileH - m.px(40.0f)) * 0.5f);
        if (cover)
            fillImage(p.draw, p.view.gameReady[game] ? cover->colour : cover->grey, cover->width, cover->height, thumb,
                ImVec2(thumb.x + m.px(29.0f), thumb.y + m.px(40.0f)), m.px(4.0f));
        p.text(p.fonts.button, 13.5f, ImVec2(thumb.x + m.px(37.0f), pos.y + tileH * 0.5f - m.px(8.0f)),
            selected ? kWhite : kTextSoft, kShortNames[game]);
    }
    y += tileH + m.px(26.0f);

    const float coverW = (std::min)(colW * 0.56f, m.px(230.0f));
    const float coverH = coverW * p.coverAspect();
    const float coverX = x + (colW - coverW) * 0.5f;
    p.cover(ImVec2(coverX, y), ImVec2(coverX + coverW, y + coverH));
    y += coverH + m.px(30.0f);

    y = p.info(x, y, colW, true) + m.px(26.0f);
    y = p.cards(x, y, colW, true) + m.px(26.0f);
    p.draw->AddLine(ImVec2(x, y), ImVec2(x + colW, y), alpha(kWhite, 15));
    y = p.statusBar(x, y + m.px(16.0f), colW, true) + m.px(22.0f);
    y = p.footer(x, y, colW);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, y + m.px(16.0f)));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}


} // namespace launcherScreen


// Draws the whole screen in a window covering the display and returns what
// the user did. The host opens its own windows (options, controls...) afterwards.
inline LauncherIntent drawLauncherScreen(const LauncherView& view, const menu::Metrics& metrics, const menu::Fonts& fonts)
{
    using namespace launcherScreen;
    LauncherIntent intent;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    // The lists of the screen in its glass colours
    const Accent accent = gameAccent(view.selectedGame);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, menu::color(alpha(kWhite, 14)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, menu::color(alpha(kWhite, 24)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, menu::color(alpha(kWhite, 30)));
    ImGui::PushStyleColor(ImGuiCol_Border, menu::color(alpha(kWhite, 26)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, menu::color(IM_COL32(22, 23, 29, 250)));
    ImGui::PushStyleColor(ImGuiCol_Header, menu::color(alpha(accent.dark, 150)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, menu::color(alpha(accent.dark, 200)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, menu::color(accent.dark));
    ImGui::PushStyleColor(ImGuiCol_Button, menu::color(alpha(kWhite, 14)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, menu::color(alpha(kWhite, 24)));
    ImGui::PushStyleColor(ImGuiCol_Text, menu::color(kTextMain));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, metrics.px(8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, metrics.px(8.0f));

    // In portrait the cover blurred fills the window behind the column
    if (metrics.portrait())
    {
        Parts back{ view, metrics, fonts, intent, ImGui::GetWindowDrawList(), view.dataFolderValid, accent };
        back.backdrop(ImVec2(0, 0), io.DisplaySize);
    }

    // Scrolls when the content is taller than the window (portrait, or a
    // very small window); dragging scrolls it on a touch screen
    ImGui::BeginChild("##launcherContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NavFlattened);
    ImGui::PushFont(fonts.text);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 area = ImGui::GetContentRegionAvail();

    // LB / RB change the game (not while a list is open)
    if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false))
            intent.selectGame = (view.selectedGame + 2) % 3;
        else if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false))
            intent.selectGame = (view.selectedGame + 1) % 3;
    }

    Parts parts{ view, metrics, fonts, intent, ImGui::GetWindowDrawList(), view.dataFolderValid, accent };
    if (metrics.portrait())
        portrait(parts, origin, area);
    else
        landscape(parts, origin, area);
    menu::dragScroll();

    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(11);
    ImGui::End();
    return intent;
}

