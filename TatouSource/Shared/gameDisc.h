///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The player's own CD of a game as an image in the game folder: a .cue with
// its .bin (one or one per track), or an .iso, .bin or .img alone (files but
// no music). The game's files are read straight from its INDARK (INDARK2,
// INDARK3; Jack's from JACK) folder and the music from its tracks; nothing is
// extracted. Its files are checked against the editions we know, as those of
// any data folder (gameFiles.h). Used by the launcher and the game.
// Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cdImage.h"

#include <cctype>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

// The folder of each game on its CD (0 AITD1, 1 AITD2, 2 AITD3) and a file
// only that game has
static const char* const kGameDiscFolder[3] = { "INDARK", "INDARK2", "INDARK3" };
static const char* const kGameDiscMarker[3] = { "LISTBOD2.PAK", "MER.PAK", "AN1.PAK" };

struct GameDisc
{
    int game = -1;                  // 0 AITD1, 1 AITD2, 2 AITD3, -1 no game's CD
    std::filesystem::path image;    // the image (the data track)
    std::filesystem::path cue;      // its track list, empty if none
};

// The game of an opened CD image, -1 if none
inline int gameOfDisc(const CdImage& image)
{
    for (int game = 0; game < 3; game++)
        if (image.hasFile(kGameDiscFolder[game], kGameDiscMarker[game]))
            return game;
    return -1;
}

// The CD image of a game in a folder, opened in `image`: a .cue first (its
// image found from it), then an .iso, .bin or .img alone
inline GameDisc findGameDisc(const std::filesystem::path& folder, CdImage& image)
{
    namespace fs = std::filesystem;
    GameDisc disc;
    std::error_code ec;
    std::vector<fs::path> cues, images;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, ec))
    {
        std::string ext = entry.path().extension().string();
        for (char& c : ext)
            c = (char)tolower((unsigned char)c);
        if (ext == ".cue")
            cues.push_back(entry.path());
        else if (ext == ".iso" || ext == ".bin" || ext == ".img")
            images.push_back(entry.path());
    }
    for (const fs::path& cue : cues)
    {
        const fs::path file = CdImage::cueImageFile(cue);
        if (!file.empty() && image.open(file, cue) && (disc.game = gameOfDisc(image)) >= 0)
        {
            disc.image = file;
            disc.cue = cue;
            return disc;
        }
    }
    for (const fs::path& file : images)
    {
        if (image.open(file) && (disc.game = gameOfDisc(image)) >= 0)
        {
            disc.image = file;
            return disc;
        }
    }
    disc.game = -1;
    return disc;
}
