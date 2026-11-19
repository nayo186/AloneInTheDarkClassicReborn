///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The improved backgrounds: one texture pack per game (Alone in the Dark
// with Jack, Alone in the Dark 2)
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

// The packs published: one per game of the launcher's library
enum class TexturePack
{
    Aitd1,   // Alone in the Dark and Jack in the Dark
    Aitd2,
};

// The pack of the game selected in the launcher (0 Alone 1, 1 Alone 2...);
// false if that game has none
bool texturePackForGame(int selectedGame, TexturePack* pack);
// What the pack is doing now (looks at UserData when idle)
PackState texturePackState(TexturePack pack);
// 0-100 while downloading
int texturePackProgress(TexturePack pack);
// Size of the pack in MB, for the button
int texturePackMegabytes(TexturePack pack);
// Starts the download (nothing if it is already downloading)
void texturePackStartDownload(TexturePack pack);
// Removes the pack from UserData
void texturePackRemove(TexturePack pack);
// Waits for a download in progress to stop (when the launcher closes)
void texturePackShutdown();
