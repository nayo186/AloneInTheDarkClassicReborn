///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Game start, save/load screen and game detection (split from main.cpp)
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
#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>
#include <fstream>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image.h"

static std::string g_versionString;

static void loadVersionString()
{
    std::filesystem::path versionPath = std::filesystem::path(homePath) / "version.txt";
    std::ifstream file(versionPath);
    if (file.is_open())
    {
        std::getline(file, g_versionString);
        // trim trailing whitespace
        while (!g_versionString.empty() && (g_versionString.back() == '\r' || g_versionString.back() == '\n' || g_versionString.back() == ' '))
            g_versionString.pop_back();
    }
}


void startGame(int startupFloor, int startupRoom, int allowSystemMenu)
{
    LoadWorld();
    initVars();

    LoadEtage(startupFloor);

    NumCamera = -1;

    ChangeSalle(startupRoom);

    NewNumCamera = 0;
    FlagInitView = 2;

    InitView();

    PlayWorld(allowSystemMenu, 1);

    /*freeScene();

    fadeOut(8,0);*/
}

// Scales an RGBA source image to fill exactly the pixel region (x1,y1)-(x2,y2) inclusive
// in logicalScreen, using bilinear interpolation + nearest-palette matching.
// Used exclusively by the save/load slot selection screen.
static void fillSavePreviewToFrame(int x1, int y1, int x2, int y2,
    unsigned char* sourceBuffer, int srcWidth, int srcHeight)
{
    int destW = x2 - x1 + 1;
    int destH = y2 - y1 + 1;
    if (destW <= 0 || destH <= 0 || !sourceBuffer) return;

    float scaleX = (destW > 1) ? (float)(srcWidth - 1) / (float)(destW - 1) : 0.0f;
    float scaleY = (destH > 1) ? (float)(srcHeight - 1) / (float)(destH - 1) : 0.0f;

    for (int y = 0; y < destH; y++)
    {
        for (int x = 0; x < destW; x++)
        {
            float srcXf = x * scaleX;
            float srcYf = y * scaleY;

            int srcX0 = (int)srcXf;
            int srcY0 = (int)srcYf;
            int srcX1c = (srcX0 + 1 < srcWidth) ? srcX0 + 1 : srcX0;
            int srcY1c = (srcY0 + 1 < srcHeight) ? srcY0 + 1 : srcY0;

            float fx = srcXf - srcX0;
            float fy = srcYf - srcY0;
            float fx1 = 1.0f - fx;
            float fy1 = 1.0f - fy;

            int idx00 = (srcY0 * srcWidth + srcX0) * 4;
            int idx10 = (srcY0 * srcWidth + srcX1c) * 4;
            int idx01 = (srcY1c * srcWidth + srcX0) * 4;
            int idx11 = (srcY1c * srcWidth + srcX1c) * 4;

            float r = (sourceBuffer[idx00 + 0] * fx1 + sourceBuffer[idx10 + 0] * fx) * fy1
                + (sourceBuffer[idx01 + 0] * fx1 + sourceBuffer[idx11 + 0] * fx) * fy;
            float g = (sourceBuffer[idx00 + 1] * fx1 + sourceBuffer[idx10 + 1] * fx) * fy1
                + (sourceBuffer[idx01 + 1] * fx1 + sourceBuffer[idx11 + 1] * fx) * fy;
            float b = (sourceBuffer[idx00 + 2] * fx1 + sourceBuffer[idx10 + 2] * fx) * fy1
                + (sourceBuffer[idx01 + 2] * fx1 + sourceBuffer[idx11 + 2] * fx) * fy;

            int ri = (int)(r + 0.5f);
            int gi = (int)(g + 0.5f);
            int bi = (int)(b + 0.5f);

            int bestIdx = 0, bestDist = 999999;
            for (int i = 16; i < 256; i++)
            {
                int pr = currentGamePalette[i][0];
                int pg = currentGamePalette[i][1];
                int pb = currentGamePalette[i][2];
                int dist = (ri - pr) * (ri - pr) + (gi - pg) * (gi - pg) + (bi - pb) * (bi - pb);
                if (dist < bestDist) { bestDist = dist; bestIdx = i; }
            }

            int dstX = x1 + x;
            int dstY = y1 + y;
            if (dstX >= 0 && dstX < 320 && dstY >= 0 && dstY < 200)
                logicalScreen[dstY * 320 + dstX] = (char)bestIdx;
        }
    }
}

