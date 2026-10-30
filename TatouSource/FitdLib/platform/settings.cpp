///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The options of the game in aitd_remaster.cfg (see settings.h)
///////////////////////////////////////////////////////////////////////////////

#include "settings.h"
#include "consoleLog.h"
#include "controls.h"
#include "gameFolder.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

Settings g_settings;

static std::string configPath()
{
    return configFolder() + "aitd_remaster.cfg";
}

static void setDefaults()
{
    Settings& c = g_settings;
    c.graphics.fullscreen = false;
    c.graphics.frameLimit = 60;
    c.graphics.vsync = true;
    c.graphics.aspect = 0;
    c.graphics.resolution = 0;
    c.graphics.enhancedText = false;
    c.graphics.customBackgrounds = false;
    strcpy(c.graphics.rendererBackend, "auto");

    c.postProcessing.enableBloom = false;
    c.postProcessing.enableFilmGrain = false;
    c.postProcessing.enableSSAO = false;
    c.postProcessing.antialiasing = 0;

    c.animation.enablePoseSmoothing = false;

    c.audio.musicVolume = 100;
    c.audio.soundVolume = 100;
    c.audio.voices[0] = '\0';

    c.cheats.enabled = false;
    c.cheats.infiniteLife = false;

    c.gameData.language[0] = '\0';

    c.debug.enableGraphicsValidation = false;
    c.debug.logLifeScripts = false;
    c.debug.logDrawOrder = false;
    c.debug.dumpLifeScripts = false;
    c.debug.generateNativeLifeScripts = false;
    c.debug.enableNativeLifeScripts = false;

    controlsResetDefaults();
    controlsAnalog().modernStick = true;
}

static bool isTrue(const char* value)
{
    return strcmp(value, "true") == 0 || strcmp(value, "1") == 0;
}

static int clampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

// A text value without its quotes, into a buffer of the given size
static void copyText(char* destination, size_t size, const char* value)
{
    const char* start = (*value == '"') ? value + 1 : value;
    size_t length = strlen(start);
    if (length > 0 && start[length - 1] == '"')
        length--;
    if (length >= size)
        length = size - 1;
    memcpy(destination, start, length);
    destination[length] = '\0';
}

// "controls.<action> = key:Left Shift, pad:leftstick, ..." (the whole value:
// it has spaces)
static void readControlsLine(const char* line)
{
    const char* equals = strchr(line, '=');
    if (!equals)
        return;
    std::string id(line + 9, equals);
    while (!id.empty() && (id.back() == ' ' || id.back() == '\t'))
        id.pop_back();
    std::string text(equals + 1);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
        text.pop_back();
    for (int a = 0; a < kGameActionCount; a++)
        if (id == gameActionId((GameAction)a))
            controlsSetBindings((GameAction)a, bindingsFromText(text));
}

void loadSettings()
{
    setDefaults();
    FILE* file = fopen(configPath().c_str(), "r");
    if (!file)
    {
        printf(CFG_TAG "No config file found, using defaults\n");
        return;
    }

    Settings& c = g_settings;
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        if (strncmp(line, "controls.", 9) == 0)
        {
            readControlsLine(line);
            continue;
        }

        char key[128];
        char value[128];
        if (sscanf(line, "%127s = %127s", key, value) != 2)
            continue;

        if (strcmp(key, "graphics.fullscreen") == 0) c.graphics.fullscreen = isTrue(value);
        else if (strcmp(key, "graphics.frameLimit") == 0) c.graphics.frameLimit = atoi(value);
        else if (strcmp(key, "graphics.vsync") == 0) c.graphics.vsync = isTrue(value);
        else if (strcmp(key, "graphics.aspect") == 0) c.graphics.aspect = atoi(value);
        else if (strcmp(key, "graphics.resolution") == 0) c.graphics.resolution = atoi(value);
        else if (strcmp(key, "graphics.enhancedText") == 0) c.graphics.enhancedText = isTrue(value);
        else if (strcmp(key, "graphics.customBackgrounds") == 0) c.graphics.customBackgrounds = isTrue(value);
        else if (strcmp(key, "graphics.renderer") == 0) copyText(c.graphics.rendererBackend, sizeof(c.graphics.rendererBackend), value);
        else if (strcmp(key, "postprocessing.bloom") == 0) c.postProcessing.enableBloom = isTrue(value);
        else if (strcmp(key, "postprocessing.filmGrain") == 0) c.postProcessing.enableFilmGrain = isTrue(value);
        else if (strcmp(key, "postprocessing.ssao") == 0) c.postProcessing.enableSSAO = isTrue(value);
        else if (strcmp(key, "postprocessing.antialiasing") == 0) c.postProcessing.antialiasing = atoi(value);
        else if (strcmp(key, "animation.poseSmoothing") == 0) c.animation.enablePoseSmoothing = isTrue(value);
        else if (strcmp(key, "audio.musicVolume") == 0) c.audio.musicVolume = atoi(value);
        else if (strcmp(key, "audio.soundVolume") == 0) c.audio.soundVolume = atoi(value);
        else if (strcmp(key, "audio.voices") == 0) copyText(c.audio.voices, sizeof(c.audio.voices), value);
        else if (strcmp(key, "controller.modernStick") == 0) controlsAnalog().modernStick = isTrue(value);
        else if (strcmp(key, "cheats.enabled") == 0) c.cheats.enabled = isTrue(value);
        else if (strcmp(key, "cheats.infiniteLife") == 0) c.cheats.infiniteLife = isTrue(value);
        else if (strcmp(key, "gamedata.language") == 0) copyText(c.gameData.language, sizeof(c.gameData.language), value);
        else if (strcmp(key, "debug.graphicsValidation") == 0) c.debug.enableGraphicsValidation = isTrue(value);
        else if (strcmp(key, "debug.logLifeScripts") == 0) c.debug.logLifeScripts = isTrue(value);
        else if (strcmp(key, "debug.logDrawOrder") == 0) c.debug.logDrawOrder = isTrue(value);
        else if (strcmp(key, "debug.dumpLifeScripts") == 0) c.debug.dumpLifeScripts = isTrue(value);
        else if (strcmp(key, "debug.generateNativeLifeScripts") == 0) c.debug.generateNativeLifeScripts = isTrue(value);
        else if (strcmp(key, "debug.enableNativeLifeScripts") == 0) c.debug.enableNativeLifeScripts = isTrue(value);
        // Anything else is an option that no longer exists: ignored
    }
    fclose(file);

    // Values out of range go back to something safe
    if (c.graphics.frameLimit != 0)
        c.graphics.frameLimit = clampInt(c.graphics.frameLimit, 15, 1000);
    if (c.graphics.aspect < 0 || c.graphics.aspect > 2)
        c.graphics.aspect = 0;
    if (c.graphics.resolution != 0 && (c.graphics.resolution < 320 || c.graphics.resolution > 3840))
        c.graphics.resolution = 0;
    if (c.postProcessing.antialiasing < 0 || c.postProcessing.antialiasing > 1)
        c.postProcessing.antialiasing = 0;
    c.audio.musicVolume = clampInt(c.audio.musicVolume, 0, 100);
    c.audio.soundVolume = clampInt(c.audio.soundVolume, 0, 100);

    printf(CFG_OK "Settings loaded\n");
}

