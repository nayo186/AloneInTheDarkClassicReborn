///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Fonts of the in-game menus (F1, controls, audio): the launcher's Georgia,
// made once at the largest scale this screen can need and drawn smaller with
// the window font scale, so the letters stay sharp at any window size.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ui/menuBase.h"

// Adds the fonts to ImGui's atlas and rebuilds its texture (after imguiCreate)
void menuFontsLoad();

// The fonts and the scale they were made at (1 if not loaded: ImGui's font)
const menu::Fonts& menuFonts();
// Scale the menu fonts were made at
float menuFontsScale();
