# Changelog

Alone in the Dark: Classic Reborn started on 2026-10-23 from [Alone in the Dark: Re-Haunted](https://github.com/spacefarergames/AloneInTheDarkReHaunted) by Spacefarer Retro Remasters, which in turn builds on [FITD](https://github.com/yaz0r/FITD) by yaz0r. The history of those projects is in their repositories.

This repository was published with a single starting commit, to keep it light: the early history carried large files that are no longer part of the project. Below are the changes of each release and, after them, the changes made in this fork before it was published, newest first, one line per change. The reasons behind larger removals are explained in `docs/DESCARTADO.md` and the fixes in `docs/baseline/BUGS.md`.

## v1.0.0-beta.8 (2026-10-30)

- Updates from the launcher: when it opens, it looks for a newer release on GitHub and offers to install it; the Update button next to the version (bottom right) looks again. Only the programs and documents are replaced, never UserData (options, saves, backgrounds) or launcher.cfg. The launcher connects to the internet only for this; the game never does (README, Updates)
- The launcher shows its version at the bottom right
- Each game can be started without the launcher, for example from a frontend: Tatou --game aitd1 or Tatou --game jack uses the folder, language and options chosen in the launcher; --data "<folder>" and --lang are optional (README, Starting a game without the launcher)

## v1.0.0-beta.7 (2026-10-30)

- Cigar smoke, next to the dining room: a grey cloud turns around the cigars, grows and shrinks, and makes the player cough and lose life while inside it; the water jug puts it out, as in the original (BUGS.md C41, C42)
- Found objects: when an object does not fit in the inventory (weight or number), the game says "You cannot carry anything else" and Enter or Esc closes it, instead of a window without the Leave and Take options that let the limit be skipped (BUGS.md C40); also in Jack in the Dark
- An object's masks (the parts of the background in front of it) cover only that object: the seated dead man in the dining room no longer disappears when the door is opened (BUGS.md C43)
- Drawing order: objects are only ordered when they meet on screen; a door no longer cuts through the player standing next to it (BUGS.md C44)
- Thin parts of the 3D models (chair bars) are drawn one game pixel wide instead of looking like a thin wire mesh (BUGS.md V24)
- tatou.log notes any drawing-order contradiction ([ORDER]), to help with bug reports

## v1.0.0-beta.6 (2026-10-29)

- Save pictures (SAVEn.png) are kept at 320x240 instead of the screen's size: about 100 KB each instead of 2-3 MB, and a little sharper in the save list (BUGS.md C39). Old pictures are replaced when that slot is saved again

## v1.0.0-beta.5 (2026-10-29)

- Linux version: ClassicReborn-<version>-linux-x64.tar.gz, for Ubuntu 22.04, Linux Mint 21, Debian 12 and newer (64-bit), with the GOG or Steam installation (Heroic, Lutris, Proton) or a copy of the game files; nothing else to install (README, On Linux)
- Camera changes with HD backgrounds no longer stall: the backgrounds of the room are decoded ahead in the background (BUGS.md L9)
- Save list: the date and time of each save, right after its slot, were hidden behind the preview (BUGS.md C38)
- The 3D models' gradients and spheres no longer read the palette in the wrong format, which Vulkan forbids (BUGS.md L7)
- Menus use Gelasio, a free font made to Georgia's measures, where the system has no Georgia (Linux)
- The packages carry Gelasio's license (Gelasio-OFL.txt)

## v1.0.0-beta.4 (2026-10-29)

- Modern left stick: the character walks where the stick points, seen from the camera, keeping the direction through camera cuts, like the Resident Evil HD remasters; the D-pad and the keys stay tank controls. Option in Controls (launcher and F1), on by default
- Cheats window in F1: "Enable cheats" and infinite life (a hit never takes the player's life; traps still kill), for AITD1 and Jack in the Dark

## v1.0.0-beta.3 (2026-10-28)

- Narrators: the French CD's narrator (from the player's own CD: voice files, .iso or .bin) or a fans' dubbing (.wav, .ogg, .voc), chosen in the Audio window, with any text language (README, Narrators)
- A book page always plays its whole narration; with some text languages the end of the first page was cut (BUGS.md C35)
- After leaving an object at your feet, the "take it?" window no longer comes back while you walk away (BUGS.md C36)
- The lamp keeps spending oil in the hand after being dropped or stored and taken again (BUGS.md C37)
- One CD image reader for the narrators and the GOG/Steam CD image

## v1.0.0-beta.2 (2026-10-28)

- Background music plays again when a piece ends: the music queued by the scripts follows (after a monster dies, after a scare) (BUGS.md C32)
- The music stops when restarting, dying or going back to the title screen (BUGS.md C33)
- Random events (ambient music and sounds, monsters) differ in every session instead of repeating (BUGS.md C34)
- Advanced options (launcher and F1): scroll bar when the window is too small to show everything

## v1.0.0-beta.1 (2026-10-28)

First public release.

## 2026-10-27

- New README and CONTRIBUTING; player folders in English (Backgrounds, Masks, Exports)
- Remove obsolete documents, game captures and scripts before the public export (block E)
- Ignore Python caches
- Dictionary of methods generated from the headers; layer checker; declarations without code removed (block D, part 2)
- Code map rewritten after the cleanup; ReShade crash handler and textured-model leftovers removed (block D, part 1)
- util depends on nothing; basic types in util/types.h, FITD debugger in core (block C, part 4)
- Custom backgrounds reach the renderer through platform hooks; render pipeline files move to platform (block C, part 3)
- Settings move to platform; platform announces menus and keys through hooks (block C, parts 1-2)
- Script patches reduced to four safety checks; switched-off workarounds removed (block B, part 3)
- Remove unused functions and files (block B, part 2)
- Native life scripts in their own folder, explained; developer settings documented
- Remove the remaster's HD mode and everything that only ran in it (block B, part 1)
- Clean the solution: only the projects and files in use (block A)
- Save list: date and time of each save in tiny digits
- AITD1 saves in the GOG layout; loading restores the clock and the lamp keeps spending oil
- The controller hides the mouse cursor in the game and F1 until the mouse moves
- Drop the buttons and paths of the picture folders (fixed folders, README later)
- Automatic help only while the controller or keyboard is in use (BUGS.md C27)
- Jack in the Dark: export every mask (MASKnn) and its full-screen pictures
- Object fixes ship inside Tatou.exe; editor and F9 only in development builds
- Object editor (F10, debug mode): visual position, turn and size fixes
- Saved fullscreen applies to every game and is saved at once (BUGS.md C26)
- Drop the double-click fullscreen switch
- Controller in every menu: sticks, B to go back, LB/RB tabs, delayed help
- Responsive F1: shared menu metrics, style and sharp launcher fonts in the game
- Responsive launcher: shared screen with landscape and portrait layouts
- "Options window" control: F1, Home and Back, editable like the others
- The game waits everywhere while F1 is open and takes no input from it
- Shared menu base (Shared/ui/menuBase.h) and controller navigation in F1
- Debug mode only by hand in launcher.cfg; drop the separate guide files
- Shorten the help texts and fit the data folder line in the launcher
- Accept the Steam installation (GAME.INST, same edition as GOG)
- Add a debug mode and a portable UserData folder next to the launcher
- Read the CD music and the voices straight from GOG's GAME.GOG
- Run from a GOG installation as well as from a copy of the data
- Add music and voices-and-effects volume controls to F1 and the launcher
- Bring the sound back to the GOG level
- Rebuild the AdLib music output for SoLoud (Jack in the Dark)
- Keep the 3D models out of the bloom
- Start Jack in the Dark from the launcher and with --game jack
- Stop looping samples on game over, floor change and full-screen pictures

## 2026-10-26

- Add a controls window to F1 and the launcher, without quick turns or map
- Rebuild the game controls from scratch, with RT to run
- Fix a stack overflow in loadPak with an index past the end of a PAK
- Add a debug item window (F9) for testing
- Make dark rooms dark, with their message (BUGS.md V23)
- Add "Enhanced text": the game's letters rebuilt at screen resolution
- Antialiasing: FXAA only; drop-down lists never cover the "(?)"
- Add antialiasing of the 3D models (FXAA) as an advanced option
- Antialias the edge of the player's masks over one screen pixel
- Export every mask of the game at once for batch enlarging
- Docs: park the soft mask edges, 3D antialiasing next
- Draw objects in a camera-based order (BUGS.md V22)
- Remove the DOSBox stub, Steam/GOG deployment, Steam overlay and updater
- Use the project's own icon for the game and the launcher
- Launcher: one page per game with embedded 3:4 covers, new name
- BUGS.md: C17 tested and resolved, C18 reviewed (not audible)
- Fix audio playing slower on 48 kHz sound devices
- Remove the remaster's HD screens (stage 2)
- Add custom full-screen pictures and rebuild the controller help
- Remove the remaster's HD camera backgrounds (stage 1)
- Add custom backgrounds and masks, and real-colour glass

## 2026-10-25

- Keep the advanced options chosen in F1 and in the launcher (C15)
- Add aspect ratio, resolution and fullscreen options
- Record the pending window cut (V22) and the discarded attempts
- Record discarded code and attempts in docs/DESCARTADO.md
- Draw glass with the original palette rule (V20)
- Draw objects in the original painter's order (V21)
- Make per-frame animations follow the game clock (V19)
- Fix the crash when closing the window with music playing (C14)
- Document the screen effects and the F1 window (C13)
- Launcher: effect options with help markers
- Rewrite the screen effects and add the F1 options window
- Stop all samples when a game starts
- Fix random crash loading the tatou model (V18)
- Document the launcher options and the game clock fix (V17)
- Launcher: default settings button and advanced options (FPS, VSync)
- Add graphics.frameLimit and graphics.vsync options
- Make the game clock follow real time, not the frame rate (V17)
- Update C1, C4 and H3 in the bug list
- Document the remaster workarounds switched off (C12)
- Switch off three more remaster workarounds (C12)
- Document the cause and fix of V16 (character select after Esc)
- Fix the character select screen after Esc during a story (V16)
- Document the modelAtlas cleanup (C2)
- Remove the unused duplicate of modelAtlas (C2)
- Document C5, C9, C10 and the decisions on V7 and V13
- Switch off the background dump by default (C5)
- Fix the crash when closing the window and the frozen window (C9, C10)
- Document the cause and fix of V15 and the resolution findings
- Grow original masks by half a 320x200 pixel (V15)
- Note V15: thin lines over walls (pending, first attempt undone)
- Document the cause and fix of the intro car flicker (V14)
- Repaint the last full image when a frame has no 3D (V14)
- Note V14: intro car sound fixed, car flicker pending
- Play the queued sample of SAMPLE_THEN and cut the looping one (V14)

## 2026-10-24

- Switch off the remaster's stuck-enemy workarounds (V4)
- Document the cause and fix of V4 (monster stuck in the window)
- Let actors already inside a wall box move out of it (V4)
- Document V13 (thrown objects) and explain V1 (lamp over the chair)
- Fix thrown objects: flight direction, landing spot and wall check (V13)
- Keep incrusted objects visible under masks of moving actors (Original mode)
- Record V12 investigation notes
- Note the lamp hidden by the table mask (V12)
- Fade the composited image so fades no longer flicker
- Fix ESC in books and inventory, faithful pause menu in Original mode
- Make the launcher the Visual Studio startup project
- Add Launcher.exe: choose game, mode and language, then start the game
- Add Original mode: the game without remaster effects
- Build Tatou.exe without a console window except in Debug
- Add launcher command-line options and remove the in-game launcher
- Document file splits and renames in the code map
- Rename meaningless font variables, Sub helpers and Lire locals
- Split remaster screens out of platform/rendererBGFX.cpp
- Move actor shadow drawing to remaster/graphics/shadows
- Split core/main.cpp into topic files
- Update docs after block C
- Fix Lire reading past the end of text when the last word wraps
- Drop unused hdBackground.h include from aitdBox
- Move model texture atlas rendering out of the core renderer
- Move camera HD background loading and floor preload behind core hooks
- Route full-screen HD background handling through core hooks
- Decouple core and life scripts from lantern lighting with core hooks
- Update docs after block A of core hooks
- Stop including the remaster config in every engine file
- Move startup options gate and controls menu behind core hooks
- Move menuMouse.h to platform
- Move the detail level toggle side effects behind a core hook
- Note pause menu scene preview (V9) and unfinished map screen (V10)
- Route systemMenu TTF text and notifications through core hooks
- Confirm H4 predates the font hook
- Note overlapping TTF text in books in High (H4)
- Decouple font from TTF fonts with a draw-text core hook
- Decouple startupMenu and main from TTF fonts with core hooks
- Decouple mainLoop and AITD1 from TTF fonts with core hooks
- Decouple inventory from TTF fonts with core hooks
- Decouple animAction from blood particles with a core hook
- Remove dead embedded-data fallbacks for game files
- Note inventory ignores ESC (V8)
- Update docs for the layered FitdLib layout
- Move engine sources into FitdLib/core and remaining helpers into util
- Note lantern lighting too bright (V7)
- Move remaster sources into FitdLib/remaster
- Move platform sources into FitdLib/platform
- Move script interpreter sources into FitdLib/scripts
- Move utility sources into FitdLib/util
- Document tatou.log and new findings C9/C10
- Write engine output to tatou.log and catch C++ exceptions
- Document heap overflow fix and ASan workflow
- Fix heap overflow in blood and smoke special objects
- Update notes: heap corruption lead and dead code removed
- Remove dead debugFont and menuFade sources
- Add code map for phase 3
- Update docs after removing embedded game data
- Remove embedded original game data
- Unify documentation
- Remove leftover modelAtlas.cpp.fixed and temp_renderer.txt
- Keep generated shader headers in LF
- Remove committed build outputs and ignore local builds
- Mark PAK count and animAction duplicates as resolved in baseline notes
- Fix PAK entry count off by one
- Update baseline notes after Low retest
- Document stuck enemy, missing blood and heap corruption
- Remove duplicate animAction sources from the build

## 2026-10-23

- Log a warning when the engine falls back to embedded game data