// Tiny 3x5 characters for the date and time of each save: the game's font is
// too big to fit them next to "Slot N" (digits, '/', ':' and space)
static void drawTinyText(int x, int y, const char* text, char color)
{
    static const unsigned char kDigits[10][5] = {
        { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
        { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 2, 2, 2 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 } };
    static const unsigned char kSlash[5] = { 1, 1, 2, 4, 4 };
    static const unsigned char kColon[5] = { 0, 2, 0, 2, 0 };
    for (; *text; text++, x += 4)
    {
        const unsigned char* glyph = nullptr;
        if (*text >= '0' && *text <= '9') glyph = kDigits[*text - '0'];
        else if (*text == '/') glyph = kSlash;
        else if (*text == ':') glyph = kColon;
        if (!glyph)
            continue;
        for (int row = 0; row < 5; row++)
            for (int column = 0; column < 3; column++)
                if ((glyph[row] >> (2 - column)) & 1)
                {
                    const int px = x + column, py = y + row;
                    if (px >= 0 && px < 320 && py >= 0 && py < 200)
                        logicalScreen[py * 320 + px] = color;
                }
    }
}

// When a save was written ("dd/mm/yy hh:mm", local time), from its file
static std::string saveDate(const char* fileName)
{
    std::error_code error;
    const auto written = std::filesystem::last_write_time(fileName, error);
    if (error)
        return std::string();
    // From the files' clock to the system's, by the gap between both now
    // (clock_cast does it exactly but is missing before GCC 13)
    const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        written - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t seconds = std::chrono::system_clock::to_time_t(systemTime);
    std::tm local = {};
    localtime_s(&local, &seconds);
    char text[32];
    strftime(text, sizeof(text), "%d/%m/%y %H:%M", &local);
    return text;
}

int parseAllSaves(int arg)
{
    const int NUM_SAVE_SLOTS = 9; // Slots 0-8 (slot 9 would overflow menu)
    bool saveExists[NUM_SAVE_SLOTS];
    std::string saveDates[NUM_SAVE_SLOTS];   // when each one was written
    int currentSelectedSlot = 0;
    int selectedSlot = -1;
    char buffer[512];
    int initialDelay = 15; // Frames to wait before accepting Enter/click to prevent accidental selection
    static Uint64 s_saveMenuSelTime = 0; // For pulsing highlight effect

    // Suppress lantern glow when opening save/restore screen

    // Preview image state
    unsigned char* previewImageData = NULL;
    int previewImageW = 0;
    int previewImageH = 0;
    int lastPreviewSlot = -1; // Track which slot's preview is loaded



    // Scan for existing save files
    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        strcpy_s(buffer, sizeof(buffer), saveFilePath(i, ".ITD").c_str());
        saveExists[i] = fileExists(buffer);
        if (saveExists[i])
            saveDates[i] = saveDate(buffer);
    }

    // Wait for initial key release to prevent instant selection
    while (key || JoyD || Click)
    {
        process_events();
    }

    // Draw save/load menu
    while (selectedSlot == -1)
    {
        // Load preview PNG when slot selection changes
        if (lastPreviewSlot != currentSelectedSlot)
        {
            if (previewImageData)
            {
                stbi_image_free(previewImageData);
                previewImageData = NULL;
            }
            previewImageW = 0;
            previewImageH = 0;

            if (saveExists[currentSelectedSlot])
            {
                const std::string pngPath = saveFilePath(currentSelectedSlot, ".png");
                int channels = 0;
                previewImageData = stbi_load(pngPath.c_str(), &previewImageW, &previewImageH, &channels, 4);
            }
            lastPreviewSlot = currentSelectedSlot;

        }

        AffBigCadre(160, 100, 280, 180);


        // Draw title (use existing text messages if available, otherwise draw slots)
        int startY = 30;
        int lineHeight = 16;

        // Display save slots (narrowed to left side to make room for preview)
        for (int i = 0; i < NUM_SAVE_SLOTS; i++)
        {
            int yPos = startY + (i * lineHeight);

            // Format slot text
            if (saveExists[i])
            {
                sprintf_s(buffer, sizeof(buffer), "Slot %d", i);
            }
            else
            {
                sprintf_s(buffer, sizeof(buffer), "Slot %d - Empty", i);
            }

            // Highlight selected slot with pulsing effect
            if (i == currentSelectedSlot)
            {
                Uint64 now = SDL_GetTicks();
                // Calculate selection pop effect
                int pop = 0;
                if (now - s_saveMenuSelTime < 250)
                {
                    float t = (float)(now - s_saveMenuSelTime) / 250.0f;
                    pop = (int)(sinf(t * 3.14159f) * 6.0f);
                }
                // Pulsing highlight color
                char color = (char)(100 + (int)(fabsf(sinf((float)now * 0.004f)) * 8.0f));
                AffRect(28, yPos - 1 - pop / 2, 160 + pop * 2, yPos + 14 + pop, color);

                // Draw text with highlight
                SetFont(PtrFont, 15);
                PrintFont(30, yPos, logicalScreen, (u8*)buffer);
            }
            else
            {
                // Draw normal text
                SetFont(PtrFont, 4);
                PrintFont(30, yPos, logicalScreen, (u8*)buffer);
            }

            // Date and time of the save, small, right after "Slot N": at the end
            // of the line the preview's frame (from x=175) covered it
            if (saveExists[i] && !saveDates[i].empty())
                drawTinyText(30 + ExtGetSizeFont((u8*)buffer) + 8, yPos + 5, saveDates[i].c_str(),
                    (i == currentSelectedSlot) ? 15 : 4);
        }

        // Draw preview box on the right side
        AffBigCadre(235, 80, 120, 90);

        if (previewImageData && previewImageW > 0 && previewImageH > 0)
        {
            // Fill from the content area top (WindowY1=43) all the way down to y=126,
            // just before the bottom bar sprite at y=127. This covers the 10-pixel black
            // gap that exists between WindowY2=116 and the visible bottom golden bar.
            // Corner sprites live at x=175-182 / x=287+ so x=183-286 is clear to overwrite.
            fillSavePreviewToFrame(183, 43, 286, 126, previewImageData, previewImageW, previewImageH);
        }
        else
        {
            // Fill preview area with black to prevent stale framebuffer content
            for (int py = 43; py <= 126; py++)
                for (int px = 183; px <= 286; px++)
                    logicalScreen[py * 320 + px] = 0;
            SetFont(PtrFont, 4);
            PrintFont(185, 70, logicalScreen, (u8*)"No Preview");
        }

        // Refresh display
        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        osystem_startFrame();


        osystem_drawUILayer();
        process_events();
        flushScreen();

        // Decrement initial delay
        if (initialDelay > 0)
            initialDelay--;

        // Handle input
        int localKey = key;
        int localClick = Click;
        int localJoyD = JoyD;

        // Mouse: hover selects slot, click confirms (keyboard/gamepad take priority)
        if (initialDelay == 0)
        {
            static ImVec2 s_saveMouse = { -1.0f, -1.0f };
            ImVec2 gm = menuGetGameMouse();
            if (menuMouseMoved(s_saveMouse, localKey || localJoyD))
            {
                // Slots are in the left column: X 28-160, startY=30, lineHeight=16
                int hit = menuMouseHitList(gm.x, gm.y, 28, 160, 30, 16, NUM_SAVE_SLOTS);
                if (hit >= 0 && hit != currentSelectedSlot)
                {
                    currentSelectedSlot = hit;
                    s_saveMenuSelTime = SDL_GetTicks();
                }
            }
            if (menuMouseClicked())
            {
                int hit = menuMouseHitList(gm.x, gm.y, 28, 160, 30, 16, NUM_SAVE_SLOTS);
                if (hit >= 0)
                    localClick = 1; // treat as confirm on the hovered slot
            }
        }

        if (!AntiRebond)
        {
            // Enter key or mouse click - select current slot (only after initial delay)
            if ((localKey == 0x1C || localClick) && initialDelay == 0)
            {
                // For load mode (arg == 0), only allow selection if save exists
                if (arg == 0 && !saveExists[currentSelectedSlot])
                {
                    // Can't load from empty slot, do nothing
                }
                else
                {
                    selectedSlot = currentSelectedSlot;
                }
                AntiRebond = 1;
            }
            // Escape key - cancel
            else if (localKey == 0x1B)
            {
                selectedSlot = -1;
                break;
            }
            // Up arrow
            else if (localJoyD == 1)
            {
                currentSelectedSlot--;
                if (currentSelectedSlot < 0)
                    currentSelectedSlot = NUM_SAVE_SLOTS - 1;
                s_saveMenuSelTime = SDL_GetTicks();
                AntiRebond = 1;
            }
            // Down arrow
            else if (localJoyD == 2)
            {
                currentSelectedSlot++;
                if (currentSelectedSlot >= NUM_SAVE_SLOTS)
                    currentSelectedSlot = 0;
                s_saveMenuSelTime = SDL_GetTicks();
                AntiRebond = 1;
            }
        }
        else
        {
            // Reset anti-rebond when keys are released
            if (!localKey && !localJoyD && !localClick)
            {
                AntiRebond = 0;
            }
        }

        osystem_flip(NULL);
    }

    // Free preview image
    if (previewImageData)
    {
        stbi_image_free(previewImageData);
        previewImageData = NULL;
    }

    // Wait for key release
    while (key || JoyD || Click)
    {
        process_events();
    }


    // Re-enable lantern glow when exiting save/restore screen

    return selectedSlot;
}

