///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Program start-up, CVars, texts, palettes and helpers (split from main.cpp)
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
#include <unordered_map>
#include <string>
#include <fstream>


int* currentCVarTable = NULL;

int getCVarsIdx(enumCVars searchedType) // TODO: optimize by reversing the table....
{
    for (int i = 0; i < CVars.size(); i++)
    {
        if (currentCVarTable[i] == -1)
        {
            ASSERT(0);
        }


        if (currentCVarTable[i] == searchedType)
            return i;
    }

    ASSERT(0);
    return 0;
}

int getCVarsIdx(int searchedType)
{
    return getCVarsIdx((enumCVars)searchedType);
}

const unsigned char defaultPalette[0x30] =
{
    0x00,
    0x00,
    0x00,
    0x3F,
    0x3F,
    0x3F,
    0x0C,
    0x0C,
    0x0E,
    0x30,
    0x2F,
    0x3F,
    0x23,
    0x2C,
    0x23,
    0x2A,
    0x1D,
    0x2A,
    0x2A,
    0x21,
    0x18,
    0x3F,
    0x05,
    0x2A,
    0x12,
    0x14,
    0x18,
    0x31,
    0x15,
    0x17,
    0x15,
    0x25,
    0x15,
    0x15,
    0x2F,
    0x3F,
    0x3F,
    0x22,
    0x15,
    0x2B,
    0x15,
    0x3F,
    0x3F,
    0x3F,
    0x21,
    0x3F,
    0x3F,
    0x3F
};

const unsigned char defaultPaletteAITD3[0x30] =
{
    0x00,
    0x00,
    0x00,
    0xFC,
    0xFC,
    0xFC,
    0x30,
    0x30,
    0x38,
    0xC0,
    0xBC,
    0xFC,
    0x78,
    0x58,
    0x3C,
    0x00,
    0x00,
    0x00,
    0xF0,
    0x70,
    0x10,
    0xFC,
    0xFC,
    0xFC,
    0x48,
    0x50,
    0x60,
    0xC4,
    0x54,
    0x5C,
    0x54,
    0x94,
    0x54,
    0x54,
    0xBC,
    0xFC,
    0xFC,
    0x88,
    0x54,
    0xAC,
    0x54,
    0xFC,
    0xFC,
    0xFC,
    0xFC,
    0xFC,
    0xFC,
    0xF8
};

// The chronometer of each object whose script runs from the hand
// (executeFoundLife). Such an object is not an actor: it borrows a free
// actor slot every time, and the script used that slot's CHRONO. A slot
// left with an old CHRONO (an old save, another object) kept the lamp's
// "CHRONO == 10" from ever being true, so its oil never ran out again
// (BUGS.md C29). Each object now keeps its own, counted from when it is
// first run here; resetFoundObjectChronos starts them again after a load.
static std::unordered_map<int, unsigned int> s_foundObjectChronos;

void resetFoundObjectChronos()
{
    s_foundObjectChronos.clear();
}

void executeFoundLife(int objIdx)
{
    int var_2;
    int actorIdx;
    int lifeOffset;
    int currentActorIdx;
    int currentActorLifeIdx;
    int currentActorLifeNum;
    int foundLife;
    tObject* currentActorPtr;
    tObject* currentActorLifePtr;

    if (objIdx == -1)
        return;

    foundLife = ListWorldObjets[objIdx].foundLife;

    if (ListWorldObjets[objIdx].foundLife == -1)
        return;

    currentActorPtr = currentProcessedActorPtr;
    currentActorIdx = currentProcessedActorIdx;
    currentActorLifeIdx = currentLifeActorIdx;
    currentActorLifePtr = currentLifeActorPtr;
    currentActorLifeNum = currentLifeNum;

    if (currentLifeNum != -1 && currentLifePtr != nullptr)
    {
        lifeOffset = (int)((currentLifePtr - HQR_Get(listLife, currentActorLifeNum)) / 2);
    }
    else
    {
        lifeOffset = 0; // Native scripts don't use bytecode offset
    }

    var_2 = 0;

    actorIdx = ListWorldObjets[objIdx].objIndex;

    if (actorIdx == -1)
    {
        tObject* currentActorEntryPtr = &ListObjets[NUM_MAX_OBJECT - 1];
        int currentActorEntry = NUM_MAX_OBJECT - 1;

        while (currentActorEntry >= 0)
        {
            if (currentActorEntryPtr->indexInWorld == -1)
                break;

            currentActorEntryPtr--;
            currentActorEntry--;
        }

        if (currentActorEntry == -1) // no space, we will have to overwrite the last actor !
        {
            currentActorEntry = NUM_MAX_OBJECT - 1;
            currentActorEntryPtr = &ListObjets[NUM_MAX_OBJECT - 1];
        }

        actorIdx = currentActorEntry;
        var_2 = 1;

        currentProcessedActorPtr = &ListObjets[actorIdx];
        currentLifeActorPtr = &ListObjets[actorIdx];
        currentProcessedActorIdx = actorIdx;
        currentLifeActorIdx = actorIdx;

        currentProcessedActorPtr->indexInWorld = objIdx;
        currentProcessedActorPtr->life = -1;
        currentProcessedActorPtr->bodyNum = -1;
        currentProcessedActorPtr->objectType = 0;
        currentProcessedActorPtr->trackMode = -1;
        currentProcessedActorPtr->room = -1;
        currentProcessedActorPtr->lifeMode = -1;
        currentProcessedActorPtr->ANIM = -1;
    }

    // The borrowed slot runs with this object's own chronometer
    if (var_2)
    {
        auto chrono = s_foundObjectChronos.find(objIdx);
        const bool valid = (chrono != s_foundObjectChronos.end()) && chrono->second <= timer;
        currentProcessedActorPtr->CHRONO = valid ? chrono->second : timer;
    }

    processLife(foundLife, true);

    if (var_2)
        s_foundObjectChronos[objIdx] = currentProcessedActorPtr->CHRONO;

    if (var_2)
    {
        currentProcessedActorPtr->indexInWorld = -1;
    }

    currentProcessedActorPtr = currentActorPtr;
    currentProcessedActorIdx = currentActorIdx;
    currentLifeActorIdx = currentActorLifeIdx;
    currentLifeActorPtr = currentActorLifePtr;

    if (currentActorLifeNum != -1 && currentLifePtr != nullptr)
    {
        currentLifeNum = currentActorLifeNum;
        currentLifePtr = HQR_Get(listLife, currentLifeNum) + lifeOffset * 2;
    }
}

