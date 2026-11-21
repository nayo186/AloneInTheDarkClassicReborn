///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Cheats chosen in F1 (g_settings.cheats): none acts while cheats are off.
// Infinite life: the player's life never goes down from a hit; a script
// death (a trap, a deadly fall or item) still kills. AITD3: only the life a
// hit of the engine takes is put back.
// Infinite ammunition (issue #15, BUGS.md C95; not Jack: he has no weapon):
// a weapon's shots never go down, and an empty weapon has one. Each game
// keeps them in variables of its own, read in its scripts (cheats.cpp); the
// log notes what the scripts lower around each shot, to find them.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// After each object's script (mainLoop.cpp): puts back the life a hit took
void applyCheats();

// The running script (life.cpp, DEC_VAR and SUB_VAR) lowers variable var by
// amount: true to leave it (a weapon's shots, with the cheat on)
bool cheatKeepsVariable(int lifeNum, int var, int amount);

// The value the running script reads for variable var (evalVar.cpp): with
// infinite ammunition, a weapon the story jams (AITD2 script 610, AITD3
// Gatling) never sees its jam;
// the variable itself is left, so turning the cheat off brings it back
int cheatVariableSeen(int lifeNum, int var, int value);

// The running script made the processed object fire (life.cpp, FIRE)
void cheatObjectFired(int lifeNum);
