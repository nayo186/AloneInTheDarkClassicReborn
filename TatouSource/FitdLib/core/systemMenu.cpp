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

static int menuEntryCount()
{
    return (int)(sizeof(s_menuEntries) / sizeof(int));
}

static int menuEntryAt(int row)
{
    return s_menuEntries[row];
}

// Screen row of an entry (-1 if hidden)
static int menuRowOf(int entry)
{
    for (int row = 0; row < menuEntryCount(); row++)
        if (menuEntryAt(row) == entry)
            return row;
    return -1;
}

static int menuEntryY(int entry)
{
    return WindowY1 + ((WindowY2 - WindowY1) / 2) - (menuEntryCount() * SIZE_FONT) / 2 + (menuRowOf(entry) * SIZE_FONT);
}

// Scale down the game screen and draw it to the preview box
// destX, destY = top-left corner of the AffBigCadre box
void scaleDownImage(int destX, int destY, char* sourceBuffer)
{
    // Preview box is created at position (80,55) with size (120x70)
    // Account for frame border (approximately 3 pixels on each side)
    int frameThickness = 3;
    int previewWidth = 120 - (frameThickness * 2);   // 114 pixels
    int previewHeight = 75 - (frameThickness * 2);   // 64 pixels
    int previewStartX = destX + frameThickness;
    int previewStartY = destY + frameThickness;

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
                int dstX = previewStartX + x;
                int dstY = previewStartY + y;

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

void AffOptionList(int selectedStringNumber)
{
    AffBigCadre(160,100,320,200);

    int backupTop = WindowY1;
    int backupBottom = WindowY2;
    int backupLeft = WindowX1;
    int backupRight = WindowX2;

    {
        // Draw preview box: center (80,55), size 120x70
        // This means top-left corner is at (80-60, 55-35) = (20, 20)
        AffBigCadre(80,55,120,70);

        scaleDownImage(20, 20, aux2);
    }

    WindowY1 = backupTop;
    WindowY2 = backupBottom;
    WindowX1 = backupLeft;
    WindowX2 = backupRight;

    SetClip(WindowX1,WindowY1,WindowX2,WindowY2);

    AffOption(0,48,selectedStringNumber);
    AffOption(1,45,selectedStringNumber);
    AffOption(2,46,selectedStringNumber);
    AffOption(3,42-musicEnabled,selectedStringNumber);
    AffOption(4,43+soundEnabled,selectedStringNumber);
    AffOption(5,49+detailLevel,selectedStringNumber);

    AffOption(9,47,selectedStringNumber);

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
        if (gameY >= itemY && gameY < itemY + SIZE_FONT)
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
                        makeSave(45);
                        break;
                    case 2: // load
                        if(restoreSave(46,1))
                        {
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
                        break;
                    case 9: // quit to main menu
                        // Jack in the Dark has no main menu to return to (startJACK
                        // calls startGame() once and then FitdMain returns), which
                        // causes the app to hang on quit. Exit straight to desktop
                        // in JACK mode, matching the AITD2/AITD3 "exit" path.
                        if (g_gameId == JACK)
                        {
                            freeAll();
                            exit(0);
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
	while(key || JoyD || Click)
	{
		process_events();
	}
	localKey = localClick = localJoyD = 0;
	FlagInitView = 2;
	RestoreTimerAnim();
}

