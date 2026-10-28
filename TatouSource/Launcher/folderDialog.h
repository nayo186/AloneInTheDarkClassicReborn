///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Folder picker. On Windows it is Explorer's own dialog (address bar to
// paste a full path, quick access, search); elsewhere SDL's dialog.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

struct SDL_Window;

// Windows: shows the dialog (modal) and returns true with the chosen folder
// (UTF-8). Elsewhere returns false: the caller uses SDL_ShowOpenFolderDialog.
bool chooseFolderNative(SDL_Window* window, const std::string& startFolder, const std::string& title,
                        std::string& chosen);
