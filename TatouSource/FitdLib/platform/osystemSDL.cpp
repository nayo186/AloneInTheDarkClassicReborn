///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// SDL-based operating system abstraction layer
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "exceptionHandler.h"
#include "logFile.h"
#include "gameFolder.h"
#include "consoleLog.h"
#include <exception>
#include <atomic>
#include <bgfx/platform.h>

/***************************************************************************
mainSDL.cpp  -  description
-------------------
begin                : Mon Jun 3 2002
copyright            : (C) 2002 by Yaz0r
email                : yaz0r@yaz0r.net
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "bgfxGlue.h"
#include "jobSystemInit.h"
#include "settings.h"
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include "osystem.h"
#include "osystemAL.h"
#include <backends/imgui_impl_sdl3.h>

void detectGame(void);

int osystem_mouseRight;
int osystem_mouseLeft;

/*void OSystem::getMouseStatus(mouseStatusStruct * mouseData)
{

SDL_GetMouseState(&mouseData->X, &mouseData->Y);

mouseData->left = mouseLeft;
mouseData->right = mouseRight;

mouseLeft = 0;
mouseRight = 0;
}*/

#ifdef WIN32
#define CALLBACK __stdcall
#else
#define CALLBACK
#endif

extern "C" {
    void Sound_Quit(void);
}

void osystemAL_deinit();

void Sound_Quit(void)
{
    osystemAL_deinit();
}

extern "C" {
    char homePath[512] = "";
    int FitdMain(int argc, char* argv[]);
}

// Command line kept by FitdInit for the game thread (launcher parameters)
static int s_argc = 0;
static char** s_argv = nullptr;

// C++ exceptions (std::exception, bad_alloc, filesystem errors...) thrown on the
// game thread are caught here so their message ends up in tatou.log.
// Hardware faults (access violations, heap corruption) are not C++ exceptions;
// those are handled by the SEH wrapper below and the ExceptionHandler.
// Must not be inlined into the __try function, or the C++ catch is lost.
#ifdef _MSC_VER
__declspec(noinline)
#endif
static int FitdMainCatchCpp(void* unused)
{
    // FitdMain is extern "C"; with /EHsc the compiler assumes it never throws
    // and drops this try/catch. Calling through a volatile pointer keeps it.
    int (*volatile fitdMain)(int, char**) = FitdMain;
    try
    {
        return fitdMain(s_argc, s_argv);
    }
    catch (const std::exception& e)
    {
        printf(SDL_ERR "CRITICAL: Unhandled C++ exception on game thread: %s" CON_RESET "\n", e.what());
    }
    catch (...)
    {
        printf(SDL_ERR "CRITICAL: Unhandled unknown C++ exception on game thread" CON_RESET "\n");
    }
    logFileFlush();
    return -1;
}

// SEH wrapper around FitdMain to catch any unhandled exceptions on the game thread.
// This is a belt-and-suspenders approach on top of the VEH in ExceptionHandler.
// Must be a plain C-style function (no C++ destructors on stack) for __try/__except.
static int FitdMainProtected(void* unused)
{
#ifdef _WIN32
    __try
    {
        return FitdMainCatchCpp(unused);
    }
    __except (ExceptionHandler::HandleException(GetExceptionInformation(), "FitdMainThread"))
    {
        printf(SDL_ERR "CRITICAL: Exception caught at FitdMainThread top level - game continuing" CON_RESET "\n");
        OutputDebugStringA("CRITICAL: FitdMainThread top-level exception caught and handled\n");
        return -1;
    }
#else
    return FitdMainCatchCpp(unused);
#endif
}

SDL_Semaphore* startOfRender = NULL;
SDL_Semaphore* endOfRender = NULL;

// Set when the game thread returns (normally or after an exception), so the
// main thread stops waiting for frames that will never come (BUGS.md C10)
static std::atomic<bool> s_gameThreadEnded(false);
static int s_gameThreadResult = 0;

static int GameThreadMain(void* unused)
{
    s_gameThreadResult = FitdMainProtected(unused);
    s_gameThreadEnded = true;
    SDL_SignalSemaphore(endOfRender);
    return s_gameThreadResult;
}

static void exitIfGameThreadEnded()
{
    if (!s_gameThreadEnded)
        return;
    printf(SDL_ERR "The game thread ended (result %d): closing the window" CON_RESET "\n", s_gameThreadResult);
    logFileFlush();
    exit(s_gameThreadResult);
}

//SDL_sem* emptyCount = NULL;
//SDL_sem* fullCount = NULL;

bool bFirst = true;

