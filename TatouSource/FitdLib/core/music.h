///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Music playback system declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _MUSIC_H_
#define _MUSIC_H_
extern unsigned int musicChrono;
// Starts the AdLib music driver (Jack in the Dark)
int initMusicDriver(void);

extern "C" {
// Fills a sound buffer with the AdLib emulation (called by the mixer)
int musicUpdate(void *udata, uint8 *stream, int len);
// Plays a music: CD track (AITD1) or AdLib (Jack)
void playMusic(int musicNumber);
extern bool g_gameUseCDA;
};

// Advances the AdLib music by one step
void callMusicUpdate(void);
// Stops the AdLib emulation
void destroyMusicDriver(void);
// Fades the music in or out
int fadeMusic(int param1, int param2, int param3);
// Silences the music, CD track and AdLib alike (BUGS.md C33)
void stopMusic();
// True when the music that was playing has ended (CD track or AdLib)
bool isMusicOver();
#endif
