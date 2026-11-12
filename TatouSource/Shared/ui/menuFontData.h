///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The menus' own font: Gelasio (SIL Open Font License, Shared/assets/fonts/
// OFL.txt), made to the measures of Georgia. The menus use Georgia where the
// system has it (Windows) and Gelasio elsewhere (Linux). Compiled into each
// program by Shared/embedFonts.cmake (menuFontData.cpp in its build folder).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <imgui.h>

extern const unsigned char kMenuFont_regular[];
extern const size_t kMenuFont_regular_size;
extern const unsigned char kMenuFont_bold[];
extern const size_t kMenuFont_bold_size;
#ifdef MENU_FONT_MODERN
// The launchers' font: Inter (SIL Open Font License, assets/fonts/Inter-OFL.txt)
extern const unsigned char kMenuFont_modern[];
extern const size_t kMenuFont_modern_size;
extern const unsigned char kMenuFont_modernBold[];
extern const size_t kMenuFont_modernBold_size;
#endif

namespace menu {

// Gelasio (regular or bold) at that size; the atlas reads it from the
// program and never frees it
inline ImFont* addEmbeddedFont(bool bold, float size)
{
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(bold ? kMenuFont_bold : kMenuFont_regular),
        (int)(bold ? kMenuFont_bold_size : kMenuFont_regular_size), size, &config);
}

#ifdef MENU_FONT_MODERN
// Inter (regular or semibold) at that size, as addEmbeddedFont
inline ImFont* addModernFont(bool bold, float size)
{
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(bold ? kMenuFont_modernBold : kMenuFont_modern),
        (int)(bold ? kMenuFont_modernBold_size : kMenuFont_modern_size), size, &config);
}
#endif

} // namespace menu
