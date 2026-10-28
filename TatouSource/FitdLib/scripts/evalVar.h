///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Variable evaluation declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _EVALVAR_
#define _EVALVAR_

// Reads and evaluates the next value of the running script (AITD1 and Jack)
int evalVar(const char* name = NULL);
// Reads and evaluates the next value of the running script (AITD2 and later)
int evalVar2(const char* name = NULL);

#endif

