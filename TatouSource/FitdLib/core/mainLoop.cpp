///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Main game loop, frame processing, and event dispatch
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "launchOptions.h"
#include "settings.h"
#include "consoleLog.h"
#include "anim2d.h"
#include "coreHooks.h"
#include "jobSystem.h"
#include "menuMouse.h"
#include <set>

#ifndef WIN32
#include <sys/time.h>
#endif

int mainLoopSwitch = 0;

void updatePendingEvents(void)
{
    // TODO: miss pending events here

    if(currentMusic!=-1)
    {
        if(currentMusic==-2)
        {
            if(evalChrono(&musicChrono)>180)
            {
                playMusic(nextMusic);
            }
        }
        else
        {
			/*
            if(fadeMusic(0,0,0x10)==-1)
            {
                currentMusic = -1;

                if(nextMusic != -1)
                {
                    playMusic(nextMusic);
                    nextMusic = -1;
                }
            }
			*/
        }
    }
}

// Hide the rocking horse object on CAMERA00_003 to avoid visual artifacts.
// The object is restored automatically when the camera changes away.
static int s_hiddenRockingHorseActorIdx = -1;
static s16 s_savedRockingHorseBodyNum = -1;

static int getAbsoluteCameraIndex()
{
    if (NumCamera < 0 || currentRoom < 0 || currentRoom >= (int)roomDataTable.size())
        return -1;
    if (NumCamera >= (int)roomDataTable[currentRoom].cameraIdxTable.size())
        return -1;
    return roomDataTable[currentRoom].cameraIdxTable[NumCamera];
}

static void restoreRockingHorse()
{
    if (s_hiddenRockingHorseActorIdx >= 0 && s_hiddenRockingHorseActorIdx < NUM_MAX_OBJECT)
    {
        ListObjets[s_hiddenRockingHorseActorIdx].bodyNum = s_savedRockingHorseBodyNum;
        s_hiddenRockingHorseActorIdx = -1;
        s_savedRockingHorseBodyNum = -1;
    }
}

static void updateRockingHorseVisibility()
{
    bool shouldHide = (g_currentFloor == 0 && getAbsoluteCameraIndex() == 3);

    if (shouldHide)
    {
        if (s_hiddenRockingHorseActorIdx >= 0)
            return; // already hidden

        for (int i = 0; i < NUM_MAX_OBJECT; i++)
        {
            tObject* obj = &ListObjets[i];
            if (obj->indexInWorld < 0 || obj->bodyNum < 0)
                continue;

            int worldIdx = obj->indexInWorld;
            if (worldIdx < 0 || worldIdx >= (int)ListWorldObjets.size())
                continue;

            textEntryStruct* entry = getTextFromIdx(ListWorldObjets[worldIdx].foundName);
            if (entry && entry->textPtr)
            {
                if (_stricmp((const char*)entry->textPtr, "ROCKING HORSE") == 0)
                {
                    s_hiddenRockingHorseActorIdx = i;
                    s_savedRockingHorseBodyNum = obj->bodyNum;
                    obj->bodyNum = -1;
                    return;
                }
            }
        }
    }
    else
    {
        restoreRockingHorse();
    }
}

// Dark rooms (BUGS.md V23). The objects' scripts run before the room change
// in the loop below, so the first frame of a room is drawn before its script
// turns the light off again: a room already found dark is drawn dark from
// that frame until its scripts have run once
static std::set<int> s_darkRooms;
static bool s_roomAssumedDark = false;

static int darkRoomKey()
{
    return g_currentFloor * 1000 + currentRoom;
}

void rememberDarkRoom()
{
    s_darkRooms.insert(darkRoomKey());
}

bool roomAssumedDark()
{
    return s_roomAssumedDark;
}

// The F1 options window is open: the game waits wherever it is (playing,
// inventory, a book, a menu...) with its clock and the last image frozen, as
// in the pause menu; the window draws itself over it. Called by
// process_events, so every loop of the game stops (BUGS.md C25). A menu that
// had already frozen the image keeps it frozen afterwards.
void waitForOptionsWindow()
{
    printf(MLOOP_TAG "Options window opened (F1)\n");
    SaveTimerAnim();
    const bool wasLive = osystem_isSceneSnapshotLive();
    const bool wasMenuActive = g_menuActive;
    osystem_freezeSceneForMenu();
    g_menuActive = true;
    while (g_isOptionsWindowOpen())
    {
        osystem_drawFrozenSceneBackground();
        process_events();
    }
    g_menuActive = wasMenuActive;
    if (wasLive)
        osystem_unfreezeSceneForMenu();
    RestoreTimerAnim();
    printf(MLOOP_TAG "Options window closed\n");
}

