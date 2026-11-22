///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// New font of the texts (textFont.h). How it works:
// - The game writes each text as always (PrintFont, into the 320x200 text
//   layer) and tells us first (g_onTextPrinted): we keep where, which letters
//   and which color, and the pixels that text covers.
// - Each frame, before the layer is shown (g_filterTextLayer), a text still
//   on it (most of its pixels still in its color: nothing covered or erased
//   it) is taken out of the layer and drawn by us instead (g_drawTextOverlay)
//   on a picture at the screen's resolution, in the same place.
// - Each word keeps the box the 1992 font gives it (its width, its line), so
//   lines and pages never change: the capitals are the game's own, rebuilt
//   smooth (Scale2x three times and a soft cut); the other letters come from
//   a modern font (Grenze Gotisch for Alone 1, IM Fell DW Pica Italic for
//   Jack, Crimson Pro for Alone 2, Arvo for Alone 3), squeezed if they do not
//   fit.
// - A soft outline (dark, or light under dark letters) keeps them readable on
//   bright pictures.
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "textFont.h"
#include "coreHooks.h"
#include "platformHooks.h"
#include "settings.h"
#include "font.h"
#include "cp437.h"
#include "translation.h"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"

// A shader program of the game (shaders/embeddedShaders.h)
bgfx::ProgramHandle loadBgfxProgram(const std::string& VSFile, const std::string& PSFile);

// The fonts compiled in (FitdLib/assets/fonts, CMakeLists.txt)
extern const unsigned char kGameFontGothic[];
extern const size_t kGameFontGothic_size;
extern const unsigned char kGameFontItalic[];
extern const size_t kGameFontItalic_size;
extern const unsigned char kGameFontBook[];
extern const size_t kGameFontBook_size;
extern const unsigned char kGameFontSlab[];
extern const size_t kGameFontSlab_size;
extern const unsigned char kGameFontRussian[];
extern const size_t kGameFontRussian_size;
extern const unsigned char kGameFontRussianBook[];
extern const size_t kGameFontRussianBook_size;

// The palette of the game (platform/rendererBGFX.cpp) and the size of the
// scene picture (platform/bgfxGlue.cpp)
extern char RGB_Pal[];
extern int outputResolution[2];

