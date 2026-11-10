///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Launcher.exe: the host of the launcher screen (Shared/ui/launcherScreen.h)
// on Windows. It opens a window that can be resized and maximised, makes the
// fonts at the scale of the screen, gives the screen what to show and acts on
// what the user does: choose the game, its language and the data folder, the
// advanced options and updates, then start Tatou.exe and close.
///////////////////////////////////////////////////////////////////////////////

#include "ui/launcherText.h"
#include "ui/launcherScreen.h"
#include "ui/menuBase.h"
#include "ui/menuFontData.h"
#include "ui/menuGamepad.h"
#include "advancedOptionsText.h"
#include "controlsWindow.h"
#include "audioWindow.h"
#include "launcherSettings.h"
#include "userFolder.h"
#include "folderDialog.h"
#include "gameCover.h"
#include "updater.h"
#include "fileCheck.h"
#ifdef __ANDROID__
#include "androidJava.h"
#endif

#include <SDL3/SDL.h>
#ifndef __ANDROID__
#include <SDL3/SDL_main.h>
#endif
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;
using namespace menu;   // palette and widgets shared with the game (Shared/ui/menuBase.h)

// The window at 100% (Windows scaling) and the smallest it can get: narrow
// enough to show the portrait layout, as on a phone
static const int kWindowWidth = 940;
static const int kWindowHeight = 600;
static const int kMinWindowWidth = 400;
static const int kMinWindowHeight = 520;

// ── Folder dialog result (SDL may call back from another thread) ────────────
static SDL_Mutex* s_dialogMutex = nullptr;
static std::string s_dialogResult;
static bool s_dialogHasResult = false;

static void SDLCALL onFolderChosen(void*, const char* const* filelist, int)
{
    if (!filelist || !filelist[0])
        return;
    SDL_LockMutex(s_dialogMutex);
    s_dialogResult = filelist[0];
    s_dialogHasResult = true;
    SDL_UnlockMutex(s_dialogMutex);
}

// ── Fonts: Georgia from Windows if present, Gelasio otherwise, made
// at the interface scale so the letters stay sharp at any size ─────────────
static ImFont* addFont(const std::string& path, float size)
{
    std::error_code ec;
    if (path.empty() || !fs::exists(fs::u8path(path), ec))
        return nullptr;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size);
}

static Fonts loadFonts(float scale)
{
    ImGui::GetIO().Fonts->Clear();
    Fonts fonts;
    std::string dir;
#ifdef _WIN32
    char windir[MAX_PATH] = {};
    if (GetWindowsDirectoryA(windir, MAX_PATH) > 0)
        dir = std::string(windir) + "\\Fonts\\";
#endif
    fonts.text = addFont(dir.empty() ? "" : dir + "georgia.ttf", 17.0f * scale);
    fonts.button = addFont(dir.empty() ? "" : dir + "georgiab.ttf", 21.0f * scale);
    fonts.header = addFont(dir.empty() ? "" : dir + "georgia.ttf", 30.0f * scale);
    fonts.title = addFont(dir.empty() ? "" : dir + "georgiab.ttf", 40.0f * scale);
    if (!fonts.text)
    {
        // No Georgia (Linux): the project's own font, made to its measures
        logLine("Georgia font not found, using Gelasio");
        fonts.text = addEmbeddedFont(false, 17.0f * scale);
        fonts.button = addEmbeddedFont(true, 21.0f * scale);
        fonts.header = addEmbeddedFont(false, 30.0f * scale);
        fonts.title = addEmbeddedFont(true, 40.0f * scale);
    }
    if (!fonts.button) fonts.button = fonts.text;
    if (!fonts.header) fonts.header = fonts.text;
    if (!fonts.title) fonts.title = fonts.button;
    return fonts;
}

// ── Launcher state ──────────────────────────────────────────────────────────
struct LauncherState
{
    LauncherSettings settings;
    std::vector<GameLanguage> languages;
    bool dataFolderValid = false;
    bool jackFound = false;     // Jack in the Dark is there too (PERE.PAK)
    bool gogInstall = false;    // a GOG installation (never written to)
    bool disc = false;          // the player's CD as an image (never written to)
    bool checking = false;      // its game files are being checked (fileCheck.h)
    Uint64 checkStart = 0;      // since when (ms): the window shows after a moment
    GameFilesResult files;      // what the check found: files in no known edition, files missing
    bool filesAccepted = false; // the player chose not to be warned about those files again
    std::string pendingGame;    // the game to start once the check ends or its warning is accepted
    std::string optionsNotSaved; // aitd_remaster.cfg that could not be written: its warning shows
    std::string message;        // error shown at the bottom
    std::string note;           // or a message that is not an error (the import's progress)
    int importing = -1;         // Android: the game being imported (0-2), -1 none
    int selectedGame = 0;       // 0 AITD1 (with Jack), 1 AITD2, 2 AITD3
    bool quit = false;
    bool launched = false;      // Tatou.exe started (it saved the options already)
};

// The data folder of the selected game
static std::string& dataFolder(LauncherState& state)
{
    if (state.selectedGame == 1)
        return state.settings.aitd2DataFolder;
    if (state.selectedGame == 2)
        return state.settings.aitd3DataFolder;
    return state.settings.aitd1DataFolder;
}

static void refreshDataFolder(LauncherState& state)
{
    const std::string& folder = dataFolder(state);
    state.dataFolderValid = !folder.empty() && dataFolderGame(folder) == state.selectedGame;
    state.jackFound = state.dataFolderValid && hasJackData(folder);
    state.gogInstall = state.dataFolderValid && isGogInstallFolder(folder);
    state.disc = state.dataFolderValid && isGameDiscFolder(folder);
    state.files = GameFilesResult();
    state.filesAccepted = false;
    state.pendingGame.clear();
    state.languages.clear();
    if (!state.dataFolderValid)
    {
        fileCheckStop();
        state.checking = false;
        logLine("Game %d: data folder not valid: '%s'", state.selectedGame + 1, folder.c_str());
        return;
    }
    readGameConfig(folder, state.settings);
    state.languages = findGameLanguages(folder);
    // Its game files against the editions we know, in a thread (followFileCheck)
    fileCheckStart(gameFilesSources(folder), gameFilesCacheFile(state.selectedGame));
    state.checking = true;
    state.checkStart = SDL_GetTicks();

    // Keep the saved language if it exists here, otherwise the first one
    bool found = false;
    for (const GameLanguage& language : state.languages)
        found |= (language.pakName == state.settings.gameLanguage);
    if (!found)
        state.settings.gameLanguage = state.languages.empty() ? "" : state.languages[0].pakName;
    logLine("Game %d: data folder OK: %s (%s, %d languages, Jack in the Dark %s)", state.selectedGame + 1, folder.c_str(),
        state.gogInstall ? "GOG installation" : state.disc ? "the player's CD image" : "copy of the data", (int)state.languages.size(),
        state.jackFound ? "found" : "not found");
}

