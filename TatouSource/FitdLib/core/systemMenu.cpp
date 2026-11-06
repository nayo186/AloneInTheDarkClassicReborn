///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// In-game pause menu, save/load, and options
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "launchOptions.h"
#include "settings.h"
#include "coreHooks.h"
#include "input.h"
#include "bgfxGlue.h"
#include "menuMouse.h"
void SetLevelDestPal(palette_t& inPalette, palette_t& outPalette, int coef);

// Helper function
void playMenuSound(const char* soundName)
{
    if (soundEnabled)
    {
        osystem_playSampleFromName((char*)soundName);
    }
}


#define SELECT_COUL 0xF
#define MENU_COUL 4
#define	SIZE_FONT 16

// Pause menu entries in display order, as in 1992 (the numbers are the
// original option numbers: 9 is "quit")
static const int s_menuEntries[] = { 0, 1, 2, 3, 4, 5, 9 };

// AITD3's entry "Difficulty Level", fourth as in its original (AITD3.EXE,
// 0x13576); its menu's "Intermediate sequence" switches the 3D animations of
// its menus, which these menus do not have (BUGS.md T16)
static const int kDifficultyEntry = 10;
static const int s_menuEntriesAITD3[] = { 0, 1, 2, kDifficultyEntry, 3, 4, 5, 9 };

static int menuEntryCount()
{
    if (g_gameId == AITD3)
        return (int)(sizeof(s_menuEntriesAITD3) / sizeof(int));
    return (int)(sizeof(s_menuEntries) / sizeof(int));
}

static int menuEntryAt(int row)
{
    return (g_gameId == AITD3) ? s_menuEntriesAITD3[row] : s_menuEntries[row];
}

// Screen row of an entry (-1 if hidden)
static int menuRowOf(int entry)
{
    for (int row = 0; row < menuEntryCount(); row++)
        if (menuEntryAt(row) == entry)
            return row;
    return -1;
}

// AITD3's rows go from y 37 every 14, as its original (0x13530)
static const int kAITD3MenuTop = 37;
static const int kAITD3MenuStep = 14;

static int menuEntryY(int entry)
{
    if (g_gameId == AITD3)
        return kAITD3MenuTop + menuRowOf(entry) * kAITD3MenuStep;
    return WindowY1 + ((WindowY2 - WindowY1) / 2) - (menuEntryCount() * SIZE_FONT) / 2 + (menuRowOf(entry) * SIZE_FONT);
}

// Scale down the game screen and draw it to the preview box x1,y1-x2,y2
void scaleDownImage(int x1, int y1, int x2, int y2, char* sourceBuffer)
{
    int previewWidth = x2 - x1 + 1;
    int previewHeight = y2 - y1 + 1;
    if (previewWidth <= 0 || previewHeight <= 0)
        return;

    // Source dimensions: 320x200 pixels (game screen)
    float scaleX = 320.0f / (float)previewWidth;
    float scaleY = 200.0f / (float)previewHeight;

    // Draw scaled image into logicalScreen at the preview position
    for(int y = 0; y < previewHeight; y++)
    {
        for(int x = 0; x < previewWidth; x++)
        {
            // Sample from source using nearest neighbor
            int srcX = (int)(x * scaleX);
            int srcY = (int)(y * scaleY);

            // Ensure we don't read outside source buffer
            if(srcX >= 0 && srcX < 320 && srcY >= 0 && srcY < 200)
            {
                char pixel = sourceBuffer[srcY * 320 + srcX];

                // Write to destination position in logicalScreen
                int dstX = x1 + x;
                int dstY = y1 + y;

                if(dstX >= 0 && dstX < 320 && dstY >= 0 && dstY < 200)
                {
                    logicalScreen[dstY * 320 + dstX] = pixel;
                }
            }
        }
    }
}

void AffOption(int n, int num, int selected)
{
    int y = menuEntryY(n);

    if(n == selected)
    {
        SelectedMessage( 160, y, num, SELECT_COUL, MENU_COUL);
    }
    else
    {
        SimpleMessage( 160, y, num, MENU_COUL);
    }
}

