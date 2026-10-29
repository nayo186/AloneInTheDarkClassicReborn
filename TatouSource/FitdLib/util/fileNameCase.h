///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// File names in any case. The game asks for its files in capitals
// (ETAGE00.PAK); Windows finds them however they are written, Linux only with
// the exact case, and a copy or a Wine/Proton installation may differ.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// The path as it is on disk, each folder and the file matched ignoring case
// (Linux and others); the same path if it exists, on Windows, or if nothing
// matches
std::string matchFileCase(const std::string& path);
