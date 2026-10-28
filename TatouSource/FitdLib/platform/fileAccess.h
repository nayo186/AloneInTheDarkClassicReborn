///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// File access utility declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _FILE_ACCESS_
#define _FILE_ACCESS_
// A game file could not be read: frees everything and exits with a message
void fatalError(int type, const char* name);
// Loads a whole .ITD file into new memory
char* loadFromItd(const char* name);
// Loads an entry of a PAK; a fatal error if it fails
char* CheckLoadMallocPak(const char* name, int index);
#endif
