///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Alone in the Dark 3 game-specific logic and startup sequence
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "coreHooks.h"
#include "consoleLog.h"
#include <climits>
#include <vector>

#define AITD3_CADRE_SPF						0
#define AITD3_ITDFONT						1
#define AITD3_LETTRE						2
#define AITD3_LIVRE							3
#define AITD3_CARNET						4
#define AITD3_CYM00001						5
#define AITD3_CYM00007						6
#define AITD3_CYM00013						7
#define AITD3_CYM00014						8
#define AITD3_CYM01017						9
#define AITD3_CYM03011						10
#define AITD3_CYM13005						11
#define AITD3_CYM13013						12
#define AITD3_MENU3							13
#define AITD3_INVENTAIRE_CAVERNE			14
#define AITD3_INVENTAIRE_COWBOY				15
#define AITD3_INVENTAIRE_COUGUAR			16
#define AITD3_OPTION_SCREEN					17
#define AITD3_SPRITES_INVENTAIRE_CAVERNE	18
#define AITD3_SPRITES_INVENTAIRE_COWBOY		19
#define AITD3_SPRITES_INVENTAIRE_COUGUAR	20

#define MENU_PAL					47
#define MAP							48

#define NBSPRITES 3 /* amount of sprites by inventory */
#define NBDISPLAY 3 /* amount of inventory screens */

#define MENU_MESSAGES				11
#define MESSAGE_LOAD_GAME			46
#define MESSAGE_SAVE_GAME			45
#define START_OPTIONS_INVENTAIRE	23      /* num message.eng */

// AITD3's game variables (DEFINES.ITD), as AITD3.EXE uses them: AITD2's
// table, but the body of the flash of kind 8 (POIGNARD) is CVar 10, not 16
// (the table of kinds at 0x166BF); CVar 2, MAX_WEIGHT_LOADABLE in AITD2, is
// the variable of the hero's life points (T3, T11), and 16 to 19 are not
// used (BUGS.md T12)
int AITD3KnownCVars[] =
{
	SAMPLE_PAGE,            // 0, a sample (0x17C7A)
	BODY_FLAMME,            // 1, body of the flash of kind 3
	MAX_WEIGHT_LOADABLE,    // 2, the hero's life points variable here
	SAMPLE_CHOC,            // 3, the sample of a hit (0xC3A1)
	DEAD_PERSO,
	JET_SARBACANE,          // 5 to 8, bodies of the flashes of kinds 4 to 7
	TIR_CANON,
	JET_SCALPEL,
	POIVRE,
	DORTOIR,
	POIGNARD,               // 10, body of the flash of kind 8
	NUM_MATRICE_PROTECT_1,
	NUM_MATRICE_PROTECT_2,
	NUM_PERSO,              // 13, the hero
	TYPE_INVENTAIRE,        // 14, the inventory's frame
	PROLOGUE,               // 15, the intro is playing (0xE104)
	-1
};

// The intro is playing: Esc on one of its pictures ends it (BUGS.md T5)
bool aitd3IntroPlaying = false;

// The intro, the story told over pictures (floor 1, room 0)
static void playIntroAITD3()
{
	aitd3IntroPlaying = true;
	startGame(1,0,0);
	aitd3IntroPlaying = false;
}

void startAITD3()
{
	// Lines every 14 pixels, as AITD2 and as the original's lists (AITD3.EXE:
	// the inventory's objects and actions, its menus): with the default 16 the
	// fifth object of the inventory went under its box (BUGS.md T15)
	fontHeight = 14;
	g_gameUseCDA = true;
	playIntroAITD3();

	while(1)
	{
		// Reset HD backgrounds before showing main menu
		// (ensures UI is not covered by HD background after game over or returning from game)
		

		int startupMenuResult = MainMenu();

		switch(startupMenuResult)
		{
		case -1: // timeout
			{
				playIntroAITD3();

				break;
			}
		case 0: // new game
			{
				startGame(0,12,1);

				break;
			}
		case 1: // continue
			{
				if(restoreSave(12,0))
				{
					updateShaking();

					FlagInitView = 2;

					InitView();

					PlayWorld(1, 1);

					//          freeScene();

					FadeOutPhys(8,0);
				}

				break;
			}
		case 2: // exit
			{
				// Out through cleanupAndExit: the sound stopped first; with music
				// playing, exit() closed its file under the sound thread (BUGS.md A33)
				cleanupAndExit();

				break;
			}
		}
	}
}

// Average in everything, as the original starts (AITD3.EXE)
s16 aitd3Difficulty[3] = { 1, 1, 1 };

