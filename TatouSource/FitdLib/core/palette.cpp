///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Color palette management, fading, and conversion
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "palette.h"

float g_fadeLevel = 1.0f; // 0.0 = black, 1.0 = full brightness

void paletteFill(palette_t& palette, unsigned char r, unsigned char g, unsigned b)
{
    r <<= 1;
    g <<= 1;
    b <<= 1;

    for (int i = 0; i < 256; i++)
    {
        palette[i][0] = r;
        palette[i][1] = g;
        palette[i][2] = b;
    }
}

void setPalette(palette_t& sourcePal)
{
    osystem_setPalette(&sourcePal);
}

void copyPalette(palette_t& source, palette_t& dest)
{
    for (int i = 0; i < 256; i++)
    {
        dest[i] = source[i];
    }
}

void copyPalette(void* source, palette_t& dest)
{
    // palette_t is std::array<std::array<unsigned char, 3>, 256> -> 768 contiguous bytes.
    memcpy(dest.data(), source, 256 * 3);
}

// AITD2's palettes leave colours 1-15 for the texts and menus, all magenta
// (63, 5, 42); the game fills them with AITD1's (ITD_RESS 3): its menu text
// is AITD1's pale green 4 and its highlight white 15 (BUGS.md A3)
static const unsigned char kTextColoursAITD2[15][3] =
{
    { 255, 255, 255 }, { 51, 51, 59 }, { 195, 191, 255 }, { 143, 179, 143 }, { 171, 119, 171 },
    { 171, 135, 99 }, { 255, 23, 171 }, { 75, 83, 99 }, { 199, 87, 95 }, { 87, 151, 87 },
    { 87, 191, 255 }, { 255, 139, 87 }, { 175, 87, 255 }, { 255, 255, 135 }, { 255, 255, 255 },
};

void convertPaletteIfRequired(palette_t& lpalette)
{
    const bool textColoursFree = g_gameId == AITD2 && lpalette[1][0] == 63 && lpalette[1][1] == 5 && lpalette[1][2] == 42;

    if (g_gameId >= JACK && g_gameId <= AITD3)
    {
        for (int i = 0; i < 256; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                lpalette[i][j] = (((unsigned int)lpalette[i][j] * 255) / 63) & 0xFF;
            }
        }
    }

    if (textColoursFree)
    {
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 3; j++)
                lpalette[i + 1][j] = kTextColoursAITD2[i][j];
    }
}

void SetLevelDestPal(palette_t& inPalette, palette_t& outPalette, int coef)
{
    // Update global fade level for HD backgrounds (coef 0-256 -> 0.0-1.0)
    g_fadeLevel = coef / 256.0f;

    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 3; j++) {
            outPalette[i][j] = (inPalette[i][j] * coef) >> 8;
        }
    }
}