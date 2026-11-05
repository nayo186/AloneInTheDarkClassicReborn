///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Title screen and startup menu system
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "coreHooks.h"
#include "input.h"
#include "settings.h"
#include "menuMouse.h"
#include <SDL.h>
#include <math.h>

// Language entry: PAK base name and display label
struct LanguageEntry
{
    const char* pakName;      // e.g. "ENGLISH"
    const char* displayName;  // e.g. "English"
};

// All supported languages (order matches languageNameTable where relevant)
static const LanguageEntry s_allLanguages[] =
{
    { "ENGLISH",  "English"  },
    { "FRANCAIS", "Fran\x87" "ais" },
    { "ITALIANO", "Italiano" },
    { "ESPAGNOL", "Espa\xA4" "ol"  },
    { "DEUTSCH",  "Deutsch"  },
};
static const int s_allLanguagesCount = sizeof(s_allLanguages) / sizeof(s_allLanguages[0]);

// UI animation state
static u32 s_mainMenuSelTime = 0;

// Helper function to play menu navigation sounds (defined in systemMenu.cpp)
extern void playMenuSound(const char* soundName);

// The three entries: from y 76 every 16 pixels; AITD2's, as in its own
// menu, every line of its font (14) and centred in the frame (BUGS.md A3)
// AITD3's, as its own menu, four entries from y 79 every 14 (AITD3.EXE)
static int menuTop() { return (g_gameId == AITD2) ? 100 - (3 * fontHeight) / 2 : (g_gameId == AITD3) ? 79 : 76; }
static int menuStep() { return (g_gameId == AITD2) ? fontHeight : (g_gameId == AITD3) ? 14 : 16; }

// AITD3's menu has a fourth entry, "Difficulty level", before "Return to
// DOS": with three, choosing it quit the game (BUGS.md T3)
static int menuEntries() { return (g_gameId == AITD3) ? 4 : 3; }
static const int kDifficultyEntry = 2;

// Ticks of the game clock the menu waits untouched before the intro plays
// again: AITD3 15 seconds (900, AITD3.EXE, 0x11286); with the others' value
// it took 18 minutes (BUGS.md T7)
static int menuTimeout() { return (g_gameId == AITD3) ? 900 : 0x10000; }

// AITD3's difficulty menu (AITD3.EXE): rows 0-2 are the three values (texts
// 80-82, value 93 + level), 3-5 the levels Beginner, Intermediate and Expert
// (83-85) and 6 "Return to the previous menu" (86)
static const int kDifficultyRows = 7;
static const int kDifficultyRowY[kDifficultyRows] = { 48, 62, 76, 92, 106, 120, 134 };
static const s16 kDifficultyLevels[3][3] = { { 2, 2, 0 }, { 1, 1, 1 }, { 0, 0, 2 } };

static void drawDifficultyMessage(int x, int y, int text, bool selected)
{
    if (selected)
        SelectedMessage(x, y + 1, text, 1, 4);
    else
        SimpleMessage(x, y, text, 4);
}

static void drawDifficultyMenu(int selectedRow)
{
    LoadPak("ITD_RESS", 13, logicalScreen);
    for (int row = 0; row < kDifficultyRows; row++)
    {
        const bool selected = row == selectedRow;
        if (row < 3)
        {
            drawDifficultyMessage(120, kDifficultyRowY[row], 80 + row, selected);
            drawDifficultyMessage(240, kDifficultyRowY[row], 93 + aitd3Difficulty[row], selected);
        }
        else
        {
            drawDifficultyMessage(160, kDifficultyRowY[row], 80 + row, selected);
        }
    }
}

// The row under the mouse, or -1
static int difficultyRowAtMouse()
{
    const ImVec2 mouse = menuGetGameMouse();
    if (mouse.x < 8 || mouse.x > 311)
        return -1;
    for (int row = 0; row < kDifficultyRows; row++)
        if (mouse.y >= kDifficultyRowY[row] && mouse.y < kDifficultyRowY[row] + 14)
            return row;
    return -1;
}

