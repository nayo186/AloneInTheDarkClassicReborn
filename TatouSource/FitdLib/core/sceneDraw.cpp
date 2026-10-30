///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Scene drawing: background overlays, actors and redraw (split from main.cpp)
///////////////////////////////////////////////////////////////////////////////

#include <bx/timer.h>   // before common.h: windows.h min/max macros
#include "common.h"
#include "launchOptions.h"
#include "settings.h"
#include "consoleLog.h"

// Menu sound helper (from systemMenu.cpp)
extern void playMenuSound(const char* soundName);

#include "anim2d.h"
#include "coreHooks.h"
#include "menuMouse.h"

#include "nativeLife.h"
#include "track.h"
#include <math.h>
#include <SDL.h>

#ifndef AITD_UE4
#include "bgfxGlue.h"
#include <bgfx/bgfx.h>
#endif

#include "settings.h"
#include "jobSystemInit.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <fstream>



int isBgOverlayRequired(int X1, int X2, int Z1, int Z2, char* data, int param)
{
    int i;
    for (i = 0; i < param; i++)
    {
        ////////////////////////////////////// DEBUG
        //  drawOverlayZone(data, 80);
        /////////////////////////////////////

        int zoneX1 = *(s16*)(data);
        int zoneZ1 = *(s16*)(data + 2);
        int zoneX2 = *(s16*)(data + 4);
        int zoneZ2 = *(s16*)(data + 6);

        if (X1 >= zoneX1 && Z1 >= zoneZ1 && X2 <= zoneX2 && Z2 <= zoneZ2)
        {
            return(1);
        }

        data += 0x8;
    }

    return(0);
}

// debug.logDrawOrder: this frame is written to tatou.log ([DRAW] lines,
// tools/readLog.py reads them)
static bool s_logDrawFrame = false;

void drawBgOverlay(tObject* actorPtr)
{
    char* data;
    char* data2;

    int numOverlayZone;

    actorPtr->screenXMin = BBox3D1;
    actorPtr->screenYMin = BBox3D2;
    actorPtr->screenXMax = BBox3D3;
    actorPtr->screenYMax = BBox3D4;

    //if(actorPtr->trackMode != 1)
    //	return;

    SetClip(BBox3D1, BBox3D2, BBox3D3, BBox3D4);

    cameraDataStruct* pCamera = cameraDataTable[NumCamera];

    // look for the correct room data of that camera
    cameraViewedRoomStruct* pcameraViewedRoomData = NULL;
    int relativeCameraIndex = -1;
    for (int i = 0; i < pCamera->numViewedRooms; i++)
    {
        if (pCamera->viewedRoomTable[i].viewedRoomIdx == actorPtr->room)
        {
            pcameraViewedRoomData = &pCamera->viewedRoomTable[i];
            relativeCameraIndex = i;
            break;
        }
    }
    if (pcameraViewedRoomData == NULL)
        return;

    if (g_gameId == AITD1)
    {
        data2 = room_PtrCamera[NumCamera] + pcameraViewedRoomData->offsetToMask;
        data = data2;
        data += 2;

        numOverlayZone = *(s16*)(data2);

        for (int i = 0; i < numOverlayZone; i++)
        {
            int numOverlay;
            char* src = data2 + *(u16*)(data + 2);

            if (isBgOverlayRequired(actorPtr->zv.ZVX1 / 10, actorPtr->zv.ZVX2 / 10,
                actorPtr->zv.ZVZ1 / 10, actorPtr->zv.ZVZ2 / 10,
                data + 4,
                *(s16*)(data)))
            {
                if (s_logDrawFrame)
                    printf("[DRAW]     mask %d (room %d) clipped to %d,%d-%d,%d\n", i, actorPtr->room, clipLeft, clipTop, clipRight, clipBottom);
                osystem_setClip(clipLeft, clipTop, clipRight, clipBottom);
                osystem_drawMask(relativeCameraIndex, i);
                osystem_clearClip();

                /*
                int j;
                numOverlay = *(s16*)src;
                src += 2;

                for(j=0;j<numOverlay;j++)
                {
                int param = *(s16*)(src);
                src+=2;

                memcpy(cameraBuffer, src, param*4);

                src+=param*4;

                drawBgOverlaySub2(param);
                }
                */

                //      blitOverlay(src);
            }

            numOverlay = *(s16*)(data);
            data += 2;
            data += ((numOverlay * 4) + 1) * 2;
        }
    }
    else
    {
        for (int i = 0; i < pcameraViewedRoomData->masks.size(); i++)
        {
            cameraMaskStruct* pMaskZones = &pcameraViewedRoomData->masks[i];

            for (int j = 0; j < pMaskZones->numTestRect; j++)
            {
                rectTestStruct* pRect = &pMaskZones->rectTests[j];

                int actorX1 = actorPtr->zv.ZVX1 / 10;
                int actorX2 = actorPtr->zv.ZVX2 / 10;
                int actorZ1 = actorPtr->zv.ZVZ1 / 10;
                int actorZ2 = actorPtr->zv.ZVZ2 / 10;

                if (actorX1 >= pRect->zoneX1 && actorZ1 >= pRect->zoneZ1 && actorX2 <= pRect->zoneX2 && actorZ2 <= pRect->zoneZ2)
                {
                    osystem_setClip(clipLeft, clipTop, clipRight, clipBottom);
                    osystem_drawMask(relativeCameraIndex, i);
                    osystem_clearClip();
                    break;
                }
            }
        }
    }

    SetClip(0, 0, 319, 199);
}