// As the original (AITD3.EXE, where a hit hands its force to the one hit): a
// hit on the hero follows the villains' hit points and any other Carnby's:
// low 2 less, high 3 more, never under 1. A hit of force 0 hurts no one and
// stays 0 (the original made it 1 or 3) (BUGS.md T3)
int difficultyHitForce(const tObject* target, int force)
{
    if (g_gameId != AITD3 || force <= 0)
        return force;
    const bool onHero = target->indexInWorld == currentWorldTarget;
    const int level = aitd3Difficulty[onHero ? DIFFICULTY_VILLAIN_HITS : DIFFICULTY_HERO_HITS];
    if (level == 0)
        force -= 2;
    else if (level == 2)
        force += 3;
    return force < 1 ? 1 : force;
}

// As the original (AITD3.EXE): C_VAR 2 names the variable with the life
// points of the hero being played (14 Carnby at the start); high doubles
// them and low halves them (BUGS.md T3)
void difficultyHeroLife()
{
    if (g_gameId != AITD3 || CVars.size() < 3)
        return;
    const int var = CVars[2];
    if (var < 0 || var >= varSize / 2)
        return;
    if (aitd3Difficulty[DIFFICULTY_LIFE] == 2)
        vars[var] *= 2;
    else if (aitd3Difficulty[DIFFICULTY_LIFE] == 0)
        vars[var] /= 2;
    printf("[LIFE] Difficulty: hero life points (vars[%d]) %d\n", var, vars[var]);
}

// The 16 colours of the texts in a palette still in 0-63: the original puts
// its table (ds:0xC64) over the colours 0-15 of every picture it shows with
// a text, which are placeholders (63, 0, 63) (BUGS.md T17)
void textColoursAITD3(palette_t& palette)
{
	for (int i = 0; i < 16; i++)
		for (int c = 0; c < 3; c++)
			palette[i][c] = defaultPaletteAITD3[i * 3 + c] >> 2;
}

// The narrator of AITD3's CD: the pages of each text that have a voice, its
// CD track and seconds (AITD3.EXE, table at ds:0x31C; BUGS.md T17)
static const BookVoices s_bookVoicesAITD3[] = {
	{  1, { {51, 10} } },
	{  2, { {25, 30}, {26, 42}, {27, 31} } },
	{  3, { {48, 10} } },
	{  4, { {49, 9} } },
	{  5, { {31, 17} } },
	{  6, { {50, 7} } },
	{  7, { {32, 10} } },
	{  8, { {33, 9} } },
	{  9, { {57, 21}, {58, 7} } },
	{ 10, { {52, 12}, {53, 16}, {54, 16}, {55, 14}, {56, 12} } },
	{ 11, { {24, 7} } },
	{ 15, { {41, 5}, {42, 30}, {43, 10} } },
	{ 16, { {44, 9} } },
	{ 17, { {45, 10}, {46, 19}, {47, 10} } },
	{ 18, { {38, 4}, {39, 9}, {40, 8} } },
	{ 19, { {36, 13}, {37, 17} } },
	{ 21, { {28, 10} } },
	{ 22, { {29, 11} } },
	{ 23, { {30, 12} } },
	{ 24, { {59, 13} } },
	{ 25, { {60, 15} } },
	{ 26, { {61, 11} } },
	{ 37, { {34, 19} } },
	{ 38, { {35, 10} } },
};

// The same table of the Spanish CD: the same tracks, read by its narrator
// for other seconds (its AITD3.EXE differs from GOG's only here)
static const BookVoices s_bookVoicesAITD3Spanish[] = {
	{  1, { {51, 9} } },
	{  2, { {25, 19}, {26, 23}, {27, 19} } },
	{  3, { {48, 12} } },
	{  4, { {49, 15} } },
	{  5, { {31, 17} } },
	{  6, { {50, 9} } },
	{  7, { {32, 12} } },
	{  8, { {33, 10} } },
	{  9, { {57, 27}, {58, 10} } },
	{ 10, { {52, 11}, {53, 14}, {54, 16}, {55, 14}, {56, 10} } },
	{ 11, { {24, 8} } },
	{ 15, { {41, 5}, {42, 36}, {43, 16} } },
	{ 16, { {44, 6} } },
	{ 17, { {45, 8}, {46, 16}, {47, 9} } },
	{ 18, { {38, 4}, {39, 10}, {40, 10} } },
	{ 19, { {36, 12}, {37, 9} } },
	{ 21, { {28, 12} } },
	{ 22, { {29, 15} } },
	{ 23, { {30, 13} } },
	{ 24, { {59, 14} } },
	{ 25, { {60, 18} } },
	{ 26, { {61, 14} } },
	{ 37, { {34, 19} } },
	{ 38, { {35, 10} } },
};

