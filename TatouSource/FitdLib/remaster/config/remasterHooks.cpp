///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Remaster hooks: connects every remaster system to the platform and core
// hooks (see remasterHooks.h)
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "remasterHooks.h"
#include "coreHooks.h"
#include "settings.h"
#include "advancedOptions.h"
#include "customBackgrounds.h"
#include "controllerHint.h"
#include "debugCheats.h"
#include "objectEditor.h"
#include "objectFixes.h"
#include "menuFonts.h"
#include "platformHooks.h"

#include <SDL3/SDL.h>

// The menus on top of everything: F1 and, in development builds, F9 and F10
static void drawMenus()
{
    advancedOptionsDraw();
    debugCheatsDraw();
    objectEditorDraw();
}

// Testing only (debug mode): F9 the item window, F10 the object editor
static void keyPressed(int scancode)
{
    if (scancode == SDL_SCANCODE_F9)
        debugCheatsToggle();
    else if (scancode == SDL_SCANCODE_F10)
        objectEditorToggle();
}

// F1 opens and closes, except while the controls window waits for an input
// or the item window is open
static void optionsAction()
{
    if (!advancedOptionsCapturingInput() && !debugCheatsWindowOpen())
        advancedOptionsToggle();
}

// The open windows keep every input from the game (the object editor freezes
// it but keeps drawing the scene)
static bool menuCapturingInput()
{
    return advancedOptionsIsOpen() || debugCheatsWindowOpen() || objectEditorIsOpen();
}

void registerRemasterHooks()
{
    // Fonts of the menus (the launcher's, sharp at any window size)
    menuFontsLoad();

    g_onDrawMenus = drawMenus;
    g_onKeyPressed = keyPressed;
    g_onOptionsAction = optionsAction;
    g_isMenuCapturingInput = menuCapturingInput;

    g_isOptionsWindowOpen = advancedOptionsPausesGame;
    customBackgroundsRegister();
    controllerHintRegister();
    debugCheatsRegister();
    objectFixesRegister();    // every build: the visual fixes of 3D objects
    objectEditorRegister();   // development builds only

}
