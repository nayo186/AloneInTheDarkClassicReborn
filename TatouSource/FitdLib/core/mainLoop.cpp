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
#include "modernStick.h"
#include "cheats.h"
#include "vibration.h"
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
                nextMusic = -1;
            }
        }
        else if(isMusicOver())
        {
            // The piece ended: silence, then the one the scripts queued with
            // NEXT_MUSIC (BUGS.md C32)
            currentMusic = -1;

            if(nextMusic != -1)
            {
                playMusic(nextMusic);
                nextMusic = -1;
            }
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

// AITD2: the scripts of the objects other than the player run 10 times a
// second, as the original did in DOSBox and on the PCs of its time. Some
// count their runs (a pirate fires after three): at 60 fps or more they
// went faster, and their speed depended on the FPS (BUGS.md A17)
static const unsigned int kAITD2ScriptTicks = 6;

// Whether the current object's script runs in this frame: the player's
// always; the others' on their 10 Hz tick, or when something they read
// happens to them (a key frame, the end of an animation, a hit), which lasts
// one frame: the original saw every such event
static bool aitd2ScriptRuns(bool scriptTick)
{
    const tObject* actorPtr = currentProcessedActorPtr;
    if (scriptTick || currentProcessedActorIdx == currentCameraTargetActor)
        return true;
    if ((actorPtr->objectType & AF_ANIMATED) && (actorPtr->END_FRAME || actorPtr->flagEndAnim))
        return true;
    return actorPtr->HIT_BY != -1 || actorPtr->HIT != -1;
}

// What a script that skips frames would miss of the other objects: the end of
// an animation, a key frame change and a touch (COL_BY) last one frame. The
// frame of each is kept, and a script sees those after its own last run
// (BUGS.md A20; the touch, A33: the door opened by touching it after the key
// missed short touches and waited seconds)
static unsigned int s_frameNumber = 0;
static std::array<unsigned int, NUM_MAX_OBJECT> s_endAnimFrame{};
static std::array<unsigned int, NUM_MAX_OBJECT> s_endFrameFrame{};
static std::array<unsigned int, NUM_MAX_OBJECT> s_touchFrame{};
static std::array<unsigned int, NUM_MAX_OBJECT> s_endAnimShownFrame{};   // the frame its next animation began (A30)
static std::array<s16, NUM_MAX_OBJECT> s_endedAnim{};            // the animation that ended at s_endAnimFrame
static std::array<int, NUM_MAX_OBJECT> s_chainedAnim{};          // the animation its last one chains to, + 1 (0: none)
static std::array<s16, NUM_MAX_OBJECT> s_touchedBy{};
static std::array<unsigned int, NUM_MAX_OBJECT> s_scriptRunFrame{};
static unsigned int s_scriptSeesAfter = 0;      // frame after which the running script sees events
static const unsigned int kAITD2StaleFrames = 120;   // an older run belongs to another object in that slot

static void recordAITD2Events()
{
    s_frameNumber++;
    s_scriptSeesAfter = s_frameNumber - 1;
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        const tObject& actor = ListObjets[i];
        if (actor.indexInWorld < 0)
            continue;
        if (actor.flagEndAnim)
        {
            s_endAnimFrame[i] = s_frameNumber;
            s_endedAnim[i] = actor.ANIM;
        }
        if (actor.END_FRAME)
            s_endFrameFrame[i] = s_frameNumber;
        if (actor.COL_BY != -1)
        {
            s_touchFrame[i] = s_frameNumber;
            s_touchedBy[i] = actor.COL_BY;
        }
    }
}

