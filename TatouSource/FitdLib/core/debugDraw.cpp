///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Debug drawing of zones and collision boxes (FITD_DEBUGGER, Debug builds;
// split from main.cpp)
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


void drawProjectedLine(s32 x1s, s32 y1s, s32 z1s, s32 x2s, s32 y2s, s32 z2s, int c)
{
    float x1 = (float)x1s;
    float x2 = (float)x2s;
    float y1 = (float)y1s;
    float y2 = (float)y2s;
    float z1 = (float)z1s;
    float z2 = (float)z2s;

    float transformedX1;
    float transformedX2;

    float transformedY1;
    float transformedY2;

    x1 -= translateX;
    x2 -= translateX;

#if !defined(AITD_UE4)
    y1 -= translateY;
    y2 -= translateY;
#endif

    z1 -= translateZ;
    z2 -= translateZ;

#if defined(AITD_UE4)
    osystem_draw3dLine(x1, y1, z1, x2, y2, z2, c);
#else
    transformPoint(&x1, &y1, &z1);
    transformPoint(&x2, &y2, &z2);

    z1 += cameraPerspective;
    z2 += cameraPerspective;

    transformedX1 = ((x1 * cameraFovX) / (float)z1) + cameraCenterX;
    transformedX2 = ((x2 * cameraFovX) / (float)z2) + cameraCenterX;

    transformedY1 = ((y1 * cameraFovY) / (float)z1) + cameraCenterY;
    transformedY2 = ((y2 * cameraFovY) / (float)z2) + cameraCenterY;

    if (z1 > 0 && z2 > 0)
        osystem_draw3dLine(transformedX1, transformedY1, z1, transformedX2, transformedY2, z2, c);
#endif
}

#ifdef FITD_DEBUGGER
void line(int x1, int y1, int x2, int y2, char c);

void drawZv(const ZVStruct& localZv) {
    // bottom
    drawProjectedLine(localZv.ZVX1, localZv.ZVY2, localZv.ZVZ1, localZv.ZVX1, localZv.ZVY2, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX1, localZv.ZVY2, localZv.ZVZ2, localZv.ZVX2, localZv.ZVY2, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY2, localZv.ZVZ2, localZv.ZVX2, localZv.ZVY2, localZv.ZVZ1, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY2, localZv.ZVZ1, localZv.ZVX1, localZv.ZVY2, localZv.ZVZ1, 10);

    // top
    drawProjectedLine(localZv.ZVX1, localZv.ZVY1, localZv.ZVZ1, localZv.ZVX1, localZv.ZVY1, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX1, localZv.ZVY1, localZv.ZVZ2, localZv.ZVX2, localZv.ZVY1, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY1, localZv.ZVZ2, localZv.ZVX2, localZv.ZVY1, localZv.ZVZ1, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY1, localZv.ZVZ1, localZv.ZVX1, localZv.ZVY1, localZv.ZVZ1, 10);

    drawProjectedLine(localZv.ZVX1, localZv.ZVY2, localZv.ZVZ1, localZv.ZVX1, localZv.ZVY1, localZv.ZVZ1, 10);
    drawProjectedLine(localZv.ZVX1, localZv.ZVY2, localZv.ZVZ2, localZv.ZVX1, localZv.ZVY1, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY2, localZv.ZVZ2, localZv.ZVX2, localZv.ZVY1, localZv.ZVZ2, 10);
    drawProjectedLine(localZv.ZVX2, localZv.ZVY2, localZv.ZVZ1, localZv.ZVX2, localZv.ZVY1, localZv.ZVZ1, 10);
}

void drawZv(tObject* actorPtr)
{
    ZVStruct localZv;

    if (currentCameraTargetActor < 0 || currentCameraTargetActor >= NUM_MAX_OBJECT)
        return;

    if (actorPtr->room != ListObjets[currentCameraTargetActor].room)
    {
        if (ListObjets[currentCameraTargetActor].room == -1) {
            return;
        }
        CopyZV(&actorPtr->zv, &localZv);
        AdjustZV(&localZv, actorPtr->room, ListObjets[currentCameraTargetActor].room);
    }
    else
    {
        CopyZV(&actorPtr->zv, &localZv);
    }

    drawZv(localZv);
}

#define DEPTH_THRESHOLD 1000

void drawProjectedQuad(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4, float z1, float z2, float z3, float z4, int color, int transprency)
{
    float transformedX1;
    float transformedX2;
    float transformedX3;
    float transformedX4;

    float transformedY1;
    float transformedY2;
    float transformedY3;
    float transformedY4;

    x1 -= translateX;
    x2 -= translateX;
    x3 -= translateX;
    x4 -= translateX;

    y1 -= translateY;
    y2 -= translateY;
    y3 -= translateY;
    y4 -= translateY;

    z1 -= translateZ;
    z2 -= translateZ;
    z3 -= translateZ;
    z4 -= translateZ;

    transformPoint(&x1, &y1, &z1);
    transformPoint(&x2, &y2, &z2);
    transformPoint(&x3, &y3, &z3);
    transformPoint(&x4, &y4, &z4);

    z1 += cameraPerspective;
    z2 += cameraPerspective;
    z3 += cameraPerspective;
    z4 += cameraPerspective;

    // Guard against division by zero - skip rendering if any Z is too small
    const float MIN_Z = 1.0f;
    if (z1 < MIN_Z || z2 < MIN_Z || z3 < MIN_Z || z4 < MIN_Z)
        return;

    transformedX1 = ((x1 * cameraFovX) / (float)z1) + cameraCenterX;
    transformedX2 = ((x2 * cameraFovX) / (float)z2) + cameraCenterX;
    transformedX3 = ((x3 * cameraFovX) / (float)z3) + cameraCenterX;
    transformedX4 = ((x4 * cameraFovX) / (float)z4) + cameraCenterX;

    transformedY1 = ((y1 * cameraFovY) / (float)z1) + cameraCenterY;
    transformedY2 = ((y2 * cameraFovY) / (float)z2) + cameraCenterY;
    transformedY3 = ((y3 * cameraFovY) / (float)z3) + cameraCenterY;
    transformedY4 = ((y4 * cameraFovY) / (float)z4) + cameraCenterY;

    if (z1 > DEPTH_THRESHOLD && z2 > DEPTH_THRESHOLD && z3 > DEPTH_THRESHOLD && z4 > DEPTH_THRESHOLD)
    {
        osystem_draw3dQuad(transformedX1, transformedY1, z1, transformedX2, transformedY2, z2, transformedX3, transformedY3, z3, transformedX4, transformedY4, z4, color, transprency);
    }

    //osystem_draw3dQuad(x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4, color);
}

