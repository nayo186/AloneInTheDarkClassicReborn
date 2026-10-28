# Contributing

Thanks for wanting to help. Bug reports, fixes and new optional features are all welcome.

## Before you start

- **Never commit game data.** No `.PAK`, `.ITD`, music, voices or pictures of the original game, and no saves or logs. The engine reads everything from the player's own copy.
- Build the project as explained in [README.md](README.md#building) and test with your own copy of the game (GOG or Steam).
- Read `docs/MAPA_CODIGO.md` (the folders, files and layers) and look up functions in `docs/DICCIONARIO.md`. These documents are in Spanish for now; translations are welcome too.

## Rules of the code

- **Layers.** `util/` → `platform/` → `core/` + `scripts/` → `remaster/`. A layer never includes a header of a layer above it. When a lower layer must tell an upper one that something happened, add a hook (a function pointer that starts empty) to `core/coreHooks.h` or `platform/platformHooks.h` and fill it from `remaster/config/remasterHooks.cpp`. Check with `python tools/checkLayers.py`: the only crossing it may print is platform → core.
- **New features are optional.** With an option off, the game must behave exactly as before. Options live in `platform/settings.*` (`aitd_remaster.cfg`) and appear in both the launcher and the F1 window; their texts go in `Shared/advancedOptionsText.h`.
- **Fix the cause.** A defect of the original game is fixed even if the 1992 game had it, but at its cause, not by hiding it.
- **Keep the engine's French names** (`AffObjet`, `LoadEtage`, `GereCollision`...): they are the names of the reverse engineering and let the code be compared with FITD. Renaming meaningless names (`var_1BA`, `...Sub1`) is welcome.
- **Nothing unused.** Remove dead code instead of commenting it out.
- **Document public functions.** Every function declared in a `.h` gets a one-line `//` comment above it, in English. Then regenerate the dictionary: `python tools/makeDictionary.py` (`--missing` lists the functions without a comment).
- **Do not edit `scripts/native/nativeLifeScripts_generated.cpp` by hand**: the engine generates it (see `scripts/native/nativeLife.h`).
- After adding, moving or removing source files, regenerate the solution with CMake (`FitdLib` compiles every `.cpp` in its folders).
- Shaders: add the `.sc` file to `FitdLib/shaders/`, an `addShaderProgram` line to `FitdLib/CMakeLists.txt`, and its entries to `embeddedShaders.cpp` and `embeddedShadersIncludes.h`.

## Reporting a bug

Say which game (*Alone in the Dark* or *Jack in the Dark*), where it happens (floor, room, what you were doing) and attach `tatou.log` from `UserData\AITD1` or `UserData\JACK` (the session before is in `tatou.prev.log`). A crash also writes `crash_log.txt`.

## Pull requests

One change per pull request, with a short explanation of what it fixes or adds and how you tested it. Fixed defects are recorded in `docs/baseline/BUGS.md` with their cause and fix.

## License

By contributing you agree that your code is released under the GNU General Public License v2, like the rest of the project.
