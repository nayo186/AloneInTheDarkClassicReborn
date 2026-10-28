///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Actor lists, view setup and object removal (split from main.cpp)
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


void DeleteObjet(int index) // remove actor
{
    tObject* actorPtr = &ListObjets[index];

    if (actorPtr->indexInWorld == -2) // flow
    {
        actorPtr->indexInWorld = -1;

        if (actorPtr->ANIM == 4)
        {
            CVars[getCVarsIdx(FOG_FLAG)] = 0;
        }

        HQ_Free_Malloc(HQ_Memory, actorPtr->frame);
    }
    else
    {
        if (actorPtr->indexInWorld >= 0)
        {
            tWorldObject* objectPtr = &ListWorldObjets[actorPtr->indexInWorld];

            objectPtr->objIndex = -1;
            actorPtr->indexInWorld = -1;

            objectPtr->body = actorPtr->bodyNum;
            objectPtr->anim = actorPtr->ANIM;
            objectPtr->frame = actorPtr->frame;
            objectPtr->animType = actorPtr->animType;
            objectPtr->animInfo = actorPtr->animInfo;
            objectPtr->flags = actorPtr->objectType & ~AF_BOXIFY;
            objectPtr->flags |= AF_SPECIAL * actorPtr->dynFlags; // ugly hack, need rewrite
            objectPtr->life = actorPtr->life;
            objectPtr->lifeMode = actorPtr->lifeMode;
            objectPtr->trackMode = actorPtr->trackMode;

            if (objectPtr->trackMode)
            {
                objectPtr->trackNumber = actorPtr->trackNumber;
                objectPtr->positionInTrack = actorPtr->positionInTrack;
                if (g_gameId != AITD1)
                {
                    objectPtr->mark = actorPtr->MARK;
                }
            }

            objectPtr->x = actorPtr->roomX + actorPtr->stepX;
            objectPtr->y = actorPtr->roomY + actorPtr->stepY;
            objectPtr->z = actorPtr->roomZ + actorPtr->stepZ;

            objectPtr->alpha = actorPtr->alpha;
            objectPtr->beta = actorPtr->beta;
            objectPtr->gamma = actorPtr->gamma;

            objectPtr->stage = actorPtr->stage;
            objectPtr->room = actorPtr->room;

            FlagGenereAffList = 1;
        }
    }
}

bool pointRotateEnable = true;

int pointRotateCosX;
int pointRotateSinX;
int pointRotateCosY;
int pointRotateSinY;
int pointRotateCosZ;
int pointRotateSinZ;

void setupPointRotate(int alpha, int beta, int gamma)
{
    pointRotateEnable = true;

    pointRotateCosX = cosTable[alpha & 0x3FF];
    pointRotateSinX = cosTable[((alpha & 0x3FF) + 0x100) & 0x3FF];

    pointRotateCosY = cosTable[beta & 0x3FF];
    pointRotateSinY = cosTable[((beta & 0x3FF) + 0x100) & 0x3FF];

    pointRotateCosZ = cosTable[gamma & 0x3FF];
    pointRotateSinZ = cosTable[((gamma & 0x3FF) + 0x100) & 0x3FF];
}

void pointRotate(int x, int y, int z, int* destX, int* destY, int* destZ)
{
    if (pointRotateEnable)
    {
        {
            int tempX = x;
            int tempY = y;
            x = ((((tempX * pointRotateSinZ) - (tempY * pointRotateCosZ))) >> 16) << 1;
            y = ((((tempX * pointRotateCosZ) + (tempY * pointRotateSinZ))) >> 16) << 1;
        }

        {
            int tempX = x;
            int tempZ = z;

            x = ((((tempX * pointRotateSinY) - (tempZ * pointRotateCosY))) >> 16) << 1;
            z = ((((tempX * pointRotateCosY) + (tempZ * pointRotateSinY))) >> 16) << 1;
        }

        {
            int tempY = y;
            int tempZ = z;
            y = ((((tempY * pointRotateSinX) - (tempZ * pointRotateCosX))) >> 16) << 1;
            z = ((((tempY * pointRotateCosX) + (tempZ * pointRotateSinX))) >> 16) << 1;
        }

        *destX = x;
        *destY = y;
        *destZ = z;
    }
}

