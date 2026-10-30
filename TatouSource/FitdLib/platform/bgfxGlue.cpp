///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// BGFX graphics library initialization and window management
///////////////////////////////////////////////////////////////////////////////
//Updated
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/platform.h>
#include <backends/imgui_impl_sdl3.h>
#include <vector>
#include <chrono>
#include "imguiBGFX.h"
#include "platformHooks.h"
#include "ui/menuGamepad.h"
#include "SDL3/SDL.h"
#include "common.h"
#include "input.h"
#include "settings.h"
#include "consoleLog.h"
#include "screenEffects.h"
#include "osystem.h"

#ifndef LANTERN_VERBOSE_LOGS
#define LANTERN_VERBOSE_LOGS 0
#endif

#if BX_PLATFORM_OSX
extern "C" {
	void* cbSetupMetalLayer(void*);
}
#elif BX_PLATFORM_WINDOWS
#include <windows.h>
#endif


SDL_Window* gWindowBGFX = nullptr;

static int s_windowedRect[4] = { 0, 0, 1410, 890 };

bool gCloseApp = false;

float gVolume = 1.f;

bool gIsFullscreen = false;
bool g_pendingFullscreenToggle = false;

// For dust particle system - we need the current floor
extern s16 g_currentFloor;

// Forward declaration — defined below at file scope
extern int outputResolution[2];
extern int windowResolution[2];

void toggleFullscreen()
{
    if (gWindowBGFX == nullptr)
        return;

    gIsFullscreen = !gIsFullscreen;
    // Saved at once, however it was switched (F1, F11, Alt+Enter): the next
    // start opens the same way
    if (g_settings.graphics.fullscreen != gIsFullscreen)
    {
        g_settings.graphics.fullscreen = gIsFullscreen;
        saveSettings();
    }

#if BX_PLATFORM_LINUX
    // On Linux (X11/Wayland) exclusive fullscreen via SDL_SetWindowFullscreen
    // does not trigger bgfx::reset() reliably. Use a borderless window
    // positioned at (0,0) at the display's native resolution instead, which
    // goes through the same resize path as a normal window resize (known working).
    if (gIsFullscreen)
    {
        SDL_GetWindowPosition(gWindowBGFX, &s_windowedRect[0], &s_windowedRect[1]);
        SDL_GetWindowSize(gWindowBGFX, &s_windowedRect[2], &s_windowedRect[3]);

        SDL_SetWindowBordered(gWindowBGFX, false);
        SDL_SetWindowAlwaysOnTop(gWindowBGFX, true);

        SDL_DisplayID display = SDL_GetDisplayForWindow(gWindowBGFX);
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
        if (mode)
        {
            SDL_Rect bounds;
            SDL_GetDisplayBounds(display, &bounds);
            SDL_SetWindowPosition(gWindowBGFX, bounds.x, bounds.y);
            SDL_SetWindowSize(gWindowBGFX, mode->w, mode->h);
        }
    }
    else
    {
        SDL_SetWindowBordered(gWindowBGFX, true);
        SDL_SetWindowAlwaysOnTop(gWindowBGFX, false);
        SDL_SetWindowSize(gWindowBGFX, s_windowedRect[2], s_windowedRect[3]);
        SDL_SetWindowPosition(gWindowBGFX, s_windowedRect[0], s_windowedRect[1]);
    }

    SDL_PumpEvents();

    // Force StartFrame() to detect a resolution change and call bgfx::reset()
    windowResolution[0] = -1;
    windowResolution[1] = -1;
#else
    SDL_SetWindowFullscreen(gWindowBGFX, gIsFullscreen);
#endif
}

