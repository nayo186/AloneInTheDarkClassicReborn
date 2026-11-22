///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Languages built into the game, beyond the ones of the player's edition:
// the Russian translations of Alone 1 and Alone 2 by the Old-Games.ru
// translation bureau (assets/translations/aitd1-russian.txt and
// aitd2-russian.txt). With one, the language PAK is "RUSSIAN" (its texts
// come from the translation) and the game's font, and the pictures and
// sequence frames with texts drawn in them, are the player's own with the
// translation's changes. Nothing changes with any other language.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "types.h"

#include <string>
#include <vector>

// The name of the built-in language (its PAK, as the game names languages)
static const char* const kRussianLanguage = "RUSSIAN";

// Uses the built-in language of that name for this session, if there is one
// for the game being started (game: --game, empty for Alone 1); false if not
bool selectTranslation(const std::string& language, const std::string& game);
// Alone 3 from the Russian CD of Old-Games.ru (its ENGLISH.PAK has the
// Russian texts, Shared/knownEditions.h): its texts are read in Russian;
// false if the language PAK is another
bool selectRussianCd(const std::string& language, const std::string& game);
// True while Russian texts are in use (a built-in translation or the
// Russian CD of Alone 3; texts in DOS code page 866)
bool isRussianTranslation();
// An entry the translation gives instead of the player's file (the texts,
// the font or a picture with its changes); null to read the file as it is
const std::vector<u8>* translatedPakEntry(const char* name, int index);
// The entries of a PAK the translation gives whole (its texts); -1 if none
int translatedPakCount(const char* name);
// A frame of a sequence as the translation shows it (320x200, unpacked):
// a copy of frame with its changes into out; false if it does not change it
bool translateSequenceFrame(const char* name, int frame, const u8* pixels, u8* out);
// True if the translation changes that picture (a PAK entry) or that frame
// of a sequence: a remade picture of the player would hide its texts
bool isTranslatedPicture(const char* name, int index, bool sequence);
