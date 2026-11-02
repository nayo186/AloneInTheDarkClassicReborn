///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Cheats chosen in F1 (see cheats.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "cheats.h"
#include "settings.h"

// The player's life in the scripts' variables (VARS.ITD)
struct PlayerLife
{
    int var;         // its index in vars
    s16 start;       // the life a game starts with
    s16 deadlyDrop;  // a drop this big is a script's death (0: life set to exactly 0)
};

// AITD1 vars[21] (Carnby and Emily; a hit subtracts its force), Jack vars[6]
// (Grace; each time she is caught, one less): both start at 20 and a script's
// death sets 0. AITD2 vars[8] (Carnby; life 288 subtracts the hit's force):
// starts at 50, healing raises it up to 60 and a deadly item or trap
// subtracts 1000.
static const PlayerLife* playerLife()
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

void applyCheats()
{
    // The life kept: the most the player had (healing raises it), at least
    // the starting one
    static s16 s_kept = 0;
    // A script's death has started: its animation goes on until the end
    static bool s_dying = false;

    const PlayerLife* player = playerLife();
    if (!g_settings.cheats.enabled || !g_settings.cheats.infiniteLife || !player || !vars)
    {
        s_kept = 0;
        s_dying = false;
        return;
    }
    s16& life = vars[player->var];
    if (s_kept < player->start)
        s_kept = player->start;
    if (life > s_kept)
        s_kept = life;
    // A life above 0 again (a saved game loaded, a new game) ends a death
    if (life > 0)
        s_dying = false;
    if (life >= s_kept || s_dying)
        return;
    // A script's death (AITD1: a fall from too high; Jack: a trap; AITD2: a
    // deadly item or trap), with its animation already started, goes on;
    // its script may change the life again (AITD2 sets -1). A hit only
    // subtracts and is put back.
    const bool scriptDeath = player->deadlyDrop > 0 ? s_kept - life >= player->deadlyDrop : life == 0;
    if (scriptDeath)
        s_dying = true;
    else
        life = s_kept;
}
