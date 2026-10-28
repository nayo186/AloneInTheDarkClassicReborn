///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Custom backgrounds: export every camera background and full-screen picture
// (title, book pages, character choice, game over...) to PNG (the original
// 320x200 pixels, to enlarge with an AI) and use the player's pictures instead
// of the originals (graphics.customBackgrounds). Pictures go in
// backgrounds_custom/ with the exported names, any size:
// - 16:10 like the stored original (320x200, 640x400, 1280x800...): the whole
//   picture is the play area, shown in 4:3 as the game always did;
// - 4:3: the whole picture is the play area;
// - wider than 4:3 (e.g. 16:9): its 4:3 centre is the play area and the sides
//   fill the black bars of fullscreen.
// Foreground masks (what covers the characters) can be replaced too, so they
// fit objects the new picture moved or redrew: <picture>_mask_<room>_<mask>.png
// in backgrounds_custom/, white = covers, same size as the custom picture.
// "Export masks of this camera" writes the original ones with a guide.
// A picture or mask saved while playing is read again at once. Replaces the
// remaster's HD background code.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// Registers the core hooks (camera, full-screen picture) and the platform
// hooks the renderer asks (platformHooks.h: the picture on show, the masks,
// the screen and masks the game made, the check for saved files)
void customBackgroundsRegister();

// Exports every mask of the game (AITD1), read from the floors' camera data
// without entering the cameras: backgrounds_export/masks (320x200, white =
// covers, ready to enlarge in a batch) and backgrounds_export/masks_guides.
// Progress as the background export (0 idle, 1 working, 2 done, 3 failed)
void customBackgroundsExportMasksStart();
// State of the mask export (0 idle, 1 working, 2 done, 3 failed)
int customBackgroundsExportMasksState();
// Number of masks exported
int customBackgroundsExportMasksCount();

// An export is running: the game stays paused (the mask export uses the
// core's polygon filler, which is not reentrant)
bool customBackgroundsExportBusy();

// Exports every camera background in a background thread; progress through
// customBackgroundsExportState (0 idle, 1 working, 2 done, 3 failed)
void customBackgroundsExportStart();
// State of the background export (0 idle, 1 working, 2 done, 3 failed)
int customBackgroundsExportState();
// Number of backgrounds exported
int customBackgroundsExportCount();
