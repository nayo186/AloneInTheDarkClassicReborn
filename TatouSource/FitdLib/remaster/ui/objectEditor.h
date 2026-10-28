///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Object editor (F10, debug mode), a development tool: lists the 3D objects
// of the camera and moves, turns or resizes the chosen one while watching the
// scene; the game stays frozen but is drawn every frame. It edits the visual
// fixes (remaster/graphics/objectFixes.h) and saves them into the project,
// so the next build gives them to every player. It only exists in
// development builds (CMake option CLASSIC_REBORN_DEV_TOOLS); players' builds
// get empty functions.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Window thread (input.cpp): F10 opens or closes the editor (debug mode only)
void objectEditorToggle();

// The editor is open: the game is frozen and gets no input
bool objectEditorIsOpen();

// Inside the active ImGui frame (bgfxGlue.cpp, next to the F1 window)
void objectEditorDraw();

// Registers the freeze hook (remasterHooks.cpp)
void objectEditorRegister();
