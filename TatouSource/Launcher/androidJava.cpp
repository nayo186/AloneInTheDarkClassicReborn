///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Android only: the app's Java side from the launcher (androidJava.h),
// through JNI calls to the static methods of the app's activity
///////////////////////////////////////////////////////////////////////////////

#include "androidJava.h"

#include <SDL3/SDL.h>
#include <jni.h>
#include <cstdio>

namespace
{

// A static method of the activity's class (from the activity object: a
// native thread cannot find the app's classes by name)
jmethodID activityMethod(JNIEnv* env, jclass& activityClass, const char* name, const char* signature)
{
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (!activity)
        return nullptr;
    activityClass = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    return env->GetStaticMethodID(activityClass, name, signature);
}

} // namespace

void androidImportStart(const std::string& userData, int game)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env)
        return;
    jclass activityClass = nullptr;
    jmethodID start = activityMethod(env, activityClass, "startImport", "(Ljava/lang/String;I)V");
    if (start)
    {
        jstring path = env->NewStringUTF(userData.c_str());
        env->CallStaticVoidMethod(activityClass, start, path, (jint)game);
        env->DeleteLocalRef(path);
    }
    if (activityClass)
        env->DeleteLocalRef(activityClass);
}

ImportProgress androidImportProgress()
{
    ImportProgress progress;
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env)
        return progress;
    jclass activityClass = nullptr;
    jmethodID status = activityMethod(env, activityClass, "importStatus", "()Ljava/lang/String;");
    if (status)
    {
        jstring text = (jstring)env->CallStaticObjectMethod(activityClass, status);
        if (text)
        {
            const char* chars = env->GetStringUTFChars(text, nullptr);
            int state = 0, kind = 0;
            if (sscanf(chars, "%d %lld %lld %d", &state, &progress.copied, &progress.total, &kind) == 4)
            {
                progress.state = (ImportState)state;
                progress.kind = (ImportKind)kind;
            }
            env->ReleaseStringUTFChars(text, chars);
            env->DeleteLocalRef(text);
        }
    }
    if (activityClass)
        env->DeleteLocalRef(activityClass);
    return progress;
}

bool androidDownload(const std::string& url, const std::string& path, int seconds)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env)
        return false;
    jclass activityClass = nullptr;
    jmethodID download = activityMethod(env, activityClass, "download", "(Ljava/lang/String;Ljava/lang/String;I)Z");
    bool ok = false;
    if (download)
    {
        jstring jurl = env->NewStringUTF(url.c_str());
        jstring jpath = env->NewStringUTF(path.c_str());
        ok = env->CallStaticBooleanMethod(activityClass, download, jurl, jpath, (jint)seconds);
        env->DeleteLocalRef(jurl);
        env->DeleteLocalRef(jpath);
    }
    if (activityClass)
        env->DeleteLocalRef(activityClass);
    return ok;
}

bool androidInstallPackage(const std::string& path)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env)
        return false;
    jclass activityClass = nullptr;
    jmethodID install = activityMethod(env, activityClass, "installPackage", "(Ljava/lang/String;)Z");
    bool ok = false;
    if (install)
    {
        jstring jpath = env->NewStringUTF(path.c_str());
        ok = env->CallStaticBooleanMethod(activityClass, install, jpath);
        env->DeleteLocalRef(jpath);
    }
    if (activityClass)
        env->DeleteLocalRef(activityClass);
    return ok;
}