// Because SDL is not well defined when using cmake (it's using the old style config.h and is somewhat broken)
extern "C" {
    size_t
        wcslcpy(SDL_OUT_Z_CAP(maxlen) wchar_t* dst, const wchar_t* src, size_t maxlen)
    {
        size_t srclen = SDL_wcslen(src);
        if (maxlen > 0) {
            size_t len = SDL_min(srclen, maxlen - 1);
            SDL_memcpy(dst, src, len * sizeof(wchar_t));
            dst[len] = '\0';
        }
        return srclen;
    }

    size_t
        wcslcat(SDL_INOUT_Z_CAP(maxlen) wchar_t* dst, const wchar_t* src, size_t maxlen)
    {
        size_t dstlen = SDL_wcslen(dst);
        size_t srclen = SDL_wcslen(src);
        if (dstlen < maxlen) {
            SDL_wcslcpy(dst + dstlen, src, maxlen - dstlen);
        }
        return dstlen + srclen;
    }
}

// Size of the scene image the game draws into: the chosen resolution
// (graphics.resolution, always 4:3) or, automatic, 4:3 at the height of the
// scene rectangle. It is then stretched to that rectangle, which has the
// chosen aspect ratio (graphics.aspect): in fullscreen it is centred with
// black bars; a window takes the shape of the aspect ratio and is filled.
int outputResolution[2] = { -1, -1 };
// Size of the window (backbuffer) and position of the scene in it
int windowResolution[2] = { -1, -1 };
static int s_sceneRect[4] = { 0, 0, 1, 1 };  // x, y, width, height

// Width / height of the chosen aspect ratio: 4:3 (the original, made for 1992
// monitors), 16:10 (as GOG shows it) or 0 = stretched to the whole screen
static float aspectRatioValue()
{
    switch (g_settings.graphics.aspect)
    {
    case 0: return 4.0f / 3.0f;
    case 1: return 16.0f / 10.0f;
    default: return 0.0f;
    }
}

// Scene rectangle: in fullscreen centred with the aspect ratio (black bars);
// in a window the whole window (its shape follows the aspect ratio,
// applyWindowAspectRatio)
static void computeSceneRect(int windowWidth, int windowHeight, int rect[4])
{
    const float ratio = gIsFullscreen ? aspectRatioValue() : 0.0f;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = windowWidth;
    rect[3] = windowHeight;
    if (ratio <= 0.0f || windowWidth <= 0 || windowHeight <= 0)
        return;
    if ((float)windowWidth / (float)windowHeight > ratio)
    {
        rect[2] = (int)((float)windowHeight * ratio + 0.5f);
        rect[0] = (windowWidth - rect[2]) / 2;
    }
    else
    {
        rect[3] = (int)((float)windowWidth / ratio + 0.5f);
        rect[1] = (windowHeight - rect[3]) / 2;
    }
}

static void computeSceneImageSize(const int rect[4], int size[2])
{
    const int width = g_settings.graphics.resolution;
    if (width > 0)
    {
        size[0] = width;
        size[1] = width * 3 / 4;
    }
    else
    {
        size[0] = rect[3] * 4 / 3;
        size[1] = rect[3];
    }
}

// Window mode (window thread): the window has the shape of the aspect ratio,
// so there are no black bars and nothing is deformed; it cannot be resized.
// Stretched only exists in fullscreen: in a window it becomes 4:3. Called
// every frame, acts on changes.
void applyWindowAspectRatio()
{
    if (gWindowBGFX == nullptr)
        return;
    if (!gIsFullscreen && g_settings.graphics.aspect == 2)
    {
        g_settings.graphics.aspect = 0;
        printf(BGFX_TAG "Window mode: aspect ratio Stretched becomes 4:3\n");
    }
    static int s_appliedAspect = -1;
    static bool s_appliedFullscreen = false;
    if (s_appliedAspect == g_settings.graphics.aspect && s_appliedFullscreen == gIsFullscreen)
        return;
    s_appliedAspect = g_settings.graphics.aspect;
    s_appliedFullscreen = gIsFullscreen;

    const float ratio = aspectRatioValue();
    if (gIsFullscreen || ratio <= 0.0f)
        return;
    int width, height;
    SDL_GetWindowSize(gWindowBGFX, &width, &height);
    SDL_SetWindowSize(gWindowBGFX, (int)((float)height * ratio + 0.5f), height);
}