namespace
{

// ---- The game's own font ----------------------------------------------------

// The 1992 font as SetFont reads it (core/font.cpp)
struct GameFont
{
    const u8* data = nullptr;
    int height = 0;
    int rowBytes = 0;
    const u8* bitmaps = nullptr;
    const u8* table = nullptr;
};

GameFont gameFont()
{
    GameFont f;
    const u8* p = (const u8*)PtrFont;
    if (!p)
        return f;
    s16 align = READ_LE_S16((void*)p);
    f.data = p;
    f.height = p[2];
    f.rowBytes = p[3];
    if (!f.rowBytes)
        f.rowBytes = READ_LE_S16((void*)(p + 4));
    u16 tableOffset = READ_LE_U16((void*)(p + 6));
    tableOffset = (u16)(((tableOffset & 0xFF) << 8) | ((tableOffset & 0xFF00) >> 8));
    f.bitmaps = p + 8;
    f.table = p + tableOffset - (align & 0xFF) * 2;
    return f;
}

// Width of a letter (0: a space) and where its picture starts (in bits)
int glyphWidth(const GameFont& f, u8 c, int* bitOffset)
{
    u16 data = READ_LE_U16((void*)(f.table + c * 2));
    data = (u16)(((data & 0xFF) << 8) | ((data & 0xFF00) >> 8));
    if (bitOffset)
        *bitOffset = data & 0xFFF;
    return (data >> 12) & 0xF;
}

bool glyphBit(const GameFont& f, int bitOffset, int row, int column)
{
    int bit = bitOffset + column;
    return (f.bitmaps[row * f.rowBytes + (bit >> 3)] & (0x80 >> (bit & 7))) != 0;
}

// Width of a word as the game measures it (ExtGetSizeFont)
int textWidth(const GameFont& f, const u8* s, int n)
{
    int width = 0;
    for (int i = 0; i < n; i++)
    {
        int w = glyphWidth(f, s[i], nullptr);
        width += (w ? w : g_fontInterWordSpace) + g_fontInterLetterSpace;
    }
    return width;
}

// The pixels PrintFont writes for this text (same steps and limits)
void footprint(const GameFont& f, int x, int y, const u8* s, std::vector<int>& out)
{
    int cursorX = x;
    for (; *s; s++)
    {
        int bitOffset;
        int w = glyphWidth(f, *s, &bitOffset);
        if (w)
        {
            for (int row = 0; row < f.height; row++)
            {
                int py = y + row;
                if (py >= 200 || cursorX < 0 || cursorX >= 320)
                    return;
                int columns = (std::min)(w, 320 - cursorX);
                for (int c = 0; c < columns; c++)
                    if (py >= 0 && glyphBit(f, bitOffset, row, c))
                        out.push_back(py * 320 + cursorX + c);
            }
            cursorX += w;
        }
        else
        {
            cursorX += g_fontInterWordSpace;
        }
        cursorX += g_fontInterLetterSpace;
    }
}

int unicodeOf(u8 c)
{
    // The built-in Russian of Alone 1 writes with the Russian code page of
    // DOS (866); its codes under 32 are signs of the game's own font
    if (isRussianTranslation())
        return c < 32 ? 0 : cp866ToUnicode(c);
    // Alone 1 writes its copyright with the code of DOS's "⌐" (© Infogrames 1992)
    if (c == 0xA9)
        return 0x00A9;
    return cp437ToUnicode(c);
}

// Capitals keep the game's own drawing: A-Z and the accented ones of DOS
bool isCapital(u8 c)
{
    if (c >= 'A' && c <= 'Z')
        return true;
    // Russian: А-Я and Ё, Є, Ї, Ў (code page 866)
    if (isRussianTranslation())
        return (c >= 0x80 && c <= 0x9F) || c == 0xF0 || c == 0xF2 || c == 0xF4 || c == 0xF6;
    switch (c)
    {
    case 0x80: case 0x8E: case 0x8F: case 0x90: case 0x92: case 0x99: case 0x9A: case 0xA5:
        return true;
    default:
        return false;
    }
}

stbtt_fontinfo s_font;
bool s_fontReady = false;

// Letters drawn with the game's own shape, rebuilt smooth: the capitals, the
// numbers and any sign the modern font does not have
bool usesGameShape(const GameFont& f, u8 c)
{
    if (glyphWidth(f, c, nullptr) == 0)
        return false;
    if (isCapital(c) || (c >= '0' && c <= '9'))   // the modern font's numbers are old style, of uneven height
        return true;
    // The ornament under a book's title in Alone 1 ("Demonia Particularis",
    // code of DOS's "µ"): the game's own drawing, not a Greek letter
    if (c == 0xE6 && !isRussianTranslation())   // in Russian, the letter ц
        return true;
    return !s_fontReady || stbtt_FindGlyphIndex(&s_font, unicodeOf(c)) == 0;
}

// ---- Grey pictures (0..1) ---------------------------------------------------

struct Alpha
{
    int w = 0, h = 0;
    std::vector<float> a;
    void resize(int width, int height) { w = width; h = height; a.assign((size_t)(std::max)(0, w * h), 0.0f); }
    float& at(int x, int y) { return a[(size_t)y * w + x]; }
    float get(int x, int y) const { return (x < 0 || y < 0 || x >= w || y >= h) ? 0.0f : a[(size_t)y * w + x]; }
};

void gaussianBlur(Alpha& img, float sigma)
{
    if (sigma <= 0.0f || img.w == 0 || img.h == 0)
        return;
    int radius = (int)std::ceil(sigma * 3.0f);
    std::vector<float> kernel(radius * 2 + 1);
    float sum = 0;
    for (int i = -radius; i <= radius; i++)
        sum += kernel[i + radius] = std::exp(-(float)(i * i) / (2 * sigma * sigma));
    for (float& k : kernel)
        k /= sum;
    std::vector<float> tmp(img.a.size());
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++)
        {
            float v = 0;
            for (int i = -radius; i <= radius; i++)
                v += img.get(x + i, y) * kernel[i + radius];
            tmp[(size_t)y * img.w + x] = v;
        }
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++)
        {
            float v = 0;
            for (int i = -radius; i <= radius; i++)
            {
                int yy = y + i;
                if (yy >= 0 && yy < img.h)
                    v += tmp[(size_t)yy * img.w + x] * kernel[i + radius];
            }
            img.at(x, y) = v;
        }
}

void dilate(Alpha& img, int radius)
{
    if (radius <= 0)
        return;
    Alpha tmp = img;
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++)
        {
            float m = 0;
            for (int i = -radius; i <= radius; i++)
                m = (std::max)(m, tmp.get(x + i, y));
            img.at(x, y) = m;
        }
    tmp = img;
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++)
        {
            float m = 0;
            for (int i = -radius; i <= radius; i++)
                m = (std::max)(m, tmp.get(x, y + i));
            img.at(x, y) = m;
        }
}

// The columns that have ink [first, last]; false: none
bool inkColumns(const Alpha& img, int* first, int* last)
{
    *first = img.w;
    *last = -1;
    for (int x = 0; x < img.w; x++)
        for (int y = 0; y < img.h; y++)
            if (img.a[(size_t)y * img.w + x] > 0.02f)
            {
                *first = (std::min)(*first, x);
                *last = (std::max)(*last, x);
                break;
            }
    return *last >= *first;
}