// ---- Cigar smoke (AITD1, special object 4) -----------------------------------
// A cloud around the cigars that swells to fill most of the room and shrinks
// back, turning, as in the original game: everything it covers turns grey (the
// screen effects' last pass, effectsComposite_ps, osystem_setSmokeArea). While
// the player touches it, FOG_FLAG is 2 for one pass of the scripts:
// the cigars' script (life 427) makes the player cough and lose 5 points; then
// 1 again, at most once per kSmokeCoughTicks and never in the middle of a
// one-shot animation (pouring the jug). The original engine did this; FITD
// never had it (BUGS.md C41).

static const int kSmokeMinRadius = 300;        // around the cigars
static const int kSmokeMaxRadius = 1400;       // most of their room
static const int kSmokePeriodTicks = 8 * 60;   // small - large - small
static const int kSmokeCoughTicks = 2 * 60;    // between two coughs

// The shape of the smoke as effectsComposite_ps draws it (same noise, same
// constants, same clock): below 0.95 there is visible smoke
static float smokeHash(float x, float y)
{
    float p3x = x * 0.1031f, p3y = y * 0.1031f, p3z = x * 0.1031f;
    p3x -= floorf(p3x); p3y -= floorf(p3y); p3z -= floorf(p3z);
    const float d = p3x * (p3y + 33.33f) + p3y * (p3z + 33.33f) + p3z * (p3x + 33.33f);
    p3x += d; p3y += d; p3z += d;
    const float v = (p3x + p3y) * p3z;
    return v - floorf(v);
}

static float smokeValueNoise(float x, float y)
{
    const float cx = floorf(x), cy = floorf(y);
    float fx = x - cx, fy = y - cy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    const float a = smokeHash(cx, cy), b = smokeHash(cx + 1.0f, cy);
    const float c = smokeHash(cx, cy + 1.0f), d = smokeHash(cx + 1.0f, cy + 1.0f);
    const float top = a + (b - a) * fx, bottom = c + (d - c) * fx;
    return top + (bottom - top) * fy;
}

static float smokeCloudNoise(float x, float y)
{
    return smokeValueNoise(x, y) * 0.5f + smokeValueNoise(x * 2.03f + 17.0f, y * 2.03f + 17.0f) * 0.3f
        + smokeValueNoise(x * 4.01f + 41.0f, y * 4.01f + 41.0f) * 0.2f;
}