void detectGame(void)
{
    const char* gameName = NULL;

    // Jack in the Dark: its folder has PERE.PAK (AITD1's has not)
    if (dataFileExists("PERE.PAK"))
    {
        g_gameId = JACK;
        CVars.resize(70);
        currentCVarTable = AITD2KnownCVars;
        gameName = "Jack in the Dark";
    }
    else if (dataFileExists("AITD2.flag"))
    {
        g_gameId = AITD2;
        CVars.resize(70);
        currentCVarTable = AITD2KnownCVars;
        gameName = "Alone in the Dark 2";
    }
    else if (dataFileExists("LISTBOD2.PAK"))
    {
        g_gameId = AITD1;
        CVars.resize(45);
        currentCVarTable = AITD1KnownCVars;
        gameName = "Alone in the Dark";
    }
    else if (dataFileExists("PERE.PAK"))
    {
        g_gameId = JACK;
        CVars.resize(70);
        currentCVarTable = AITD2KnownCVars;
        gameName = "Jack in the Dark";
    }
    else if (dataFileExists("MER.PAK"))
    {
        g_gameId = AITD2;
        CVars.resize(70);
        currentCVarTable = AITD2KnownCVars;
        gameName = "Alone in the Dark 2";
    }
    else if (dataFileExists("AN1.PAK"))
    {
        g_gameId = AITD3;
        CVars.resize(70);
        currentCVarTable = AITD2KnownCVars;
        gameName = "Alone in the Dark 3";
    }
    else if (dataFileExists("PURSUIT.PAK"))
    {
        g_gameId = TIMEGATE;
        CVars.resize(100); // TODO: figure this
        currentCVarTable = AITD2KnownCVars; // TODO: figure this
        gameName = "Time Gate";
    }
    else
    {
        printf(MAIN_ERR "FATAL: Game detection failed..." CON_RESET "\n");
        assert(0);
        return;
    }

    loadVersionString();

    std::string titleStr = gameName;
    if (!g_versionString.empty())
    {
        titleStr += " - " + g_versionString;
    }

    printf(MAIN_TAG "%s\n", titleStr.c_str());
#ifndef AITD_UE4
    SDL_SetWindowTitle(gWindowBGFX, titleStr.c_str());
#endif
}
