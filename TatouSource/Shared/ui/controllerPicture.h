///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controller drawing of the controls window (controlsWindow.h):
// a PNG of 1400 x 862 with a transparent background (Shared/assets/
// controller.png, made for the project), compiled into the launcher and the
// game by Shared/embedPicture.cmake. Each program turns it into a texture of
// its own renderer and hands it to the window (ControlsWindowStyle).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>

extern const unsigned char kControllerPicture[];
extern const size_t kControllerPicture_size;