// localX/Y: the point in the smoke's ellipse (the unit circle); t: seconds
static float smokeShapeDistance(float localX, float localY, float t)
{
    const float angle = t * 0.25f;
    const float swirlX = localX * cosf(angle) - localY * sinf(angle);
    const float swirlY = localX * sinf(angle) + localY * cosf(angle);
    const float edge = smokeCloudNoise(swirlX * 2.2f + t * 0.03f, swirlY * 2.2f - t * 0.04f);
    return sqrtf(localX * localX + localY * localY) + (edge - 0.5f) * 0.7f;
}

static void AffSmoke(int actorIdx)
{
    tObject* actorPtr = &ListObjets[actorIdx];

    // No box on screen: the smoke is a screen effect, not a model, and must
    // not draw masks. It kept the box of the object drawn before it, and its
    // room's masks covered that object (BUGS.md C42)
    BBox3D1 = 32000;
    BBox3D2 = 32000;
    BBox3D3 = -32000;
    BBox3D4 = -32000;

    // Gone with its cigars (put out with the jug, the script deletes them) or
    // once they are no longer loaded
    const int emitter = actorPtr->trackNumber;
    if (emitter < 0 || emitter >= (int)ListWorldObjets.size() || ListWorldObjets[emitter].objIndex < 0)
    {
        DeleteObjet(actorIdx);
        return;
    }

    actorPtr->numOfFrames += ticksThisFrame;

    // Shown only from its own room: the grey is a screen effect that walls do
    // not hide, so seen through a door it greyed the other room
    if (actorPtr->room != currentRoom)
        return;

    const float phase = (float)(actorPtr->numOfFrames % kSmokePeriodTicks) / (float)kSmokePeriodTicks;
    const float swell = 0.5f - 0.5f * cosf(phase * 6.2831853f);
    const float radius = kSmokeMinRadius + (kSmokeMaxRadius - kSmokeMinRadius) * swell;

    // On screen: an ellipse centred on the cigars, as wide and tall as the box
    // around the cloud (a disc of that radius around them, from their height
    // up to half the radius; Y grows downwards). The centre of that box moved
    // away from the cigars as the cloud grew
    float centerX, centerY;
    if (!projectToScreen((float)actorPtr->worldX, (float)actorPtr->worldY, (float)actorPtr->worldZ, &centerX, &centerY))
        return;
    float minX = 32000.f, minY = 32000.f, maxX = -32000.f, maxY = -32000.f;
    for (int corner = 0; corner < 8; corner++)
    {
        float sx, sy;
        if (!projectToScreen(actorPtr->worldX + ((corner & 1) ? radius : -radius),
                actorPtr->worldY - ((corner & 2) ? radius * 0.5f : 0.0f),
                actorPtr->worldZ + ((corner & 4) ? radius : -radius), &sx, &sy))
            continue;
        minX = (std::min)(minX, sx);
        maxX = (std::max)(maxX, sx);
        minY = (std::min)(minY, sy);
        maxY = (std::max)(maxY, sy);
    }
    if (maxX <= minX || maxY <= minY)
        return;
    const float radiusX = (maxX - minX) * 0.5f, radiusY = (maxY - minY) * 0.5f;
    osystem_setSmokeArea(centerX, centerY, radiusX, radiusY);

    // FOG_FLAG went back to 1 at the start of the frame (AllRedraw); the
    // player touching the cloud (in the room and on screen) sets
    // it to 2 again, after the cough, unless a one-shot animation is playing
    s16& fog = CVars[getCVarsIdx(FOG_FLAG)];
    static unsigned int s_lastCough = 0;
    if (currentCameraTargetActor < 0 || fog != 1 || timer - s_lastCough < (unsigned int)kSmokeCoughTicks)
        return;
    const tObject* player = &ListObjets[currentCameraTargetActor];
    if (player->indexInWorld < 0 || player->room != actorPtr->room)
        return;
    const bool busy = !(player->animType & ANIM_REPEAT) || (player->newAnim >= 0 && !(player->newAnimType & ANIM_REPEAT));
    if (busy)
        return;
    const float dx = (float)(player->roomX + player->stepX - actorPtr->roomX);
    const float dz = (float)(player->roomZ + player->stepZ - actorPtr->roomZ);
    if (dx * dx + dz * dz >= radius * radius)
        return;
    // Any visible smoke touching the player's legs or chest: up to the outer
    // edge of the outer band (0.95 in effectsComposite_ps)
    const double seconds = fmod((double)bx::getHPCounter() / (double)bx::getHPFrequency(), 10000.0);
    for (const int height : { 200, 1000 })
    {
        float playerX, playerY;
        if (!projectToScreen((float)(player->worldX + player->stepX), (float)(player->worldY + player->stepY - height),
                (float)(player->worldZ + player->stepZ), &playerX, &playerY))
            continue;
        if (smokeShapeDistance((playerX - centerX) / radiusX, (playerY - centerY) / radiusY, (float)seconds) < 0.95f)
        {
            fog = 2;
            s_lastCough = timer;
            return;
        }
    }
}

