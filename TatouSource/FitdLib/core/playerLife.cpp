///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The player's life (see playerLife.h)
///////////////////////////////////////////////////////////////////////////////

#include "playerLife.h"

// AITD1 vars[21] (Carnby and Emily; a hit subtracts its force), Jack vars[6]
// (Grace; each time she is caught, one less): both start at 20 and a script's
// death sets 0. AITD2 vars[8] (Carnby; life 288 subtracts the hit's force):
// starts at 50, healing raises it up to 60 and a deadly item or trap
// subtracts 1000.
const PlayerLife* playerLife()
{
    static const PlayerLife kAITD1 = { 21, 20, 0 };
    static const PlayerLife kJack = { 6, 20, 0 };
    static const PlayerLife kAITD2 = { 8, 50, 1000 };
    switch (g_gameId)
    {
    case AITD1: return &kAITD1;
    case JACK: return &kJack;
    case AITD2: return &kAITD2;
    default: return nullptr;
    }
}

// When the engine last hit the player (game clock, 60 a second)
static unsigned int s_hitTime = 0;
static bool s_hit = false;

void notePlayerHit()
{
    s_hit = true;
    s_hitTime = timer;
}

bool playerHitRecently()
{
    return s_hit && timer - s_hitTime <= 60;
}

void forgetPlayerHit()
{
    s_hit = false;
}

s16* playerLifeValue()
{
    if (!vars)
        return nullptr;
    int var = -1;
    if (const PlayerLife* life = playerLife())
        var = life->var;
    else if (g_gameId == AITD3 && CVars.size() >= 3)
        var = CVars[2];   // as difficultyHeroLife (AITD3.cpp)
    if (var < 0 || var >= varSize / 2)
        return nullptr;
    return &vars[var];
}
