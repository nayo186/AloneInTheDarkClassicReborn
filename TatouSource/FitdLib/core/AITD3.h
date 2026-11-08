///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Alone in the Dark 3 function declarations
///////////////////////////////////////////////////////////////////////////////

// AITD3's start (inherited from FITD, not played yet)
void startAITD3();
// AITD3's intro is playing (the original's flag at 0xE104)
extern bool aitd3IntroPlaying;
// Opens a letter, book or notebook of AITD3 (READ)
void AITD3_ReadBook(int index, int type);
// The narrator reads one page of a text of AITD3, if it has a voice
void AITD3_playBookVoice(int index, int page);
// Puts AITD3's 16 text colours (0-63) over colours 0-15 of a picture's palette
void textColoursAITD3(palette_t& palette);
// Keeps the game screen, darkened, as the background of AITD3's inventory
void beginInventoryAITD3();
// Makes that background from the copy of the whole scene once it arrives;
// true if it changed and the inventory must be drawn again
bool updateInventorySceneAITD3();
// Draws AITD3's inventory frame over that background
void drawInventoryAITD3();
// Puts back a box of AITD3's inventory screen before writing in it
void restoreInventoryBoxAITD3(int x1, int y1, int x2, int y2);
// Draws the sprites of AITD3's inventory frame that go over its texts
void redrawInventorySpriteAITD3();
// Forgets the background of AITD3's inventory when it closes
void endInventoryAITD3();
// Shows AITD3's map (Tab) with the arrow where Carnby is
void AITD3_showMap();

// AITD3's difficulty (AITD3.EXE): each value is 0 low, 1 average or 2 high
enum
{
    DIFFICULTY_LIFE,          // Carnby's life points
    DIFFICULTY_HERO_HITS,     // Carnby's hit points: the force of his hits
    DIFFICULTY_VILLAIN_HITS,  // villains' hit points: the force of theirs
};
extern s16 aitd3Difficulty[3];
// The force a hit gives `target`, with AITD3's difficulty applied
int difficultyHitForce(const tObject* target, int force);
// AITD3: doubles or halves the hero's life points when C_VAR 2 names the
// variable that holds them
void difficultyHeroLife();

// The turn an object is drawn and collides with: its beta, but for the
// doors whose script turns them into the wall (they open as far as they can)
int shownBetaAITD3(const tObject* object);

// True if the camera does not draw the object at its present beta (an open
// door behind a wall, whose edge showed along a frame)
bool hiddenAtBetaAITD3(const tObject* object);
