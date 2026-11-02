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
// AITD2 saves are written as its original (GOG) too (BUGS.md A6): the picture
// and the palette, the world and the variables ciphered, room for 400 world
// objects, inventories of 50 and objects with fields AITD1 has not.
// Older saves of this engine (two inventories, 32-bit times, no picture) and
// Jack keep the engine's layout. loadSave works out which one a file uses
// from its header and the size of its blocks.
struct SaveLayout
{
    int inventories;     // AITD1 has one; the engine writes NUM_MAX_INVENTORY
    int memoTicksSize;   // 2 (original) or 4 bytes
    bool stageFirst;     // objects: the original saves the floor before the room
    bool aitd2;          // AITD2's original layout
};
static SaveLayout s_layout = { NUM_MAX_INVENTORY, 4, false, false };
static const SaveLayout kOriginalLayout = { 1, 2, true, false };
static const SaveLayout kOriginalAITD2Layout = { NUM_MAX_INVENTORY, 2, true, true };
static const SaveLayout kEngineLayout = { NUM_MAX_INVENTORY, 4, false, false };

// AITD2's original: every byte of the world and of the variables is xored
// with 0x35; room for 400 world objects (27 values each, the last one MARK);
// 50 objects per inventory; the first 4 bytes of the picture hold where the
// objects start
static const u8 kAITD2Cipher = 0x35;
static const int kAITD2WorldSlots = 400;
static const int kAITD2WorldObjectSize = 27 * 2;
static const int kAITD2InventorySize = 50;
static const u32 kAITD2PaletteOffset = 20 + 80 * 50;

// The cipher of the bytes read and written now (0: plain)
static u8 s_cipher = 0;

static size_t saveRead(void* data, size_t size, size_t count, FILE* fHandle)
{
    const size_t read = fread(data, size, count, fHandle);
    if (s_cipher)
        for (size_t i = 0; i < size * count; i++)
            ((u8*)data)[i] ^= s_cipher;
    return read;
}

static size_t saveWrite(const void* data, size_t size, size_t count, FILE* fHandle)
{
    if (!s_cipher)
        return fwrite(data, size, count, fHandle);
    std::vector<u8> ciphered((const u8*)data, (const u8*)data + size * count);
    for (u8& value : ciphered)
        value ^= s_cipher;
    return fwrite(ciphered.data(), size, count, fHandle);
}

// A value of AITD2's original whose use is unknown: skipped when loading,
// written as the original has it in a free object
static void skipValue(FILE* fHandle)
{
    s16 unknown;
    saveRead(&unknown, 2, 1, fHandle);
}
static void writeValue(s16 value, FILE* fHandle)
{
    saveWrite(&value, 2, 1, fHandle);
}

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
    saveRead(&pRotateStruct->startValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->endValue) == 2);
    saveRead(&pRotateStruct->endValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->numSteps) == 2);
    saveRead(&pRotateStruct->numSteps,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->memoTicks) == 4);
    if (s_layout.memoTicksSize == 2)
    {
        // Only the low 16 bits were saved: the same time ago from the
        // restored clock (timer is read before the objects)
        u16 memo16 = 0;
        saveRead(&memo16, 2, 1, fHandle);
        pRotateStruct->memoTicks = timer - (u16)((u16)timer - memo16);
    }
    else
        saveRead(&pRotateStruct->memoTicks,4,1,fHandle);
}