void osystem_getSceneRect(int rect[4])
{
    for (int i = 0; i < 4; i++)
        rect[i] = s_sceneRect[i];
}

bool osystem_windowToGame(float windowX, float windowY, float* gameX, float* gameY)
{
    if (s_sceneRect[2] <= 0 || s_sceneRect[3] <= 0)
        return false;
    *gameX = (windowX - s_sceneRect[0]) * 320.0f / s_sceneRect[2];
    *gameY = (windowY - s_sceneRect[1]) * 200.0f / s_sceneRect[3];
    return true;
}

// VSync is an option of the launcher (graphics.vsync)
static uint32_t getVsyncResetFlag()
{
    return g_settings.graphics.vsync ? BGFX_RESET_VSYNC : 0;
}

void StartFrame()
{
    int windowSize[2];
    SDL_GetWindowSizeInPixels(gWindowBGFX, &windowSize[0], &windowSize[1]);

    // VSync can change live (F1 window): it needs a reset like a new size
    static bool s_vsyncApplied = g_settings.graphics.vsync;
    const bool vsyncChanged = (s_vsyncApplied != g_settings.graphics.vsync);
    s_vsyncApplied = g_settings.graphics.vsync;

    // Guard against zero dimensions (e.g. during minimize or window transitions)
    if (windowSize[0] > 0 && windowSize[1] > 0)
    {
        if (windowSize[0] != windowResolution[0] || windowSize[1] != windowResolution[1] || vsyncChanged)
        {
            windowResolution[0] = windowSize[0];
            windowResolution[1] = windowSize[1];
            bgfx::reset(windowSize[0], windowSize[1], getVsyncResetFlag());
        }

        // The scene rectangle follows the window and the aspect option, the
        // scene image the resolution option (both live in F1)
        computeSceneRect(windowResolution[0], windowResolution[1], s_sceneRect);
        int sceneSize[2];
        computeSceneImageSize(s_sceneRect, sceneSize);
        if (sceneSize[0] != outputResolution[0] || sceneSize[1] != outputResolution[1])
        {
            outputResolution[0] = sceneSize[0];
            outputResolution[1] = sceneSize[1];
            if (g_screenEffects)
                g_screenEffects->resize(outputResolution[0], outputResolution[1]);
        }
        if (g_screenEffects)
            g_screenEffects->setOutputRect(s_sceneRect[0], s_sceneRect[1], s_sceneRect[2], s_sceneRect[3],
                windowResolution[0], windowResolution[1]);
    }

    // Custom backgrounds: a picture saved while playing is read again at once
    if (g_onFrameStart)
        g_onFrameStart();
    if (g_screenEffects)
    {
        // Sides of a picture wider than 4:3 fill the black bars of fullscreen
        BackgroundPicture picture;
        const bool sides = gIsFullscreen && g_getBackgroundPicture && g_getBackgroundPicture(&picture) && picture.u0 > 0.0f;
        g_screenEffects->setSideFill(sides ? picture.picture : bgfx::TextureHandle(BGFX_INVALID_HANDLE),
                                     sides ? picture.u0 : 0.0f, sides ? picture.u1 : 1.0f);
    }

    // While the controller is in use the cursor stays hidden (input.cpp);
    // ImGui would otherwise show it again every frame
    ImGuiIO& frameIo = ImGui::GetIO();
    if (controllerInUse())
        frameIo.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    else
        frameIo.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

    // Pull the input from SDL2 instead
    ImGui_ImplSDL3_NewFrame();
    // The controller in the menus (Shared/ui/menuGamepad.h: left stick moves,
    // right stick scrolls)
    menu::feedGamepad(g_controllerState.connected ? g_controllerState.gamepad : nullptr);
    imguiBeginFrame(0, 0, 0, 0, windowResolution[0], windowResolution[1], -1);

    // Menu bar is hidden for cleaner look - uncomment to show debug info
    /*
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::Text(" %.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

        ImGui::PushItemWidth(100);
        ImGui::SliderFloat("Volume", &gVolume, 0, 1);
        ImGui::PopItemWidth();

        ImGui::EndMainMenuBar();
    }
    */

    // The scene is always drawn offscreen: the screen effects, the pause menu
    // preview and the repainted frames (V14) read it
    if (g_screenEffects)
        g_screenEffects->beginScene();

    bgfx::setViewRect(0, 0, 0, outputResolution[0], outputResolution[1]);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
    bgfx::touch(0);
}

