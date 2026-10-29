///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Game save and load functionality
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "zlib.h"
#include <filesystem>
#include "gameFolder.h"

std::string saveFilePath(int slot, const char* extension)
{
    return std::string(savesFolder()) + "SAVE" + std::to_string(slot) + extension;
}

extern "C" {
    extern char homePath[512];
}

// Minimal PNG writer using zlib for IDAT compression
static void writePNGChunk(FILE* f, const char* type, const unsigned char* data, unsigned int length)
{
    unsigned char lenBuf[4];
    lenBuf[0] = (length >> 24) & 0xFF;
    lenBuf[1] = (length >> 16) & 0xFF;
    lenBuf[2] = (length >> 8) & 0xFF;
    lenBuf[3] = length & 0xFF;
    fwrite(lenBuf, 1, 4, f);
    fwrite(type, 1, 4, f);

    unsigned long crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)type, 4);
    if (data && length > 0)
    {
        fwrite(data, 1, length, f);
        crc = crc32(crc, data, length);
    }

    unsigned char crcBuf[4];
    crcBuf[0] = (crc >> 24) & 0xFF;
    crcBuf[1] = (crc >> 16) & 0xFF;
    crcBuf[2] = (crc >> 8) & 0xFF;
    crcBuf[3] = crc & 0xFF;
    fwrite(crcBuf, 1, 4, f);
}

// The save list shows the picture in a 104x84 box of the 320x200 screen
// (parseAllSaves, core/game.cpp): it is kept 320 wide, more than enough, not
// at the screen's size (1440x1080 made 2-3 MB files). Each pixel is the
// average of its area of the screen (RGB, the alpha is not used).
static const int kPreviewWidth = 320;

static std::vector<unsigned char> shrinkPreview(const unsigned char* rgba, int width, int height, int& outW, int& outH)
{
    outW = width < kPreviewWidth ? width : kPreviewWidth;
    outH = (int)((long long)height * outW / width);
    if (outH < 1)
        outH = 1;
    std::vector<unsigned char> rgb((size_t)outW * outH * 3);
    for (int y = 0; y < outH; y++)
    {
        const int y0 = (int)((long long)y * height / outH);
        const int y1 = (int)((long long)(y + 1) * height / outH);
        for (int x = 0; x < outW; x++)
        {
            const int x0 = (int)((long long)x * width / outW);
            const int x1 = (int)((long long)(x + 1) * width / outW);
            unsigned int sum[3] = { 0, 0, 0 };
            for (int sy = y0; sy < y1; sy++)
            {
                const unsigned char* p = rgba + ((size_t)sy * width + x0) * 4;
                for (int sx = x0; sx < x1; sx++, p += 4)
                {
                    sum[0] += p[0];
                    sum[1] += p[1];
                    sum[2] += p[2];
                }
            }
            const unsigned int count = (unsigned int)((y1 - y0) * (x1 - x0));
            unsigned char* out = &rgb[((size_t)y * outW + x) * 3];
            for (int c = 0; c < 3; c++)
                out[c] = (unsigned char)(count ? sum[c] / count : 0);
        }
    }
    return rgb;
}

static bool writePNG(const char* filename, const unsigned char* rgb, int width, int height)
{
    FILE* f = nullptr;
    fopen_s(&f, filename, "wb");
    if (!f) return false;

    static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);

    unsigned char ihdr[13];
    ihdr[0] = (width >> 24) & 0xFF;  ihdr[1] = (width >> 16) & 0xFF;
    ihdr[2] = (width >> 8) & 0xFF;   ihdr[3] = width & 0xFF;
    ihdr[4] = (height >> 24) & 0xFF; ihdr[5] = (height >> 16) & 0xFF;
    ihdr[6] = (height >> 8) & 0xFF;  ihdr[7] = height & 0xFF;
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;   // 8-bit RGB
    writePNGChunk(f, "IHDR", ihdr, 13);

    size_t rawSize = (size_t)height * ((size_t)width * 3 + 1);
    unsigned char* raw = (unsigned char*)malloc(rawSize);
    if (!raw) { fclose(f); return false; }

    size_t rawPos = 0;
    for (int y = 0; y < height; y++)
    {
        raw[rawPos++] = 0;
        memcpy(raw + rawPos, rgb + (size_t)y * width * 3, (size_t)width * 3);
        rawPos += (size_t)width * 3;
    }

    uLongf compSize = (uLongf)(rawSize + rawSize / 100 + 64);
    unsigned char* comp = (unsigned char*)malloc(compSize);
    if (!comp) { free(raw); fclose(f); return false; }

    int zret = compress2(comp, &compSize, raw, (uLong)rawSize, Z_BEST_COMPRESSION);
    free(raw);
    if (zret != Z_OK) { free(comp); fclose(f); return false; }

    writePNGChunk(f, "IDAT", comp, (unsigned int)compSize);
    free(comp);

    writePNGChunk(f, "IEND", NULL, 0);
    fclose(f);
    return true;
}

