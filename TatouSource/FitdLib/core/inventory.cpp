///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Inventory system and item management
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "coreHooks.h"
#include "input.h"
#include <SDL.h>
#include <math.h>

// Menu sound helper (from systemMenu.cpp)
extern void playMenuSound(const char* soundName);

// UI animation state for pulsing highlight and selection pop
static u32 s_invObjSelTime = 0;   // Time when object selection changed
static u32 s_invActSelTime = 0;   // Time when action selection changed

sBody* ShowObjet;
int ShowBody;
int ShowBeta;

// The model of the inventory and of the "object found" window turns 8 angle
// units per tick of the game clock (60 a second), as it did at 60 frames per
// second. The original subtracted 8 per frame, so it turned faster the more
// frames per second were drawn (BUGS.md V19).
static unsigned int showBetaTimer;

static void startTurnShowObjet()
{
    ShowBeta = 0;
    showBetaTimer = timer;
}

// Returns the ticks elapsed since the last call
static int turnShowObjet()
{
    unsigned int elapsed = timer - showBetaTimer;
    showBetaTimer = timer;
    if (elapsed > 15)
        elapsed = 15;  // a long wait (loading) does not jump
    ShowBeta -= 8 * (int)elapsed;
    return (int)elapsed;
}

s16 currentInventory = 0;
s16 numObjInInventoryTable[NUM_MAX_INVENTORY];
s16 inHandTable[NUM_MAX_INVENTORY];
s16 inventoryTable[NUM_MAX_INVENTORY][INVENTORY_SIZE];

int statusLeft;
int statusTop;
int statusRight;
int statusBottom;

int numInventoryActions;
s16 inventoryActionTable[5];

int DrawListObjets(int startIdx, int selectIdx, int selectColor)
{
    int y = WindowY1+1;
    int var_6 = startIdx;
    int var_8 = 0;
    int i;
    int currentObj;
    tWorldObject* objPtr;

	if(g_gameId <= JACK)
	{
		AffBigCadre(160,50,320,100);
		y = WindowY1+1;
	}
	else
	{
		SetClip(27,25,292,98);
		AffRect(27,25,292,98,0);

		WindowX1 = 30;
		WindowY1 = 27;
		WindowX2 = 288;
		WindowY2 = 95;

		y = 28;
	}

	for(i=0;i<5;i++)
	{
		if(startIdx>=numObjInInventoryTable[currentInventory])
			break;

		currentObj = inventoryTable[currentInventory][startIdx];

		objPtr = &ListWorldObjets[currentObj];

		if(startIdx == selectIdx)
		{
			if(g_gameId <= JACK)
			{
				if(selectColor == 15)
				{
					// Pulsing highlight effect
					u32 now = (u32)SDL_GetTicks();
					char color = (char)(100 + (int)(fabsf(sinf((float)now * 0.004f)) * 8.0f));

					// Selection pop effect
					int pop = 0;
					if (s_invObjSelTime)
					{
						float t = (float)(now - s_invObjSelTime) / 250.0f;
						if (t < 1.0f)
							pop = (int)(sinf(t * 3.14159f) * 6.0f);
					}
					int x1 = 0xA - pop;
					int x2 = 0x135 + pop;
					if (x1 < 8) x1 = 8;
					if (x2 > 311) x2 = 311;
					AffRect(x1,y,x2,y+0x10,color);
				}

				SelectedMessage(160,y,objPtr->foundName,selectColor,4);
			}
			else
			{
				SimpleMessage(160,y,objPtr->foundName,selectColor);
			}

			var_8 = currentObj;
		}
		else
		{
			SimpleMessage(160,y,objPtr->foundName,4);
		}

		y += fontHeight;
		startIdx++;
	}


	if(var_6>0)
	{
		AffSpfI(298,10,10,PtrCadre);
	}

    if(var_6+5 < numObjInInventoryTable[currentInventory])
    {
        AffSpfI(298,74,9,PtrCadre);
    }

    return(var_8);
}

