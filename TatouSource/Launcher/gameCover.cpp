///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game covers (see gameCover.h)
///////////////////////////////////////////////////////////////////////////////

#include "gameCover.h"
#include "launcherSettings.h"

#include <cstddef>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// coverData.cpp, generated at build time by embedCovers.cmake
extern const unsigned char kCover_aitd1[];
extern const size_t kCover_aitd1_size;
extern const unsigned char kCover_aitd2[];
extern const size_t kCover_aitd2_size;
extern const unsigned char kCover_aitd3[];
extern const size_t kCover_aitd3_size;

static SDL_Texture* makeTexture(SDL_Renderer* renderer, const unsigned char* rgba, int width, int height)
{
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (texture)
    {
        SDL_UpdateTexture(texture, nullptr, rgba, width * 4);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }
    return texture;
}

GameCover loadGameCover(SDL_Renderer* renderer, int game)
{
    static const unsigned char* const data[] = { kCover_aitd1, kCover_aitd2, kCover_aitd3 };
    const size_t sizes[] = { kCover_aitd1_size, kCover_aitd2_size, kCover_aitd3_size };

    GameCover cover;
    if (game < 0 || game > 2)
        return cover;

    int channels = 0;
    unsigned char* pixels = stbi_load_from_memory(data[game], (int)sizes[game], &cover.width, &cover.height, &channels, 4);
    if (!pixels)
    {
        logLine("Cover %d could not be decoded", game + 1);
        return GameCover();
    }
    cover.colour = makeTexture(renderer, pixels, cover.width, cover.height);

    // Greyscale copy (perceived brightness)
    const int count = cover.width * cover.height;
    for (int i = 0; i < count; i++)
    {
        unsigned char* p = &pixels[i * 4];
        const unsigned char grey = (unsigned char)((p[0] * 299 + p[1] * 587 + p[2] * 114) / 1000);
        p[0] = p[1] = p[2] = grey;
    }
    cover.grey = makeTexture(renderer, pixels, cover.width, cover.height);
    stbi_image_free(pixels);
    return cover;
}

void freeGameCover(GameCover& cover)
{
    if (cover.colour)
        SDL_DestroyTexture(cover.colour);
    if (cover.grey)
        SDL_DestroyTexture(cover.grey);
    cover = GameCover();
}
