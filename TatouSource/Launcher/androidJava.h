///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Android only: what the launcher asks the app's Java side
// (android/.../ClassicRebornActivity.java):
// - importing: Android's folder picker and a copy of what the chosen folder
//   holds into the app's UserData, where other apps cannot copy, recognised
//   by its contents: a game with the languages, narrators, backgrounds, masks
//   and saves its folder has (as the PC's portable UserData/<game>), the
//   PC's whole UserData, or only a language, a narrator or pictures; the
//   launcher follows the progress;
// - for the updates (updater.cpp): downloads (Android has no curl) and the
//   new app handed to Android's installer, which asks the player.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

enum class ImportState { Idle, Picking, Copying, Done, Failed, Cancelled };
// What the chosen folder held (None with Failed: nothing was recognised)
enum class ImportKind { None, Game, Language, Narrator, Pictures };

struct ImportProgress
{
    ImportState state = ImportState::Idle;
    ImportKind kind = ImportKind::None;
    long long copied = 0;   // bytes
    long long total = 0;
};

// Opens the folder picker; what it holds goes into userData (the UTF-8 path
// of the app's UserData), for game (the launcher's tab: 0 AITD1, 1 AITD2, 2 AITD3)
void androidImportStart(const std::string& userData, int game);
// Where the import is (read every frame)
ImportProgress androidImportProgress();

// Downloads url into the file at path (UTF-8); false if it failed (any thread)
bool androidDownload(const std::string& url, const std::string& path, int seconds);
// Hands the app package at path to Android's installer, which asks the
// player to confirm; false if it could not (any thread)
bool androidInstallPackage(const std::string& path);
