///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark 2 - Jack Is Back Again
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Embedded data registry header
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>

// Look up an embedded file by filename (case-insensitive, path-stripped).
// Returns true if found, with outData pointing to the embedded byte array
// and outSize set to its length. The data pointer is valid for the lifetime
// of the program (static storage).
//
// Only remaster helper assets (e.g. ControllerHint.png) are embedded; game
// data (PAK/ITD) always comes from the player's own copy on disk.
bool getEmbeddedFile(const char* filename, const unsigned char** outData, size_t* outSize);
