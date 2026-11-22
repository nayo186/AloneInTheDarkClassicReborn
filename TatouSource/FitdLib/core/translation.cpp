///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Built-in languages (translation.h). The file of a translation (made by
// tools/makeRussianTranslation.py) has, after its comments:
//   @@ text N            the text N of the language PAK, in UTF-8; a line
//                        break is \r\n and {XX} is a byte of its own
//   @@ glyph XX W        a letter of the game's font: W columns, then one
//                        line of # and . for each of its rows
//   @@ patch PAK N SIZE  an entry of the player's PAK (of that size), then
//                        the bytes that change
//   @@ frame PAK N       a frame of a sequence (unpacked), then the pixels
//                        that change
//   @@ end
// The changes are base64 of zlib data: runs of a 32-bit offset, a 16-bit
// length and the new bytes.
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "translation.h"
#include "consoleLog.h"

#include "zlib.h"

#include <cstring>
#include <map>
#include <mutex>
#include <sstream>

// The translations compiled in (assets/translations, CMakeLists.txt)
extern const unsigned char kTranslationRussian[];
extern const size_t kTranslationRussian_size;
extern const unsigned char kTranslationRussianAitd2[];
extern const size_t kTranslationRussianAitd2_size;

namespace
{

struct Glyph
{
    int width = 0;
    std::vector<std::string> rows;
};

// The runs of a change, unpacked: offset (4 bytes), length (2), bytes
struct Change
{
    size_t size = 0;           // of the entry it is made for (0: a frame)
    std::vector<u8> runs;
};

typedef std::pair<std::string, int> EntryKey;   // PAK and entry (or frame)

bool s_active = false;
int s_game = AITD1;           // the game of the translation in use
int s_fontEntry = 5;          // the font's entry of ITD_RESS in that game
std::vector<std::vector<u8>> s_texts;
std::map<int, Glyph> s_glyphs;
std::map<EntryKey, Change> s_patches;
std::map<EntryKey, Change> s_frames;
std::map<EntryKey, std::vector<u8>> s_built;   // entries already changed
std::mutex s_mutex;

// The DOS code page 866 code of a Unicode letter ('?' if it has none)
u8 cp866(int code)
{
    if (code < 128)
        return (u8)code;
    if (code >= 0x0410 && code <= 0x043F)
        return (u8)(0x80 + code - 0x0410);
    if (code >= 0x0440 && code <= 0x044F)
        return (u8)(0xE0 + code - 0x0440);
    switch (code)
    {
    case 0x0401: return 0xF0;
    case 0x0451: return 0xF1;
    case 0x0404: return 0xF2;
    case 0x0454: return 0xF3;
    case 0x0407: return 0xF4;
    case 0x0457: return 0xF5;
    case 0x040E: return 0xF6;
    case 0x045E: return 0xF7;
    case 0x00B0: return 0xF8;
    case 0x00B7: return 0xFA;
    case 0x2116: return 0xFC;
    case 0x00A0: return 0xFF;
    default: return '?';
    }
}

// A line of the file as the game's bytes ({XX}: that byte)
void appendLine(const std::string& line, std::vector<u8>& out)
{
    for (size_t i = 0; i < line.size();)
    {
        const unsigned char c = (unsigned char)line[i];
        if (c == '{' && i + 3 < line.size() && line[i + 3] == '}')
        {
            out.push_back((u8)strtol(line.substr(i + 1, 2).c_str(), nullptr, 16));
            i += 4;
            continue;
        }
        int code = c, length = 1;
        if (c >= 0xF0 && i + 3 < line.size()) { code = ((c & 0x07) << 18) | ((line[i + 1] & 0x3F) << 12) | ((line[i + 2] & 0x3F) << 6) | (line[i + 3] & 0x3F); length = 4; }
        else if (c >= 0xE0 && i + 2 < line.size()) { code = ((c & 0x0F) << 12) | ((line[i + 1] & 0x3F) << 6) | (line[i + 2] & 0x3F); length = 3; }
        else if (c >= 0xC0 && i + 1 < line.size()) { code = ((c & 0x1F) << 6) | (line[i + 1] & 0x3F); length = 2; }
        out.push_back(cp866(code));
        i += length;
    }
}

// base64, then zlib: the runs of a change
bool unpackChange(const std::string& text, std::vector<u8>& out)
{
    static const std::string kDigits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<u8> packed;
    int value = 0, bits = 0;
    for (char c : text)
    {
        const size_t digit = kDigits.find(c);
        if (digit == std::string::npos)
            continue;   // '=' and line ends
        value = (value << 6) | (int)digit;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            packed.push_back((u8)(value >> bits));
            value &= (1 << bits) - 1;
        }
    }
    z_stream stream = {};
    if (inflateInit(&stream) != Z_OK)
        return false;
    stream.next_in = packed.data();
    stream.avail_in = (uInt)packed.size();
    int result = Z_OK;
    u8 buffer[16384];
    while (result == Z_OK)
    {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        result = inflate(&stream, Z_NO_FLUSH);
        out.insert(out.end(), buffer, buffer + (sizeof(buffer) - stream.avail_out));
    }
    inflateEnd(&stream);
    return result == Z_STREAM_END;
}

