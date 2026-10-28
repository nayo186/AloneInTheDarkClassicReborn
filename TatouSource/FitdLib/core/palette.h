///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Palette management declarations
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
typedef std::array<std::array<unsigned char, 3>, 256> palette_t;
// Fills a palette with one colour
void paletteFill(palette_t& palette, unsigned char r, unsigned char g, unsigned b);
// Sets a palette on screen
void setPalette(palette_t& palette);
// Copies a palette
void copyPalette(void* source, palette_t& dest);
// Copies a palette
void copyPalette(palette_t& source, palette_t& dest);
// Converts the palette of Jack and later games from 6-bit to 8-bit colours
void convertPaletteIfRequired(palette_t& lpalette);
// Makes the palette `coef`/256 of the way to black (fades)
void SetLevelDestPal(palette_t& inPalette, palette_t& outPalette, int coef);

extern palette_t currentGamePalette;
extern float g_fadeLevel; // 0.0 = black, 1.0 = full brightness (used for HD backgrounds)
