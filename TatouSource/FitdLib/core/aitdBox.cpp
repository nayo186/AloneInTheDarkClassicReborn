///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// UI dialog box and frame rendering
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

void AffSpf(int left, int top, int index, char* gfxData)
{
    char* outPtr;
    char* inPtr;

    int width;
    int height;

    int offset;

    int i;
    int j;

    if(g_gameId >= AITD3)
        return;

    inPtr = gfxData + READ_LE_U16(index * 2 + gfxData);

    inPtr +=4;

    width = READ_LE_U16(inPtr);
    inPtr+=2;
    height = READ_LE_U16(inPtr);
    inPtr+=2;

    // Bounds check: ensure sprite doesn't draw beyond screen boundaries
    if (left < 0 || top < 0 || left >= 320 || top >= 200)
        return;

    // Store original width for input stride
    int originalWidth = width;

    // Clamp width/height to stay within screen bounds
    if (left + width > 320)
        width = 320 - left;
    if (top + height > 200)
        height = 200 - top;

    if (width <= 0 || height <= 0)
        return;

    outPtr = logicalScreen + top*320 + left;

    // Per-row memcpy is significantly faster than the original byte-at-a-time loop.
    // The source rows still advance by the unclamped width.
    for (i = 0; i < height; i++)
    {
        memcpy(outPtr, inPtr, width);
        inPtr += originalWidth;
        outPtr += 320;
    }
}

void SizeSpf(int index, char* gfxData, int* width, int* height)
{
    char* inPtr = gfxData + READ_LE_U16(index * 2 + gfxData) + 4;
    *width = READ_LE_U16(inPtr);
    *height = READ_LE_U16(inPtr + 2);
}

void AffSpfI(int left, int top, int index, char* gfxData)
{
	char* outPtr;
	char* inPtr;

	int width;
	int height;

	int offset;

	int i;
	int j;

	// AITD3's sprite sets are shorter than AITD2's (its CADRE has only the
	// menu's arrows): an index past the table is not drawn (BUGS.md T1)
	if(g_gameId > AITD3 || (g_gameId == AITD3 && index * 2 >= READ_LE_U16(gfxData)))
		return;

	outPtr = logicalScreen + top*320 + left;
	inPtr = gfxData + READ_LE_U16(index * 2 + gfxData); // alignement unsafe

	inPtr +=4;

	width = READ_LE_U16(inPtr); // alignement unsafe
	inPtr+=2;
	height = READ_LE_U16(inPtr); // alignement unsafe
	inPtr+=2;

	// Bounds check: ensure sprite doesn't draw beyond screen boundaries
	if (left < 0 || top < 0 || left >= 320 || top >= 200)
		return;
	
	// Clamp width/height to stay within screen bounds
	int originalWidth = width;
	if (left + width > 320)
		width = 320 - left;
	if (top + height > 200)
		height = 200 - top;

	if (width <= 0 || height <= 0)
		return;

	offset = 320 - width;

	for (i = 0; i < height; i++)
	{
		// Per-row scan, but only branch once per pixel and avoid double-increment.
		char* dst = outPtr;
		const char* src = inPtr;
		for (j = 0; j < width; j++)
		{
			char color = src[j];
			if (color)
				dst[j] = color;
		}
		inPtr += originalWidth;
		outPtr += 320;
	}
}

// One piece of AITD2's frame: colour 0 is see-through, and so is colour 11,
// a stray column at the edge of its left side (and all of piece 8): drawn,
// it made a cyan line inside every frame (BUGS.md A1). Pixels from x
// `clipRight` and y `clipBottom` on are left out
static void affCadrePiece(int left, int top, int index, int clipRight = 320, int clipBottom = 200)
{
    int width, height;
    SizeSpf(index, PtrCadre, &width, &height);
    const unsigned char* pixels = (const unsigned char*)PtrCadre + READ_LE_U16(index * 2 + PtrCadre) + 8;
    if (clipRight > 320) clipRight = 320;
    if (clipBottom > 200) clipBottom = 200;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const unsigned char colour = pixels[y * width + x];
            if (colour != 0 && colour != 11 && left + x >= 0 && left + x < clipRight && top + y >= 0 && top + y < clipBottom)
                logicalScreen[(top + y) * 320 + left + x] = colour;
        }
    }
}

