///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Visual fixes of 3D objects (see objectFixes.h)
///////////////////////////////////////////////////////////////////////////////

#include "objectFixes.h"
#include "common.h"
#include "renderer.h"
#include "consoleLog.h"
#include "coreHooks.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>

// The fixes compiled in (objectFixesData.cpp, made by embedObjectFixes.cmake)
extern const char* const kObjectFixes_aitd1;
extern const char* const kObjectFixes_jack;

static std::mutex s_mutex;
static std::vector<ObjectFix> s_fixes;
static SeenObject s_seen[NUM_MAX_OBJECT];
static std::atomic_int s_blinking(-1);

std::mutex& objectFixesMutex() { return s_mutex; }
std::vector<ObjectFix>& objectFixes() { return s_fixes; }
const SeenObject* objectFixesSeen() { return s_seen; }
void objectFixesSetBlinking(int listIndex) { s_blinking.store(listIndex); }

const char* objectFixesFileName()
{
    return (g_gameId == JACK) ? "jack.cfg" : "aitd1.cfg";
}

bool objectFixIsEmpty(const ObjectFix& fix)
{
    for (int axis = 0; axis < 3; axis++)
        if (fix.offset[axis] || fix.rotation[axis] || fix.scale[axis] != 100 || fix.grow[axis])
            return false;
    return true;
}

ObjectFix* findObjectFix(int floor, int world, int room, int body)
{
    for (ObjectFix& fix : s_fixes)
    {
        if (fix.floor != floor)
            continue;
        if ((world >= 0) ? fix.world == world : (fix.world < 0 && fix.room == room && fix.body == body))
            return &fix;
    }
    return nullptr;
}

// ── The text form: one line per object ─────────────────────────────────────
// floor world room body  dx dy dz  dalpha dbeta dgamma  sizeX sizeY sizeZ (%)  growX growY growZ
static void parseFixes(const char* text)
{
    std::string line;
    for (const char* c = text; ; c++)
    {
        if (*c && *c != '\n')
        {
            line += *c;
            continue;
        }
        ObjectFix fix;
        // The sizes and growth may be missing (lines saved before they existed)
        if (!line.empty() && line[0] != '#' &&
            sscanf(line.c_str(), "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &fix.floor, &fix.world, &fix.room, &fix.body,
                &fix.offset[0], &fix.offset[1], &fix.offset[2], &fix.rotation[0], &fix.rotation[1], &fix.rotation[2],
                &fix.scale[0], &fix.scale[1], &fix.scale[2], &fix.grow[0], &fix.grow[1], &fix.grow[2]) >= 10)
            s_fixes.push_back(fix);
        line.clear();
        if (!*c)
            break;
    }
}

bool objectFixesSave()
{
#if CLASSIC_REBORN_DEV_TOOLS
    // Into the project, where the next build takes them from
    const std::string path = std::string(OBJECT_FIXES_SOURCE_DIR) + "/" + objectFixesFileName();
    FILE* file = fopen(path.c_str(), "w");
    if (!file)
    {
        printf(MAIN_WARN "Object fixes: cannot write %s" CON_RESET "\n", path.c_str());
        return false;
    }
    fprintf(file, "# Visual fixes of 3D objects (object editor, F10 in debug mode). Compiled into Tatou.exe.\n");
    fprintf(file, "# floor world room body  dx dy dz  dalpha dbeta dgamma (1024 = a full turn)  sizeX sizeY sizeZ (%%)  growX growY growZ (0 both, 1 +, -1 -)\n");
    int count = 0;
    for (const ObjectFix& fix : s_fixes)
    {
        if (objectFixIsEmpty(fix))
            continue;
        fprintf(file, "%d %d %d %d  %d %d %d  %d %d %d  %d %d %d  %d %d %d\n", fix.floor, fix.world, fix.room, fix.body,
            fix.offset[0], fix.offset[1], fix.offset[2], fix.rotation[0], fix.rotation[1], fix.rotation[2],
            fix.scale[0], fix.scale[1], fix.scale[2], fix.grow[0], fix.grow[1], fix.grow[2]);
        count++;
    }
    fclose(file);
    printf(MAIN_OK "Object fixes: %d saved in %s\n", count, path.c_str());
    return true;
#else
    return false;   // players' builds have no editor
#endif
}

// ── Draw hook (game thread, core/sceneDraw.cpp) ────────────────────────────
static bool onObjectDraw(int listIndex, int* x, int* y, int* z, int* alpha, int* beta, int* gamma, ObjectSize* size)
{
    if (listIndex < 0 || listIndex >= NUM_MAX_OBJECT)
        return true;
    const tObject& object = ListObjets[listIndex];
    std::lock_guard<std::mutex> lock(s_mutex);

    SeenObject& seen = s_seen[listIndex];
    seen.lastSeen = SDL_GetTicks();
    seen.world = object.indexInWorld;
    seen.room = object.room;
    seen.body = object.bodyNum;
    seen.floor = g_currentFloor;

    if (const ObjectFix* fix = findObjectFix(g_currentFloor, object.indexInWorld, object.room, object.bodyNum))
    {
        *x += fix->offset[0];
        *y += fix->offset[1];
        *z += fix->offset[2];
        *alpha = (*alpha + fix->rotation[0]) & 1023;
        *beta = (*beta + fix->rotation[1]) & 1023;
        *gamma = (*gamma + fix->rotation[2]) & 1023;
        for (int axis = 0; axis < 3; axis++)
        {
            size->scale[axis] = fix->scale[axis] / 100.0f;
            size->grow[axis] = fix->grow[axis];
        }
    }

    // The object chosen in the editor blinks, to be found
    if (listIndex == s_blinking.load())
        return (SDL_GetTicks() / 250) % 2 == 0;
    return true;
}

void objectFixesRegister()
{
    g_onObjectDraw = onObjectDraw;
    parseFixes((g_gameId == JACK) ? kObjectFixes_jack : kObjectFixes_aitd1);
    printf(MAIN_OK "Object fixes: %d for this game\n", (int)s_fixes.size());
}
