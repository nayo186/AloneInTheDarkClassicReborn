///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The zoomed aspect ratios (graphics.aspect 3 = 16:9, 4 = 16:10): while
// playing, the 4:3 scene is enlarged to the width of a wide screen (up to
// 16:9; wider, the sides picture beyond) and part of it is out of the screen
// above and below; the view follows the character gently (a band in the
// middle where it does not move, a jump at a change of camera) and "Look up"
// / "Look down" (right stick) move it without moving the character. Menus,
// the inventory, books and pictures stay 4:3 whole, with a still picture of
// the game in the sides (blurred). The texts are always drawn in the 4:3 box
// of the screen, so messages are never cut. Alone 1 and Jack.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// A zoomed aspect ratio is chosen
bool widescreenZoomed();

// Where the scene goes in the window this frame (x, y, width, height; it
// may go out of the window above and below while playing), from the scene
// rectangle the aspect ratio gives (x, y, width, height). Called once per
// frame, when the image is composed: it moves the view.
void widescreenSceneQuad(const int sceneRect[4], int quad[4]);

// The 4:3 box of the window where texts and menus go (x, y, width, height)
void widescreenTextBox(const int sceneRect[4], int box[4]);

// This frame is a frame of play: inside the game's loop, no screen over it
// (osystem_screenOverPlay)
bool widescreenPlaying();