void renderInventoryObject(int arg)
{
    SetClip(statusLeft,statusTop,statusRight,statusBottom);
    AffRect(statusLeft,statusTop,statusRight,statusBottom,0);

    turnShowObjet();

    setCameraTarget(0,0,0,60,ShowBeta,0,24000);
    if (ShowObjet)
    {
        setCurrentBodyNum(ShowBody, ShowObjet, HQ_Bodys->string);
        AffObjet(0,0,0,0,0,0,ShowObjet);
    }

    if(arg!=-1)
    {
        u8 buffer[256];
        SetFont(PtrFont,4);
        sprintf_s((char*)buffer, sizeof(buffer), "%d",vars[arg]);
        PrintFont(statusLeft+4,statusTop+4,logicalScreen,buffer);
    }

	switch(g_gameId)
	{
	case AITD2:
		redrawInventorySpriteAITD2();
		break;
	default:
		break;
	}

    menuWaitVSync();
}

void drawInventoryActions(int arg)
{
    int y = 0;

	if(g_gameId <= JACK)
	{
		AffBigCadre(240,150,160,100);
		y = 150 - ((numInventoryActions<<4)/2);
	}
	else
	{
		SetClip(162,100,292,174);
		AffRect(162,100,292,174, 0);

		WindowX1 = 166;
		WindowY1 = 104;
		WindowX2 = 288;
		WindowY2 = 170;

		y = 139 - ((numInventoryActions*fontHeight)/2);
	}

	for(int i=0;i<numInventoryActions;i++)
	{
		if(arg == i)
		{
			if(g_gameId <= JACK)
			{
				// Pulsing highlight effect
				u32 now = (u32)SDL_GetTicks();
				char color = (char)(100 + (int)(fabsf(sinf((float)now * 0.004f)) * 8.0f));

				// Selection pop effect
				int pop = 0;
				if (s_invActSelTime)
				{
					float t = (float)(now - s_invActSelTime) / 250.0f;
					if (t < 1.0f)
						pop = (int)(sinf(t * 3.14159f) * 6.0f);
				}
				int x1 = 170 - pop;
				int x2 = 309 + pop;
				if (x1 < 168) x1 = 168;
				if (x2 > 311) x2 = 311;
				AffRect(x1,y,x2,y+16,color);
				SelectedMessage(240,y,inventoryActionTable[i],15,4);
			}
			else
			{
				SimpleMessage(240,y,inventoryActionTable[i],1);
			}
        }
        else
        {
            SimpleMessage(240,y,inventoryActionTable[i],4);
        }

        y += fontHeight;
    }

	switch(g_gameId)
	{
	case AITD2:
		redrawInventorySpriteAITD2();
		break;
	default:
		break;
	}
}

