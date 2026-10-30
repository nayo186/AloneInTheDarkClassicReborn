///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The launcher screen: game tabs, cover, title, play buttons, settings, game
// language, data folder and messages. Only layout and drawing: the host
// (Launcher.exe today, the game itself on Android) fills a LauncherView and
// acts on the LauncherIntent it gets back, so the same screen works in both.
// Two layouts from the same parts (menuBase.h metrics):
//  - landscape (a PC window, a phone on its side): cover on the left, menu on
//    the right, data folder at the bottom, as the original design;
//  - portrait (a narrow window, a phone upright): one column that scrolls.
// Everything sits in a scrolling area, so nothing is ever cut off; on a touch
// screen it scrolls by dragging.
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
    ImTextureID grey = 0;      // shown while the game has no data folder
    int width = 0;
    int height = 0;
};

// What the screen shows; the host fills it every frame
struct LauncherView
{
    int selectedGame = 0;              // 0 AITD1, 1 AITD2, 2 AITD3
    bool dataFolderValid = false;      // AITD1's data folder is usable
    bool jackFound = false;            // Jack in the Dark is in it (JACK\PERE.PAK)
    bool isDefault = true;             // no advanced option changed
    bool debugMode = false;            // noted at the bottom (launcher.cfg debugMode = 1)
    bool canBrowse = true;             // a folder picker exists (not on every system)
    std::string version;               // this build's version, shown at the bottom right
    bool canUpdate = false;            // an Update button (Launcher.exe of a published version)
    bool updateAvailable = false;      // a newer version was found: the button is gold
    std::string dataFolder;            // UTF-8, empty: none chosen
    std::string message;               // bottom line, empty: none
    ImU32 messageColor = menu::kTextDim;
    std::vector<std::string> gameLanguages;   // display names
    int gameLanguage = -1;             // index in gameLanguages
    const LauncherCover* cover = nullptr;
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
    bool browse = false;
    bool update = false;
    bool exit = false;
};