unsigned int currentSaveEntrySize;

// Save layout (BUGS.md C28). AITD1 saves are written as the original game
// (GOG) writes them, so they load in either: one inventory, the start time
// of each moving value in 16 bits, and an 80x50 picture after the header.
// Older saves of this engine (two inventories, 32-bit times, no picture) and
// the other games keep the engine's layout. loadSave works out which one a
// file uses from the size of its blocks.
struct SaveLayout
{
    int inventories;     // AITD1 has one; the engine writes NUM_MAX_INVENTORY
    int memoTicksSize;   // 2 (original) or 4 bytes
    bool stageFirst;     // objects: the original saves the floor before the room
};
static SaveLayout s_layout = { NUM_MAX_INVENTORY, 4, false };
static const SaveLayout kOriginalLayout = { 1, 2, true };
static const SaveLayout kEngineLayout = { NUM_MAX_INVENTORY, 4, false };

// The original's picture of the save (its save menu shows it): 80x50 palette
// indexes, then 32 bytes that change between saves without a pattern (left
// over memory, written as zeros here)
static const int kThumbnailWidth = 80;
static const int kThumbnailHeight = 50;
static const int kThumbnailPadding = 32;

static u32 swapBytes(u32 value)
{
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

void loadInterpolatedValue(RealValue* pRotateStruct, FILE* fHandle)
{
    ASSERT(sizeof(pRotateStruct->startValue) == 2);
    fread(&pRotateStruct->startValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->endValue) == 2);
    fread(&pRotateStruct->endValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->numSteps) == 2);
    fread(&pRotateStruct->numSteps,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->memoTicks) == 4);
    if (s_layout.memoTicksSize == 2)
    {
        // Only the low 16 bits were saved: the same time ago from the
        // restored clock (timer is read before the objects)
        u16 memo16 = 0;
        fread(&memo16, 2, 1, fHandle);
        pRotateStruct->memoTicks = timer - (u16)((u16)timer - memo16);
    }
    else
        fread(&pRotateStruct->memoTicks,4,1,fHandle);
}

void saveInterpolatedValue(RealValue* pRotateStruct, FILE* fHandle)
{
    ASSERT(sizeof(pRotateStruct->startValue) == 2);
    fwrite(&pRotateStruct->startValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->endValue) == 2);
    fwrite(&pRotateStruct->endValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->numSteps) == 2);
    fwrite(&pRotateStruct->numSteps,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->memoTicks) == 4);
    if (s_layout.memoTicksSize == 2)
    {
        const u16 memo16 = (u16)pRotateStruct->memoTicks;
        fwrite(&memo16, 2, 1, fHandle);
    }
    else
        fwrite(&pRotateStruct->memoTicks,4,1,fHandle);
}

