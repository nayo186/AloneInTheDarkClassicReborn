///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Cheats chosen in F1 (see cheats.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "cheats.h"
#include "playerLife.h"
#include "settings.h"

// AITD3: its life (the variable its C_VAR 2 names, vars[14] for Carnby) is
// put back only when the engine's hit on the player took it, within a
// second; any other change of the scripts goes on, so their deaths (the
// gunman after 12 shots, being swallowed, a fall) still kill: the hero is
// never left alive in a scene made for his death
static void applyCheatsAITD3()
{
    static s16 s_kept = 0;
    static const s16* s_keptVar = nullptr;
    s16* life = playerLifeValue();
    if (!g_settings.cheats.enabled || !g_settings.cheats.infiniteLife || !life)
    {
        s_keptVar = nullptr;
        return;
    }
    if (life != s_keptVar)
    {
        s_keptVar = life;
        s_kept = *life;
        return;
    }
    if (*life < s_kept && playerHitRecently())
        *life = s_kept;
    else
        s_kept = *life;
}

void applyCheats()
{
    if (g_gameId == AITD3)
    {
        applyCheatsAITD3();
        return;
    }
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
