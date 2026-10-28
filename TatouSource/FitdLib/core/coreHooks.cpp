///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Core hooks: storage for the event callbacks declared in coreHooks.h
///////////////////////////////////////////////////////////////////////////////

#include "coreHooks.h"

bool (*g_isOptionsWindowOpen)() = nullptr;
bool (*g_isGameFrozenLive)() = nullptr;
bool (*g_onObjectDraw)(int listIndex, int* x, int* y, int* z, int* alpha, int* beta, int* gamma, ObjectSize* size) = nullptr;
void (*g_onCameraBackgroundReady)(const char* pakName, int index, const unsigned char* pixels) = nullptr;
void (*g_onScreenPictureShown)(const char* pakName, int index) = nullptr;
void (*g_onStartMenuDrawn)() = nullptr;
void (*g_onGameLoopTick)() = nullptr;