// AITD3's difficulty menu, from the main menu, as the original: up and down
// choose a row, left and right change a value, Enter on a level sets the
// three values and on "Return" goes back. Also Esc goes back, the mouse
// chooses and a click on a value changes it (BUGS.md T3)
void difficultyMenuAITD3()
{
    int row = 0;
    bool back = false;
    bool antiRebond = true;  // the Enter that opened it
    ImVec2 lastMouse = { -1.0f, -1.0f };

    while (!back)
    {
        flushScreen();
        drawDifficultyMenu(row);
        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        osystem_startFrame();
        process_events();

        int step = 0;      // -1 or +1 on a value
        bool enter = false;

        if (menuMouseMoved(lastMouse, JoyD || key))
        {
            const int hit = difficultyRowAtMouse();
            if (hit >= 0)
                row = hit;
        }
        if (menuMouseClicked() && difficultyRowAtMouse() >= 0)
        {
            row = difficultyRowAtMouse();
            if (row < 3)
                step = 1;
            else
                enter = true;
        }

        if (antiRebond)
        {
            if (!JoyD && !key && !Click)
                antiRebond = false;
        }
        else if (JoyD & 1)
        {
            row = (row + kDifficultyRows - 1) % kDifficultyRows;
            playMenuSound("Navigation.wav");
            antiRebond = true;
        }
        else if (JoyD & 2)
        {
            row = (row + 1) % kDifficultyRows;
            playMenuSound("Navigation.wav");
            antiRebond = true;
        }
        else if ((JoyD & 4) && row < 3)
        {
            step = -1;
            antiRebond = true;
        }
        else if ((JoyD & 8) && row < 3)
        {
            step = 1;
            antiRebond = true;
        }
        else if (key == 28 || Click)
        {
            enter = true;
            antiRebond = true;
        }
        else if (key == 0x1B)
        {
            back = true;
            antiRebond = true;
        }

        if (step != 0)
        {
            aitd3Difficulty[row] = (s16)((aitd3Difficulty[row] + step + 3) % 3);
            playMenuSound("Navigation.wav");
        }
        if (enter)
        {
            playMenuSound("Select.wav");
            if (row >= 3 && row <= 5)
                for (int i = 0; i < 3; i++)
                    aitd3Difficulty[i] = kDifficultyLevels[row - 3][i];
            else if (row == 6)
                back = true;
        }

        osystem_stopFrame();
        osystem_flip(NULL);
    }
    printf("[MENU] Difficulty: life %d, hero hits %d, villain hits %d\n",
        aitd3Difficulty[DIFFICULTY_LIFE], aitd3Difficulty[DIFFICULTY_HERO_HITS], aitd3Difficulty[DIFFICULTY_VILLAIN_HITS]);

    while (JoyD || key || Click)
        process_events();
}

void DrawMenu(int selectedEntry)
{
    if (g_gameId == AITD3) {
        LoadPak("ITD_RESS", 13, logicalScreen);
    }
    else {
        AffBigCadre(160, 100, 320, 80);
    }

    int currentY = menuTop();

    for(int i=0; i<menuEntries(); i++)
    {
        if(i == selectedEntry) // highlight selected entry
        {
            if (g_gameId == AITD3) {
                SelectedMessage(160, currentY+1, i + 11, 1, 4);
            }
            else {
                u32 now = (u32)SDL_GetTicks();
                char color = (char)(100 + (int)(fabsf(sinf((float)now * 0.004f)) * 8.0f));
                int pop = 0;
                if (s_mainMenuSelTime)
                {
                    float t = (float)(now - s_mainMenuSelTime) / 250.0f;
                    if (t < 1.0f)
                        pop = (int)(sinf(t * 3.14159f) * 6.0f);
                }
                // AITD2: inside its frame (WindowX1-2), wider than AITD1's
                const bool insideFrame = g_gameId == AITD2;
                int x1 = (insideFrame ? WindowX1 : 10) - pop;
                int x2 = (insideFrame ? WindowX2 : 309) + pop;
                const int leftmost = insideFrame ? WindowX1 : 8;
                const int rightmost = insideFrame ? WindowX2 : 311;
                if (x1 < leftmost) x1 = leftmost;
                if (x2 > rightmost) x2 = rightmost;
                AffRect(x1, currentY, x2, currentY + menuStep(), color);
                SelectedMessage(160, currentY, i + 11, 15, 4);
            }
        }
        else
        {
            SimpleMessage(160,currentY,i+11,4);
        }

        currentY += menuStep(); // next line
    }
}

