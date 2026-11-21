///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// "Return to the launcher" (see returnToLauncher.h)
///////////////////////////////////////////////////////////////////////////////

#include "returnToLauncher.h"
#include "consoleLog.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <cstdio>
#include <string>

#ifdef __ANDROID__
#include <jni.h>
#endif

static bool s_startedByLauncher = false;
static std::atomic_bool s_requested(false);

void setStartedByLauncher(bool started)
{
    s_startedByLauncher = started;
}

bool startedByLauncher()
{
    return s_startedByLauncher;
}

void requestReturnToLauncher()
{
    s_requested.store(true, std::memory_order_release);
}

bool returnToLauncherRequested()
{
    return s_requested.load(std::memory_order_acquire);
}

#ifdef __ANDROID__
// ClassicRebornActivity.restartOnLauncher: a small activity in a process of
// its own waits for this one to end and starts the app again (the launcher's
// screen comes first when the app starts without arguments, Fitd/fitd.cpp)
bool openLauncher()
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (!env || !activity)
        return false;
    bool started = false;
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID method = activityClass ? env->GetStaticMethodID(activityClass, "restartOnLauncher", "()Z") : nullptr;
    if (method)
        started = env->CallStaticBooleanMethod(activityClass, method);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        started = false;
    }
    if (activityClass)
        env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
    printf(MAIN_TAG "Return to the launcher: %s\n", started ? "the app starts again" : "could not restart the app");
    return started;
}
#else
// The Launcher next to Tatou (the packages keep them together); it finds its
// own folder and settings by itself
bool openLauncher()
{
    const char* base = SDL_GetBasePath();
    if (!base)
        return false;
#ifdef _WIN32
    const std::string path = std::string(base) + "Launcher.exe";
#else
    const std::string path = std::string(base) + "Launcher";
#endif
    const char* argv[] = { path.c_str(), nullptr };
    SDL_Process* process = SDL_CreateProcess(argv, false);
    if (!process)
    {
        printf(MAIN_WARN "Return to the launcher: could not start %s: %s" CON_RESET "\n", path.c_str(), SDL_GetError());
        return false;
    }
    SDL_DestroyProcess(process);   // does not stop the launcher
    printf(MAIN_TAG "Return to the launcher: %s started\n", path.c_str());
    return true;
}
#endif
