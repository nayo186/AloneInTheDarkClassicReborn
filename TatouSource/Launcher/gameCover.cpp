///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game covers (see gameCover.h)
///////////////////////////////////////////////////////////////////////////////

#include "gameCover.h"
#include "launcherSettings.h"
#include "ui/controllerPicture.h"

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

// The cover small, blurred and darkened: the background of the whole window,
// so it takes the cover's colours without drawing the eye (a few passes of a
// box blur on a picture of 96 pixels across, smoothed again when stretched)
static SDL_Texture* makeBackground(SDL_Renderer* renderer, const unsigned char* rgba, int width, int height)
{
    const int smallW = 96;
    const int smallH = (std::max)(1, height * smallW / width);
    std::vector<float> picture((size_t)smallW * smallH * 3);
    for (int y = 0; y < smallH; y++)
        for (int x = 0; x < smallW; x++)
        {
            const unsigned char* p = rgba + ((size_t)(y * height / smallH) * width + (size_t)(x * width / smallW)) * 4;
            for (int c = 0; c < 3; c++)
                picture[((size_t)y * smallW + x) * 3 + c] = p[c];
        }
    const int radius = 4;
    std::vector<float> pass(picture.size());
    for (int round = 0; round < 3; round++)
    {
        for (int horizontal = 0; horizontal < 2; horizontal++)
        {
            for (int y = 0; y < smallH; y++)
                for (int x = 0; x < smallW; x++)
                    for (int c = 0; c < 3; c++)
                    {
                        float sum = 0.0f;
                        for (int k = -radius; k <= radius; k++)
                        {
                            const int sx = horizontal ? std::clamp(x + k, 0, smallW - 1) : x;
                            const int sy = horizontal ? y : std::clamp(y + k, 0, smallH - 1);
                            sum += picture[((size_t)sy * smallW + sx) * 3 + c];
                        }
                        pass[((size_t)y * smallW + x) * 3 + c] = sum / (2 * radius + 1);
                    }
            picture.swap(pass);
        }
    }
    // Darkened over the window's own background colour
    static const float base[3] = { 11.0f, 12.0f, 16.0f };
    const float strength = 0.42f;
    std::vector<unsigned char> out((size_t)smallW * smallH * 4);
    for (size_t i = 0; i < (size_t)smallW * smallH; i++)
    {
        for (int c = 0; c < 3; c++)
            out[i * 4 + c] = (unsigned char)(base[c] + (picture[i * 3 + c] - base[c]) * strength);
        out[i * 4 + 3] = 255;
    }
    return makeTexture(renderer, out.data(), smallW, smallH);
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
    cover.background = makeBackground(renderer, pixels, cover.width, cover.height);

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

SDL_Texture* loadControllerDrawing(SDL_Renderer* renderer, float& aspect)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(kControllerPicture, (int)kControllerPicture_size, &width, &height, &channels, 4);
    if (!pixels)
    {
        logLine("The controller drawing could not be decoded");
        return nullptr;
    }
    SDL_Texture* texture = makeTexture(renderer, pixels, width, height);
    if (texture)
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    aspect = (float)height / (float)width;
    stbi_image_free(pixels);
    return texture;
}

void freeGameCover(GameCover& cover)
{
    if (cover.colour)
        SDL_DestroyTexture(cover.colour);
    if (cover.grey)
        SDL_DestroyTexture(cover.grey);
    if (cover.background)
        SDL_DestroyTexture(cover.background);
    cover = GameCover();
}