namespace launcherScreen
{

// The size, in interface units, each layout was drawn for
static const ImVec2 kDesignLandscape(940.0f, 600.0f);
static const ImVec2 kDesignPortrait(480.0f, 860.0f);

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

// The cover filling its frame (cropped if it is not exactly 3:4)
inline void drawCover(ImDrawList* draw, const LauncherView& view, bool linked, ImVec2 artMin, ImVec2 artMax, float border)
{
    draw->AddRectFilled(ImVec2(artMin.x - border, artMin.y - border), ImVec2(artMax.x + border, artMax.y + border), menu::kPanel, 4.0f);
    draw->AddRect(ImVec2(artMin.x - border, artMin.y - border), ImVec2(artMax.x + border, artMax.y + border), menu::kPanelBorder, 4.0f, 0, 2.0f);
    const ImTextureID picture = view.cover ? (linked ? view.cover->colour : view.cover->grey) : 0;
    if (picture && view.cover->width > 0 && view.cover->height > 0)
    {
        const float frameW = artMax.x - artMin.x, frameH = artMax.y - artMin.y;
        const float scale = (std::max)(frameW / view.cover->width, frameH / view.cover->height);
        const float u = frameW / (view.cover->width * scale), v = frameH / (view.cover->height * scale);
        draw->AddImage(picture, artMin, artMax, ImVec2((1.0f - u) * 0.5f, (1.0f - v) * 0.5f), ImVec2((1.0f + u) * 0.5f, (1.0f + v) * 0.5f));
    }
    else
    {
        draw->AddRectFilledMultiColor(artMin, artMax, IM_COL32(40, 42, 52, 255), IM_COL32(40, 42, 52, 255),
            IM_COL32(18, 18, 22, 255), IM_COL32(18, 18, 22, 255));
    }
    draw->AddRect(artMin, artMax, linked ? menu::kGold : menu::kPanelBorder, 0.0f, 0, 1.0f);
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

// Places the parts of the screen; 'u' turns interface units into pixels
struct Parts
{
    const LauncherView& view;
    const menu::Metrics& m;
    const menu::Fonts& fonts;
    LauncherIntent& intent;
    ImDrawList* draw;
    bool isAitd1;
    bool linked;

    // Size in pixels of `units` of the screen design
    float u(float units) const { return m.px(units); }

    void tabs(ImVec2 pos, float width, float height, bool shortNames)
    {
        static const char* const names[] = { "Alone in the Dark", "Alone in the Dark 2", "Alone in the Dark 3" };
        static const char* const shortLabels[] = { "AITD", "AITD 2", "AITD 3" };
        const float gap = u(10.0f);
        const float tabW = (width - gap * 2.0f) / 3.0f;
        for (int i = 0; i < 3; i++)
        {
            ImGui::PushID(i);
            if (menu::darkButton("##tab", shortNames ? shortLabels[i] : names[i], ImVec2(pos.x + (tabW + gap) * i, pos.y),
                ImVec2(tabW, height), fonts.text, true, view.selectedGame == i))
                intent.selectGame = i;
            ImGui::PopID();
        }
    }

    // "Interface [English v]" ending at rightX
    void interfaceLanguage(float rightX, float y)
    {
        const float comboW = u(140.0f);
        const char* label = tr(UiText::InterfaceLanguage);
        const float labelW = ImGui::CalcTextSize(label).x;
        const float x = rightX - comboW - ImGui::GetStyle().ItemSpacing.x - labelW;
        ImGui::SetCursorScreenPos(ImVec2(x, y + ImGui::GetStyle().FramePadding.y));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", label);
        const int chosen = comboAt("##uilang", ImVec2(rightX - comboW, y), comboW, uiLanguageName(uiLanguage()),
            uiLanguageNames(), (int)uiLanguage(), true);
        if (chosen >= 0)
            intent.uiLanguage = chosen;
    }

    void title(float centerX, float y, float width)
    {
        static const char* const titles[] = { "ALONE IN THE DARK", "ALONE IN THE DARK 2", "ALONE IN THE DARK 3" };
        static const char* const years[] = { "1992", "1993", "1994" };
        fittedCentredText(draw, fonts.title, centerX, y, width, linked ? menu::kGold : menu::kTextDisabled, titles[view.selectedGame]);
        char subtitle[160];
        snprintf(subtitle, sizeof(subtitle), "%s  (%s)", tr(UiText::Subtitle), years[view.selectedGame]);
        fittedCentredText(draw, fonts.text, centerX, y + u(48.0f), width, menu::kText, subtitle);
        fittedCentredText(draw, fonts.text, centerX, y + u(72.0f), width, menu::kTextDim, "Classic Reborn");
    }

    // Play (and Jack under it for AITD1); returns the y after them
    float playButtons(float x, float y, float width)
    {
        static const UiText playTexts[] = { UiText::PlayAitd1, UiText::PlayAitd2, UiText::PlayAitd3 };
        const float playH = m.pressHeight(54.0f);
        if (menu::goldButton("##play", tr(playTexts[view.selectedGame]), ImVec2(x, y), ImVec2(width, playH), fonts.button, linked))
            intent.play = true;
        if (!linked)
            tooltipWhenHovered(tr(isAitd1 ? UiText::DataFolderMissing : UiText::ComingSoon));
        y += playH + u(10.0f);
        if (isAitd1)
        {
            const bool jackReady = linked && view.jackFound;
            const float jackH = m.pressHeight(38.0f);
            if (menu::darkButton("##jack", tr(UiText::PlayJack), ImVec2(x, y), ImVec2(width, jackH), fonts.button, jackReady))
                intent.playJack = true;
            if (!jackReady)
                tooltipWhenHovered(tr(linked ? UiText::JackMissing : UiText::DataFolderMissing));
            y += jackH + u(10.0f);
        }
        return y;
    }

    // Settings, game language, advanced options and exit; returns the y after them
    float settings(float x, float y, float width)
    {
        const float lineH = ImGui::GetTextLineHeight();
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", tr(UiText::Settings));
        y += lineH + u(6.0f);
        const float defaultsH = m.pressHeight(36.0f);
        if (menu::darkButton("##defaults", tr(UiText::Defaults), ImVec2(x, y), ImVec2(width, defaultsH), fonts.button, linked, view.isDefault)
            && !view.isDefault)
            intent.restoreDefaults = true;
        y += defaultsH + u(6.0f);
        if (linked)
        {
            ImGui::SetCursorScreenPos(ImVec2(x, y));
            menu::fittedText(menu::kTextDim, tr(view.isDefault ? UiText::DefaultsHelp : UiText::DefaultsChanged), width);
        }
        y += lineH + u(12.0f);

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", tr(UiText::GameLanguage));
        y += lineH + u(6.0f);
        const char* current = "";
        if (linked)
            current = (view.gameLanguage >= 0 && view.gameLanguage < (int)view.gameLanguages.size())
                ? view.gameLanguages[view.gameLanguage].c_str() : tr(UiText::NoLanguageFound);
        const int chosen = comboAt("##gamelang", ImVec2(x, y), width, current, view.gameLanguages, view.gameLanguage,
            linked && !view.gameLanguages.empty());
        if (chosen >= 0)
            intent.gameLanguage = chosen;
        y += ImGui::GetFrameHeight() + u(16.0f);

        const float buttonH = m.pressHeight(38.0f);
        if (menu::darkButton("##advanced", advancedText(AdvancedText::Title, (int)uiLanguage()), ImVec2(x, y), ImVec2(width, buttonH),
            fonts.button, linked))
            intent.openAdvanced = true;
        y += buttonH + u(10.0f);
        if (menu::darkButton("##exit", tr(UiText::Exit), ImVec2(x, y), ImVec2(width, buttonH), fonts.button, true))
            intent.exit = true;
        return y + buttonH;
    }

    // Data folder line (label, path, Browse) and the message line. inline:
    // Browse on the same line as the path (landscape) or under it (portrait).
    // Returns the y after them.
    float bottom(float x, float y, float width, bool browseInline)
    {
        const float lineH = ImGui::GetTextLineHeight();
        const float browseW = browseInline ? u(130.0f) : width;
        const float browseH = m.pressHeight(ImGui::GetFrameHeight() / m.scale);
        draw->AddLine(ImVec2(x, y), ImVec2(x + width, y), menu::kPanelBorder);
        y += u(10.0f);

        const char* label = tr(UiText::DataFolder);
        const float pathEnd = browseInline ? x + width - browseW - u(16.0f) : x + width;
        float pathX = x, pathY = y;
        ImGui::SetCursorScreenPos(ImVec2(x, y + (browseInline ? u(4.0f) : 0.0f)));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s:", label);
        if (browseInline)
        {
            ImGui::SameLine();
            pathX = ImGui::GetCursorScreenPos().x;
            pathY = ImGui::GetCursorScreenPos().y;
        }
        else
        {
            pathY = y + lineH + u(4.0f);
        }
        ImGui::SetCursorScreenPos(ImVec2(pathX, pathY));
        if (isAitd1)
            menu::fittedText(view.dataFolderValid ? menu::kText : menu::kError,
                view.dataFolder.empty() ? std::string(tr(UiText::DataFolderMissing)) : view.dataFolder, pathEnd - pathX);
        else
            ImGui::TextColored(menu::color(menu::kTextDisabled), "%s", tr(UiText::ComingSoon));

        if (view.canBrowse)
        {
            const ImVec2 browsePos = browseInline ? ImVec2(x + width - browseW, y) : ImVec2(x, pathY + lineH + u(8.0f));
            ImGui::SetCursorScreenPos(browsePos);
            ImGui::BeginDisabled(!isAitd1);
            if (ImGui::Button(tr(UiText::Browse), ImVec2(browseW, browseH)))
                intent.browse = true;
            ImGui::EndDisabled();
            y = browseInline ? y + browseH : browsePos.y + browseH;
        }
        else
        {
            y = pathY + lineH;
        }
        y += u(8.0f);

        // Version and Update on the right of the message line (landscape) or
        // on a line of their own (portrait)
        const float rowEnd = versionRow(x + width, y);
        float noteW = browseInline ? x + width - rowEnd : 0.0f;
        if (!browseInline && rowEnd < x + width)
            y += ImGui::GetFrameHeight() + u(8.0f);

        // Message on the left, the debug mode note on the right
        if (view.debugMode)
        {
            const float debugW = ImGui::CalcTextSize(tr(UiText::DebugMode)).x;
            noteW += debugW + (noteW > 0.0f ? u(16.0f) : 0.0f);
            ImGui::SetCursorScreenPos(ImVec2(x + width - noteW, y + (browseInline ? textOffset() : 0.0f)));
            ImGui::TextColored(menu::color(menu::kError), "%s", tr(UiText::DebugMode));
        }
        if (!view.message.empty())
        {
            ImGui::SetCursorScreenPos(ImVec2(x, y + (browseInline ? textOffset() : 0.0f)));
            menu::fittedText(view.messageColor, view.message, width - noteW - (noteW > 0.0f ? u(16.0f) : 0.0f));
        }
        return y + (browseInline ? ImGui::GetFrameHeight() : lineH);
    }

    // Text on a line with buttons, centred on them
    float textOffset() const { return ImGui::GetStyle().FramePadding.y; }

    // "v1.0.0-beta.8 [Update]" ending at rightX; returns where it starts
    float versionRow(float rightX, float y)
    {
        if (view.version.empty())
            return rightX;
        float left = rightX;
        if (view.canUpdate)
        {
            const char* label = tr(UiText::Update);
            const float buttonW = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 4.0f;
            left -= buttonW;
            ImGui::SetCursorScreenPos(ImVec2(left, y));
            if (view.updateAvailable)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, menu::color(menu::kGold));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.16f, 0.13f, 0.09f, 1.0f));
            }
            if (ImGui::Button(tr(UiText::Update), ImVec2(buttonW, m.pressHeight(ImGui::GetFrameHeight() / m.scale))))
                intent.update = true;
            if (view.updateAvailable)
                ImGui::PopStyleColor(2);
            left -= u(10.0f);
        }
        const std::string version = "v" + view.version;
        left -= ImGui::CalcTextSize(version.c_str()).x;
        ImGui::SetCursorScreenPos(ImVec2(left, y + textOffset()));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", version.c_str());
        return left;
    }
};