// ---- Letters ----------------------------------------------------------------

int s_baselineRow = 13;     // the line the small letters stand on (rows of the game's font)

void loadFont()
{
    if (s_fontReady)
        return;
    // Each game its own: Grenze Gotisch with Alone 1's gothic capitals,
    // IM Fell DW Pica Italic with Jack's script capitals, Crimson Pro with
    // Alone 2's book capitals, Arvo with Alone 3's slab (western) capitals
    // (Grenze and Crimson Pro have no Russian letters: in Russian, Alone 1
    // has Ponomar and Alone 2 Alegreya)
    const bool russian = isRussianTranslation();
    const unsigned char* data = g_gameId == JACK ? kGameFontItalic
                              : g_gameId == AITD2 ? (russian ? kGameFontRussianBook : kGameFontBook)
                              : g_gameId == AITD3 ? kGameFontSlab
                              : russian ? kGameFontRussian : kGameFontGothic;
    s_fontReady = stbtt_InitFont(&s_font, data, stbtt_GetFontOffsetForIndex(data, 0)) != 0;

    // The small letters stand where the game's "a" ends
    GameFont f = gameFont();
    if (f.data)
    {
        int bitOffset;
        int w = glyphWidth(f, 'a', &bitOffset);
        for (int row = 0; row < f.height; row++)
            for (int c = 0; c < w; c++)
                if (glyphBit(f, bitOffset, row, c))
                    s_baselineRow = row + 1;
    }
}

// Scale2x (EPX): doubles a pixel drawing keeping 1-pixel lines and smoothing diagonals
std::vector<u8> scale2x(const std::vector<u8>& b, int w, int h)
{
    std::vector<u8> o((size_t)w * h * 4);
    auto at = [&](int x, int y) { x = std::clamp(x, 0, w - 1); y = std::clamp(y, 0, h - 1); return b[(size_t)y * w + x]; };
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            u8 A = at(x, y - 1), B = at(x - 1, y), C = at(x + 1, y), D = at(x, y + 1), E = at(x, y);
            bool ok = A != D && B != C;
            o[(size_t)(y * 2) * w * 2 + x * 2] = (ok && B == A) ? A : E;
            o[(size_t)(y * 2) * w * 2 + x * 2 + 1] = (ok && A == C) ? C : E;
            o[(size_t)(y * 2 + 1) * w * 2 + x * 2] = (ok && B == D) ? B : E;
            o[(size_t)(y * 2 + 1) * w * 2 + x * 2 + 1] = (ok && D == C) ? D : E;
        }
    return o;
}

// A letter of the game (a capital, a sign) rebuilt smooth: its picture K times bigger
// (width x K, the font's height x K)
const Alpha& smoothCapital(u8 c, int K)
{
    static std::unordered_map<int, Alpha> cache;
    int key = c | (K << 8);
    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;

    GameFont f = gameFont();
    int bitOffset;
    int gw = glyphWidth(f, c, &bitOffset);
    int w = gw + 2, h = f.height + 2;
    std::vector<u8> bits((size_t)w * h, 0);
    for (int row = 0; row < f.height; row++)
        for (int x = 0; x < gw; x++)
            bits[(size_t)(row + 1) * w + x + 1] = glyphBit(f, bitOffset, row, x) ? 1 : 0;
    for (int i = 0; i < 3; i++)
    {
        bits = scale2x(bits, w, h);
        w *= 2;
        h *= 2;
    }
    Alpha big;
    big.resize(w, h);
    for (size_t i = 0; i < bits.size(); i++)
        big.a[i] = bits[i] ? 1.0f : 0.0f;
    gaussianBlur(big, 2.2f);

    // From 8 times to K times: each pixel is the average of its square
    Alpha out;
    out.resize(gw * K, f.height * K);
    for (int y = 0; y < out.h; y++)
        for (int x = 0; x < out.w; x++)
        {
            float sx0 = (x + K) * 8.0f / K, sx1 = (x + K + 1) * 8.0f / K;
            float sy0 = (y + K) * 8.0f / K, sy1 = (y + K + 1) * 8.0f / K;
            float sum = 0;
            int n = 0;
            for (int yy = (int)sy0; yy < (int)std::ceil(sy1); yy++)
                for (int xx = (int)sx0; xx < (int)std::ceil(sx1); xx++)
                {
                    sum += big.get(xx, yy);
                    n++;
                }
            float v = n ? sum / n : 0.0f;
            out.at(x, y) = std::clamp((v - 0.5f) * 4.0f + 0.5f, 0.0f, 1.0f);
        }
    return cache.emplace(key, std::move(out)).first->second;
}

