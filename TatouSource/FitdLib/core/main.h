///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Core game function declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIN_
#define _MAIN_

extern int AntiRebond;

// Empty: the original waited for the screen refresh between menu frames
void menuWaitVSync();
// Draws sprite number `param` of `gfx` at (x, y), skipping its colour 0 (transparent)
void AffSpfI(int x, int y, int param, char* gfx);
// Width and height of sprite number `index` of `gfx`
void SizeSpf(int index, char* gfx, int* width, int* height);
// The scenery zones the current object stands in: room change, floor change and other zone triggers
void GereDec();
// Runs the script of an object when it is found ("found" life of the world object)
void executeFoundLife(int objIdx);
// The chronometers of the objects run from the hand start again (after a load)
void resetFoundObjectChronos();
// Sets up the view of the current camera: projection, angles, position, visible rooms and the objects to draw
void InitView();
// Switches camera when the followed object enters another camera's zone
void GereSwitchCamera(void);
// Builds the list of active objects (those in the current room or the rooms seen from the camera)
void GenereActiveList();
// Builds the list of objects to draw (NbAffObjets, Index) and puts it in drawing order
void GenereAffList();
// Redraws the whole screen: background, objects, masks and texts
void AllRedraw(int mode);
// Finds the objects whose collision box touches zvPtr (up to 3); returns how many
int CheckObjectCol(int actorIdx, ZVStruct* zvPtr);
// Moves a box along a line from (X, Y, Z) at angle beta and returns the first object it hits (-1 none, -2 a wall)
int checkLineProjectionWithActors( int actorIdx, int X, int Y, int Z, int beta, int room, int param );
// Counts the fixed collision boxes of the room that touch zvPtr (fills hardColTable)
int AsmCheckListCol(ZVStruct* zvPtr, roomDataStruct* pRoomData);
// Takes an object out of the inventory
void DeleteInventoryObjet(int objIdx);
// Turns a step of animation (animMoveX/Z) by the object's angles
void walkStep(int angle1, int angle2, int angle3);
// Places a thrown object where it stopped, on the floor near (x, z)
void throwStoppedAt(int x, int z);
// The scenery zone (sceZone) at (x, y, z) in the room, or nullptr
sceZoneStruct* processActor2Sub(int x, int y, int z, roomDataStruct* pRoomData);
// Loads the world objects (OBJETS.ITD): every object of the game with its starting state
void LoadWorld(void);
// Resets the game variables for a new game (inventory, flags, game over...)
void initVars();

// Clears the 320x200 screen and the text layer
void flushScreen(void);
// Save and load screen: lists the slots with their date and returns the chosen one (-1 none)
int parseAllSaves(int arg);
// The text entry number `index` of the language's texts
textEntryStruct* getTextFromIdx(int index);
// Closes everything (controller, sound, music, graphics) and ends the program
void cleanupAndExit(void);
// Debug: draws a 3D box as six projected faces
void drawProjectedBox(int x1,int x2,int y1,int y2,int z1,int z2, int color, int transparency);

// Plays an animated sequence (cinematic) with its fades
void playSequence(int sequenceIdx, int fadeStart, int fadeOutVar);
// AITD1 ending: the burning tree of ENDSEQ.PAK, then the screen turns red (END_SEQUENCE)
void playEndSequence();

// True if the file exists (path as given)
bool fileExists(const char* name);
bool dataFileExists(const char* name);   // in the game data folder (homePath)

// Program start of the original: screen, resources, texts, palette
void OpenProgram(void);
// Frees the game's resource lists before exiting
void freeAll(void);
// Starts a game on the given floor and room: loads the world, the floor and runs the main loop
void startGame(int startupEtage, int startupRoom, int allowSystemMenu);
void AffRect(int x1, int y1, int x2, int y2, char color); // fast recode. No RE
// Copies a collision box
void CopyZV(ZVStruct* source, ZVStruct* dest);
// Moves a collision box from the coordinates of one room to those of another
void AdjustZV(ZVStruct* zvPtr, int startRoom, int destRoom);
// Puts a world object in the inventory (as the script instruction TAKE)
void take(int objIdx);
// An object was found: shows it and asks the player to take it or not
void FoundObjet(int objIdx, int param);
// Starts a hit: plays the animation and sets the frame, range and force of the blow
void hit(int animNumber,int arg_2,int arg_4,int arg_6,int hitForce,int arg_A);
// True if two collision boxes overlap
int CubeIntersect(ZVStruct* zvPtr1,ZVStruct* zvPtr2);
// Puts an object at another object's position (room, floor and coordinates)
void PutAtObjet(int objIdx, int objIdxToPutAt);
// Sets the drawing clip rectangle on the 320x200 screen
void SetClip(int left, int top, int right, int bottom);

