///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Launch options: command-line parameters passed by the launcher
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "settings.h"
#include "launchOptions.h"
#include "coreHooks.h"
#include <algorithm>
#include <cctype>

LaunchOptions g_launchOptions;

static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string toUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    return s;
}

void parseLaunchOptions(int argc, char* argv[])
{
    for (int i = 1; i + 1 < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--game")
        {
            g_launchOptions.game = toLower(argv[++i]);
        }
        else if (arg == "--lang")
        {
            g_launchOptions.language = toUpper(argv[++i]);
        }
    }

    printf(MAIN_TAG "Launch options: game=%s lang=%s\n",
        g_launchOptions.game.empty() ? "(default)" : g_launchOptions.game.c_str(),
        g_launchOptions.language.empty() ? "(saved)" : g_launchOptions.language.c_str());
}

void applyLaunchOptions()
{
    // --lang only applies to this session: the saved language in
    // g_settings stays untouched (the launcher owns it), so a config
    // save during the game never overwrites the launcher's choice.

    // Game: Alone in the Dark 1, or Jack in the Dark (platform/gameFolder.cpp
    // already entered its folder; detectGame recognises it by its files)
    if (!g_launchOptions.game.empty() && g_launchOptions.game != "aitd1" && g_launchOptions.game != "jack")
    {
        printf(MAIN_WARN "--game %s is not supported yet, starting with the data in the working directory" CON_RESET "\n",
            g_launchOptions.game.c_str());
    }

    // Language: launcher choice first, then the saved one; the PAK must exist
    std::string language = g_launchOptions.language;
    if (language.empty())
        language = toUpper(g_settings.gameData.language);

    if (!language.empty())
    {
        if (dataFileExists((language + ".PAK").c_str()))
        {
            languageNameString = language;
            printf(MAIN_OK "Language: %s\n", language.c_str());
        }
        else
        {
            printf(MAIN_WARN "Language file %s.PAK not found, using the first available language" CON_RESET "\n",
                language.c_str());
        }
    }
}
