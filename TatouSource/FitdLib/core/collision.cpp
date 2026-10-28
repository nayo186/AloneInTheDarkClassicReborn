///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Collisions, camera switching and thrown objects (split from main.cpp)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
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

#include <array>
#include <filesystem>
#include <string>
#include <fstream>


void walkStep(int angle1, int angle2, int angle3)
{
    Rotate(angle3, angle1, angle2, &animMoveZ, &animMoveX);
}

void addActorToBgInscrust(int actorIdx)
{
    ListObjets[actorIdx].objectType |= AF_BOXIFY + AF_DRAWABLE;
    ListObjets[actorIdx].objectType &= ~AF_ANIMATED;

    //FlagRefreshAux2 = 1;
}

int CubeIntersect(ZVStruct* zvPtr1, ZVStruct* zvPtr2)
{
    if (zvPtr1->ZVX1 >= zvPtr2->ZVX2)
        return 0;

    if (zvPtr2->ZVX1 >= zvPtr1->ZVX2)
        return 0;

    if (zvPtr1->ZVY1 >= zvPtr2->ZVY2)
        return 0;

    if (zvPtr2->ZVY1 >= zvPtr1->ZVY2)
        return 0;

    if (zvPtr1->ZVZ1 >= zvPtr2->ZVZ2)
        return 0;

    if (zvPtr2->ZVZ1 >= zvPtr1->ZVZ2)
        return 0;

    return 1;
}

void AdjustZV(ZVStruct* zvPtr, int startRoom, int destRoom)
{
    unsigned int Xdif = 10 * (roomDataTable[destRoom].worldX - roomDataTable[startRoom].worldX);
    unsigned int Ydif = 10 * (roomDataTable[destRoom].worldY - roomDataTable[startRoom].worldY);
    unsigned int Zdif = 10 * (roomDataTable[destRoom].worldZ - roomDataTable[startRoom].worldZ);

    zvPtr->ZVX1 -= Xdif;
    zvPtr->ZVX2 -= Xdif;
    zvPtr->ZVY1 += Ydif;
    zvPtr->ZVY2 += Ydif;
    zvPtr->ZVZ1 += Zdif;
    zvPtr->ZVZ2 += Zdif;
}

int CheckObjectCol(int actorIdx, ZVStruct* zvPtr)
{
    int currentCollisionSlot = 0;

    int actorRoom = ListObjets[actorIdx].room;

    for (int i = 0; i < 3; i++)
    {
        currentProcessedActorPtr->COL[i] = -1;
    }

    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        tObject* currentActor = &ListObjets.at(i);

        if (currentActor->indexInWorld == -1)
            continue;
        if (i == actorIdx)
            continue;

        ZVStruct* currentActorZv = &currentActor->zv;

        if (currentActor->room != actorRoom)
        {
            ZVStruct localZv;

            CopyZV(zvPtr, &localZv);

            AdjustZV(&localZv, actorRoom, currentActor->room);

            if (CubeIntersect(&localZv, currentActorZv))
            {
                currentProcessedActorPtr->COL[currentCollisionSlot++] = i;

                if (currentCollisionSlot == 3)
                    return(3);
            }
        }
        else
        {
            if (CubeIntersect(zvPtr, currentActorZv))
            {
                currentProcessedActorPtr->COL[currentCollisionSlot++] = i;

                if (currentCollisionSlot == 3)
                    return(3);
            }
        }
    }

    return(currentCollisionSlot);
}

