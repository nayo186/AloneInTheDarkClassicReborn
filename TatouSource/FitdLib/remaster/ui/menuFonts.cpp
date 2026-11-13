///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Fonts of the in-game menus (see menuFonts.h)
///////////////////////////////////////////////////////////////////////////////

#include "menuFonts.h"
#include "consoleLog.h"
#include "ui/menuFontData.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

extern void imguiRebuildFontTexture();

static menu::Fonts s_fonts;
static float s_scale = 1.0f;

const menu::Fonts& menuFonts() { return s_fonts; }
float menuFontsScale() { return s_scale; }


void menuFontsLoad()
{
    // The F1 window is drawn for a 900-pixel-high screen (advancedOptions.cpp):
    // the largest it can get is the whole desktop, or the screen's scaling
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
    const float contentScale = SDL_GetDisplayContentScale(display);
    s_scale = (std::max)({ 1.0f, mode ? mode->h / 900.0f : 1.0f, contentScale > 0.0f ? contentScale : 1.0f });

    // Inter, the launcher's font (compiled in, the same on every system)
    s_fonts.text = menu::addModernFont(false, (15.0f + menu::kFontBump) * s_scale);
    s_fonts.smallText = menu::addModernFont(false, (13.0f + menu::kFontBump) * s_scale);
    s_fonts.button = menu::addModernFont(true, (16.0f + menu::kFontBump) * s_scale);
    s_fonts.header = menu::addModernFont(true, (20.0f + menu::kFontBump) * s_scale);
    s_fonts.title = s_fonts.header;
    printf(MAIN_OK "Menu fonts (Inter) made at scale %.2f\n", s_scale);
    imguiRebuildFontTexture();
}