void processInventory(void)
{
    int exitMenu = 0;
    int choice = 0;
    int firstTime = 1;
    unsigned int chrono;
    int lastSelectedObjectIdx;
    int selectedWorldObjectIdx;
    int selectedObjectIdx;
    int selectedActions;
    int firstObjectDisplayedIdx;
    int antiBounce;
    int modeSelect;
    int var_C;
    int numActionForObject;

    if(!numObjInInventoryTable[currentInventory])
        return;

    // Play inventory open sound
    playMenuSound("Expand.wav");


    firstObjectDisplayedIdx = 0;
    lastSelectedObjectIdx = -1;
    selectedObjectIdx = 0;
    modeSelect = 0;
    antiBounce = 2;

    startTurnShowObjet();

    SaveTimerAnim();
    s16 savedShakingAmplitude = shakingAmplitude;
    s16 savedShakeVar1 = shakeVar1;
    pauseShaking();

    if(lightOff!=0)
    {
        //makeBlackPalette();
    }

	switch(g_gameId)
	{
	case AITD1:
	case JACK:
		AffBigCadre(80,150,160,100);

		statusLeft = WindowX1;
		statusTop = WindowY1;
		statusRight = WindowX2;
		statusBottom = WindowY2;

		SetProjection(((statusRight-statusLeft)/2)+statusLeft,((statusBottom-statusTop)/2) + statusTop,128,400,390);

		break;
	case AITD2:
		drawInventoryAITD2();
		break;
	case AITD3:
		drawInventoryAITD3();
		break;
	default:
		assert(0);
	}


	while(!exitMenu)
	{

		process_events();

		// If window was resized during process_events, force a full redraw
		bool needRedraw = false;
		if (g_windowWasResized)
		{
			resetWindowResizeFlag();
			needRedraw = true;

			// Force redraw of inventory UI
			switch(g_gameId)
			{
			case AITD1:
			case JACK:
				AffBigCadre(80,150,160,100);
				statusLeft = WindowX1;
				statusTop = WindowY1;
				statusRight = WindowX2;
				statusBottom = WindowY2;
				SetProjection(((statusRight-statusLeft)/2)+statusLeft,((statusBottom-statusTop)/2) + statusTop,128,400,390);
				break;
			case AITD2:
				drawInventoryAITD2();
				break;
			case AITD3:
				drawInventoryAITD3();
				break;
			default:
				break;
			}

			// Redraw current state
			if (modeSelect == 0)
			{
				DrawListObjets(firstObjectDisplayedIdx, selectedObjectIdx, 14);
			}
			else
			{
				DrawListObjets(firstObjectDisplayedIdx, selectedObjectIdx, 14);
				drawInventoryActions(lastSelectedObjectIdx);
			}
		}

		osystem_drawBackground();

        localKey = key;
        localJoyD = JoyD;
        localClick = Click;

        if(!localKey && !localJoyD && !localClick)
        {
            antiBounce = 0;
        }

        if(localKey == 0x1B)  // ESC (0x1B from the input layer, 1 in DOS)
        {
            // Play back sound
            playMenuSound("Back.wav");
            choice = 0;
            exitMenu = 1;
        }

        if(modeSelect == 0)
        {
            if(antiBounce<1)
            {
				if((localKey == 0x1C) || (localClick != 0) || (localJoyD == 0xC))
				{
					// Play select sound
					playMenuSound("Select.wav");
					DrawListObjets(firstObjectDisplayedIdx,selectedObjectIdx,14);
					menuWaitVSync();
					osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
					modeSelect = 1;
					lastSelectedObjectIdx = -1;
					selectedActions = 0;
					antiBounce = 2;
					continue;
				}
                else
                {
                    if(localJoyD&1 && selectedObjectIdx>0)
                    {   
                        // Play navigation sound
                        playMenuSound("Navigation.wav");
                        selectedObjectIdx--;
                        s_invObjSelTime = (u32)SDL_GetTicks();
                    }

                    if(localJoyD&2 && selectedObjectIdx < (numObjInInventoryTable[currentInventory]-1))
                    {
                        // Play navigation sound
                        playMenuSound("Navigation.wav");
                        selectedObjectIdx++;
                        s_invObjSelTime = (u32)SDL_GetTicks();
                    }

                    if(firstObjectDisplayedIdx+5 <= selectedObjectIdx)
                    {
                        firstObjectDisplayedIdx++;
                    }

                    if(selectedObjectIdx < firstObjectDisplayedIdx)
                    {
                        firstObjectDisplayedIdx--;
                    }

                    if(localKey || localJoyD || localClick)
                    {
                        if(antiBounce==0)
                        {
                            antiBounce = 1;
                            startChrono(&chrono);
                        }
                    }
                }
            }
            else
            {
                if(antiBounce == 1)
                {
                    if(evalChrono(&chrono)>0x280000)
                    {
                        antiBounce = -1;
                    }
                }
            }

            if(lastSelectedObjectIdx!=selectedObjectIdx)
            {
                selectedWorldObjectIdx = DrawListObjets(firstObjectDisplayedIdx,selectedObjectIdx,15);

                ShowBody = ListWorldObjets[selectedWorldObjectIdx].foundBody;

                ShowObjet = HQR_Get(HQ_Bodys,ShowBody);

                var_C = ListWorldObjets[selectedWorldObjectIdx].foundFlag;

                numInventoryActions = 0;
                numActionForObject = 0;

                while(numActionForObject<11)
                {
                    if(var_C & (1<<numActionForObject))
                    {
                        if(numInventoryActions<5)
                        {
                            inventoryActionTable[numInventoryActions++] = numActionForObject+23;
                        }
                    }
                    numActionForObject ++;
                }

                drawInventoryActions(-1);
                //osystem_flip(NULL);

                lastSelectedObjectIdx = selectedObjectIdx;
            }
        }
        else // actions
        {
            if(antiBounce<1)
            {
                if(localKey == 0x1C || localClick)
                {
                    // Play select sound
                    playMenuSound("Select.wav");
                    selectedObjectIdx = inventoryTable[currentInventory][selectedObjectIdx];
                    action = 1 << (inventoryActionTable[selectedActions] - 23);
                    choice = 1;
                    exitMenu = 1;
                }

                if(localJoyD & 0xC)
                {
                    // Play back sound
                    playMenuSound("Back.wav");
                    drawInventoryActions(-1);
                    modeSelect = 0;
                    lastSelectedObjectIdx = -1;
                    antiBounce = 2;
                    continue;
                }

                if(localJoyD&1 && selectedActions>0)
                {
                    // Play navigation sound
                    playMenuSound("Navigation.wav");
                    selectedActions --;
                    s_invActSelTime = (u32)SDL_GetTicks();
                }
                else if(localJoyD&1 && selectedActions == 0)
                {
                    // At top of actions, go back to object selection
                    // Play back sound
                    playMenuSound("Back.wav");
                    drawInventoryActions(-1);
                    modeSelect = 0;
                    lastSelectedObjectIdx = -1;
                    antiBounce = 2;
                    continue;
                }

                if(localJoyD&2 && selectedActions<(numInventoryActions-1))
                {
                    // Play navigation sound
                    playMenuSound("Navigation.wav");
                    selectedActions++;
                    s_invActSelTime = (u32)SDL_GetTicks();
                }
                else if(localJoyD&2 && selectedActions == (numInventoryActions-1))
                {
                    // At bottom of actions, go back to object selection
                    // Play back sound
                    playMenuSound("Back.wav");
                    drawInventoryActions(-1);
                    modeSelect = 0;
                    lastSelectedObjectIdx = -1;
                    antiBounce = 2;
                    continue;
                }

                if(localKey || localJoyD || localClick)
                {
                    if(antiBounce==0)
                    {
                        antiBounce = 1;
                        startChrono(&chrono);
                    }
                }
            }
            else
            {
                if(antiBounce==1)
                {
                    if(evalChrono(&chrono)>0x280000)
                    {
                        antiBounce = -1;
                    }
                }
            }

            if(lastSelectedObjectIdx != selectedActions)
            {
                lastSelectedObjectIdx = selectedActions;
                drawInventoryActions(lastSelectedObjectIdx);
                menuWaitVSync();
            }
        }
        renderInventoryObject(ListWorldObjets[selectedWorldObjectIdx].floorLife);

        if(firstTime)
        {
            firstTime = 0;
            if(lightOff)
            {
                FadeInPhys(0x40,0);
            }
        }

		osystem_CopyBlockPhys((unsigned char*)logicalScreen,0,0,320,200);
        //osystem_flip(NULL);
    }


    RestoreTimerAnim();

    FlagInitView = 1;

    while(Click || key || JoyD)
    {
      process_events();
    }

    localJoyD = 0;
    localKey = 0;
    localClick = 0;

    if(choice == 1)
    {
        executeFoundLife(selectedObjectIdx);
    }

    shakingAmplitude = savedShakingAmplitude;
    shakeVar1 = savedShakeVar1;
    updateShaking();
}