void CopyZV(ZVStruct* source, ZVStruct* dest)
{
    memcpy(dest, source, sizeof(ZVStruct));
}

void RefreshAux2(void)
{
    FastCopyScreen(aux, aux2);

    //TODO: implementer la suite
}

void InitDeplacement(int trackMode, int trackNumber)
{
    currentProcessedActorPtr->trackMode = trackMode;

    switch (trackMode)
    {
    case 2:
    {
        currentProcessedActorPtr->trackNumber = trackNumber;
        currentProcessedActorPtr->MARK = -1;
        // Reset speed interpolator when switching to follow mode to ensure movement starts properly
        currentProcessedActorPtr->speedChange.numSteps = 0;
        break;
    }
    case 3:
    {
        currentProcessedActorPtr->trackNumber = trackNumber;
        currentProcessedActorPtr->positionInTrack = 0;
        currentProcessedActorPtr->MARK = -1;
        // Reset speed interpolator so there is no leftover state from a prior
        // trackMode (e.g. follow-mode speedChange mid-ramp) that would produce
        // incorrect movement speed at the start of the patrol track.
        currentProcessedActorPtr->speedChange.numSteps = 0;
        break;
    }
    }
}

s16 ObjSalleRel = 0;

int IsInCamera(int roomNumber)
{
    int numZone = cameraDataTable[NumCamera]->numViewedRooms;

    for (int i = 0; i < numZone; i++)
    {
        if (cameraDataTable[NumCamera]->viewedRoomTable[i].viewedRoomIdx == roomNumber)
        {
            ObjSalleRel = i;
            return(1);
        }
    }

    ObjSalleRel = -1;

    return 0;
}

int IsInCamRectTestAITD2(int X, int Z) // TODO: not 100% exact
{
    // if(changeCameraSub1(X,X,Z,Z,&cameraDataTable[currentCamera]->cameraZoneDefTable[cameraVisibilityVar]))
    return 1;

    return 0;
}

void updateAllActorAndObjectsAITD2()
{
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        tObject* pObject = &ListObjets[i];

        if (pObject->indexInWorld == -1)
        {
            continue;
        }

        pObject->lifeMode &= ~4;

        if (pObject->stage == g_currentFloor)
        {
            switch (pObject->lifeMode)
            {
            case 0: // OFF
                break;
            case 1: // STAGE
                continue;
            case 2: // ROOM
                if (pObject->room == currentRoom)
                {
                    continue;
                }
                break;
            case 3: // CAMERA
                if (isInViewList(pObject->room))
                {
                    continue;
                }
                break;
            default:
                break;
            }

            if (pObject->bodyNum != -1)
            {
                if (IsInCamera(pObject->room))
                {
                    if (IsInCamRectTestAITD2(pObject->roomX + pObject->stepX, pObject->roomZ + pObject->stepZ))
                    {
                        pObject->lifeMode |= 4;
                        continue;
                    }
                }
            }
        }
        DeleteObjet(i);
    }

    for (int i = 0; i < maxObjects; i++)
    {
        tWorldObject* currentObject = &ListWorldObjets[i];

        if (currentObject->objIndex != -1)
        {
            if (currentWorldTarget == i)
            {
                currentCameraTargetActor = currentObject->objIndex;
            }
            continue;
        }

        if (currentObject->stage != g_currentFloor)
            continue;

        bool isOk = false;
        switch (currentObject->lifeMode & 3)
        {
        case 0: // OFF
            isOk = false;
            break;
        case 1: // STAGE
            isOk = true;
            break;
        case 3: // CAMERA
            if (!isInViewList(currentObject->room))
            {
                isOk = false;
            }
            else
            {
                isOk = true;
            }
            break;
        case 2: // ROOM
            if (currentObject->room != currentRoom)
            {
                isOk = false;
            }
            else
            {
                isOk = true;
            }
            break;
        }

        if (!isOk) {
            if ((currentObject->body == -1) || (!IsInCamera(currentObject->room)) || (!IsInCamRectTestAITD2(currentObject->x, currentObject->z)))
            {
                continue;
            }
            else {
                currentObject->lifeMode |= 4;
            }
        }

        currentObject->objIndex = InitObjet(currentObject->body, currentObject->typeZV, currentObject->foundName,
            currentObject->flags & ~AF_SPECIAL,
            currentObject->x, currentObject->y, currentObject->z,
            currentObject->stage, currentObject->room,
            currentObject->alpha, currentObject->beta, currentObject->gamma,
            currentObject->anim,
            currentObject->frame, currentObject->animType, currentObject->animInfo);

        if (currentObject->objIndex != -1)
        {
            currentProcessedActorPtr = &ListObjets[currentObject->objIndex];
            currentProcessedActorIdx = currentObject->objIndex;

            if (currentWorldTarget == i)
            {
                currentCameraTargetActor = currentProcessedActorIdx;
            }

            currentProcessedActorPtr->dynFlags = (currentObject->flags & 0x20) / 0x20; // recheck
            currentProcessedActorPtr->life = currentObject->life;
            currentProcessedActorPtr->lifeMode = currentObject->lifeMode;

            currentProcessedActorPtr->indexInWorld = i;

            InitDeplacement(currentObject->trackMode, currentObject->trackNumber);

            currentProcessedActorPtr->positionInTrack = currentObject->positionInTrack;

            if (g_gameId != AITD1)
            {
                currentProcessedActorPtr->MARK = currentObject->mark;
            }

            FlagGenereAffList = 1;
        }
    }
    //FlagGenereActiveList = true;
}


