///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Cameras, background masks and viewed rooms (split from main.cpp)
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


void loadCamera(int cameraIdx)
{
    char name[16];
    int useSpecial = -1;

    sprintf_s(name, sizeof(name), "CAMERA%02d", g_currentFloor);

    if (g_gameId == AITD1)
    {
        if (CVars[getCVarsIdx(KILLED_SORCERER)] == 1)
        {
            switch (g_currentFloor)
            {
            case 6:
            {
                if (cameraIdx == 0)
                {
                    useSpecial = AITD1_CAM06000;
                }
                if (cameraIdx == 5)
                {
                    useSpecial = AITD1_CAM06005;
                }
                if (cameraIdx == 8)
                {
                    useSpecial = AITD1_CAM06008;
                }
                break;
            }
            case 7:
            {
                if (cameraIdx == 0)
                {
                    useSpecial = AITD1_CAM07000;
                }
                if (cameraIdx == 1)
                {
                    useSpecial = AITD1_CAM07001;
                }
                break;
            }
            }
        }

        if (useSpecial != -1)
        {
            strcpy(name, "ITD_RESS");
            cameraIdx = useSpecial;
        }
    }

    if (!LoadPak(name, cameraIdx, aux))
    {
        fatalError(0, name);
    }

    if (g_gameId == AITD3)
    {
        //memmove(aux,aux+4,64000+0x300);
    }

    if (g_gameId >= JACK)
    {
        copyPalette(aux + 64000, currentGamePalette);

        if (g_gameId == AITD3)
        {
            for (int i = 0; i < 16; i++) {
                currentGamePalette[i][0] = defaultPaletteAITD3[i * 3 + 0];
                currentGamePalette[i][1] = defaultPaletteAITD3[i * 3 + 1];
                currentGamePalette[i][2] = defaultPaletteAITD3[i * 3 + 2];
            }
        }
        else
        {
            for (int i = 0; i < 16; i++) {
                currentGamePalette[i][0] = defaultPalette[i * 3 + 0];
                currentGamePalette[i][1] = defaultPalette[i * 3 + 1];
                currentGamePalette[i][2] = defaultPalette[i * 3 + 2];
            }
        }
        convertPaletteIfRequired(currentGamePalette);

        osystem_setPalette(&currentGamePalette);
    }

    if (g_onCameraBackgroundReady)
        g_onCameraBackgroundReady(name, cameraIdx, (const unsigned char*)aux);

}

struct sMaskStruct
{
    u16 x1;
    u16 y1;
    u16 x2;
    u16 y2;
    u16 deltaX;
    u16 deltaY;

    std::array<u8, 320 * 200> mask;
};

std::vector<std::vector<sMaskStruct>> g_maskBuffers;

void loadMask(int cameraIdx)
{
    if (g_gameId == TIMEGATE)
        return;

    char name[16];

    sprintf_s(name, sizeof(name), "MASK%02d", g_currentFloor);

    if (g_MaskPtr)
    {
        free(g_MaskPtr);
    }

    g_MaskPtr = (unsigned char*)loadPak(name, cameraIdx);

    if (!g_MaskPtr)
    {
        printf("[MASK] Warning: loadPak returned NULL for %s index %d\n", name, cameraIdx);
        g_maskBuffers.clear();
        return;
    }

    g_maskBuffers.clear();
    g_maskBuffers.resize(cameraDataTable[NumCamera]->numViewedRooms);
    for (int i = 0; i < cameraDataTable[NumCamera]->numViewedRooms; i++)
    {
        cameraViewedRoomStruct* pRoomView = &cameraDataTable[NumCamera]->viewedRoomTable[i];
        unsigned char* pViewedRoomMask = g_MaskPtr + READ_LE_U32(g_MaskPtr + i * 4);

        g_maskBuffers[i].reserve(pRoomView->masks.size());
        for (int j = 0; j < pRoomView->masks.size(); j++)
        {
            sMaskStruct* pDestMask = &g_maskBuffers[i].emplace_back();
            unsigned char* pMaskData = pViewedRoomMask + READ_LE_U32(pViewedRoomMask + j * 4);

            pDestMask->mask.fill(0);

            pDestMask->x1 = READ_LE_U16(pMaskData);
            pMaskData += 2;
            pDestMask->y1 = READ_LE_U16(pMaskData);
            pMaskData += 2;
            pDestMask->x2 = READ_LE_U16(pMaskData);
            pMaskData += 2;
            pDestMask->y2 = READ_LE_U16(pMaskData);
            pMaskData += 2;
            pDestMask->deltaX = READ_LE_U16(pMaskData);
            pMaskData += 2;
            pDestMask->deltaY = READ_LE_U16(pMaskData);
            pMaskData += 2;

            assert(pDestMask->deltaX == pDestMask->x2 - pDestMask->x1 + 1);
            assert(pDestMask->deltaY == pDestMask->y2 - pDestMask->y1 + 1);

            for (int k = 0; k < pDestMask->deltaY; k++)
            {
                u16 uNumEntryForLine = READ_LE_U16(pMaskData);
                pMaskData += 2;

                unsigned char* pSourceBuffer = (unsigned char*)aux;

                int offset = pDestMask->x1 + pDestMask->y1 * 320 + k * 320;

                for (int l = 0; l < uNumEntryForLine; l++)
                {
                    unsigned char uNumSkip = *(pMaskData++);
                    unsigned char uNumCopy = *(pMaskData++);

                    offset += uNumSkip;

                    for (int m = 0; m < uNumCopy; m++)
                    {
                        if (offset >= 0 && offset < (int)pDestMask->mask.size())
                            pDestMask->mask[offset] = 0xFF;
                        offset++;
                    }
                }
            }

            osystem_createMask(pDestMask->mask, i, j, (int)cameraDataTable[NumCamera]->viewedRoomTable[i].viewedRoomIdx, pDestMask->x1, pDestMask->y1, pDestMask->x2, pDestMask->y2);
        }
    }
}

