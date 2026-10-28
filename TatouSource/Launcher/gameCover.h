///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game covers: the 3:4 picture of each game of the trilogy (AITD1, AITD2,
// AITD3; Jack in the Dark belongs to AITD1). The pictures are part of the
// project (Launcher/assets/covers) and are compiled into Launcher.exe by
// embedCovers.cmake, so they cannot be replaced from outside.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL3/SDL.h>

struct GameCover
{
    SDL_Texture* colour = nullptr;
    SDL_Texture* grey = nullptr;   // shown while the game has no data folder
    int width = 0;
    int height = 0;
};

// game: 0 = AITD1, 1 = AITD2, 2 = AITD3
GameCover loadGameCover(SDL_Renderer* renderer, int game);

// Frees the textures of a cover
void freeGameCover(GameCover& cover);