void GenereActiveList()
{
    if (g_gameId > JACK)
    {
        updateAllActorAndObjectsAITD2();
        return;
    }

    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        tObject* currentActor = &ListObjets[i];
        if (currentActor->indexInWorld == -1)
            continue;

        if (currentActor->stage == g_currentFloor)
        {
            if (currentActor->life != -1)
            {
                switch (currentActor->lifeMode)
                {
                case 0: // STAGE
                    continue;
                case 1: // ROOM
                    if (currentActor->room == currentRoom)
                    {
                        continue;
                    }
                    break;
                case 2: // CAMERA
                    if (isInViewList(currentActor->room))
                    {
                        continue;
                    }
                    break;
                default:
                {
                    break;
                }
                }
            }
            else
            {
                if (isInViewList(currentActor->room))
                {
                    continue;
                }
            }
        }

        DeleteObjet(i);
    }

    for (int i = 0; i < maxObjects; i++)
    {
        tWorldObject* currentObject = &ListWorldObjets[i];
        if (currentObject->objIndex != -1) {
            if (currentWorldTarget == i)
            {
                currentCameraTargetActor = currentObject->objIndex;
            }
            continue;
        }

        if (currentObject->stage != g_currentFloor) {
            continue;
        }

        if (currentObject->life != -1)
        {
            switch (currentObject->lifeMode)
            {
            case -1:
                continue;

            case 0: // Stage
                break;

            case 1: // Room
                if (currentObject->room != currentRoom)
                {
                    continue;
                }
                break;
            case 2: // Camera
                if (!isInViewList(currentObject->room))
                {
                    continue;
                }
                break;
            }
        }
        else
        {
            if (!isInViewList(currentObject->room)) {
                continue;
            }
        }

        currentObject->objIndex = InitObjet(currentObject->body, currentObject->typeZV, currentObject->foundName,
            currentObject->flags & ~AF_SPECIAL,
            currentObject->x, currentObject->y, currentObject->z,
            currentObject->stage, currentObject->room,
            currentObject->alpha, currentObject->beta, currentObject->gamma,
            currentObject->anim,
            currentObject->frame, currentObject->animType, currentObject->animInfo);

        if (currentObject->objIndex != -1)
        {
            currentProcessedActorPtr = &ListObjets[currentObject->objIndex];
            currentProcessedActorIdx = currentObject->objIndex;

            if (currentWorldTarget == i)
            {
                currentCameraTargetActor = currentProcessedActorIdx;
            }

            currentProcessedActorPtr->dynFlags = (currentObject->flags & 0x20) / 0x20; // recheck
            currentProcessedActorPtr->life = currentObject->life;
            currentProcessedActorPtr->lifeMode = currentObject->lifeMode;

            currentProcessedActorPtr->indexInWorld = i;

            InitDeplacement(currentObject->trackMode, currentObject->trackNumber);

            currentProcessedActorPtr->positionInTrack = currentObject->positionInTrack;

            FlagGenereAffList = 1;
        }
    }

    //  FlagGenereActiveList = 0;

    //TODO: object update
}