void InitCopyBox(char* var0, char* var1)
{
    screenSm1 = var0;
    screenSm2 = var0;

    screenSm3 = var1;
    screenSm4 = var1;
    screenSm5 = var1;
}

void allocTextes(void)
{
    int currentIndex;
    u8* currentPosInTextes;
    int textCounter;
    int stringIndex;
    u8* stringPtr;
    int textLength;

    tabTextes = (textEntryStruct*)malloc(NUM_MAX_TEXT_ENTRY * sizeof(textEntryStruct)); // 2000 = 250 * 8

    ASSERT_PTR(tabTextes);

    if (!tabTextes)
    {
        fatalError(1, "TabTextes");
    }

    // setup languageNameString (skip auto-detection if already set)
    if (!languageNameString.length())
    {
        for (int i = 0; i < languageNameTable.size(); i++)
        {
            char tempString[20];

            strcpy(tempString, languageNameTable[i].c_str());
            strcat(tempString, ".PAK");

            if (dataFileExists(tempString))
            {
                languageNameString = languageNameTable[i].c_str();
                break;
            }
        }
    }

    if (!languageNameString.length())
    {
        printf(MAIN_WARN "Unable to detect language file.." CON_RESET "\n");
        assert(0);
    }

    systemTextes = (u8*)CheckLoadMallocPak(languageNameString.c_str(), 0); // todo: use real language name
    textLength = getPakSize(languageNameString.c_str(), 0);

    for (currentIndex = 0; currentIndex < NUM_MAX_TEXT_ENTRY; currentIndex++)
    {
        tabTextes[currentIndex].index = -1;
        tabTextes[currentIndex].textPtr = NULL;
        tabTextes[currentIndex].width = 0;
    }

    currentPosInTextes = systemTextes;

    textCounter = 0;

    while (currentPosInTextes < systemTextes + textLength)
    {
        currentIndex = *(currentPosInTextes++);

        if (currentIndex == 26)
            break;

        if (currentIndex == '@') // start of string marker
        {
            stringIndex = 0;

            while ((currentIndex = *(currentPosInTextes++)) >= '0' && currentIndex <= '9') // parse string number
            {
                stringIndex = stringIndex * 10 + currentIndex - 48;
            }

            if (currentIndex == ':') // start of string
            {
                stringPtr = currentPosInTextes;

                do
                {
                    currentPosInTextes++;
                } while ((unsigned char)*(currentPosInTextes - 1) >= ' '); // detect the end of the string

                *(currentPosInTextes - 1) = 0; // add the end of string

                if (textCounter >= NUM_MAX_TEXT_ENTRY)
                    break;

                tabTextes[textCounter].index = stringIndex;
                tabTextes[textCounter].textPtr = stringPtr;
                tabTextes[textCounter].width = ExtGetSizeFont(stringPtr);

                textCounter++;
            }

            if (currentIndex == 26)
            {
                return;
            }
        }
    }
}

