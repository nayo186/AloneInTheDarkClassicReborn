///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// OS abstraction layer interface
///////////////////////////////////////////////////////////////////////////////

#ifndef _OSYSTEM_H_
#define _OSYSTEM_H_

#ifdef __cplusplus
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "palette.h"

#endif

#define byte char
#define u8 unsigned char
#ifndef __cplusplus
#define true 1
#define false 0
#endif

    enum e_rendererType
    {
        OPENGL_OLD,
        OPENGL_ES,
        OPENGL_3_2,
    };

    extern enum e_rendererType g_rendererType;

	// Starts the system layer: window, graphics, sound, controller and ImGui
	void osystem_init();

	extern int osystem_mouseRight;
	extern int osystem_mouseLeft;

	// Draws the 320x200 background (or the player's picture) at the back of the scene
	void osystem_drawBackground();
	// Start of a game frame: waits for the render thread and opens the frame
	u32 osystem_startOfFrame();
	// End of a game frame: draws what is pending and the text layer, and shows the frame
	void osystem_endOfFrame();


	// Sets the game palette (256 RGB colours) and clears the screen fade
	void osystem_setPalette(unsigned byte * palette);
	// Sets the game palette (256 RGB colours) and clears the screen fade
	void osystem_setPalette(palette_t* palette);
	// Darkens the palette for a room without light (and back)
	void osystem_setDarkRoom(bool dark);
	// Dark room (lightOff): covers the scene drawn so far with black; the
	// text layer is drawn after it and the glass of this frame is skipped
	void osystem_drawDarkness();
	// Draws the 3D primitives still pending
	void osystem_flip(unsigned char *videoBuffer);
	// Sends a rectangle of the 320x200 screen to the graphics card
	void osystem_CopyBlockPhys(unsigned char* videoBuffer, int left, int top, int right, int bottom);
	// Plays a sound effect read from a file by its name
	void osystem_playSampleFromName(char* sampleName);
	// Plays a sound effect from memory (the game's VOC data)
	void osystem_playSample(char* samplePtr,int size);
	// Plays a sound effect in a loop (ambient sound; one at a time)
	void osystem_playLoopingSample(char* samplePtr, int size);
	// Stops every sound effect
	void osystem_stopSample();
	// Stops the looping sound effects
	void osystem_stopLoopingSamples();
	// True while a sound effect is playing
	bool osystem_isSamplePlaying();

	// Plays a narrator voice by its file name (loose file or GAME.GOG)
	void osystem_playVO(const char* voFileName);
	// Plays the narrator voice of a book or message by its number
	void osystem_playVocByIndex(int vocIndex);
	// Plays the narrator voice of one page of a book
	void osystem_playVocPage(int vocIndex, int page);
	// Stops the narrator voice
	void osystem_stopVO();

	// VOC pre-decode cache for seamless page turns during book reading
	void osystem_preDecodeVocPage(int vocIndex, int page);
	// Forgets the voices decoded in the background
	void osystem_clearVocCache();
	// The narrators with voices: the folders of Voices in the working folder
	std::vector<std::string> osystem_findNarrators();

	extern float g_voPitchMultiplier;
	extern bool g_playWorldActive;
	extern bool g_3dFlushedThisFrame;
	extern bool g_menuActive;
	//    void getMouseStatus(mouseStatusStruct * mouseData);

	// Makes the texture of a foreground mask from its 320x200 shape
	void osystem_createMask(const std::array<u8, 320 * 200>& mask, int roomId, int maskId, int actualRoomNumber, int maskX1, int maskY1, int maskX2, int maskY2);
	// Draws a foreground mask of the camera over the objects behind it
	void osystem_drawMask(int roomId, int maskId);

	// Opens a frame of the renderer (creates its resources the first time)
	void osystem_startFrame();
	// Empty: the renderer needs nothing at the end of a frame
	void osystem_stopFrame();

	// Sets the drawing clip rectangle (320x200 coordinates)
	void osystem_setClip(float left, float top, float right, float bottom);
	// Removes the drawing clip rectangle
	void osystem_clearClip();


	// Adds a filled polygon of a 3D model (plain, noise, ramp or glass by polyType)
	void osystem_fillPoly(float* buffer, int numPoint, unsigned char color,u8 polyType);
	// Adds a 3D line
	void osystem_draw3dLine(float x1, float y1, float z1, float x2, float y2, float z2, unsigned char color);
	// Adds the four edges of a 3D quad as lines
	void osystem_draw3dQuad(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, unsigned char color, int transparency);
	// Adds a sphere of a 3D model (a disc facing the camera)
	void osystem_drawSphere(float X, float Y, float Z, u8 color, u8 material, float size);
	// Adds a point of a 3D model (a small disc facing the camera)
	void osystem_drawPoint(float X, float Y, float Z, u8 color, u8 material, float size);
	// Sends the polygons, spheres and lines of the frame to the graphics card
	void osystem_flushPendingPrimitives();
	// Draws the text layer (uiLayer) over the scene
	void osystem_drawUILayer();



	// Plays a CD music track (loose file or GAME.GOG)
	int osystem_playTrack(int trackId);
	// Stops the CD music
	void osystem_stopTrack();
	// True while a CD music track (CD, loose file or GAME.GOG) is still playing
	int osystem_isTrackPlaying();
	// Starts the AdLib music output (Jack in the Dark)
	void osystem_playAdlib();

	// Scene preview capture for pause menu
	void osystem_updateSceneSnapshot();
	// Freezes the scene snapshot for the pause menu preview
	void osystem_captureScenePreview();
	// Marks the pause menu preview as ready
	void osystem_finalizeScenePreview();
	// The pixels of the pause menu preview, or nullptr
	unsigned char* osystem_getScenePreviewData();
	// Width of the pause menu preview (0 if none)
	int osystem_getScenePreviewWidth();
	// Height of the pause menu preview (0 if none)
	int osystem_getScenePreviewHeight();

	// Freeze/unfreeze scene snapshot for menu backgrounds
	void osystem_freezeSceneForMenu();
	// The scene snapshot follows the game again (menu closed)
	void osystem_unfreezeSceneForMenu();
	// Draws the frozen scene behind a menu
	void osystem_drawFrozenSceneBackground();

	// Painter's order (Original mode): the objects drawn next use depth slice
	// `slice` (0 = nearest); -1 = plain depth (BUGS.md V21)
	static const int OSYSTEM_DEPTH_SLICES = 128;
	// Window position (pixels) to the 320x200 game screen, through the scene
	// rectangle (graphics.aspect leaves black bars); false before the first frame
	bool osystem_windowToGame(float windowX, float windowY, float* gameX, float* gameY);
	// Rectangle of the window (pixels) where the play area is drawn: x, y, width, height
	void osystem_getSceneRect(int rect[4]);

	// Plain screens (title armadillo, intro screens, books) suspend the screen
	// effects without changing the player's options; calls nest (BUGS.md C15)
	void osystem_suspendScreenEffects(bool suspend);

	// Limits the depth drawn (-1: all), used to draw objects in front of or behind a mask
	void osystem_setDepthSlice(int slice);
	// True if a depth slice was used this frame
	bool osystem_depthSlicedThisFrame();
	// Draws the glass polygons of the frame (Original mode), after the scene
	void osystem_drawDeferredGlass();
	// True if the frozen image was drawn since the last call (screen effects
	// reuse its ambient occlusion)
	bool osystem_takeFrozenSceneDrawn();
	// True while the scene snapshot follows the game (not frozen by a menu)
	bool osystem_isSceneSnapshotLive();

	// Brightness of the final image, 0 (black) to 1 (normal). Fades use it so
	// the whole picture (background, models, masks) changes together.
	bool osystem_canFadeScreen();
	// Sets the screen fade (0 black, 1 normal)
	void osystem_setScreenFade(float level);
	// The cigar smoke's ellipse on the 320x200 screen for this frame: the image
	// turns grey inside it (core/sceneDraw.cpp, AffSmoke)
	void osystem_setSmokeArea(float centerX, float centerY, float radiusX, float radiusY);

	// Number of the 3D object being drawn (1..255, 0 = none): its masks cover
	// only its own pixels. See rendererBGFX.cpp.
	void osystem_setObjectStencil(int objectId);




#endif