int checkActorInRoom(int room)
{
    int i;

    for (i = 0; i < cameraDataTable[NumCamera]->numViewedRooms; i++)
    {
        if (cameraDataTable[NumCamera]->viewedRoomTable[i].viewedRoomIdx == room)
        {
            return(1);
        }
    }

    return(0);
}

void GenereAffList()
{
    NbAffObjets = 0;

    for (int i = 0; i < ListObjets.size(); i++)
    {
        tObject* actorPtr = &ListObjets[i];
        if (actorPtr->indexInWorld != -1 && actorPtr->bodyNum != -1)
        {
            if (checkActorInRoom(actorPtr->room))
            {
                Index[NbAffObjets] = i;
                if (!(actorPtr->objectType & (AF_SPECIAL | AF_ANIMATED)))
                {
                    actorPtr->objectType |= AF_BOXIFY;
                    //  FlagRefreshAux2 = 1;
                }
                NbAffObjets++;
            }
        }
    }
}

void InitView()
{
    int x;
    int y;
    int z;

    SaveTimerAnim();

    resetAnim2D();

    NumCamera = NewNumCamera;

    assert(NewNumCamera < roomDataTable[currentRoom].numCameraInRoom);

    loadCamera(roomDataTable[currentRoom].cameraIdxTable[NewNumCamera]);
    if (g_gameId >= JACK)
    {
        loadMask(roomDataTable[currentRoom].cameraIdxTable[NewNumCamera]);
    }
    else
    {
        createAITD1Mask();
    }
    cameraBackgroundChanged = true;

    if (g_gameId >= JACK) {
        load2dAnims(roomDataTable[currentRoom].cameraIdxTable[NewNumCamera]);
    }

    cameraDataStruct* pCamera = cameraDataTable[NumCamera];

    SetAngleCamera(pCamera->alpha, pCamera->beta, pCamera->gamma);

#ifdef FITD_DEBUGGER
    if (debuggerVar_topCamera)
        SetAngleCamera(0x100, 0, 0);
#endif

    x = (pCamera->x - roomDataTable[currentRoom].worldX) * 10;
    y = (roomDataTable[currentRoom].worldY - pCamera->y) * 10;
    z = (roomDataTable[currentRoom].worldZ - pCamera->z) * 10;

#ifdef FITD_DEBUGGER
    if (debuggerVar_topCamera)
    {
        if (currentCameraTargetActor != -1)
        {
            x = ListObjets[currentCameraTargetActor].worldX + ListObjets[currentCameraTargetActor].stepX;
            y = debufferVar_topCameraZoom;
            z = ListObjets[currentCameraTargetActor].worldZ + ListObjets[currentCameraTargetActor].stepZ;
        }
    }
#endif
    SetPosCamera(x, y, z); // setup camera position

    SetProjection(160, 100, pCamera->focal1, pCamera->focal2, pCamera->focal3); // setup projection

#ifdef FITD_DEBUGGER
    if (debuggerVar_topCamera)
        SetProjection(160, 100, 1000, 100, 100);
#endif

    InitViewedRoomList();
    GenereActiveList();
    GenereAffList();
    //  TriListObjet();
    RefreshAux2();
    /*  InitScreenCoors();
    */
    if (FlagInitView == 2)
    {
        flagRedraw = 2;
    }
    else
    {
        if (flagRedraw != 2)
        {
            flagRedraw = 1;
        }
    }

    FlagInitView = 0;
    RestoreTimerAnim();

    if (currentCameraZoneList[NumCamera]) for (int i = 0; i < currentCameraZoneList[NumCamera]->hybrids.size(); i++) {
        startAnim2d(i);
    }
}