void AITD3_playBookVoice(int index, int page)
{
	if (languageNameString == "ESPAGNOL")
		playBookVoice(s_bookVoicesAITD3Spanish, (int)(sizeof(s_bookVoicesAITD3Spanish) / sizeof(s_bookVoicesAITD3Spanish[0])), index, page);
	else
		playBookVoice(s_bookVoicesAITD3, (int)(sizeof(s_bookVoicesAITD3) / sizeof(s_bookVoicesAITD3[0])), index, page);
}

// READ: a letter, a book or a notebook, with the text areas of AITD3.EXE
// (0x17DD6), the same as AITD2's; nothing opened before (BUGS.md T17)
void AITD3_ReadBook(int index, int type)
{
	int resIdx, left, top, right, bottom;
	switch (type)
	{
	case 0: resIdx = AITD3_LETTRE; turnPageFlag = 0; left = 60; top = 10; right = 245; bottom = 190; break;
	case 1: resIdx = AITD3_LIVRE;  turnPageFlag = 1; left = 48; top = 2;  right = 260; bottom = 197; break;
	case 2: resIdx = AITD3_CARNET; turnPageFlag = 0; left = 50; top = 20; right = 250; bottom = 199; break;
	default: return;
	}

	coreScreenPictureShown("ITD_RESS", resIdx);

	LoadPak("ITD_RESS", resIdx, aux);
	palette_t lpalette;
	copyPalette(aux + 64000, lpalette);
	textColoursAITD3(lpalette);
	convertPaletteIfRequired(lpalette);
	copyPalette(lpalette, currentGamePalette);
	setPalette(lpalette);
	osystem_CopyBlockPhys((unsigned char*)aux, 0, 0, 320, 200);
	Lire(index, left, top, right, bottom, 0, 124, 124);
	endBookVoice();
}

// The inventory of each TYPE_INVENTAIRE: its frame and its 3 sprites, drawn
// over the texts (AITD3.EXE, 0x1071B, 0xFBC2, 0x106D3, tables at 0xFE2 and
// 0xFF4): 0 Carnby the cowboy, 1 the caverns, 2 the cougar. FITD had 0 and 1
// the other way round: the cavern's frame showed in the town (BUGS.md T10)
static const int kInventoryPicture[3] = { AITD3_INVENTAIRE_COWBOY, AITD3_INVENTAIRE_CAVERNE, AITD3_INVENTAIRE_COUGUAR };
static const int kInventorySprites[3] = { AITD3_SPRITES_INVENTAIRE_COWBOY, AITD3_SPRITES_INVENTAIRE_CAVERNE, AITD3_SPRITES_INVENTAIRE_COUGUAR };
static const int kInventorySpriteX[3][3] = { { 0, 128, 286 }, { 0, 129, 286 }, { 0, 110, 282 } };
static const int kInventorySpriteY[3][3] = { { 100, 100, 100 }, { 100, 100, 100 }, { 140, 119, 141 } };

// The game screen when the inventory opened, darkened, and the screen of
// the inventory (that scene under its frame), which its boxes restore
static std::vector<char> s_inventoryScene;
static std::vector<char> s_inventoryScreen;
static char* s_inventorySprites = nullptr;
static int s_inventorySpritesType = -1;

static int inventoryTypeAITD3()
{
	const int type = CVars[getCVarsIdx(TYPE_INVENTAIRE)];
	return (type >= 0 && type < 3) ? type : 0;
}

// The original's dark tone of a colour of the palette (0x1596F): the low 4
// bits are its place in its ramp of 16, from light to dark, and become one
// of 8 dark tones of the palette (120-127)
static char darkTone(int colour)
{
	return (char)(120 + ((colour & 15) >> 1));
}

// Frames until the copy of the scene the graphics card is asked for arrives
static int s_sceneCaptureWait = 0;

// As the original: the scene darkened. First from the game's picture
// (logicalScreen), which has no 3D models (the graphics card draws them);
// then from a copy of the whole scene (updateInventorySceneAITD3)
void beginInventoryAITD3()
{
	s_inventoryScene.resize(64000);
	for (int i = 0; i < 64000; i++)
		s_inventoryScene[i] = darkTone((unsigned char)logicalScreen[i]);
	osystem_captureScenePreview();
	s_sceneCaptureWait = 4;
}

