///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Corrections of the games' texts (textFixes.h). Each line of the table:
//   LANGUAGE number = text        (# starts a comment)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "textFixes.h"
#include "font.h"
#include "cp437.h"
#include "consoleLog.h"

#include <deque>
#include <sstream>
#include <string>

// The tables compiled in (assets/textFixes, embedTextFixes.cmake)
extern const char* const kTextFixes_aitd1;
extern const char* const kTextFixes_jack;

namespace
{

std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// The corrected texts stay alive while the game uses them
std::deque<std::string> s_storage;

} // namespace

void applyTextFixes(textEntryStruct* texts, int count)
{
    const char* table = g_gameId == AITD1 ? kTextFixes_aitd1 : g_gameId == JACK ? kTextFixes_jack : nullptr;
    if (!table)
        return;
    std::istringstream lines(table);
    std::string line;
    while (std::getline(lines, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        size_t space = line.find(' '), equal = line.find('=');
        if (space == std::string::npos || equal == std::string::npos || equal < space)
            continue;
        std::string language = line.substr(0, space);
        int number = atoi(line.substr(space + 1, equal - space - 1).c_str());
        if (language != languageNameString)
            continue;
        std::string text = utf8ToCp437(trim(line.substr(equal + 1)));
        for (int i = 0; i < count; i++)
        {
            if (texts[i].index != number)
                continue;
            s_storage.push_back(text);
            texts[i].textPtr = (u8*)s_storage.back().data();
            texts[i].width = ExtGetSizeFont(texts[i].textPtr);
            printf(MAIN_TAG "Text %d of %s corrected: \"%s\"\n", number, language.c_str(), trim(line.substr(equal + 1)).c_str());
        }
    }
}
