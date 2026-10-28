///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Visual fixes of 3D objects: an object drawn moved, turned or resized where
// the original data leaves a gap or a wrong shape. Only how it is drawn
// changes; collisions and scripts keep the real position. The fixes are part
// of the project (assets/objectFixes/aitd1.cfg and jack.cfg), compiled into
// Tatou.exe (embedObjectFixes.cmake), so every player gets them. They are
// made with the object editor (remaster/ui/objectEditor.cpp), which only
// exists in development builds.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <mutex>
#include <vector>

// The fix of one object of one floor. A world object (it is in the game's
// object list) is found by its index; anything else by room and body.
struct ObjectFix
{
    int floor = 0;
    int world = -1;
    int room = 0;
    int body = 0;
    int offset[3] = {};                 // world units, added to where it is drawn
    int rotation[3] = {};               // angles, 1024 = a full turn
    int scale[3] = { 100, 100, 100 };   // size along its own axes, % (100 = as modelled)
    int grow[3] = {};                   // 0 both sides, +1 only towards +, -1 only towards - (ObjectSize)
};

// An object drawn lately, as the draw hook saw it (game thread)
struct SeenObject
{
    unsigned long long lastSeen = 0;    // SDL_GetTicks, 0 = never
    int world = -1;
    int room = 0;
    int body = 0;
    int floor = 0;
};

// Loads the fixes of the running game and registers the draw hook
// (remasterHooks.cpp)
void objectFixesRegister();

// ── For the object editor (development builds) ─────────────────────────────

// Guards everything below (the draw hook runs on the game thread)
std::mutex& objectFixesMutex();
// Every visual fix of the current game
std::vector<ObjectFix>& objectFixes();
// True if a fix changes nothing
bool objectFixIsEmpty(const ObjectFix& fix);
// The fix of an object (by floor and world index, or floor, room and body), or nullptr
ObjectFix* findObjectFix(int floor, int world, int room, int body);

// The objects drawn lately, by their place in ListObjets (NUM_MAX_OBJECT)
const SeenObject* objectFixesSeen();

// The object (place in ListObjets) that blinks to be found, -1 none
void objectFixesSetBlinking(int listIndex);

// Writes the fixes into the project's file for this game (the next build
// compiles them in); returns false if it cannot
bool objectFixesSave();
// Name of the fixes file of the current game (aitd1.cfg or jack.cfg)
const char* objectFixesFileName();