// True if the game files found need a warning before playing: files missing,
// or files in no known edition the player did not accept
static bool filesNeedWarning(const LauncherState& state)
{
    return !state.files.missing.empty() || (!state.files.unknown.empty() && !state.filesAccepted);
}

// Advanced option texts (shared with the in-game F1 window) in the interface language
static const char* advText(AdvancedText text)
{
    return advancedText(text, (int)uiLanguage());
}

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
    if (ImGui::Checkbox(advText(label), value))
        logLine("Advanced option: %s=%s", logName, *value ? "true" : "false");
    helpMarker(advText(help));
}

// Antialiasing of the 3D models: off or FXAA (enough for these flat polygons)
static const char* antialiasingLabel(int value)
{
    return (value == 1) ? "FXAA" : advText(AdvancedText::AntialiasingOff);
}

static void antialiasingCombo(int& value, float column)
{
    if (value < 0 || value > 1)
        value = 0;
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(color(kText), "%s", advText(AdvancedText::Antialiasing));
    helpMarker(advText(AdvancedText::AntialiasingHelp));
    comboColumn(column);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##antialiasing", antialiasingLabel(value)))
    {
        for (int i = 0; i <= 1; i++)
        {
            if (ImGui::Selectable(antialiasingLabel(i), value == i))
            {
                value = i;
                logLine("Advanced option: antialiasing=%d", i);
            }
        }
        ImGui::EndCombo();
    }
    menu::helpWhenFocused(advText(AdvancedText::AntialiasingHelp));
}

// Under Scanlines, indented: their darkness in 4 steps (grey while they are off)
static void scanlineLevelSlider(bool enabled, int& level, float column)
{
    ImGui::BeginDisabled(!enabled);
    ImGui::Indent();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(color(kText), "%s", advText(AdvancedText::ScanlineLevel));
    helpMarker(advText(AdvancedText::ScanlineLevelHelp));
    comboColumn(column);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderInt("##scanlineLevel", &level, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp))
        logLine("Advanced option: scanlineLevel=%d", level);
    menu::helpWhenFocused(advText(AdvancedText::ScanlineLevelHelp));
    ImGui::Unindent();
    ImGui::EndDisabled();
}

// The first connected controller, for setting controls with it
static SDL_Gamepad* s_gamepad = nullptr;

static void handleGamepadEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_GAMEPAD_ADDED && !s_gamepad)
    {
        s_gamepad = SDL_OpenGamepad(event.gdevice.which);
        if (s_gamepad)
            logLine("Controller connected: %s", SDL_GetGamepadName(s_gamepad));
    }
    else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && s_gamepad &&
             SDL_GetGamepadID(s_gamepad) == event.gdevice.which)
    {
        SDL_CloseGamepad(s_gamepad);
        s_gamepad = nullptr;
        logLine("Controller disconnected");
    }
}

// Controls and audio windows (Shared/), popups over the advanced options
static ControlsWindowState s_controlsWindow;
static AudioWindowState s_audioWindow;
static std::vector<std::string> s_narrators;   // looked for when the audio window opens

// B closes the advanced options (saving them, as Close does)
static bool s_closeAdvanced = false;