// Small letters, numbers and signs with the modern font, standing on
// `baseline` (pixels from the top of a picture `height` tall)
Alpha modernLetters(const u8* s, int n, int K, int height, int baseline)
{
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, 16.0f * K * 0.95f);
    float pen = 0;
    std::vector<int> codes(n);
    for (int i = 0; i < n; i++)
        codes[i] = unicodeOf(s[i]);
    // Width first
    float width = 2;
    for (int i = 0; i < n; i++)
    {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&s_font, codes[i], &advance, &lsb);
        width += advance * scale;
        if (i + 1 < n)
            width += stbtt_GetCodepointKernAdvance(&s_font, codes[i], codes[i + 1]) * scale;
    }
    Alpha img;
    img.resize((int)std::ceil(width) + 2 * K, height);
    std::vector<unsigned char> glyph;
    pen = (float)K;
    for (int i = 0; i < n; i++)
    {
        int x0, y0, x1, y1;
        float shift = pen - std::floor(pen);
        stbtt_GetCodepointBitmapBoxSubpixel(&s_font, codes[i], scale, scale, shift, 0, &x0, &y0, &x1, &y1);
        int gw = x1 - x0, gh = y1 - y0;
        if (gw > 0 && gh > 0)
        {
            glyph.assign((size_t)gw * gh, 0);
            stbtt_MakeCodepointBitmapSubpixel(&s_font, glyph.data(), gw, gh, gw, scale, scale, shift, 0, codes[i]);
            int ox = (int)std::floor(pen) + x0, oy = baseline + y0;
            for (int y = 0; y < gh; y++)
                for (int x = 0; x < gw; x++)
                {
                    int px = ox + x, py = oy + y;
                    if (px >= 0 && py >= 0 && px < img.w && py < img.h)
                        img.at(px, py) = (std::max)(img.at(px, py), glyph[(size_t)y * gw + x] / 255.0f);
                }
        }
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&s_font, codes[i], &advance, &lsb);
        pen += advance * scale;
        if (i + 1 < n)
            pen += stbtt_GetCodepointKernAdvance(&s_font, codes[i], codes[i + 1]) * scale;
    }
    return img;
}

// One word with its natural shape: the game's capitals and the modern letters
// side by side. `top`: where the font's first row goes in the picture
Alpha composeWord(const u8* s, int n, int K, int height, int top)
{
    std::vector<Alpha> pieces;
    int i = 0;
    while (i < n)
    {
        GameFont f = gameFont();
        if (usesGameShape(f, s[i]))
        {
            const Alpha& cap = smoothCapital(s[i], K);
            int first, last;
            Alpha piece;
            if (inkColumns(cap, &first, &last))
            {
                piece.resize(last - first + 1 + K, height);
                for (int y = 0; y < cap.h; y++)
                    for (int x = first; x <= last; x++)
                        if (top + y >= 0 && top + y < height)
                            piece.at(x - first, top + y) = cap.a[(size_t)y * cap.w + x];
            }
            else
            {
                piece.resize(cap.w, height);
            }
            pieces.push_back(std::move(piece));
            i++;
            continue;
        }
        int j = i;
        while (j < n && !usesGameShape(f, s[j]))
            j++;
        Alpha letters = modernLetters(s + i, j - i, K, height, top + s_baselineRow * K);
        int first, last;
        if (inkColumns(letters, &first, &last))
        {
            Alpha piece;
            piece.resize(last - first + 1, height);
            for (int y = 0; y < height; y++)
                for (int x = first; x <= last; x++)
                    piece.at(x - first, y) = letters.at(x, y);
            pieces.push_back(std::move(piece));
        }
        i = j;
    }
    int width = 0;
    for (const Alpha& p : pieces)
        width += p.w;
    Alpha word;
    word.resize(width, height);
    int x = 0;
    for (const Alpha& p : pieces)
    {
        for (int y = 0; y < height; y++)
            for (int px = 0; px < p.w; px++)
                word.at(x + px, y) = p.a[(size_t)y * p.w + px];
        x += p.w;
    }
    return word;
}

// Narrower picture (linear), the same height
Alpha squeeze(const Alpha& src, int width)
{
    Alpha out;
    out.resize(width, src.h);
    float ratio = (float)src.w / width;
    for (int x = 0; x < width; x++)
    {
        // average of the source columns that fall in this one
        float s0 = x * ratio, s1 = (x + 1) * ratio;
        for (int y = 0; y < src.h; y++)
        {
            float sum = 0, weight = 0;
            for (int sx = (int)s0; sx < (int)std::ceil(s1) && sx < src.w; sx++)
            {
                float w = (std::min)(s1, (float)sx + 1) - (std::max)(s0, (float)sx);
                sum += src.a[(size_t)y * src.w + sx] * w;
                weight += w;
            }
            out.at(x, y) = weight > 0 ? sum / weight : 0.0f;
        }
    }
    return out;
}

