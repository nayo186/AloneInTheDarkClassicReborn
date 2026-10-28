///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Native life scripts: the game's own scripts turned into C++.
//
// What they are. The logic of the game (doors, enemies, items, events) is not
// in the executable: it is in "life scripts", bytecode stored in LISTLIFE.PAK
// that the engine interprets every frame (scripts/life.cpp). This system
// translates that bytecode into readable C++, one function per script
// (nativeLifeScripts_generated.cpp, 563 scripts of AITD1), so the game's
// logic can be read and studied like normal code.
//
// How it is used. Developer settings in aitd_remaster.cfg, all off:
//   debug.dumpLifeScripts = true            writes LISTLIFE_dump.txt (every
//                                           script as readable pseudo-code)
//   debug.generateNativeLifeScripts = true  writes nativeLifeScripts_generated.cpp
//                                           in the working folder (copy it
//                                           here; never edit it by hand)
//   debug.enableNativeLifeScripts = true    runs the generated C++ instead of
//                                           the interpreter (AITD1 only; each
//                                           script is checked against
//                                           LISTLIFE_dump.txt and skipped if
//                                           its bytecode changed)
// The game always plays with the interpreter; this is a tool to understand
// the scripts (and, adapting the generator to its instructions, could do the
// same for Alone in the Dark 2).
//
// Files: nativeLife.cpp (registry, dump and generator), nativeLifeHelpers.h
// (the functions the generated code calls), nativeLifeHelpersImpl.cpp,
// nativeLifeScripts_AITD1.cpp (registers the generated scripts) and
// nativeLifeScripts_generated.cpp (the output).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "life.h"

// Signature for a native life script function.
// Returns true if the script handled execution (exitLife), false to keep running.
// The function is called once per frame (like processLife) and must handle its own
// state machine logic, similar to the bytecode interpreter.
typedef void (*NativeLifeFunc)(int lifeNum, bool callFoundLife);

// Look up whether a native override exists for the given life script number.
// Returns the function pointer if registered, or nullptr if bytecode should be used.
NativeLifeFunc getNativeLifeScript(int lifeNum);

// Returns the total number of registered native life script overrides.
int getNativeLifeScriptCount();

// Initialize all native life script registrations (call during startup).
void initNativeLifeScripts();

// Dump a life script's bytecode to console as readable pseudo-C.
// If lifeNum is -1, dumps all life scripts.
void dumpLifeScript(int lifeNum);

// Dump all life scripts to console.
void dumpAllLifeScripts();

// Generate compilable C code for a single life script using nativeLifeHelpers.h
void generateNativeLifeScriptC(int lifeNum);

// Generate compilable C code for all life scripts to nativeLifeScripts_generated.cpp
void generateAllNativeLifeScripts();

// Validate registered native scripts against a LISTLIFE_dump.txt reference file.
// Any script whose live decompilation differs from the reference is removed from
// the registry so the bytecode interpreter runs instead.
void validateNativeLifeScriptsAgainstDump(const char* dumpFilePath);

// Register a native C function only if the script contains no risky opcodes
// (animations, rotations, movement, combat, spatial changes, etc.).
// Safe scripts are pure logic/branch/variable/flag/message/audio scripts.
void registerNativeLifeScriptIfSafe(int lifeNum, NativeLifeFunc func);