void OpenProgram(void)
{
    //time_t ltime;
    FILE* fHandle;

    setupScreen();
    //setupInterrupt();
    //setupInterrupt2();
    //setupInterrupt3();

    //setupVideoMode();

//	time( &ltime );

//	srand(ltime);

    if (!initMusicDriver())
    {
        musicConfigured = 0;
        musicEnabled = 0;
    }

    // Initialize controller support
    initController();

    // TODO: reverse sound init code


    aux = (char*)malloc(65068);
    if (!aux)
    {
        fatalError(1, "Aux");
    }

    aux2 = (char*)malloc(65068);
    if (!aux2)
    {
        fatalError(1, "Aux2");
    }

    InitCopyBox(aux2, logicalScreen);
    /*  InitCopyPlot(aux2);
    InitSpecialCopyPoly(aux2); */

    BufferAnim.resize(NB_BUFFER_ANIM);

    switch (g_gameId)
    {
    case AITD3:
        PtrFont = CheckLoadMallocPak("ITD_RESS", 1);
        break;
    case JACK:
    case AITD2:
        PtrFont = CheckLoadMallocPak("ITD_RESS", 1);
        break;
    case AITD1:
        PtrFont = CheckLoadMallocPak("ITD_RESS", 5);
        break;
    case TIMEGATE:
        PtrFont = CheckLoadMallocPak("ITD_RESS", 2);
        break;
    default:
        assert(0);
    }

    SetFont(PtrFont, 14);

    if (g_gameId == AITD1)
    {
        SetFontSpace(2, 0);
    }
    else
    {
        SetFontSpace(2, 1);
    }

    switch (g_gameId)
    {
    case JACK:
    case AITD2:
    case AITD3:
    {
        PtrCadre = CheckLoadMallocPak("ITD_RESS", 0);
        break;
    }
    case AITD1:
    {
        PtrCadre = CheckLoadMallocPak("ITD_RESS", 4);
        break;
    }
    case TIMEGATE:
    {
        PtrCadre = CheckLoadMallocPak("ITD_RESS", 0);
        break;
    }
    }

    PtrPrioritySample = loadFromItd("PRIORITY.ITD");

    // read cvars definitions
    {
        fHandle = Open("DEFINES.ITD", "rb");
        if (!fHandle)
        {
            fatalError(0, "DEFINES.ITD");
        }
        for (int i = 0; i < CVars.size(); i++) {
            s16 cvarValue = 0;
            fread(&cvarValue, 2, 1, fHandle);
            CVars[i] = READ_BE_S16(&cvarValue);
        }
        fclose(fHandle);
    }

    allocTextes();

    //  if(musicConfigured)
    {
        listMus = HQR_InitRessource<char>("LISTMUS", 110000, 40);
    }

    char sampleFileName[256] = "";
    if (g_gameId == TIMEGATE)
    {
        strcpy(sampleFileName, "SAMPLES");
    }
    else
    {
        strcpy(sampleFileName, "LISTSAMP");
    }

    listSamp = HQR_InitRessource<char>(sampleFileName, 64000, 30);

    HQ_Memory = HQR_Init(10000, 50);
}

void freeAll(void)
{
    /*  HQR_Free(hqrUnk);

    HQR_Free(listSamp);

    HQR_Free(listMus);

    free(languageData);

    free(tabTextes);

    free(priority);

    free(aitdBoxGfx);

    free(fontData);

    free(bufferAnim);

    if(aux != aux3)
    {
    free(aux);
    }

    free(aux2);*/

    //TODO: implement all the code that restore the interrupts & all
}

textEntryStruct* getTextFromIdx(int index)
{
#ifndef _WIN32
    // Platform-specific text overrides
    // String 13 = "Return to Windows" -> platform name
    if (index == 13)
    {
#if defined(__APPLE__)
        static textEntryStruct platformReturnEntry = { 13, (u8*)"Return to macOS", 0 };
#else
        static textEntryStruct platformReturnEntry = { 13, (u8*)"Return to Linux", 0 };
#endif
        if (platformReturnEntry.width == 0)
            platformReturnEntry.width = ExtGetSizeFont(platformReturnEntry.textPtr);
        return &platformReturnEntry;
    }
#endif

    int currentIndex;

    for (currentIndex = 0; currentIndex < NUM_MAX_TEXT_ENTRY; currentIndex++)
    {
        if (tabTextes[currentIndex].index == index)
        {
            return(&tabTextes[currentIndex]);
        }
    }

    return(NULL);
}

void AffRect(int x1, int y1, int x2, int y2, char color) // fast recode. No RE
{
    // Clamp to screen bounds (320x200) to prevent out-of-bounds writes
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > 319) x2 = 319;
    if (y2 > 199) y2 = 199;
    if (x1 > x2 || y1 > y2) return;

    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;

    char* dest = logicalScreen + y1 * 320 + x1;
    char* dest2 = (char*)uiLayer.data() + y1 * 320 + x1;

    int i;
    int j;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            *(dest++) = color;
            *(dest2++) = color;
        }

        dest += 320 - width;
        dest2 += 320 - width;
    }
}

void loadPalette(void)
{
    palette_t localPalette;

    if (g_gameId == AITD2)
    {
        //loadPakToPtr("ITD_RESS",59,aux);
        return; // palette data not loaded for AITD2 yet
    }
    else
    {
        LoadPak("ITD_RESS", 3, aux);
    }
    copyPalette(aux, currentGamePalette);

    copyPalette(currentGamePalette, localPalette);
    //  fadeInSub1(localPalette);

    // to finish
}
