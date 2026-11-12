///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Controller help for new players: with a game controller connected, the
// start menu shows a picture of its buttons (embedded ControllerHint.png)
// at the bottom of the play area. Always on. Replaces the remaster's version
// in menuBackgrounds.cpp.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Registers the start menu hook (remasterHooks.cpp)
void controllerHintRegister();

#include <imgui.h>
// The controls window's controller drawing (Shared/ui/controllerPicture.h)
// as an ImGui texture, made the first time; false if it could not be read
bool controllerDrawingTexture(ImTextureID* texture, float* aspect);