void take(int objIdx)
{
    tWorldObject* objPtr = &ListWorldObjets[objIdx];

    if (numObjInInventoryTable[currentInventory] >= INVENTORY_SIZE)
        return;

    if (numObjInInventoryTable[currentInventory] == 0)
    {
        inventoryTable[currentInventory][0] = objIdx;
    }
    else
    {
        int i;

        for (i = numObjInInventoryTable[currentInventory]; i > 0; i--)
        {
            inventoryTable[currentInventory][i + 1] = inventoryTable[currentInventory][i];
        }

        inventoryTable[currentInventory][1] = objIdx;
    }

    numObjInInventoryTable[currentInventory]++;

    action = 0x800;

    {
        s16 savedShakingAmplitude = shakingAmplitude;
        s16 savedShakeVar1 = shakeVar1;
        executeFoundLife(objIdx);
        shakingAmplitude = savedShakingAmplitude;
        shakeVar1 = savedShakeVar1;
    }

    if (objPtr->objIndex != -1)
    {
        DeleteObjet(objPtr->objIndex);
    }

    objPtr->foundFlag &= 0xBFFF;
    objPtr->foundFlag |= 0x8000;

    objPtr->room = -1;
    objPtr->stage = -1;
}

void Glisser(int flag)
{
    switch (flag)
    {
    case 1:
    case 2:
    {
        hardColStepZ = 0;
        break;
    }
    case 4:
    case 8:
    {
        hardColStepX = 0;
        break;
    }
    default:
    {
        break;
    }
    }
}

// Area of the X/Z overlap of two zones (0 if they do not overlap)
static long long overlapXZ(ZVStruct* zv, ZVStruct* fixZv)
{
    s32 overlapX = ((zv->ZVX2 < fixZv->ZVX2) ? zv->ZVX2 : fixZv->ZVX2) - ((zv->ZVX1 > fixZv->ZVX1) ? zv->ZVX1 : fixZv->ZVX1);
    s32 overlapZ = ((zv->ZVZ2 < fixZv->ZVZ2) ? zv->ZVZ2 : fixZv->ZVZ2) - ((zv->ZVZ1 > fixZv->ZVZ1) ? zv->ZVZ1 : fixZv->ZVZ1);

    if (overlapX <= 0 || overlapZ <= 0)
        return 0;

    return (long long)overlapX * overlapZ;
}

void GereCollision(ZVStruct* oldZv, ZVStruct* animatedZv, ZVStruct* fixZv)
{
    s32 oldpos = 0;
    s32 oldtype;
    s32 halfX;
    s32 halfZ;
    s32 pos;
    s32 type;

    if (oldZv->ZVX2 > fixZv->ZVX1)
    {
        if (fixZv->ZVX2 <= oldZv->ZVX1)
        {
            oldpos = 8; // right
        }
        else {
            oldpos = 0; // center
        }
    }
    else
    {
        oldpos = 4; // left
    }

    if (oldZv->ZVZ2 > fixZv->ZVZ1)
    {
        if (oldZv->ZVZ1 >= fixZv->ZVZ2)
        {
            oldpos |= 2;
        }
        else {
            oldpos |= 0; // center
        }
    }
    else
    {
        oldpos |= 1;
    }

    if ((oldpos == 5) || (oldpos == 9) || (oldpos == 6) || (oldpos == 10))
    {
        oldtype = 2; // corner
    }
    else
    {
        if (oldpos == 0)
        {
            oldtype = 0;

            // Already inside the box: FITD cancelled every move, so an actor left
            // overlapping a wall (e.g. the monster when its window track turns
            // collisions back on) could never get out. Allow moves that do not go
            // deeper into the box (BUGS.md V4).
            if (overlapXZ(animatedZv, fixZv) <= overlapXZ(oldZv, fixZv))
                return;

            hardColStepZ = 0;
            hardColStepX = 0;

            return;
        }
        else
        {
            oldtype = 1; // corner
        }
    }

    halfX = (animatedZv->ZVX1 + animatedZv->ZVX2) / 2;
    halfZ = (animatedZv->ZVZ1 + animatedZv->ZVZ2) / 2;

    if (fixZv->ZVX1 > halfX)
    {
        pos = 4; // left
    }
    else
    {
        if (fixZv->ZVX2 < halfX)
        {
            pos = 0; // center
        }
        else
        {
            pos = 8; // right
        }
    }

    if (fixZv->ZVZ1 > halfZ)
    {
        pos |= 1; // up
    }
    else
    {
        if (fixZv->ZVZ2 < halfZ)
        {
            pos |= 0; // center
        }
        else
        {
            pos |= 2; // bellow
        }
    }

    if ((pos == 5) || (pos == 9) || (pos == 6) || (pos == 10))
    {
        type = 2; // corner
    }
    else
    {
        if (pos == 0)
        {
            type = 0; // center
        }
        else
        {
            type = 1; // border
        }
    }

    if (oldtype == 1) // border
    {
        Glisser(oldpos);
    }
    else
    {
        if ((type == 1) && (pos & oldpos))
        {
            Glisser(pos);
        }
        else {
            if ((pos == oldpos) || (pos + oldpos == 15))
            {
                int Xmod = abs(animatedZv->ZVX1 - oldZv->ZVX1); // recheck
                int Zmod = abs(animatedZv->ZVZ1 - oldZv->ZVZ1);

                if (Xmod > Zmod)
                {
                    hardColStepZ = 0;
                }
                else
                {
                    hardColStepX = 0;
                }
            }
            else
            {
                if ((type == 0) || ((type == 1) && ((pos & oldpos) == 0)))
                {
                    hardColStepZ = 0;
                    hardColStepX = 0;
                }
                else
                {
                    Glisser(oldpos & pos);
                }
            }
        }
    }
}

