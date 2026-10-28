///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Fonts of the in-game menus (see menuFonts.h)
///////////////////////////////////////////////////////////////////////////////

#include "menuFonts.h"
#include "consoleLog.h"

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

static ImFont* addFont(const std::string& path, float size)
{
    std::error_code error;
    if (path.empty() || !std::filesystem::exists(std::filesystem::u8path(path), error))
        return nullptr;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size);
}

void menuFontsLoad()
{
    // The F1 window is drawn for a 900-pixel-high screen (advancedOptions.cpp):
    // the largest it can get is the whole desktop, or the screen's scaling
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
    const float contentScale = SDL_GetDisplayContentScale(display);
    s_scale = (std::max)({ 1.0f, mode ? mode->h / 900.0f : 1.0f, contentScale > 0.0f ? contentScale : 1.0f });

    std::string dir;
#ifdef _WIN32
    char windir[MAX_PATH] = {};
    if (GetWindowsDirectoryA(windir, MAX_PATH) > 0)
        dir = std::string(windir) + "\\Fonts\\";
#endif
    s_fonts.text = addFont(dir.empty() ? "" : dir + "georgia.ttf", 17.0f * s_scale);
    s_fonts.button = addFont(dir.empty() ? "" : dir + "georgiab.ttf", 21.0f * s_scale);
    s_fonts.header = addFont(dir.empty() ? "" : dir + "georgia.ttf", 30.0f * s_scale);
    s_fonts.title = s_fonts.header;
    if (!s_fonts.text)
    {
        // No Georgia (another system): ImGui's own font at the same scale
        ImFontConfig config;
        config.SizePixels = 15.0f * s_scale;
        s_fonts.text = ImGui::GetIO().Fonts->AddFontDefault(&config);
        printf(MAIN_WARN "Menu fonts: Georgia not found, ImGui's font used" CON_RESET "\n");
    }
    if (!s_fonts.button) s_fonts.button = s_fonts.text;
    if (!s_fonts.header) s_fonts.header = s_fonts.text;
    if (!s_fonts.title) s_fonts.title = s_fonts.header;
    imguiRebuildFontTexture();
    printf(MAIN_OK "Menu fonts made at scale %.2f\n", s_scale);
}
