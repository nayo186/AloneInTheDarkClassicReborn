///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// GOG's CD image (GAME.GOG, raw 2352-byte sectors, and its track list
// GAME.INS): the music and the narrator's voices are read straight from it,
// nothing is extracted. Track 1 holds the CD's files (ISO 9660: INDARK with
// the voices, JACK); tracks 2-17 are the CD music, 16-bit stereo 44.1 kHz.
// Only read; opened on first use when the game runs from a GOG installation
// (gameFolder.h).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <vector>

namespace SoLoud { class AudioSource; }

// True once GAME.GOG and GAME.INS were read (only for a GOG installation)
bool gogImageAvailable();

// A file of the CD, e.g. ("INDARK", "000123.VOC"); false if not there
bool gogImageReadFile(const char* folder, const char* name, std::vector<unsigned char>& data);

// A music track of the CD (CD numbering: 2 = the first music track) as a
// SoLoud source that reads it from the image while it plays; null if there
// is no such track. The caller deletes it (after stopping it).
SoLoud::AudioSource* gogImageCreateTrack(int track);