// A tool froze the game (object editor): nothing moves and the clock stops,
// but the scene is drawn every frame so its changes are seen at once
static void waitFrozenLive()
{
    printf(MLOOP_TAG "Game frozen (object editor)\n");
    SaveTimerAnim();
    while (g_isGameFrozenLive())
    {
        process_events();
        AllRedraw(1);
    }
    RestoreTimerAnim();
    printf(MLOOP_TAG "Game running again\n");
}

void PlayWorld(int allowSystemMenu, int deltaTime)
{
    g_playWorldActive = true;
    bool bLoop = true;

    while(bLoop)
	{
		// Process pending job callbacks from background threads
		JobSystem::instance().processPendingCallbacks();

		process_events();   // waits there while F1 is open

		if (g_isGameFrozenLive && g_isGameFrozenLive())
		{
			waitFrozenLive();
			continue;
		}

		if (g_onGameLoopTick)
			g_onGameLoopTick();

		// Auto-hide cursor during gameplay after inactivity
		menuUpdateGameplayCursor();

		localKey = key;
		localJoyD = JoyD;
		localClick = Click;

        if(localKey)
        {
            if(localKey == 0x1B)
            {
                // g_playWorldActive = false;

                // Freeze the scene snapshot NOW, before the ESC-draining loop.
                // The snapshot still holds the last game frame (with 3D objects).
                // Without this, each process_events() in the loop would overwrite
                // the snapshot with a cleared framebuffer (no 3D objects).
                osystem_captureScenePreview();

                while(key==0x1B)
                {
                    process_events();
                }


                // Ensure cursor is visible in menu
                menuRestoreCursorForMenu();


                printf(MLOOP_TAG "Pause menu opened\n");
                Uint64 pauseStart = SDL_GetTicks();
                g_menuActive = true;
                processSystemMenu();
                g_menuActive = false;
                printf(MLOOP_TAG "Pause menu closed after %llu ms\n", (unsigned long long)(SDL_GetTicks() - pauseStart));



                while(key==0x1B || key == 0x1C)
                {
                    process_events();
                    localKey = key;
                }
                printf(MLOOP_TAG "Back to the game after the pause menu\n");
            }

            if(localKey == 0x1C || localKey == 0x17)
            {
                if(allowSystemMenu == 0)
                {
                    break;
                }

                if(statusScreenAllowed)
                {
                    // Temporarily exit PlayWorld gate so inventory frames render normally
                    // g_playWorldActive = false;

                    // Immediately suppress lantern light to prevent UI bleed


                    // Ensure cursor is visible in inventory
                    menuRestoreCursorForMenu();


                    processInventory();



                    // Re-enable lantern lighting after inventory closes

                    while(key==0x1C || key==0x17 || key==0x1B)
                    {
                        process_events();
                        localKey = key;
                    }
                }
            }
        }
        else
        {
            //      input5 = 0;
        }

        if(localClick)
        {
            if(!allowSystemMenu)
            {
                break;
            }

            action = 0x2000;
        }
        else
        {
            action = 0;
        }

        executeFoundLife(inHandTable[currentInventory]);

        if(FlagChangeEtage == 0)
        {
            if(g_gameId == AITD1)
            {
                bool isDark = (CVars[getCVarsIdx(LIGHT_OBJECT)] != -1);
                if (isDark != g_roomIsDark)
                {
                    g_roomIsDark = isDark;
                    osystem_setDarkRoom(isDark);
                    // Reload camera to switch between normal and _DARK HD background
                    InitView();
                }
            }

            for(currentProcessedActorIdx = 0; currentProcessedActorIdx < NUM_MAX_OBJECT; currentProcessedActorIdx++)
            {
                currentProcessedActorPtr = &ListObjets[currentProcessedActorIdx];
                if(currentProcessedActorPtr->indexInWorld >= 0)
                {
                    currentProcessedActorPtr->COL_BY = -1;
                    currentProcessedActorPtr->HIT_BY = -1;
                    currentProcessedActorPtr->HIT = -1;
                    currentProcessedActorPtr->HARD_DEC = -1;
                    currentProcessedActorPtr->HARD_COL = -1;
                }
            }

            for(currentProcessedActorIdx = 0; currentProcessedActorIdx < NUM_MAX_OBJECT; currentProcessedActorIdx++)
            {
                currentProcessedActorPtr = &ListObjets[currentProcessedActorIdx];
                if(currentProcessedActorPtr->indexInWorld >= 0)
                {
                    int flag = currentProcessedActorPtr->objectType;

                    if((flag & AF_ANIMATED) || (g_gameId >= AITD2 && flag & 0x200))
                    {
                        GereAnim();
                    }
                    if(flag & AF_TRIGGER)
                    {
                        GereDec();
                    }

                    if(currentProcessedActorPtr->animActionType)
                    {
                        GereFrappe();
                    }
                }
            }

            
            for(currentProcessedActorIdx = 0; currentProcessedActorIdx < NUM_MAX_OBJECT; currentProcessedActorIdx++)
            {
                currentProcessedActorPtr = &ListObjets[currentProcessedActorIdx];
                if(currentProcessedActorPtr->indexInWorld >= 0)
                {
                    if(currentProcessedActorPtr->life != -1)
                    {
                        switch(g_gameId)
                        {
                        case AITD2:
                        case AITD3:
                        case TIMEGATE:
                            {
                                if(currentProcessedActorPtr->lifeMode&3)
                                    if(!(currentProcessedActorPtr->lifeMode&4))
                                        processLife(currentProcessedActorPtr->life, false);
                                break;
                            }
						case JACK:
                        case AITD1:
                            {
                                if(currentProcessedActorPtr->life != -1)
                                    if(currentProcessedActorPtr->lifeMode != -1)
                                        processLife(currentProcessedActorPtr->life, false);
                                break;
                            }
                        }
                    }
                }

                if(FlagChangeEtage)
                    break;
            }

            if(FlagGameOver)
            {
                break;
            }
        }

        // The scripts of the current room have run: they decide the light now
        s_roomAssumedDark = false;

        if(FlagChangeEtage)
        {
            // Reset dark room state before loading new floor
            if (g_roomIsDark)
            {
                g_roomIsDark = false;
                osystem_setDarkRoom(false);
            }
            LoadEtage(NewNumEtage);
        }

        if(FlagChangeSalle)
        {
			ChangeSalle(NewNumSalle);
            InitView();
            s_roomAssumedDark = (s_darkRooms.count(darkRoomKey()) != 0);
            // BUG FIX: Removed continue statement - AllRedraw() MUST execute after room transitions
            // to render 3D objects. Objects disappear during combat when room changes occur.
        }

        GereSwitchCamera();

        // Handle 2d objects
        //if (g_gameId >= AITD2) // no need for this test since it would do anything if there are 2d objects
        {
            int memocam = NumCamera;

            NumCamera = NewNumCamera;

            for (currentProcessedActorIdx = 0; currentProcessedActorIdx < NUM_MAX_OBJECT; currentProcessedActorIdx++)
            {
                currentProcessedActorPtr = &ListObjets[currentProcessedActorIdx];
                if (currentProcessedActorPtr->indexInWorld >= 0)
                {
                    if (currentProcessedActorPtr->life != -1)
                    {
                        if (currentProcessedActorPtr->objectType & AF_OBJ_2D)
                        {
                            if (currentProcessedActorPtr->lifeMode & 3)
                                if (!(currentProcessedActorPtr->lifeMode & 4))
                                {
                                    processLife(currentProcessedActorPtr->life, false);
                                    FlagGenereAffList = 1;
                                }
                        }
                    }
                }

                if (FlagChangeEtage)
                    break;
            }

            if (FlagGameOver)
            {
                break;
            }

            NumCamera = memocam;
        }
        if (FlagInitView
#ifdef FITD_DEBUGGER
            || debuggerVar_topCamera
#endif
            )
        {
            InitView();
        }

        //    if(FlagGenereActiveList)
        {
            GenereActiveList();
        } 

        //    if(actorTurnedToObj)
        {
            updateRockingHorseVisibility();
            GenereAffList();
        }

        sortActorList();

        handleAnim2d();

        //    if(FlagRefreshAux2)
        {
            //      setupCameraSub4();
        }

        //    mainLoopSub1();


        updateShaking();

        AllRedraw(flagRedraw);

        updateNextSample();

        updatePendingEvents();
    }

    shakeVar1 = 0;
    shakingAmplitude = 0;

    stopShaking();
    g_playWorldActive = false;
    // The original stopped every sound on leaving the game (game over, back to
    // the menu); a looping sample, like the shaking-floor monster's, otherwise
    // kept playing over the game over picture and the menu (BUGS.md C21)
    osystem_stopSample();
}