void saveInterpolatedValue(RealValue* pRotateStruct, FILE* fHandle)
{
    ASSERT(sizeof(pRotateStruct->startValue) == 2);
    saveWrite(&pRotateStruct->startValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->endValue) == 2);
    saveWrite(&pRotateStruct->endValue,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->numSteps) == 2);
    saveWrite(&pRotateStruct->numSteps,2,1,fHandle);

    ASSERT(sizeof(pRotateStruct->memoTicks) == 4);
    if (s_layout.memoTicksSize == 2)
    {
        const u16 memo16 = (u16)pRotateStruct->memoTicks;
        saveWrite(&memo16, 2, 1, fHandle);
    }
    else
        saveWrite(&pRotateStruct->memoTicks,4,1,fHandle);
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
    s_cipher = 0;

    // Which layout (s_layout): AITD1 saves come in two. The data block holds
    // the room, 300 objects, the variables, the inventories and the clock;
    // its size tells how many inventories, and the size of the objects block
    // (50 of them) how long the times are.
    s_layout = kEngineLayout;
    if (g_gameId == AITD1)
    {
        u32 offsets[5] = {};
        saveRead(offsets, 4, 5, fHandle);
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
    // AITD2's original starts with the picture (at 20) and the palette after it
    else if (g_gameId == AITD2)
    {
        u32 offsets[2] = {};
        saveRead(offsets, 4, 2, fHandle);
        if (swapBytes(offsets[0]) == 20 && swapBytes(offsets[1]) == kAITD2PaletteOffset)
            s_layout = kOriginalAITD2Layout;
        printf(MAIN_TAG "Save %d: %s layout\n", saveNumber, s_layout.aitd2 ? "original (GOG)" : "older engine");
    }

    // The header: where the data, the variables and the objects start (AITD2's
    // original has its palette before and the objects right after the variables)
    const int headerShift = s_layout.aitd2 ? 4 : 0;

    fseek(fHandle,8 + headerShift,SEEK_SET);

    saveRead(&var28,4,1,fHandle);

    var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);

    fseek(fHandle,var28,SEEK_SET);
    s_cipher = s_layout.aitd2 ? kAITD2Cipher : 0;

    ASSERT(sizeof(currentRoom) == 2);
    saveRead(&currentRoom,2,1,fHandle);

    ASSERT(sizeof(g_currentFloor) == 2);
    saveRead(&g_currentFloor,2,1,fHandle);

    ASSERT(sizeof(NumCamera) == 2);
    saveRead(&NumCamera,2,1,fHandle);

    ASSERT(sizeof(currentWorldTarget) == 2);
    saveRead(&currentWorldTarget,2,1,fHandle);

    ASSERT(sizeof(currentCameraTargetActor) == 2);
    saveRead(&currentCameraTargetActor,2,1,fHandle);

    ASSERT(sizeof(maxObjects) == 2);
    saveRead(&maxObjects,2,1,fHandle);

    if(g_gameId == AITD1)
    {
        oldNumMaxObj = maxObjects;
        maxObjects = 300; // fix for save engine..
        ListWorldObjets.resize(maxObjects);
    }

    for(i=0;i<maxObjects;i++)
    {
        ASSERT(sizeof(ListWorldObjets[i].objIndex) == 2);
        saveRead(&ListWorldObjets[i].objIndex,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].body) == 2);
        saveRead(&ListWorldObjets[i].body,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].flags) == 2);
        saveRead(&ListWorldObjets[i].flags,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].typeZV) == 2);
        saveRead(&ListWorldObjets[i].typeZV,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundBody) == 2);
        saveRead(&ListWorldObjets[i].foundBody,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundName) == 2);
        saveRead(&ListWorldObjets[i].foundName,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundFlag) == 2);
        saveRead(&ListWorldObjets[i].foundFlag,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundLife) == 2);
        saveRead(&ListWorldObjets[i].foundLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].x) == 2);
        saveRead(&ListWorldObjets[i].x,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].y) == 2);
        saveRead(&ListWorldObjets[i].y,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].z) == 2);
        saveRead(&ListWorldObjets[i].z,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].alpha) == 2);
        saveRead(&ListWorldObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].beta) == 2);
        saveRead(&ListWorldObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].gamma) == 2);
        saveRead(&ListWorldObjets[i].gamma,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].stage) == 2);
        saveRead(&ListWorldObjets[i].stage,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].room) == 2);
        saveRead(&ListWorldObjets[i].room,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].lifeMode) == 2);
        saveRead(&ListWorldObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].life) == 2);
        saveRead(&ListWorldObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].floorLife) == 2);
        saveRead(&ListWorldObjets[i].floorLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].anim) == 2);
        saveRead(&ListWorldObjets[i].anim,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].frame) == 2);
        saveRead(&ListWorldObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animType) == 2);
        saveRead(&ListWorldObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animInfo) == 2);
        saveRead(&ListWorldObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackMode) == 2);
        saveRead(&ListWorldObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackNumber) == 2);
        saveRead(&ListWorldObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].positionInTrack) == 2);
        saveRead(&ListWorldObjets[i].positionInTrack,2,1,fHandle);

        if (s_layout.aitd2)
            saveRead(&ListWorldObjets[i].mark,2,1,fHandle);
    }
    if (s_layout.aitd2)
        fseek(fHandle, (kAITD2WorldSlots - maxObjects) * kAITD2WorldObjectSize, SEEK_CUR);

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
        saveRead(&CVars[i], 2, 1, fHandle);
    }

	// AITD1 uses one inventory: the second (AITD2/3) stays empty
	for(int inventoryId=s_layout.inventories; inventoryId<NUM_MAX_INVENTORY; inventoryId++)
	{
		inHandTable[inventoryId] = -1;
		numObjInInventoryTable[inventoryId] = 0;
	}
	// AITD2's original: both objects in hand, both counts, then both lists
	if (s_layout.aitd2)
	{
		saveRead(inHandTable, 2, NUM_MAX_INVENTORY, fHandle);
		saveRead(numObjInInventoryTable, 2, NUM_MAX_INVENTORY, fHandle);
		for(int inventoryId=0; inventoryId<NUM_MAX_INVENTORY; inventoryId++)
		{
			saveRead(inventoryTable[inventoryId], 2, INVENTORY_SIZE, fHandle);
			fseek(fHandle, (kAITD2InventorySize - INVENTORY_SIZE) * 2, SEEK_CUR);
			if (numObjInInventoryTable[inventoryId] > INVENTORY_SIZE)
				numObjInInventoryTable[inventoryId] = INVENTORY_SIZE;
		}
	}
	else for(int inventoryId=0; inventoryId<s_layout.inventories; inventoryId++)
	{
		ASSERT(sizeof(inHandTable[inventoryId]) == 2);
		saveRead(&inHandTable[inventoryId],2,1,fHandle);

		ASSERT(sizeof(numObjInInventoryTable[inventoryId]) == 2);
		saveRead(&numObjInInventoryTable[inventoryId],2,1,fHandle);

		if(g_gameId == AITD1)
		{
			ASSERT(INVENTORY_SIZE == 30);
		}

		for(i=0;i<INVENTORY_SIZE;i++)
		{
			ASSERT(sizeof(inventoryTable[inventoryId][i]) == 2);
			saveRead(&inventoryTable[inventoryId][i],2,1,fHandle);
		}
	}

    ASSERT(sizeof(statusScreenAllowed) == 2);
    saveRead(&statusScreenAllowed,2,1,fHandle);

    ASSERT(sizeof(FlagGameOver) == 2);
    saveRead(&FlagGameOver,2,1,fHandle);

    ASSERT(sizeof(lightOff) == 2);
    saveRead(&lightOff,2,1,fHandle);

    ASSERT(sizeof(shakingAmplitude) == 2);
    saveRead(&shakingAmplitude,2,1,fHandle);

    ASSERT(sizeof(shakeVar1) == 2);
    saveRead(&shakeVar1,2,1,fHandle);

    if (s_layout.aitd2)
        skipValue(fHandle);

    ASSERT(sizeof(timer) == 4);
    saveRead(&timer,4,1,fHandle);
    // The clock itself, not only its copy: process_events sets timer from
    // timeGlobal every frame, which wiped the saved time (BUGS.md C29)
    setGameClock(timer);
    resetFoundObjectChronos();   // the lamp in hand counts again from now

    ASSERT(sizeof(timerFreeze1) == 4);
    saveRead(&timerFreeze1,4,1,fHandle);

    ASSERT(sizeof(currentMusic) == 2);
    saveRead(&currentMusic,2,1,fHandle);

    //timerFreeze = 1;

    var_E = NumCamera;

    LoadEtage(g_currentFloor);
    NumCamera = -1;
    ChangeSalle(currentRoom);
    var_16 = currentMusic;
    currentMusic = -1;
    playMusic(var_16);

    s_cipher = 0;
    fseek(fHandle,12 + headerShift,SEEK_SET);
    saveRead(&offsetToVars,4,1,fHandle);
    offsetToVars = ((offsetToVars & 0xFF) << 24) | ((offsetToVars & 0xFF00) << 8) | (( offsetToVars & 0xFF0000) >> 8) | ((offsetToVars & 0xFF000000) >> 24);
    fseek(fHandle,offsetToVars,SEEK_SET);

    saveRead(&tempVarSize,2,1,fHandle);
    {
        int savedVarSize = varSize; // varSize set by LoadWorld from VARS.ITD
        varSize = tempVarSize;
        if (varSize > savedVarSize)
            varSize = savedVarSize; // clamp to allocated buffer size
    }

    s_cipher = s_layout.aitd2 ? kAITD2Cipher : 0;
    saveRead(vars,varSize,1,fHandle);
    s_cipher = 0;

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

    if (s_layout.aitd2)
        offsetToActors = offsetToVars + 2 + tempVarSize;
    else
    {
        fseek(fHandle,16,SEEK_SET);
        saveRead(&offsetToActors,4,1,fHandle);
        offsetToActors = ((offsetToActors & 0xFF) << 24) | ((offsetToActors & 0xFF00) << 8) | (( offsetToActors & 0xFF0000) >> 8) | ((offsetToActors & 0xFF000000) >> 24);
    }
    fseek(fHandle,offsetToActors,SEEK_SET);

    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        ASSERT(sizeof(ListObjets[i].indexInWorld) == 2);
        saveRead(&ListObjets[i].indexInWorld,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].bodyNum) == 2);
        saveRead(&ListObjets[i].bodyNum,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].objectType) == 2);
        saveRead(&ListObjets[i].objectType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].dynFlags) == 2);
        saveRead(&ListObjets[i].dynFlags,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX1) == 2);
        saveRead(&ListObjets[i].zv.ZVX1,2,1,fHandle);
        ListObjets[i].zv.ZVX1 = (s16)ListObjets[i].zv.ZVX1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVX2) == 2);
        saveRead(&ListObjets[i].zv.ZVX2,2,1,fHandle);
        ListObjets[i].zv.ZVX2 = (s16)ListObjets[i].zv.ZVX2;

        //    ASSERT(sizeof(actorTable[i].zv.ZVY1) == 2);
        saveRead(&ListObjets[i].zv.ZVY1,2,1,fHandle);
        ListObjets[i].zv.ZVY1 = (s16)ListObjets[i].zv.ZVY1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVY2) == 2);
        saveRead(&ListObjets[i].zv.ZVY2,2,1,fHandle);
        ListObjets[i].zv.ZVY2 = (s16)ListObjets[i].zv.ZVY2;

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ1) == 2);
        saveRead(&ListObjets[i].zv.ZVZ1,2,1,fHandle);
        ListObjets[i].zv.ZVZ1 = (s16)ListObjets[i].zv.ZVZ1;

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ2) == 2);
        saveRead(&ListObjets[i].zv.ZVZ2,2,1,fHandle);
        ListObjets[i].zv.ZVZ2 = (s16)ListObjets[i].zv.ZVZ2;

        ASSERT(sizeof(ListObjets[i].screenXMin) == 2);
        saveRead(&ListObjets[i].screenXMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMin) == 2);
        saveRead(&ListObjets[i].screenYMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMax) == 2);
        saveRead(&ListObjets[i].screenXMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMax) == 2);
        saveRead(&ListObjets[i].screenYMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomX) == 2);
        saveRead(&ListObjets[i].roomX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomY) == 2);
        saveRead(&ListObjets[i].roomY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomZ) == 2);
        saveRead(&ListObjets[i].roomZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldX) == 2);
        saveRead(&ListObjets[i].worldX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldY) == 2);
        saveRead(&ListObjets[i].worldY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldZ) == 2);
        saveRead(&ListObjets[i].worldZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].alpha) == 2);
        saveRead(&ListObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].beta) == 2);
        saveRead(&ListObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].gamma) == 2);
        saveRead(&ListObjets[i].gamma,2,1,fHandle);

        // The original saves the floor first (s_layout.stageFirst)
        if (s_layout.stageFirst)
        {
            saveRead(&ListObjets[i].stage,2,1,fHandle);
            saveRead(&ListObjets[i].room,2,1,fHandle);
        }
        else
        {
            saveRead(&ListObjets[i].room,2,1,fHandle);
            saveRead(&ListObjets[i].stage,2,1,fHandle);
        }

        ASSERT(sizeof(ListObjets[i].lifeMode) == 2);
        saveRead(&ListObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].life) == 2);
        saveRead(&ListObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].CHRONO) == 4);
        saveRead(&ListObjets[i].CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ROOM_CHRONO) == 4);
        saveRead(&ListObjets[i].ROOM_CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ANIM) == 2);
        saveRead(&ListObjets[i].ANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animType) == 2);
        saveRead(&ListObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animInfo) == 2);
        saveRead(&ListObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnim) == 2);
        saveRead(&ListObjets[i].newAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimType) == 2);
        saveRead(&ListObjets[i].newAnimType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimInfo) == 2);
        saveRead(&ListObjets[i].newAnimInfo,2,1,fHandle);

        if (s_layout.aitd2)
        {
            skipValue(fHandle);
            skipValue(fHandle);
        }

        ASSERT(sizeof(ListObjets[i].frame) == 2);
        saveRead(&ListObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].numOfFrames) == 2);
        saveRead(&ListObjets[i].numOfFrames,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].END_FRAME) == 2);
        saveRead(&ListObjets[i].END_FRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].flagEndAnim) == 2);
        saveRead(&ListObjets[i].flagEndAnim,2,1,fHandle);

        if (s_layout.aitd2)
            saveRead(&ListObjets[i].memoTicks,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackMode) == 2);
        saveRead(&ListObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackNumber) == 2);
        saveRead(&ListObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].MARK) == 2);
        saveRead(&ListObjets[i].MARK,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].positionInTrack) == 2);
        saveRead(&ListObjets[i].positionInTrack,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepX) == 2);
        saveRead(&ListObjets[i].stepX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepY) == 2);
        saveRead(&ListObjets[i].stepY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepZ) == 2); // 45
        saveRead(&ListObjets[i].stepZ,2,1,fHandle);

        if (s_layout.aitd2)
        {
            saveRead(&ListObjets[i].animNegX,2,1,fHandle);
            saveRead(&ListObjets[i].animNegY,2,1,fHandle);
            saveRead(&ListObjets[i].animNegZ,2,1,fHandle);
        }

        loadInterpolatedValue(&ListObjets[i].YHandler,fHandle);

        ASSERT(sizeof(ListObjets[i].falling) == 2);
        saveRead(&ListObjets[i].falling,2,1,fHandle);

        loadInterpolatedValue(&ListObjets[i].rotate,fHandle);

        ASSERT(sizeof(ListObjets[i].direction) == 2); 
        saveRead(&ListObjets[i].direction,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].speed) == 2);
        saveRead(&ListObjets[i].speed,2,1,fHandle);

        loadInterpolatedValue(&ListObjets[i].speedChange,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[0]) == 2);
        saveRead(&ListObjets[i].COL[0],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[1]) == 2);
        saveRead(&ListObjets[i].COL[1],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[2]) == 2);
        saveRead(&ListObjets[i].COL[2],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL_BY) == 2);
        saveRead(&ListObjets[i].COL_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_DEC) == 2);
        saveRead(&ListObjets[i].HARD_DEC,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_COL) == 2);
        saveRead(&ListObjets[i].HARD_COL,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT) == 2);
        saveRead(&ListObjets[i].HIT,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT_BY) == 2);
        saveRead(&ListObjets[i].HIT_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionType) == 2);
        saveRead(&ListObjets[i].animActionType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionANIM) == 2);
        saveRead(&ListObjets[i].animActionANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionFRAME) == 2);
        saveRead(&ListObjets[i].animActionFRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionParam) == 2);
        saveRead(&ListObjets[i].animActionParam,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hitForce) == 2);
        saveRead(&ListObjets[i].hitForce,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPointID) == 2);
        saveRead(&ListObjets[i].hotPointID,2,1,fHandle);

        if (s_layout.aitd2)
            skipValue(fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.x) == 2);
        saveRead(&ListObjets[i].hotPoint.x,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.y) == 2);
        saveRead(&ListObjets[i].hotPoint.y,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.z) == 2);
        saveRead(&ListObjets[i].hotPoint.z,2,1,fHandle);

        if (s_layout.aitd2)
        {
            saveRead(&ListObjets[i].hardMat,2,1,fHandle);
            skipValue(fHandle);
        }
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

    // Special objects (smoke, sparks...) are not saved: their data lives in
    // memory that a load does not bring back (all games, BUGS.md A7)
    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        if (ListObjets[i].indexInWorld == -2)
            DeleteObjet(i);
    }

    std::error_code folderError;
    if (savesFolder()[0])
        std::filesystem::create_directories(savesFolder(), folderError);
    strcpy_s(buffer, sizeof(buffer), saveFilePath(entry, ".ITD").c_str());

    fopen_s(&fHandle, buffer,"wb+");

    if(!fHandle)
        return 0;

    s_cipher = 0;
    s_layout = (g_gameId == AITD1) ? kOriginalLayout : (g_gameId == AITD2) ? kOriginalAITD2Layout : kEngineLayout;
    const int headerShift = s_layout.aitd2 ? 4 : 0;
    if (g_gameId == AITD1 || s_layout.aitd2)
    {
        // As the original: the picture, (AITD2: its palette,) its padding,
        // then the data; the offsets of the variables (and, in AITD1, of the
        // objects) are filled in below
        const u32 pictureOffset = 20;
        const u32 paletteOffset = pictureOffset + kThumbnailWidth * kThumbnailHeight;
        const u32 paddingOffset = s_layout.aitd2 ? paletteOffset + 768 : paletteOffset;
        const u32 dataOffset = paddingOffset + kThumbnailPadding;
        const u32 header[5] = { swapBytes(pictureOffset), swapBytes(s_layout.aitd2 ? paletteOffset : paddingOffset),
            swapBytes(s_layout.aitd2 ? paddingOffset : dataOffset), s_layout.aitd2 ? swapBytes(dataOffset) : 0, 0 };
        saveWrite(header, 4, 5, fHandle);

        // The camera picture, a pixel out of every 4x4 (palette indexes)
        unsigned char thumbnail[kThumbnailWidth * kThumbnailHeight] = {};
        if (aux)
            for (int y = 0; y < kThumbnailHeight; y++)
                for (int x = 0; x < kThumbnailWidth; x++)
                    thumbnail[y * kThumbnailWidth + x] = (unsigned char)aux[(y * 4) * 320 + x * 4];
        saveWrite(thumbnail, sizeof(thumbnail), 1, fHandle);
        // AITD2: the palette of the picture, in the game's 6-bit colours
        if (s_layout.aitd2)
        {
            u8 palette[768];
            for (int i = 0; i < 256; i++)
                for (int c = 0; c < 3; c++)
                    palette[i * 3 + c] = currentGamePalette[i][c] >> 2;
            saveWrite(palette, sizeof(palette), 1, fHandle);
        }
        const unsigned char padding[kThumbnailPadding] = {};
        saveWrite(padding, sizeof(padding), 1, fHandle);
        s_cipher = s_layout.aitd2 ? kAITD2Cipher : 0;
    }
    else
    {
        saveWrite(&var28,4,1,fHandle);
        saveWrite(&var28,4,1,fHandle);

        var28 = ftell(fHandle)+12;
        var28 = swapBytes((u32)var28);

        saveWrite(&var28,4,1,fHandle);

        saveWrite(&var28,4,1,fHandle);
        saveWrite(&var28,4,1,fHandle);
    }

    ASSERT(sizeof(currentRoom) == 2);
    saveWrite(&currentRoom,2,1,fHandle);

    ASSERT(sizeof(g_currentFloor) == 2);
    saveWrite(&g_currentFloor,2,1,fHandle);

    ASSERT(sizeof(NumCamera) == 2);
    saveWrite(&NumCamera,2,1,fHandle);

    ASSERT(sizeof(currentWorldTarget) == 2);
    saveWrite(&currentWorldTarget,2,1,fHandle);

    ASSERT(sizeof(currentCameraTargetActor) == 2);
    saveWrite(&currentCameraTargetActor,2,1,fHandle);

    ASSERT(sizeof(maxObjects) == 2);
    saveWrite(&maxObjects,2,1,fHandle);

    if(g_gameId == AITD1)
    {
        oldNumMaxObj = maxObjects;
        maxObjects = 300; // fix for save engine..
        ListWorldObjets.resize(maxObjects);
    }

    for(i=0;i<maxObjects;i++)
    {
        ASSERT(sizeof(ListWorldObjets[i].objIndex) == 2);
        saveWrite(&ListWorldObjets[i].objIndex,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].body) == 2);
        saveWrite(&ListWorldObjets[i].body,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].flags) == 2);
        saveWrite(&ListWorldObjets[i].flags,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].typeZV) == 2);
        saveWrite(&ListWorldObjets[i].typeZV,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundBody) == 2);
        saveWrite(&ListWorldObjets[i].foundBody,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundName) == 2);
        saveWrite(&ListWorldObjets[i].foundName,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundFlag) == 2);
        saveWrite(&ListWorldObjets[i].foundFlag,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].foundLife) == 2);
        saveWrite(&ListWorldObjets[i].foundLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].x) == 2);
        saveWrite(&ListWorldObjets[i].x,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].y) == 2);
        saveWrite(&ListWorldObjets[i].y,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].z) == 2);
        saveWrite(&ListWorldObjets[i].z,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].alpha) == 2);
        saveWrite(&ListWorldObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].beta) == 2);
        saveWrite(&ListWorldObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].gamma) == 2);
        saveWrite(&ListWorldObjets[i].gamma,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].stage) == 2);
        saveWrite(&ListWorldObjets[i].stage,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].room) == 2);
        saveWrite(&ListWorldObjets[i].room,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].lifeMode) == 2);
        saveWrite(&ListWorldObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].life) == 2);
        saveWrite(&ListWorldObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].floorLife) == 2);
        saveWrite(&ListWorldObjets[i].floorLife,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].anim) == 2);
        saveWrite(&ListWorldObjets[i].anim,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].frame) == 2);
        saveWrite(&ListWorldObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animType) == 2);
        saveWrite(&ListWorldObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].animInfo) == 2);
        saveWrite(&ListWorldObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackMode) == 2);
        saveWrite(&ListWorldObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].trackNumber) == 2);
        saveWrite(&ListWorldObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListWorldObjets[i].positionInTrack) == 2);
        saveWrite(&ListWorldObjets[i].positionInTrack,2,1,fHandle);

        if (s_layout.aitd2)
            saveWrite(&ListWorldObjets[i].mark,2,1,fHandle);
    }
    if (s_layout.aitd2)
    {
        const std::vector<u8> emptySlots((kAITD2WorldSlots - maxObjects) * kAITD2WorldObjectSize, 0);
        saveWrite(emptySlots.data(), 1, emptySlots.size(), fHandle);
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
        saveWrite(&CVars[i], 2, 1, fHandle);
    }

	// AITD2's original: both objects in hand, both counts, then both lists
	if (s_layout.aitd2)
	{
		saveWrite(inHandTable, 2, NUM_MAX_INVENTORY, fHandle);
		saveWrite(numObjInInventoryTable, 2, NUM_MAX_INVENTORY, fHandle);
		const s16 emptyEntries[kAITD2InventorySize - INVENTORY_SIZE] = {};
		for(int inventoryId=0; inventoryId<NUM_MAX_INVENTORY; inventoryId++)
		{
			saveWrite(inventoryTable[inventoryId], 2, INVENTORY_SIZE, fHandle);
			saveWrite(emptyEntries, 2, kAITD2InventorySize - INVENTORY_SIZE, fHandle);
		}
	}
	else for(int inventoryId = 0; inventoryId<s_layout.inventories; inventoryId++)
	{
		ASSERT(sizeof(inHandTable[inventoryId]) == 2);
		saveWrite(&inHandTable[inventoryId],2,1,fHandle);

		ASSERT(sizeof(numObjInInventoryTable[inventoryId]) == 2);
		saveWrite(&numObjInInventoryTable[inventoryId],2,1,fHandle);

		if(g_gameId == AITD1)
		{
			ASSERT(INVENTORY_SIZE == 30);
		}

		for(i=0;i<INVENTORY_SIZE;i++)
		{
			ASSERT(sizeof(inventoryTable[inventoryId][i]) == 2);
			saveWrite(&inventoryTable[inventoryId][i],2,1,fHandle);
		}
	}

    ASSERT(sizeof(statusScreenAllowed) == 2);
    saveWrite(&statusScreenAllowed,2,1,fHandle);

    ASSERT(sizeof(FlagGameOver) == 2);
    saveWrite(&FlagGameOver,2,1,fHandle);

    ASSERT(sizeof(lightOff) == 2);
    saveWrite(&lightOff,2,1,fHandle);

    ASSERT(sizeof(shakingAmplitude) == 2);
    saveWrite(&shakingAmplitude,2,1,fHandle);

    ASSERT(sizeof(shakeVar1) == 2);
    saveWrite(&shakeVar1,2,1,fHandle);

    if (s_layout.aitd2)
        writeValue(0, fHandle);

    // The time the game stopped at, not the one running inside the menu
    const unsigned int savedClock = gameClockForSave();
    saveWrite(&savedClock,4,1,fHandle);

    ASSERT(sizeof(timerFreeze1) == 4);
    saveWrite(&timerFreeze1,4,1,fHandle);

    ASSERT(sizeof(currentMusic) == 2);
    saveWrite(&currentMusic,2,1,fHandle);

    //timerFreeze = 1;

    s_cipher = 0;
    var28 = ftell(fHandle);
    temp = var28;
    fseek(fHandle,12 + headerShift,SEEK_SET);
    var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);
    saveWrite(&var28,4,1,fHandle);
    fseek(fHandle,temp,SEEK_SET);

    saveWrite(&varSize,2,1,fHandle);
    s_cipher = s_layout.aitd2 ? kAITD2Cipher : 0;
    saveWrite(vars,varSize,1,fHandle);
    s_cipher = 0;

    // AITD2's original: the objects right after the variables, and where they
    // start in the first 4 bytes of the picture (GOG reads it from there)
    if (s_layout.aitd2)
    {
        const u32 actorsOffset = swapBytes((u32)ftell(fHandle));
        temp = ftell(fHandle);
        fseek(fHandle, 20, SEEK_SET);
        saveWrite(&actorsOffset, 4, 1, fHandle);
        fseek(fHandle, temp, SEEK_SET);
    }
    else
    {
        var28 = ftell(fHandle);
        temp = var28;
        fseek(fHandle,16,SEEK_SET);
        var28 = ((var28 & 0xFF) << 24) | ((var28 & 0xFF00) << 8) | (( var28 & 0xFF0000) >> 8) | ((var28 & 0xFF000000) >> 24);
        saveWrite(&var28,4,1,fHandle);
        fseek(fHandle,temp,SEEK_SET);
    }

    for(i=0;i<NUM_MAX_OBJECT;i++)
    {
        ASSERT(sizeof(ListObjets[i].indexInWorld) == 2);
        saveWrite(&ListObjets[i].indexInWorld,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].bodyNum) == 2);
        saveWrite(&ListObjets[i].bodyNum,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].objectType) == 2);
        saveWrite(&ListObjets[i].objectType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].dynFlags) == 2);
        saveWrite(&ListObjets[i].dynFlags,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX1) == 2);
        saveWrite(&ListObjets[i].zv.ZVX1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVX2) == 2);
        saveWrite(&ListObjets[i].zv.ZVX2,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVY1) == 2);
        saveWrite(&ListObjets[i].zv.ZVY1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVY2) == 2);
        saveWrite(&ListObjets[i].zv.ZVY2,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ1) == 2);
        saveWrite(&ListObjets[i].zv.ZVZ1,2,1,fHandle);

        //    ASSERT(sizeof(actorTable[i].zv.ZVZ2) == 2);
        saveWrite(&ListObjets[i].zv.ZVZ2,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMin) == 2);
        saveWrite(&ListObjets[i].screenXMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMin) == 2);
        saveWrite(&ListObjets[i].screenYMin,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenXMax) == 2);
        saveWrite(&ListObjets[i].screenXMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].screenYMax) == 2);
        saveWrite(&ListObjets[i].screenYMax,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomX) == 2);
        saveWrite(&ListObjets[i].roomX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomY) == 2);
        saveWrite(&ListObjets[i].roomY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].roomZ) == 2);
        saveWrite(&ListObjets[i].roomZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldX) == 2);
        saveWrite(&ListObjets[i].worldX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldY) == 2);
        saveWrite(&ListObjets[i].worldY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].worldZ) == 2);
        saveWrite(&ListObjets[i].worldZ,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].alpha) == 2);
        saveWrite(&ListObjets[i].alpha,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].beta) == 2);
        saveWrite(&ListObjets[i].beta,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].gamma) == 2);
        saveWrite(&ListObjets[i].gamma,2,1,fHandle);

        // The original saves the floor first (s_layout.stageFirst)
        if (s_layout.stageFirst)
        {
            saveWrite(&ListObjets[i].stage,2,1,fHandle);
            saveWrite(&ListObjets[i].room,2,1,fHandle);
        }
        else
        {
            saveWrite(&ListObjets[i].room,2,1,fHandle);
            saveWrite(&ListObjets[i].stage,2,1,fHandle);
        }

        ASSERT(sizeof(ListObjets[i].lifeMode) == 2);
        saveWrite(&ListObjets[i].lifeMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].life) == 2);
        saveWrite(&ListObjets[i].life,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].CHRONO) == 4);
        saveWrite(&ListObjets[i].CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ROOM_CHRONO) == 4);
        saveWrite(&ListObjets[i].ROOM_CHRONO,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].ANIM) == 2);
        saveWrite(&ListObjets[i].ANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animType) == 2);
        saveWrite(&ListObjets[i].animType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animInfo) == 2);
        saveWrite(&ListObjets[i].animInfo,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnim) == 2);
        saveWrite(&ListObjets[i].newAnim,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimType) == 2);
        saveWrite(&ListObjets[i].newAnimType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].newAnimInfo) == 2);
        saveWrite(&ListObjets[i].newAnimInfo,2,1,fHandle);

        if (s_layout.aitd2)
        {
            writeValue(0, fHandle);
            writeValue(-1, fHandle);
        }

        ASSERT(sizeof(ListObjets[i].frame) == 2);
        saveWrite(&ListObjets[i].frame,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].numOfFrames) == 2);
        saveWrite(&ListObjets[i].numOfFrames,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].END_FRAME) == 2);
        saveWrite(&ListObjets[i].END_FRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].flagEndAnim) == 2);
        saveWrite(&ListObjets[i].flagEndAnim,2,1,fHandle);

        if (s_layout.aitd2)
            saveWrite(&ListObjets[i].memoTicks,4,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackMode) == 2);
        saveWrite(&ListObjets[i].trackMode,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].trackNumber) == 2);
        saveWrite(&ListObjets[i].trackNumber,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].MARK) == 2);
        saveWrite(&ListObjets[i].MARK,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].positionInTrack) == 2);
        saveWrite(&ListObjets[i].positionInTrack,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepX) == 2);
        saveWrite(&ListObjets[i].stepX,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepY) == 2);
        saveWrite(&ListObjets[i].stepY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].stepZ) == 2); // 45
        saveWrite(&ListObjets[i].stepZ,2,1,fHandle);

        if (s_layout.aitd2)
        {
            saveWrite(&ListObjets[i].animNegX,2,1,fHandle);
            saveWrite(&ListObjets[i].animNegY,2,1,fHandle);
            saveWrite(&ListObjets[i].animNegZ,2,1,fHandle);
        }

        saveInterpolatedValue(&ListObjets[i].YHandler,fHandle);

        ASSERT(sizeof(ListObjets[i].falling) == 2);
        saveWrite(&ListObjets[i].falling,2,1,fHandle);

        saveInterpolatedValue(&ListObjets[i].rotate,fHandle);

        ASSERT(sizeof(ListObjets[i].direction) == 2); 
        saveWrite(&ListObjets[i].direction,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].speed) == 2);
        saveWrite(&ListObjets[i].speed,2,1,fHandle);

        saveInterpolatedValue(&ListObjets[i].speedChange,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[0]) == 2);
        saveWrite(&ListObjets[i].COL[0],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[1]) == 2);
        saveWrite(&ListObjets[i].COL[1],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL[2]) == 2);
        saveWrite(&ListObjets[i].COL[2],2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].COL_BY) == 2);
        saveWrite(&ListObjets[i].COL_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_DEC) == 2);
        saveWrite(&ListObjets[i].HARD_DEC,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HARD_COL) == 2);
        saveWrite(&ListObjets[i].HARD_COL,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT) == 2);
        saveWrite(&ListObjets[i].HIT,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].HIT_BY) == 2);
        saveWrite(&ListObjets[i].HIT_BY,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionType) == 2);
        saveWrite(&ListObjets[i].animActionType,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionANIM) == 2);
        saveWrite(&ListObjets[i].animActionANIM,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionFRAME) == 2);
        saveWrite(&ListObjets[i].animActionFRAME,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].animActionParam) == 2);
        saveWrite(&ListObjets[i].animActionParam,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hitForce) == 2);
        saveWrite(&ListObjets[i].hitForce,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPointID) == 2);
        saveWrite(&ListObjets[i].hotPointID,2,1,fHandle);

        if (s_layout.aitd2)
            writeValue(0, fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.x) == 2);
        saveWrite(&ListObjets[i].hotPoint.x,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.y) == 2);
        saveWrite(&ListObjets[i].hotPoint.y,2,1,fHandle);

        ASSERT(sizeof(ListObjets[i].hotPoint.z) == 2);
        saveWrite(&ListObjets[i].hotPoint.z,2,1,fHandle);

        if (s_layout.aitd2)
        {
            saveWrite(&ListObjets[i].hardMat,2,1,fHandle);
            writeValue(0, fHandle);
        }
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