// Advanced options window: centred, never wider than the screen and never
// taller (its content scrolls); sizes in interface units (metrics)
static void drawAdvancedOptions(LauncherState& state, const Fonts& fonts, const Metrics& metrics)
{
    ImGuiIO& io = ImGui::GetIO();
    const float width = (std::min)(metrics.px(520.0f), io.DisplaySize.x * 0.94f);
    const float column = metrics.px(200.0f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, io.DisplaySize.y * 0.94f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, metrics.px(24.0f, 20.0f));
    if (!ImGui::BeginPopupModal("##advancedPopup", nullptr, menu::kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PopStyleVar();
        return;
    }

    ImGui::PushFont(fonts.header);
    ImGui::TextColored(color(kGoldLight), "%s", advText(AdvancedText::Title));
    ImGui::PopFont();
    ImGui::Spacing();

    // FPS
    static const int kFrameLimits[] = { 30, 60, 120, 144, 0 };
    char current[32];
    if (state.settings.frameLimit > 0)
        snprintf(current, sizeof(current), "%d", state.settings.frameLimit);
    else
        snprintf(current, sizeof(current), "%s", advText(AdvancedText::FrameLimitNone));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(color(kText), "%s", advText(AdvancedText::FrameLimit));
    helpMarker(advText(AdvancedText::FrameLimitHelp));
    comboColumn(column);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##framelimit", current))
    {
        for (int limit : kFrameLimits)
        {
            char label[32];
            if (limit > 0)
                snprintf(label, sizeof(label), "%d", limit);
            else
                snprintf(label, sizeof(label), "%s", advText(AdvancedText::FrameLimitNone));
            if (ImGui::Selectable(label, state.settings.frameLimit == limit))
            {
                state.settings.frameLimit = limit;
                logLine("Advanced option: frameLimit=%d", limit);
            }
        }
        ImGui::EndCombo();
    }
    menu::helpWhenFocused(advText(AdvancedText::FrameLimitHelp));

    optionCheckbox(AdvancedText::VSync, AdvancedText::VSyncHelp, &state.settings.vsync, "vsync");

    // Aspect ratio
    static const AdvancedText kAspects[] = { AdvancedText::Aspect4x3, AdvancedText::Aspect16x10, AdvancedText::AspectStretched };
    if (state.settings.aspect < 0 || state.settings.aspect > 2)
        state.settings.aspect = 0;
    // Stretched only in fullscreen: a window takes 4:3
    if (!state.settings.fullscreen && state.settings.aspect == 2)
        state.settings.aspect = 0;
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(color(kText), "%s", advText(AdvancedText::AspectRatio));
    helpMarker(advText(AdvancedText::AspectRatioHelp));
    comboColumn(column);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##aspect", advText(kAspects[state.settings.aspect])))
    {
        for (int i = 0; i < 3; i++)
        {
            const ImGuiSelectableFlags flags = (i == 2 && !state.settings.fullscreen) ? ImGuiSelectableFlags_Disabled : 0;
            if (ImGui::Selectable(advText(kAspects[i]), state.settings.aspect == i, flags))
            {
                state.settings.aspect = i;
                logLine("Advanced option: aspect=%d", i);
            }
        }
        ImGui::EndCombo();
    }
    menu::helpWhenFocused(advText(AdvancedText::AspectRatioHelp));

    // Resolution of the scene image (disabled with integer scaling, which does not use it)
    {
        ImGui::BeginDisabled(state.settings.integerScale != 0);
        char current[64];
        advancedResolutionLabel(state.settings.resolution, (int)uiLanguage(), current, sizeof(current));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(color(kText), "%s", advText(AdvancedText::Resolution));
        helpMarker(advText(AdvancedText::ResolutionHelp));
        comboColumn(column);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##resolution", current))
        {
            for (int i = 0; i < kAdvancedResolutionCount; i++)
            {
                const int resolution = kAdvancedResolutions[i];
                char label[64];
                advancedResolutionLabel(resolution, (int)uiLanguage(), label, sizeof(label));
                if (ImGui::Selectable(label, state.settings.resolution == resolution))
                {
                    state.settings.resolution = resolution;
                    logLine("Advanced option: resolution=%d", resolution);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(advText(AdvancedText::ResolutionHelp));
        ImGui::EndDisabled();
    }

    optionCheckbox(AdvancedText::Fullscreen, AdvancedText::FullscreenHelp, &state.settings.fullscreen, "fullscreen");

    // Integer scaling: its size and the 3D resolution that fits it, for the
    // main screen (where the game opens; in a smaller window the engine takes
    // the nearest lower ones that fit)
    ImGui::Spacing();
    ImGui::TextColored(color(kGold), "%s", advText(AdvancedText::IntegerScale));
    ImGui::Separator();
    int screenWidth = 0, screenHeight = 0;
    if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay()))
    {
        screenWidth = (int)(mode->w * mode->pixel_density);
        screenHeight = (int)(mode->h * mode->pixel_density);
    }
    {
        char current[64];
        integerScaleLabel(state.settings.integerScale, state.settings.aspect, (int)uiLanguage(), current, sizeof(current));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(color(kText), "%s", advText(AdvancedText::IntegerSize));
        helpMarker(advText(AdvancedText::IntegerScaleHelp));
        comboColumn(column);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##integerScale", current))
        {
            for (int entry = -1; entry <= kIntegerScaleMax; entry++)
            {
                // First Off (0), then Automatic (-1), then the sizes
                const int choice = (entry == -1) ? 0 : (entry == 0 ? kIntegerScaleAuto : entry);
                if (choice > 0 && !integerScaleOffered(state.settings.aspect, choice, screenWidth, screenHeight))
                    continue;
                char label[64];
                integerScaleLabel(choice, state.settings.aspect, (int)uiLanguage(), label, sizeof(label));
                if (ImGui::Selectable(label, state.settings.integerScale == choice))
                {
                    state.settings.integerScale = choice;
                    logLine("Advanced option: integerScale=%d", choice);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(advText(AdvancedText::IntegerScaleHelp));
    }
    {
        // 3D resolution: the whole size, the ones that divide it exactly and the original
        int scene[2] = { 0, 0 };
        if (!integerScaleResolve(state.settings.integerScale, state.settings.aspect, screenWidth, screenHeight, scene))
            scene[0] = scene[1] = 0;
        int currentImage[2];
        integerResolutionImage(state.settings.integerResolution, scene[0], scene[1], currentImage);
        char current[64];
        integerResolutionLabel(currentImage, (int)uiLanguage(), current, sizeof(current));
        ImGui::BeginDisabled(state.settings.integerScale == 0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(color(kText), "%s", advText(AdvancedText::IntegerResolution));
        helpMarker(advText(AdvancedText::IntegerResolutionHelp));
        comboColumn(column);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##integerResolution", current))
        {
            // First the whole size (0), then from the sharpest down to the original (1)
            for (int entry = kIntegerScaleMax + 1; entry >= 1; entry--)
            {
                const int choice = (entry == kIntegerScaleMax + 1) ? 0 : entry;
                int image[2];
                if (!integerResolutionOffered(choice, scene[0], scene[1], image))
                    continue;
                char label[64];
                integerResolutionLabel(image, (int)uiLanguage(), label, sizeof(label));
                if (ImGui::Selectable(label, image[0] == currentImage[0] && image[1] == currentImage[1]))
                {
                    state.settings.integerResolution = choice;
                    logLine("Advanced option: integerResolution=%d", choice);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(advText(AdvancedText::IntegerResolutionHelp));
        ImGui::EndDisabled();
    }

    // Screen effects and animation smoothing (all off by default)
    ImGui::Spacing();
    ImGui::TextColored(color(kGold), "%s", advText(AdvancedText::Effects));
    ImGui::Separator();
    optionCheckbox(AdvancedText::FilmGrain, AdvancedText::FilmGrainHelp, &state.settings.filmGrain, "filmGrain");
    optionCheckbox(AdvancedText::Scanlines, AdvancedText::ScanlinesHelp, &state.settings.scanlines, "scanlines");
    scanlineLevelSlider(state.settings.scanlines, state.settings.scanlineLevel, column);
    optionCheckbox(AdvancedText::Bloom, AdvancedText::BloomHelp, &state.settings.bloom, "bloom");
    optionCheckbox(AdvancedText::Ssao, AdvancedText::SsaoHelp, &state.settings.ssao, "ssao");
    antialiasingCombo(state.settings.antialiasing, column);
    optionCheckbox(AdvancedText::AnimationSmoothing, AdvancedText::AnimationSmoothingHelp,
        &state.settings.animationSmoothing, "animationSmoothing");
    optionCheckbox(AdvancedText::EnhancedText, AdvancedText::EnhancedTextHelp, &state.settings.enhancedText, "enhancedText");
    optionCheckbox(AdvancedText::CustomBackgrounds, AdvancedText::CustomBackgroundsHelp,
        &state.settings.customBackgrounds, "customBackgrounds");
    ImGui::Spacing();
    ImGui::Spacing();

    // Controls, Audio and Close side by side
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float gap = metrics.px(10.0f);
    const float third = (ImGui::GetContentRegionAvail().x - gap * 2.0f) / 3.0f;
    const ImVec2 buttonSize(third, metrics.pressHeight(40.0f));
    if (darkButton("##controls", advText(AdvancedText::Controls), pos, buttonSize, fonts.button, true))
    {
        if (state.settings.controls.size() != (size_t)kGameActionCount)
            state.settings.restoreDefaultControls();
        s_controlsWindow.open = true;
        s_controlsWindow.justOpened = true;
    }
    if (darkButton("##audio", advText(AdvancedText::Audio), ImVec2(pos.x + third + gap, pos.y), buttonSize, fonts.button, true))
    {
        s_audioWindow.open = true;
        s_audioWindow.justOpened = true;
        s_narrators = findAitd1Narrators(dataFolder(state));
    }
    if (darkButton("##closeAdvanced", advText(AdvancedText::Close), ImVec2(pos.x + (third + gap) * 2.0f, pos.y), buttonSize,
        fonts.button, true) || s_closeAdvanced)
    {
        s_closeAdvanced = false;
        // Saved now, not only when playing (they were lost by leaving with EXIT)
        writeGameConfig(dataFolder(state), state.settings);
        ImGui::CloseCurrentPopup();
    }
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));
    menu::dragScroll();

    // Their sizes follow the scale; the fonts are already made at it
    ControlsWindowStyle controlsStyle;
    controlsStyle.scale = metrics.scale;
    controlsStyle.fontScale = 1.0f;
    controlsStyle.titleColor = color(kGoldLight);
    controlsStyle.titleFont = fonts.header;
    if (drawControlsWindow(s_controlsWindow, state.settings.controls, state.settings.modernStick, state.settings.vibration,
        (int)uiLanguage(), s_gamepad,
        true, controlsStyle))
    {
        for (int a = 0; a < kGameActionCount; a++)
            logLine("Controls: %s = %s", gameActionId((GameAction)a), bindingsToText(state.settings.controls[a]).c_str());
        logLine("Controls: modern left stick %s, vibration %s", state.settings.modernStick ? "on" : "off",
            state.settings.vibration ? "on" : "off");
    }
    if (drawAudioWindow(s_audioWindow, state.settings.musicVolume, state.settings.soundVolume, state.settings.voices,
        s_narrators, (int)uiLanguage(), true, controlsStyle))
        logLine("Audio: music %d, voices and effects %d, narrator %s", state.settings.musicVolume, state.settings.soundVolume,
            state.settings.voices.empty() ? "of the game" : state.settings.voices.c_str());

    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// B closes the updates window (as its last button does)
static bool s_closeUpdate = false;

// Updates window: what the updater is doing, and Install / Not now when a
// newer version was found; after installing, Accept closes the launcher
static void drawUpdateWindow(LauncherState& state, const Fonts& fonts, const Metrics& metrics)
{
    ImGuiIO& io = ImGui::GetIO();
    const float width = (std::min)(metrics.px(480.0f), io.DisplaySize.x * 0.94f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, io.DisplaySize.y * 0.94f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, metrics.px(24.0f, 20.0f));
    if (!ImGui::BeginPopupModal("##updatePopup", nullptr, menu::kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PopStyleVar();
        return;
    }

    ImGui::PushFont(fonts.header);
    ImGui::TextColored(color(kGoldLight), "%s", tr(UiText::UpdateTitle));
    ImGui::PopFont();
    ImGui::Spacing();

    const UpdateState update = updaterState();
    const std::string latest = updaterLatestVersion();
    char text[512] = "";
    switch (update)
    {
    case UpdateState::Idle:
    case UpdateState::Checking: snprintf(text, sizeof(text), "%s", tr(UiText::UpdateChecking)); break;
    case UpdateState::UpToDate: snprintf(text, sizeof(text), tr(UiText::UpdateLatest), launcherVersion()); break;
    case UpdateState::CheckFailed: snprintf(text, sizeof(text), "%s", tr(UiText::UpdateCheckFailed)); break;
    case UpdateState::Available: snprintf(text, sizeof(text), tr(UiText::UpdateAvailable), latest.c_str(), launcherVersion()); break;
    case UpdateState::Installing: snprintf(text, sizeof(text), tr(UiText::UpdateInstalling), latest.c_str()); break;
#ifdef __ANDROID__
    case UpdateState::Installed: snprintf(text, sizeof(text), tr(UiText::UpdateConfirm), latest.c_str()); break;
#else
    case UpdateState::Installed: snprintf(text, sizeof(text), tr(UiText::UpdateDone), latest.c_str()); break;
#endif
    case UpdateState::InstallFailed: snprintf(text, sizeof(text), "%s", tr(UiText::UpdateFailed)); break;
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(update == UpdateState::CheckFailed || update == UpdateState::InstallFailed ? kError : kText), "%s", text);
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    ImGui::Spacing();

    // Install and Not now side by side; otherwise one button (none while working)
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float gap = metrics.px(10.0f);
    const float full = ImGui::GetContentRegionAvail().x;
    const float buttonH = metrics.pressHeight(40.0f);
    const bool closing = s_closeUpdate;
    s_closeUpdate = false;
    if (update == UpdateState::Available)
    {
        const ImVec2 half((full - gap) * 0.5f, buttonH);
        if (goldButton("##updateInstall", tr(UiText::UpdateInstall), pos, half, fonts.button, true))
            updaterStartInstall();
        if (darkButton("##updateLater", tr(UiText::UpdateLater), ImVec2(pos.x + half.x + gap, pos.y), half, fonts.button, true) || closing)
            ImGui::CloseCurrentPopup();
    }
    else if (update == UpdateState::Installed)
    {
        // Android: its installer is the one that closes and updates the app
        if (goldButton("##updateAccept", tr(UiText::UpdateAccept), pos, ImVec2(full, buttonH), fonts.button, true) || closing)
#ifdef __ANDROID__
            ImGui::CloseCurrentPopup();
#else
            state.quit = true;
#endif
    }
    else if (update != UpdateState::Checking && update != UpdateState::Installing)
    {
        if (darkButton("##updateClose", tr(UiText::UpdateClose), pos, ImVec2(full, buttonH), fonts.button, true) || closing)
            ImGui::CloseCurrentPopup();
    }
    else
        ImGui::Dummy(ImVec2(full, buttonH));
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));

    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// B closes the about window
static bool s_closeAbout = false;

// About window: what the project is, its author and credits, thanks to the
// testers and a button that opens the repository in the browser
static void drawAboutWindow(const Fonts& fonts, const Metrics& metrics)
{
    ImGuiIO& io = ImGui::GetIO();
    const float width = (std::min)(metrics.px(480.0f), io.DisplaySize.x * 0.94f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, io.DisplaySize.y * 0.94f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, metrics.px(24.0f, 20.0f));
    if (!ImGui::BeginPopupModal("##aboutPopup", nullptr, menu::kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PopStyleVar();
        return;
    }

    ImGui::PushFont(fonts.header);
    ImGui::TextColored(color(kGoldLight), "Alone in the Dark: Classic Reborn");
    ImGui::PopFont();
    if (updatesEnabled())
        ImGui::TextColored(color(kTextDim), "v%s", launcherVersion());
    ImGui::Spacing();

    char author[96];
    snprintf(author, sizeof(author), tr(UiText::AboutAuthor), "nayo186");
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(kText), "%s", tr(UiText::AboutText));
    ImGui::Spacing();
    ImGui::TextColored(color(kGold), "%s", author);
    ImGui::Spacing();
    ImGui::TextColored(color(kTextDim), "%s", tr(UiText::AboutCredits));
    ImGui::Spacing();
    ImGui::TextColored(color(kText), "%s", tr(UiText::AboutThanks));
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    ImGui::Spacing();

    // Open GitHub and Close side by side
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float gap = metrics.px(10.0f);
    const ImVec2 half((ImGui::GetContentRegionAvail().x - gap) * 0.5f, metrics.pressHeight(40.0f));
    if (goldButton("##aboutGithub", tr(UiText::AboutGithub), pos, half, fonts.button, true))
    {
        if (!SDL_OpenURL("https://github.com/nayo186/AloneInTheDarkClassicReborn"))
            logLine("Could not open GitHub: %s", SDL_GetError());
    }
    const bool closing = s_closeAbout;
    s_closeAbout = false;
    if (darkButton("##aboutClose", tr(UiText::UpdateClose), ImVec2(pos.x + half.x + gap, pos.y), half, fonts.button, true) || closing)
        ImGui::CloseCurrentPopup();
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));

    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// What the screen shows, from the launcher's state
static LauncherView makeView(LauncherState& state, const LauncherCover& cover)
{
    LauncherView view;
    view.selectedGame = state.selectedGame;
    view.dataFolderValid = state.dataFolderValid;
    view.jackFound = state.jackFound;
    view.isDefault = state.settings.isDefault();
    view.debugMode = state.settings.debugMode;
    view.dataFolder = dataFolder(state);
    view.canUpdate = updatesEnabled();
    view.version = updatesEnabled() ? launcherVersion() : "";
    view.updateAvailable = (updaterState() == UpdateState::Available);
    view.cover = &cover;
#ifdef __ANDROID__
    view.importGame = true;
    view.canBrowse = (state.importing < 0);
#endif
    for (int i = 0; i < (int)state.languages.size(); i++)
    {
        view.gameLanguages.push_back(state.languages[i].displayName);
        if (state.languages[i].pakName == state.settings.gameLanguage)
            view.gameLanguage = i;
    }

    // Bottom line: an error, a folder that is not valid, or what it is
    if (!state.message.empty())
    {
        view.message = state.message;
        view.messageColor = kError;
    }
    else if (!state.note.empty())
    {
        view.message = state.note;
        view.messageColor = kTextDim;
    }
    else if (!view.dataFolder.empty() && !state.dataFolderValid)
    {
        view.message = tr(UiText::DataFolderInvalid);
        view.messageColor = kError;
    }
    else if (state.dataFolderValid && state.checking)
    {
        view.message = tr(UiText::FilesChecking);
        view.messageColor = kTextDim;
    }
    else if (state.dataFolderValid && !state.files.missing.empty())
    {
        char text[160];
        snprintf(text, sizeof(text), tr(UiText::FilesMissing), (int)state.files.missing.size());
        view.message = text;
        view.messageColor = kError;
    }
    else if (state.dataFolderValid && !state.files.unknown.empty())
    {
        // Accepted by the player: still said, but not as an error
        view.message = tr(UiText::FilesUnknown);
        view.messageColor = state.filesAccepted ? kTextDim : kError;
    }
    else if (state.dataFolderValid)
    {
        view.message = tr(state.gogInstall ? UiText::InstallGog : state.disc ? UiText::InstallDisc : UiText::InstallCopy);
        view.messageColor = kTextDim;
    }
    return view;
}

#ifdef __ANDROID__
// Android: the progress of an import; a game once copied: its portable
// folder becomes its data folder; a language: the game's languages again
static void followImport(LauncherState& state)
{
    if (state.importing < 0)
        return;
    const ImportProgress progress = androidImportProgress();
    switch (progress.state)
    {
    case ImportState::Copying:
    {
        char text[128];
        const int percent = progress.total > 0 ? (int)(progress.copied * 100 / progress.total) : 0;
        snprintf(text, sizeof(text), tr(UiText::ImportCopying), percent);
        state.note = text;
        break;
    }
    case ImportState::Done:
    {
        static const UiText done[] = { UiText::ImportDone, UiText::ImportDone, UiText::ImportLanguageDone,
                                       UiText::ImportNarratorDone, UiText::ImportPicturesDone };
        logLine("Import: copied %lld bytes (kind %d)", progress.copied, (int)progress.kind);
        // Every game now in its portable folder (the whole UserData brings several)
        if (progress.kind == ImportKind::Game)
        {
            std::string* folders[3] = { &state.settings.aitd1DataFolder, &state.settings.aitd2DataFolder,
                                        &state.settings.aitd3DataFolder };
            for (int game = 0; game < 3; game++)
                if (dataFolderGame(portableGameFolder(game)) == game)
                    *folders[game] = portableGameFolder(game);
            saveLauncherSettings(state.settings);
        }
        state.message.clear();
        state.note = tr(done[(int)progress.kind]);
        state.importing = -1;
        refreshDataFolder(state);
        break;
    }
    case ImportState::Failed:
        logLine("Import: failed after %lld of %lld bytes%s", progress.copied, progress.total,
            progress.kind == ImportKind::None ? " (nothing recognised in the folder)" : "");
        state.note.clear();
        state.message = tr(progress.kind == ImportKind::None ? UiText::ImportNothing : UiText::ImportFailed);
        state.importing = -1;
        break;
    case ImportState::Cancelled:
        logLine("Import: cancelled");
        state.note.clear();
        state.importing = -1;
        break;
    default:
        break;
    }
}
#endif

// Saves the settings (shared by AITD1 and Jack) and starts the game. If they
// cannot be saved (a read-only or protected folder), the game waits for the
// player's answer (drawOptionsWarning) and may start with its old options
static void startGame(LauncherState& state, const char* game, bool withOldOptions = false)
{
    if (!withOldOptions && !writeGameConfig(dataFolder(state), state.settings))
    {
        state.pendingGame = game;
        state.optionsNotSaved = gameConfigFile(dataFolder(state));
        return;
    }
    saveLauncherSettings(state.settings);
    const LaunchResult result = launchGame(state.settings, game);
    if (result == LaunchResult::Started)
    {
        state.launched = true;
        state.quit = true;
    }
    else
        state.message = tr(result == LaunchResult::TatouMissing ? UiText::TatouMissing : UiText::LaunchFailed);
}

// The check of the game files ended: noted in launcher.log; a game the player
// asked for meanwhile starts, or its warning opens
static void followFileCheck(LauncherState& state)
{
    if (!state.checking)
        return;
    const FileCheckState check = fileCheckState();
    if (!check.finished)
        return;
    state.checking = false;
    state.files = check.result;
    state.filesAccepted = check.accepted;
    if (state.files.unknown.empty() && state.files.missing.empty())
        logLine("Game files: a known edition (%d files checked)", check.total);
    if (!state.files.missing.empty())
    {
        logLine("Game files: %d needed files are missing:", (int)state.files.missing.size());
        for (const std::string& name : state.files.missing)
            logLine("  %s", name.c_str());
    }
    if (!state.files.unknown.empty())
    {
        logLine("Game files: %d are in no edition we know (another edition, changed or damaged)%s:", (int)state.files.unknown.size(),
            state.filesAccepted ? "; the player chose not to be warned again" : "");
        for (const std::string& line : state.files.unknown)
            logLine("  %s", line.c_str());
    }
    if (state.pendingGame.empty())
        return;
    if (filesNeedWarning(state))
        ImGui::OpenPopup("##filesPopup");
    else
    {
        const std::string game = state.pendingGame;
        state.pendingGame.clear();
        startGame(state, game.c_str());
    }
}

// A panel in the middle of the window, for the popups below; false if closed
static bool beginCentredPopup(const char* id, const Metrics& metrics)
{
    ImGuiIO& io = ImGui::GetIO();
    const float width = (std::min)(metrics.px(480.0f), io.DisplaySize.x * 0.94f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, io.DisplaySize.y * 0.94f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, metrics.px(24.0f, 20.0f));
    if (ImGui::BeginPopupModal(id, nullptr, menu::kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize))
        return true;
    ImGui::PopStyleVar();
    return false;
}

static void endCentredPopup()
{
    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// While the game files are checked (if it takes more than a moment): what the
// launcher is doing and how far it got, so it is not taken for frozen
static void drawFileCheck(LauncherState& state, const Fonts& fonts, const Metrics& metrics)
{
    if (state.checking && SDL_GetTicks() - state.checkStart > 300 && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        ImGui::OpenPopup("##checkPopup");
    if (!beginCentredPopup("##checkPopup", metrics))
        return;
    if (!state.checking)
        ImGui::CloseCurrentPopup();
    ImGui::PushFont(fonts.header);
    ImGui::TextColored(color(kGoldLight), "%s", tr(UiText::FilesChecking));
    ImGui::PopFont();
    ImGui::Spacing();
    const FileCheckState check = fileCheckState();
    char progress[32];
    snprintf(progress, sizeof(progress), "%d / %d", check.done, check.total);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color(kGold));
    ImGui::ProgressBar(check.total > 0 ? (float)check.done / (float)check.total : 0.0f, ImVec2(-1.0f, metrics.px(22.0f)),
        check.total > 0 ? progress : "");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));
    endCentredPopup();
}

// Before playing with game files missing or in no edition we know: what may
// happen, and Play anyway or Cancel; files that only differ can be accepted
// for good ("Do not warn again", until they change)
static void drawFilesWarning(LauncherState& state, const Fonts& fonts, const Metrics& metrics)
{
    if (!beginCentredPopup("##filesPopup", metrics))
        return;
    const bool missing = !state.files.missing.empty();
    ImGui::PushFont(fonts.header);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(kGoldLight), "%s", tr(missing ? UiText::FilesMissingTitle : UiText::FilesUnknownTitle));
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(kText), "%s", tr(missing ? UiText::FilesMissingText : UiText::FilesUnknownText));
    if (missing)
    {
        // The first few missing, the rest in launcher.log
        std::string names;
        const size_t shown = (std::min)(state.files.missing.size(), (size_t)6);
        for (size_t i = 0; i < shown; i++)
            names += (i ? ", " : "") + state.files.missing[i];
        if (state.files.missing.size() > shown)
            names += ", ...";
        ImGui::Spacing();
        ImGui::TextColored(color(kTextDim), "%s", names.c_str());
    }
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    static bool dontWarn = false;
    if (!missing)
        ImGui::Checkbox(tr(UiText::FilesDontWarn), &dontWarn);
    ImGui::Spacing();

    // Play anyway and Cancel side by side
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float gap = metrics.px(10.0f);
    const ImVec2 half((ImGui::GetContentRegionAvail().x - gap) * 0.5f, metrics.pressHeight(40.0f));
    if (goldButton("##filesPlay", tr(UiText::PlayAnyway), pos, half, fonts.button, true))
    {
        ImGui::CloseCurrentPopup();
        if (!missing && dontWarn)
        {
            fileCheckAccept();
            state.filesAccepted = true;
            logLine("Game files in no known edition: the player chose not to be warned again");
        }
        dontWarn = false;
        logLine("Game files %s: playing anyway", missing ? "missing" : "in no known edition");
        const std::string game = state.pendingGame;
        state.pendingGame.clear();
        startGame(state, game.c_str());
    }
    else if (darkButton("##filesCancel", tr(UiText::Cancel), ImVec2(pos.x + half.x + gap, pos.y), half, fonts.button, true))
    {
        ImGui::CloseCurrentPopup();
        dontWarn = false;
        state.pendingGame.clear();
    }
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));
    endCentredPopup();
}

// The options could not be saved before playing (startGame): which file and
// why it may be, and Play anyway (with the options the game had) or Cancel
static void drawOptionsWarning(LauncherState& state, const Fonts& fonts, const Metrics& metrics)
{
    if (!state.optionsNotSaved.empty() && !ImGui::IsPopupOpen("##optionsPopup"))
        ImGui::OpenPopup("##optionsPopup");
    if (!beginCentredPopup("##optionsPopup", metrics))
        return;
    ImGui::PushFont(fonts.header);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(kGoldLight), "%s", tr(UiText::OptionsNotSavedTitle));
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(color(kText), "%s", tr(UiText::OptionsNotSavedText));
    ImGui::Spacing();
    ImGui::TextColored(color(kTextDim), "%s", state.optionsNotSaved.c_str());
    ImGui::PopTextWrapPos();
    ImGui::Spacing();

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float gap = metrics.px(10.0f);
    const ImVec2 half((ImGui::GetContentRegionAvail().x - gap) * 0.5f, metrics.pressHeight(40.0f));
    if (goldButton("##optionsPlay", tr(UiText::PlayAnyway), pos, half, fonts.button, true))
    {
        ImGui::CloseCurrentPopup();
        logLine("Options not saved: playing anyway with the old ones");
        const std::string game = state.pendingGame;
        state.pendingGame.clear();
        state.optionsNotSaved.clear();
        startGame(state, game.c_str(), true);
    }
    else if (darkButton("##optionsCancel", tr(UiText::Cancel), ImVec2(pos.x + half.x + gap, pos.y), half, fonts.button, true))
    {
        ImGui::CloseCurrentPopup();
        state.pendingGame.clear();
        state.optionsNotSaved.clear();
    }
    ImGui::Dummy(ImVec2(0.0f, metrics.px(4.0f)));
    endCentredPopup();
}

static void handleIntent(LauncherState& state, const LauncherIntent& intent, SDL_Window* window)
{
    if (intent.selectGame >= 0 && intent.selectGame != state.selectedGame)
    {
        // Each game reads its own folder's options: keep the ones of the game left
        if (state.dataFolderValid)
            writeGameConfig(dataFolder(state), state.settings);
        state.selectedGame = intent.selectGame;
        state.message.clear();
        if (state.importing < 0)
            state.note.clear();
        refreshDataFolder(state);
    }
    if (intent.uiLanguage >= 0)
    {
        setUiLanguage((UiLanguage)intent.uiLanguage);
        state.settings.uiLanguage = uiLanguageCode((UiLanguage)intent.uiLanguage);
        saveLauncherSettings(state.settings);
        SDL_SetWindowTitle(window, tr(UiText::WindowTitle));
        logLine("Interface language: %s", state.settings.uiLanguage.c_str());
    }
    if (intent.gameLanguage >= 0 && intent.gameLanguage < (int)state.languages.size())
        state.settings.gameLanguage = state.languages[intent.gameLanguage].pakName;
    if (intent.restoreDefaults)
    {
        state.settings.restoreDefaults();
        logLine("Advanced options restored to the defaults");
    }
    if (intent.openAdvanced)
        ImGui::OpenPopup("##advancedPopup");
    if (intent.update)
    {
        // Looks again unless a newer version is already known
        if (updaterState() != UpdateState::Available)
            updaterStartCheck();
        ImGui::OpenPopup("##updatePopup");
    }
    if (intent.about)
        ImGui::OpenPopup("##aboutPopup");
#ifdef __ANDROID__
    // Android: what the chosen folder holds is copied into UserData (androidJava.h)
    if (intent.browse)
    {
        const std::string userData = launcherFolder() + kUserDataFolder;
        logLine("Import: choosing a folder (game %d) for %s", state.selectedGame + 1, userData.c_str());
        state.importing = state.selectedGame;
        androidImportStart(userData, state.selectedGame);
    }
#else
    if (intent.browse)
    {
        // Explorer's dialog on Windows (a full path can be pasted), SDL's elsewhere
        const std::string folder = dataFolder(state);
        std::string chosen;
        if (chooseFolderNative(window, folder, tr(UiText::DataFolder), chosen))
        {
            const char* list[] = { chosen.c_str(), nullptr };
            onFolderChosen(nullptr, list, 0);
        }
#ifndef _WIN32
        else
            SDL_ShowOpenFolderDialog(onFolderChosen, nullptr, window, folder.empty() ? nullptr : folder.c_str(), false);
#endif
    }
#endif
    // Not while a game is being copied
    if (state.importing >= 0)
        return;
    const char* game = intent.play ? (state.selectedGame == 2 ? "aitd3" : state.selectedGame == 1 ? "aitd2" : "aitd1")
        : intent.playJack ? "jack" : nullptr;
    // While the files are checked the game waits for the result
    // (followFileCheck); files missing or unknown: warned first (drawFilesWarning)
    if (game && state.checking)
        state.pendingGame = game;
    else if (game && filesNeedWarning(state))
    {
        state.pendingGame = game;
        ImGui::OpenPopup("##filesPopup");
    }
    else if (game)
        startGame(state, game);
    if (intent.exit)
        state.quit = true;
}

// First size of the window: the saved one, or the design at the screen's
// scaling; never larger than the screen
static void initialWindowSize(const LauncherSettings& settings, int& width, int& height)
{
    SDL_Rect usable = { 0, 0, 1920, 1040 };
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    SDL_GetDisplayUsableBounds(display, &usable);
    float scale = SDL_GetDisplayContentScale(display);
    if (scale <= 0.0f)
        scale = 1.0f;
    width = settings.windowWidth > 0 ? settings.windowWidth : (int)(kWindowWidth * scale);
    height = settings.windowHeight > 0 ? settings.windowHeight : (int)(kWindowHeight * scale);
    width = std::clamp(width, kMinWindowWidth, (std::max)(kMinWindowWidth, usable.w * 95 / 100));
    height = std::clamp(height, kMinWindowHeight, (std::max)(kMinWindowHeight, usable.h * 95 / 100));
}

#ifdef __ANDROID__
// Android: the app opens the launcher's screen first; when a game is chosen
// it returns and the game starts in the same program (Fitd/fitd.cpp)
int launcherMain()
#else
int main(int, char**)
#endif
{
    logOpen();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        logLine("ERROR: SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    LauncherState state;
    loadLauncherSettings(state.settings);
    setUiLanguage(uiLanguageFromCode(state.settings.uiLanguage.c_str()));
    setLauncherDebugMode(state.settings.debugMode);
    // The portable folders exist from the first time the launcher opens
    prepareUserData();
    // What the last update left, then a look for a newer version
    logLine("Version %s", launcherVersion());
    updaterCleanup();
    updaterStartCheck();
    bool offerUpdate = updatesEnabled();   // the window opens by itself once, if there is one

    // Each game's portable copy (UserData\<game>\GAME), then, the first
    // time, the launcher's own folder if it holds the game data
    findPortableGames(state.settings);
    if (state.settings.aitd1DataFolder.empty() && dataFolderGame(launcherFolder()) == 0)
        state.settings.aitd1DataFolder = launcherFolder();
    refreshDataFolder(state);

    int windowWidth = kWindowWidth, windowHeight = kWindowHeight;
    initialWindowSize(state.settings, windowWidth, windowHeight);
#ifdef __ANDROID__
    // Android: the whole screen, upright or on its side as the phone is held
    // (a resizable window without orientation hint: SDLActivity.setOrientationBis);
    // the game turns to landscape (platform/osystemSDL.cpp)
    SDL_Window* window = SDL_CreateWindow(tr(UiText::WindowTitle), windowWidth, windowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);
#else
    SDL_Window* window = SDL_CreateWindow(tr(UiText::WindowTitle), windowWidth, windowHeight,
        SDL_WINDOW_RESIZABLE | (state.settings.windowMaximized ? SDL_WINDOW_MAXIMIZED : 0));
#endif
#ifdef __ANDROID__
    // Android: SDL's own OpenGL ES (its Vulkan renderer failed on the window); the game uses bgfx
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, "opengles2") : nullptr;
#else
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
#endif
    if (!window || !renderer)
    {
        logLine("ERROR: could not create the window: %s", SDL_GetError());
        fileCheckStop();
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(window, kMinWindowWidth, kMinWindowHeight);
    SDL_SetRenderVSync(renderer, 1);
    s_dialogMutex = SDL_CreateMutex();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    // Keyboard and controller can move through the menus too
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    // The controller reaches ImGui through menu::feedGamepad (left stick
    // moves, right stick scrolls), not through the backend
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);

    // One cover per game of the trilogy, compiled into the launcher
    GameCover covers[3];
    LauncherCover screenCovers[3];
    for (int i = 0; i < 3; i++)
    {
        covers[i] = loadGameCover(renderer, i);
        screenCovers[i].colour = (ImTextureID)(intptr_t)covers[i].colour;
        screenCovers[i].grey = (ImTextureID)(intptr_t)covers[i].grey;
        screenCovers[i].width = covers[i].width;
        screenCovers[i].height = covers[i].height;
    }

    Fonts fonts;
    float fontScale = 0.0f;
    bool lastTouch = false;
    while (!state.quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            handleGamepadEvent(event);
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                state.quit = true;
        }

#ifdef __ANDROID__
        followImport(state);
#endif

        // Folder chosen in the dialog
        SDL_LockMutex(s_dialogMutex);
        if (s_dialogHasResult)
        {
            dataFolder(state) = s_dialogResult;
            s_dialogHasResult = false;
            state.message.clear();
            refreshDataFolder(state);
            saveLauncherSettings(state.settings);
        }
        SDL_UnlockMutex(s_dialogMutex);

        // Scale and layout from the window's size and the screen's density;
        // fonts and style are made again when they change
        int pixelsW = 0, pixelsH = 0;
        SDL_GetWindowSize(window, &pixelsW, &pixelsH);
        const Metrics metrics = computeMetrics(ImVec2((float)pixelsW, (float)pixelsH), SDL_GetWindowDisplayScale(window),
            launcherScreen::kDesignLandscape, launcherScreen::kDesignPortrait);
        if (fontsNeedRebuild(fontScale, metrics.scale))
        {
            fontScale = metrics.scale;
            fonts = loadFonts(fontScale);
            ImGui_ImplSDLRenderer3_DestroyFontsTexture();
            ImGui_ImplSDLRenderer3_CreateFontsTexture();
            applyStyle(metrics);
            lastTouch = metrics.touch;
        }
        else if (metrics.touch != lastTouch)
        {
            applyStyle(metrics);   // bigger grips for fingers
            lastTouch = metrics.touch;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        feedGamepad(s_gamepad);
        ImGui::NewFrame();

        // B goes back: from Controls or Audio to the advanced options, from
        // them to the launcher screen (not while a new input is awaited)
        static BackButton back;
        const bool capturing = s_controlsWindow.capturing >= 0 || s_controlsWindow.waitRelease;
        if (back.released((ImGui::IsPopupOpen("##advancedPopup") && !capturing) || ImGui::IsPopupOpen("##updatePopup")
            || ImGui::IsPopupOpen("##aboutPopup")))
        {
            if (ImGui::IsPopupOpen("##updatePopup"))
                s_closeUpdate = true;
            else if (ImGui::IsPopupOpen("##aboutPopup"))
                s_closeAbout = true;
            else if (s_controlsWindow.open)
                s_controlsWindow.requestClose = true;
            else if (s_audioWindow.open)
                s_audioWindow.requestClose = true;
            else
                s_closeAdvanced = true;
        }
        const LauncherIntent intent = drawLauncherScreen(makeView(state, screenCovers[state.selectedGame]), metrics, fonts);
        handleIntent(state, intent, window);
        followFileCheck(state);
        // The check at start found a newer version: asked once
        if (offerUpdate && updaterState() != UpdateState::Checking)
        {
            if (updaterState() == UpdateState::Available && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
                ImGui::OpenPopup("##updatePopup");
            offerUpdate = false;
        }
        ImGui::PushFont(fonts.text);
        drawAdvancedOptions(state, fonts, metrics);
        drawUpdateWindow(state, fonts, metrics);
        drawAboutWindow(fonts, metrics);
        drawFilesWarning(state, fonts, metrics);
        drawOptionsWarning(state, fonts, metrics);
        drawFileCheck(state, fonts, metrics);
        ImGui::PopFont();
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 27, 26, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // The window as it was left, for next time
    state.settings.windowMaximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
    if (!state.settings.windowMaximized)
        SDL_GetWindowSize(window, &state.settings.windowWidth, &state.settings.windowHeight);
    saveLauncherSettings(state.settings);

    // Leaving without playing (EXIT or the window): keep the options chosen
    if (!state.launched && state.dataFolderValid)
        writeGameConfig(dataFolder(state), state.settings);

    fileCheckStop();
    updaterShutdown();
    logLine("Launcher closed");
    for (GameCover& cover : covers)
        freeGameCover(cover);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyMutex(s_dialogMutex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
#ifdef __ANDROID__
    return state.launched ? 0 : 1;   // SDL stays on for the game
#else
    SDL_Quit();
    return 0;
#endif
}