s16 GiveDistance2D(int x1, int z1, int x2, int z2)
{
    //int axBackup = x1;
    x1 -= x2;
    if ((s16)x1 < 0)
    {
        x1 = -(s16)x1;
    }

    z1 -= z2;
    if ((s16)z1 < 0)
    {
        z1 = -(s16)z1;
    }

    if ((x1 + z1) > 0xFFFF)
    {
        return(0x7D00);
    }
    else
    {
        return(x1 + z1);
    }
}

void InitRealValue(s16 startValue, s16 endValue, s16 time, RealValue* realValue)
{
    realValue->startValue = startValue;
    realValue->endValue = endValue;
    realValue->numSteps = time;
    realValue->memoTicks = timer;
}

s16 updateActorRotation(RealValue* rotatePtr)
{
    int timeDif;
    int angleDif;

    if (!rotatePtr->numSteps)
        return(rotatePtr->endValue);

    timeDif = timer - rotatePtr->memoTicks;

    if (timeDif > rotatePtr->numSteps)
    {
        rotatePtr->numSteps = 0;
        return(rotatePtr->endValue);
    }

    angleDif = (rotatePtr->endValue & 0x3FF) - (rotatePtr->startValue & 0x3FF);

    if (angleDif <= 0x200)
    {
        if (angleDif >= -0x200)
        {
            int angle = (rotatePtr->endValue & 0x3FF) - (rotatePtr->startValue & 0x3FF);
            return (rotatePtr->startValue & 0x3FF) + (angle * timeDif) / rotatePtr->numSteps;
        }
        else
        {
            s16 angle = ((rotatePtr->endValue & 0x3FF) + 0x400) - ((rotatePtr->startValue & 0x3FF));
            return (((rotatePtr->startValue & 0x3FF)) + ((angle * timeDif) / rotatePtr->numSteps));
        }
    }
    else
    {
        int angle = (rotatePtr->endValue & 0x3FF) - ((rotatePtr->startValue & 0x3FF) + 0x400);
        return ((angle * timeDif) / rotatePtr->numSteps) + ((rotatePtr->startValue & 0x3FF));
    }
}

void removeFromBGIncrust(int actorIdx)
{
    tObject* actorPtr = &ListObjets[actorIdx];

    actorPtr->objectType &= ~AF_BOXIFY;

    //  FlagRefreshAux2 = 1;

    BBox3D1 = actorPtr->screenXMin;

    if (BBox3D1 > -1)
    {
        BBox3D2 = actorPtr->screenYMin;
        BBox3D3 = actorPtr->screenXMax;
        BBox3D4 = actorPtr->screenYMax;

        //deleteSubSub();
    }
}

int findObjectInInventory(int objIdx)
{
    for (int i = 0; i < numObjInInventoryTable[currentInventory]; i++)
    {
        if (inventoryTable[currentInventory][i] == objIdx)
        {
            return(i);
        }
    }

    return(-1);
}

void DeleteInventoryObjet(int objIdx)
{
    int inventoryIdx;

    inventoryIdx = findObjectInInventory(objIdx);

    if (inventoryIdx != -1)
    {
        memmove(&inventoryTable[currentInventory][inventoryIdx], &inventoryTable[currentInventory][inventoryIdx + 1], (30 - inventoryIdx - 1) * 2);

        numObjInInventoryTable[currentInventory]--;
    }

    ListWorldObjets[objIdx].foundFlag &= 0x7FFF;
}

void deleteObject(int objIdx)
{
    tWorldObject* objPtr;
    int actorIdx;
    tObject* actorPtr;

    objPtr = &ListWorldObjets[objIdx];
    actorIdx = objPtr->objIndex;

    if (actorIdx != -1)
    {
        actorPtr = &ListObjets[actorIdx];

        actorPtr->room = -1;
        actorPtr->stage = -1;

        //    FlagGenereActiveList = 1;

        if (actorPtr->objectType & AF_BOXIFY)
        {
            removeFromBGIncrust(actorIdx);
        }
    }

    objPtr->room = -1;
    objPtr->stage = -1;

    DeleteInventoryObjet(objIdx);
}
