///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// New font of the texts (Alone 1 and Jack): the game measures, places and
// writes every text with its 1992 font as always; this draws each one again
// at the screen's resolution, in the same place and width, with the
// original capitals rebuilt smooth and modern small letters, and a soft
// outline. Lines and pages stay the same in every language.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Fills the hooks (core: each text written; platform: the text layer shown)
void textFontRegister();