// The text number of a menu entry
static int entryText(int entry)
{
    switch (entry)
    {
    case 0: return 48;
    case 1: return 45;
    case 2: return 46;
    case 3: return 42 - musicEnabled;
    case 4: return 43 + soundEnabled;
    case 5: return 49 + detailLevel;
    case kDifficultyEntry: return 53;
    default: return 47;
    }
}

// AITD2: the preview goes in the top left corner, in a frame that ends
// before the widest entry (its font is bigger than AITD1's) and keeps the
// screen's shape; the picture fills only the inside of the frame (BUGS.md A22)
static void affPreviewAITD2()
{
    int textLeft = 160;
    for (int row = 0; row < menuEntryCount(); row++)
    {
        textEntryStruct* entry = getTextFromIdx(entryText(menuEntryAt(row)));
        if (entry && 160 - entry->width / 2 < textLeft)
            textLeft = 160 - entry->width / 2;
    }

    const int left = WindowX1 + 2;
    const int top = WindowY1 + 2;
    const int width = textLeft - 6 - left;
    if (width < 40)
        return;
    const int height = width * 10 / 16;

    AffBigCadre(left + width / 2, top + height / 2, width, height);
    scaleDownImage(WindowX1, WindowY1, WindowX2, WindowY2, aux2);
}

// AITD3's option screen (ITD_RESS 17, with the menu's own colours) while
// the pause menu is open (BUGS.md T1)
static char* s_optionScreenAITD3 = nullptr;

// AITD3's pause menu: its option screen with the entries in the middle and
// its two arrows (CADRE 1 and 2) beside the chosen one, in the colours of
// its main menu
static void affOptionListAITD3(int selected)
{
    memcpy(logicalScreen, s_optionScreenAITD3, 64000);
    WindowX1 = 0;
    WindowY1 = 0;
    WindowX2 = 319;
    WindowY2 = 199;
    SetClip(WindowX1, WindowY1, WindowX2, WindowY2);

    for (int row = 0; row < menuEntryCount(); row++)
    {
        const int entry = menuEntryAt(row);
        const int y = menuEntryY(entry);
        if (entry != selected)
        {
            SimpleMessage(160, y, entryText(entry), 4);
            continue;
        }
        SelectedMessage(160, y, entryText(entry), 1, 4);
        const textEntryStruct* text = getTextFromIdx(entryText(entry));
        const int halfWidth = text ? text->width / 2 : 0;
        int arrowWidth, arrowHeight;
        SizeSpf(1, PtrCadre, &arrowWidth, &arrowHeight);
        const int arrowTop = y + (fontHeight - arrowHeight) / 2;
        AffSpfI(160 - halfWidth - 6 - arrowWidth, arrowTop, 1, PtrCadre);
        AffSpfI(160 + halfWidth + 6, arrowTop, 2, PtrCadre);
    }
}

// Frees AITD3's option screen and gives the game its colours back
static void closeOptionScreenAITD3()
{
    if (!s_optionScreenAITD3)
        return;
    free(s_optionScreenAITD3);
    s_optionScreenAITD3 = nullptr;
    setPalette(currentGamePalette);
}

void AffOptionList(int selectedStringNumber)
{
    if (g_gameId == AITD3)
    {
        affOptionListAITD3(selectedStringNumber);
        menuWaitVSync();
        return;
    }

    // AITD2's frame pieces are not solid: the menu is drawn on black
    if (g_gameId == AITD2)
        AffRect(0, 0, 319, 199, 0);

    AffBigCadre(160,100,320,200);

    int backupTop = WindowY1;
    int backupBottom = WindowY2;
    int backupLeft = WindowX1;
    int backupRight = WindowX2;

    if (g_gameId == AITD2)
    {
        affPreviewAITD2();
    }
    else
    {
        // Draw preview box: center (80,55), size 120x70; the picture starts
        // 3 pixels inside its top-left corner (20, 20)
        AffBigCadre(80,55,120,70);

        scaleDownImage(23, 23, 136, 91, aux2);
    }

    WindowY1 = backupTop;
    WindowY2 = backupBottom;
    WindowX1 = backupLeft;
    WindowX2 = backupRight;

    SetClip(WindowX1,WindowY1,WindowX2,WindowY2);

    for (int row = 0; row < menuEntryCount(); row++)
        AffOption(menuEntryAt(row), entryText(menuEntryAt(row)), selectedStringNumber);

    menuWaitVSync();
}

