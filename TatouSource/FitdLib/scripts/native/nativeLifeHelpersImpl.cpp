///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Implementations of non-inline helpers declared in nativeLifeHelpers.h
///////////////////////////////////////////////////////////////////////////////

#include "nativeLifeHelpers.h"
#include "gameTime.h"
#include "pak.h"
#include "coreHooks.h"


void life_Picture(int pictureIndex, int delay, int sampleId)
{
    SaveTimerAnim();

    // A full-screen picture's sample replaced any loop on the original's
    // single sample channel (BUGS.md C21)
    osystem_stopLoopingSamples();
    coreScreenPictureShown("ITD_RESS", pictureIndex);

    LoadPak("ITD_RESS", pictureIndex, aux);
    FastCopyScreen(aux, frontBuffer);
    osystem_CopyBlockPhys((unsigned char*)frontBuffer, 0, 0, 320, 200);
    osystem_drawBackground();

    unsigned int chrono;
    startChrono(&chrono);
    playSound(sampleId);

    do
    {
        process_events();
        osystem_startFrame();
        osystem_drawBackground();
        osystem_stopFrame();
        osystem_flip(NULL);

        if (evalChrono(&chrono) > (unsigned int)delay)
            break;
    } while (!key && !Click);

    RestoreTimerAnim();

    FlagInitView = 1;
}

void life_WaitGameOver()
{
    while (key || JoyD || Click)
        process_events();
    while (!key && !JoyD && !Click)
        process_events();
    FlagGameOver = 1;
}