// Runs the current object's script, seeing the events since its last run
static void runAITD2Script()
{
    const unsigned int lastRun = s_scriptRunFrame[currentProcessedActorIdx];
    s_scriptSeesAfter = s_frameNumber - lastRun <= kAITD2StaleFrames ? lastRun : s_frameNumber - 1;
    if (currentProcessedActorPtr->COL_BY == -1 && s_touchFrame[currentProcessedActorIdx] > s_scriptSeesAfter)
        currentProcessedActorPtr->COL_BY = s_touchedBy[currentProcessedActorIdx];
    processLife(currentProcessedActorPtr->life, false);
    s_scriptRunFrame[currentProcessedActorIdx] = s_frameNumber;
    s_scriptSeesAfter = s_frameNumber - 1;
}

// The end of an animation is seen by a script after it, and by every script
// in the frame the animation it chains to begins by itself, also one that saw
// it already: the player gets control back after the key of the door on
// floor 6 when its script sees that end after the key's script said so
// (vars[141]); it waited for the end of the next animation, 7 seconds. Not
// when a script started the next one at that end: it saw it, and would take
// the new one for ended (Carnby slid down the last steps of the stairs of
// floors 5 and 6, script 441, BUGS.md A33)
bool aitd2EndAnimSeen(int actorIdx)
{
    return actorIdx >= 0 && actorIdx < NUM_MAX_OBJECT
        && (s_endAnimFrame[actorIdx] > s_scriptSeesAfter || s_endAnimShownFrame[actorIdx] == s_frameNumber);
}

// The animation a script sees: the one that ended, in its first run after
// that end, also when the next one began meanwhile; the original saw both in
// the frame of the end. Grace waited forever by the car: her script (10
// times a second, A17) waits for the trunk's animation 219 and its end, and
// the trunk was already in 218, the one it chains to (BUGS.md A34)
int aitd2AnimSeen(int actorIdx)
{
    if (actorIdx < 0 || actorIdx >= NUM_MAX_OBJECT)
        return -1;
    if (s_endAnimFrame[actorIdx] > s_scriptSeesAfter)
        return s_endedAnim[actorIdx];
    return ListObjets[actorIdx].ANIM;
}

bool aitd2EndFrameSeen(int actorIdx)
{
    return actorIdx >= 0 && actorIdx < NUM_MAX_OBJECT && s_endFrameFrame[actorIdx] > s_scriptSeesAfter;
}

// Kept by slot, these events belonged to the previous animation or to the
// object that had the slot before: the woman of the intro, put in a slot
// just freed, saw an end of animation and the camera left her at once
// (BUGS.md A27). A new animation keeps the events of the last frame: the end
// of the one before, which the original showed to every script; the door
// pushed with the key waits for it (BUGS.md A30)
void aitd2ForgetEvents(int actorIdx, bool newObject)
{
    if (actorIdx < 0 || actorIdx >= NUM_MAX_OBJECT)
        return;
    if (newObject || s_endAnimFrame[actorIdx] != s_frameNumber)
        s_endAnimFrame[actorIdx] = 0;
    else if (ListObjets[actorIdx].ANIM + 1 == s_chainedAnim[actorIdx])
        s_endAnimShownFrame[actorIdx] = s_frameNumber + 1;   // this frame, once its events are recorded
    s_chainedAnim[actorIdx] = 0;
    if (newObject || s_endFrameFrame[actorIdx] != s_frameNumber)
        s_endFrameFrame[actorIdx] = 0;
    if (newObject)
    {
        s_touchFrame[actorIdx] = 0;
        s_endAnimShownFrame[actorIdx] = 0;
        s_scriptRunFrame[actorIdx] = s_frameNumber;
    }
}

void aitd2NoteChainedAnim(int actorIdx, int anim)
{
    if (actorIdx >= 0 && actorIdx < NUM_MAX_OBJECT)
        s_chainedAnim[actorIdx] = anim + 1;
}