int AsmCheckListCol(ZVStruct* zvPtr, roomDataStruct* pRoomData)
{
    u16 i;
    int hardColVar = 0;
    hardColStruct* pCurrentEntry = pRoomData->hardColTable.data();

#ifdef FITD_DEBUGGER
    if (debuggerVar_noHardClip)
        return 0;
#endif

    for (i = 0; i < pRoomData->numHardCol; i++)
    {
        if (((pCurrentEntry->zv.ZVX1) < (zvPtr->ZVX2)) && ((zvPtr->ZVX1) < (pCurrentEntry->zv.ZVX2)))
        {
            if (((pCurrentEntry->zv.ZVY1) < (zvPtr->ZVY2)) && ((zvPtr->ZVY1) < (pCurrentEntry->zv.ZVY2)))
            {
                if (((pCurrentEntry->zv.ZVZ1) < (zvPtr->ZVZ2)) && ((zvPtr->ZVZ1) < (pCurrentEntry->zv.ZVZ2)))
                {
                    ASSERT(hardColVar < 10);
                    if (hardColVar < 10)
                    {
                        hardColTable[hardColVar++] = pCurrentEntry;
                    }
                }
            }
        }

        pCurrentEntry++;
    }

    return hardColVar;
}

void menuWaitVSync()
{}

int testCrossProduct(int x1, int z1, int x2, int z2, int x3, int z3, int x4, int z4)
{
    int returnFlag = 0;

    int xAB = x1 - x2;
    int yCD = z3 - z4;
    int xCD = x3 - x4;
    int yAB = z1 - z2;

    int xAC = x1 - x3;
    int yAC = z1 - z3;

    int DotProduct = (xAB * yCD) - (xCD * yAC);

    if (DotProduct)
    {
        int Dda = xAC * yCD - xCD * yAC;
        int Dmu = -xAB * yAC + xAC * yAB;

        if (DotProduct < 0)
        {
            DotProduct = -DotProduct;
            Dda = -Dda;
            Dmu = -Dmu;
        }

        if (Dda >= 0 && Dmu >= 0 && DotProduct >= Dda && DotProduct >= Dmu)
            returnFlag = 1;

    }

    return(returnFlag);
}

