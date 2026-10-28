///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The portable user folder, next to Launcher.exe and Tatou.exe. Outside the
// debug mode everything the game writes goes there, whether the data comes
// from a GOG installation or from a copy (neither is ever written to):
//   UserData\aitd_remaster.cfg         options shared by both games
//   UserData\AITD1\ and UserData\JACK\ log of each game, and
//       Saves\        the saves (SAVEn.ITD and their pictures SAVEn.png)
//       Backgrounds\  the player's camera backgrounds and full-screen pictures
//       Masks\        the player's masks
//       Exports\      what F1 exports
// The launcher creates the folders the first time it opens (empty: the
// general README explains them). With the debug
// mode on and a copy of the data, everything stays inside the copy as before
// (backgrounds_custom, backgrounds_export). Shared by the game and the launcher.
///////////////////////////////////////////////////////////////////////////////

#pragma once

static const char* const kUserDataFolder = "UserData";
static const char* const kSavesFolder = "Saves";
static const char* const kPicturesFolder = "Backgrounds";
static const char* const kMasksFolder = "Masks";
static const char* const kExportsFolder = "Exports";
