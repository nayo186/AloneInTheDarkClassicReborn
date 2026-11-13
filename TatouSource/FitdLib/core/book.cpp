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
#include "sequence.h"
#include <math.h>
#include <SDL.h>

#include "bgfxGlue.h"
#include <bgfx/bgfx.h>

#include "settings.h"
#include "jobSystemInit.h"

#include <algorithm>
#include <array>
#include <vector>
#include <filesystem>
#include <string>
#include <fstream>


// The page on screen (picture and text layer), for AITD3's page curl
static std::vector<char> s_shownPage;
static std::vector<unsigned char> s_shownText;

static void rememberShownPage()
{
    s_shownPage.assign(logicalScreen, logicalScreen + 64000);
    s_shownText.assign(uiLayer.begin(), uiLayer.end());
}

// AITD3's books turn their page as the original (AITD3.EXE, 0x17432
// forward, 0x174CF back, 0x17361 the back of the sheet): the fold, a light
// line (colour 16), goes from x 260 to the left 10 pixels a step; behind it
// the back of the sheet goes up to x `back` = 280 - (280 - fold) / 2, in
// bands of colours 19 to 24 that each take half of what is left; past it the
// page below. One step a tick of the game clock, at any FPS (BUGS.md T18)
static void curlPageAITD3(bool forward)
{
    const std::vector<char> nextPage(logicalScreen, logicalScreen + 64000);
    const std::vector<unsigned char> nextText(uiLayer.begin(), uiLayer.end());
    if (s_shownPage.size() != 64000 || s_shownText.size() != 64000)
    {
        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        return;
    }

    std::vector<int> folds;
    for (int fold = 260; 280 - (280 - fold) / 2 >= -1; fold -= 10)
        folds.push_back(fold);

    // Forward the fold uncovers the next page; back it is the same sheet
    // laid down again, from the left, over the page on screen
    const std::vector<char>& leftPage = forward ? s_shownPage : nextPage;
    const std::vector<unsigned char>& leftText = forward ? s_shownText : nextText;
    const std::vector<char>& rightPage = forward ? nextPage : s_shownPage;
    const std::vector<unsigned char>& rightText = forward ? nextText : s_shownText;

    unsigned int start;
    startChrono(&start);
    for (;;)
    {
        const int step = evalChrono(&start);
        if (step >= (int)folds.size())
            break;
        const int fold = folds[forward ? step : folds.size() - 1 - step];
        const int back = 280 - (280 - fold) / 2;

        // The colour of each column of the back of the sheet
        unsigned char bands[320] = {};
        int from = fold + 1;
        for (int colour = 19; colour <= 24; colour++)
        {
            const int to = (colour == 24) ? back : from + (back - from) / 2;
            for (int x = (std::max)(from, 0); x <= (std::min)(to, 319); x++)
                bands[x] = (unsigned char)colour;
            from = to;
        }

        for (int y = 0; y < 200; y++)
        {
            for (int x = 0; x < 320; x++)
            {
                const int i = y * 320 + x;
                if (x < fold)
                {
                    logicalScreen[i] = leftPage[i];
                    uiLayer[i] = leftText[i];
                }
                else if (x == fold)
                {
                    logicalScreen[i] = 16;
                    uiLayer[i] = 0;
                }
                else if (x <= back)
                {
                    logicalScreen[i] = (char)bands[x];
                    uiLayer[i] = 0;
                }
                else
                {
                    logicalScreen[i] = rightPage[i];
                    uiLayer[i] = rightText[i];
                }
            }
        }
        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        process_events();
        osystem_startFrame();
        osystem_drawBackground();
        osystem_stopFrame();
        osystem_flip(NULL);
    }

    memcpy(logicalScreen, nextPage.data(), 64000);
    std::copy(nextText.begin(), nextText.end(), uiLayer.begin());
    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
}

// A page is turned as in 1992: the new page simply replaces the old one;
// AITD3's books curl it as their original
void turnPageForward()
{
    if (g_gameId == AITD3)
    {
        curlPageAITD3(true);
        return;
    }
    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
}

void turnPageBackward()
{
    if (g_gameId == AITD3)
    {
        curlPageAITD3(false);
        return;
    }
    osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
}

// A text with a voice was read: its voice took the CD from the music
static bool s_bookVoicePlayed = false;

// The narrator of the CD versions: each page of a text plays a CD track for
// some seconds, from a table of the game's executable by text and page
// (AITD2, BUGS.md A18; AITD3, T17)
void playBookVoice(const BookVoices* table, int count, int index, int page)
{
    for (int i = 0; i < count; i++)
    {
        const BookVoices& voices = table[i];
        if (voices.text != index)
            continue;
        if (!s_bookVoicePlayed)
        {
            osystem_stopTrack();
            s_bookVoicePlayed = true;
        }
        const int pages = (int)(sizeof(voices.pages) / sizeof(voices.pages[0]));
        const BookVoice voice = page < pages ? voices.pages[page] : BookVoice{};
        if (voice.track)
            osystem_playVoiceTrack(voice.track, voice.seconds);
        else
            osystem_stopVO();
        return;
    }
}