// When the copy of the scene arrives (a few frames after asking), the
// darkened scene is made from it: each pixel takes the nearest colour of the
// palette (16-255) and its dark tone, as the original's single picture had
// Carnby and the others in it (BUGS.md T19). True if the background changed
bool updateInventorySceneAITD3()
{
	if (s_sceneCaptureWait <= 0 || --s_sceneCaptureWait > 0)
		return false;
	osystem_finalizeScenePreview();
	const unsigned char* rgba = osystem_getScenePreviewData();
	const int width = osystem_getScenePreviewWidth();
	const int height = osystem_getScenePreviewHeight();
	if (!rgba || width <= 0 || height <= 0 || s_inventoryScene.size() != 64000)
		return false;

	// Nearest colour, remembered by its colour at 5 bits a channel
	std::vector<short> nearest(32 * 32 * 32, -1);
	for (int y = 0; y < 200; y++)
	{
		const int sy = y * height / 200;
		for (int x = 0; x < 320; x++)
		{
			const unsigned char* p = rgba + ((size_t)sy * width + (size_t)x * width / 320) * 4;
			const int key = ((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3);
			if (nearest[key] < 0)
			{
				int best = 16, bestDistance = INT_MAX;
				for (int c = 16; c < 256; c++)
				{
					const int dr = p[0] - currentGamePalette[c][0];
					const int dg = p[1] - currentGamePalette[c][1];
					const int db = p[2] - currentGamePalette[c][2];
					const int distance = dr * dr + dg * dg + db * db;
					if (distance < bestDistance)
					{
						bestDistance = distance;
						best = c;
					}
				}
				nearest[key] = (short)best;
			}
			s_inventoryScene[y * 320 + x] = darkTone(nearest[key]);
		}
	}
	return true;
}

void drawInventoryAITD3()
{
	const int type = inventoryTypeAITD3();
	if (s_inventoryScene.size() != 64000)
		beginInventoryAITD3();

	// The frame over the darkened scene: its colour 0 lets the scene through
	// (0x159AD); before, the scene was black
	char* picture = CheckLoadMallocPak("ITD_RESS", kInventoryPicture[type]);
	s_inventoryScreen.assign(s_inventoryScene.begin(), s_inventoryScene.end());
	for (int i = 0; i < 64000; i++)
		if (picture[i])
			s_inventoryScreen[i] = picture[i];
	free(picture);
	memcpy(logicalScreen, s_inventoryScreen.data(), 64000);

	if (s_inventorySpritesType != type)
	{
		free(s_inventorySprites);
		s_inventorySprites = CheckLoadMallocPak("ITD_RESS", kInventorySprites[type]);
		s_inventorySpritesType = type;
	}

	statusLeft = 27;
	statusTop = 100;
	statusRight = 159;
	statusBottom = 174;

	SetProjection(((statusRight-statusLeft)/2)+statusLeft,((statusBottom-statusTop)/2) + statusTop,128,400,390);
}

// The original restores a box from the inventory's screen before writing
// in it (0x1534:0x69C); clearing it to black hid the scene (BUGS.md T10)
void restoreInventoryBoxAITD3(int x1, int y1, int x2, int y2)
{
	if (s_inventoryScreen.size() != 64000)
		return;
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 > 319) x2 = 319;
	if (y2 > 199) y2 = 199;
	for (int y = y1; y <= y2; y++)
	{
		memcpy(logicalScreen + y * 320 + x1, s_inventoryScreen.data() + y * 320 + x1, x2 - x1 + 1);
		memset(uiLayer.data() + y * 320 + x1, 0, x2 - x1 + 1);
	}
}

void redrawInventorySpriteAITD3()
{
	if (!s_inventorySprites)
		return;
	const int type = inventoryTypeAITD3();
	for (int i = 0; i < 3; i++)
		AffSpfI(kInventorySpriteX[type][i], kInventorySpriteY[type][i], i, s_inventorySprites);
}

void endInventoryAITD3()
{
	s_inventoryScene.clear();
	s_inventoryScreen.clear();
	s_sceneCaptureWait = 0;
	osystem_unfreezeSceneForMenu();   // the capture froze the copy of the scene
}

