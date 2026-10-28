///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Platform hooks: the platform layer (window, ImGui, keyboard) announces what
// happens and the layers above answer, so platform never names the
// remaster's menus. Like core/coreHooks.h: each pointer starts empty and the
// remaster fills it (remaster/config/remasterHooks.cpp); an empty pointer
// means "nobody listens".
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bgfx/bgfx.h>

// bgfx and ImGui are ready (end of initBgfxGlue). The program (Fitd/fitd.cpp)
// points it to registerRemasterHooks, which loads the menu fonts and fills
// the other hooks.
extern void (*g_onGraphicsReady)();

// Inside the ImGui frame, after everything else: the menus drawn on top
// (F1 and the debug windows)
extern void (*g_onDrawMenus)();

// A key with no meaning of its own was pressed (SDL scancode): F9, F10...
extern void (*g_onKeyPressed)(int scancode);

// The "Options window" action (F1, Home, Back by default) was pressed
extern void (*g_onOptionsAction)();

// True while a menu owns keyboard, mouse and controller: the game receives
// no input
extern bool (*g_isMenuCapturingInput)();

// ---- Pictures of the player (remaster/graphics/customBackgrounds.cpp) -------

// A picture that replaces the game's background on screen
struct BackgroundPicture
{
    bgfx::TextureHandle picture;   // the player's image
    bgfx::TextureHandle original;  // the game's own picture of this screen
    float u0, u1;                  // the 4:3 part of the image (0..1 across)
};

// Each frame, before drawing: pictures saved or removed are picked up
extern void (*g_onFrameStart)();

// The picture for the screen on show; false: the game's own background
extern bool (*g_getBackgroundPicture)(BackgroundPicture* out);

// The player's picture of a mask of the game (invalid handle: none);
// originalShape: 16:10 like the scene, otherwise the shape of the picture
extern bgfx::TextureHandle (*g_getMaskPicture)(int roomId, int maskId, bool* originalShape);

// The 320x200 screen was sent to the graphics card
extern void (*g_onScreenUploaded)(const unsigned char* screen);

// A mask of the game was made (its 320x200 shape and rectangle)
extern void (*g_onMaskCreated)(int roomId, int maskId, const unsigned char* shape, int x1, int y1, int x2, int y2);