void osystem_suspendScreenEffects(bool suspend)
{
    if (g_screenEffects)
        g_screenEffects->suspend(suspend);
}

bool osystem_canFadeScreen()
{
    return g_screenEffects != nullptr;
}

void osystem_setScreenFade(float level)
{
    if (g_screenEffects)
        g_screenEffects->setScreenFade(level);
}

void osystem_setSmokeArea(float centerX, float centerY, float radiusX, float radiusY)
{
    // From the 320x200 screen to the scene image's texture coordinates
    if (g_screenEffects)
        g_screenEffects->setSmokeArea(centerX / 320.0f, centerY / 200.0f, radiusX / 320.0f, radiusY / 200.0f);
}

void EndFrame()
{
#ifdef FITD_DEBUGGER
    // Debug menu toggle (` key) - only in debug builds
    if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false))
    {
        debuggerVar_debugMenuDisplayed = !debuggerVar_debugMenuDisplayed;
    }
#endif


    // The menus (F1 and the debug windows) are drawn last so they stay above
    // the game and its overlays at every resolution
    if (g_onDrawMenus)
        g_onDrawMenus();

    // Always call imguiEndFrame() to render ImGui windows to bgfx
    imguiEndFrame();

    // Draw the offscreen scene on the screen with the enabled effects.
    // In PlayWorld, a frame without 3D (first pass, camera change) would show
    // the background alone, so it repaints the last complete image instead.
    // Glass after the whole scene (it reads what is behind it, BUGS.md V20)
    osystem_drawDeferredGlass();

    // The ambient occlusion is computed from the models of this frame; over a
    // frozen image (menus, F1 window) the one of that image is reused
    if (g_screenEffects && (!g_playWorldActive || g_menuActive || g_3dFlushedThisFrame))
    {
        g_screenEffects->endScene(g_3dFlushedThisFrame, osystem_takeFrozenSceneDrawn());
    }
    else if (g_screenEffects)
    {
        // Skipping the composite still presents a frame, so the screen showed
        // the older swap-chain images in turn: whatever moved last (e.g. the
        // intro car when it parks) flickered between two poses (BUGS.md V14).
        // Repaint the last complete image instead, as DOS kept its one screen.
        osystem_drawFrozenSceneBackground();
        g_screenEffects->endScene(false, osystem_takeFrozenSceneDrawn());
    }

    // Snapshot the composited scene for the pause menu preview.
    // Must happen after endScene (so the PP chain has written the final image)
    // but before bgfx::frame() (which presents and advances the frame).
    osystem_updateSceneSnapshot();

    bgfx::frame();

    {
        static Uint64 last_time = SDL_GetPerformanceCounter();
        Uint64 now = SDL_GetPerformanceCounter();

        double freq = (double)SDL_GetPerformanceFrequency();
        double secs = (now - last_time) / freq;
        // Frames per second limit, an option of the launcher (graphics.frameLimit, 0 = none)
        int frameLimit = g_settings.graphics.frameLimit;
        double timeToWait = frameLimit > 0 ? ((1.0 / frameLimit) - secs) * 1000.0 : 0.0;
        if (timeToWait > 0)
        {
            SDL_DelayPrecise((Uint64)(timeToWait * 1000000.0));
        }

        last_time = SDL_GetPerformanceCounter();
    }
}

