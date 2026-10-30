///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The options of the game, kept in aitd_remaster.cfg (shared by AITD1 and
// Jack, configFolder()): the ones the launcher and the F1 window change, the
// controls (platform/controls.cpp, "controls.<action>" lines) and a few
// developer settings that are only edited by hand.
///////////////////////////////////////////////////////////////////////////////

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

struct Settings
{
    // Screen (launcher and F1)
    struct {
        bool fullscreen;
        int frameLimit;         // frames per second: 0 = no limit (default 60)
        bool vsync;             // wait for the monitor refresh (default on)
        int aspect;             // 0 = 4:3 (the original), 1 = 16:10, 2 = stretched
        int resolution;         // width of the scene image (always 4:3): 0 = the screen's, 640 ... 1920
        bool enhancedText;      // the game's letters rebuilt smooth (textSmooth_ps)
        bool customBackgrounds; // the player's pictures (customBackgrounds.cpp)
        // Developer setting, by hand: "auto" lets bgfx choose; or d3d11,
        // d3d12, opengl, vulkan, metal
        char rendererBackend[32];
    } graphics;

    // Screen effects (screenEffects.cpp), launcher and F1; all off by default
    struct {
        bool enableBloom;
        bool enableFilmGrain;
        bool enableSSAO;
        int antialiasing;       // 3D models: 0 off, 1 FXAA
    } postProcessing;

    // Smoother movement between the key poses (anim.cpp): only the drawn pose
    // changes. Launcher and F1, off by default.
    struct {
        bool enablePoseSmoothing;
    } animation;

    // Volumes, 0-100 (100 = the GOG level), launcher and F1
    struct {
        int musicVolume;   // background music (CD tracks, AdLib)
        int soundVolume;   // voices and sound effects
        char voices[64];   // narrator: a folder of Voices (Shared/narrators.h), empty = the game's (English)
    } audio;

    // Cheats (F1, core/cheats.cpp): none acts while enabled is off
    struct {
        bool enabled;
        bool infiniteLife;   // a hit never takes the player's life
    } cheats;

    // The game's language: the PAK chosen in the launcher, e.g. "ESPAGNOL"
    // (empty: the first available)
    struct {
        char language[16];
    } gameData;

    // Developer settings, by hand only (all off)
    struct {
        bool enableGraphicsValidation;  // graphics API validation layers
        bool logLifeScripts;            // every script instruction in tatou.log (very verbose)
        bool logDrawOrder;              // once a second: objects in drawing order and their masks ([DRAW])
        bool dumpLifeScripts;           // all LISTLIFE scripts written to a file at start
        bool generateNativeLifeScripts; // native C++ code generated for the scripts
        bool enableNativeLifeScripts;   // the generated C++ scripts used instead of the interpreter
    } debug;
};

extern Settings g_settings;

// Reads aitd_remaster.cfg (defaults for anything missing)
void loadSettings();

// Writes the options and the controls to aitd_remaster.cfg
void saveSettings();

#endif // _SETTINGS_H_