int FitdInit(int argc, char* argv[])
{
    s_argc = argc;
    s_argv = argv;

    // Jack in the Dark runs in the JACK subfolder (before the log file opens)
    selectGameFolder(argc, argv);

#ifdef WIN32
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

    // Initialize exception handler for crash logging and graceful recovery
    // Set to true to try continuing on heap corruption (logs error and continues)
    logFileInit();
    ExceptionHandler::Initialize(true);

    printf(SDL_OK "Log file: tatou.log (previous session: tatou.prev.log)\n");
    logGameFolder();
    printf(SDL_OK "Exception handler initialized - crash logs will be written to crash_log.txt\n");

#endif
    startOfRender = SDL_CreateSemaphore(0);
    endOfRender = SDL_CreateSemaphore(0);

    osystem_init();

    loadSettings();

    // Initialize job system for multithreaded asset loading
    initJobSystem();

    unsigned int flags = 0;
    // Not resizable: a window keeps the size and shape of the aspect ratio
    // (applyWindowAspectRatio, bgfxGlue.cpp); fullscreen fills the screen
    //flags |= SDL_WINDOW_ALLOW_HIGHDPI;

#ifdef __IPHONEOS__
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

    // Game is running in dos resolution 13h, ie 320x200x256, but is displayed in 4:3, so pixel are not square (1.6:1)
    // We still need to create a 4:3 window for the actual display on screen.
    int resolution[2] = { 1410, 890 };

    gWindowBGFX = SDL_CreateWindow("FITD", resolution[0], resolution[1], flags);

    // Fullscreen saved in the options (F1 or the launcher): switched by the
    // window thread on its first frame, for every game (it used to be done
    // on AITD1's Tatou screen only, so Jack always opened in a window)
    if (g_settings.graphics.fullscreen)
        g_pendingFullscreenToggle = true;

#ifdef _WIN32
    // Now that the main window is visible, show the console window
    // positioned so it doesn't overlap the game window
    HWND hConsole = GetConsoleWindow();
    if (hConsole)
    {
        int wx, wy;
        SDL_GetWindowPosition(gWindowBGFX, &wx, &wy);

        // Place the console to the right of the main window
        int consoleX = wx + resolution[0] + 8;

        // If it would go off-screen, place it below the main window instead
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        RECT consoleRect;
        GetWindowRect(hConsole, &consoleRect);
        int consoleW = consoleRect.right - consoleRect.left;
        int consoleH = consoleRect.bottom - consoleRect.top;

        if (consoleX + consoleW > screenW)
        {
            // Doesn't fit to the right — place below
            SetWindowPos(hConsole, NULL, wx, wy + resolution[1] + 8, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            SetWindowPos(hConsole, NULL, consoleX, wy, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        ShowWindow(hConsole, SW_SHOWNOACTIVATE);
    }
#endif

    char version[256];

    getVersion(version);

    printf(SDL_TAG "%s", version);

    detectGame();

    // Call renderFrame before creating the game thread to signal bgfx
    // to not create its own internal render thread. This keeps all D3D11
    // calls (device creation, Present) on the main thread so Steam
    // Overlay can hook them properly.
    bgfx::renderFrame();

    SDL_CreateThread(GameThreadMain, "FitdMainThread", NULL);

    // During init, bgfx::init() calls frame() internally which needs
    // multiple renderFrame() calls from this thread. Pump renderFrame
    // in a polling loop until the game thread signals its first
    // endOfRender (meaning init is done and the game loop has started).
    SDL_SignalSemaphore(startOfRender);
    do {
        bgfx::renderFrame(100);
        exitIfGameThreadEnded();
    } while (!SDL_TryWaitSemaphore(endOfRender));

    unsigned long int t_start = static_cast<unsigned long>(SDL_GetTicks());
    unsigned long int t_lastUpdate = t_start;

    int FRAMES_PER_SECOND = 25;

    u32 startOfPreviousFrame = static_cast<u32>(SDL_GetTicks());
    bool bFirstFrame = true;

    while (1)
    {
        u32 startOfFrame = static_cast<u32>(SDL_GetTicks());

        assert(startOfPreviousFrame <= startOfFrame);

        u32 tickDifference = startOfFrame - startOfPreviousFrame;

        if (tickDifference < static_cast<u32>(1000 / FRAMES_PER_SECOND))
        {
           // SDL_Delay((1000 / FRAMES_PER_SECOND) - tickDifference);
        }

        startOfPreviousFrame = startOfFrame;

        unsigned long int t_sinceStart = static_cast<unsigned long>(SDL_GetTicks());

        int delta = 0;

        //if(t_sinceStart + 10 > t_lastUpdate)
        {
            delta = (t_sinceStart - t_lastUpdate);
            t_lastUpdate = t_sinceStart;
        }

        osystemAL_udpate();

        SDL_PumpEvents();

        SDL_GL_MakeCurrent(NULL, NULL);

        // Don't process events on first frame to avoid race condition with the init code
        if(!bFirstFrame)
        {
            readKeyboard();
        }
        else {
            bFirstFrame = false;
        }

        // Process deferred fullscreen toggle on the main/window thread
        if (g_pendingFullscreenToggle)
        {
            g_pendingFullscreenToggle = false;
            toggleFullscreen();
        }
        applyWindowAspectRatio();

        SDL_SignalSemaphore(startOfRender);

        // Normal operation: one renderFrame per game frame. Waits for the
        // game thread to submit via bgfx::frame(), then for its endOfRender
        // signal. Both waits are in short steps so a game thread that has
        // ended does not leave the window frozen (BUGS.md C10).
        for (;;)
        {
            if (bgfx::renderFrame(100) != bgfx::RenderFrame::Timeout)
            {
                while (!SDL_WaitSemaphoreTimeout(endOfRender, 100))
                    exitIfGameThreadEnded();
                break;
            }
            if (SDL_TryWaitSemaphore(endOfRender))
                break;
            exitIfGameThreadEnded();
        }
    }

    return 0;
}

// Game clock (timer) rate: 60 ticks per second of real time, the speed the
// game always had at 60 frames per second. The old code advanced at least
// 1 tick per frame and dropped the rest, so the game speed followed the
// frame rate (half speed at 30 fps, double at 120).
static const Uint64 TIMER_TICK_NS = 1000000000ull / 60;
// Longer waits (loading, a window drag) are not counted: at most 1/4 second
static const u32 TIMER_MAX_TICKS_PER_FRAME = 15;

static Uint64 lastFrameTimeNS = 0;

// Ticks of game clock to add this frame; can be 0 above 60 frames per second
u32 osystem_startOfFrame()
{
    SDL_WaitSemaphore(startOfRender);

    StartFrame();
    osystem_startFrame();

    static bool firstFrame = true;
    if (firstFrame)
    {
        //
#ifdef USE_IMGUI
        //ImGui_ImplSdlGL3_Init(sdl_window);
#endif
        lastFrameTimeNS = SDL_GetTicksNS();

        firstFrame = false;
    }

    // Whole ticks elapsed; the rest of a tick stays for the next frame
    Uint64 now = SDL_GetTicksNS();
    Uint64 ticks = (now - lastFrameTimeNS) / TIMER_TICK_NS;
    lastFrameTimeNS += ticks * TIMER_TICK_NS;

    if (ticks > TIMER_MAX_TICKS_PER_FRAME)
    {
        ticks = TIMER_MAX_TICKS_PER_FRAME;
        lastFrameTimeNS = now;
    }

#ifdef USE_IMGUI
    //ImGui_ImplSdlGL3_NewFrame(sdl_window);
#endif

    return static_cast<u32>(ticks);
}

void osystem_endOfFrame()
{
    osystem_flushPendingPrimitives();

    osystem_drawUILayer();

#ifdef FITD_DEBUGGER
    debugger_draw();
#endif

#ifdef USE_IMGUI
    //ImGui::Render();
#endif

   // osystem_flip(NULL);


    EndFrame();

    if (bFirst)
        bFirst = false;

    SDL_SignalSemaphore(endOfRender);
    //SDL_SemPost(emptyCount);

}

// A file of the game data, in homePath (platform/gameFolder.cpp)
bool dataFileExists(const char* name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s%s", homePath, name);
    return fileExists(path);
}

bool fileExists(const char* name)
{
    FILE* fHandle;

    fopen_s(&fHandle, name, "rb");

    if (fHandle)
    {
        fclose(fHandle);
        return 1;
    }
    return 0;
}

void osystem_init()  // that's the constructor of the system dependent
// object used for the SDL port
{
#if defined(__linux__)
    // On Linux, prefer the Wayland backend with X11 as fallback. On modern
    // Wayland-first distros (e.g. CachyOS, Fedora, recent Ubuntu) some libX11
    // versions interact badly with the shipped SDL3 X11 path and can fault
    // inside libX11 with a corrupted Display*. Setting this hint here is a
    // no-op if the user already exported SDL_VIDEODRIVER / SDL_VIDEO_DRIVER
    // (env vars override hints at OVERRIDE priority).
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        assert(0);
    }

    // SDL_ShowCursor (SDL_DISABLE);

    // SDL_EnableUNICODE (SDL_ENABLE); // not much used in fact

    SDL_PumpEvents();

    int screen_width = 800;
    int screen_height = 600;

    Uint32 windowFlags = SDL_WINDOW_OPENGL;

#ifdef RUN_FULLSCREEN
    windowFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    osystem_mouseLeft = 0;
    osystem_mouseRight = 0;


    osystemAL_init();
}

int posInStream = 0;
volatile bool deviceStatus = false;



