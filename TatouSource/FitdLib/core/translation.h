///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Languages built into the game, beyond the ones of the player's edition:
// today the Russian translation of Alone 1 by the Old-Games.ru translation
// bureau (assets/translations/aitd1-russian.txt). With it the language PAK
// is "RUSSIAN" (its texts come from the translation) and, of ITD_RESS, the
// font, the frames' plate and the title of the start menu's book are the
// player's own with the translation's changes. Nothing changes with any
// other language.
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
// True while the Russian translation is in use (texts in DOS code page 866)
bool isRussianTranslation();
// An entry the translation gives instead of the player's file (the texts,
// or an ITD_RESS entry with its changes); null to read the file as it is
const std::vector<u8>* translatedPakEntry(const char* name, int index);
// The entries of a PAK the translation gives whole (its texts); -1 if none
int translatedPakCount(const char* name);