// The runs of a change over data (of size bytes)
void applyChange(const Change& change, u8* data, size_t size)
{
    const std::vector<u8>& runs = change.runs;
    for (size_t i = 0; i + 6 <= runs.size();)
    {
        const size_t offset = READ_LE_U32((void*)&runs[i]);
        const size_t length = READ_LE_U16((void*)&runs[i + 4]);
        i += 6;
        if (i + length > runs.size())
            break;
        if (offset + length <= size)
            memcpy(data + offset, &runs[i], length);
        i += length;
    }
}

void parse(const unsigned char* file, size_t fileSize)
{
    std::istringstream lines(std::string((const char*)file, fileSize));
    std::string line;
    enum { None, Text, GlyphRows, Changes } section = None;
    std::vector<std::string> textLines;
    std::string changeText;
    Change* change = nullptr;
    int current = -1;
    auto finish = [&]() {
        if (section == Text && current >= 0)
        {
            if ((int)s_texts.size() <= current)
                s_texts.resize(current + 1);
            std::vector<u8>& text = s_texts[current];
            for (size_t n = 0; n < textLines.size(); n++)
            {
                if (n)
                {
                    text.push_back('\r');
                    text.push_back('\n');
                }
                appendLine(textLines[n], text);
            }
        }
        else if (section == Changes && change && !unpackChange(changeText, change->runs))
            printf(MAIN_WARN "Russian translation: a change could not be read\n");
        textLines.clear();
        changeText.clear();
        change = nullptr;
    };
    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.compare(0, 3, "@@ ") == 0)
        {
            finish();
            std::istringstream header(line.substr(3));
            std::string kind, pak;
            header >> kind;
            section = None;
            if (kind == "text")
            {
                header >> current;
                section = Text;
            }
            else if (kind == "glyph")
            {
                std::string code;
                header >> code;
                current = (int)strtol(code.c_str(), nullptr, 16);
                header >> s_glyphs[current].width;
                section = GlyphRows;
            }
            else if (kind == "patch" || kind == "frame")
            {
                header >> pak >> current;
                change = &(kind == "patch" ? s_patches : s_frames)[EntryKey(pak, current)];
                if (kind == "patch")
                    header >> change->size;
                section = Changes;
            }
            continue;
        }
        if (section == None)
            continue;
        if (section == Text)
            textLines.push_back(line);
        else if (section == GlyphRows)
            s_glyphs[current].rows.push_back(line);
        else
            changeText += line;
    }
    finish();
    printf(MAIN_TAG "Russian translation: %d texts, %d letters, %d pictures, %d frames\n", (int)s_texts.size(),
        (int)s_glyphs.size(), (int)s_patches.size(), (int)s_frames.size());
}