void fillpoly(s16* datas, int n, unsigned char c);
extern unsigned char* polyBackBuffer;

void createAITD1Mask()
{
    if (!room_PtrCamera[NumCamera])
        return;

    for (int viewedRoomIdx = 0; viewedRoomIdx < cameraDataTable[NumCamera]->numViewedRooms; viewedRoomIdx++)
    {
        cameraViewedRoomStruct* pcameraViewedRoomData = &cameraDataTable[NumCamera]->viewedRoomTable[viewedRoomIdx];

        char* data2 = room_PtrCamera[NumCamera] + pcameraViewedRoomData->offsetToMask;
        char* data = data2;
        data += 2;

        int numMask = READ_LE_U16(data2);
        if (numMask < 0 || numMask > 256)
        {
            printf("[MASK] Warning: invalid numMask %d for camera %d room %d\n", numMask, NumCamera, viewedRoomIdx);
            continue;
        }

        for (int maskIdx = 0; maskIdx < numMask; maskIdx++)
        {
            sMaskStruct theMaskStruct;
            sMaskStruct* pDestMask = &theMaskStruct;
            pDestMask->mask.fill(0);
            polyBackBuffer = pDestMask->mask.data();

            int numMaskZone = READ_LE_U16(data);
            char* src = data2 + READ_LE_U16(data + 2);

            int minX = 319;
            int maxX = 0;
            int minY = 199;
            int maxY = 0;

            /*if(isBgOverlayRequired( actorPtr->zv.ZVX1 / 10, actorPtr->zv.ZVX2 / 10,
            actorPtr->zv.ZVZ1 / 10, actorPtr->zv.ZVZ2 / 10,
            data+4,
            *(s16*)(data) ))*/
            {
                int numMaskPoly = READ_LE_U16(src);
                src += 2;

                if (numMaskPoly > 256)
                {
                    printf("[MASK] Warning: invalid numMaskPoly %d for camera %d mask %d\n", numMaskPoly, NumCamera, maskIdx);
                    polyBackBuffer = nullptr;
                    goto nextMask;
                }

                for (int maskPolyIdx = 0; maskPolyIdx < numMaskPoly; maskPolyIdx++)
                {
                    int numPoints = READ_LE_U16(src);
                    src += 2;

                    if (numPoints > 64)
                    {
                        printf("[MASK] Warning: invalid numPoints %d for camera %d mask %d poly %d\n", numPoints, NumCamera, maskIdx, maskPolyIdx);
                        polyBackBuffer = nullptr;
                        goto nextMask;
                    }

                    if (numPoints * 4 <= (int)sizeof(cameraBuffer))
                        memcpy(cameraBuffer, src, numPoints * 4);

                    fillpoly((short*)src, numPoints, 0xFF);

                    for (int verticeId = 0; verticeId < numPoints; verticeId++)
                    {
                        short verticeX = *(short*)(src + verticeId * 4 + 0);
                        short verticeY = *(short*)(src + verticeId * 4 + 2);

                        minX = std::min<int>(minX, verticeX);
                        minY = std::min<int>(minY, verticeY);
                        maxX = std::max<int>(maxX, verticeX);
                        maxY = std::max<int>(maxY, verticeY);
                    }

                    src += numPoints * 4;
                    //drawBgOverlaySub2(param);
                }

                //      blitOverlay(src);

                polyBackBuffer = nullptr;

            }

            osystem_createMask(pDestMask->mask, viewedRoomIdx, maskIdx, (int)pcameraViewedRoomData->viewedRoomIdx, minX - 1, minY - 1, maxX + 1, maxY + 1);

            nextMask:
            int numOverlay = READ_LE_U16(data);
            data += 2;
            data += ((numOverlay * 4) + 1) * 2;
        }

        /*		unsigned char* pViewedRoomMask = g_MaskPtr + READ_LE_U32(g_MaskPtr + i*4);

        for(int j=0; j<pRoomView->numMask; j++)
        {
        unsigned char* pMaskData = pViewedRoomMask + READ_LE_U32(pViewedRoomMask + j*4);

        maskStruct* pDestMask = &g_maskBuffers[i][j];

        memset(pDestMask->mask, 0, 320*200);

        pDestMask->x1 = READ_LE_U16(pMaskData);
        pMaskData += 2;
        pDestMask->y1 = READ_LE_U16(pMaskData);
        pMaskData += 2;
        pDestMask->x2 = READ_LE_U16(pMaskData);
        pMaskData += 2;
        pDestMask->y2 = READ_LE_U16(pMaskData);
        pMaskData += 2;
        pDestMask->deltaX = READ_LE_U16(pMaskData);
        pMaskData += 2;
        pDestMask->deltaY = READ_LE_U16(pMaskData);
        pMaskData += 2;

        assert(pDestMask->deltaX == pDestMask->x2 - pDestMask->x1 + 1);
        assert(pDestMask->deltaY == pDestMask->y2 - pDestMask->y1 + 1);

        for(int k=0; k<pDestMask->deltaY; k++)
        {
        u16 uNumEntryForLine = READ_LE_U16(pMaskData);
        pMaskData += 2;

        unsigned char* pDestBuffer = pDestMask->mask;
        unsigned char* pSourceBuffer = (unsigned char*)aux;

        int offset = pDestMask->x1 + pDestMask->y1 * 320 + k * 320;

        for(int l=0; l<uNumEntryForLine; l++)
        {
        unsigned char uNumSkip = *(pMaskData++);
        unsigned char uNumCopy = *(pMaskData++);

        offset += uNumSkip;

        for(int m=0; m<uNumCopy; m++)
        {
        pDestBuffer[offset] = 0xFF;
        offset++;
        }
        }
        }

        osystem_createMask(pDestMask->mask, i, j, (unsigned char*)aux, pDestMask->x1, pDestMask->y1, pDestMask->x2, pDestMask->y2);
        }*/
    }

    polyBackBuffer = NULL;
}