bgfx::Init initparam;

void createBgfxInitParams()
{
#if BX_PLATFORM_LINUX
    const char* sdlVideoDriver = SDL_GetCurrentVideoDriver();
    if (sdlVideoDriver == NULL) sdlVideoDriver = "(null)";
    if (SDL_strcmp(sdlVideoDriver, "x11") == 0)
    {
        initparam.platformData.ndt = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        initparam.platformData.nwh = (void*)SDL_GetNumberProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        initparam.platformData.type = bgfx::NativeWindowHandleType::Default;
    }
    else if (SDL_strcmp(sdlVideoDriver, "wayland") == 0)
    {
        initparam.platformData.ndt = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        initparam.platformData.nwh = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        initparam.platformData.type = bgfx::NativeWindowHandleType::Wayland;
    }
    printf("[BGFX] Linux SDL video driver='%s' ndt=%p nwh=%p\n", sdlVideoDriver, initparam.platformData.ndt, initparam.platformData.nwh);
#elif BX_PLATFORM_OSX
    initparam.platformData.ndt = NULL;
    initparam.platformData.nwh = cbSetupMetalLayer((void*)SDL_GetPointerProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL));
#elif BX_PLATFORM_WINDOWS
    initparam.platformData.ndt = NULL;
    initparam.platformData.nwh = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(gWindowBGFX), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif // BX_PLATFORM_
}

int initBgfxGlue(int argc, char* argv[])
{
    createBgfxInitParams();

    // Check for renderer command-line argument
    bool rendererSpecified = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d3d11") == 0 || strcmp(argv[i], "--d3d11") == 0)
        {
            initparam.type = bgfx::RendererType::Direct3D11;
            rendererSpecified = true;
            printf(BGFX_TAG "Forcing DirectX 11 renderer\n");
        }
        else if (strcmp(argv[i], "-d3d12") == 0 || strcmp(argv[i], "--d3d12") == 0)
        {
            initparam.type = bgfx::RendererType::Direct3D12;
            rendererSpecified = true;
            printf(BGFX_TAG "Forcing DirectX 12 renderer\n");
        }
        else if (strcmp(argv[i], "-opengl") == 0 || strcmp(argv[i], "--opengl") == 0)
        {
            initparam.type = bgfx::RendererType::OpenGL;
            rendererSpecified = true;
            printf(BGFX_TAG "Forcing OpenGL renderer\n");
        }
        else if (strcmp(argv[i], "-vulkan") == 0 || strcmp(argv[i], "--vulkan") == 0)
        {
            initparam.type = bgfx::RendererType::Vulkan;
            rendererSpecified = true;
            printf(BGFX_TAG "Forcing Vulkan renderer\n");
        }
    }

    // If no command-line override, check config file setting
    if (!rendererSpecified)
    {
        const char* backend = g_settings.graphics.rendererBackend;
        if (strcmp(backend, "d3d11") == 0)
            initparam.type = bgfx::RendererType::Direct3D11;
        else if (strcmp(backend, "d3d12") == 0)
            initparam.type = bgfx::RendererType::Direct3D12;
        else if (strcmp(backend, "opengl") == 0)
            initparam.type = bgfx::RendererType::OpenGL;
        else if (strcmp(backend, "vulkan") == 0)
            initparam.type = bgfx::RendererType::Vulkan;
        else if (strcmp(backend, "metal") == 0)
            initparam.type = bgfx::RendererType::Metal;
        else
#if BX_PLATFORM_LINUX
            initparam.type = bgfx::RendererType::Vulkan; // Linux default (Wayland surface handled via platformData.type)
#elif BX_PLATFORM_OSX
            initparam.type = bgfx::RendererType::Metal;
#else
            initparam.type = bgfx::RendererType::Direct3D11; // default fallback
#endif

        printf(BGFX_TAG "Using %s renderer (from config)\n", backend);
    }

    // ReShade compatibility settings
    // Set resolution explicitly to match window size
    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(gWindowBGFX, &windowWidth, &windowHeight);
    initparam.resolution.width = windowWidth;
    initparam.resolution.height = windowHeight;
    initparam.resolution.reset = getVsyncResetFlag();

    // Set buffer sizes for stable rendering
    initparam.limits.transientVbSize = 6 << 20; // 6MB
    initparam.limits.transientIbSize = 2 << 20; // 2MB

    // Alternative options (uncomment to test):
    //initparam.type = bgfx::RendererType::OpenGL;
    //initparam.type = bgfx::RendererType::Vulkan;
    //initparam.type = bgfx::RendererType::Direct3D12;