// Returns which menu item (0-9) the mouse is currently hovering over, or -1 if none
static int getSystemMenuItemAtMouse()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
        return -1;

    float gameX, gameY;  // through the scene rectangle (black bars, graphics.aspect)
    if (!osystem_windowToGame(io.MousePos.x, io.MousePos.y, &gameX, &gameY))
        return -1;

    const int wx1 = 0, wy1 = 0, wx2 = 320, wy2 = 200;

    if (gameX < wx1 || gameX > wx2)
        return -1;

    for (int row = 0; row < menuEntryCount(); row++)
    {
        int itemY = wy1 + (wy2 - wy1) / 2 - (menuEntryCount() * SIZE_FONT) / 2 + row * SIZE_FONT;
        int itemHeight = SIZE_FONT;
        if (g_gameId == AITD3)
        {
            itemY = kAITD3MenuTop + row * kAITD3MenuStep;
            itemHeight = kAITD3MenuStep;
        }
        if (gameY >= itemY && gameY < itemY + itemHeight)
            return menuEntryAt(row);
    }
    return -1;
}

void processSystemMenu(void)
{

    //int entry = -1;
    int exitMenu = 0;
    int currentSelectedEntry;

    // Freeze the scene snapshot so it preserves the last game frame (with 3D objects)
    osystem_freezeSceneForMenu();

    // The game frame the menu is drawn over
    memcpy(aux2, logicalScreen, 64000);

    SaveTimerAnim();
    //pauseShaking();

    // AITD3: the option screen and its colours; the game's stay in
    // currentGamePalette (the save's thumbnail is made with them)
    palette_t menuPalette;
    if (g_gameId == AITD3)
    {
        coreScreenPictureShown("ITD_RESS", 17);
        s_optionScreenAITD3 = CheckLoadMallocPak("ITD_RESS", 17);
        copyPalette(s_optionScreenAITD3 + 64000, menuPalette);
        convertPaletteIfRequired(menuPalette);
        setPalette(menuPalette);
    }

	if(lightOff)
	{
		//makeBlackPalette();
		//FadeInPhys(0x40,0);
	}

	//clearScreenSystemMenu(unkScreenVar,aux2);

	currentSelectedEntry = 0;

	int previewFrameCounter = 0;
	ImVec2 lastMousePos = { -1.0f, -1.0f }; // reset each menu open so hover requires deliberate movement

	while(!exitMenu)
	{
		// Finalize scene preview readback after enough frames for async GPU readback
		if (previewFrameCounter < 4)
		{
			previewFrameCounter++;
			if (previewFrameCounter == 3)
			{
				osystem_finalizeScenePreview();
			}
		}

		// Reset logicalScreen to the captured game frame each iteration so
		// AffBigCadre/AffOption always paint over a clean base with no leftover pixels.
		memcpy(logicalScreen, aux2, 64000);

		AffOptionList(currentSelectedEntry);
		osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
		osystem_startFrame();
		process_events();

		// If window was resized during process_events, force a full redraw
		if (g_windowWasResized)
		{
			resetWindowResizeFlag();
			// Redraw the menu UI immediately with new window size
			AffOptionList(currentSelectedEntry);
			osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
		}

		flushScreen();

		//  while(!exitMenu)
        {
            localKey = key;
            localClick = Click;
            localJoyD = JoyD;

            // Mouse: hover selects, click confirms (keyboard/gamepad take priority)
            {
                if (menuMouseMoved(lastMousePos, localKey || localJoyD))
                {
                    int mouseItem = getSystemMenuItemAtMouse();
                    if (mouseItem >= 0 && mouseItem != currentSelectedEntry)
                    {
                        currentSelectedEntry = mouseItem;
                    }
                }
                if (menuMouseClicked() && getSystemMenuItemAtMouse() >= 0)
                {
                    localKey = 0x1C;
                    localClick = 0;
                }
            }

            if(!AntiRebond)
            {
                if(localKey == 0x1C || localClick) // enter
                {
                    // Play select sound
                    playMenuSound("Select.wav");

                    switch(currentSelectedEntry)
                    {
                    case 0: // exit menu
                        exitMenu = 1;
                        break;
                    case 1: // save
                        // AITD3: the save list in the game's colours
                        if (g_gameId == AITD3)
                            setPalette(currentGamePalette);
                        makeSave(45);
                        if (g_gameId == AITD3)
                            setPalette(menuPalette);
                        break;
                    case 2: // load
                        if (g_gameId == AITD3)
                            setPalette(currentGamePalette);
                        if(restoreSave(46,1))
                        {
                            closeOptionScreenAITD3();
                            osystem_unfreezeSceneForMenu();
                            FlagInitView = 2;
                            RestoreTimerAnim();
                            // Enter (confirm = inventory) chose the save: wait
                            // until it is released, or the game opens the
                            // inventory at once
                            while (key || JoyD || Click)
                                process_events();
                            //updateShaking();
                            return;
                        }
                        if (g_gameId == AITD3)
                            setPalette(menuPalette);
                        break;
                    case kDifficultyEntry: // AITD3: its difficulty menu
                        difficultyMenuAITD3();
                        break;
                    case 9: // quit to main menu
                        // Jack in the Dark has no main menu to return to (startJACK
                        // calls startGame() once and then FitdMain returns), which
                        // causes the app to hang on quit. Exit straight to desktop
                        // in JACK mode, matching the AITD2/AITD3 "exit" path.
                        if (g_gameId == JACK)
                        {
                            // Out through cleanupAndExit: the sound stopped first; with music
                            // playing, exit() closed its file under the sound thread (BUGS.md A33)
                            cleanupAndExit();
                        }
                        FlagGameOver = 1;
                        exitMenu = 1;
                        break;

                    }
                }
                else
                {
                    if(localKey == 0x1B)
                    {
                        // Play back sound
                        playMenuSound("Back.wav");
                        exitMenu = 1;
                    }
                    if(localJoyD == 1) // up
                    {
                        // previous visible entry (wraps around)
                        currentSelectedEntry = menuEntryAt((menuRowOf(currentSelectedEntry) + menuEntryCount() - 1) % menuEntryCount());

                        // Play navigation sound
                        playMenuSound("Navigation.wav");


                        AntiRebond = 1;
                    }
                    if(localJoyD == 2) // bottom
                    {
                        // next visible entry (wraps around)
                        currentSelectedEntry = menuEntryAt((menuRowOf(currentSelectedEntry) + 1) % menuEntryCount());

                        // Play navigation sound
                        playMenuSound("Navigation.wav");


                        AntiRebond = 1;
                    }
                    // Handle left/right for music toggle
                    if((localJoyD == 4 || localJoyD == 8) && currentSelectedEntry == 3) // left or right on music option
                    {
                        musicEnabled = !musicEnabled;

                        if(!musicEnabled)
                        {
                            // Stop music by playing track -1
                            playMusic(-1);
                        }

                        // Play navigation sound
                        playMenuSound("Navigation.wav");

                        AntiRebond = 1;
                    }
                    // Handle left/right for sound toggle
                    if((localJoyD == 4 || localJoyD == 8) && currentSelectedEntry == 4) // left or right on sound option
                    {
                        soundEnabled = !soundEnabled;
                        // Play navigation sound
                        playMenuSound("Navigation.wav");
                        AntiRebond = 1;
                    }
                }
            }
            else
            {
                if(!localKey && !localJoyD)
                {
                    AntiRebond = 0;
                }
            }
        }

		osystem_flip(NULL);
	}

	// Re-enable per-frame scene snapshot updates now that the menu is closing
	osystem_unfreezeSceneForMenu();

	// Save configuration changes
	saveSettings();

	// Short fade to black like the original (fadeOut(32,2)); the scene then
	// fades back in when it is redrawn
	FadeOutPhys(32, 0);
	closeOptionScreenAITD3();
	while(key || JoyD || Click)
	{
		process_events();
	}
	localKey = localClick = localJoyD = 0;
	FlagInitView = 2;
	RestoreTimerAnim();
}

