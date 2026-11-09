///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The controller's vibration (see vibration.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "vibration.h"
#include "playerLife.h"
#include "input.h"

// The player's life after the last script; s_known false: not seen yet
static s16 s_lastLife = 0;
static bool s_known = false;
static const s16* s_lastVar = nullptr;   // the variable it was read from
static void vibrateDeath()
{
    controllerRumble(0.75f, 0.6f, 600);   // moderate
}

void vibrateOnLifeChange()
{
    const s16* life = playerLifeValue();
    if (life != s_lastVar)
    {
        // Another variable (AITD3: another hero, or C_VAR 2 set): noted, not a hit
        if (life)
            printf(LIFE_TAG "Player's life: vars[%d] = %d\n", (int)(life - vars), *life);
        s_lastVar = life;
        s_known = false;
    }
    if (!life)
        return;
    if (s_known && *life < s_lastLife && s_lastLife > 0)
    {
        if (*life <= 0)
            vibrateDeath();
        else if (!playerHitRecently())
            controllerRumble(0.35f, 0.5f, 150);   // life lost without a hit (a trap): small
    }
    s_lastLife = *life;
    s_known = true;
}

void vibrateOnShot(int shooter)
{
    if (shooter >= 0 && shooter == currentCameraTargetActor)
        controllerRumble(0.2f, 0.45f, 80);
}

void vibrateOnHit(int target)
{
    if (target < 0 || target != currentCameraTargetActor)
        return;
    notePlayerHit();
    controllerRumble(0.35f, 0.5f, 150);   // a hit: small
}

void vibrateOnDeath()
{
    vibrateDeath();
}

void forgetPlayerLife()
{
    s_known = false;
    forgetPlayerHit();
}
