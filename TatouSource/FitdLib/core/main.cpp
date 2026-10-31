///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// FitdMain: the engine start (settings, launch options, graphics, game
// start) and a few helpers of the original engine (messages, hits, clip,
// exit); the rest of the old main.cpp is split by subject in core/
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "fileNameCase.h"

// Menu sound helper (from systemMenu.cpp)
extern void playMenuSound(const char* soundName);

#include "anim2d.h"
#include "coreHooks.h"
#include "launchOptions.h"
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
#include <ctime>

// stb_image for loading save slot preview PNGs (static, one copy per file that needs it)

extern "C" {
    extern char homePath[512];
}

FILE* Open(const char* filename, const char* mode) {
    std::filesystem::path path = std::filesystem::path(homePath) / filename;
    return fopen(mode[0] == 'r' ? matchFileCase(path.string()).c_str() : path.string().c_str(), mode);
}


int AntiRebond;

extern "C" {
    int FitdMain(int argc, char* argv[]);
    void setCurrentContext(void);
}

int FitdMain(int argc, char* argv[])
{
    // A different chance in every session: the scripts' RANDOM uses rand(),
    // which without a seed repeats the same numbers each time (BUGS.md C34)
    srand((unsigned int)time(nullptr));

    // Load the settings early, before any systems that depend on them
    loadSettings();

    // Parameters from the launcher (game, language, mode) override the saved ones
    parseLaunchOptions(argc, argv);
    applyLaunchOptions();

#ifndef AITD_UE4
    initBgfxGlue(argc, argv);
#endif


    osystem_startOfFrame();

    //  int protectionToBeDone = 1;

    OpenProgram();

    paletteFill(currentGamePalette, 0, 0, 0);

    loadPalette();

    // Apply the loaded palette to the renderer
    osystem_setPalette(&currentGamePalette);

    switch (g_gameId)
    {
    case AITD1:
        startAITD1();
        break;
    case JACK:
        startJACK();
        break;
    case AITD2:
        startAITD2();
        break;
    case AITD3:
        startAITD3();
        break;
    case TIMEGATE:
        startGame(0, 5, 1);
        break;
    default:
        assert(0);
        break;
    }

    return(0);
}

int drawTextOverlay(void)
{
    int var_14 = 0;
    int var_10 = 183;
    messageStruct* currentMessage;

    BBox3D4 = 199;
    BBox3D1 = 319;
    BBox3D3 = 0;

    currentMessage = messageTable;

    // Shown in the dark too: the dark room's own message is one (BUGS.md V23)
    {
        int i;

        for (i = 0; i < 5; i++)
        {
            if (currentMessage->string)
            {
                int width = currentMessage->string->width;
                int X = 160 - width / 2;
                int Y = X + width;

                if (X < BBox3D1)
                {
                    BBox3D1 = X;
                }

                if (Y > BBox3D3)
                {
                    BBox3D3 = Y;
                }

                // Counted in ticks of the game clock: the original counted
                // drawn frames, so a message lasted less the higher the FPS
                // (under a second at 60). It now stays one second longer
                // before fading out (BUGS.md C49)
                const int kMessageFullTicks = 26 + 60;
                const int kMessageFadeTicks = 30;

                currentMessage->time += ticksThisFrame;

                if (currentMessage->time > kMessageFullTicks + kMessageFadeTicks)
                {
                    currentMessage->string = NULL;
                }
                else
                {
                    if (currentMessage->time < kMessageFullTicks)
                    {
                        SetFont(PtrFont, 16);
                    }
                    else
                    {
                        SetFont(PtrFont, 16 + (currentMessage->time - kMessageFullTicks) / 2);
                    }

                    PrintFont(X, var_10 + 1, logicalScreen, currentMessage->string->textPtr);
                }

                var_10 -= 16;
                var_14 = 1;

            }

            currentMessage++;
        }
    }


    BBox3D2 = var_10;
    return(var_14);
}

void makeMessage(int messageIdx)
{
    textEntryStruct* messagePtr;

    messagePtr = getTextFromIdx(messageIdx);

    if (messagePtr)
    {
        int i;

        for (i = 0; i < 5; i++)
        {
            if (messageTable[i].string == messagePtr)
            {
                messageTable[i].time = 0;
                return;
            }
        }

        for (i = 0; i < 5; i++)
        {
            if (messageTable[i].string == NULL)
            {
                messageTable[i].string = messagePtr;
                messageTable[i].time = 0;
                return;
            }
        }
    }
}

void hit(int animNumber, int arg_2, int arg_4, int arg_6, int hitForce, int arg_A)
{
    if (InitAnim(animNumber, 0, arg_A))
    {
        currentProcessedActorPtr->animActionANIM = animNumber;
        currentProcessedActorPtr->animActionFRAME = arg_2;
        currentProcessedActorPtr->animActionType = 1;
        currentProcessedActorPtr->animActionParam = arg_6;
        currentProcessedActorPtr->hotPointID = arg_4;
        currentProcessedActorPtr->hitForce = hitForce;
    }
}

void SetClip(int left, int top, int right, int bottom)
{
    clipLeft = left;
    clipTop = top;
    clipRight = right;
    clipBottom = bottom;
}

extern "C" {
    void Sound_Quit(void);
}

void cleanupAndExit(void)
{
    shutdownController();

    // Stop the music, voice and CD streams while the sound engine still
    // exists: they keep a pointer to it, and stopping them after Sound_Quit
    // deleted it crashed on exit with music playing (BUGS.md C14)
    extern void closeAudioArchive();
    closeAudioArchive();

    Sound_Quit();

    // The resource lists are not freed: exit(0) follows and the system frees
    // the memory. Closing the window calls this from the main thread, while
    // the game thread may still use them, and on the start screen some of
    // them do not exist yet (HQR_Reset on a null pointer crashed, BUGS.md C9).
    /* HQR_Free(listMus);
    HQR_Free(listSamp);
    HQR_Free(HQ_Memory);
    HQR_Free(listLife);
    HQR_Free(listTrack);
    HQR_Free(HQ_Bodys);
    HQR_Free(HQ_Anims); */

    /* free(tabTextes);
    free(aux);
    free(aux2);
    free(bufferAnim);

    free(screen); */

    destroyMusicDriver();

    // Shutdown job system and clear VOC cache
    osystem_clearVocCache();
    shutdownJobSystem();

    exit(0);
}

