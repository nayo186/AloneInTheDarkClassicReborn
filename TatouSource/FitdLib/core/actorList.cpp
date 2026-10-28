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

// Which box is nearer the camera: 1 = a, -1 = b. Boxes apart along an axis:
// the one on the camera's side of the gap is in front (the widest gap
// decides). Boxes that overlap on every axis: the nearer centre.
static int nearerToCamera(const ZVStruct& a, const ZVStruct& b)
{
    const int aMin[3] = { a.ZVX1, a.ZVY1, a.ZVZ1 }, aMax[3] = { a.ZVX2, a.ZVY2, a.ZVZ2 };
    const int bMin[3] = { b.ZVX1, b.ZVY1, b.ZVZ1 }, bMax[3] = { b.ZVX2, b.ZVY2, b.ZVZ2 };
    const int camera[3] = { translateX, translateY, translateZ };

    int bestGap = 0;
    int result = 0;
    for (int axis = 0; axis < 3; axis++)
    {
        int gap, side;   // side: 1 if a is on the low side of the gap
        if (aMax[axis] < bMin[axis])
        {
            gap = bMin[axis] - aMax[axis];
            side = 1;
        }
        else if (bMax[axis] < aMin[axis])
        {
            gap = aMin[axis] - bMax[axis];
            side = -1;
        }
        else
            continue;

        if (gap > bestGap)
        {
            bestGap = gap;
            // Middle of the gap: the camera on a's side means a is in front
            const long long middle2 = (side == 1) ? (long long)aMax[axis] + bMin[axis] : (long long)bMax[axis] + aMin[axis];
            const bool cameraLow = 2LL * camera[axis] < middle2;
            result = (cameraLow == (side == 1)) ? 1 : -1;
        }
    }
    if (result != 0)
        return result;

    return squaredCameraDistance(a) <= squaredCameraDistance(b) ? 1 : -1;
}

// Topological order: every object is drawn after the ones it is in front
// of. Contradictions (cycles) are broken by drawing the farthest first.
static void sortByCamera()
{
    const int count = NbAffObjets;
    if (count < 2)
        return;

    std::vector<ZVStruct> boxes(count);
    std::vector<long long> distance(count);
    for (int i = 0; i < count; i++)
    {
        boxes[i] = roomBox(Index[i]);
        distance[i] = squaredCameraDistance(boxes[i]);
    }

    // drawAfter[i * count + j]: i must be drawn after j (i is in front of j)
    std::vector<char> drawAfter(count * count, 0);
    std::vector<int> pending(count, 0);   // objects that must be drawn before i
    for (int i = 0; i < count; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (nearerToCamera(boxes[i], boxes[j]) > 0)
            {
                drawAfter[i * count + j] = 1;
                pending[i]++;
            }
            else
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
            for (int i = 0; i < count; i++)
                if (!done[i] && (next < 0 || distance[i] > distance[next]))
                    next = i;

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
