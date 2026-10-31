///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Status screen, inventory UI, and character sheet
///////////////////////////////////////////////////////////////////////////////

// seg 4

#include "common.h"
#include "consoleLog.h"
#include "coreHooks.h"
#include "mainLoop.h"



//////////// stuff to move


void FastCopyScreen(void* source, void* dest)
{
	memcpy(dest, source, 64000);
}

// Fade of the final image. In DOS a fade only changed the palette of one
// 320x200 picture that already held everything. Here each frame is rebuilt
// on the GPU and the models are only submitted once, so a palette fade
// flickered (frames with the background only). Instead the palette stays
// at full brightness and the composited image is faded; when the 3D scene
// is on screen its snapshot is shown during the fade.
static void fadeScreen(bool fadeIn, int step)
{
    const bool useSnapshot = g_playWorldActive && osystem_isSceneSnapshotLive();
    const bool savedMenuActive = g_menuActive;
    g_menuActive = true;  // compose every fade frame, even without new 3D

    setPalette(currentGamePalette);  // also resets the screen fade
    g_fadeLevel = 1.0f;

    if (fadeIn)
    {
        osystem_setScreenFade(0.0f);
        if (useSnapshot)
            process_events();  // compose the new scene in black and capture it
    }
    if (useSnapshot)
        osystem_freezeSceneForMenu();

    // One step per tick of the game clock, as at 60 frames per second (the
    // original stepped once per frame, BUGS.md V19)
    int i = fadeIn ? 0 : 256;
    while (fadeIn ? (i < 256) : (i >= 0))
    {
        osystem_setScreenFade(i / 256.0f);
        if (useSnapshot)
        {
            osystem_drawFrozenSceneBackground();
        }
        else
        {
            osystem_drawBackground();
        }
        process_events();
        i += (fadeIn ? step : -step) * ticksThisFrame;
    }
    osystem_setScreenFade(fadeIn ? 1.0f : 0.0f);

    if (useSnapshot)
        osystem_unfreezeSceneForMenu();
    g_menuActive = savedMenuActive;
}

void FadeInPhys(int step,int start)
{
    SaveTimerAnim();

    if(fadeState == 2) // only used for the ending ?
    {
    }
    else if (osystem_canFadeScreen())
    {
        fadeScreen(true, step);
    }
    else
    {
        for(int i=0;i<256;i+=step*ticksThisFrame)  // per tick (V19)
        {
			process_events();
            palette_t localPalette;
            SetLevelDestPal(currentGamePalette,localPalette,i);
            setPalette(localPalette);
			osystem_drawBackground();
        }
    }

    fadeState = 1;

    RestoreTimerAnim();
}

void FadeOutPhys(int step, int var2)
{
    SaveTimerAnim();

    if (osystem_canFadeScreen())
    {
        fadeScreen(false, step);
    }
    else
    {
        for(int i=256;i>=0;i-=step*ticksThisFrame)  // per tick (V19)
        {
			process_events();
            palette_t localPalette;
			SetLevelDestPal(currentGamePalette,localPalette,i);
			setPalette(localPalette);
			osystem_drawBackground();
        }
    }

    RestoreTimerAnim();
}

void process_events( void )
{
    osystem_endOfFrame();
	u32 timeIncrease = osystem_startOfFrame();  // 0 above 60 frames per second
#ifdef FITD_DEBUGGER
	if(debuggerVar_fastForward)
	{
		timeIncrease = 8;
	}
#endif
    timeGlobal += timeIncrease;
	timer=timeGlobal;
	ticksThisFrame = (int)timeIncrease;

	// Every loop of the game passes here: while F1 is open it waits (once:
	// the wait itself calls process_events)
	static bool waiting = false;
	if (!waiting && g_isOptionsWindowOpen && g_isOptionsWindowOpen())
	{
		waiting = true;
		waitForOptionsWindow();
		waiting = false;
	}
}

void startChrono(unsigned int* chrono)
{
    *chrono = timer;
}

int evalChrono(unsigned int* chrono)
{
    return(timer-*chrono);
}

// bp = x, bx = y, cx = z
// out
// xOut = dx, yOut = ax
void Rotate(unsigned int x, unsigned int y, unsigned int z, int* xOut, int* yOut)
{
    if(x)
    {
        int var1 = (((cosTable[(x+0x100)&0x3FF] * y) << 1) &0xFFFF0000) -  (((cosTable[x&0x3FF] *z) <<1) & 0xFFFF0000);
        int var2 = (((cosTable[x&0x3FF] * y) << 1) & 0xFFFF0000) + (((cosTable[(x+0x100)&0x3FF] *z) <<1) & 0xFFFF0000);

        *yOut = var1>>16;
        *xOut = var2>>16;
    }
    else
    {
        *xOut = z;
        *yOut = y;
    }
}