// Debug: draws a collision box as projected lines
void drawZv(const ZVStruct& localZv);
// Debug builds (FITD_DEBUGGER): an object's box and a room's zones (debugDraw.cpp)
void drawZv(tObject* actorPtr);
// Debug: draws the scenery zones of a room
void drawSceZone(int roomNumber);
// Debug: draws the fixed collision boxes of a room
void drawHardCol(int roomNumber);

///
// Opens a book or letter: picture, pages and narrator voice
void readBook(int index, int type, int vocIndex = -1);
// The reader of books and documents: shows the pages of text `index` in the rectangle and turns them
int Lire(int index, int left, int top, int right, int bottom, int mode, int color, int shadow, int vocIndex = -1);

// Sets the camera projection: screen centre, perspective and field of view
void SetProjection(int centerX, int centerY, int x, int y, int z);
// Sets the camera angles (1024 steps per turn)
void SetAngleCamera(int x, int y, int z);
// Sets the camera position
void SetPosCamera(int x, int y, int z);
// Distance between two points on the floor plane (x, z)
s16 GiveDistance2D(int x1, int z1, int x2, int z2);
// Sets how the current object moves: still, controlled by the player, following an object or a track
void InitDeplacement(int trackMode, int trackNumber);
// Starts a value that goes from beta to newBeta in `param` ticks (rotations, speeds)
void InitRealValue(s16 beta, s16 newBeta, s16 param, RealValue* rotatePtr);
// The current value of an interpolated value (InitRealValue)
s16 updateActorRotation(RealValue* rotatePtr);
// Removes a world object from the room (and from the screen)
void deleteObject(int objIdx);
// Takes an object out of the background (it is drawn as a moving object again)
void removeFromBGIncrust(int actorIdx);
// Shows message number messageIdx at the bottom of the screen
void makeMessage(int messageIdx);
// Draws the messages at the bottom of the screen and keeps them for their time
int drawTextOverlay(void);
// Corrects a move so the object does not go into a fixed collision box (slides along it)
void GereCollision(ZVStruct* startZv, ZVStruct* zvPtr2, ZVStruct* zvPtr3);

// Pastes an object into the background (drawn once, not every frame)
void addActorToBgInscrust(int actorIdx);

// Sets the angles for pointRotate
void setupPointRotate(int alpha, int beta, int gamma);
// Turns a point by the angles of setupPointRotate
void pointRotate(int x, int y, int z, int* destX, int* destY, int* destZ);

// Shared between the files main.cpp was split into
extern "C" {
    extern char homePath[512];
}
// Opens a file of the game data folder (homePath)
FILE* Open(const char* filename, const char* mode);
extern const unsigned char defaultPalette[0x30];
extern const unsigned char defaultPaletteAITD3[0x30];
// True if the room is seen from the current camera
int isInViewList(int value);
// Loads the camera picture (CAMERAnn.PAK) into the background
void loadCamera(int cameraIdx);
// Loads the foreground masks of a camera (Jack and later: MASKnn.PAK)
void loadMask(int cameraIdx);
// Builds the foreground masks of the current camera (AITD1: polygons of the camera data)
void createAITD1Mask();
// Builds the list of rooms seen from the current camera
void InitViewedRoomList();
// Removes an active object from the list of objects
void DeleteObjet(int index);
// Loads the game palette (ITD_RESS)
void loadPalette(void);
// True if a box on the floor (X1..X2, Z1..Z2) is inside one of the mask zones: the mask must be drawn over the object
int isBgOverlayRequired(int X1, int X2, int Z1, int Z2, char* data, int param);

#endif

