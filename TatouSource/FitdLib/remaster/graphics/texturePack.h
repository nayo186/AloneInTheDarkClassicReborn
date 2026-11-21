///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Texture packs: one file with the player's backgrounds and masks of every
// game, so a player only has to put that file in UserData (or let the
// launcher download it). A pack is a ZIP (stored, or deflated) with the
// extension .texture, e.g. UserData\nayo.texture, holding the folders of the
// user folder: AITD1/Backgrounds/CAMERA00_000.jpg, AITD1/Masks/...,
// JACK/Backgrounds/..., JACK/Masks/... (tools/makeTexturePack.py makes one).
// Pictures are read straight from it, nothing is extracted. A loose file in
// the usual folders comes first, so a single picture can still be changed.
// Thread-safe: the pictures are read from the job threads.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <string>
#include <vector>

// The packs' file with that name ("AITD1/Backgrounds/CAMERA00_000.jpg",
// any case) is there; time is the pack's (a pack replaced is read again)
bool texturePackHas(const std::string& name, std::filesystem::file_time_type* time = nullptr);

// The bytes of that file of the packs; false if none has it or it cannot be read
bool texturePackRead(const std::string& name, std::vector<unsigned char>& data);

// The names of the packs' files that start with prefix (any case), in lower
// case ("aitd1/masks/camera00_002_mask_0_1.png")
void texturePackList(const std::string& prefix, std::vector<std::string>& names);

// The folder of the current game in the packs: "AITD1", "JACK", "AITD2", "AITD3"
const char* texturePackGameFolder();
