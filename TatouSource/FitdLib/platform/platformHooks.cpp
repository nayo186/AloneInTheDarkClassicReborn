///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Platform hooks (see platformHooks.h)
///////////////////////////////////////////////////////////////////////////////

#include "platformHooks.h"

void (*g_onGraphicsReady)() = nullptr;
void (*g_onDrawMenus)() = nullptr;
void (*g_onKeyPressed)(int scancode) = nullptr;
void (*g_onOptionsAction)() = nullptr;
bool (*g_isMenuCapturingInput)() = nullptr;

void (*g_onFrameStart)() = nullptr;
bool (*g_getBackgroundPicture)(BackgroundPicture* out) = nullptr;
bgfx::TextureHandle (*g_getMaskPicture)(int roomId, int maskId, bool* originalShape, float crop[4]) = nullptr;
void (*g_onScreenUploaded)(const unsigned char* screen) = nullptr;
void (*g_onMaskCreated)(int roomId, int maskId, const unsigned char* shape, int x1, int y1, int x2, int y2) = nullptr;

const unsigned char* (*g_filterTextLayer)(const unsigned char* layer) = nullptr;
void (*g_drawTextOverlay)(bgfx::ViewId view) = nullptr;
bgfx::TextureHandle (*g_getWidescreenSides)(float* aspect) = nullptr;
