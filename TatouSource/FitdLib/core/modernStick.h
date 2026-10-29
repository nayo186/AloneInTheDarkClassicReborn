///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Modern left stick: the player walks where the stick points, seen from the
// camera, like the Resident Evil HD remasters. The d-pad and the keys stay
// tank controls. Through a camera cut the direction is kept until the stick
// is released or pushed elsewhere, so the player does not walk back.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Each frame, after localJoyD is read (mainLoop.cpp): while the left stick
// steers the player, turns its tank directions into "forward" or a turn on
// the spot towards the stick, for the scripts' animations
void updateModernStick();

// In processTrack for the player under manual control: turns it towards the
// stick and returns true, or returns false when the stick is not steering
// (then the tank controls turn it)
bool modernStickTurn();
