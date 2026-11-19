///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The zoomed aspect ratios (widescreen.h)
///////////////////////////////////////////////////////////////////////////////

#include <bx/timer.h>
#include "common.h"   // after bx: windows.h min/max macros
#include "widescreen.h"
#include "settings.h"
#include "controls.h"
#include "osystem.h"
#include "advancedOptionsText.h"
#include "screenEffects.h"

#include <algorithm>
#include <cmath>

namespace
{

// What the game told: where the character is (osystem_setPlayView, the
// last frame of play) and how many screens over the game are shown
int s_playerTop = -1, s_playerBottom = -1, s_camera = -1;
int s_screensOver = 0;

// The view
bool s_playing = false;        // the last composed frame was a frame of play
bool s_placed = false;         // the view has a position (false: jump to the character)
int s_lastCamera = -1;
float s_top = 0.0f;            // how far down the view is, in screen pixels (0: the top of the scene)
float s_look = 0.0f;           // Look up / down: -1 the top of the scene, 1 the bottom
int64_t s_lastTime = 0;

// Where the view goes to show the character: in the middle band (a third of
// the screen) it does not move; out of it, the view goes with it
float followTop(float current, float playerCentre, float screenHeight)
{
    const float band = screenHeight / 6.0f;
    const float middle = current + screenHeight / 2.0f;
    if (playerCentre > middle + band)
        return playerCentre - band - screenHeight / 2.0f;
    if (playerCentre < middle - band)
        return playerCentre + band - screenHeight / 2.0f;
    return current;
}

} // namespace

void osystem_screenOverPlay(bool shown)
{
    s_screensOver += shown ? 1 : (s_screensOver > 0 ? -1 : 0);
}

void osystem_setPlayView(int playerTop, int playerBottom, int camera)
{
    s_playerTop = playerTop;
    s_playerBottom = playerBottom;
    s_camera = camera;
}

bool widescreenZoomed()
{
    return aspectIsZoomed(g_settings.graphics.aspect);
}

float osystem_textFade()
{
    return (widescreenZoomed() && g_screenEffects) ? g_screenEffects->screenFade() : 1.0f;
}

bool widescreenPlaying()
{
    return s_playing;
}

void widescreenTextBox(const int sceneRect[4], int box[4])
{
    // As tall as the screen, 4:3, in the middle
    box[3] = sceneRect[3];
    box[2] = (std::min)(sceneRect[2], (int)std::lround(sceneRect[3] * 4.0 / 3.0));
    box[0] = sceneRect[0] + (sceneRect[2] - box[2]) / 2;
    box[1] = sceneRect[1];
}

void widescreenSceneQuad(const int sceneRect[4], int quad[4])
{
    const int64_t now = bx::getHPCounter();
    const float seconds = s_lastTime ? (float)((double)(now - s_lastTime) / (double)bx::getHPFrequency()) : 0.0f;
    s_lastTime = now;
    const float step = (std::min)(seconds, 0.1f);

    // Playing: inside the game's loop (Alone 1, Jack) with no screen over it.
    // Its fades count too (closing a menu, the scene fades in before it is
    // drawn again): deciding by the frames drawn showed them 4:3 a moment
    s_playing = widescreenZoomed() && g_playWorldActive && s_screensOver == 0 && (g_gameId == AITD1 || g_gameId == JACK);

    // The scene at the screen's width: taller than the screen
    const float sceneHeight = sceneRect[2] * 3.0f / 4.0f;
    const float range = sceneHeight - sceneRect[3];
    if (!s_playing || range <= 1.0f)
    {
        // Menus, inventory, books, pictures: the whole 4:3 scene in its box
        widescreenTextBox(sceneRect, quad);
        s_placed = false;
        s_look = 0.0f;
        return;
    }

    const bool seen = s_playerTop >= 0 && s_playerBottom >= s_playerTop;
    const float playerCentre = seen ? (s_playerTop + s_playerBottom) * 0.5f * sceneHeight / 200.0f : sceneHeight / 2.0f;
    if (!s_placed || s_camera != s_lastCamera)
    {
        // A new camera (or back from a menu): straight to the character
        s_top = std::clamp(playerCentre - sceneRect[3] / 2.0f, 0.0f, range);
        s_placed = true;
        s_lastCamera = s_camera;
    }
    else if (seen)
    {
        const float wanted = std::clamp(followTop(s_top, playerCentre, (float)sceneRect[3]), 0.0f, range);
        s_top += (wanted - s_top) * (1.0f - std::exp(-step * 4.0f));
    }

    // Look up / down: the view goes toward the top or the bottom of the scene
    // while held, and comes back to the character when released
    const float look = std::clamp(controlsLook(), -1.0f, 1.0f);
    s_look += (look - s_look) * (1.0f - std::exp(-step * 6.0f));
    float top = s_top;
    if (s_look < 0.0f)
        top += (0.0f - s_top) * -s_look;
    else
        top += (range - s_top) * s_look;

    quad[0] = sceneRect[0];
    quad[2] = sceneRect[2];
    quad[3] = (int)std::lround(sceneHeight);
    quad[1] = sceneRect[1] - (int)std::lround(std::clamp(top, 0.0f, range));
}