void cleanClip()
{
    // Clamp clip bounds to screen dimensions to prevent buffer overruns
    int xStart = (clipLeft < 0) ? 0 : clipLeft;
    int xEnd = (clipRight > 320) ? 320 : clipRight;
    int yStart = (clipTop < 0) ? 0 : clipTop;
    int yEnd = (clipBottom > 200) ? 200 : clipBottom;

    for (int x = xStart; x < xEnd; x++)
    {
        for (int y = yStart; y < yEnd; y++)
        {
            logicalScreen[y * 320 + x] = 0;
            uiLayer[y * 320 + x] = 0;
        }
    }
}

void DrawFoundWindow(int menuState, int objectName, int zoomFactor)
{
    cleanClip();

    setCameraTarget(0, 0, 0, 60, ShowBeta, 0, zoomFactor);

    sBody* foundBody = HQR_Get(HQ_Bodys, ShowBody);
    if (foundBody)
    {
        setCurrentBodyNum(ShowBody, foundBody, HQ_Bodys->string);
        AffObjet(0, 0, 0, 0, 0, 0, foundBody);
    }

    SimpleMessage(160, WindowY1, 20, 1);
    SimpleMessage(160, WindowY1 + 16, objectName, 1);
    SimpleMessage(160, WindowY1 + 16, objectName, 1);

    switch (menuState)
    {
    case 0:
    {
        SelectedMessage(130, WindowY2 - 16, 21, 1, 4);
        SimpleMessage(190, WindowY2 - 16, 22, 4);
        break;
    }
    case 1:
    {
        SimpleMessage(130, WindowY2 - 16, 21, 4);
        SelectedMessage(190, WindowY2 - 16, 22, 1, 4);
        break;
    }
    case 2:
    {
        SelectedMessage(160, WindowY2 - 16, 10, 1, 4);
        break;
    }
    }
}