int isInPoly(int x1, int x2, int z1, int z2, cameraViewedRoomStruct* pCameraZoneDef)
{
    int xMid = (x1 + x2) / 2;
    int zMid = (z1 + z2) / 2;

    int i;

    for (i = 0; i < pCameraZoneDef->coverZones.size(); i++)
    {
        int j;
        int flag = 0;

        for (j = 0; j < pCameraZoneDef->coverZones[i].numPoints; j++)
        {
            int zoneX1;
            int zoneZ1;
            int zoneX2;
            int zoneZ2;

            zoneX1 = pCameraZoneDef->coverZones[i].pointTable[j].x;
            zoneZ1 = pCameraZoneDef->coverZones[i].pointTable[j].y;
            zoneX2 = pCameraZoneDef->coverZones[i].pointTable[j + 1].x;
            zoneZ2 = pCameraZoneDef->coverZones[i].pointTable[j + 1].y;

            if (testCrossProduct(xMid, zMid, xMid - 10000, zMid, zoneX1, zoneZ1, zoneX2, zoneZ2))
            {
                flag |= 1;
            }

            if (testCrossProduct(xMid, zMid, xMid + 10000, zMid, zoneX1, zoneZ1, zoneX2, zoneZ2))
            {
                flag |= 2;
            }
        }

        if (flag == 3)
        {
            return(1);
        }
    }

    return(0);
}

int findBestCamera(void)
{
    int foundAngle = 32000;
    int foundCamera = -1;

    if (currentCameraTargetActor < 0 || currentCameraTargetActor >= NUM_MAX_OBJECT)
        return -1;

    tObject* actorPtr = &ListObjets[currentCameraTargetActor];

    int x1 = actorPtr->zv.ZVX1 / 10;
    int x2 = actorPtr->zv.ZVX2 / 10;
    int z1 = actorPtr->zv.ZVZ1 / 10;
    int z2 = actorPtr->zv.ZVZ2 / 10;

    int i;

    for (i = 0; i < numCameraInRoom; i++)
    {
        ASSERT(i < NUM_MAX_CAMERA_IN_ROOM);
        if (currentCameraZoneList[i])
            if (isInPoly(x1, x2, z1, z2, currentCameraZoneList[i])) // if in camera zone ?
            {
                // we try to select the best camera that looks behind the player
                int newAngle = actorPtr->beta + (((cameraDataTable[i]->beta) + 0x200) & 0x3FF);

                if (newAngle < 0)
                {
                    newAngle = -newAngle;
                }

                if (newAngle < foundAngle)
                {
                    foundAngle = newAngle;
                    foundCamera = i;
                }
            }
    }

    return(foundCamera);
}

void GereSwitchCamera(void)
{
    int localCurrentCam = NumCamera;
    int newCamera;

    if (NumCamera != -1)
    {
        if (currentCameraTargetActor == -1)
        {
            // TODO: Happens at the start of AITD3, likely a bug
            return;
        }

        tObject* actorPtr;
        int zvx1;
        int zvx2;
        int zvz1;
        int zvz2;

        actorPtr = &ListObjets[currentCameraTargetActor];

        zvx1 = actorPtr->zv.ZVX1 / 10;
        zvx2 = actorPtr->zv.ZVX2 / 10;

        zvz1 = actorPtr->zv.ZVZ1 / 10;
        zvz2 = actorPtr->zv.ZVZ2 / 10;

        if (currentCameraZoneList[NumCamera] && isInPoly(zvx1, zvx2, zvz1, zvz2, currentCameraZoneList[NumCamera])) // is still in current camera zone ?
        {
            return;
        }
    }

#ifdef FITD_DEBUGGER
    //printf("Exited current camera cover zone...\n");
#endif

    newCamera = findBestCamera(); // find new camera

    if (newCamera != -1)
    {
        localCurrentCam = newCamera;
    }

    if (NumCamera != localCurrentCam)
    {
        NewNumCamera = localCurrentCam;
        FlagInitView = 1;
    }

#ifdef FITD_DEBUGGER
    /* if(newCamera == -1)
    {
    printf("No new camera found...\n");
    }*/
#endif
}

bool isPointInZV(int x, int y, int z, ZVStruct* pZV)
{
    if (pZV->ZVX1 <= x && pZV->ZVX2 >= x)
    {
        if (pZV->ZVY1 <= y && pZV->ZVY2 >= y)
        {
            if (pZV->ZVZ1 <= z && pZV->ZVZ2 >= z)
            {
                return(true);
            }
        }
    }

    return false;
}