void setCameraTarget(int x,int y,int z,int alpha,int beta,int gamma,int time)
{
    int x1;
    int y1;
    int x2;
    int y2;

    Rotate(alpha+0x200,-time,0,&x1,&y1);
    Rotate(beta+0x200,y1,0,&x2,&y2);

    SetPosCamera(x2+x,-x1+y,y2+z);
    SetAngleCamera(alpha,beta,gamma);
}

// Who asked for a sample, for the [SOUND] lines of tatou.log
static void logSample(const char* what, int num)
{
    printf(MAIN_TAG "[SOUND] %s %d by object %d (life %d), floor %d room %d, tick %u\n",
        what, num, currentProcessedActorIdx, currentLifeNum, g_currentFloor, currentRoom, timer);
}

void playSound(int num)
{
	if(num == -1)
		return;

	if(!soundEnabled)
		return;

	// Skip re-triggering the same sample if it is still playing.
	// Life script opcodes (LM_SAMPLE, LM_ANIM_SAMPLE) fire every game tick,
	// so without this guard the sound restarts from the beginning each frame.
	if(num == LastSample && osystem_isSamplePlaying())
		return;

	char sampleFileName[256] = "";
	if (g_gameId == TIMEGATE)
	{
		strcpy(sampleFileName, "SAMPLES");
	}
	else
	{
		strcpy(sampleFileName, "LISTSAMP");
	}

	int size = getPakSize(sampleFileName,num);
	if(!size)
		return;

	char* ptr = HQR_Get(listSamp,num);
	if(!ptr)
		return;

	osystem_playSample(ptr, size);
	LastSample = num;
}

// Looping sample asked by a script in this pass of the game loop, and in how
// many passes in a row (updateLoopingSample)
static bool s_loopRequested = false;
static int s_loopPasses = 0;

static void startLoopingSample(int num)
{
	if(num == -1)
		return;

	if(!soundEnabled)
		return;

	// Skip re-triggering the same looping sample if it is still playing.
	static int s_lastLoopingSample = -1;
	if(num == s_lastLoopingSample && osystem_isSamplePlaying())
		return;

	char sampleFileName[256] = "";
	if (g_gameId == TIMEGATE)
	{
		strcpy(sampleFileName, "SAMPLES");
	}
	else
	{
		strcpy(sampleFileName, "LISTSAMP");
	}

	int size = getPakSize(sampleFileName,num);
	if(!size)
		return;

	char* ptr = HQR_Get(listSamp,num);
	if(!ptr)
		return;

	// AITD1: the sorcerer's scream as the tree burns (sample 90) was hidden
	// under the music; louder than in the original (BUGS.md C50)
	const float volume = (g_gameId == AITD1 && num == 90) ? 2.0f : 1.0f;
	osystem_playLoopingSample(ptr, size, volume);
	s_lastLoopingSample = num;
	logSample("Looping sample", num);
}

void playSoundLooping(int num)
{
	s_loopRequested = true;
	startLoopingSample(num);
}

// Called once per pass of the game loop, after the scripts. A loop that a
// script asks for in every pass (the library talisman, the shaking monster)
// stops when the script stops asking: the original had a single sample
// channel and the next sample (a footstep) cut it, here it played on in
// every room and after taking the talisman (BUGS.md C47). A loop asked for
// only once (a scene) keeps playing.
void updateLoopingSample()
{
	if (s_loopRequested)
	{
		s_loopPasses++;
	}
	else
	{
		if (s_loopPasses >= 2)
		{
			printf(MAIN_TAG "[SOUND] Looping sample no longer asked for: stopped\n");
			osystem_stopLoopingSamples();
		}
		s_loopPasses = 0;
	}
	s_loopRequested = false;
}

// LM_SAMPLE_THEN / LM_SAMPLE_THEN_REPEAT: play a sample, then the next one
// (looping if 0x4000 is set). The original had a single sample channel, so
// the new sample cut the looping one (e.g. the intro car engine) and the next
// sample started when it ended; here samples overlap, so do both explicitly.
void playSoundThen(int num, int next)
{
	osystem_stopLoopingSamples();
	playSound(num);
	nextSample = next;
	printf(MAIN_TAG "Sound: sample %d, then %d%s\n", num, next & 0x3FFF, (next & 0x4000) ? " (looping)" : "");
}

// Called every frame: start the sample queued by playSoundThen when the
// current one has finished
void updateNextSample()
{
	if (nextSample == -1 || osystem_isSamplePlaying())
		return;

	int num = nextSample & 0x3FFF;
	bool repeat = (nextSample & 0x4000) != 0;
	nextSample = -1;

	printf(MAIN_TAG "Sound: queued sample %d starts%s\n", num, repeat ? " (looping)" : "");
	if (repeat)
		startLoopingSample(num);
	else
		playSound(num);
}

////////////////////////

