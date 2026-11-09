///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The player's life in the scripts' variables (VARS.ITD) of each game: used
// by the infinite life cheat (cheats.cpp) and the controller's vibration
// (vibration.cpp)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "common.h"

struct PlayerLife
{
    int var;         // its index in vars
    s16 start;       // the life a game starts with
    s16 deadlyDrop;  // a drop this big is a script's death (0: life set to exactly 0)
};

// The player's life of AITD1, Jack and AITD2, in a variable of their own;
// null in the other games
const PlayerLife* playerLife();
// The player's life now in any of the four games (AITD3: the variable its
// C_VAR 2 names, that of the hero being played); null if unknown
s16* playerLifeValue();
// The engine hit the player (animAction.cpp, through vibration.cpp)
void notePlayerHit();
// True within a second of that hit: the life the player's script takes then
// is the hit's (vibration.cpp, AITD3's infinite life in cheats.cpp)
bool playerHitRecently();
// A saved game was loaded: no hit before it counts
void forgetPlayerHit();