sceZoneStruct* processActor2Sub(int x, int y, int z, roomDataStruct* pRoomData)
{
    u32 i;
    sceZoneStruct* pCurrentZone;

    pCurrentZone = pRoomData->sceZoneTable.data();

    for (i = 0; i < pRoomData->numSceZone; i++)
    {
        if (pCurrentZone->zv.ZVX1 <= x && pCurrentZone->zv.ZVX2 >= x)
        {
            if (pCurrentZone->zv.ZVY1 <= y && pCurrentZone->zv.ZVY2 >= y)
            {
                if (pCurrentZone->zv.ZVZ1 <= z && pCurrentZone->zv.ZVZ2 >= z)
                {
                    return(pCurrentZone);
                }
            }
        }

        pCurrentZone++;
    }

    return(NULL);

}

void GereDec()
{
    bool onceMore = false;
    bool flagFloorChange = false;
    int zoneIdx = 0;

    do
    {
        onceMore = false;
        if (currentProcessedActorPtr->room < 0 || currentProcessedActorPtr->room >= getNumberOfRoom())
            return;
        roomDataStruct* pRoomData = &roomDataTable[currentProcessedActorPtr->room];
        for (u32 i = 0; i < pRoomData->numSceZone; i++)
        {
            sceZoneStruct* pCurrentZone = &pRoomData->sceZoneTable[i];

            if (isPointInZV(currentProcessedActorPtr->roomX + currentProcessedActorPtr->stepX,
                currentProcessedActorPtr->roomY + currentProcessedActorPtr->stepY,
                currentProcessedActorPtr->roomZ + currentProcessedActorPtr->stepZ,
                &pCurrentZone->zv))
            {
                switch (pCurrentZone->type)
                {
                case 0:
                {
                    int oldRoom = currentProcessedActorPtr->room;

                    currentProcessedActorPtr->room = (short)pCurrentZone->parameter;

                    int x = (roomDataTable[currentProcessedActorPtr->room].worldX - roomDataTable[oldRoom].worldX) * 10;
                    int y = (roomDataTable[currentProcessedActorPtr->room].worldY - roomDataTable[oldRoom].worldY) * 10;
                    int z = (roomDataTable[currentProcessedActorPtr->room].worldZ - roomDataTable[oldRoom].worldZ) * 10;

                    currentProcessedActorPtr->roomX -= x;
                    currentProcessedActorPtr->roomY += y;
                    currentProcessedActorPtr->roomZ += z;

                    currentProcessedActorPtr->zv.ZVX1 -= x;
                    currentProcessedActorPtr->zv.ZVX2 -= x;

                    currentProcessedActorPtr->zv.ZVY1 += y;
                    currentProcessedActorPtr->zv.ZVY2 += y;

                    currentProcessedActorPtr->zv.ZVZ1 += z;
                    currentProcessedActorPtr->zv.ZVZ2 += z;

                    onceMore = true;
                    if (currentProcessedActorIdx == currentCameraTargetActor)
                    {
                        FlagChangeSalle = 1;
                        NewNumSalle = (short)pCurrentZone->parameter;
                        if (g_gameId > AITD1)
                        {
                            ChangeSalle(NewNumSalle);

                            // Hack: this wasn't in the original game, but is required here, or else we would use the current camera index in the new room and potentially go out of bound
                            InitView();
                        }
                    }
                    else
                    {
                        FlagGenereAffList = 1;
                    }

                    startChrono(&currentProcessedActorPtr->ROOM_CHRONO);


                    break;
                }
                case 8:
                {
                    assert(g_gameId != AITD1);
                    if (g_gameId != AITD1)
                    {
                        currentProcessedActorPtr->hardMat = (short)pCurrentZone->parameter;
                    }
                    break;
                }
                case 9: // Scenar
                {
                    if ((g_gameId == AITD1) || !flagFloorChange)
                    {
                        currentProcessedActorPtr->HARD_DEC = (short)pCurrentZone->parameter;
                    }
                    break;
                }
                case 10: // stage
                {
                    int idxW = currentProcessedActorPtr->indexInWorld;
                    if (idxW < 0 || idxW >= (int)ListWorldObjets.size())
                        break;

                    int life = ListWorldObjets[idxW].floorLife;

                    if (life == -1)
                        return;

                    currentProcessedActorPtr->life = life;

                    currentProcessedActorPtr->HARD_DEC = (short)pCurrentZone->parameter;
                    flagFloorChange = true;
                    break;
                }
                }

                if (g_gameId == AITD1) // AITD1 stops at the first zone
                    return;
            }
            if (onceMore)
                break;
        }
    } while (onceMore);
}

