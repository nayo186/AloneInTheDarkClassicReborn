///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Startup menu declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _STARTUPMENU_H_
#define _STARTUPMENU_H_
// Draws the start menu with an entry selected
void DrawMenu(int selectedEntry);
// The start menu: new game, load or quit; returns the choice
int MainMenu(void);
#endif
