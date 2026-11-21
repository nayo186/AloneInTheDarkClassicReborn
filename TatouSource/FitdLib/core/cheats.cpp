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
#include "consoleLog.h"

#include <set>

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

static void applyInfiniteAmmo();

void applyCheats()
{
    applyInfiniteAmmo();
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

// ── Infinite ammunition ──────────────────────────────────────────────────────

// The variables that keep a weapon's shots, read in each game's scripts
// (LISTLIFE: the weapon's script fires only while it is above 0 and lowers
// it by one shot): AITD1 13 the revolver (script 12) and 92 the rifle
// (script 365); AITD2, seen in the player's logs: 0 the Thompson (script 0,
// 10 shots), 9 and 37 (scripts 5 and 28), and later in the game 201 the
// Thompson (script 294) and 87 the pirate's pistol (script 292, one shot,
// loaded by script 95); read in the whole LISTLIFE dump: 6 (script 610, 30
// shots), 7 (script 350, 30 shots), 88 (script 317, one shot, loaded by
// scripts 95, 298 and 299) and 429 (script 688, 4 more from scripts 702 and
// 734). AITD3 104 the revolver (script 236, six in the cylinder, script 261
// reloads it), 319 the Winchester (script 1082, 10 from its reloads), 323
// (script 850, six from script 747), 401 (script 1094, 10 from script
// 1172), 444 the Gatling (script 1194, 20 shots from script 1175) and 465
// (scripts 1222 and 1224). The log notes what the scripts lower around each
// shot ("Shot of script"), to find any other
static const int kAmmoAITD1[] = { 13, 92 };
static const int kAmmoAITD2[] = { 0, 6, 7, 9, 37, 87, 88, 201, 429 };
static const int kAmmoAITD3[] = { 104, 319, 323, 401, 444, 465 };

// Weapons the story jams for good, not their shots: a scene sets a variable
// and the weapon's script then only says it is jammed. AITD2: script 459
// sets vars[344] and script 610 fires only while it is 0; AITD3: the
// sequence of script 1164 sets vars[455] and the Gatling (script 1194)
// fires only while it is 0. With the cheat on that script reads it as 0
// (cheatVariableSeen); the variable stays, so turning the cheat off or a
// save keeps the jam
struct StoryJam { int game; int life; int var; };
static const StoryJam kStoryJams[] = {
    { AITD2, 610, 344 },
    { AITD3, 1194, 455 },
};

static void gameAmmo(const int** list, int* count)
{
    *list = nullptr;
    *count = 0;
    if (g_gameId == AITD1) { *list = kAmmoAITD1; *count = (int)(sizeof(kAmmoAITD1) / sizeof(int)); }
    else if (g_gameId == AITD2) { *list = kAmmoAITD2; *count = (int)(sizeof(kAmmoAITD2) / sizeof(int)); }
    else if (g_gameId == AITD3) { *list = kAmmoAITD3; *count = (int)(sizeof(kAmmoAITD3) / sizeof(int)); }
}

static bool ammoVariable(int var)
{
    const int* list;
    int count;
    gameAmmo(&list, &count);
    for (int i = 0; i < count; i++)
        if (list[i] == var)
            return true;
    return false;
}

static bool infiniteAmmoOn()
{
    return g_settings.cheats.enabled && g_settings.cheats.infiniteAmmo && g_gameId != JACK;
}

// Scripts that made an object fire, and the last shot (the game's clock)
static std::set<int> s_firingScripts;
static int s_firedLife = -1;
static unsigned int s_firedTime = 0;

// With the cheat on, a weapon is never empty: one shot at least, also when
// its shots ran out before the cheat was turned on
static void applyInfiniteAmmo()
{
    if (!infiniteAmmoOn() || !vars)
        return;
    const int* list;
    int count;
    gameAmmo(&list, &count);
    for (int i = 0; i < count; i++)
        if (vars[list[i]] < 1)
            vars[list[i]] = 1;
}

int cheatVariableSeen(int lifeNum, int var, int value)
{
    if (value == 0 || !infiniteAmmoOn())
        return value;
    for (const StoryJam& jam : kStoryJams)
        if (jam.game == g_gameId && jam.life == lifeNum && jam.var == var)
            return 0;
    return value;
}

bool cheatKeepsVariable(int lifeNum, int var, int amount)
{
    if (amount <= 0 || g_gameId == JACK || !vars)
        return false;
    // What a script that fires lowers: to find the shots of each weapon
    if (s_firingScripts.count(lifeNum))
        printf(CFG_TAG "Shot of script %d: variable %d goes down by %d (was %d; object %d, player %d, %u ticks after its shot)\n",
            lifeNum, var, amount, vars[var], currentProcessedActorIdx, currentCameraTargetActor,
            s_firedLife == lifeNum ? timer - s_firedTime : 0u);
    return infiniteAmmoOn() && ammoVariable(var);
}

void cheatObjectFired(int lifeNum)
{
    if (g_gameId == JACK)
        return;
    s_firingScripts.insert(lifeNum);
    s_firedLife = lifeNum;
    s_firedTime = timer;
    printf(CFG_TAG "Shot: script %d, object %d, player %d\n", lifeNum, currentProcessedActorIdx, currentCameraTargetActor);
}