inline void landscape(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float margin = m.px(24.0f);
    // The design is centred; the header and the bottom take the whole width
    const float blockW = m.px(kDesignLandscape.x);
    const float left = origin.x + (std::max)(0.0f, (area.x - blockW) * 0.5f);
    const float fullLeft = origin.x + margin, fullW = area.x - margin * 2.0f;

    // Header: tabs on the left, interface language on the right
    const float tabsW = (std::min)(m.px(620.0f), fullW - m.px(260.0f));
    p.tabs(ImVec2(fullLeft, origin.y + m.px(16.0f)), tabsW, m.pressHeight(34.0f), tabsW < m.px(560.0f));
    p.interfaceLanguage(fullLeft + fullW, origin.y + m.px(20.0f));

    // Bottom bar anchored to the bottom of the area (or under the menu)
    const float menuTop = origin.y + m.px(64.0f);
    const float coverH = m.px(456.0f), coverW = m.px(342.0f);
    const float rightX = left + m.px(400.0f), rightW = m.px(516.0f);
    const float bottomH = m.px(84.0f);
    const float middleH = (std::max)(coverH, m.px(456.0f));
    const float freeH = area.y - (menuTop - origin.y) - bottomH;
    const float middleTop = menuTop + (std::max)(0.0f, (freeH - middleH) * 0.5f);

    drawCover(p.draw, p.view, p.linked, ImVec2(left + margin, middleTop), ImVec2(left + margin + coverW, middleTop + coverH), m.px(6.0f));

    p.title(rightX + rightW * 0.5f, middleTop - m.px(6.0f), rightW);
    float y = p.playButtons(rightX, middleTop + m.px(100.0f), rightW);
    y = p.settings(rightX, y, rightW);

    const float bottomTop = (std::max)(y + m.px(6.0f), origin.y + area.y - bottomH);
    const float end = p.bottom(fullLeft, bottomTop, fullW, true);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, end));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}

