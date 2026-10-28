///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// System pause menu declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _SYSTEMMENU_H_
#define _SYSTEMMENU_H_

// The 1992 pause menu: continue, save, load, music, sound, detail, quit
void processSystemMenu(void);
// Shrinks a 320x200 picture into the save preview box
void scaleDownImage(int destX, int destY, char* sourceBuffer);

#endif