void PlayWorld(int allowSystemMenu, int deltaTime)
{
    g_playWorldActive = true;
    bool bLoop = true;
    unsigned int framesSinceStart = 0;   // since the game began or the pause menu closed, for tatou.log

    while(bLoop)
	{
        framesSinceStart++;
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
		updateModernStick();

        if(localKey)
        {
            // AITD2's intro (the scene on the ship) ends with ESC as with
            // Enter: the pause menu let a game be loaded inside the intro,
            // which then went back to the main menu (BUGS.md A21)
            // AITD3's too: Esc ends it, as on its pictures (BUGS.md T5)
            if(localKey == 0x1B && (g_gameId == AITD2 || g_gameId == AITD3) && allowSystemMenu == 0)
            {
                while(key==0x1B)
                {
                    process_events();
                }
                break;
            }

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



                while(key==0x1B || key == 0x1C || Click)
                {
                    process_events();
                }
                // The menu leaves its own last key (Enter) in localKey: a
                // game loaded there opened the inventory at once (BUGS.md A24).
                // And the action button (Space, A) that confirmed in it in
                // localClick: the game began with the action of the object in
                // hand, Carnby jumped or took his fighting stance (BUGS.md C51)
                localKey = key;
                localClick = Click;
                localJoyD = JoyD;
                printf(MLOOP_TAG "Back to the game after the pause menu\n");
                framesSinceStart = 0;
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

                    printf(MLOOP_TAG "Inventory opened by key 0x%02X, frame %u of play\n", localKey, framesSinceStart);


                    processInventory();



                    // Re-enable lantern lighting after inventory closes

                    while(key==0x1C || key==0x17 || key==0x1B)
                    {
                        process_events();
                        localKey = key;
                    }
                }
            }

            // AITD3's map (Tab), as AITD3.EXE (0xE518): not with Carnby dead.
            // Here also not in the intro nor while the scripts close the
            // inventory, the scenes played with the engine (BUGS.md T28)
            if(localKey == 0x0F && g_gameId == AITD3 && allowSystemMenu && statusScreenAllowed
               && CVars[getCVarsIdx(DEAD_PERSO)] == 0)
            {
                AITD3_showMap();
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

                    // 2D objects (AITD2+) play their hybrid's frames, not a
                    // 3D animation (BUGS.md A15)
                    if(g_gameId >= AITD2 && (flag & AF_OBJ_2D))
                    {
                        animObjet2d();
                    }
                    else if(flag & AF_ANIMATED)
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

            
            // AITD2's 10 Hz tick for the scripts (aitd2ScriptRuns); the
            // clock goes back when a game is loaded
            static unsigned int s_lastScriptTick = 0;
            bool scriptTick = true;
            if (g_gameId == AITD2)
            {
                scriptTick = timer - s_lastScriptTick >= kAITD2ScriptTicks || timer < s_lastScriptTick;
                if (scriptTick)
                    s_lastScriptTick = timer;
                recordAITD2Events();
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
                                    if(g_gameId != AITD2)
                                    {
                                        processLife(currentProcessedActorPtr->life, false);
                                        vibrateOnLifeChange();   // a hit or a death vibrates the controller
                                        applyCheats();   // a hit taken in this script is put back (infinite life)
                                    }
                                    else if(aitd2ScriptRuns(scriptTick))
                                    {
                                        runAITD2Script();
                                        vibrateOnLifeChange();   // a hit or a death vibrates the controller
                                        applyCheats();
                                    }
                                break;
                            }
						case JACK:
                        case AITD1:
                            {
                                if(currentProcessedActorPtr->life != -1)
                                    if(currentProcessedActorPtr->lifeMode != -1)
                                    {
                                        processLife(currentProcessedActorPtr->life, false);
                                        vibrateOnLifeChange();   // a hit or a death vibrates the controller
                                        applyCheats();   // a hit taken in this script is put back (infinite life)
                                    }
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

        updateLoopingSample();
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
    // AITD3 stopped the CD too (AITD3.EXE, 0x181BB): the intro's narration
    // is its music track, and skipped with Esc it went on over the main
    // menu (BUGS.md T4)
    if (g_gameId == AITD3)
        stopMusic();
}