int loadSave(int saveNumber)
{
    char buffer[256];
    FILE* fHandle;
    unsigned int var28;
    int var_E;
    int var_16;
    unsigned int offsetToVars;
    u16 tempVarSize;
    unsigned int offsetToActors;
    int i;
    int oldNumMaxObj;

    strcpy_s(buffer, sizeof(buffer), saveFilePath(saveNumber, ".ITD").c_str());

    fopen_s(&fHandle, buffer,"rb");

    if(!fHandle)
    {
        return(0);
    }

    LoadWorld();
    initVars();

    // Which layout (s_layout): AITD1 saves come in two. The data block holds
    // the room, 300 objects, the variables, the inventories and the clock;
    // its size tells how many inventories, and the size of the objects block
    // (50 of them) how long the times are.
    s_layout = kEngineLayout;
    if (g_gameId == AITD1)
    {
        u32 offsets[5] = {};
        fread(offsets, 4, 5, fHandle);
        fseek(fHandle, 0, SEEK_END);
        const long fileSize = ftell(fHandle);
        const long dataSize = (long)swapBytes(offsets[3]) - (long)swapBytes(offsets[2]);
        const long withoutInventories = 12 + 300 * 52 + (long)CVars.size() * 2 + 20;
        const long objectsSize = fileSize - (long)swapBytes(offsets[4]);
        s_layout.inventories = (dataSize - withoutInventories == (4 + INVENTORY_SIZE * 2)) ? 1 : NUM_MAX_INVENTORY;
        s_layout.memoTicksSize = (objectsSize == NUM_MAX_OBJECT * 160) ? 2 : 4;
        s_layout.stageFirst = (s_layout.memoTicksSize == 2);
        printf(MAIN_TAG "Save %d: %s layout (%d inventory, %d-byte times)\n", saveNumber,
            s_layout.inventories == 1 ? "original (GOG)" : "older engine", s_layout.inventories, s_layout.memoTicksSize);
    }

    fseek(fHandle,8,SEEK_SET);

    fread(&var28,4,1,fHandle);

    var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);

    fseek(fHandle,var28,SEEK_SET);

    ASSERT(sizeof(currentRoom) == 2);
    fread(&currentRoom,2,1,fHandle);

    ASSERT(sizeof(g_currentFloor) == 2);
    fread(&g_currentFloor,2,1,fHandle);

    ASSERT(sizeof(NumCamera) == 2);
    fread(&NumCamera,2,1,fHandle);

    ASSERT(sizeof(currentWorldTarget) == 2);
    fread(&currentWorldTarget,2,1,fHandle);

    ASSERT(sizeof(currentCameraTargetActor) == 2);
    fread(&currentCameraTargetActor,2,1,fHandle);

    ASSERT(sizeof(maxObjects) == 2);
    fread(&maxObjects,2,1,fHandle);

    if(g_gameId == AITD1)
    {
        oldNumMaxObj = maxObjects;
        maxObjects = 300; // fix for save engine..
        ListWorldObjets.resize(maxObjects);
    }

    for(i=0;i<maxObjects;i++)
    {
        ASSERT(sizeof(ListWorldObjets[i].objIndex) == 2);
        fread(&ListWorldObjets[i].objIndex,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].body) == 2);
        fread(&ListWorldObjets[i].body,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].flags) == 2);
        fread(&ListWorldObjets[i].flags,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].typeZV) == 2);
        fread(&ListWorldObjets[i].typeZV,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundBody) == 2);
        fread(&ListWorldObjets[i].foundBody,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundName) == 2);
        fread(&ListWorldObjets[i].foundName,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundFlag) == 2);
        fread(&ListWorldObjets[i].foundFlag,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundLife) == 2);
        fread(&ListWorldObjets[i].foundLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].x) == 2);
        fread(&ListWorldObjets[i].x,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].y) == 2);
        fread(&ListWorldObjets[i].y,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].z) == 2);
        fread(&ListWorldObjets[i].z,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].alpha) == 2);
        fread(&ListWorldObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].beta) == 2);
        fread(&ListWorldObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].gamma) == 2);
        fread(&ListWorldObjets[i].gamma,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].stage) == 2);
        fread(&ListWorldObjets[i].stage,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].room) == 2);
        fread(&ListWorldObjets[i].room,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].lifeMode) == 2);
        fread(&ListWorldObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].life) == 2);
        fread(&ListWorldObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].floorLife) == 2);
        fread(&ListWorldObjets[i].floorLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].anim) == 2);
        fread(&ListWorldObjets[i].anim,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].frame) == 2);
        fread(&ListWorldObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animType) == 2);
        fread(&ListWorldObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animInfo) == 2);
        fread(&ListWorldObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackMode) == 2);
        fread(&ListWorldObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackNumber) == 2);
        fread(&ListWorldObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].positionInTrack) == 2);
        fread(&ListWorldObjets[i].positionInTrack,2,1,fHandle);
    }

    if(g_gameId == AITD1)
    {
        maxObjects = oldNumMaxObj;
    }

    if(g_gameId == AITD1)
    {
        ASSERT(CVars.size() == 45);
    }

    for(i=0;i< CVars.size();i++)
    {
        ASSERT(sizeof(CVars[i]) == 2);
        fread(&CVars[i], 2, 1, fHandle);
    }
	
	// AITD1 uses one inventory: the second (AITD2/3) stays empty
	for(int inventoryId=s_layout.inventories; inventoryId<NUM_MAX_INVENTORY; inventoryId++)
	{
		inHandTable[inventoryId] = -1;
		numObjInInventoryTable[inventoryId] = 0;
	}
	for(int inventoryId=0; inventoryId<s_layout.inventories; inventoryId++)
	{
		ASSERT(sizeof(inHandTable[inventoryId]) == 2);
		fread(&inHandTable[inventoryId],2,1,fHandle);

		ASSERT(sizeof(numObjInInventoryTable[inventoryId]) == 2);
		fread(&numObjInInventoryTable[inventoryId],2,1,fHandle);

		if(g_gameId == AITD1)
		{
			ASSERT(INVENTORY_SIZE == 30);
		}

		for(i=0;i<INVENTORY_SIZE;i++)
		{
			ASSERT(sizeof(inventoryTable[inventoryId][i]) == 2);
			fread(&inventoryTable[inventoryId][i],2,1,fHandle);
		}
	}

    ASSERT(sizeof(statusScreenAllowed) == 2);
    fread(&statusScreenAllowed,2,1,fHandle);

    ASSERT(sizeof(FlagGameOver) == 2);
    fread(&FlagGameOver,2,1,fHandle);

    ASSERT(sizeof(lightOff) == 2);
    fread(&lightOff,2,1,fHandle);

    ASSERT(sizeof(shakingAmplitude) == 2);
    fread(&shakingAmplitude,2,1,fHandle);

    ASSERT(sizeof(shakeVar1) == 2);
    fread(&shakeVar1,2,1,fHandle);

    ASSERT(sizeof(timer) == 4);
    fread(&timer,4,1,fHandle);
    // The clock itself, not only its copy: process_events sets timer from
    // timeGlobal every frame, which wiped the saved time (BUGS.md C29)
    setGameClock(timer);
    resetFoundObjectChronos();   // the lamp in hand counts again from now

    ASSERT(sizeof(timerFreeze1) == 4);
    fread(&timerFreeze1,4,1,fHandle);

    ASSERT(sizeof(currentMusic) == 2);
    fread(&currentMusic,2,1,fHandle);

    //timerFreeze = 1;

    var_E = NumCamera;

    LoadEtage(g_currentFloor);
    NumCamera = -1;
    ChangeSalle(currentRoom);
    var_16 = currentMusic;
    currentMusic = -1;
    playMusic(var_16);

    fseek(fHandle,12,SEEK_SET);
    fread(&offsetToVars,4,1,fHandle);
    offsetToVars = ((offsetToVars & 0xFF) << 24) | ((offsetToVars & 0xFF00) << 8) | (( offsetToVars & 0xFF0000) >> 8) | ((offsetToVars & 0xFF000000) >> 24);
    fseek(fHandle,offsetToVars,SEEK_SET);

    fread(&tempVarSize,2,1,fHandle);
    {
        int savedVarSize = varSize; // varSize set by LoadWorld from VARS.ITD
        varSize = tempVarSize;
        if (varSize > savedVarSize)
            varSize = savedVarSize; // clamp to allocated buffer size
    }

    fread(vars,varSize,1,fHandle);

    if(g_gameId == AITD1)
    {
        configureHqrHero(HQ_Bodys,listBodySelect[CVars[getCVarsIdx(CHOOSE_PERSO)]]);
        configureHqrHero(HQ_Anims,listAnimSelect[CVars[getCVarsIdx(CHOOSE_PERSO)]]);
    }
    else
    {
		/*
        configureHqrHero(listBody,0);
        configureHqrHero(listAnim,0);
		*/
    }

    fseek(fHandle,16,SEEK_SET);
    fread(&offsetToActors,4,1,fHandle);
    offsetToActors = ((offsetToActors & 0xFF) << 24) | ((offsetToActors & 0xFF00) << 8) | (( offsetToActors & 0xFF0000) >> 8) | ((offsetToActors & 0xFF000000) >> 24);
    fseek(fHandle,offsetToActors,SEEK_SET);

    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        ASSERT(sizeof(ListObjets[i].indexInWorld) == 2);
        fread(&ListObjets[i].indexInWorld,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].bodyNum) == 2);
        fread(&ListObjets[i].bodyNum,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].objectType) == 2);
        fread(&ListObjets[i].objectType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].dynFlags) == 2);
        fread(&ListObjets[i].dynFlags,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX1) == 2);
        fread(&ListObjets[i].zv.ZVX1,2,1,fHandle);
        ListObjets[i].zv.ZVX1 = (s16)ListObjets[i].zv.ZVX1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVX2) == 2);
        fread(&ListObjets[i].zv.ZVX2,2,1,fHandle);
        ListObjets[i].zv.ZVX2 = (s16)ListObjets[i].zv.ZVX2;

        //    ASSERT(sizeof(actorTable[i].zv.ZVY1) == 2);
        fread(&ListObjets[i].zv.ZVY1,2,1,fHandle);
        ListObjets[i].zv.ZVY1 = (s16)ListObjets[i].zv.ZVY1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVY2) == 2);
        fread(&ListObjets[i].zv.ZVY2,2,1,fHandle);
        ListObjets[i].zv.ZVY2 = (s16)ListObjets[i].zv.ZVY2;

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ1) == 2);
        fread(&ListObjets[i].zv.ZVZ1,2,1,fHandle);
        ListObjets[i].zv.ZVZ1 = (s16)ListObjets[i].zv.ZVZ1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ2) == 2);
        fread(&ListObjets[i].zv.ZVZ2,2,1,fHandle);
        ListObjets[i].zv.ZVZ2 = (s16)ListObjets[i].zv.ZVZ2;

        ASSERT(sizeof(ListObjets[i].screenXMin) == 2);
        fread(&ListObjets[i].screenXMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMin) == 2);
        fread(&ListObjets[i].screenYMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMax) == 2);
        fread(&ListObjets[i].screenXMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMax) == 2);
        fread(&ListObjets[i].screenYMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomX) == 2);
        fread(&ListObjets[i].roomX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomY) == 2);
        fread(&ListObjets[i].roomY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomZ) == 2);
        fread(&ListObjets[i].roomZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldX) == 2);
        fread(&ListObjets[i].worldX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldY) == 2);
        fread(&ListObjets[i].worldY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldZ) == 2);
        fread(&ListObjets[i].worldZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].alpha) == 2);
        fread(&ListObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].beta) == 2);
        fread(&ListObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].gamma) == 2);
        fread(&ListObjets[i].gamma,2,1,fHandle);

        // The original saves the floor first (s_layout.stageFirst)
        if (s_layout.stageFirst)
        {
            fread(&ListObjets[i].stage,2,1,fHandle);
            fread(&ListObjets[i].room,2,1,fHandle);
        }
        else
        {
            fread(&ListObjets[i].room,2,1,fHandle);
            fread(&ListObjets[i].stage,2,1,fHandle);
        }

        ASSERT(sizeof(ListObjets[i].lifeMode) == 2);
        fread(&ListObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].life) == 2);
        fread(&ListObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].CHRONO) == 4);
        fread(&ListObjets[i].CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ROOM_CHRONO) == 4);
        fread(&ListObjets[i].ROOM_CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ANIM) == 2);
        fread(&ListObjets[i].ANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animType) == 2);
        fread(&ListObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animInfo) == 2);
        fread(&ListObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnim) == 2);
        fread(&ListObjets[i].newAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimType) == 2);
        fread(&ListObjets[i].newAnimType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimInfo) == 2);
        fread(&ListObjets[i].newAnimInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].frame) == 2);
        fread(&ListObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].numOfFrames) == 2);
        fread(&ListObjets[i].numOfFrames,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].END_FRAME) == 2);
        fread(&ListObjets[i].END_FRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].flagEndAnim) == 2);
        fread(&ListObjets[i].flagEndAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackMode) == 2);
        fread(&ListObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackNumber) == 2);
        fread(&ListObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].MARK) == 2);
        fread(&ListObjets[i].MARK,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].positionInTrack) == 2);
        fread(&ListObjets[i].positionInTrack,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepX) == 2);
        fread(&ListObjets[i].stepX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepY) == 2);
        fread(&ListObjets[i].stepY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepZ) == 2); // 45
        fread(&ListObjets[i].stepZ,2,1,fHandle);

        loadInterpolatedValue(&ListObjets[i].YHandler,fHandle);

        ASSERT(sizeof(ListObjets[i].falling) == 2);
        fread(&ListObjets[i].falling,2,1,fHandle);

        loadInterpolatedValue(&ListObjets[i].rotate,fHandle);

        ASSERT(sizeof(ListObjets[i].direction) == 2); 
        fread(&ListObjets[i].direction,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].speed) == 2);
        fread(&ListObjets[i].speed,2,1,fHandle);

        loadInterpolatedValue(&ListObjets[i].speedChange,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[0]) == 2);
        fread(&ListObjets[i].COL[0],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[1]) == 2);
        fread(&ListObjets[i].COL[1],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[2]) == 2);
        fread(&ListObjets[i].COL[2],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL_BY) == 2);
        fread(&ListObjets[i].COL_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_DEC) == 2);
        fread(&ListObjets[i].HARD_DEC,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_COL) == 2);
        fread(&ListObjets[i].HARD_COL,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT) == 2);
        fread(&ListObjets[i].HIT,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT_BY) == 2);
        fread(&ListObjets[i].HIT_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionType) == 2);
        fread(&ListObjets[i].animActionType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionANIM) == 2);
        fread(&ListObjets[i].animActionANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionFRAME) == 2);
        fread(&ListObjets[i].animActionFRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionParam) == 2);
        fread(&ListObjets[i].animActionParam,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hitForce) == 2);
        fread(&ListObjets[i].hitForce,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPointID) == 2);
        fread(&ListObjets[i].hotPointID,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.x) == 2);
        fread(&ListObjets[i].hotPoint.x,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.y) == 2);
        fread(&ListObjets[i].hotPoint.y,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.z) == 2);
        fread(&ListObjets[i].hotPoint.z,2,1,fHandle);
    }
    fclose(fHandle);

    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        if(ListObjets[i].indexInWorld != -1 && ListObjets[i].bodyNum != -1)
        {
            sBody* bodyPtr = HQR_Get(HQ_Bodys,ListObjets[i].bodyNum);

            if(ListObjets[i].ANIM != -1)
            {
                sAnimation* animPtr = HQR_Get(HQ_Anims,ListObjets[i].ANIM);
                SetAnimObjet(ListObjets[i].frame,animPtr,bodyPtr);
            }
        }
    }

    NewNumCamera = var_E;

    return(1);
}

