///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Game timing, chronometer, and frame rate management
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

int timerSaved = false;
unsigned int timerSavedValue = 0;

void SaveTimerAnim(void)
{
    if(timerSaved==0)
    {
        timerSavedValue = timeGlobal;
    }
	timerSaved++;
}

// The game clock of a loaded save: every timer the save holds (an object's
// CHRONO, the lamp's oil...) counts from it. Also what RestoreTimerAnim puts
// back, so a menu that was open while loading keeps it (BUGS.md C29).
void setGameClock(unsigned int value)
{
    timeGlobal = value;
    timer = value;
    if (timerSaved)
        timerSavedValue = value;
}

// The game's time as a save must hold it: while a menu has stopped the game
// (SaveTimerAnim) the clock keeps running inside the menu, and the game goes
// back to the time it stopped at. Saving the running one put every
// chronometer of the save a few seconds behind: the original game, which
// takes them as they are, missed the lamp's "CHRONO == 10" for ever (C29).
unsigned int gameClockForSave(void)
{
    return timerSaved ? timerSavedValue : timer;
}

void RestoreTimerAnim(void)
{
	assert(timerSaved);
	timerSaved--;
    if(timerSaved == 0)
    {
        timeGlobal = timerSavedValue;
		timer = timeGlobal;
    }
}
