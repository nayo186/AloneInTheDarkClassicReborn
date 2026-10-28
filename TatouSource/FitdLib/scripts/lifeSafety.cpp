///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Safety checks after each life script instruction (see lifeSafety.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "lifeSafety.h"
#include "life.h"
#include "consoleLog.h"

// The animation must exist and its frame must be inside it
static void checkAnimation(int lifeNum, tObject* actor)
{
    if (actor->ANIM < 0)
        return;

    sAnimation* anim = HQR_Get(HQ_Anims, actor->ANIM);
    if (!anim)
    {
        printf(LIFE_WARN "Animation %d not found (life %d, object %d): animation stopped" CON_RESET "\n",
               actor->ANIM, lifeNum, actor->indexInWorld);
        actor->ANIM = -1;
        return;
    }
    if (anim->m_numFrames > 0 && actor->frame >= anim->m_numFrames)
    {
        printf(LIFE_WARN "Frame %d past the end of animation %d (life %d, object %d): last frame used" CON_RESET "\n",
               actor->frame, actor->ANIM, lifeNum, actor->indexInWorld);
        actor->frame = anim->m_numFrames - 1;
    }
}

// The body (3D model) must exist
static void checkBody(int lifeNum, tObject* actor)
{
    if (actor->bodyNum < 0)
        return;

    if (actor->bodyNum > 5000 || !HQR_Get(HQ_Bodys, actor->bodyNum))
    {
        printf(LIFE_WARN "Body %d not found (life %d, object %d): object hidden" CON_RESET "\n",
               actor->bodyNum, lifeNum, actor->indexInWorld);
        actor->bodyNum = -1;
    }
}

// Speed and movement mode inside their ranges (negative speed is walking back)
static void checkMovement(int lifeNum, tObject* actor)
{
    if (actor->speed < -200 || actor->speed > 200)
    {
        printf(LIFE_WARN "Speed %d out of range (life %d, object %d): reset" CON_RESET "\n",
               actor->speed, lifeNum, actor->indexInWorld);
        actor->speed = 20;
    }
    if (actor->trackMode < 0 || actor->trackMode > 3)
    {
        printf(LIFE_WARN "Movement mode %d out of range (life %d, object %d): manual" CON_RESET "\n",
               actor->trackMode, lifeNum, actor->indexInWorld);
        actor->trackMode = 1;
    }
    if (actor->speedChange.numSteps < -1)
        actor->speedChange.numSteps = 0;
}

void checkObjectAfterLifeOpcode(int lifeNum, int opcode, tObject* actor)
{
    if (!actor)
        return;

    switch (opcode)
    {
    case LM_ANIM_ONCE:
    case LM_ANIM_REPEAT:
    case LM_ANIM_ALL_ONCE:
        checkAnimation(lifeNum, actor);
        break;
    case LM_BODY:
        checkBody(lifeNum, actor);
        break;
    case LM_DO_MOVE:
    case LM_MOVE:
        checkBody(lifeNum, actor);
        checkMovement(lifeNum, actor);
        break;
    default:
        break;
    }
}
