///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Remaster hooks: connects the remaster to the engine through the hooks of
// platform (platform/platformHooks.h) and core (core/coreHooks.h).
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Connects the remaster to the engine: menu fonts, platform hooks (menus,
// keys) and core hooks. Called once bgfx and ImGui are ready
// (g_onGraphicsReady, set by Fitd/fitd.cpp).
void registerRemasterHooks();
