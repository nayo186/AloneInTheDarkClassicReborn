///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The improved backgrounds of Alone in the Dark and Jack: one texture pack
// (FitdLib/remaster/graphics/texturePack.h) the player can download from the
// advanced options into UserData, or remove. Only when the player asks: it is
// downloaded on a thread of its own to a .part file, kept only when it arrived
// whole (its size, a ZIP), and an older version of the pack is removed then.
// The same on PC (curl, as the updates) and Android (the app's download).
///////////////////////////////////////////////////////////////////////////////

#pragma once

enum class PackState
{
    Missing,      // not in UserData
    Downloading,
    Installed,
    Failed,       // nothing was kept; can be tried again
};

// What the pack is doing now (looks at UserData when idle)
PackState texturePackState();
// 0-100 while downloading
int texturePackProgress();
// Size of the pack in MB, for the button
int texturePackMegabytes();
// Starts the download (nothing if it is already downloading)
void texturePackStartDownload();
// Removes the pack from UserData
void texturePackRemove();
// Waits for a download in progress to stop (when the launcher closes)
void texturePackShutdown();