inline void portrait(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float colW = (std::min)(area.x - m.px(32.0f), m.px(520.0f));
    const float x = origin.x + (area.x - colW) * 0.5f;
    float y = origin.y + m.px(16.0f);

    p.tabs(ImVec2(x, y), colW, m.pressHeight(34.0f), colW < m.px(600.0f));
    y += m.pressHeight(34.0f) + m.px(12.0f);
    p.interfaceLanguage(x + colW, y);
    y += ImGui::GetFrameHeight() + m.px(16.0f);

    p.title(x + colW * 0.5f, y, colW);
    y += m.px(104.0f);

    const float coverW = (std::min)(colW * 0.62f, m.px(270.0f));
    const float coverH = coverW * 4.0f / 3.0f;
    const float coverX = x + (colW - coverW) * 0.5f;
    drawCover(p.draw, p.view, p.linked, ImVec2(coverX, y), ImVec2(coverX + coverW, y + coverH), m.px(5.0f));
    y += coverH + m.px(20.0f);

    y = p.playButtons(x, y, colW);
    y = p.settings(x, y, colW);
    y = p.bottom(x, y + m.px(20.0f), colW, false);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, y + m.px(16.0f)));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}

} // namespace launcherScreen

// Draws the whole screen in a window covering the display and returns what
// the user did. The host opens its own popups (advanced options) afterwards.
inline LauncherIntent drawLauncherScreen(const LauncherView& view, const menu::Metrics& metrics, const menu::Fonts& fonts)
{
    LauncherIntent intent;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, menu::color(menu::kBackground));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    // Scrolls when the content is taller than the window (portrait, or a
    // very small window); dragging scrolls it on a touch screen
    ImGui::BeginChild("##launcherContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NavFlattened);
    ImGui::PushFont(fonts.text);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 area = ImGui::GetContentRegionAvail();

    // LB / RB change the game tab (not while a list is open)
    if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false))
            intent.selectGame = (view.selectedGame + 2) % 3;
        else if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false))
            intent.selectGame = (view.selectedGame + 1) % 3;
    }

    launcherScreen::Parts parts{ view, metrics, fonts, intent, ImGui::GetWindowDrawList(), view.selectedGame == 0, false };
    parts.linked = parts.isAitd1 && view.dataFolderValid;
    if (metrics.portrait())
        launcherScreen::portrait(parts, origin, area);
    else
        launcherScreen::landscape(parts, origin, area);
    menu::dragScroll();

    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::End();
    return intent;
}