// The game's font with the translation's letters: each one is added at
// the end of the font's rows and its entry of the table points there
// (core/font.cpp reads it as always)
bool addGlyphs(std::vector<u8>& font)
{
    if (font.size() < 8)
        return false;
    const int align = READ_LE_S16(font.data());
    const int height = font[2];
    int rowBytes = font[3] ? font[3] : READ_LE_S16(font.data() + 4);
    const int tableOffset = (font[6] << 8) | font[7];
    // The table starts at the first code with a letter (align, 32): the
    // entries before it fall on the end of the last row and are no letters
    const int firstCode = align & 0xFF;
    const int table = tableOffset - firstCode * 2;
    const size_t bitmapsEnd = 8 + (size_t)height * rowBytes;
    if (height <= 0 || rowBytes <= 0 || tableOffset < (int)bitmapsEnd || bitmapsEnd > font.size())
        return false;

    int bits = rowBytes * 8;
    std::map<int, int> start;
    for (const auto& glyph : s_glyphs)
    {
        if (glyph.first < firstCode || ((int)glyph.second.rows.size() < height && glyph.second.width))
            return false;
        start[glyph.first] = bits;
        bits += glyph.second.width;
    }
    if (bits > 0xFFF)
        return false;   // past what an entry of the table can point to
    const int newRowBytes = (bits + 7) / 8;
    const int grow = height * (newRowBytes - rowBytes);

    std::vector<u8> out(font.begin(), font.begin() + 8);
    out.resize(8 + (size_t)height * newRowBytes, 0);
    for (int row = 0; row < height; row++)
        memcpy(&out[8 + (size_t)row * newRowBytes], &font[8 + (size_t)row * rowBytes], rowBytes);
    for (const auto& glyph : s_glyphs)
        for (int row = 0; row < height && glyph.second.width; row++)
            for (int x = 0; x < glyph.second.width; x++)
                if (x < (int)glyph.second.rows[row].size() && glyph.second.rows[row][x] == '#')
                {
                    const int bit = start[glyph.first] + x;
                    out[8 + (size_t)row * newRowBytes + (bit >> 3)] |= (u8)(0x80 >> (bit & 7));
                }
    out.insert(out.end(), font.begin() + bitmapsEnd, font.end());
    // The table: 256 entries (GOG's can stop before the last ones)
    const size_t newTable = (size_t)table + grow;
    if (out.size() < newTable + 512)
        out.resize(newTable + 512, 0);
    for (const auto& glyph : s_glyphs)
    {
        const int value = glyph.second.width ? (glyph.second.width << 12) | start[glyph.first] : 0;
        out[newTable + glyph.first * 2] = (u8)(value >> 8);
        out[newTable + glyph.first * 2 + 1] = (u8)value;
    }
    if (newRowBytes <= 255)
        out[3] = (u8)newRowBytes;
    else
    {
        out[3] = 0;
        out[4] = (u8)newRowBytes;
        out[5] = (u8)(newRowBytes >> 8);
    }
    const int newTableOffset = tableOffset + grow;
    out[6] = (u8)(newTableOffset >> 8);
    out[7] = (u8)newTableOffset;
    font.swap(out);
    return true;
}

} // namespace

bool selectTranslation(const std::string& language, const std::string& game)
{
    s_active = false;
    if (language != kRussianLanguage)
        return false;
    const unsigned char* file = nullptr;
    size_t size = 0;
    if (game.empty() || game == "aitd1")
    {
        s_game = AITD1;
        s_fontEntry = 5;
        file = kTranslationRussian;
        size = kTranslationRussian_size;
    }
    else if (game == "aitd2")
    {
        s_game = AITD2;
        s_fontEntry = 1;
        file = kTranslationRussianAitd2;
        size = kTranslationRussianAitd2_size;
    }
    else
        return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    parse(file, size);
    s_active = !s_texts.empty();
    return s_active;
}

bool isRussianTranslation()
{
    return s_active && g_gameId == s_game;
}

int translatedPakCount(const char* name)
{
    if (!s_active || strcmp(name, kRussianLanguage) != 0)
        return -1;
    return (int)s_texts.size();
}

const std::vector<u8>* translatedPakEntry(const char* name, int index)
{
    if (!s_active)
        return nullptr;
    std::lock_guard<std::mutex> lock(s_mutex);
    if (strcmp(name, kRussianLanguage) == 0)
        return index >= 0 && index < (int)s_texts.size() ? &s_texts[index] : nullptr;
    if (g_gameId != s_game)
        return nullptr;
    const EntryKey key(name, index);
    const bool font = strcmp(name, "ITD_RESS") == 0 && index == s_fontEntry;
    auto patch = s_patches.find(key);
    if (!font && patch == s_patches.end())
        return nullptr;
    auto built = s_built.find(key);
    if (built != s_built.end())
        return built->second.empty() ? nullptr : &built->second;

    // The player's own entry, with the translation's changes; if it is not
    // the one the translation was made for, it stays as it is
    std::vector<u8>& entry = s_built[key];
    if (!readPakEntryFromFile(name, index, entry))
        entry.clear();
    else if (font)
    {
        if (!addGlyphs(entry))
        {
            printf(MAIN_WARN "Russian translation: the game's font is not the expected one, left as it is\n");
            entry.clear();
        }
    }
    else if (entry.size() != patch->second.size)
    {
        printf(MAIN_WARN "Russian translation: %s %d is not the expected one, left as it is\n", name, index);
        entry.clear();
    }
    else
        applyChange(patch->second, entry.data(), entry.size());
    return entry.empty() ? nullptr : &entry;
}

bool translateSequenceFrame(const char* name, int frame, const u8* pixels, u8* out)
{
    if (!isRussianTranslation())
        return false;
    auto found = s_frames.find(EntryKey(name, frame));
    if (found == s_frames.end())
        return false;
    memcpy(out, pixels, 320 * 200);
    applyChange(found->second, out, 320 * 200);
    return true;
}

bool isTranslatedPicture(const char* name, int index, bool sequence)
{
    if (!isRussianTranslation())
        return false;
    if (sequence)
        return s_frames.count(EntryKey(name, index)) != 0;
    return s_patches.count(EntryKey(name, index)) != 0;
}
