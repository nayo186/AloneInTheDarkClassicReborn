///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The graphics engines a player can choose (graphics.renderer in
// aitd_remaster.cfg; the launcher's Advanced options and F1), each by its own
// name, the system's default first (Direct3D 11 on Windows, Vulkan on Linux
// and Android): to try another when the picture looks wrong (BUGS.md C78).
// One that does not start on a computer is passed over by bgfx for another.
// macOS has only Metal and Linux only Vulkan: no choice. OpenGL is not
// offered on a computer: bgfx opens OpenGL 2.1 there, and the picture comes
// out with black boxes and wrong colours.
// Header-only, for the game and the launcher.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>

namespace renderer {

struct Choice
{
    const char* id;     // the value of graphics.renderer
    const char* name;   // shown as it is
};

// The first is the one the game uses by default ("auto")
#if defined(__APPLE__)
inline constexpr Choice kChoices[] = { { "metal", "Metal" } };
#elif defined(_WIN32)
inline constexpr Choice kChoices[] = { { "d3d11", "Direct3D 11" }, { "vulkan", "Vulkan" } };
#elif defined(__ANDROID__)
inline constexpr Choice kChoices[] = { { "vulkan", "Vulkan" }, { "opengles", "OpenGL ES" } };
#else
inline constexpr Choice kChoices[] = { { "vulkan", "Vulkan" } };
#endif
inline constexpr int kChoiceCount = (int)(sizeof(kChoices) / sizeof(kChoices[0]));

// The choice of a graphics.renderer value; 0 (the default) for "auto" or
// one that is not of this system's
inline int choiceIndex(const char* id)
{
    for (int i = 0; i < kChoiceCount; i++)
        if (id && strcmp(id, kChoices[i].id) == 0)
            return i;
    return 0;
}

// Whether the system lets the player choose (more than one)
inline bool canChoose() { return kChoiceCount > 1; }

} // namespace renderer
