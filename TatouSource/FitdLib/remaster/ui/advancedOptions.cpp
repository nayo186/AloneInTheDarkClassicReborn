///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// In-game advanced options window (F1), styled like the launcher
// (replaces the remaster's remasterOptions.cpp)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "advancedOptions.h"
#include "advancedOptionsText.h"
#include "settings.h"
#include "consoleLog.h"
#include "bgfxGlue.h"
#include "customBackgrounds.h"
#include "debugCheats.h"
#include "controls.h"
#include "controlsWindow.h"
#include "audioWindow.h"
#include "input.h"
#include "ui/menuBase.h"
#include "menuFonts.h"
#include "ui/menuGamepad.h"
#include "bgfxGlue.h"

#include <imgui.h>
#include <atomic>

static std::atomic_bool s_open(false);

bool advancedOptionsIsOpen()
{
    return s_open.load(std::memory_order_acquire);
}

bool advancedOptionsPausesGame()
{
    // The game also waits while an export runs (customBackgrounds.cpp) and
    // while the debug item window is open (debugCheats.cpp)
    return advancedOptionsIsOpen() || customBackgroundsExportBusy() || debugCheatsWindowOpen();
}

void advancedOptionsToggle()
{
    s_open.store(!advancedOptionsIsOpen(), std::memory_order_release);
}

// The controls window waits for a new input (input.cpp then leaves the
// "Options window" controls alone: they may be the new input)
static std::atomic_bool s_capturing(false);

bool advancedOptionsCapturingInput()
{
    return s_capturing.load(std::memory_order_acquire);
}

static void advancedOptionsClose()
{
    s_open.store(false, std::memory_order_release);
}

// Controls window (Shared/controlsWindow.h): shown instead of the options
// window, so they never overlap; changes apply at once (controls.cpp) and are
// saved with the rest when F1 closes
static ControlsWindowState s_controls;
static std::vector<std::vector<InputBinding>> s_controlBindings;

// Audio window (Shared/audioWindow.h): the volumes apply at once (the mixer
// reads g_settings.audio every frame) and are saved when F1 closes
static AudioWindowState s_audio;

static void openControlsWindow()
{
    s_controlBindings.assign(kGameActionCount, {});
    for (int a = 0; a < kGameActionCount; a++)
        s_controlBindings[a] = controlsBindings((GameAction)a);
    s_controls.open = true;
    s_controls.justOpened = true;
}

// The launcher's colors (Shared/ui/menuBase.h); the panel lets the game show through a little
static const ImVec4 kPanel = menu::color(menu::kPanel, 0.97f);
static const ImVec4 kGold = menu::color(menu::kGold);
static const ImVec4 kGoldLight = menu::color(menu::kGoldLight);
static const ImVec4 kText = menu::color(menu::kText);
static const ImVec4 kTextDisabled = menu::color(menu::kTextDisabled);
static const ImVec4 kButtonDark = menu::color(menu::kButtonDark);
static const ImVec4 kButtonHover = menu::color(menu::kButtonHover);
static const ImVec4 kHeader = menu::color(menu::kHeader);

// The size, in interface units, the F1 windows are drawn for (a 900-pixel-high
// screen gives scale 1, as before; a phone upright uses the narrow one)
static const ImVec2 kDesignLandscape(1200.0f, 900.0f);
static const ImVec2 kDesignPortrait(560.0f, 900.0f);

// Texts in the language of the game (launcher order: EN, ES, FR, IT, DE)
static int textLanguage()
{
    if (languageNameString == "ESPAGNOL") return 1;
    if (languageNameString == "FRANCAIS") return 2;
    if (languageNameString == "ITALIANO") return 3;
    if (languageNameString == "DEUTSCH") return 4;
    return 0;
}

static const char* text(AdvancedText id)
{
    return advancedText(id, textLanguage());
}

// "(?)": mouse, touch, keyboard and controller (Shared/ui/menuBase.h)
using menu::helpMarker;

// Next widget at the column of the drop-down lists, or right after a long
// label and its "(?)" so the list never covers them
static void comboColumn(float column)
{
    ImGui::SameLine();
    if (ImGui::GetCursorPosX() < column)
        ImGui::SetCursorPosX(column);
}

