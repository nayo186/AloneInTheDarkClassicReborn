///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Built-in languages (translation.h). The file of a translation (made by
// tools/makeRussianTranslation.py) has, after its comments:
//   @@ text N                the text N of the language PAK, in UTF-8; a line
//                            break is \r\n and {XX} is a byte of its own
//   @@ glyph XX W            a letter of the game's font: W columns, then
//                            one line of # and . for each of its rows
//   @@ patch ITD_RESS N SIZE an entry of the player's ITD_RESS (of that
//                            size), then lines "offset HEX": the new bytes
//   @@ end
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "translation.h"
#include "consoleLog.h"

#include <cstring>
#include <map>
#include <mutex>
#include <sstream>

// The translation compiled in (assets/translations, CMakeLists.txt)
extern const unsigned char kTranslationRussian[];
extern const size_t kTranslationRussian_size;

namespace
{

struct Glyph
{
    int width = 0;
    std::vector<std::string> rows;
};

struct Patch
{
    size_t size = 0;
    std::vector<std::pair<size_t, std::vector<u8>>> bytes;
};

bool s_active = false;
bool s_parsed = false;
std::vector<std::vector<u8>> s_texts;
std::map<int, Glyph> s_glyphs;
std::map<int, Patch> s_patches;          // by ITD_RESS entry
std::map<int, std::vector<u8>> s_built;  // ITD_RESS entries already changed
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

void parse()
{
    if (s_parsed)
        return;
    s_parsed = true;
    std::istringstream file(std::string((const char*)kTranslationRussian, kTranslationRussian_size));
    std::string line;
    enum { None, Text, GlyphRows, PatchBytes } section = None;
    std::vector<std::string> textLines;
    int current = -1;
    auto finishText = [&]() {
        if (section != Text || current < 0)
            return;
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
        textLines.clear();
    };
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.compare(0, 3, "@@ ") == 0)
        {
            finishText();
            std::istringstream header(line.substr(3));
            std::string kind;
            header >> kind;
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
            else if (kind == "patch")
            {
                std::string pak;
                header >> pak >> current;
                header >> s_patches[current].size;
                section = PatchBytes;
            }
            else
                section = None;
            continue;
        }
        if (section == None && (line.empty() || line[0] == '#'))
            continue;
        switch (section)
        {
        case Text:
            textLines.push_back(line);
            break;
        case GlyphRows:
            s_glyphs[current].rows.push_back(line);
            break;
        case PatchBytes:
        {
            std::istringstream values(line);
            size_t offset = 0;
            std::string hex;
            values >> offset >> hex;
            std::vector<u8> bytes;
            for (size_t i = 0; i + 1 < hex.size(); i += 2)
                bytes.push_back((u8)strtol(hex.substr(i, 2).c_str(), nullptr, 16));
            s_patches[current].bytes.emplace_back(offset, std::move(bytes));
            break;
        }
        default:
            break;
        }
    }
    finishText();
    printf(MAIN_TAG "Russian translation: %d texts, %d letters, %d pictures\n", (int)s_texts.size(),
        (int)s_glyphs.size(), (int)s_patches.size());
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
    for (const auto& glyph : s_glyphs)
        if (glyph.first < firstCode)
            return false;

    int bits = rowBytes * 8;
    std::map<int, int> start;
    for (const auto& glyph : s_glyphs)
    {
        if ((int)glyph.second.rows.size() < height && glyph.second.width)
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
    // The table: 256 entries (GOG's stops two bytes short of the last)
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
    s_active = language == kRussianLanguage && (game.empty() || game == "aitd1");
    if (!s_active)
        return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    parse();
    s_active = !s_texts.empty();
    return s_active;
}

bool isRussianTranslation()
{
    return s_active && g_gameId == AITD1;
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
    if (g_gameId != AITD1 || strcmp(name, "ITD_RESS") != 0)
        return nullptr;
    const bool font = index == 5;
    auto patch = s_patches.find(index);
    if (!font && patch == s_patches.end())
        return nullptr;
    auto built = s_built.find(index);
    if (built != s_built.end())
        return built->second.empty() ? nullptr : &built->second;

    // The player's own entry, with the translation's changes; if it is not
    // the one the translation was made for, it stays as it is
    std::vector<u8>& entry = s_built[index];
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
        printf(MAIN_WARN "Russian translation: ITD_RESS %d is not the expected one, left as it is\n", index);
        entry.clear();
    }
    else
        for (const auto& bytes : patch->second.bytes)
            if (bytes.first + bytes.second.size() <= entry.size())
                memcpy(&entry[bytes.first], bytes.second.data(), bytes.second.size());
    return entry.empty() ? nullptr : &entry;
}