void saveSettings()
{
    FILE* file = fopen(configPath().c_str(), "w");
    if (!file)
    {
        printf(CFG_ERR "Failed to save config file" CON_RESET "\n");
        return;
    }
    const Settings& c = g_settings;
    auto flag = [](bool value) { return value ? "true" : "false"; };

    fprintf(file, "# Alone in the Dark: Classic Reborn - options (written by the launcher and the game)\n\n");

    fprintf(file, "# Screen\n");
    fprintf(file, "graphics.fullscreen = %s\n", flag(c.graphics.fullscreen));
    fprintf(file, "graphics.frameLimit = %d\n", c.graphics.frameLimit);
    fprintf(file, "graphics.vsync = %s\n", flag(c.graphics.vsync));
    fprintf(file, "graphics.aspect = %d\n", c.graphics.aspect);
    fprintf(file, "graphics.resolution = %d\n", c.graphics.resolution);
    fprintf(file, "graphics.enhancedText = %s\n", flag(c.graphics.enhancedText));
    fprintf(file, "graphics.customBackgrounds = %s\n\n", flag(c.graphics.customBackgrounds));

    fprintf(file, "# Screen effects and animation\n");
    fprintf(file, "postprocessing.bloom = %s\n", flag(c.postProcessing.enableBloom));
    fprintf(file, "postprocessing.filmGrain = %s\n", flag(c.postProcessing.enableFilmGrain));
    fprintf(file, "postprocessing.ssao = %s\n", flag(c.postProcessing.enableSSAO));
    fprintf(file, "postprocessing.antialiasing = %d\n", c.postProcessing.antialiasing);
    fprintf(file, "animation.poseSmoothing = %s\n\n", flag(c.animation.enablePoseSmoothing));

    fprintf(file, "# Audio (0-100), narrator (a folder of Voices, empty = English) and language\n");
    fprintf(file, "audio.musicVolume = %d\n", c.audio.musicVolume);
    fprintf(file, "audio.soundVolume = %d\n", c.audio.soundVolume);
    fprintf(file, "audio.voices = \"%s\"\n", c.audio.voices);
    fprintf(file, "gamedata.language = \"%s\"\n\n", c.gameData.language);

    fprintf(file, "# Cheats (F1): enabled turns them on\n");
    fprintf(file, "cheats.enabled = %s\n", flag(c.cheats.enabled));
    fprintf(file, "cheats.infiniteLife = %s\n\n", flag(c.cheats.infiniteLife));

    fprintf(file, "# Controls (modernStick: the left stick walks where it points, seen from the camera)\n");
    fprintf(file, "controller.modernStick = %s\n", flag(controlsAnalog().modernStick));
    for (int a = 0; a < kGameActionCount; a++)
        fprintf(file, "controls.%s = %s\n", gameActionId((GameAction)a), bindingsToText(controlsBindings((GameAction)a)).c_str());

    fprintf(file, "\n# Developer settings (by hand)\n");
    fprintf(file, "graphics.renderer = %s\n", c.graphics.rendererBackend);
    fprintf(file, "debug.graphicsValidation = %s\n", flag(c.debug.enableGraphicsValidation));
    fprintf(file, "debug.logLifeScripts = %s\n", flag(c.debug.logLifeScripts));
    fprintf(file, "debug.logDrawOrder = %s\n", flag(c.debug.logDrawOrder));
    fprintf(file, "debug.dumpLifeScripts = %s\n", flag(c.debug.dumpLifeScripts));
    fprintf(file, "debug.generateNativeLifeScripts = %s\n", flag(c.debug.generateNativeLifeScripts));
    fprintf(file, "debug.enableNativeLifeScripts = %s\n", flag(c.debug.enableNativeLifeScripts));

    fclose(file);
    printf(CFG_OK "Settings saved\n");
}