// ---- A text, drawn ----------------------------------------------------------

// Its letters and outline, K times the game's pixels, from (x0, y0)
struct Raster
{
    int x0 = 0, y0 = 0;
    Alpha text, outline;
};

const int kAbove = 4;   // game rows over the first row of the font (accents)
const int kBelow = 6;   // and under its last (tails of g, j, p)

Raster rasterText(int x, int y, const std::string& text, int K)
{
    GameFont f = gameFont();
    const u8* s = (const u8*)text.data();
    int n = (int)text.size();
    int total = textWidth(f, s, n);
    Raster r;
    int margin = total / 8 + 3;   // room for a sentence a fifth wider (half on each side)
    r.x0 = (x - margin) * K;
    r.y0 = (y - kAbove) * K;
    int height = (f.height + kAbove + kBelow) * K;
    int top = kAbove * K;
    r.text.resize((total + 2 * margin) * K, height);

    // The words and where the game puts each one
    struct Word { int start, length, x, box; Alpha picture; };
    std::vector<Word> words;
    int cursor = x;
    int i = 0;
    while (i < n)
    {
        if (s[i] == ' ' || glyphWidth(f, s[i], nullptr) == 0)   // a space (its own blank letter in Alone 1's font)
        {
            cursor += textWidth(f, s + i, 1);
            i++;
            continue;
        }
        int j = i;
        while (j < n && s[j] != ' ' && glyphWidth(f, s[j], nullptr) != 0)
            j++;
        int wordWidth = textWidth(f, s + i, j - i);
        Word w{ i, j - i, cursor, (wordWidth - g_fontInterLetterSpace) * K, composeWord(s + i, j - i, K, height, top) };
        words.push_back(std::move(w));
        cursor += wordWidth;
        i = j;
    }

    auto paste = [&](const Alpha& picture, int px)
    {
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < picture.w; xx++)
            {
                int tx = px + xx;
                if (tx >= 0 && tx < r.text.w)
                    r.text.at(tx, yy) = (std::max)(r.text.at(tx, yy), picture.a[(size_t)yy * picture.w + xx]);
            }
    };

    if (words.size() == 1)
    {
        // One word (the books write them one by one): in its own box, with
        // some air on each side, as the game's letters had inside theirs
        Word& w = words[0];
        int limit = (std::min)((int)(w.box * 0.94f), w.box - 2 * K);
        if (w.picture.w > limit && limit > 0)
            w.picture = squeeze(w.picture, limit);
        paste(w.picture, w.x * K - r.x0 + (w.box - w.picture.w) / 2);
    }
    else if (!words.empty())
    {
        // A whole sentence (menus, messages): the words with the modern
        // font's spaces (a little wider: its capitals have long strokes to
        // the left), in the width the game gave the sentence
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&s_font, ' ', &advance, &lsb);
        int space = (std::max)((int)std::lround(1.4f * advance * stbtt_ScaleForMappingEmToPixels(&s_font, 16.0f * K * 0.95f)),
                               (g_fontInterWordSpace + g_fontInterLetterSpace) * K);
        // before a capital of the game, one pixel more: its strokes reach left
        auto gapBefore = [&](size_t k)
        {
            return space + (usesGameShape(f, s[words[k].start]) ? K : 0);
        };
        int gaps = 0, letters = 0;
        for (size_t k = 0; k < words.size(); k++)
        {
            letters += words[k].picture.w;
            if (k)
                gaps += gapBefore(k);
        }
        int first = words.front().x * K;
        int box = words.back().x * K + words.back().box - first;
        // A sentence written in one go is a menu line, a message or a name
        // of the inventory, centered by the game: it may be a fifth wider
        // than the game's measure (same center), so the modern letters,
        // wider than those of 1992, keep their spaces
        int limit = (int)(box * 1.2f);
        // Still too wide: the words get narrower, the spaces stay (they only
        // shrink if the words alone would have to lose more than a quarter)
        float wordScale = 1.0f, gapScale = 1.0f;
        if (letters + gaps > limit && limit > 0 && letters > 0)
        {
            wordScale = (float)(limit - gaps) / letters;
            if (wordScale < 0.75f)
            {
                wordScale = 0.75f;
                gapScale = gaps > 0 ? (std::max)(0.3f, (limit - letters * wordScale) / (float)gaps) : 1.0f;
            }
        }
        std::vector<Alpha> pictures;
        int natural = 0;
        for (size_t k = 0; k < words.size(); k++)
        {
            const Alpha& p = words[k].picture;
            int w = (std::max)(1, (int)std::lround(p.w * wordScale));
            pictures.push_back(w < p.w ? squeeze(p, w) : p);
            natural += pictures.back().w + (k ? (int)std::lround(gapBefore(k) * gapScale) : 0);
        }
        Alpha line;
        line.resize(natural, height);
        int lx = 0;
        for (size_t k = 0; k < words.size(); k++)
        {
            if (k)
                lx += (int)std::lround(gapBefore(k) * gapScale);
            const Alpha& p = pictures[k];
            for (int yy = 0; yy < height; yy++)
                for (int xx = 0; xx < p.w && lx + xx < line.w; xx++)
                    line.at(lx + xx, yy) = p.a[(size_t)yy * p.w + xx];
            lx += p.w;
        }
        if (line.w > limit && limit > 0)
            line = squeeze(line, limit);
        paste(line, first - r.x0 + (box - line.w) / 2);
    }

    r.outline = r.text;
    dilate(r.outline, (std::max)(1, K / 2));
    gaussianBlur(r.outline, 0.375f * K);
    return r;
}

