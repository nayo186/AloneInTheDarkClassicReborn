///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Books and letters: reading pages and page turns (split from main.cpp)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "launchOptions.h"
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


// A page is turned as in 1992: the new page simply replaces the old one
void turnPageForward()
{
    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
}

void turnPageBackward()
{
    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
}

void readBook(int index, int type, int vocIndex)
{
    SaveTimerAnim();


    // No screen effects during book reading (2D overlay screen)
    osystem_suspendScreenEffects(true);

    // Force endScene() to fire during book render frames (same as system menu / map)
    g_menuActive = true;

    switch (g_gameId)
    {
    case AITD1:
        AITD1_ReadBook(index, type, vocIndex);
        break;
    case JACK:
        JACK_ReadBook(index, type);
        break;
    case AITD2:
        AITD2_ReadBook(index, type);
        break;
    default:
        assert(0);

    }

    g_menuActive = false;

    osystem_suspendScreenEffects(false);

    RestoreTimerAnim();

}

int Lire(int index, int startx, int top, int endx, int bottom, int demoMode, int color, int shadow, int vocIndex)
{
    bool lastPageReached = false;
    u8 tabString[] = "    ";
    int firstpage = 1;
    int page = 0;
    int quit = 0;
    int previousPage = -1;
    int wordEndChar;
    std::array<u8*, 100> ptrpage;
    int currentTextIdx;
    int maxStringWidth;
    u8* textPtr;

    printf(MAIN_TAG "Book: reading text %d (demoMode %d)\n", index, demoMode);

    // Stop lantern lighting while reading book

    SetFont(PtrFont, color);

    maxStringWidth = endx - startx + 4;

    int textIndexMalloc = HQ_Malloc(HQ_Memory, getPakSize(languageNameString.c_str(), index) + 300);
    textPtr = (u8*)HQ_PtrMalloc(HQ_Memory, textIndexMalloc);

    if (!LoadPak(languageNameString.c_str(), index, (char*)textPtr))
    {
        fatalError(1, languageNameString.c_str());
    }

    ptrpage.fill(nullptr);
    ptrpage[0] = textPtr;

    //  LastSample = -1;
    //  LastPriority = -1;

    while (!quit)
    {
        u8* ptrt;
        int currentTextY;
        FastCopyScreen(aux, logicalScreen);
        process_events();
        SetClip(startx, top, endx, bottom);

        uiLayer.fill(0);
        ptrt = ptrpage[page];

        currentTextY = top;
        lastPageReached = false;
        int vocLinesOnPage = 0;

        while (currentTextY <= bottom - 16)
        {
            int line_type = 1;
            int lineWidth = 0;
            int currentStringWidth;
            int currentTextX;

            regularTextEntryStruct* currentText = textTable;

            int numWordInLine = 0;

            int interWordSpace = 0;

            while (true) {
                while (*ptrt == '#')
                {
                    //char* var_1BE = var_1C2;
                    ptrt++;

                    switch (*(ptrt++))
                    {
                    case 'P': // page change
                    {
                        if (currentTextY > top) // Hu ?
                            goto pageChange;
                        break;
                    }
                    case 'T': // tab
                    {
                        currentText->textPtr = tabString;
                        currentText->width = ExtGetSizeFont(currentText->textPtr) + 3;
                        lineWidth += currentText->width;
                        numWordInLine++;
                        currentText++;
                        break;
                    }
                    case 'C': // center
                    {
                        line_type &= ~1;
                        line_type |= 8;
                        break;
                    }
                    case 'G': // print number
                    {
                        currentTextIdx = 0;

                        while (*ptrt >= '0' && *ptrt <= '9')
                        {
                            currentTextIdx = (currentTextIdx * 10 + *ptrt - 48);
                            ptrt++;
                        }

                        if (LoadPak("ITD_RESS", 9, aux2))
                        {
                            assert(0); // when is this used?
                            /*  var_C = printTextSub3(currentTextIdx,aux2);
                            var_A = printTextSub4(currentTextIdx,aux2);

                            if(currentTextY + var_A > bottom)
                            {
                            var_1C2 = var_1BE;

                            goto pageChange;
                            }
                            else
                            {
                            printTextSub5((((right-left)/2)+left)-var_C, currentTextY, currentTextIdx, aux2);
                            currentTextY = var_A;
                            }*/
                        }

                        break;
                    }
                    }
                }

                currentText->textPtr = ptrt;

                do
                {
                    wordEndChar = *((unsigned char*)ptrt++);
                } while (wordEndChar > ' '); // go to the end of the string

                *(ptrt - 1) = 0; // add end of string marker to cut the word

                currentStringWidth = ExtGetSizeFont(currentText->textPtr) + 3;

                if (currentStringWidth > maxStringWidth) {
                    quit = 1;
                    break;
                }

                if (lineWidth + currentStringWidth > maxStringWidth)
                {
                    // Restore the separator overwritten above before re-reading the
                    // word on the next line; otherwise an end-of-text marker (0x1A)
                    // is lost and reading runs past the end of the text
                    *(ptrt - 1) = (u8)wordEndChar;
                    ptrt = currentText->textPtr;
                    break;
                }

                currentText->width = currentStringWidth;
                lineWidth += currentStringWidth;

                numWordInLine++;
                currentText++;

                // eval the character that caused the 'end of word' state
                if (wordEndChar == 26) {
                    line_type &= ~1;
                    line_type |= 4;
                    lastPageReached = true;
                    break;
                }

                if (((wordEndChar == 13) || (wordEndChar == 0)) && (*ptrt < ' ')) {
                    ++ptrt;
                    if (*ptrt == 0xD)
                    {
                        ptrt += 2;
                        line_type &= ~1;
                        line_type |= 2;
                        break;
                    }
                    if (*ptrt == '#')
                    {
                        line_type &= ~1;
                        break;
                    }
                }
            }

            if (line_type & 1) // stretch words on line
            {
                if (numWordInLine > 1)
                    interWordSpace = (maxStringWidth - lineWidth) / (numWordInLine - 1);
            }

            currentText = textTable;

            if (line_type & 8) // center
            {
                currentTextX = startx + ((maxStringWidth - lineWidth) / 2);
            }
            else
            {
                currentTextX = startx;
            }

            for (int i = 0; i < numWordInLine; i++) {
                PrintFont(currentTextX, currentTextY, logicalScreen, currentText->textPtr);
                currentTextX += currentText->width + interWordSpace; // add inter word space
                currentText++;
            }
            currentTextIdx = 0;
            vocLinesOnPage++;


            if (line_type & 2) // font size
            {
                currentTextY += 8;
            }

            currentTextY += 16;

            if (lastPageReached)
                break;
        }

    pageChange:
        if (lastPageReached)
        {
            *(ptrt - 1) = 0x1A; // rewrite End Of Text
        }
        else
        {
            if (page + 1 < (int)ptrpage.size())
                ptrpage[page + 1] = ptrt;
            else
                quit = 1;
        }

        if (demoMode == 0)
        {
            if (page > 0)
            {
                AffSpfI(startx - 19, 185, 12, PtrCadre);
            }

            if (!lastPageReached)
            {
                AffSpfI(endx + 4, 185, 11, PtrCadre);
            }
        }

        if (demoMode == 2)
        {
            if (page > 0)
            {
                AffSpfI(startx - 3, 191, 13, PtrCadre);
            }

            if (!lastPageReached)
            {
                AffSpfI(endx - 10, 191, 14, PtrCadre);
            }
        }

        // When using HD background, copy only pixels that differ from the baseline book
        // background (aux) into uiLayer. This reveals the HD background texture while still
        // overlaying text and navigation arrows on top. The portrait-exclusion approach used
        // previously was wrong for regular books (entire SD background covered the HD image).
        // For intro reading, aux already has the portrait side intact and the text area zeroed,
        // so the delta naturally includes only drawn text pixels - no portrait exclusion needed.

        if (firstpage)
        {
            if (demoMode != 1)
            {
                osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
                FadeInPhys(16, 0);
            }
            else
            {
                if (turnPageFlag)
                {
                    turnPageForward();
                }
                else
                {
                    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
                }
            }

            firstpage = 0;
        }
        else
        {
            if (turnPageFlag)
            {
                if (previousPage < page)
                {
                    turnPageForward();
                }
                else
                {
                    turnPageBackward();
                }
            }
            else
            {
                osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            }
        }

        osystem_drawBackground();
        printf(MAIN_TAG "Book: page %d shown%s\n", page, lastPageReached ? " (last)" : "");

        // Play per-page voice-over: concatenate all line VOCs for this page
        // VOC naming: BBSSLL.VOC where BB=vocIndex, SS=page, LL=line
        if (vocIndex >= 0 && vocLinesOnPage > 0)
        {
            osystem_playVocPageLines(vocIndex, page, vocLinesOnPage);

            // PHASE 3: Pre-load next page VOC in background to eliminate page-turn stalls
            // Schedule the next page to be decoded while user reads current page
            if (page + 1 < (int)ptrpage.size() && !lastPageReached)
            {
                osystem_preDecodeVocPage(vocIndex, page + 1);
            }

        }

        if (demoMode != 1) // mode != 1: normal behavior (user can flip pages)
        {
            do
            {
                process_events();
            } while (key || JoyD || Click);

            while (1)
            {
                process_events();
                localKey = key;
                localJoyD = JoyD;
                localClick = Click;

                // Mouse: click anywhere to exit, left region for prev page, right region for next page
                {
                    static ImVec2 s_lireMouse = { -1.0f, -1.0f };
                    ImVec2 gm = menuGetGameMouse();
                    if (menuMouseClicked() && gm.x >= 0.0f)
                    {
                        // Click anywhere: exit book
                        printf(MAIN_TAG "Book: closed by mouse click on page %d\n", page);
                        quit = 1;
                        break;
                    }
                }

                if ((localKey == 0x1B) || localClick)  // ESC (0x1B from the input layer, 1 in DOS)
                {
                    printf(MAIN_TAG "Book: closed by %s on page %d\n", localKey == 0x1B ? "ESC" : "action key", page);
                    quit = 1;
                    break;
                }

                if ((demoMode == 2) && (localKey == 0x1C)) {
                    quit = 1;
                    break;
                }

                // flip to next page
                if (JoyD & 0xA || localKey == 0x1C)
                {
                    printf(MAIN_TAG "Book: next page requested (key=0x%02X JoyD=0x%02X Click=%d)\n", localKey, JoyD, localClick);
                    if (!lastPageReached)
                    {
                        previousPage = page;
                        page++;

                        if (demoMode == 2 || vocIndex >= 0)
                        {
                            playSound(CVars[getCVarsIdx(SAMPLE_PAGE)]);
                            LastSample = -1;
                            LastPriority = -1;
                        }
                        break;
                    }
                    else {
                        if (localKey == 0x1C) {
                            printf(MAIN_TAG "Book: closed by Enter on the last page %d\n", page);
                            quit = 1;
                            break;
                        }
                    }
                }

                // flip to previous page
                if (JoyD & 5) {
                    printf(MAIN_TAG "Book: previous page requested (key=0x%02X JoyD=0x%02X Click=%d)\n", localKey, JoyD, localClick);
                    if (page > 0) {
                        previousPage = page;
                        page--;
                        if (demoMode == 2 || vocIndex >= 0)
                        {
                            playSound(CVars[getCVarsIdx(SAMPLE_PAGE)]);
                            LastSample = -1;
                            LastPriority = -1;
                        }
                        break;
                    }
                }
            }
        }
        else // Demo mode: pages automatically flips
        {
            unsigned int var_6;
            startChrono(&var_6);

            do
            {
                process_events();
                if (evalChrono(&var_6) > 300)
                {
                    break;
                }
            } while (!key && !Click);

            if (key || Click)
            {
                quit = 1;
            }

            if (!lastPageReached)
            {
                page++;
                playSound(CVars[getCVarsIdx(SAMPLE_PAGE)]);
                LastSample = -1;
            }
            else
            {
                quit = 1;
                demoMode = 0;
            }
        }
    }

    // Wait until the key/click that closed the book is released; otherwise the
    // same ESC reaches the game and opens the pause menu
    while (key || JoyD || Click)
    {
        process_events();
    }

    // Stop any playing voice-over VOC when exiting
    if (vocIndex >= 0)
    {
        osystem_stopVO();
        osystem_clearVocCache();
    }

    HQ_Free_Malloc(HQ_Memory, textIndexMalloc);

    printf(MAIN_TAG "Book: text %d closed\n", index);

    // Resume lantern lighting when book closes

    return(demoMode);
}