// The map (Tab): where the arrow goes on it, by floor and room (AITD3.EXE,
// 0x905E); false where the original has none
static bool mapArrowAITD3(int floor, int room, int& x, int& y)
{
	auto at = [&](int px, int py) { x = px; y = py; return true; };
	switch (floor)
	{
	case 0:
		if (room == 0 || room == 15) return at(207, 134);
		if (room == 1) return at(187, 139);
		if (room == 12) return at(183, 189);
		if (room == 14) return at(211, 113);
		return false;
	case 1: return at(193, 75);
	case 2:
		if (room == 0) return at(172, 162);
		if (room == 1 || room == 2) return at(220, 174);
		return false;
	case 3:
		if (room == 0 || room == 1) return at(245, 61);
		if (room == 2 || room == 3) return at(204, 61);
		if (room == 5) return at(123, 65);
		if (room == 4 || room == 6 || room == 7 || room == 9) return at(156, 70);
		return false;
	case 4: return at(93, 70);
	case 5: return at(90, 45);
	case 6: case 7: case 8: return at(98, 122);
	case 9:
		if (room == 2 || room == 3 || (room >= 5 && room <= 7)) return at(212, 94);
		if (room >= 8 && room <= 10) return at(233, 105);
		return false;
	case 10: return at(173, 100);
	case 11: return at(117, 31);
	case 12:
		if (room == 11) return at(241, 142);
		if (room == 13) return at(279, 138);
		return false;
	case 13: return at(256, 75);
	default: return false;
	}
}

extern unsigned char* polyBackBuffer;
void line(int x1, int y1, int x2, int y2, unsigned char c);

// The arrow, pointing down at (x, y), with the lines and colours 32-37 of
// the map's palette of the original (0x8E14). In the text layer, so that it
// also goes over a picture of the player's own
static void drawMapArrowAITD3(int x, int y)
{
	unsigned char* const saved = polyBackBuffer;
	polyBackBuffer = uiLayer.data();
	line(x, y, x - 3, y - 4, 0x20);
	line(x, y, x - 2, y - 4, 0x21);
	line(x, y, x - 1, y - 4, 0x22);
	line(x, y, x, y - 12, 0x23);
	line(x, y, x + 1, y - 4, 0x22);
	line(x, y, x + 2, y - 4, 0x21);
	line(x, y, x + 3, y - 4, 0x20);
	for (int side = -3; side <= 3; side += 6)
	{
		line(x, y - 11, x + side, y - 14, 0x25);
		line(x, y - 9, x + side, y - 12, 0x24);
		line(x, y - 7, x + side, y - 10, 0x23);
	}
	polyBackBuffer = saved;
}

// The map of the town (ITD_RESS 48), as the original (0x923A): the game
// stops, the map comes up from black and its arrow shows where Carnby is,
// half a second on and half off, until a key or 10 seconds. Where the
// original has no arrow it closed the map at once; here it stays until a
// key, as the others (BUGS.md T28)
void AITD3_showMap()
{
	int arrowX = 0, arrowY = 0;
	const bool arrow = mapArrowAITD3(g_currentFloor, currentRoom, arrowX, arrowY);
	printf(MLOOP_TAG "Map opened: floor %d, room %d, %s\n", g_currentFloor, currentRoom, arrow ? "with its arrow" : "no arrow there");

	SaveTimerAnim();
	while (key || Click)   // Tab, which opened it
		process_events();

	FadeOutPhys(32, 0);
	coreScreenPictureShown("ITD_RESS", MAP);
	LoadPak("ITD_RESS", MAP, aux);
	palette_t mapPalette;
	copyPalette(aux + 64000, mapPalette);
	convertPaletteIfRequired(mapPalette);
	copyPalette(mapPalette, currentGamePalette);
	setPalette(mapPalette);
	uiLayer.fill(0);
	FastCopyScreen(aux, frontBuffer);
	osystem_CopyBlockPhys((unsigned char*)frontBuffer, 0, 0, 320, 200);
	osystem_drawBackground();

	// g_menuActive: the fades show the map, not the 3D scene behind it
	g_menuActive = true;
	FadeInPhys(0x10, 0);

	unsigned int chrono;
	startChrono(&chrono);
	bool arrowShown = false;
	while (!key && !Click && (!arrow || evalChrono(&chrono) < 600))
	{
		process_events();
		const bool showArrow = arrow && (evalChrono(&chrono) / 30) % 2 == 0;
		if (showArrow != arrowShown)
		{
			uiLayer.fill(0);
			if (showArrow)
				drawMapArrowAITD3(arrowX, arrowY);
			arrowShown = showArrow;
		}
		osystem_startFrame();
		osystem_drawBackground();
		osystem_stopFrame();
		osystem_flip(NULL);
	}

	// The key that closed it does nothing in the game
	FadeOutPhys(32, 0);
	g_menuActive = false;
	uiLayer.fill(0);
	while (key || Click || JoyD)
		process_events();
	localKey = localClick = localJoyD = 0;
	FlagInitView = 2;
	RestoreTimerAnim();
	printf(MLOOP_TAG "Map closed\n");
}
