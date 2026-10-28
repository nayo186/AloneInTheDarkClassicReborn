///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// World loading and variable initialization (split from main.cpp)
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


void LoadWorld(void)
{
    u8* pObjectData;
    u8* pObjectDataBackup;
    size_t objectDataSize;
    FILE* fHandle;
    int i;
    int choosePersoBackup;

    {
        char objectPath[512];
        strcpy(objectPath, homePath);
        strcat(objectPath, "OBJETS.ITD");
        fHandle = fopen(objectPath, "rb");
        if (!fHandle) {
            fatalError(0, "OBJETS.ITD");
        }

        fseek(fHandle, 0, SEEK_END);
        objectDataSize = ftell(fHandle);
        fseek(fHandle, 0, SEEK_SET);

        pObjectDataBackup = pObjectData = (u8*)malloc(objectDataSize);
        ASSERT(pObjectData);

        fread(pObjectData, objectDataSize, 1, fHandle);
        fclose(fHandle);
    }

    maxObjects = READ_LE_U16(pObjectData);
    pObjectData += 2;

    ListWorldObjets.resize(maxObjects);

    for (i = 0; i < maxObjects; i++)
    {
        ListWorldObjets[i].objIndex = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].body = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].flags = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].typeZV = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].foundBody = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].foundName = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].foundFlag = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].foundLife = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].x = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].y = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].z = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].alpha = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].beta = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].gamma = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].stage = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].room = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].lifeMode = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].life = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].floorLife = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].anim = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].frame = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].animType = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].animInfo = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].trackMode = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].trackNumber = READ_LE_U16(pObjectData);
        pObjectData += 2;

        ListWorldObjets[i].positionInTrack = READ_LE_U16(pObjectData);
        pObjectData += 2;

        if (g_gameId >= JACK)
        {
            ListWorldObjets[i].mark = READ_LE_U16(pObjectData);
            pObjectData += 2;
        }
        ListWorldObjets[i].flags |= 0x20;
    }

    free(pObjectDataBackup);

    vars = (s16*)loadFromItd("VARS.ITD");

    varSize = fileSize;

    if (g_gameId == AITD1)
    {
        choosePersoBackup = CVars[getCVarsIdx(CHOOSE_PERSO)]; // backup hero selection
    }

    {
        char definestPath[512];
        strcpy(definestPath, homePath);
        strcat(definestPath, "DEFINES.ITD");

        fHandle = fopen(definestPath, "rb");
        if (!fHandle)
        {
            fatalError(0, "DEFINES.ITD");
        }

        fread(&CVars[0], CVars.size(), 2, fHandle);
        fclose(fHandle);

        for (i = 0; i < CVars.size(); i++)
        {
            CVars[i] = ((CVars[i] & 0xFF) << 8) | ((CVars[i] & 0xFF00) >> 8);
        }
    }
    //////////////////////////////////////////////

    if (g_gameId == AITD1)
    {
        CVars[getCVarsIdx(CHOOSE_PERSO)] = choosePersoBackup;
    }

    listLife = HQR_InitRessource<char>("LISTLIFE", 65000, 100);
    listTrack = HQR_InitRessource<char>("LISTTRAK", 20000, 100);
    if (g_gameId >= JACK) {
        HQ_Hybrides = HQR_InitRessource<sHybrid>("LISTHYB", 20000, 10); // TODO: recheck size for other games
    }

    // Initialize native life scripts (safe-filter: only scripts with no risky opcodes are registered)
    initNativeLifeScripts();

    // TODO: missing dos memory check here

    if (g_gameId == AITD1)
    {
        HQ_Bodys = HQR_InitRessource<sBody>(listBodySelect[CVars[getCVarsIdx(CHOOSE_PERSO)]], 37000, 50); // was calculated from free mem size
        HQ_Anims = HQR_InitRessource<sAnimation>(listAnimSelect[CVars[getCVarsIdx(CHOOSE_PERSO)]], 30000, 80); // was calculated from free mem size
    }
    else
    {
        HQ_Bodys = HQR_InitRessource<sBody>("LISTBODY", 37000, 50); // was calculated from free mem size
        HQ_Anims = HQR_InitRessource<sAnimation>("LISTANIM", 30000, 80); // was calculated from free mem size

        HQ_Matrices = HQR_InitRessource<char>("LISTMAT", 64000, 5);
    }


    for (i = 0; i < NUM_MAX_OBJECT; i++)
    {
        ListObjets[i].indexInWorld = -1;
    }

    if (g_gameId == AITD1)
    {
        currentWorldTarget = CVars[getCVarsIdx(WORLD_NUM_PERSO)];
    }
}

void clearMessageTable(void)
{
    int i;

    for (i = 0; i < 5; i++)
    {
        messageTable[i].string = NULL;
    }
}

void initVars()
{
    FlagGameOver = 0;

    currentInventory = 0;

    for (int i = 0; i < NUM_MAX_INVENTORY; i++)
    {
        numObjInInventoryTable[i] = 0;
        inHandTable[i] = -1;
    }

    action = 0;

    genVar1 = genVar2;
    genVar3 = genVar4;

    NbPhysBoxs = 0;
    NbLogBoxs = 0;

    // Start each game in silence: a looping sample of the intro (skipped with
    // Esc before its script stops it) kept playing. The original had a single
    // sample channel, so the first sound of the game replaced it.
    osystem_stopSample();
    LastSample = -1;
    nextSample = -1;
    LastPriority = -1;
    currentMusic = -1;
    nextMusic = -1;

    lightOff = 0;
    lightVar2 = 0;

    currentCameraTargetActor = -1;
    currentWorldTarget = -1;

    statusScreenAllowed = 1;

    shakingAmplitude = 0;
    shakeVar1 = 0;
    g_shakeOffsetX = 0.f;
    g_shakeOffsetY = 0.f;

    clearMessageTable();
}