void AffSpecialObjet(int actorIdx) // draw flow
{
    tObject* actorPtr = &ListObjets[actorIdx];

    int mode = actorPtr->ANIM;

    if (mode == 4)
    {
        AffSmoke(actorIdx);
        return;
    }

    // Lifetime and particle motion advance once per tick of the game clock,
    // as at 60 frames per second (they advanced once per frame, BUGS.md V19)
    const int ticks = ticksThisFrame;
    actorPtr->numOfFrames += ticks;
    if (actorPtr->END_FRAME > 0 && actorPtr->numOfFrames >= actorPtr->END_FRAME)
    {
        DeleteObjet(actorIdx);
        return;
    }

    // Guard against division by zero when END_FRAME is not set
    if (actorPtr->END_FRAME <= 0)
    {
        DeleteObjet(actorIdx);
        return;
    }

    // modes 2 (impact) and 3 (flash) have no particle data, draw a simple point
    if (mode == 2 || mode == 3)
    {
        float wx = (float)actorPtr->worldX;
        float wy = (float)actorPtr->worldY;
        float wz = (float)actorPtr->worldZ;

        wx -= translateX;
        wy -= translateY;
        wz -= translateZ;

        transformPoint(&wx, &wy, &wz);

        float depth = wz + cameraPerspective;
        if (depth <= 50)
            return;

        float sx = ((wx * cameraFovX) / depth) + cameraCenterX;
        float sy = ((wy * cameraFovY) / depth) + cameraCenterY;

        // fade size over lifetime
        float lifeRatio = 1.0f - (float)actorPtr->numOfFrames / (float)actorPtr->END_FRAME;
        float pointSize = (mode == 3) ? 65.f * lifeRatio : 35.f * lifeRatio;
        float transformedSize = (pointSize * (float)cameraFovX) / depth;
        u8 color = (mode == 3) ? 0xFF : 0x90;

        if (transformedSize > 0.1f)
            osystem_drawPoint(sx, sy, depth, color, 0, transformedSize);
        osystem_flushPendingPrimitives();
        return;
    }

    // modes 0 (evaporate/bubble) and 1 (blood) have particle data in HQ_Memory
    char* data = HQ_PtrMalloc(HQ_Memory, actorPtr->frame);
    if (!data)
    {
        DeleteObjet(actorIdx);
        return;
    }

    s16 color = *(s16*)(data);
    s16 numPoints = *(s16*)(data + 2);
    s16* positions = (s16*)(data + 4);

    if (mode == 0) color = 0x5A; // evaporate: purple
    if (mode == 1) color = 0x28; // blood: bright red

    // animate particles: update positions each tick
    for (int tick = 0; tick < ticks; tick++)
    for (int i = 0; i < numPoints; i++)
    {
        switch (mode)
        {
        case 0: // evaporate: particles rise upward and spread outward
            positions[i * 3 + 1] -= 3; // rise (Y is inverted in AITD)
            positions[i * 3 + 0] += (rand() % 3) - 1; // slight X drift
            positions[i * 3 + 2] += (rand() % 3) - 1; // slight Z drift
            break;
        case 1: // blood: splatter outward from center with gravity
        {
            int px = positions[i * 3 + 0];
            int pz = positions[i * 3 + 2];
            // Push outward fast from center based on current position
            int spreadX = (px > 0) ? (rand() % 8) + 3 : -((rand() % 8) + 3);
            int spreadZ = (pz > 0) ? (rand() % 8) + 3 : -((rand() % 8) + 3);
            positions[i * 3 + 0] += spreadX + (rand() % 5) - 2;
            positions[i * 3 + 1] += 6; // gravity pull
            positions[i * 3 + 2] += spreadZ + (rand() % 5) - 2;
            break;
        }
        }
    }

    float baseX = (float)actorPtr->worldX;
    float baseY = (float)actorPtr->worldY;
    float baseZ = (float)actorPtr->worldZ;

    BBox3D1 = 32000;
    BBox3D2 = 32000;
    BBox3D3 = -32000;
    BBox3D4 = -32000;

    float lifeRatio = 1.0f - (float)actorPtr->numOfFrames / (float)actorPtr->END_FRAME;
    float baseSize = (mode == 1) ? 55.f : 65.f;
    float pointSize = baseSize * lifeRatio;
    if (pointSize < 1.f)
        pointSize = 1.f;

    for (int i = 0; i < numPoints; i++)
    {
        float wx = baseX + (float)positions[i * 3 + 0];
        float wy = baseY + (float)positions[i * 3 + 1];
        float wz = baseZ + (float)positions[i * 3 + 2];

        wx -= translateX;
        wy -= translateY;
        wz -= translateZ;

        transformPoint(&wx, &wy, &wz);

        float depth = wz + cameraPerspective;
        if (depth <= 50)
            continue;

        float sx = ((wx * cameraFovX) / depth) + cameraCenterX;
        float sy = ((wy * cameraFovY) / depth) + cameraCenterY;

        if (sx < BBox3D1) BBox3D1 = (int)sx;
        if (sx > BBox3D3) BBox3D3 = (int)sx;
        if (sy < BBox3D2) BBox3D2 = (int)sy;
        if (sy > BBox3D4) BBox3D4 = (int)sy;

        float transformedSize = (pointSize * (float)cameraFovX) / depth;
        osystem_drawPoint(sx, sy, depth, (u8)color, 0, transformedSize);
    }

    osystem_flushPendingPrimitives();
}

