///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Save and load game declarations
///////////////////////////////////////////////////////////////////////////////

#include <string>

// Path of a save file of slot `slot` (".ITD", or ".png" for its picture), in the saves folder
std::string saveFilePath(int slot, const char* extension);
// Reads a save file (both the GOG layout and older ones)
int loadSave(int saveNumber);
// The load screen: chooses a slot and loads it
int restoreSave(int arg0, int arg1);
// The save screen: chooses a slot and writes the game (GOG layout for AITD1)
int makeSave(int arg0);
