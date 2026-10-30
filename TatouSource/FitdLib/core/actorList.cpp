///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Actor list sorting for depth-correct rendering order
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "actorList.h"

#include <algorithm>
#include <vector>


// ── Camera-based drawing order (BUGS.md V22) ───────────────────────────────

// An object's box in the current room's coordinates
static ZVStruct roomBox(int actorIdx)
{
    tObject& actor = ListObjets[actorIdx];
    ZVStruct box;
    CopyZV(&actor.zv, &box);
    if (actor.room != currentRoom)
        AdjustZV(&box, actor.room, currentRoom);
    return box;
}

static long long squaredCameraDistance(const ZVStruct& box)
{
    const long long dx = (box.ZVX1 + box.ZVX2) / 2 - translateX;
    const long long dy = (box.ZVY1 + box.ZVY2) / 2 - translateY;
    const long long dz = (box.ZVZ1 + box.ZVZ2) / 2 - translateZ;
    return dx * dx + dy * dy + dz * dz;
}

// Which box is nearer the camera: 1 = a, -1 = b, 0 = neither can hide the
// other (any order). Boxes apart along an axis: the camera on a's side of the
// gap means b cannot hide a. If two axes disagree, neither hides the other:
// ordering them anyway made cycles (a door beside the player "in front" of
// them, the other leaf behind) that drew a door over the player (BUGS.md C44).
// Boxes that overlap on every axis: see below.
static int nearerToCamera(const ZVStruct& a, const ZVStruct& b)
{
    const int aMin[3] = { a.ZVX1, a.ZVY1, a.ZVZ1 }, aMax[3] = { a.ZVX2, a.ZVY2, a.ZVZ2 };
    const int bMin[3] = { b.ZVX1, b.ZVY1, b.ZVZ1 }, bMax[3] = { b.ZVX2, b.ZVY2, b.ZVZ2 };
    const int camera[3] = { translateX, translateY, translateZ };

    bool aInFront = false;
    bool bInFront = false;
    for (int axis = 0; axis < 3; axis++)
    {
        int side;   // 1 if a is on the low side of the gap
        if (aMax[axis] < bMin[axis])
            side = 1;
        else if (bMax[axis] < aMin[axis])
            side = -1;
        else
            continue;

        // Middle of the gap: the camera on a's side means a is in front
        const long long middle2 = (side == 1) ? (long long)aMax[axis] + bMin[axis] : (long long)bMax[axis] + aMin[axis];
        const bool cameraLow = 2LL * camera[axis] < middle2;
        if (cameraLow == (side == 1))
            aInFront = true;
        else
            bInFront = true;
    }
    if (aInFront != bInFront)
        return aInFront ? 1 : -1;
    if (aInFront && bInFront)
        return 0;

    // Boxes that overlap on every axis (the player touching a door): along
    // the axis where they overlap least (a door's thickness), the one whose
    // centre is on the camera's side is in front. The nearer centre alone put
    // a big door in front of the player standing right before it (BUGS.md C44)
    int leastAxis = -1;
    int leastOverlap = 0;
    for (int axis = 0; axis < 3; axis++)
    {
        const int overlap = (aMax[axis] < bMax[axis] ? aMax[axis] : bMax[axis]) - (aMin[axis] > bMin[axis] ? aMin[axis] : bMin[axis]);
        if (leastAxis < 0 || overlap < leastOverlap)
        {
            leastAxis = axis;
            leastOverlap = overlap;
        }
    }
    const long long aCentre2 = (long long)aMin[leastAxis] + aMax[leastAxis];
    const long long bCentre2 = (long long)bMin[leastAxis] + bMax[leastAxis];
    const long long camera2 = 2LL * camera[leastAxis];
    const long long aGap = aCentre2 > camera2 ? aCentre2 - camera2 : camera2 - aCentre2;
    const long long bGap = bCentre2 > camera2 ? bCentre2 - camera2 : camera2 - bCentre2;
    if (aGap != bGap)
        return aGap < bGap ? 1 : -1;

    return squaredCameraDistance(a) <= squaredCameraDistance(b) ? 1 : -1;
}

// Rectangle of a box on the 320x200 screen (its 8 corners projected). A box
// partly behind the camera covers the whole screen, to be safe
struct ScreenRect
{
    float left, top, right, bottom;
};