#if BX_CONFIG_DEBUG
    // DXGI reports validation failures by raising 0x87A. Keep the validation
    // device opt-in so ordinary Debug gameplay does not stop at
    // KernelBase!RaiseException in Visual Studio.
    initparam.debug = g_settings.debug.enableGraphicsValidation;
#else
    initparam.debug = false;
#endif

    printf(BGFX_TAG "Initializing BGFX with resolution: %dx%d\n", windowWidth, windowHeight);

    bgfx::init(initparam);

    // Print actual renderer type for debugging
    const char* rendererName = bgfx::getRendererName(bgfx::getRendererType());
    printf(BGFX_OK "BGFX initialized successfully with renderer: %s\n", rendererName);

    // Small delay to allow ReShade hooks to stabilize (helps prevent crashes)
    SDL_Delay(100);

    imguiCreate();

    ImGuiIO& io = ImGui::GetIO();
    // Menus (F1 and its windows) with keyboard and controller too
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    if ((bgfx::getRendererType() == bgfx::RendererType::Direct3D11) || (bgfx::getRendererType() == bgfx::RendererType::Direct3D12)) {
        ImGui_ImplSDL3_InitForD3D(gWindowBGFX);
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::Metal) {
        ImGui_ImplSDL3_InitForMetal(gWindowBGFX);
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::OpenGL) {
        ImGui_ImplSDL3_InitForOpenGL(gWindowBGFX, SDL_GL_GetCurrentContext());
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::Vulkan) {
        ImGui_ImplSDL3_InitForVulkan(gWindowBGFX);
    }
    else {
        ImGui_ImplSDL3_InitForOther(gWindowBGFX);
    }

    // The controller reaches ImGui through menu::feedGamepad, not the backend
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
    // Screen effects (offscreen scene; the effects are off unless the launcher enables them)
    // Sync outputResolution with the initial window size so the first
    // StartFrame() does not trigger a redundant bgfx::reset().
    windowResolution[0] = windowWidth;
    windowResolution[1] = windowHeight;
    computeSceneRect(windowWidth, windowHeight, s_sceneRect);
    computeSceneImageSize(s_sceneRect, outputResolution);

    g_screenEffects = new ScreenEffects();
    g_screenEffects->init(outputResolution[0], outputResolution[1]);
    g_screenEffects->setOutputRect(s_sceneRect[0], s_sceneRect[1], s_sceneRect[2], s_sceneRect[3], windowWidth, windowHeight);

    printf(BGFX_OK "Screen effects initialized (bloom %s, film grain %s, SSAO %s)\n",
        g_settings.postProcessing.enableBloom ? "on" : "off",
        g_settings.postProcessing.enableFilmGrain ? "on" : "off",
        g_settings.postProcessing.enableSSAO ? "on" : "off");

    // The layers above connect now (menu fonts, hooks): registerRemasterHooks
    if (g_onGraphicsReady)
        g_onGraphicsReady();

    return true;
}

