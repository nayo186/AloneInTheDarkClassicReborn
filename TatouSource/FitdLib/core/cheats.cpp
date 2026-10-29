///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Cheats chosen in F1 (see cheats.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "cheats.h"
#include "settings.h"

// The player's life in the scripts' variables (VARS.ITD): AITD1 vars[21]
// (Carnby and Emily; a hit subtracts its force), Jack vars[6] (Grace; each
// time she is caught, one less). Both start at 20.
static int playerLifeVar()
{
    if (g_gameId == AITD1)
        return 21;
    if (g_gameId == JACK)
        return 6;
    return -1;
}
static const s16 kStartLife = 20;

void applyCheats()
{
    // The life kept: the most the player had (healing raises it), at least
    // the starting one
    static s16 s_kept = 0;

    const int lifeVar = playerLifeVar();
    if (!g_settings.cheats.enabled || !g_settings.cheats.infiniteLife || lifeVar < 0 || !vars)
    {
        s_kept = 0;
        return;
    }
    s16& life = vars[lifeVar];
    if (s_kept < kStartLife)
        s_kept = kStartLife;
    if (life > s_kept)
        s_kept = life;
    // Exactly 0 is a script's death (AITD1: a fall from too high; Jack: a
    // trap), with its animation already started: it goes on. A hit only
    // subtracts and is put back.
    if (life < s_kept && life != 0)
        life = s_kept;
}