static ScreenRect screenRect(const ZVStruct& box)
{
    ScreenRect rect = { 32000.f, 32000.f, -32000.f, -32000.f };
    for (int corner = 0; corner < 8; corner++)
    {
        float x, y;
        if (!projectToScreen((float)((corner & 1) ? box.ZVX2 : box.ZVX1), (float)((corner & 2) ? box.ZVY2 : box.ZVY1),
                (float)((corner & 4) ? box.ZVZ2 : box.ZVZ1), &x, &y))
            return { -32000.f, -32000.f, 32000.f, 32000.f };
        rect.left = (std::min)(rect.left, x);
        rect.right = (std::max)(rect.right, x);
        rect.top = (std::min)(rect.top, y);
        rect.bottom = (std::max)(rect.bottom, y);
    }
    return rect;
}

static bool rectsOverlap(const ScreenRect& a, const ScreenRect& b)
{
    // A margin: a model can stick out of its box a little
    const float margin = 4.0f;
    return a.left <= b.right + margin && b.left <= a.right + margin && a.top <= b.bottom + margin && b.top <= a.bottom + margin;
}

// Topological order: every object is drawn after the ones it is in front
// of. Only objects that meet on screen are ordered: the rule for boxes says
// which one the other cannot hide, and for objects apart on screen that made
// false cycles (the camera between them: BUGS.md C44). Contradictions left
// (cycles) are broken by drawing the farthest first.
static void sortByCamera()
{
    const int count = NbAffObjets;
    if (count < 2)
        return;

    std::vector<ZVStruct> boxes(count);
    std::vector<long long> distance(count);
    std::vector<ScreenRect> rects(count);
    for (int i = 0; i < count; i++)
    {
        boxes[i] = roomBox(Index[i]);
        distance[i] = squaredCameraDistance(boxes[i]);
        rects[i] = screenRect(boxes[i]);
    }

    // drawAfter[i * count + j]: i must be drawn after j (i is in front of j)
    std::vector<char> drawAfter(count * count, 0);
    std::vector<int> pending(count, 0);   // objects that must be drawn before i
    for (int i = 0; i < count; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (!rectsOverlap(rects[i], rects[j]))
                continue;
            const int nearer = nearerToCamera(boxes[i], boxes[j]);
            if (nearer > 0)
            {
                drawAfter[i * count + j] = 1;
                pending[i]++;
            }
            else if (nearer < 0)
            {
                drawAfter[j * count + i] = 1;
                pending[j]++;
            }
        }
    }

    std::vector<int> order;
    std::vector<char> done(count, 0);
    order.reserve(count);
    while ((int)order.size() < count)
    {
        // Among the objects with nothing left behind them, the farthest
        int next = -1;
        for (int i = 0; i < count; i++)
            if (!done[i] && pending[i] == 0 && (next < 0 || distance[i] > distance[next]))
                next = i;
        if (next < 0)   // contradiction: the farthest remaining object
        {
            for (int i = 0; i < count; i++)
                if (!done[i] && (next < 0 || distance[i] > distance[next]))
                    next = i;
            // Always in tatou.log, at most once a second: an object may be
            // drawn over another in front of it there (tools/readLog.py)
            {
                static unsigned int s_lastOrderLog = 0;
                if (timer - s_lastOrderLog >= 60)
                {
                    s_lastOrderLog = timer;
                    printf("[ORDER] contradiction: floor %d room %d camera %d at %d,%d,%d\n", g_currentFloor, currentRoom, NumCamera,
                        (int)translateX, (int)translateY, (int)translateZ);
                    for (int i = 0; i < count; i++)
                    {
                        if (done[i])
                            continue;
                        const ZVStruct& b = boxes[i];
                        printf("[ORDER]   actor %d world %d room %d box X %d..%d Y %d..%d Z %d..%d\n", Index[i],
                            ListObjets[Index[i]].indexInWorld, ListObjets[Index[i]].room,
                            b.ZVX1, b.ZVX2, b.ZVY1, b.ZVY2, b.ZVZ1, b.ZVZ2);
                    }
                }
            }
        }

        done[next] = 1;
        order.push_back(next);
        for (int i = 0; i < count; i++)
            if (!done[i] && drawAfter[i * count + next])
                pending[i]--;
    }

    std::vector<int> sorted(count);
    for (int k = 0; k < count; k++)
        sorted[k] = Index[order[k]];
    for (int k = 0; k < count; k++)
        Index[k] = sorted[k];
}

void sortActorList()
{
    sortByCamera();
}