int checkLineProjectionWithActors(int actorIdx, int X, int Y, int Z, int beta, int room, int param)
{
    ZVStruct localZv;
    int foundFlag = -2;
    int tempX;
    int tempZ;

    localZv.ZVX1 = X - param;
    localZv.ZVX2 = X + param;
    localZv.ZVY1 = Y - param;
    localZv.ZVY2 = Y + param;
    localZv.ZVZ1 = Z - param;
    localZv.ZVZ2 = Z + param;

    walkStep(param * 2, 0, beta);

    int iterationLimit = 2000;
    while (foundFlag == -2)
    {
        if (--iterationLimit <= 0)
        {
            foundFlag = -1;
            break;
        }

        localZv.ZVX1 += animMoveX;
        localZv.ZVX2 += animMoveX;

        localZv.ZVZ1 += animMoveZ;
        localZv.ZVZ2 += animMoveZ;

        tempX = X;
        tempZ = Z;

        X += animMoveX;
        Z += animMoveZ;

        if (X > 20000 || X < -20000 || Z > 20000 || Z < -20000)
        {
            foundFlag = -1;
            break;
        }

        for (int i = 0; i < ListObjets.size(); i++)
        {
            tObject* actorCheckPtr = &ListObjets[i];
            if (actorCheckPtr->indexInWorld != -1 && i != actorIdx && !(actorCheckPtr->objectType & AF_SPECIAL))
            {
                ZVStruct* zvPtr = &actorCheckPtr->zv;

                if (room != actorCheckPtr->room)
                {
                    ZVStruct localZv2;

                    CopyZV(&localZv, &localZv2);
                    AdjustZV(&localZv2, room, actorCheckPtr->room);

                    if (!CubeIntersect(&localZv2, zvPtr))
                    {
                        continue;
                    }
                }
                else
                {
                    if (!CubeIntersect(&localZv, zvPtr))
                    {
                        continue;
                    }
                }

                foundFlag = i;
                break;
            }
        }

        if (foundFlag == -2 && AsmCheckListCol(&localZv, &roomDataTable[room]) > 0)
        {
            foundFlag = -1;
        }
    }

    animMoveX = tempX;
    animMoveY = Y;
    animMoveZ = tempZ;

    return(foundFlag);
}

// Moves an object that already exists in the room (it has an actor) to a new
// position. FITD updated currentProcessedActorPtr here instead, i.e. the
// actor running the script (e.g. the vase when it puts the key where it broke),
// so the object stayed where it was and could not be reached. The zone and the
// position are moved the same way setStage() does it.
static void moveExistingObject(tObject* actorPtr, int room, int stage, int x, int y, int z,
    int worldX, int worldY, int worldZ, int alpha, int beta, int gamma)
{
    int dx = x - (actorPtr->roomX + actorPtr->stepX);
    int dy = y - (actorPtr->roomY + actorPtr->stepY);
    int dz = z - (actorPtr->roomZ + actorPtr->stepZ);

    actorPtr->zv.ZVX1 += dx;
    actorPtr->zv.ZVX2 += dx;
    actorPtr->zv.ZVY1 += dy;
    actorPtr->zv.ZVY2 += dy;
    actorPtr->zv.ZVZ1 += dz;
    actorPtr->zv.ZVZ2 += dz;

    actorPtr->roomX = x;
    actorPtr->roomY = y;
    actorPtr->roomZ = z;
    actorPtr->worldX = worldX;
    actorPtr->worldY = worldY;
    actorPtr->worldZ = worldZ;
    actorPtr->stepX = 0;
    actorPtr->stepY = 0;
    actorPtr->stepZ = 0;
    actorPtr->room = room;
    actorPtr->stage = stage;
    actorPtr->alpha = alpha;
    actorPtr->beta = beta;
    actorPtr->gamma = gamma;

    ListWorldObjets[actorPtr->indexInWorld].foundFlag |= 0x4000;
    ListWorldObjets[actorPtr->indexInWorld].flags |= 0x80;
}

