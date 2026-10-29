///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Modern left stick (see modernStick.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "controls.h"
#include "modernStick.h"

#include <cmath>
#include <cstdlib>

// Angles of the engine: 1024 = a full turn
static const int kTurnPerTick = 24;              // half a turn in about a third of a second
static const int kTurnOnSpotStanding = 0x80;     // further than 45 degrees: turn before walking
static const int kTurnOnSpotWalking = 0x180;     // walking: only a sharp turn back (135 degrees) stops it
static const int kProbeDistance = 200;           // the step projected to find the stick's direction
static const float kReleaseHold = 0.785f;        // radians (45 degrees) the stick must move to drop the kept direction

struct ModernStickState
{
    bool steering = false;     // the stick steers the player this frame
    int actor = -1;            // the player (ListObjets index)
    int target = 0;            // the angle to face
    bool walking = false;      // walking forward (else turning on the spot)
    bool hadStick = false;     // the stick was steering the frame before
    int cameraKey = -1;        // floor, room and camera of the last frame
    bool holding = false;      // keeping the direction of the camera before the cut
    float heldAngle = 0.0f;    // stick angle when the camera cut
    unsigned int lastTurn = 0; // game clock of the last turn
};
static ModernStickState s_stick;

// The shortest signed turn from one angle to another (-512..511)
static int angleDiff(int to, int from)
{
    int diff = (to - from) & 0x3FF;
    if (diff >= 0x200)
        diff -= 0x400;
    return diff;
}

static float radiansDiff(float a, float b)
{
    float diff = std::fmod(a - b, 6.2831853f);
    if (diff > 3.1415927f)
        diff -= 6.2831853f;
    if (diff < -3.1415927f)
        diff += 6.2831853f;
    return diff;
}

// A point of the room on the 320x200 screen, through the current camera (as
// debugDraw.cpp draws its lines)
static bool projectToScreen(float x, float y, float z, float& screenX, float& screenY)
{
    x -= translateX;
    y -= translateY;
    z -= translateZ;
    transformPoint(&x, &y, &z);
    z += cameraPerspective;
    if (z <= 1.0f)
        return false;
    screenX = x * cameraFovX / z + cameraCenterX;
    screenY = y * cameraFovY / z + cameraCenterY;
    return true;
}

// How well a +Z step at this angle matches the stick on screen (1 = the same
// way): the step as walkStep makes it, projected with the camera
static bool screenMatch(const tObject& actor, int beta, float stickX, float stickY, float& match)
{
    const float x = (float)(actor.worldX + actor.stepX);
    const float y = (float)(actor.worldY + actor.stepY);
    const float z = (float)(actor.worldZ + actor.stepZ);
    int moveX = 0;
    int moveZ = 0;
    Rotate(beta & 0x3FF, 0, kProbeDistance, &moveZ, &moveX);
    float fromX, fromY, toX, toY;
    if (!projectToScreen(x, y, z, fromX, fromY) || !projectToScreen(x + moveX, y, z + moveZ, toX, toY))
        return false;
    const float dx = toX - fromX;
    const float dy = toY - fromY;
    const float length = std::sqrt(dx * dx + dy * dy) * std::sqrt(stickX * stickX + stickY * stickY);
    if (length < 0.0001f)
        return false;
    match = (dx * stickX + dy * stickY) / length;
    return true;
}

// The angle whose step goes the stick's way on screen: every 16th angle,
// then each one around the best. Projecting the real step works with any
// camera and keeps to the engine's own angles and axes.
static bool angleForStick(const tObject& actor, float stickX, float stickY, int& beta)
{
    int best = -1;
    float bestMatch = -2.0f;
    for (int candidate = 0; candidate < 0x400; candidate += 16)
    {
        float match;
        if (screenMatch(actor, candidate, stickX, stickY, match) && match > bestMatch)
        {
            bestMatch = match;
            best = candidate;
        }
    }
    if (best < 0)
        return false;
    const int coarse = best;
    for (int candidate = coarse - 15; candidate <= coarse + 15; candidate++)
    {
        float match;
        if (screenMatch(actor, candidate, stickX, stickY, match) && match > bestMatch)
        {
            bestMatch = match;
            best = candidate;
        }
    }
    // The walk animations step towards -Z (walkStep with a negative step):
    // facing forward is half a turn from the probed +Z step
    beta = (best + 0x200) & 0x3FF;
    return true;
}

void updateModernStick()
{
    ModernStickState& s = s_stick;
    s.steering = false;
    const int cameraKey = (g_currentFloor * 256 + currentRoom) * 64 + NumCamera;
    const bool cameraChanged = (cameraKey != s.cameraKey);
    s.cameraKey = cameraKey;

    // Only the stick moving the player under manual control; holding Action
    // (fighting) or another direction gives the stick's tank directions
    const ControlsLeftStick& stick = controlsLeftStick();
    const int player = currentCameraTargetActor;
    const bool canSteer = stick.active && stick.otherDirections == 0 && !localClick &&
        player >= 0 && player < NUM_MAX_OBJECT && ListObjets[player].indexInWorld >= 0 &&
        ListObjets[player].trackMode == 1;
    if (!canSteer)
    {
        s.hadStick = false;
        s.holding = false;
        s.walking = false;
        s.lastTurn = timer;
        return;
    }
    const tObject& actor = ListObjets[player];

    // Through a camera cut the direction is kept until the stick moves
    const float stickAngle = std::atan2(stick.y, stick.x);
    if (cameraChanged && s.hadStick)
    {
        s.holding = true;
        s.heldAngle = stickAngle;
    }
    if (s.holding && std::fabs(radiansDiff(stickAngle, s.heldAngle)) > kReleaseHold)
        s.holding = false;
    if (!s.holding)
    {
        int beta;
        if (angleForStick(actor, stick.x, stick.y, beta))
            s.target = beta;
        else if (!s.hadStick)
            return;   // no direction yet: leave the tank directions
    }
    s.hadStick = true;

    // The scripts choose the animations: forward to walk, a side to turn on
    // the spot first when the stick points far from where the player faces
    const int diff = angleDiff(s.target, actor.beta);
    const int turnOnSpot = s.walking ? kTurnOnSpotWalking : kTurnOnSpotStanding;
    localJoyD &= ~0x0F;
    if (std::abs(diff) > turnOnSpot)
    {
        localJoyD |= (diff > 0) ? 4 : 8;   // left turns to bigger angles (GereManualRot)
        s.walking = false;
    }
    else
    {
        localJoyD |= 1;
        s.walking = true;
    }
    s.steering = true;
    s.actor = player;
}

bool modernStickTurn()
{
    ModernStickState& s = s_stick;
    if (!s.steering || currentProcessedActorIdx != s.actor)
        return false;

    // Turns by the time passed, not by the frames (any frame rate)
    unsigned int ticks = timer - s.lastTurn;
    if (ticks > 6)
        ticks = 6;
    s.lastTurn = timer;

    tObject* actor = currentProcessedActorPtr;
    const int diff = angleDiff(s.target, actor->beta);
    const int step = kTurnPerTick * (int)ticks;
    if (std::abs(diff) <= step)
    {
        actor->beta = s.target;
        actor->direction = 0;
    }
    else
    {
        actor->beta = (actor->beta + ((diff > 0) ? step : -step)) & 0x3FF;
        actor->direction = (diff > 0) ? 1 : -1;
    }
    actor->rotate.numSteps = 0;
    return true;
}
