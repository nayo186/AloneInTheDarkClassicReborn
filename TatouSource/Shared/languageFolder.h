///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Languages from the player's CD (AITD3): a folder Languages\<name> in the
// working folder (Shared/userFolder.h), e.g. Languages\Spanish, holds either
// - loose files: the language PAK (ESPAGNOL.PAK) and the data files that go
//   with it (LISTSAMP.PAK, LISTLIFE.PAK, PRIORITY.ITD, ITD_RESS.PAK; the
//   whole installed folder of the CD also works), and the CD tracks as
//   NN.ogg (track - 1, as in a copy of the data) or TrackNN.ogg (as GOG
//   names them), .wav or .mp3;
// - or the CD image: one .bin (or .img) with its .cue of the same name, for
//   the tracks; an .iso or a .bin without .cue has the files but no tracks.
// Used by the game and the launcher. Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cdImage.h"

#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

// The folder of AITD3's files on its CD
static const char* const kLanguageImageFolder = "INDARK3";

// The folder in Languages of a language, by the name of its PAK (ESPAGNOL:
// Spanish); empty if it is not one of the game's languages
inline const char* languagePackName(const char* languagePak)
{
    static const char* const names[][2] = {
        { "ENGLISH", "English" }, { "FRANCAIS", "French" }, { "ITALIANO", "Italian" },
        { "ESPAGNOL", "Spanish" }, { "DEUTSCH", "German" },
    };
    for (const auto& name : names)
        if (strcmp(languagePak, name[0]) == 0)
            return name[1];
    return "";
}

// The CD image in a language folder whose INDARK3 folder has the language
// PAK (e.g. "ESPAGNOL.PAK"), opened with its .cue if there is one; false if
// the folder has none
inline bool openLanguageImage(const std::filesystem::path& folder, const std::string& languagePak, CdImage& image)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, ec))
    {
        std::string ext = entry.path().extension().string();
        for (char& c : ext)
            c = (char)tolower((unsigned char)c);
        if (ext != ".bin" && ext != ".img" && ext != ".iso")
            continue;
        fs::path cue = entry.path();
        cue.replace_extension(".cue");
        if (!fs::exists(cue, ec))
        {
            cue.replace_extension(".CUE");
            if (!fs::exists(cue, ec))
                cue.clear();
        }
        if (image.open(entry.path(), cue) && image.hasFile(kLanguageImageFolder, languagePak))
            return true;
    }
    return false;
}
