///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Font rendering declarations
///////////////////////////////////////////////////////////////////////////////


extern int fontHeight;
// Chooses the font and the colour to write with
void SetFont(char* fontData, int color);
// Sets the space between words and between letters
void SetFontSpace(int interWordSpace, int interLetterSpace);
// Width in pixels of a text with the current font
int ExtGetSizeFont(u8* string);
// Writes a text at (x, y) on a 320x200 surface
void PrintFont(int x, int y, char* surface, u8* string);
// Writes text number `index` highlighted (selected menu entry)
void SelectedMessage(int x, int y, int index, int color1, int color2);
// Writes text number `index` at (x, y) in a colour
void SimpleMessage(int x, int y, int index, int color);
