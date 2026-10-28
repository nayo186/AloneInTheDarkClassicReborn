///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Main game loop declarations
///////////////////////////////////////////////////////////////////////////////

extern "C" {
// The main loop of a game: each frame runs objects, scripts, cameras and rooms and draws, until the game ends
void PlayWorld(int allowSystemMenu, int deltaTime);

// The game waits while the F1 window is open (called by process_events)
void waitForOptionsWindow();
}

// Rooms found dark (LM_LIGHT): entering one again is dark from its first
// frame, before its scripts run again (BUGS.md V23)
void rememberDarkRoom();
// True if the current room is dark (no lamp light)
bool roomAssumedDark();