void endBookVoice()
{
    if (!s_bookVoicePlayed)
        return;
    s_bookVoicePlayed = false;
    osystem_stopVO();
    const int music = currentMusic;
    if (music >= 0)
    {
        currentMusic = -1;
        playMusic(music);
    }
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
    case AITD3:
        AITD3_ReadBook(index, type);
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

    // AITD3: the samples given by DEF_SEQUENCE_SAMPLE before READ_ON_PICTURE
    // (the narrator of the piano's story) play one after the other while
    // the text is open (AITD3.EXE 0x17BAB; BUGS.md T26)
    int nextSequenceSample = 0;

    // Stop lantern lighting while reading book

    SetFont(PtrFont, color);

    // The first page comes up from black: setting the picture's palette had
    // put the brightness back, and one frame showed the last screen in the
    // new colours at full brightness (BUGS.md A33)
    if (demoMode != 1)
        osystem_setScreenFade(0.0f);

    maxStringWidth = endx - startx + 4;

    // The text in memory of its own: it was taken from HQ_Memory, the 1992
    // pool of 10000 bytes shared with blood, smoke and sparks; with half of
    // it in use (the effects of a fight, those left by loading a game), the
    // cellar book of the rats (text 17, about 5 KB) found no room and was
    // written at address 0: the game closed (issue #12, BUGS.md C76)
    std::vector<u8> text((size_t)(std::max)(getPakSize(languageNameString.c_str(), index), 0) + 300, 0);
    textPtr = text.data();

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

        // Lines as tall as the font: 16 in AITD1 and Jack, 14 in AITD2, which
        // fit one more per page (BUGS.md A29)
        while (currentTextY <= bottom - fontHeight)
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
                    u8* commandStart = ptrt;
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
                    case 'G': // illustration (sprite of ITD_RESS 9), centred
                    {
                        // Was left unwritten (an assert), so books showed no
                        // illustration, like the sacrificial dagger (BUGS.md C48)
                        currentTextIdx = 0;

                        while (*ptrt >= '0' && *ptrt <= '9')
                        {
                            currentTextIdx = (currentTextIdx * 10 + *ptrt - 48);
                            ptrt++;
                        }

                        if (LoadPak("ITD_RESS", 9, aux2))
                        {
                            int pictureWidth;
                            int pictureHeight;
                            SizeSpf(currentTextIdx, aux2, &pictureWidth, &pictureHeight);
                            printf(MAIN_TAG "Book: illustration %d (%dx%d) at y %d\n", currentTextIdx, pictureWidth, pictureHeight, currentTextY);

                            if (currentTextY + pictureHeight > bottom && currentTextY > top)
                            {
                                // Does not fit: it starts the next page
                                ptrt = commandStart;
                                goto pageChange;
                            }

                            AffSpfI(((endx - startx) / 2) + startx - pictureWidth / 2, currentTextY, currentTextIdx, aux2);
                            currentTextY += pictureHeight;
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


            if (line_type & 2) // blank line after a paragraph: half a line
            {
                currentTextY += fontHeight / 2;
            }

            currentTextY += fontHeight;

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
        if (g_gameId == AITD3)
            rememberShownPage();
        printf(MAIN_TAG "Book: page %d shown%s\n", page, lastPageReached ? " (last)" : "");

        if (g_gameId == AITD2)
            AITD2_playBookVoice(index, page);
        else if (g_gameId == AITD3)
            AITD3_playBookVoice(index, page);

        // Play per-page voice-over: concatenate all line VOCs for this page
        // VOC naming: BBSSLL.VOC where BB=vocIndex, SS=page, LL=line
        if (vocIndex >= 0 && vocLinesOnPage > 0)
        {
            osystem_playVocPage(vocIndex, page);

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
                // A sample queued by SAMPLE_THEN still follows the one playing:
                // the narrator of AITD2's story said its second part only after
                // the page, and the next picture's voice cut it (BUGS.md A34)
                updateNextSample();
                if (g_gameId == AITD3 && nextSequenceSample < numSequenceParam && !osystem_isSamplePlaying())
                {
                    playSound(sequenceParams[nextSequenceSample++].sample);
                    LastSample = -1;
                    LastPriority = -1;
                }
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

                        // AITD3 turns every page with its sound (AITD3.EXE,
                        // 0x17C7A; BUGS.md T18)
                        if (demoMode == 2 || vocIndex >= 0 || g_gameId == AITD3)
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
                        if (demoMode == 2 || vocIndex >= 0 || g_gameId == AITD3)
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

    printf(MAIN_TAG "Book: text %d closed\n", index);

    // Resume lantern lighting when book closes

    return(demoMode);
}