void PutAtObjet(int objIdx, int objIdxToPutAt)
{
    tWorldObject* objPtr = &ListWorldObjets[objIdx];
    tWorldObject* objPtrToPutAt = &ListWorldObjets[objIdxToPutAt];

    if (objPtrToPutAt->objIndex != -1)
    {
        tObject* actorToPutAtPtr = &ListObjets[objPtrToPutAt->objIndex];

        DeleteInventoryObjet(objIdx);

        printf(MAIN_TAG "PutAt: world object %d placed at object %d (x=%d y=%d z=%d room=%d)%s\n",
            objIdx, objIdxToPutAt, actorToPutAtPtr->roomX, actorToPutAtPtr->roomY, actorToPutAtPtr->roomZ,
            actorToPutAtPtr->room, objPtr->objIndex != -1 ? " (already in the room, actor moved)" : "");

        if (objPtr->objIndex == -1)
        {
            objPtr->x = actorToPutAtPtr->roomX;
            objPtr->y = actorToPutAtPtr->roomY;
            objPtr->z = actorToPutAtPtr->roomZ;
            objPtr->room = actorToPutAtPtr->room;
            objPtr->stage = actorToPutAtPtr->stage;
            objPtr->alpha = actorToPutAtPtr->alpha;
            objPtr->beta = actorToPutAtPtr->beta;
            objPtr->gamma = actorToPutAtPtr->gamma;

            objPtr->foundFlag |= 0x4000;
            objPtr->flags |= 0x80;

            //      FlagGenereActiveList = 1;
            //      FlagRefreshAux2 = 1;
        }
        else
        {
            moveExistingObject(&ListObjets[objPtr->objIndex], actorToPutAtPtr->room, actorToPutAtPtr->stage,
                actorToPutAtPtr->roomX, actorToPutAtPtr->roomY, actorToPutAtPtr->roomZ,
                actorToPutAtPtr->worldX, actorToPutAtPtr->worldY, actorToPutAtPtr->worldZ,
                actorToPutAtPtr->alpha, actorToPutAtPtr->beta, actorToPutAtPtr->gamma);

            //      FlagGenereActiveList = 1;
            //      FlagRefreshAux2 = 1;
        }

    }
    else
    {
        DeleteInventoryObjet(objIdx);

        if (objPtr->objIndex == -1)
        {
            objPtr->x = objPtrToPutAt->x;
            objPtr->y = objPtrToPutAt->y;
            objPtr->z = objPtrToPutAt->z;
            objPtr->room = objPtrToPutAt->room;
            objPtr->stage = objPtrToPutAt->stage;
            objPtr->alpha = objPtrToPutAt->alpha;
            objPtr->beta = objPtrToPutAt->beta;
            objPtr->gamma = objPtrToPutAt->gamma;

            objPtr->foundFlag |= 0x4000;
            objPtr->flags |= 0x80;

            //      FlagGenereActiveList = 1;
            //      FlagRefreshAux2 = 1;
        }
        else
        {
            // The target has no actor: room coordinates are also used as world ones,
            // as setStage() does
            moveExistingObject(&ListObjets[objPtr->objIndex], objPtrToPutAt->room, objPtrToPutAt->stage,
                objPtrToPutAt->x, objPtrToPutAt->y, objPtrToPutAt->z,
                objPtrToPutAt->x, objPtrToPutAt->y, objPtrToPutAt->z,
                objPtrToPutAt->alpha, objPtrToPutAt->beta, objPtrToPutAt->gamma);

            //      FlagGenereActiveList = 1;
            //      FlagRefreshAux2 = 1;
        }
    }
}