void drawProjectedBox(int x1, int x2, int y1, int y2, int z1, int z2, int color, int transprency)
{
    //bottom
    drawProjectedQuad((float)x1, (float)x1, (float)x2, (float)x2, (float)y1, (float)y1, (float)y1, (float)y1, (float)z1, (float)z2, (float)z2, (float)z1, color, transprency);
    //top
    drawProjectedQuad((float)x1, (float)x1, (float)x2, (float)x2, (float)y2, (float)y2, (float)y2, (float)y2, (float)z1, (float)z2, (float)z2, (float)z1, color, transprency);
    //left
    drawProjectedQuad((float)x1, (float)x1, (float)x1, (float)x1, (float)y1, (float)y2, (float)y2, (float)y1, (float)z1, (float)z1, (float)z2, (float)z2, color, transprency);
    //right
    drawProjectedQuad((float)x2, (float)x2, (float)x2, (float)x2, (float)y1, (float)y2, (float)y2, (float)y1, (float)z1, (float)z1, (float)z2, (float)z2, color, transprency);
    //front
    drawProjectedQuad((float)x1, (float)x2, (float)x2, (float)x1, (float)y1, (float)y1, (float)y2, (float)y2, (float)z1, (float)z1, (float)z1, (float)z1, color, transprency);
    //back
    drawProjectedQuad((float)x1, (float)x2, (float)x2, (float)x1, (float)y1, (float)y1, (float)y2, (float)y2, (float)z2, (float)z2, (float)z2, (float)z2, color, transprency);
}

void drawRoomZv(ZVStruct* zoneData, int color, int transparency)
{
    ZVStruct cameraZv = { -100,100,-100,100,-100,100 };

    cameraZv.ZVX1 += translateX;
    cameraZv.ZVX2 += translateX;

    cameraZv.ZVY1 += translateY;
    cameraZv.ZVY2 += translateY;

    cameraZv.ZVZ1 += translateZ;
    cameraZv.ZVZ2 += translateZ;

    if (CubeIntersect(&cameraZv, zoneData))
    {
        return;
    }

    drawProjectedBox(zoneData->ZVX1, zoneData->ZVX2, zoneData->ZVY1, zoneData->ZVY2, zoneData->ZVZ1, zoneData->ZVZ2, color, transparency);
}

void drawSceZone(int roomNumber)
{
    u32 i;
    ZVStruct dataLocal;

    for (i = 0; i < roomDataTable[roomNumber].numSceZone; i++)
    {
        memcpy(&dataLocal, &roomDataTable[roomNumber].sceZoneTable[i].zv, sizeof(ZVStruct));
        if (roomNumber != currentRoom)
        {
            AdjustZV(&dataLocal, roomNumber, currentRoom);
        }

        //		if(roomDataTable[roomNumber].sceZoneTable[i].parameter == 4)
        //			if(roomDataTable[roomNumber].sceZoneTable[i].type)
        {
            drawRoomZv(&dataLocal, 20, 40);
        }
    }
}

void drawHardCol(int roomNumber)
{
    ZVStruct dataLocal;

    for (u32 i = 0; i < roomDataTable[roomNumber].numHardCol; i++)
    {
        /*if(roomDataTable[roomNumber].hardColTable[i].type != 9)
            continue;*/

        CopyZV(&roomDataTable[roomNumber].hardColTable[i].zv, &dataLocal);

        if (roomNumber != currentRoom)
        {
            AdjustZV(&dataLocal, roomNumber, currentRoom);
        }

        switch (roomDataTable[roomNumber].hardColTable[i].type)
        {
        case 0: // objects
            drawRoomZv(&dataLocal, 9, 150);
            break;
        case 1: // walls
            drawRoomZv(&dataLocal, 100, 255);
            break;
        case 2: // dummy
            drawRoomZv(&dataLocal, 180, 255);
            break;
        case 3: // ground/climb
            drawRoomZv(&dataLocal, 70, 255);
            break;
        case 4: // over door zones
            drawRoomZv(&dataLocal, 50, 255);
            break;
        case 9: // scenario
            drawRoomZv(&dataLocal, 60, 255);
            break;
        case 10: // monsters
            drawRoomZv(&dataLocal, 80, 255);
            break;
        default:
            drawRoomZv(&dataLocal, 40, 40);
            break;
        }

    }
}
#endif