void getHotPoint(int hotPointIdx, sBody* bodyPtr, point3dStruct* hotPoint)
{
    hotPoint->x = 0;
    hotPoint->y = 0;
    hotPoint->z = 0;

    if (bodyPtr->m_flags & 2)
    {
        ASSERT(hotPointIdx < bodyPtr->m_groups.size());

        if (hotPointIdx < bodyPtr->m_groups.size())
        {
            auto& group = bodyPtr->m_groups[hotPointIdx];

            int pointIdx = group.m_baseVertices; // first point
            *hotPoint = pointBuffer[pointIdx];
        }
    }
}

void AllRedraw(int flagFlip)
{
    uiLayer.fill(0);

    // The cigar smoke's FOG_FLAG 2 lasts one pass of the scripts, even if the
    // cloud is not drawn in this frame (AffSmoke sets it again)
    if (g_gameId == AITD1 && CVars[getCVarsIdx(FOG_FLAG)] == 2)
        CVars[getCVarsIdx(FOG_FLAG)] = 1;

    //if(flagFlip == 2)
    {
        if (cameraBackgroundChanged)
        {
            osystem_CopyBlockPhys((unsigned char*)aux, 0, 0, 320, 200);
            // Re-draw the background quad now that the texture has been updated.
            // osystem_startFrame() already submitted a background quad at the
            // start of the frame using the old/stale texture, so re-submit it
            // to prevent a single-frame pop-in (e.g. returning from the pause menu).
            osystem_drawBackground();
            cameraBackgroundChanged = false;
        }
    }

    if (flagFlip == 0)
    {
        //ClsAuxLog();
    }
    else
    {
        NbPhysBoxs = 0;
        FastCopyScreen(aux2, logicalScreen);
    }

    SetClip(0, 0, 319, 199);
    NbLogBoxs = 0;

    // Safety: If the object draw list is empty, force regeneration (prevents 3D objects from disappearing)
    if (NbAffObjets == 0 && !lightOff) {
        GenereActiveList();
        GenereAffList();
    }

    // Painter's order (Original mode): Index is sorted far to near
    // (sortActorList); each object gets a depth slice so nearer objects stay
    // in front, as the original drew them (BUGS.md V21)
    const int numDrawn = (NbAffObjets + NbAnim2D < (int)Index.size()) ? NbAffObjets + NbAnim2D : (int)Index.size();
    // debug.logDrawOrder: the drawing order and masks, once a second
    {
        static unsigned int s_lastDrawLog = 0;
        s_logDrawFrame = g_settings.debug.logDrawOrder && timer - s_lastDrawLog >= 60;
        if (s_logDrawFrame)
        {
            s_lastDrawLog = timer;
            printf("[DRAW] frame: floor %d, camera %d, room %d, %d objects (far to near)\n", g_currentFloor, NumCamera, currentRoom, numDrawn);
        }
    }

    for (int i = 0; i < NbAffObjets + NbAnim2D && i < (int)Index.size(); i++)
    {
        int currentDrawActor = Index[i];
        osystem_setDepthSlice(numDrawn - 1 - i);
        if (currentDrawActor & 0x8000) {
            if (s_logDrawFrame)
                printf("[DRAW]   #%d 2D animation %d\n", i, currentDrawActor & 0x7FFF);
            // Anim2d
            SetClip(0, 0, 319, 199);
            int num = currentDrawActor & 0x7FFF;
            AffHyb(TabAnim2d[num].pAnim->id, 0, 0, PtrAnim2D);

            // TODO: Clipping stuff
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        }
        else {
            tObject* actorPtr = &ListObjets[currentDrawActor];

            // this is commented out to draw actors backed into the background
            //if(actorPtr->_flags & (AF_ANIMATED + AF_DRAWABLE + AF_SPECIAL))
            {
                actorPtr->objectType &= ~AF_DRAWABLE;

                if (actorPtr->objectType & AF_SPECIAL)
                {
                    AffSpecialObjet(currentDrawActor);
                }
                else
                {
                    if (actorPtr->objectType & AF_OBJ_2D) {
                        sHybrid* pHybrid = HQR_Get(HQ_Hybrides, actorPtr->ANIM);
                        if (pHybrid && actorPtr->bodyNum >= 0 && actorPtr->bodyNum < (int)pHybrid->animations.size()) {
                            auto& anim = pHybrid->animations[actorPtr->bodyNum];
                            if (actorPtr->frame >= 0 && actorPtr->frame < (int)anim.anims.size()) {
                                s16 numHybrid = anim.anims[actorPtr->frame].id;
                                AffHyb(numHybrid, 0, 0, pHybrid);
                            }
                        }

                        // TODO: bounding volume
                    }
                    else
                    {
                        sBody* bodyPtr = HQR_Get(HQ_Bodys, actorPtr->bodyNum);
                        if (!bodyPtr) continue;

                        // Where it is drawn: its position, unless a listener
                        // corrects it (object editor's visual fixes)
                        int drawX = actorPtr->worldX + actorPtr->stepX;
                        int drawY = actorPtr->worldY + actorPtr->stepY;
                        int drawZ = actorPtr->worldZ + actorPtr->stepZ;
                        int drawAlpha = actorPtr->alpha, drawBeta = actorPtr->beta, drawGamma = actorPtr->gamma;
                        ObjectSize drawSize;
                        if (g_onObjectDraw && !g_onObjectDraw(currentDrawActor, &drawX, &drawY, &drawZ, &drawAlpha, &drawBeta, &drawGamma, &drawSize))
                            continue;

                        if (HQ_Load)
                        {
                            //          initAnimInBody(actorPtr->FRAME, HQR_Get(listAnim, actorPtr->ANIM), bodyPtr);
                        }

                        // Its own number in the stencil: its masks cover only
                        // it (16 + its place, clear of the 2D masks' 1)
                        osystem_setObjectStencil(16 + (i % 240));

                        setCurrentBodyNum(actorPtr->bodyNum, bodyPtr, HQ_Bodys->string);
                        setNextObjectSize(drawSize);
                        AffObjet(drawX, drawY, drawZ, drawAlpha, drawBeta, drawGamma, bodyPtr);
                        setNextObjectSize(ObjectSize());



                        if (actorPtr->animActionType != 0)
                        {
                            if (actorPtr->hotPointID != -1)
                            {
                                getHotPoint(actorPtr->hotPointID, bodyPtr, &actorPtr->hotPoint);
                            }
                        }

                    }
                }

                if (BBox3D1 < 0)
                    BBox3D1 = 0;
                if (BBox3D3 > 319)
                    BBox3D3 = 319;
                if (BBox3D2 < 0)
                    BBox3D2 = 0;
                if (BBox3D4 > 199)
                    BBox3D4 = 199;

                if (s_logDrawFrame)
                    printf("[DRAW]   #%d actor %d world %d body %d room %d type %#x anim %d box %d,%d-%d,%d\n", i, currentDrawActor,
                        actorPtr->indexInWorld, actorPtr->bodyNum, actorPtr->room, actorPtr->objectType, actorPtr->ANIM,
                        BBox3D1, BBox3D2, BBox3D3, BBox3D4);
                if (BBox3D1 <= 319 && BBox3D2 <= 199 && BBox3D3 >= 0 && BBox3D4 >= 0) // is the character on screen ?
                {
                    if (g_gameId == AITD1)
                    {
                        if (actorPtr->indexInWorld == CVars[getCVarsIdx(LIGHT_OBJECT)])
                        {
                            lightX = (BBox3D3 + BBox3D1) / 2;
                            lightY = (BBox3D4 + BBox3D2) / 2;
                        }
                    }

#ifdef FITD_DEBUGGER
                    if (backgroundMode == backgroundModeEnum_2D)
#endif
                    {
                        //if(g_gameId == AITD1)
                        drawBgOverlay(actorPtr);
                    }
                    osystem_setObjectStencil(0);
                    //addToRedrawBox();
                }
                else
                {
                    actorPtr->screenYMax = -1;
                    actorPtr->screenXMax = -1;
                    actorPtr->screenYMin = -1;
                    actorPtr->screenXMin = -1;
                }
            }
        }
    }
    osystem_setDepthSlice(-1);

    // Dark room (LIGHT, lightOff): the original turned the palette black. The
    // scene is still drawn (hit points, positions) and then covered with
    // black; the messages and the menus stay visible (BUGS.md V23)
    if (lightOff || roomAssumedDark())
        osystem_drawDarkness();

#ifdef FITD_DEBUGGER
    {
        if (backgroundMode == backgroundModeEnum_3D)
        {
            for (int i = 0; i < NUM_MAX_OBJECT; i++)
            {
                tObject* actorPtr = &ListObjets[i];

                if (actorPtr->indexInWorld != -1)
                {
                    drawZv(actorPtr);
                }
            }
        }

        /////////////////////////////////////
        for (int i = 0; i < getNumberOfRoom(); i++)
        {
            if (hardColDisplayMode != displayNone) {
                drawHardCol(i);
            }

            if (sceColDisplayMode != displayNone) {
                drawSceZone(i);
            }
        }

    }
#endif

    if (drawTextOverlay())
    {
        //addToRedrawBox();
    }

    // Fade in after a redraw or a change of light, also in a dark room (the
    // black scene with its message): a fade out before it, e.g. closing a
    // book, left the screen black, menus included (BUGS.md V23)
    if (flagFlip)
    {
        if (flagFlip == 2 || lightVar2)
        {
            osystem_flip(NULL);
            FadeInPhys(0x10, 0);
            lightVar2 = 0;
        }
    }

    //    osystem_stopFrame();

    //	osystem_flip(NULL);

    flagRedraw = 0;
}