static void optionCheckbox(AdvancedText label, AdvancedText help, bool* value, const char* logName)
{
    if (ImGui::Checkbox(text(label), value))
        printf(CFG_TAG "Advanced option (F1): %s=%s\n", logName, *value ? "true" : "false");
    helpMarker(text(help));
}

static void resolutionLabel(int resolution, char* label, size_t size)
{
    if (resolution > 0)
        snprintf(label, size, "%d × %d", resolution, advancedResolutionHeight(resolution));
    else
        snprintf(label, size, "%s", text(AdvancedText::ResolutionAuto));
}

static void resolutionCombo(float width, int& resolution)
{
    char current[64];
    resolutionLabel(resolution, current, sizeof(current));
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text(AdvancedText::Resolution));
    helpMarker(text(AdvancedText::ResolutionHelp));
    comboColumn(width * 0.4f);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##resolution", current))
    {
        for (int i = 0; i < kAdvancedResolutionCount; i++)
        {
            char label[64];
            resolutionLabel(kAdvancedResolutions[i], label, sizeof(label));
            if (ImGui::Selectable(label, resolution == kAdvancedResolutions[i]))
            {
                resolution = kAdvancedResolutions[i];
                printf(CFG_TAG "Advanced option (F1): resolution=%d\n", resolution);
            }
        }
        ImGui::EndCombo();
    }
    menu::helpWhenFocused(text(AdvancedText::ResolutionHelp));
}

// Antialiasing of the 3D models: off or FXAA (enough for these flat polygons)
static const char* antialiasingLabel(int value)
{
    return (value == 1) ? "FXAA" : text(AdvancedText::AntialiasingOff);
}

static void antialiasingCombo(float width, int& value)
{
    if (value < 0 || value > 1)
        value = 0;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text(AdvancedText::Antialiasing));
    helpMarker(text(AdvancedText::AntialiasingHelp));
    comboColumn(width * 0.4f);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##antialiasing", antialiasingLabel(value)))
    {
        for (int i = 0; i <= 1; i++)
        {
            if (ImGui::Selectable(antialiasingLabel(i), value == i))
            {
                value = i;
                printf(CFG_TAG "Advanced option (F1): antialiasing=%d\n", i);
            }
        }
        ImGui::EndCombo();
    }
    menu::helpWhenFocused(text(AdvancedText::AntialiasingHelp));
}

