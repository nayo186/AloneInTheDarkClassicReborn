///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Status screen and inventory UI declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _TATOU_H_
#define _TATOU_H_

// AITD1's start screen: the turning 3D armadillo (Tatou) until a key is pressed
int make3dTatou(void);
// Between two game frames: shows the frame, starts the next one, advances the game clock and waits while F1 is open
void process_events( void );
// Copies a whole 320x200 screen
void FastCopyScreen(void* source, void* dest);
// Turns the point (y, z) by angle x (1024 steps per turn)
void Rotate(unsigned int x, unsigned int y, unsigned int z, int* xOut, int* yOut);
// Fades the screen in from black
void FadeInPhys(int var1,int var2);
// Fades the screen out to black
void FadeOutPhys(int var1, int var2);
// Plays sound effect number `num` (LISTSAMP)
void playSound(int num);
// Plays sound effect number `num` in a loop
void playSoundLooping(int num);
// Plays a sound effect and then another one
void playSoundThen(int num, int next);
// Starts the sound effect waiting after the current one (playSoundThen)
void updateNextSample();
// Moves the camera towards a position and angles in `time` ticks
void setCameraTarget(int x,int y,int z,int alpha,int beta,int gamma,int time);

// Starts a chronometer (keeps the current time)
void startChrono(unsigned int* chrono);
// Ticks passed since startChrono
int evalChrono(unsigned int* chrono);
//////////////
#endif
