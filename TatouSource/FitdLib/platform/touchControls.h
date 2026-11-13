///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// On-screen controls for touch screens (phones): a semi-transparent Xbox
// controller drawn over the game. It is a virtual controller: its buttons
// (A B X Y, Back, Start, RT, the D-pad) and its left stick go through the
// same controls as a real one (controls.cpp, Shared/controlsBindings.h), so
// they mean the same as on PC (the D-pad turns as a tank, the stick walks
// where it points with the modern stick) and a change in the Controls window
// changes them too. A button shows only when something is assigned to it.
// Sized in millimetres, they sit in the
// black bars at the sides of the picture when these are wide enough,
// otherwise over its edges. Several fingers at once. They show at a touch and
// hide when the keyboard or a controller is used (shown from the start on
// Android). Android's Back key is the B button.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL.h>

// A finger event or the Back key (window thread, readKeyboard)
void touchControlsEvent(const SDL_Event& event);
// Once per frame before the inputs are read (window thread): places the
// controls for the window; menuOpen: the F1 window owns the screen (all let go)
void touchControlsFrame(bool menuOpen);
// A button of the virtual controller (SDL_GamepadButton) is held
bool touchPadButton(int button);
// An axis of the virtual controller (SDL_GamepadAxis): the left stick from
// -1 to 1, the right trigger 0 or 1; 0 for the rest
float touchPadAxis(int axis);
// A shown button or the D-pad of the visible controls is at that point of the
// window (SDL's mouse coordinates). The mouse SDL makes from such a touch
// must not reach the menus: Start on the right picked Carnby (BUGS.md C79)
bool touchControlsAt(float x, float y);
// The keyboard or a controller was used: the controls hide until a touch
void touchControlsHide();
// Draws the controls over the game (game thread, inside the ImGui frame)
void touchControlsDraw();
