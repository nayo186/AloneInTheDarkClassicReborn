///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Launcher.exe: the host of the launcher screen (Shared/ui/launcherScreen.h)
// on Windows. It opens a window that can be resized and maximised, makes the
// fonts at the scale of the screen, gives the screen what to show and acts on
// what the user does: choose the game, its language and the data folder, the
// advanced options, then start Tatou.exe and close.
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
#include "folderDialog.h"
#include "gameCover.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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
    std::string message;        // error shown at the bottom
    int selectedGame = 0;       // 0 = Alone in the Dark (AITD2/AITD3 later)
    bool quit = false;
    bool launched = false;      // Tatou.exe started (it saved the options already)
};

static void refreshDataFolder(LauncherState& state)
{
    state.dataFolderValid = isAitd1DataFolder(state.settings.aitd1DataFolder);
    state.jackFound = state.dataFolderValid && hasJackData(state.settings.aitd1DataFolder);
    state.gogInstall = state.dataFolderValid && isGogInstallFolder(state.settings.aitd1DataFolder);
    state.languages.clear();
    if (!state.dataFolderValid)
    {
        logLine("Data folder not valid: '%s'", state.settings.aitd1DataFolder.c_str());
        return;
    }
    readGameConfig(state.settings.aitd1DataFolder, state.settings);
    state.languages = findGameLanguages(state.settings.aitd1DataFolder);

    // Keep the saved language if it exists here, otherwise the first one
    bool found = false;
    for (const GameLanguage& language : state.languages)
        found |= (language.pakName == state.settings.gameLanguage);
    if (!found)
        state.settings.gameLanguage = state.languages.empty() ? "" : state.languages[0].pakName;
    logLine("Data folder OK: %s (%s, %d languages, Jack in the Dark %s)", state.settings.aitd1DataFolder.c_str(),
        state.gogInstall ? "GOG installation" : "copy of the data", (int)state.languages.size(),
        state.jackFound ? "found" : "not found");
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

    // Resolution of the scene image
    {
        char current[64];
        if (state.settings.resolution > 0)
            snprintf(current, sizeof(current), "%d × %d", state.settings.resolution, advancedResolutionHeight(state.settings.resolution));
        else
            snprintf(current, sizeof(current), "%s", advText(AdvancedText::ResolutionAuto));
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
                if (resolution > 0)
                    snprintf(label, sizeof(label), "%d × %d", resolution, advancedResolutionHeight(resolution));
                else
                    snprintf(label, sizeof(label), "%s", advText(AdvancedText::ResolutionAuto));
                if (ImGui::Selectable(label, state.settings.resolution == resolution))
                {
                    state.settings.resolution = resolution;
                    logLine("Advanced option: resolution=%d", resolution);
                }
            }
            ImGui::EndCombo();
        }
        menu::helpWhenFocused(advText(AdvancedText::ResolutionHelp));
    }

    optionCheckbox(AdvancedText::Fullscreen, AdvancedText::FullscreenHelp, &state.settings.fullscreen, "fullscreen");

    // Screen effects and animation smoothing (all off by default)
    ImGui::Spacing();
    ImGui::TextColored(color(kGold), "%s", advText(AdvancedText::Effects));
    ImGui::Separator();
    optionCheckbox(AdvancedText::FilmGrain, AdvancedText::FilmGrainHelp, &state.settings.filmGrain, "filmGrain");
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
        s_narrators = findAitd1Narrators(state.settings.aitd1DataFolder);
    }
    if (darkButton("##closeAdvanced", advText(AdvancedText::Close), ImVec2(pos.x + (third + gap) * 2.0f, pos.y), buttonSize,
        fonts.button, true) || s_closeAdvanced)
    {
        s_closeAdvanced = false;
        // Saved now, not only when playing (they were lost by leaving with EXIT)
        writeGameConfig(state.settings.aitd1DataFolder, state.settings);
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
    if (drawControlsWindow(s_controlsWindow, state.settings.controls, state.settings.modernStick, (int)uiLanguage(), s_gamepad,
        true, controlsStyle))
    {
        for (int a = 0; a < kGameActionCount; a++)
            logLine("Controls: %s = %s", gameActionId((GameAction)a), bindingsToText(state.settings.controls[a]).c_str());
        logLine("Controls: modern left stick %s", state.settings.modernStick ? "on" : "off");
    }
    if (drawAudioWindow(s_audioWindow, state.settings.musicVolume, state.settings.soundVolume, state.settings.voices,
        s_narrators, (int)uiLanguage(), true, controlsStyle))
        logLine("Audio: music %d, voices and effects %d, narrator %s", state.settings.musicVolume, state.settings.soundVolume,
            state.settings.voices.empty() ? "of the game" : state.settings.voices.c_str());

    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

// What the screen shows, from the launcher's state
static LauncherView makeView(const LauncherState& state, const LauncherCover& cover)
{
    LauncherView view;
    view.selectedGame = state.selectedGame;
    view.dataFolderValid = state.dataFolderValid;
    view.jackFound = state.jackFound;
    view.isDefault = state.settings.isDefault();
    view.debugMode = state.settings.debugMode;
    view.dataFolder = state.settings.aitd1DataFolder;
    view.cover = &cover;
    for (int i = 0; i < (int)state.languages.size(); i++)
    {
        view.gameLanguages.push_back(state.languages[i].displayName);
        if (state.languages[i].pakName == state.settings.gameLanguage)
            view.gameLanguage = i;
    }

    // Bottom line: an error, a folder that is not valid, or what it is
    const bool isAitd1 = (state.selectedGame == 0);
    if (!state.message.empty())
    {
        view.message = state.message;
        view.messageColor = kError;
    }
    else if (isAitd1 && !state.settings.aitd1DataFolder.empty() && !state.dataFolderValid)
    {
        view.message = tr(UiText::DataFolderInvalid);
        view.messageColor = kError;
    }
    else if (isAitd1 && state.dataFolderValid)
    {
        view.message = tr(state.gogInstall ? UiText::InstallGog : UiText::InstallCopy);
        view.messageColor = kTextDim;
    }
    return view;
}

// Saves the settings (shared by AITD1 and Jack) and starts the game
static void startGame(LauncherState& state, const char* game)
{
    if (!writeGameConfig(state.settings.aitd1DataFolder, state.settings))
    {
        state.message = tr(UiText::LaunchFailed);
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

static void handleIntent(LauncherState& state, const LauncherIntent& intent, SDL_Window* window)
{
    if (intent.selectGame >= 0)
        state.selectedGame = intent.selectGame;
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
    if (intent.browse)
    {
        // Explorer's dialog on Windows (a full path can be pasted), SDL's elsewhere
        const std::string& folder = state.settings.aitd1DataFolder;
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
    if (intent.play)
        startGame(state, "aitd1");
    else if (intent.playJack)
        startGame(state, "jack");
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

int main(int, char**)
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

    // First run: use the launcher's own folder if it holds the game data
    if (state.settings.aitd1DataFolder.empty() && isAitd1DataFolder(launcherFolder()))
        state.settings.aitd1DataFolder = launcherFolder();
    refreshDataFolder(state);

    int windowWidth = kWindowWidth, windowHeight = kWindowHeight;
    initialWindowSize(state.settings, windowWidth, windowHeight);
    SDL_Window* window = SDL_CreateWindow(tr(UiText::WindowTitle), windowWidth, windowHeight,
        SDL_WINDOW_RESIZABLE | (state.settings.windowMaximized ? SDL_WINDOW_MAXIMIZED : 0));
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!window || !renderer)
    {
        logLine("ERROR: could not create the window: %s", SDL_GetError());
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

        // Folder chosen in the dialog
        SDL_LockMutex(s_dialogMutex);
        if (s_dialogHasResult)
        {
            state.settings.aitd1DataFolder = s_dialogResult;
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
        if (back.released(ImGui::IsPopupOpen("##advancedPopup") && !capturing))
        {
            if (s_controlsWindow.open)
                s_controlsWindow.requestClose = true;
            else if (s_audioWindow.open)
                s_audioWindow.requestClose = true;
            else
                s_closeAdvanced = true;
        }
        const LauncherIntent intent = drawLauncherScreen(makeView(state, screenCovers[state.selectedGame]), metrics, fonts);
        handleIntent(state, intent, window);
        ImGui::PushFont(fonts.text);
        drawAdvancedOptions(state, fonts, metrics);
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
        writeGameConfig(state.settings.aitd1DataFolder, state.settings);

    logLine("Launcher closed");
    for (GameCover& cover : covers)
        freeGameCover(cover);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyMutex(s_dialogMutex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