// ---- The texts on screen ----------------------------------------------------

struct Shown
{
    int x, y, color;
    std::string text;
    std::vector<int> pixels;   // what PrintFont writes (320x200 indexes)
    bool shadow = false;       // the darker copy one row lower (SelectedMessage)
    int x1 = 0, y1 = 0, x2 = -1, y2 = -1;   // the rectangle of those pixels
    // Written over by another text (the last page of a book): kept for a
    // while, drawn only where the screen still shows it whole (AITD3's
    // turning page shows both pages, BUGS.md T53)
    bool replaced = false;
    int missingFrames = 0;     // frames in a row not on screen
};

// A text off the screen is forgotten after this many frames (about two
// seconds): AITD3's turning page hides the new page for a moment, and the
// game does not write it again (BUGS.md T53)
const int kForgetFrames = 120;

std::vector<Shown> s_texts;
std::vector<int> s_visible;    // indexes in s_texts drawn by us this frame
std::array<unsigned char, 320 * 200> s_filtered;
std::array<unsigned char, 320 * 200> s_hidden{};   // pixels of the letters we draw

// The games with a new font
bool textFontForGame()
{
    return g_gameId == AITD1 || g_gameId == JACK || g_gameId == AITD2 || g_gameId == AITD3;
}

bool active()
{
    // "Enhanced text" (F1, launcher) is this font in the games that have it
    return textFontForGame() && g_settings.graphics.enhancedText && PtrFont;
}

void onTextPrinted(int x, int y, const unsigned char* text, int color)
{
    if (!active() || color == 0 || !text || !*text)
        return;
    std::string s((const char*)text);

    // Written again in the same place: the newest one counts
    for (size_t i = 0; i < s_texts.size(); i++)
        if (s_texts[i].x == x && s_texts[i].y == y && s_texts[i].text == s)
        {
            s_texts.erase(s_texts.begin() + i);
            break;
        }
    // The game's shadow: the same text one row lower, written just before
    if (!s_texts.empty())
    {
        Shown& last = s_texts.back();
        if (last.text == s && last.y == y + 1 && std::abs(last.x - x) <= 1)
            last.shadow = true;
    }

    Shown shown{ x, y, color, s, {}, false };
    footprint(gameFont(), x, y, text, shown.pixels);
    if (shown.pixels.empty())
        return;
    shown.x1 = 320; shown.y1 = 200;
    for (int p : shown.pixels)
    {
        shown.x1 = (std::min)(shown.x1, p % 320); shown.x2 = (std::max)(shown.x2, p % 320);
        shown.y1 = (std::min)(shown.y1, p / 320); shown.y2 = (std::max)(shown.y2, p / 320);
    }
    // A text written over others replaces them (a new page over the last
    // one): their pixels may still be in the same color, but they are gone.
    // Only when it really covers them (a third of their pixels): the lines
    // of a menu touch each other a little (Jack's tall capitals). The
    // game's own shadow (same text, one row lower) stays with it.
    for (size_t i = 0; i < s_texts.size();)
    {
        const Shown& old = s_texts[i];
        bool overlaps = old.x1 <= shown.x2 && shown.x1 <= old.x2 && old.y1 <= shown.y2 && shown.y1 <= old.y2;
        bool itsShadow = old.text == s && std::abs(old.y - y) <= 1 && std::abs(old.x - x) <= 1;
        bool covered = false;
        if (overlaps && !itsShadow)
        {
            // how much of it lies in the new text's rectangle
            int under = 0;
            for (int p : old.pixels)
            {
                int px = p % 320, py = p / 320;
                if (px >= shown.x1 && px <= shown.x2 && py >= shown.y1 && py <= shown.y2)
                    under++;
            }
            covered = under * 3 >= (int)old.pixels.size();
        }
        if (covered)
            s_texts[i].replaced = true;
        i++;
    }

    s_texts.push_back(std::move(shown));
    if (s_texts.size() > 800)
        s_texts.erase(s_texts.begin(), s_texts.begin() + 200);
}