void advancedOptionsDraw()
{
    // Save once when the window closes (F1, Esc or the button)
    static bool s_wasOpen = false;

    // Controller: B goes back (from Controls or Audio to the options, from
    // the options out of F1). Not while the controls window waits for an
    // input (B may be the new one). B acts when released, so the game never
    // sees it held (pause), and not when it only closed a list or a help
    // popup (ImGui does that on press). The window itself opens and closes
    // with the "Options window" controls (input.cpp).
    static menu::BackButton s_back;
    const bool capturing = s_controls.capturing >= 0 || s_controls.waitRelease;
    s_capturing.store(capturing, std::memory_order_release);
    if (s_back.released(advancedOptionsIsOpen() && !capturing))
    {
        if (s_controls.open)
            s_controls.open = false;
        else if (s_audio.open)
            s_audio.open = false;
        else
            advancedOptionsClose();
    }

    if (!advancedOptionsIsOpen())
    {
        if (s_wasOpen)
        {
            s_wasOpen = false;
            // Closed with F1 while the controls window was open: give back
            // ImGui's navigation (drawControlsWindow does it when closed)
            s_controls.open = false;
            s_controls.capturing = -1;
            s_controls.waitRelease = false;
            drawControlsWindow(s_controls, s_controlBindings, textLanguage(), nullptr, false);
            s_audio.open = false;
            saveSettings();
        }
        return;
    }
    // With a controller connected, the gold cursor shows from the start
    if (!s_wasOpen && g_controllerState.connected)
        ImGui::SetNavCursorVisible(true);
    s_wasOpen = true;

    // Scale from the window and the screen's density (Shared/ui/menuBase.h):
    // the windows grow on large screens and shrink on small ones, and scroll
    // when they do not fit. The menu style (the launcher's) is used only here
    // and the game's own ImGui style is put back afterwards.
    ImGuiIO& io = ImGui::GetIO();
    const float density = gWindowBGFX ? SDL_GetWindowDisplayScale(gWindowBGFX) : 1.0f;
    const menu::Metrics metrics = menu::computeMetrics(io.DisplaySize, density, kDesignLandscape, kDesignPortrait);
    const float scale = metrics.scale;
    const float fontScale = scale / menuFontsScale();
    const float width = (std::min)(470.0f * scale, io.DisplaySize.x * 0.94f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, io.DisplaySize.y * 0.94f));

    // The fonts are made large (menuFonts.cpp) and drawn at this scale; the
    // global font scale also reaches the lists and help popups (windows of
    // their own)
    const ImGuiStyle gameStyle = ImGui::GetStyle();
    const float gameFontScale = io.FontGlobalScale;
    menu::applyStyle(metrics, 0.97f);
    io.FontGlobalScale = fontScale;
    ImGui::PushFont(menuFonts().text);

    const bool controlsOpen = s_controls.open || s_audio.open;   // shown instead of the options
    if (!controlsOpen && ImGui::Begin("##advancedOptions", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::PushFont(menuFonts().header);
        ImGui::TextColored(kGoldLight, "%s", text(AdvancedText::Title));
        ImGui::PopFont();
        ImGui::Spacing();

        // FPS
        static const int kFrameLimits[] = { 30, 60, 120, 144, 0 };
        int& frameLimit = g_settings.graphics.frameLimit;
        char current[32];
        if (frameLimit > 0)
            snprintf(current, sizeof(current), "%d", frameLimit);
        else
            snprintf(current, sizeof(current), "%s", text(AdvancedText::FrameLimitNone));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text(AdvancedText::FrameLimit));
        helpMarker(text(AdvancedText::FrameLimitHelp));
        comboColumn(width * 0.4f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##framelimit", current))
        {
            for (int limit : kFrameLimits)
            {
                char label[32];
                if (limit > 0)
                    snprintf(label, sizeof(label), "%d", limit);
                else
                    snprintf(label, sizeof(label), "%s", text(AdvancedText::FrameLimitNone));
                if (ImGui::Selectable(label, frameLimit == limit))
                {
                    frameLimit = limit;
                    printf(CFG_TAG "Advanced option (F1): frameLimit=%d\n", limit);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(text(AdvancedText::FrameLimitHelp));

        optionCheckbox(AdvancedText::VSync, AdvancedText::VSyncHelp, &g_settings.graphics.vsync, "vsync");

        // Aspect ratio (live: the scene rectangle follows it, bgfxGlue.cpp)
        static const AdvancedText kAspects[] = { AdvancedText::Aspect4x3, AdvancedText::Aspect16x10, AdvancedText::AspectStretched };
        int& aspect = g_settings.graphics.aspect;
        if (aspect < 0 || aspect > 2)
            aspect = 0;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text(AdvancedText::AspectRatio));
        helpMarker(text(AdvancedText::AspectRatioHelp));
        comboColumn(width * 0.4f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##aspect", text(kAspects[aspect])))
        {
            for (int i = 0; i < 3; i++)
            {
                // Stretched only in fullscreen
                const ImGuiSelectableFlags flags = (i == 2 && !gIsFullscreen) ? ImGuiSelectableFlags_Disabled : 0;
                if (ImGui::Selectable(text(kAspects[i]), aspect == i, flags))
                {
                    aspect = i;
                    printf(CFG_TAG "Advanced option (F1): aspect=%d\n", i);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(text(AdvancedText::AspectRatioHelp));

        // Resolution of the scene image (live: bgfxGlue.cpp resizes it)
        resolutionCombo(width, g_settings.graphics.resolution);

        // Fullscreen: the window thread switches it (g_pendingFullscreenToggle)
        bool fullscreen = gIsFullscreen;
        if (ImGui::Checkbox(text(AdvancedText::Fullscreen), &fullscreen))
        {
            g_pendingFullscreenToggle = true;
            printf(CFG_TAG "Advanced option (F1): fullscreen=%s\n", fullscreen ? "true" : "false");
        }
        helpMarker(text(AdvancedText::FullscreenHelp));

        // Screen effects and animation smoothing
        ImGui::Spacing();
        ImGui::TextColored(kGold, "%s", text(AdvancedText::Effects));
        ImGui::Separator();
        optionCheckbox(AdvancedText::FilmGrain, AdvancedText::FilmGrainHelp,
            &g_settings.postProcessing.enableFilmGrain, "filmGrain");
        optionCheckbox(AdvancedText::Bloom, AdvancedText::BloomHelp,
            &g_settings.postProcessing.enableBloom, "bloom");
        optionCheckbox(AdvancedText::Ssao, AdvancedText::SsaoHelp,
            &g_settings.postProcessing.enableSSAO, "ssao");
        antialiasingCombo(width, g_settings.postProcessing.antialiasing);
        optionCheckbox(AdvancedText::AnimationSmoothing, AdvancedText::AnimationSmoothingHelp,
            &g_settings.animation.enablePoseSmoothing, "animationSmoothing");
        optionCheckbox(AdvancedText::EnhancedText, AdvancedText::EnhancedTextHelp,
            &g_settings.graphics.enhancedText, "enhancedText");
        optionCheckbox(AdvancedText::CustomBackgrounds, AdvancedText::CustomBackgroundsHelp,
            &g_settings.graphics.customBackgrounds, "customBackgrounds");
        // Export of every camera background (customBackgrounds.cpp, background thread)
        const int exportState = customBackgroundsExportState();
        ImGui::BeginDisabled(exportState == 1);
        if (ImGui::Button(text(AdvancedText::ExportBackgrounds)))
            customBackgroundsExportStart();
        ImGui::EndDisabled();
        helpMarker(text(AdvancedText::ExportBackgroundsHelp));
        if (exportState == 1)
            ImGui::TextColored(kGoldLight, "%s %d", text(AdvancedText::ExportWorking), customBackgroundsExportCount());
        else if (exportState == 2)
            ImGui::TextColored(kGoldLight, "%s (%d)", text(AdvancedText::ExportDone), customBackgroundsExportCount());
        else if (exportState == 3)
            ImGui::TextColored(kGoldLight, "%s", text(AdvancedText::ExportFailed));

        // Every mask of the game (customBackgrounds.cpp, background thread)
        const int maskState = customBackgroundsExportMasksState();
        ImGui::BeginDisabled(maskState == 1);
        if (ImGui::Button(text(AdvancedText::ExportMasks)))
            customBackgroundsExportMasksStart();
        ImGui::EndDisabled();
        helpMarker(text(AdvancedText::ExportMasksHelp));
        if (maskState == 1)
            ImGui::TextColored(kGoldLight, "%s %d", text(AdvancedText::ExportWorking), customBackgroundsExportMasksCount());
        else if (maskState == 2)
            ImGui::TextColored(kGoldLight, "%d %s", customBackgroundsExportMasksCount(), text(AdvancedText::ExportMasksDone));
        else if (maskState == 3)
            ImGui::TextColored(kGoldLight, "%s", text(AdvancedText::ExportFailed));

        ImGui::Spacing();
        ImGui::Spacing();
        // Controls, Audio and Close side by side (finger-sized after a touch)
        const float third = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
        const float buttonH = metrics.pressHeight(34.0f);
        if (ImGui::Button(text(AdvancedText::Controls), ImVec2(third, buttonH)))
            openControlsWindow();
        ImGui::SameLine();
        if (ImGui::Button(text(AdvancedText::Audio), ImVec2(third, buttonH)))
        {
            s_audio.open = true;
            s_audio.justOpened = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(text(AdvancedText::Close), ImVec2(third, buttonH))
            || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            advancedOptionsClose();
        }
    }
    if (!controlsOpen)
        ImGui::End();

    ControlsWindowStyle controlsStyle;
    controlsStyle.scale = scale;
    controlsStyle.fontScale = 1.0f;   // the global font scale already applies
    controlsStyle.titleColor = kGoldLight;
    controlsStyle.titleFont = menuFonts().header;
    SDL_Gamepad* gamepad = g_controllerState.connected ? g_controllerState.gamepad : nullptr;
    if (drawControlsWindow(s_controls, s_controlBindings, textLanguage(), gamepad, false, controlsStyle))
    {
        for (int a = 0; a < kGameActionCount; a++)
        {
            controlsSetBindings((GameAction)a, s_controlBindings[a]);
            printf(CFG_TAG "Controls: %s = %s\n", gameActionId((GameAction)a), bindingsToText(s_controlBindings[a]).c_str());
        }
    }
    drawAudioWindow(s_audio, g_settings.audio.musicVolume, g_settings.audio.soundVolume,
        textLanguage(), false, controlsStyle);

    io.FontGlobalScale = gameFontScale;
    ImGui::PopFont();
    ImGui::GetStyle() = gameStyle;
}