void FoundObjet(int objIdx, int param)
{
    tWorldObject* objPtr;

    if (objIdx < 0)
        return;

    if (param == 2)
    {
        printf(INV_TAG "foundObject with param == 2\n");
    }

    objPtr = &ListWorldObjets[objIdx];

    if (param != 0 && (objPtr->foundFlag & 0xC000))
    {
        return;
    }

    if (objPtr->trackNumber)
    {
        if (timer - objPtr->trackNumber < 300) // prevent from reopening the window every frame
            return;
    }

    objPtr->trackNumber = 0;



    SaveTimerAnim();
    pauseShaking(); // pause the shaking when in foundObject screen

    int weight = 0;
    for (int i = 0; i < numObjInInventoryTable[currentInventory]; i++)
    {
        weight += ListWorldObjets[inventoryTable[currentInventory][i]].positionInTrack;
    }

    int choix = 1;

    if (objPtr->positionInTrack + weight > CVars[getCVarsIdx(MAX_WEIGHT_LOADABLE)] || numObjInInventoryTable[currentInventory] + 1 == 30)
    {
        choix = 3;
    }

    ShowBody = objPtr->foundBody;
    ShowObjet = HQR_Get(HQ_Bodys, ShowBody);

    SetProjection(160, 100, 128, 300, 298);

    int zoom = 15000;
    int stepzoom = -200;
    startTurnShowObjet();

    memset(frontBuffer, 0, 320 * 200);
    FastCopyScreen(frontBuffer, logicalScreen);

    AffBigCadre(160, 100, 240, 120);

    DrawFoundWindow(choix, objPtr->foundName, zoom);
    osystem_flip(NULL);

    AntiRebond = 1;

    int exitflag = 0;

    while (!exitflag)
    {
        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);

        process_events();

        // If window was resized, redraw the found object UI
        if (g_windowWasResized)
        {
            resetWindowResizeFlag();
            memset(frontBuffer, 0, 320 * 200);
            FastCopyScreen(frontBuffer, logicalScreen);
            AffBigCadre(160, 100, 240, 120);
            DrawFoundWindow(choix, objPtr->foundName, zoom);
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        }

        osystem_drawBackground();

        localKey = key;
        localJoyD = JoyD;
        localClick = Click;

        if (!AntiRebond)
        {
            if (localKey == 0x1B)  // ESC (0x1B from the input layer, 1 in DOS)
            {
                if (choix != 2)
                {
                    choix = 0;
                }

                exitflag = 1;
            }
            if (choix != 2)
            {
                if (localJoyD & 4)
                {
                    choix = 0;
                }

                if (localJoyD & 8)
                {
                    choix = 1;
                }
            }

            if (localKey == 28 || localClick != 0)
            {
                while (key)
                {
                    process_events();
                }

                exitflag = 1;
            }
        }
        else
        {
            if (!localKey && !localJoyD && !localClick)
                AntiRebond = 0;
        }

        // One zoom step per tick of the game clock, as at 60 frames per
        // second (the original stepped once per frame, BUGS.md V19)
        for (int ticks = turnShowObjet(); ticks > 0; ticks--)
        {
            zoom += stepzoom; // zoom / dezoom

            if (zoom > 8000) // zoom management
                stepzoom = -stepzoom;

            if (zoom < 25000)
                stepzoom = -stepzoom;
        }

        DrawFoundWindow(choix, objPtr->foundName, zoom);

        //    menuWaitVSync();
    }


    RestoreTimerAnim();


    if (choix == 1)
    {
        take(objIdx);
    }
    else
    {
        objPtr->trackNumber = timer;
    }

    while (key && Click)
    {
        process_events();
    }

    localJoyD = 0;
    localKey = 0;
    localClick = 0;

    //  if(mainLoopVar1 != 0)
    {
        //setupShaking(-600);
    }

    FlagInitView = 1;
}