const unsigned char* filterTextLayer(const unsigned char* layer)
{
    s_visible.clear();
    if (!active())
    {
        s_texts.clear();
        return nullptr;
    }
    memcpy(s_filtered.data(), layer, s_filtered.size());

    // How much of each text the layer still shows
    std::vector<float> parts(s_texts.size());
    for (size_t i = 0; i < s_texts.size(); i++)
    {
        const Shown& t = s_texts[i];
        int same = 0;
        for (int p : t.pixels)
            if (layer[p] == t.color)
                same++;
        parts[i] = (float)same / t.pixels.size();
    }

    std::vector<Shown> kept;
    kept.reserve(s_texts.size());
    for (size_t i = 0; i < s_texts.size(); i++)
    {
        Shown& t = s_texts[i];
        const float part = parts[i];
        if (part < 0.02f)
        {
            // covered or erased: forgotten if it does not come back
            if (++t.missingFrames <= kForgetFrames)
                kept.push_back(std::move(t));
            continue;
        }
        t.missingFrames = 0;
        // A replaced text only where it is whole and no newer text there is
        // on screen (the page that was turned)
        bool shows = part >= (t.replaced ? 0.9f : 0.6f);
        if (shows && t.replaced)
            for (size_t j = 0; j < s_texts.size() && shows; j++)
            {
                const Shown& other = s_texts[j];
                if (j != i && !other.replaced && parts[j] >= 0.6f
                    && other.x1 <= t.x2 && t.x1 <= other.x2 && other.y1 <= t.y2 && t.y1 <= other.y2)
                    shows = false;
            }
        if (t.replaced && !shows)
        {
            kept.push_back(std::move(t));
            continue;
        }
        bool drawn = !t.shadow && shows;
        // the shadow goes with its text: the outline replaces it
        if (drawn || t.shadow)
            for (int p : t.pixels)
                if (layer[p] == t.color)
                    s_hidden[p] = 1;
        if (drawn)
            s_visible.push_back((int)kept.size());
        kept.push_back(std::move(t));
    }
    s_texts = std::move(kept);

    // The letters taken out leave what was around them: the selection bar
    // of the menus is in this layer too, and must not keep their holes
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++)
        {
            int p = y * 320 + x;
            if (!s_hidden[p])
                continue;
            unsigned char left = 0, right = 0;
            bool hasLeft = false, hasRight = false;
            for (int d = 1; d <= 12 && !hasLeft && x - d >= 0; d++)
                if (!s_hidden[p - d]) { left = layer[p - d]; hasLeft = true; }
            for (int d = 1; d <= 12 && !hasRight && x + d < 320; d++)
                if (!s_hidden[p + d]) { right = layer[p + d]; hasRight = true; }
            s_filtered[p] = (hasLeft && hasRight && left == right) ? left : 0;
        }
    s_hidden.fill(0);
    return s_filtered.data();
}

// ---- The picture of the letters ---------------------------------------------

bgfx::TextureHandle s_texture = BGFX_INVALID_HANDLE;
int s_textureK = 0;
std::vector<unsigned char> s_pixels;   // RGBA, color multiplied by alpha
std::string s_signature;
std::unordered_map<std::string, Raster> s_rasters;

std::string keyOf(const Shown& t, int K)
{
    return std::to_string(t.x) + "," + std::to_string(t.y) + "," + std::to_string(K) + "," + t.text;
}

