///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Bitmap font rendering and text measurement
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "coreHooks.h"

int fontHeight = 16;

char* fontDataPtr = NULL;
s16 fontCharHeight = 0;
s16 fontRowBytes = 0x1234;
char* fontGlyphBitmaps = NULL;
extern char* fontGlyphBitmaps;
char* fontGlyphTable = NULL;
extern char* fontGlyphTable;
s16 currentFontColor = 0;
extern s16 currentFontColor;
s16 g_fontInterWordSpace = 2;
s16 g_fontInterLetterSpace = 1;
s16 fontDrawColor = 18;
s16 fontCursorX = 0;
s16 fontCursorY = 0x1234;
s16 fontPixelX = 0x1234;
s16 fontBitMask = 0x80;

unsigned char flagTable[]= {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

void SetFont(char* fontData, int color)
{
    s16 tempDx;
    s16 tempAxFlip;

    fontDataPtr = fontData; // fontPtr

    tempDx = READ_LE_S16(fontData); // alignement
    fontData+=2;

    fontCharHeight = *(fontData++); // character height
    fontRowBytes = *(unsigned char*)(fontData++); // character size

    if(!fontRowBytes)
    {
        fontRowBytes = READ_LE_S16(fontData);
    }

    fontData+=2;

    tempAxFlip = READ_LE_S16(fontData);
    fontData+=2;

    tempAxFlip = ((tempAxFlip & 0xFF) << 8 ) | ((tempAxFlip &0xFF00) >> 8);

    fontGlyphBitmaps = fontData;

    fontGlyphTable = fontDataPtr + tempAxFlip - (tempDx&0xFF)*2;

    currentFontColor = color;

    fontDrawColor = color;
}

void SetFontSpace(int interWordSpace, int interLetterSpace)
{
    g_fontInterWordSpace = interWordSpace;
    g_fontInterLetterSpace = interLetterSpace;
}

int ExtGetSizeFont(u8* string)
{
    int width = 0;
    unsigned char character;

    while((character = *(string++)))
    {
        char* dataPtr;
        u16 data;

        dataPtr = fontGlyphTable + character*2;
        data = READ_LE_S16(dataPtr);

        data>>=4;

        data &= 0xF;

        if(!data)
        {
            width += g_fontInterWordSpace;
        }

        width += g_fontInterLetterSpace;
        width += data;
    }

    return(width);
}

void PrintFont(int x, int y, char* surface, u8* string)
{
    unsigned char character;

    fontCursorX = x;
    fontCursorY = y;

    while ((character = *(string++)))
    {
        char* dataPtr;
        u16 data;
        u16 dx;

        dataPtr = fontGlyphTable + character*2;
        data = READ_LE_U16(dataPtr);

        data = ((data & 0xFF)<<8)| ((data&0xFF00)>>8);

        dx = data;

        data >>= 12;

        if(data&0xF) // real character (width != 0)
        {
            char* characterPtr;
            int bp;
            int ch;

            dx &= 0xFFF;

            characterPtr = (dx>>3) + fontGlyphBitmaps;

            fontBitMask = flagTable[dx & 7];

            bp = fontCursorY;

            fontPixelX = fontCursorX;

            for(ch = fontCharHeight; ch>0; ch--)
            {
                if (bp >= 200)
                    return;
                if (fontPixelX < 0 || fontPixelX >= 320)
                    return;

                int cl = data&0xF;

                // Skip this scanline if character would write past right edge
                if (fontPixelX + cl > 320)
                    cl = 320 - fontPixelX;

                char* outPtr = (char*)uiLayer.data() + bp * 320 + fontPixelX;


                int dh = fontBitMask;

                int al = *characterPtr;

                int bx;

                bp++;

                for(bx = 0; cl>0; cl--)
                {
                    if(dh & al)
                    {
                        *(outPtr) = (char)fontDrawColor;
                    }

                    outPtr++;

                    dh = ((dh>>1) & 0x7F) | ((dh<<7)&0x80);

                    if(dh&0x80)
                    {
                        bx++;
                        al = *(characterPtr + bx);
                    }
                }

                characterPtr += fontRowBytes;
            }

            fontCursorX += data&0xF;
        }
        else // space character
        {
            fontCursorX += g_fontInterWordSpace;
        }

        fontCursorX += g_fontInterLetterSpace;
    }
}

void SelectedMessage(int x, int y, int index, int color1, int color2)
{
    textEntryStruct* entryPtr;
    u8* textPtr;

    entryPtr = getTextFromIdx(index);

    if(!entryPtr)
        return;

    textPtr = entryPtr->textPtr;

    x -= (entryPtr->width/2); // center using bitmap font metrics

    SetFont(PtrFont,color2);
    PrintFont(x,y+1,logicalScreen,textPtr);

    SetFont(PtrFont,color1);
    PrintFont(x,y,logicalScreen,textPtr);

}

void SimpleMessage(int x, int y, int index, int color)
{
    textEntryStruct* entryPtr = getTextFromIdx(index);

    if(!entryPtr)
        return;

    u8* textPtr = entryPtr->textPtr;

    x -= (entryPtr->width/2); // center using bitmap font metrics

    SetFont(PtrFont,color);

    PrintFont(x,y+1,logicalScreen,textPtr);
}