void SetAngleCamera(int x, int y, int z)
{
    transformX = x & 0x3FF;
    if (transformX)
    {
        transformXCos = cosTable[transformX];
        transformXSin = cosTable[(transformX + 0x100) & 0x3FF];
        transformUseX = true;
    }
    else
    {
        transformUseX = false;
    }

    transformY = y & 0x3FF;
    if (transformY)
    {
        transformYCos = cosTable[transformY];
        transformYSin = cosTable[(transformY + 0x100) & 0x3FF];
        transformUseY = true;
    }
    else
    {
        transformUseY = false;
    }

    transformZ = z & 0x3FF;
    if (transformZ)
    {
        transformZCos = cosTable[transformZ];
        transformZSin = cosTable[(transformZ + 0x100) & 0x3FF];
        transformUseZ = true;
    }
    else
    {
        transformUseZ = false;
    }
}

void SetPosCamera(int x, int y, int z)
{
    translateX = x;
    translateY = y;
    translateZ = z;
}

void SetProjection(int centerX, int centerY, int x, int y, int z)
{
    cameraCenterX = centerX;
    cameraCenterY = centerY;

    cameraPerspective = x;
    cameraFovX = y;
    cameraFovY = z;
}

int isInViewList(int value)
{
    char* ptr = currentCameraVisibilityList;
    int var;

    while ((var = *(ptr++)) != -1)
    {
        if (value == var)
        {
            return(1);
        }
    }

    return(0);
}

// setup visibility list
void InitViewedRoomList()
{
    u32 i;
    int j;
    int var_10;

    char* dataTabPos = currentCameraVisibilityList;
    char* dataTabEnd = currentCameraVisibilityList + sizeof(currentCameraVisibilityList) - 1;

    *dataTabPos = -1;

    // visibility list: add linked rooms
    for (i = 0; i < roomDataTable[currentRoom].numSceZone; i++)
    {
        if (roomDataTable[currentRoom].sceZoneTable[i].type == 0)
        {
            var_10 = roomDataTable[currentRoom].sceZoneTable[i].parameter;
            if (!isInViewList(var_10))
            {
                if (dataTabPos >= dataTabEnd) break;
                *(dataTabPos++) = var_10;
                *(dataTabPos) = -1;
            }
        }
    }

    // visibility list: add room seen by the current camera
    for (j = 0; j < cameraDataTable[NumCamera]->numViewedRooms; j++)
    {
        if (!isInViewList(cameraDataTable[NumCamera]->viewedRoomTable[j].viewedRoomIdx))
        {
            if (dataTabPos >= dataTabEnd) break;
            *(dataTabPos++) = (char)cameraDataTable[NumCamera]->viewedRoomTable[j].viewedRoomIdx;
            *(dataTabPos) = -1;
        }
    }
}
