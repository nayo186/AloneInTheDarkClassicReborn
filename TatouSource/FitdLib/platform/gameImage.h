///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The game's CD image: GOG's (GAME.GOG, raw 2352-byte sectors, and its track
// list GAME.INS) or the player's own CD (Shared/gameDisc.h). The music and
// the narrator's voices are read straight from it, and with the player's CD
// the game files too (languagePack.cpp); nothing is extracted. Track 1 holds
// the CD's files (ISO 9660: INDARK with the voices, JACK); the next ones are
// the CD music, 16-bit stereo 44.1 kHz. Only read; opened on first use
// (gameFolder.h).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <vector>

namespace SoLoud { class AudioSource; }
class CdImage;

// True once the CD image was read (a GOG installation or the player's CD)
bool gameImageAvailable();

// True if the CD has this file, e.g. ("INDARK2", "MER.PAK")
bool gameImageHasFile(const char* folder, const char* name);
// A file of the CD, e.g. ("INDARK", "000123.VOC"); false if not there
bool gameImageReadFile(const char* folder, const char* name, std::vector<unsigned char>& data);

// A music track of the CD (CD numbering: 2 = the first music track) as a
// SoLoud source that reads it from the image while it plays; null if there
// is no such track. The caller deletes it (after stopping it).
SoLoud::AudioSource* gameImageCreateTrack(int track);

// The same for a track of any CD image read with its track list (a
// language's CD: languagePack.h)
SoLoud::AudioSource* createCdTrack(const CdImage& image, int track);