int restoreSave(int arg0, int arg1)
{
    int selectedSave;
    //  restoreSaveVar1 = arg0;

    if(arg1==0)
    {
        flushScreen();
        osystem_flip(NULL);
        FadeInPhys(0x40,0);
    }

    selectedSave = parseAllSaves(0);

    if(arg1==0)
    {
        //  fadeOut(8,0);
    }

    if(selectedSave == -1)
    {
        return(0);
    }

    if(arg1==0)
    {
        //freeScene();
    }

    return(loadSave(selectedSave));

}

int makeSaveFile(int entry)
{
    FILE* fHandle;
    char buffer[100];
    int i;
    unsigned long int var28 = 0;
    int temp;
    int oldNumMaxObj;

	if(g_gameId == AITD1)
	{
        // For safety, destroy special objects before mallocs
		for(i=0;i<NUM_MAX_OBJECT;i++)
		{
            if (ListObjets[i].indexInWorld == -2) // Special objects
            {
                ListObjets[i].indexInWorld = -1;
                if (ListObjets[i].ANIM == 4)
                {
                    CVars[getCVarsIdx(FOG_FLAG)] = 0;
                    HQ_Free_Malloc(HQ_Memory, ListObjets[i].frame);
                }
            }
		}
	}

    std::error_code folderError;
    if (savesFolder()[0])
        std::filesystem::create_directories(savesFolder(), folderError);
    strcpy_s(buffer, sizeof(buffer), saveFilePath(entry, ".ITD").c_str());

    fopen_s(&fHandle, buffer,"wb+");

    if(!fHandle)
        return 0;

    s_layout = (g_gameId == AITD1) ? kOriginalLayout : kEngineLayout;
    if (g_gameId == AITD1)
    {
        // As the original: the picture, its padding, then the data; the
        // offsets of the variables and the objects are filled in below
        const u32 pictureOffset = 20;
        const u32 paddingOffset = pictureOffset + kThumbnailWidth * kThumbnailHeight;
        const u32 dataOffset = paddingOffset + kThumbnailPadding;
        const u32 header[5] = { swapBytes(pictureOffset), swapBytes(paddingOffset), swapBytes(dataOffset), 0, 0 };
        fwrite(header, 4, 5, fHandle);

        // The camera picture, a pixel out of every 4x4 (palette indexes)
        unsigned char thumbnail[kThumbnailWidth * kThumbnailHeight] = {};
        if (aux)
            for (int y = 0; y < kThumbnailHeight; y++)
                for (int x = 0; x < kThumbnailWidth; x++)
                    thumbnail[y * kThumbnailWidth + x] = (unsigned char)aux[(y * 4) * 320 + x * 4];
        fwrite(thumbnail, sizeof(thumbnail), 1, fHandle);
        const unsigned char padding[kThumbnailPadding] = {};
        fwrite(padding, sizeof(padding), 1, fHandle);
    }
    else
    {
        fwrite(&var28,4,1,fHandle);
        fwrite(&var28,4,1,fHandle);

        var28 = ftell(fHandle)+12;
        var28 = swapBytes((u32)var28);

        fwrite(&var28,4,1,fHandle);

        fwrite(&var28,4,1,fHandle);
        fwrite(&var28,4,1,fHandle);
    }

    ASSERT(sizeof(currentRoom) == 2);
    fwrite(&currentRoom,2,1,fHandle);

    ASSERT(sizeof(g_currentFloor) == 2);
    fwrite(&g_currentFloor,2,1,fHandle);

    ASSERT(sizeof(NumCamera) == 2);
    fwrite(&NumCamera,2,1,fHandle);

    ASSERT(sizeof(currentWorldTarget) == 2);
    fwrite(&currentWorldTarget,2,1,fHandle);

    ASSERT(sizeof(currentCameraTargetActor) == 2);
    fwrite(&currentCameraTargetActor,2,1,fHandle);

    ASSERT(sizeof(maxObjects) == 2);
    fwrite(&maxObjects,2,1,fHandle);

    if(g_gameId == AITD1)
    {
        oldNumMaxObj = maxObjects;
        maxObjects = 300; // fix for save engine..
        ListWorldObjets.resize(maxObjects);
    }

    for(i=0;i<maxObjects;i++)
    {
        ASSERT(sizeof(ListWorldObjets[i].objIndex) == 2);
        fwrite(&ListWorldObjets[i].objIndex,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].body) == 2);
        fwrite(&ListWorldObjets[i].body,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].flags) == 2);
        fwrite(&ListWorldObjets[i].flags,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].typeZV) == 2);
        fwrite(&ListWorldObjets[i].typeZV,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundBody) == 2);
        fwrite(&ListWorldObjets[i].foundBody,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundName) == 2);
        fwrite(&ListWorldObjets[i].foundName,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundFlag) == 2);
        fwrite(&ListWorldObjets[i].foundFlag,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundLife) == 2);
        fwrite(&ListWorldObjets[i].foundLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].x) == 2);
        fwrite(&ListWorldObjets[i].x,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].y) == 2);
        fwrite(&ListWorldObjets[i].y,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].z) == 2);
        fwrite(&ListWorldObjets[i].z,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].alpha) == 2);
        fwrite(&ListWorldObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].beta) == 2);
        fwrite(&ListWorldObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].gamma) == 2);
        fwrite(&ListWorldObjets[i].gamma,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].stage) == 2);
        fwrite(&ListWorldObjets[i].stage,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].room) == 2);
        fwrite(&ListWorldObjets[i].room,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].lifeMode) == 2);
        fwrite(&ListWorldObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].life) == 2);
        fwrite(&ListWorldObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].floorLife) == 2);
        fwrite(&ListWorldObjets[i].floorLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].anim) == 2);
        fwrite(&ListWorldObjets[i].anim,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].frame) == 2);
        fwrite(&ListWorldObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animType) == 2);
        fwrite(&ListWorldObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animInfo) == 2);
        fwrite(&ListWorldObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackMode) == 2);
        fwrite(&ListWorldObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackNumber) == 2);
        fwrite(&ListWorldObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].positionInTrack) == 2);
        fwrite(&ListWorldObjets[i].positionInTrack,2,1,fHandle);
    }

    if(g_gameId == AITD1)
    {
        maxObjects = oldNumMaxObj;
    }

    if(g_gameId == AITD1)
    {
        ASSERT(CVars.size() == 45);
    }

    for(i=0;i< CVars.size();i++)
    {
        ASSERT(sizeof(CVars[i]) == 2);
        fwrite(&CVars[i], 2, 1, fHandle);
    }

	for(int inventoryId = 0; inventoryId<s_layout.inventories; inventoryId++)
	{
		ASSERT(sizeof(inHandTable[inventoryId]) == 2);
		fwrite(&inHandTable[inventoryId],2,1,fHandle);

		ASSERT(sizeof(numObjInInventoryTable[inventoryId]) == 2);
		fwrite(&numObjInInventoryTable[inventoryId],2,1,fHandle);

		if(g_gameId == AITD1)
		{
			ASSERT(INVENTORY_SIZE == 30);
		}

		for(i=0;i<INVENTORY_SIZE;i++)
		{
			ASSERT(sizeof(inventoryTable[inventoryId][i]) == 2);
			fwrite(&inventoryTable[inventoryId][i],2,1,fHandle);
		}
	}

    ASSERT(sizeof(statusScreenAllowed) == 2);
    fwrite(&statusScreenAllowed,2,1,fHandle);

    ASSERT(sizeof(FlagGameOver) == 2);
    fwrite(&FlagGameOver,2,1,fHandle);

    ASSERT(sizeof(lightOff) == 2);
    fwrite(&lightOff,2,1,fHandle);

    ASSERT(sizeof(shakingAmplitude) == 2);
    fwrite(&shakingAmplitude,2,1,fHandle);

    ASSERT(sizeof(shakeVar1) == 2);
    fwrite(&shakeVar1,2,1,fHandle);

    // The time the game stopped at, not the one running inside the menu
    const unsigned int savedClock = gameClockForSave();
    fwrite(&savedClock,4,1,fHandle);

    ASSERT(sizeof(timerFreeze1) == 4);
    fwrite(&timerFreeze1,4,1,fHandle);

    ASSERT(sizeof(currentMusic) == 2);
    fwrite(&currentMusic,2,1,fHandle);

    //timerFreeze = 1;

    var28 = ftell(fHandle);
    temp = var28;
    fseek(fHandle,12,SEEK_SET);
    var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);
    fwrite(&var28,4,1,fHandle);
    fseek(fHandle,temp,SEEK_SET);

    fwrite(&varSize,2,1,fHandle);
    fwrite(vars,varSize,1,fHandle);

    var28 = ftell(fHandle);
    temp = var28;
    fseek(fHandle,16,SEEK_SET);
    var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);
    fwrite(&var28,4,1,fHandle);
    fseek(fHandle,temp,SEEK_SET);

    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        ASSERT(sizeof(ListObjets[i].indexInWorld) == 2);
        fwrite(&ListObjets[i].indexInWorld,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].bodyNum) == 2);
        fwrite(&ListObjets[i].bodyNum,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].objectType) == 2);
        fwrite(&ListObjets[i].objectType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].dynFlags) == 2);
        fwrite(&ListObjets[i].dynFlags,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX1) == 2);
        fwrite(&ListObjets[i].zv.ZVX1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX2) == 2);
        fwrite(&ListObjets[i].zv.ZVX2,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVY1) == 2);
        fwrite(&ListObjets[i].zv.ZVY1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVY2) == 2);
        fwrite(&ListObjets[i].zv.ZVY2,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ1) == 2);
        fwrite(&ListObjets[i].zv.ZVZ1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ2) == 2);
        fwrite(&ListObjets[i].zv.ZVZ2,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMin) == 2);
        fwrite(&ListObjets[i].screenXMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMin) == 2);
        fwrite(&ListObjets[i].screenYMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMax) == 2);
        fwrite(&ListObjets[i].screenXMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMax) == 2);
        fwrite(&ListObjets[i].screenYMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomX) == 2);
        fwrite(&ListObjets[i].roomX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomY) == 2);
        fwrite(&ListObjets[i].roomY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomZ) == 2);
        fwrite(&ListObjets[i].roomZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldX) == 2);
        fwrite(&ListObjets[i].worldX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldY) == 2);
        fwrite(&ListObjets[i].worldY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldZ) == 2);
        fwrite(&ListObjets[i].worldZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].alpha) == 2);
        fwrite(&ListObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].beta) == 2);
        fwrite(&ListObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].gamma) == 2);
        fwrite(&ListObjets[i].gamma,2,1,fHandle);

        // The original saves the floor first (s_layout.stageFirst)
        if (s_layout.stageFirst)
        {
            fwrite(&ListObjets[i].stage,2,1,fHandle);
            fwrite(&ListObjets[i].room,2,1,fHandle);
        }
        else
        {
            fwrite(&ListObjets[i].room,2,1,fHandle);
            fwrite(&ListObjets[i].stage,2,1,fHandle);
        }

        ASSERT(sizeof(ListObjets[i].lifeMode) == 2);
        fwrite(&ListObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].life) == 2);
        fwrite(&ListObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].CHRONO) == 4);
        fwrite(&ListObjets[i].CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ROOM_CHRONO) == 4);
        fwrite(&ListObjets[i].ROOM_CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ANIM) == 2);
        fwrite(&ListObjets[i].ANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animType) == 2);
        fwrite(&ListObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animInfo) == 2);
        fwrite(&ListObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnim) == 2);
        fwrite(&ListObjets[i].newAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimType) == 2);
        fwrite(&ListObjets[i].newAnimType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimInfo) == 2);
        fwrite(&ListObjets[i].newAnimInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].frame) == 2);
        fwrite(&ListObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].numOfFrames) == 2);
        fwrite(&ListObjets[i].numOfFrames,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].END_FRAME) == 2);
        fwrite(&ListObjets[i].END_FRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].flagEndAnim) == 2);
        fwrite(&ListObjets[i].flagEndAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackMode) == 2);
        fwrite(&ListObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackNumber) == 2);
        fwrite(&ListObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].MARK) == 2);
        fwrite(&ListObjets[i].MARK,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].positionInTrack) == 2);
        fwrite(&ListObjets[i].positionInTrack,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepX) == 2);
        fwrite(&ListObjets[i].stepX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepY) == 2);
        fwrite(&ListObjets[i].stepY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepZ) == 2); // 45
        fwrite(&ListObjets[i].stepZ,2,1,fHandle);

        saveInterpolatedValue(&ListObjets[i].YHandler,fHandle);

        ASSERT(sizeof(ListObjets[i].falling) == 2);
        fwrite(&ListObjets[i].falling,2,1,fHandle);

        saveInterpolatedValue(&ListObjets[i].rotate,fHandle);

        ASSERT(sizeof(ListObjets[i].direction) == 2); 
        fwrite(&ListObjets[i].direction,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].speed) == 2);
        fwrite(&ListObjets[i].speed,2,1,fHandle);

        saveInterpolatedValue(&ListObjets[i].speedChange,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[0]) == 2);
        fwrite(&ListObjets[i].COL[0],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[1]) == 2);
        fwrite(&ListObjets[i].COL[1],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[2]) == 2);
        fwrite(&ListObjets[i].COL[2],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL_BY) == 2);
        fwrite(&ListObjets[i].COL_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_DEC) == 2);
        fwrite(&ListObjets[i].HARD_DEC,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_COL) == 2);
        fwrite(&ListObjets[i].HARD_COL,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT) == 2);
        fwrite(&ListObjets[i].HIT,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT_BY) == 2);
        fwrite(&ListObjets[i].HIT_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionType) == 2);
        fwrite(&ListObjets[i].animActionType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionANIM) == 2);
        fwrite(&ListObjets[i].animActionANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionFRAME) == 2);
        fwrite(&ListObjets[i].animActionFRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionParam) == 2);
        fwrite(&ListObjets[i].animActionParam,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hitForce) == 2);
        fwrite(&ListObjets[i].hitForce,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPointID) == 2);
        fwrite(&ListObjets[i].hotPointID,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.x) == 2);
        fwrite(&ListObjets[i].hotPoint.x,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.y) == 2);
        fwrite(&ListObjets[i].hotPoint.y,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.z) == 2);
        fwrite(&ListObjets[i].hotPoint.z,2,1,fHandle);
    }

    fclose(fHandle);

    // Save screenshot PNG alongside save file
    std::filesystem::path pngPath = std::filesystem::path(saveFilePath(entry, ".png"));
    unsigned char* previewData = osystem_getScenePreviewData();
    int previewW = osystem_getScenePreviewWidth();
    int previewH = osystem_getScenePreviewHeight();
    if (previewData && previewW > 0 && previewH > 0)
    {
        int width, height;
        const std::vector<unsigned char> preview = shrinkPreview(previewData, previewW, previewH, width, height);
        writePNG(pngPath.string().c_str(), preview.data(), width, height);
    }
    else
    {
        // Remove stale preview PNG so old screenshots don't persist
        std::error_code ec;
        std::filesystem::remove(pngPath, ec);
    }

    return 1;
}

int makeSave(int arg0)
{
    // Show save slot selection screen (arg=1 means save mode - can save to any slot)
    int selectedSlot = parseAllSaves(1);

    if(selectedSlot == -1)
    {
        // User cancelled
        return 0;
    }

    return makeSaveFile(selectedSlot);
}

