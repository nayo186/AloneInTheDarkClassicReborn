///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Screen shaking effect (split from main.cpp)
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


// Shake state - only triggers via life script LM_SHAKING opcode
static int s_shakeMaxAmplitude = 0;

// SHAKING(n) shakes the screen until SHAKING(0), as in the original: the
// remaster stopped it after one second, so the shaking of the ending (one
// SHAKING(1) when the tree burns, SHAKING(0) once escaped) ended at once
// (BUGS.md C50). Also stopped on a floor change and on leaving the game
void updateShaking()
{
    if (s_shakeMaxAmplitude > 0)
    {
        // Random offsets: amplitude 1 moves the screen up to ~2 pixels
        const float amplitude = (float)s_shakeMaxAmplitude;
        g_shakeOffsetX = ((float)(rand() % 200) - 100.0f) / 100.0f * amplitude * 2.0f;
        g_shakeOffsetY = ((float)(rand() % 200) - 100.0f) / 100.0f * amplitude * 2.0f;
    }
    else
    {
        // No shake active - ensure offsets are zero
        g_shakeOffsetX = 0.f;
        g_shakeOffsetY = 0.f;
    }
}

void stopShaking()
{
    shakingAmplitude = 0;
    g_shakeOffsetX = 0.f;
    g_shakeOffsetY = 0.f;
    s_shakeMaxAmplitude = 0;
}

void setupShaking(int amplitude)
{
    // Called from LM_SHAKING life script opcode
    // amplitude = 0 means stop shaking (handled by stopShaking() in life.cpp)
    // amplitude > 0 means start shaking with given intensity
    if (amplitude > 0)
    {
        shakingAmplitude = amplitude;
        s_shakeMaxAmplitude = amplitude;
    }
}

void pauseShaking()
{
    g_shakeOffsetX = 0.f;
    g_shakeOffsetY = 0.f;
}