// AITD2's frame: its corners are taller than its sides, so each piece is
// placed from its own edge; drawn as in AITD1 the right side ended above
// the bottom corner, with a gap (BUGS.md A1)
static void affCadreAITD2(int left, int top, int right, int bottom)
{
    int cornerWidth[4], cornerHeight[4];
    for (int i = 0; i < 4; i++)
        SizeSpf(i, PtrCadre, &cornerWidth[i], &cornerHeight[i]);
    int barWidth, topHeight, bottomHeight, leftWidth, sideHeight, rightWidth, unused;
    SizeSpf(4, PtrCadre, &barWidth, &topHeight);
    SizeSpf(5, PtrCadre, &unused, &bottomHeight);
    SizeSpf(6, PtrCadre, &leftWidth, &sideHeight);
    SizeSpf(7, PtrCadre, &rightWidth, &unused);

    WindowX1 = left + leftWidth;
    WindowY1 = top + topHeight;
    WindowX2 = right - rightWidth - 1;
    WindowY2 = bottom - bottomHeight - 1;
    AffRect(WindowX1, WindowY1, WindowX2, WindowY2, 0);  // the corners reach inside

    const int barsEnd = right - cornerWidth[1];
    for (int x = left + cornerWidth[0]; x < barsEnd; x += barWidth)
    {
        affCadrePiece(x, top, 4, barsEnd);
        affCadrePiece(x, bottom - bottomHeight, 5, barsEnd);
    }
    const int sidesEnd = bottom - cornerHeight[2];
    for (int y = top + cornerHeight[0]; y < sidesEnd; y += sideHeight)
    {
        affCadrePiece(left, y, 6, 320, sidesEnd);
        affCadrePiece(right - rightWidth, y, 7, 320, sidesEnd);
    }

    affCadrePiece(left, top, 0);
    affCadrePiece(right - cornerWidth[1], top, 1);
    affCadrePiece(left, bottom - cornerHeight[2], 2);
    affCadrePiece(right - cornerWidth[3], bottom - cornerHeight[3], 3);

    SetClip(WindowX1, WindowY1, WindowX2, WindowY2);
}

// AITD3's frame: its CADRE has no frame pieces, only the menu's arrows, and
// drawn with AITD2's pieces it read past them and closed the game (BUGS.md
// T1). Two lines, slate and brown (colours 8 and 4, the same in every
// camera), around a black inside
static void affCadreAITD3(int left, int top, int right, int bottom)
{
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > 319) right = 319;
    if (bottom > 199) bottom = 199;

    AffRect(left, top, right, bottom, 8);
    AffRect(left + 1, top + 1, right - 1, bottom - 1, 4);
    AffRect(left + 2, top + 2, right - 2, bottom - 2, 0);

    WindowX1 = left + 3;
    WindowY1 = top + 3;
    WindowX2 = right - 3;
    WindowY2 = bottom - 3;
    SetClip(WindowX1, WindowY1, WindowX2, WindowY2);
}

// The frame of menus and messages, made of the CADRE pieces: corners 0-3,
// top and bottom bars 4-5, sides 6-7 and "in the dark" 8 (AITD1 and Jack:
// 20 pixel corners, 8 pixel bars)
void AffBigCadre(int x, int y, int width, int height)
{
    int top;
    int right;
    int left;
    int bottom;

    int currentLeftPosition;
    int currentTopPosition;

    int halfWidth;
    int halfHeight;

    SetClip(0,0,319,199);

    halfWidth = width/2;
    currentLeftPosition = left = x - halfWidth;

    halfHeight = height/2;
    currentTopPosition = top = y - halfHeight;

    right = x + halfWidth;
    bottom = y + halfHeight;

    // The frame covers the text drawn under it, as on the original's one
    // screen: the letters live in their own layer (uiLayer), and the
    // inventory's showed through the frame of the "You find" window (BUGS.md A46)
    for (int ty = (top < 0 ? 0 : top); ty <= (bottom > 199 ? 199 : bottom); ty++)
        for (int tx = (left < 0 ? 0 : left); tx <= (right > 319 ? 319 : right); tx++)
            uiLayer[ty * 320 + tx] = 0;

    if (g_gameId == AITD3)
    {
        affCadreAITD3(left, top, right, bottom);
        return;
    }
    if (g_gameId >= AITD2)
    {
        affCadreAITD2(left, top, right, bottom);
        return;
    }

    AffSpf(currentLeftPosition,currentTopPosition,0,PtrCadre); // draw top left corner

    while(1) // draw top bar
    {
        currentLeftPosition += 20;

        if(right - 20 <= currentLeftPosition)
            break;

        AffSpf(currentLeftPosition,currentTopPosition,4,PtrCadre);
    }

    AffSpf(currentLeftPosition,currentTopPosition,1,PtrCadre); // draw top right corner

    currentLeftPosition = left;

    while(1) // draw left bar
    {
        currentTopPosition += 20;

        if(bottom - 20 <= currentTopPosition)
            break;

        AffSpf(currentLeftPosition,currentTopPosition,6,PtrCadre);
    }

    currentLeftPosition = right - 8;
    currentTopPosition = top + 20;

    while(bottom - 20 > currentTopPosition)
    {
        AffSpf(currentLeftPosition,currentTopPosition,7,PtrCadre);

        currentTopPosition += 20;
    }

    currentLeftPosition = left;

    AffSpf(currentLeftPosition,currentTopPosition,2,PtrCadre); // draw bottom left corner

    while(1) // draw bottom bar
    {
        currentLeftPosition += 20;

        if(right-20 <= currentLeftPosition)
            break;

        AffSpf(currentLeftPosition,currentTopPosition+12,5,PtrCadre);
    }

    AffSpf(currentLeftPosition,currentTopPosition,3,PtrCadre); // draw bottom right corner

    AffSpf(x-20,currentTopPosition+12,8,PtrCadre); // draw "in the dark"

    WindowX1 = left + 8;
    WindowY1 = top + 8;
    WindowX2 = right - 9;
    WindowY2 = bottom - 9;

    AffRect(WindowX1,WindowY1,WindowX2,WindowY2,0);
    SetClip(WindowX1,WindowY1,WindowX2,WindowY2);

}
