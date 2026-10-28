///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Game timing and chronometer declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _ITD_TIME_H_
#define _ITD_TIME_H_
// Stops the game clock (menus, books); calls nest
void SaveTimerAnim(void);
// Lets the game clock run again from where it stopped
void RestoreTimerAnim(void);
// Sets the game clock (loading a game)
void setGameClock(unsigned int value);
// The game clock to save: the time it stopped at if it is stopped
unsigned int gameClockForSave(void);
#endif