void compose(int K)
{
    int W = 320 * K, H = 200 * K;
    s_pixels.assign((size_t)W * H * 4, 0);
    if (s_rasters.size() > 600)
        s_rasters.clear();
    for (int index : s_visible)
    {
        const Shown& t = s_texts[index];
        std::string key = keyOf(t, K);
        auto it = s_rasters.find(key);
        if (it == s_rasters.end())
            it = s_rasters.emplace(key, rasterText(t.x, t.y, t.text, K)).first;
        const Raster& r = it->second;

        float tr = (u8)RGB_Pal[t.color * 3 + 0] / 255.0f;
        float tg = (u8)RGB_Pal[t.color * 3 + 1] / 255.0f;
        float tb = (u8)RGB_Pal[t.color * 3 + 2] / 255.0f;
        // A dark outline under light letters, a light one under dark letters
        bool light = 0.299f * tr + 0.587f * tg + 0.114f * tb > 0.45f;
        float orr = light ? 0.0f : 0.92f, og = light ? 0.0f : 0.86f, ob = light ? 0.0f : 0.75f;

        for (int y = 0; y < r.text.h; y++)
        {
            int sy = r.y0 + y;
            if (sy < 0 || sy >= H)
                continue;
            for (int x = 0; x < r.text.w; x++)
            {
                int sx = r.x0 + x;
                if (sx < 0 || sx >= W)
                    continue;
                float a = r.text.a[(size_t)y * r.text.w + x];
                float o = r.outline.a[(size_t)y * r.text.w + x] * 0.85f;
                if (a <= 0.0f && o <= 0.0f)
                    continue;
                unsigned char* d = &s_pixels[((size_t)sy * W + sx) * 4];
                float dr = d[0] / 255.0f, dg = d[1] / 255.0f, db = d[2] / 255.0f, da = d[3] / 255.0f;
                // outline over what is there, then the letter over both
                dr = orr * o + dr * (1 - o); dg = og * o + dg * (1 - o); db = ob * o + db * (1 - o); da = o + da * (1 - o);
                dr = tr * a + dr * (1 - a); dg = tg * a + dg * (1 - a); db = tb * a + db * (1 - a); da = a + da * (1 - a);
                d[0] = (unsigned char)(dr * 255 + 0.5f);
                d[1] = (unsigned char)(dg * 255 + 0.5f);
                d[2] = (unsigned char)(db * 255 + 0.5f);
                d[3] = (unsigned char)(da * 255 + 0.5f);
            }
        }
    }
}

void drawTextOverlay(bgfx::ViewId view)
{
    if (s_visible.empty())
        return;
    loadFont();
    if (!s_fontReady)
        return;

    // As many times the game's pixels as the scene picture needs
    int K = outputResolution[1] > 0 ? (outputResolution[1] + 199) / 200 : 4;
    K = std::clamp(K, 2, 8);

    std::string signature = std::to_string(K);
    for (int index : s_visible)
    {
        const Shown& t = s_texts[index];
        signature += "|" + keyOf(t, K) + "#" + std::to_string(t.color) + ":" +
            std::to_string((u8)RGB_Pal[t.color * 3]) + "," + std::to_string((u8)RGB_Pal[t.color * 3 + 1]) + "," +
            std::to_string((u8)RGB_Pal[t.color * 3 + 2]);
    }
    if (K != s_textureK)
    {
        if (bgfx::isValid(s_texture))
            bgfx::destroy(s_texture);
        s_texture = bgfx::createTexture2D((uint16_t)(320 * K), (uint16_t)(200 * K), false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        s_textureK = K;
        s_signature.clear();
    }
    if (signature != s_signature)
    {
        compose(K);
        bgfx::updateTexture2D(s_texture, 0, 0, 0, 0, (uint16_t)(320 * K), (uint16_t)(200 * K), bgfx::copy(s_pixels.data(), (uint32_t)s_pixels.size()));
        s_signature = signature;
    }

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    bgfx::TransientVertexBuffer buffer;
    bgfx::allocTransientVertexBuffer(&buffer, 6, layout);
    struct Vertex { float position[3]; float texcoord[2]; };
    const Vertex quad[6] = {
        { { 0.f, 0.f, 1000.f }, { 0.f, 0.f } }, { { 320.f, 200.f, 1000.f }, { 1.f, 1.f } }, { { 320.f, 0.f, 1000.f }, { 1.f, 0.f } },
        { { 0.f, 0.f, 1000.f }, { 0.f, 0.f } }, { { 0.f, 200.f, 1000.f }, { 0.f, 1.f } }, { { 320.f, 200.f, 1000.f }, { 1.f, 1.f } },
    };
    memcpy(buffer.data, quad, sizeof(quad));

    static bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle textureUniform = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle fadeUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(program))
    {
        program = loadBgfxProgram("background_vs", "imageBackground_ps");
        textureUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
        fadeUniform = bgfx::createUniform("u_fadeLevel", bgfx::UniformType::Vec4);
    }
    float fade[4] = { osystem_textFade(), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(fadeUniform, fade);
    bgfx::setTexture(0, textureUniform, s_texture);
    bgfx::setVertexBuffer(0, &buffer);
    // The picture's color is already multiplied by its alpha
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_MSAA | BGFX_STATE_DEPTH_TEST_ALWAYS
        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA));
    bgfx::submit(view, program);
}

} // namespace

void textFontRegister()
{
    g_onTextPrinted = onTextPrinted;
    g_filterTextLayer = filterTextLayer;
    g_drawTextOverlay = drawTextOverlay;
}
