///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Log tags: every engine message (printf) starts with the tag of its
// subsystem, e.g. "[CFG] Settings loaded", and goes to tatou.log
// (logFile.cpp). TAG is information; OK, WARN and ERR say how it went.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// The console colours of the original remaster, now empty (the log is a
// plain text file); kept because many messages end with CON_RESET
#define CON_RESET ""
#define CON_PINK  ""

// Settings (settings.cpp)
#define CFG_TAG    "[CFG] "
#define CFG_OK     "[CFG] "
#define CFG_ERR    "[CFG] "

// Life script interpreter (life.cpp)
#define LIFE_TAG   "[LIFE] "
#define LIFE_OK    "[LIFE] "
#define LIFE_WARN  "[LIFE] "

// Main engine (main.cpp)
#define MAIN_TAG   "[MAIN] "
#define MAIN_OK    "[MAIN] "
#define MAIN_WARN  "[MAIN] "
#define MAIN_ERR   "[MAIN] "

// Polygon renderer (polys.cpp)
#define POLY_ERR   "[POLY] "

// BGFX graphics init (bgfxGlue.cpp)
#define BGFX_TAG   "[BGFX] "
#define BGFX_OK    "[BGFX] "

// PAK file system (pak.cpp)
#define PAK_TAG    "[PAK] "

// Variable evaluator (evalVar.cpp)
#define EVAL_WARN  "[EVAL] "

// Audio system (osystemAL.cpp)
#define AL_TAG     "[AL] "
#define AL_OK      "[AL] "
#define AL_WARN    "[AL] "

// OS / SDL layer (osystemSDL.cpp)
#define SDL_TAG    "[SDL] "
#define SDL_OK     "[SDL] "
#define SDL_ERR    "[SDL] "

// Input / controller (input.cpp)
#define INP_TAG    "[INP] "
#define INP_OK     "[INP] "
#define INP_ERR    "[INP] "

// 3D/primitive renderer (renderer.cpp)
#define RNDR_WARN  "[RNDR] "

// Main loop / stuck-player recovery (mainLoop.cpp)
#define MLOOP_TAG  "[MLOOP] "
#define MLOOP_WARN "[MLOOP] "

// Track / path system (track.cpp)
#define TRACK_WARN "[TRACK] "

// Music system (music.cpp)
#define MUSIC_TAG  "[MUSIC] "
#define MUSIC_OK   "[MUSIC] "
#define MUSIC_WARN "[MUSIC] "

// HQR archive (hqr.cpp)
#define HQR_WARN   "[HQR] "

// Inventory (inventory.cpp)
#define INV_TAG    "[INV] "

// Animation actions (animAction.cpp)
#define AACT_TAG   "[AACT] "
#define AACT_WARN  "[AACT] "

// Object system (object.cpp)
#define OBJ_WARN   "[OBJ] "

// File access (fileAccess.cpp)
#define FACC_ERR   "[FILE] "

// FM OPL emulator (fmopl.cpp)
#define FMOPL_WARN "[FMOPL] "

// Voice Over playback (osystemAL.cpp)
#define VO_TAG     "[VO] "
#define VO_OK      "[VO] "
#define VO_WARN    "[VO] "

// Sequence player (sequence.cpp)
#define SEQ_TAG    "[SEQ] "
