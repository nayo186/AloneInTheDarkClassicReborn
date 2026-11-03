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
// AITD2: whether the object's animation ended (or its key frame changed)
// since the running script last ran, as it would have seen it every frame
// (BUGS.md A20)
bool aitd2EndAnimSeen(int actorIdx);
bool aitd2EndFrameSeen(int actorIdx);
// The animation the running script sees: the one that ended since its last
// run, even if the next one began (BUGS.md A34)
int aitd2AnimSeen(int actorIdx);
// Forgets those events of an object that starts a new animation (but those of
// the last frame), or of a slot a new object takes (newObject: all; its
// script then sees only what comes)
void aitd2ForgetEvents(int actorIdx, bool newObject);
// The animation an ended one chains to by itself (ANIM_ONCE's next): its
// start shows that end once more to every script (BUGS.md A33)
void aitd2NoteChainedAnim(int actorIdx, int anim);

