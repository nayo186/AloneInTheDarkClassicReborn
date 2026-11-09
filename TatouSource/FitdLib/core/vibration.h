///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controller's vibration (controller.vibration, on by default; the
// Controls window turns it off): small when the player is hit, moderate when
// the player dies, a short touch at each of the player's shots
// (platform/input.cpp makes it vibrate)
///////////////////////////////////////////////////////////////////////////////

#pragma once

// After an object's script (mainLoop.cpp): the player's life went down (a
// hit) or to 0 or below (death)
void vibrateOnLifeChange();
// A shot left (animAction.cpp, DO_TIR): only the player's vibrates
void vibrateOnShot(int shooter);
// An object was hit (animAction.cpp, where HIT_BY is set): the player's hit vibrates
void vibrateOnHit(int target);
// A death of the scripts that never touches the life (AITD3's script 8, the
// journey to the land of the dead; life.cpp): it vibrates now
void vibrateOnDeath();
// A saved game was loaded: the life it brings is not a hit
void forgetPlayerLife();