void throwStoppedAt(int x, int z)
{
    int x2;
    int y2;
    int z2;
    int foundPosition;
    int step;

    ZVStruct zvCopy;
    ZVStruct zvLocal;

    sBody* bodyPtr = HQR_Get(HQ_Bodys, currentProcessedActorPtr->bodyNum);

    GiveZVObjet(bodyPtr, &zvLocal);

    printf(MAIN_TAG "Throw: object %d (world %d) stopped at x=%d z=%d room=%d roomY=%d\n",
        currentProcessedActorIdx, currentProcessedActorPtr->indexInWorld, x, z,
        currentProcessedActorPtr->room, currentProcessedActorPtr->roomY);

    x2 = x;
    y2 = (currentProcessedActorPtr->roomY / 2000) * 2000;
    z2 = z;

    foundPosition = 0;
    step = 0;

    while (!foundPosition)
    {
        if (step > 20000)
        {
            foundPosition = 1;
            break;
        }

        // Search backwards, towards the thrower: the object moved with
        // walkStep(0, +speed, beta), so back is beta + 0x200 with a positive step
        // (FITD searched forwards, behind the wall it hit; BUGS.md V13)
        walkStep(0, step, currentProcessedActorPtr->beta + 0x200);
        CopyZV(&zvLocal, &zvCopy);

        x2 = x + animMoveX;
        z2 = z + animMoveZ;

        zvCopy.ZVX1 += x2;
        zvCopy.ZVX2 += x2;

        zvCopy.ZVY1 += y2;
        zvCopy.ZVY2 += y2;

        zvCopy.ZVZ1 += z2;
        zvCopy.ZVZ2 += z2;

        if (!AsmCheckListCol(&zvCopy, &roomDataTable[currentProcessedActorPtr->room]))
        {
            foundPosition = 1;
        }

        if (foundPosition)
        {
            if (y2 < -500)
            {
                zvCopy.ZVY1 += 100; // is the object reachable ? (100 is Carnby height. If hard col at Y + 100, carnby can't reach that spot)
                zvCopy.ZVY2 += 100;

                if (!AsmCheckListCol(&zvCopy, &roomDataTable[currentProcessedActorPtr->room]))
                {
                    y2 += 2000;
                    foundPosition = 0;
                }
                else
                {
                    zvCopy.ZVY1 -= 100;
                    zvCopy.ZVY2 -= 100;
                }
            }
        }
        else
        {
            step += 100;
        }
    }

    printf(MAIN_TAG "Throw: placed at x=%d y=%d z=%d (searched %d units back from the stop point%s)\n",
        x2, y2, z2, step, step > 20000 ? ", gave up: no free spot found" : "");

    currentProcessedActorPtr->worldX = x2;
    currentProcessedActorPtr->roomX = x2;
    currentProcessedActorPtr->worldY = y2;
    currentProcessedActorPtr->roomY = y2;
    currentProcessedActorPtr->worldZ = z2;
    currentProcessedActorPtr->roomZ = z2;

    currentProcessedActorPtr->stepX = 0;
    currentProcessedActorPtr->stepZ = 0;

    currentProcessedActorPtr->animActionType = 0;
    currentProcessedActorPtr->speed = 0;
    currentProcessedActorPtr->gamma = 0;

    GiveZVObjet(bodyPtr, &currentProcessedActorPtr->zv);

    currentProcessedActorPtr->zv.ZVX1 += x2;
    currentProcessedActorPtr->zv.ZVX2 += x2;
    currentProcessedActorPtr->zv.ZVY1 += y2;
    currentProcessedActorPtr->zv.ZVY2 += y2;
    currentProcessedActorPtr->zv.ZVZ1 += z2;
    currentProcessedActorPtr->zv.ZVZ2 += z2;

    ListWorldObjets[currentProcessedActorPtr->indexInWorld].foundFlag |= 0x4000;
    ListWorldObjets[currentProcessedActorPtr->indexInWorld].foundFlag &= 0xEFFF;

    addActorToBgInscrust(currentProcessedActorIdx);
}
