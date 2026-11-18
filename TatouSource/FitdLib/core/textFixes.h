///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Corrections of the games' texts (translation mistakes): a table per game
// in assets/textFixes, compiled in, applied when the texts are loaded.
///////////////////////////////////////////////////////////////////////////////

#pragma once

struct textEntryStruct;

// Replaces the texts of the language in use that the game's table corrects
void applyTextFixes(textEntryStruct* texts, int count);
