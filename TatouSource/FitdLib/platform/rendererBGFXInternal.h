///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Internal declarations shared by rendererBGFX.cpp and the file split from
// it (sceneCapture.cpp)
///////////////////////////////////////////////////////////////////////////////

#pragma once

extern unsigned int gameViewId;
extern bgfx::TextureHandle g_backgroundTexture;
extern unsigned char physicalScreen[320 * 200];

// Frozen game frame shown behind menus (set by sceneEffects, drawn by menus)
extern bgfx::TextureHandle s_frozenSceneTex;
extern int s_frozenSceneWidth;
extern int s_frozenSceneHeight;

// Submits an RGBA picture quad (imageBackground_ps, with the fade level)
void submitImageBackground(bgfx::ViewId viewId);
