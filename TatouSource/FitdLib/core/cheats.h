///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Cheats chosen in F1 (g_settings.cheats): none acts while cheats are off.
// Infinite life: the player's life never goes down from a hit; a script
// that sets it to 0 (a trap: a deadly fall, a scripted death) still kills.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// After each object's script (mainLoop.cpp): puts back the life a hit took
void applyCheats();