int MainMenu(void)
{
    int currentSelectedEntry = 0;
    unsigned int chrono;
    int selectedEntry = -1;
    int AntiRebond = 0;

    // Suppress lantern glow when opening startup menu


    flushScreen();

    // AITD2 and 3: the menu's own colours, not those of the intro's last
    // picture (AITD2: ITD_RESS 59, BUGS.md A3)
    if (g_gameId == AITD2 || g_gameId == AITD3) {
        LoadPak("ITD_RESS", (g_gameId == AITD2) ? 59 : 47, aux);

        palette_t lpalette;
        copyPalette(aux, lpalette);
        convertPaletteIfRequired(lpalette);
        copyPalette(lpalette, currentGamePalette);
        setPalette(lpalette);
    }

    DrawMenu(0);

    osystem_startFrame();
    osystem_stopFrame();
    osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);

    osystem_flip(NULL);
    FadeInPhys(16,0);
    startChrono(&chrono);

	while(evalChrono(&chrono) <= menuTimeout()) // exit loop only if time out or if choice made
	{
		flushScreen();
		DrawMenu(currentSelectedEntry);
		osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
		osystem_startFrame();
		// AITD3: "Difficulty level" opens its menu and comes back here
		if(g_gameId == AITD3 && selectedEntry == kDifficultyEntry)
		{
			osystem_stopFrame();
			osystem_flip(NULL);
			difficultyMenuAITD3();
			selectedEntry = -1;
			AntiRebond = 1;
			startChrono(&chrono);
			continue;
		}
		if(selectedEntry!=-1 || evalChrono(&chrono) > menuTimeout())
		{
			break;
		}

		// AITD3: the time in the F1 window (process_events waits there) does
		// not count, nor does a moving mouse: the intro does not start as
		// soon as it closes (BUGS.md T7)
		const unsigned int beforeEvents = timer;
		process_events();
		if (g_gameId == AITD3 && timer - beforeEvents > 30)
			startChrono(&chrono);

		// Handle window resize
		if (g_windowWasResized)
		{
			resetWindowResizeFlag();
			DrawMenu(currentSelectedEntry);
			osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
		}


		// Mouse: hover selects, click confirms (keyboard/gamepad take priority)
		{
			static ImVec2 s_mainMenuMouse = { -1.0f, -1.0f };
			ImVec2 gm = menuGetGameMouse();
			if (menuMouseMoved(s_mainMenuMouse, JoyD || key))
			{
				if (g_gameId == AITD3)
					startChrono(&chrono);
				int hit = menuMouseHitList(gm.x, gm.y, 8, 311, menuTop(), menuStep(), menuEntries());
				if (hit >= 0 && hit != currentSelectedEntry)
				{
					currentSelectedEntry = hit;
					s_mainMenuSelTime = (u32)SDL_GetTicks();
					startChrono(&chrono);
				}
			}
			if (menuMouseClicked())
			{
				int hit = menuMouseHitList(gm.x, gm.y, 8, 311, menuTop(), menuStep(), menuEntries());
				if (hit >= 0)
				{
					playMenuSound("Select.wav");
					selectedEntry = hit;
				}
			}
		}

		if(AntiRebond)
		{
			if(!JoyD && !key && !Click)
				AntiRebond = 0;
		}
		else if(JoyD&1) // up key
		{
			currentSelectedEntry--;
			if(currentSelectedEntry<0)
				currentSelectedEntry = menuEntries() - 1;
			s_mainMenuSelTime = (u32)SDL_GetTicks();
			playMenuSound("Navigation.wav");
			startChrono(&chrono);
			AntiRebond = 1;
		}
		else if(JoyD&2) // down key
		{
			currentSelectedEntry++;
			if(currentSelectedEntry>=menuEntries())
				currentSelectedEntry = 0;
			s_mainMenuSelTime = (u32)SDL_GetTicks();
			playMenuSound("Navigation.wav");
			startChrono(&chrono);
			AntiRebond = 1;
		}
		else if(key == 28 || Click != 0) // select current entry
		{
			playMenuSound("Select.wav");
			selectedEntry = currentSelectedEntry;
		}

        // The controller help for new players (remaster/ui/controllerHint.cpp)
        if (g_onStartMenuDrawn)
            g_onStartMenuDrawn();

        osystem_stopFrame();
        osystem_flip(NULL);
    }

    // AITD3's "Return to DOS" is its fourth entry: for its start it is the
    // same as the others' third (2, quit)
    if(g_gameId == AITD3 && selectedEntry == 3)
        selectedEntry = 2;

    if(selectedEntry==2) // if exit game, do not fade
    {
        FadeOutPhys(16,0);
    }

    // Enter or the action button that chose the entry must not reach the game:
    // AITD2's new game began with Enter down and opened the inventory (BUGS.md A26)
    while(JoyD || key || Click)
    {
        process_events();
    }

    // Re-enable lantern glow when exiting startup menu

    return(selectedEntry);
}
