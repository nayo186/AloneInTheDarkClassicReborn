///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Core hooks: events the original engine announces without knowing who
// listens. Optional systems (the remaster) register here; when nothing is
// registered the engine behaves as the original game.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

struct sBody;
struct ObjectSize;
struct primEntryStruct;

// A camera background was loaded: the picture (320x200 palette indexes) and
// where it comes from (PAK name and index). Custom backgrounds use it.
extern void (*g_onCameraBackgroundReady)(const char* pakName, int index, const unsigned char* pixels);

// A full-screen picture (title, book page, character choice...) is about to
// be shown; the core loads and draws it as always. Custom backgrounds use it.
extern void (*g_onScreenPictureShown)(const char* pakName, int index);

// A frame of the start menu was drawn (the controller help goes over it).
extern void (*g_onStartMenuDrawn)();

// Once per loop of the game, on the game thread, before the objects' scripts
// (debug cheats act here).
extern void (*g_onGameLoopTick)();

// An object of the camera is about to be drawn at this position (world
// units), rotation (1024 = a full turn) and size (renderer.h); listIndex is
// its place in ListObjets. A listener may change how it is drawn (a visual
// fix: the game keeps the real position) and returns false to skip drawing
// it this frame.
extern bool (*g_onObjectDraw)(int listIndex, int* x, int* y, int* z, int* alpha, int* beta, int* gamma, ObjectSize* size);

// True while a tool freezes the game but needs the scene drawn every frame
// (object editor): PlayWorld then only redraws, with the clock stopped.
extern bool (*g_isGameFrozenLive)();

// True while an options window over the game is open (F1): the game waits.
extern bool (*g_isOptionsWindowOpen)();

// A full-screen picture from a PAK is about to be shown
inline void coreScreenPictureShown(const char* pakName, int index)
{
    if (g_onScreenPictureShown)
        g_onScreenPictureShown(pakName, index);
}
