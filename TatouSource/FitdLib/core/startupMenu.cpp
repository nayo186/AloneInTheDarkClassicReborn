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

void DrawMenu(int selectedEntry)
{
    if (g_gameId == AITD3) {
        LoadPak("ITD_RESS", 13, logicalScreen);
    }
    else {
        AffBigCadre(160, 100, 320, 80);
    }

    int currentY = 76;

    for(int i=0; i<3; i++)
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
                int x1, x2;
                {
                    x1 = 10 - pop;
                    x2 = 309 + pop;
                    if (x1 < 8) x1 = 8;
                    if (x2 > 311) x2 = 311;
                }
                AffRect(x1, currentY, x2, currentY+16, color);
                SelectedMessage(160, currentY, i + 11, 15, 4);
            }
        }
        else
        {
            SimpleMessage(160,currentY,i+11,4);
        }

        currentY+=16; // next line
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

    if (g_gameId == AITD3) {
        LoadPak("ITD_RESS", 47, aux);

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

	while(evalChrono(&chrono) <= 0x10000) // exit loop only if time out or if choice made
	{
		flushScreen();
		DrawMenu(currentSelectedEntry);
		osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
		osystem_startFrame();
		if(selectedEntry!=-1 || evalChrono(&chrono) > 0x10000)
		{
			break;
		}

		process_events();

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
				// 3 items starting at Y=76, 16px each
				int hit = menuMouseHitList(gm.x, gm.y, 8, 311, 76, 16, 3);
				if (hit >= 0 && hit != currentSelectedEntry)
				{
					currentSelectedEntry = hit;
					s_mainMenuSelTime = (u32)SDL_GetTicks();
					startChrono(&chrono);
				}
			}
			if (menuMouseClicked())
			{
				int hit = menuMouseHitList(gm.x, gm.y, 8, 311, 76, 16, 3);
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
				currentSelectedEntry = 2;
			s_mainMenuSelTime = (u32)SDL_GetTicks();
			playMenuSound("Navigation.wav");
			startChrono(&chrono);
			AntiRebond = 1;
		}
		else if(JoyD&2) // down key
		{
			currentSelectedEntry++;
			if(currentSelectedEntry>2)
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

    if(selectedEntry==2) // if exit game, do not fade
    {
        FadeOutPhys(16,0);
    }

    while(JoyD)
    {
        process_events();
    }

    // Re-enable lantern glow when exiting startup menu

    return(selectedEntry);
}
