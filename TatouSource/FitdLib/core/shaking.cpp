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
static int s_shakeFramesRemaining = 0;
static int s_shakeMaxAmplitude = 0;

void updateShaking()
{
    // Only update shake if it was triggered by a life script (via setupShaking)
    if (s_shakeFramesRemaining > 0 && s_shakeMaxAmplitude > 0)
    {
        // Calculate current amplitude (fades out over time)
        float progress = (float)s_shakeFramesRemaining / 60.0f; // 60 ticks (1 second) max duration
        float currentAmplitude = (float)s_shakeMaxAmplitude * progress;

        // Generate random shake offsets - scale to visible pixel displacement
        // For amplitude 1, this gives ~5 pixel max displacement (visible shake)
        // For amplitude 10, this gives ~50 pixel displacement (dramatic shake)
        g_shakeOffsetX = ((float)(rand() % 200) - 100.0f) / 100.0f * currentAmplitude * 2.0f;
        g_shakeOffsetY = ((float)(rand() % 200) - 100.0f) / 100.0f * currentAmplitude * 2.0f;

        // Counted in ticks of the game clock, not frames (BUGS.md V19)
        s_shakeFramesRemaining -= ticksThisFrame;

        // Stop shaking when duration expires
        if (s_shakeFramesRemaining <= 0)
        {
            stopShaking();
        }
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
    shakeVar1 = 0;
    g_shakeOffsetX = 0.f;
    g_shakeOffsetY = 0.f;
    s_shakeFramesRemaining = 0;
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
        s_shakeFramesRemaining = 60; // Shake for ~1 second (60 frames at 60fps)
    }
}

void pauseShaking()
{
    g_shakeOffsetX = 0.f;
    g_shakeOffsetY = 0.f;
}
