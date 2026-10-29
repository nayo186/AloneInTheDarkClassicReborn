# Diccionario de métodos

Cada función pública del proyecto con para qué sirve, por capa y archivo. Las capas y los archivos se explican en `docs/MAPA_CODIGO.md`.

**Generado** por `tools/makeDictionary.py` a partir de los comentarios de las cabeceras (`.h`): no se edita a mano. Para cambiar una descripción, se cambia el comentario de la función y se vuelve a generar.

## util: Herramientas sueltas

### `util/exceptionHandler.h`

| Función | Para qué sirve |
| --- | --- |
| `Initialize` | Installs the crash handler (crash_log.txt and tatou.log) |
| `Shutdown` | Removes the crash handler |

### `util/jobSystem.h`

| Función | Para qué sirve |
| --- | --- |
| `instance` | The one job system of the program |
| `init` | Starts the threads: numIOThreads for files, numWorkerThreads for work (-1: by the processor) |
| `shutdown` | Stops the worker threads (waits for the running jobs) |
| `waitForJob` | Wait for job completion |
| `waitForAllJobs` | Waits until every job is done |
| `isJobComplete` | True once a job is done |
| `getCompletedJobCount` | Number of jobs done |
| `getPendingJobCount` | Number of jobs waiting |
| `processPendingCallbacks` | Process callbacks from main thread (call once per frame) |
| `getNumIOThreads` | Number of threads for files |
| `getNumWorkerThreads` | Number of worker threads |
| `isInitialized` | True once the worker threads are running |
| `dumpStats` | Writes the threads and job counts to tatou.log |
| `workerThreadMain` | The loop of a thread: takes jobs of its type and runs them |
| `enqueueJob` | Adds a job to the queue |
| `dequeueJob` | Takes the next job for a worker thread (false if none) |

### `util/logFile.h`

| Función | Para qué sirve |
| --- | --- |
| `logFileInit` | Redirects stdout/stderr (unbuffered) to tatou.log in the working directory. The previous log is kept as tatou.prev.log. Call once, as early as possible. |
| `logFileFlush` | Flushes stdout/stderr. Safe to call from crash handlers and at exit. |

### `util/platform_compat.h`

| Función | Para qué sirve |
| --- | --- |
| `ctime_s` | ctime_s shim |
| `fopen_s` | fopen_s shim |
| `strncpy_s` | strncpy_s shim |
| `strcpy_s` | strcpy_s shim |
| `strcat_s` | strcat_s shim |

### `util/safeDelete.h`

| Función | Para qué sirve |
| --- | --- |
| `SafeDeleteObject` | Deletes an object without crashing if its memory is already damaged; true if it went well |
| `SafeDeleteArray` | Deletes an array without crashing if its memory is already damaged; true if it went well |
| `GetStats` | Get statistics |
| `ResetStats` | Reset statistics |

### `util/unpack.h`

| Función | Para qué sirve |
| --- | --- |
| `PAK_deflate` | Decompresses a PAK entry stored with deflate (zlib) |
| `PAK_explode` | Decompresses a PAK entry stored with PKWARE's explode |

## platform: El sistema

### `platform/bgfxGlue.h`

| Función | Para qué sirve |
| --- | --- |
| `initBgfxGlue` | Creates the window and starts bgfx, ImGui and the screen effects |
| `StartFrame` | Start of a frame: window size, scene rectangle, ImGui frame, controller in the menus |
| `EndFrame` | End of a frame: draws the menus and the effects and shows the frame |
| `applyWindowAspectRatio` | Window mode: keeps the window in the shape of the aspect ratio (window thread) |
| `toggleFullscreen` | Switches between window and fullscreen |

### `platform/controls.h`

| Función | Para qué sirve |
| --- | --- |
| `controlsBindings` | The inputs of an action (starts with the defaults) |
| `controlsSetBindings` | Replaces the inputs of an action |
| `controlsResetDefaults` | Puts every action back to its default inputs |
| `controlsAnalog` | The sticks' settings (dead zone, sensitivity, inverted Y, modern left stick) |
| `controlsLeftStick` | The left stick as read at the last controlsUpdate |
| `controlsUpdate` | Each frame (input.cpp): sets JoyD, key and Click from the keyboard state and the controller (null when none is connected) |
| `controlsActionHeld` | Whether any input of the action is held now (GameAction::Options: input.cpp) |

### `platform/fileAccess.h`

| Función | Para qué sirve |
| --- | --- |
| `fatalError` | A game file could not be read: frees everything and exits with a message |
| `loadFromItd` | Loads a whole .ITD file into new memory |
| `CheckLoadMallocPak` | Loads an entry of a PAK; a fatal error if it fails |

### `platform/gameFolder.h`

| Función | Para qué sirve |
| --- | --- |
| `selectGameFolder` | Called first, before the log file opens (reads --game aitd1\|jack and --debug / --final) |
| `logGameFolder` | Writes to the log what selectGameFolder found (once the log file is open) |
| `isDebugMode` | Debug mode: test tools on; with a copy of the data, written into the copy |
| `configFolder` | Folder of aitd_remaster.cfg, with a trailing separator (empty: the working directory) |
| `customPicturesFolder` | The player's pictures, masks and the exports, relative to the working directory, with a trailing slash: Backgrounds/, Masks/, Exports/ in the portable folder, or backgrounds_custom/ (both) and backgrounds_export/ inside a copy in debug mode |
| `savesFolder` | Folder of the saves, relative to the working directory, with a trailing slash: Saves/, or empty (the copy itself) in debug mode with a copy |
| `customMasksFolder` | Folder of the player's masks (Masks, or backgrounds_custom in debug mode) |
| `exportsFolder` | Folder of the exported backgrounds and masks |
| `usesPortableFolders` | True when the working folder is UserData (outside the debug mode or with GOG/Steam) |
| `isGogInstall` | GOG installation: the CD image and its track list (empty for a copy) |
| `gogImagePath` | Path of the CD image (...\GAME.GOG) |
| `gogCuePath` | Path of the track list (...\GAME.INS on GOG, GAME.INST on Steam) |

### `platform/gogImage.h`

| Función | Para qué sirve |
| --- | --- |
| `gogImageAvailable` | True once GAME.GOG and GAME.INS were read (only for a GOG installation) |
| `gogImageReadFile` | A file of the CD, e.g. ("INDARK", "000123.VOC"); false if not there |
| `gogImageCreateTrack` | A music track of the CD (CD numbering: 2 = the first music track) as a SoLoud source that reads it from the image while it plays; null if there is no such track. The caller deletes it (after stopping it). |

### `platform/imguiBGFX.h`

| Función | Para qué sirve |
| --- | --- |
| `imguiRebuildFontTexture` | Rebuild font texture after adding fonts dynamically |
| `imguiBeginFrame` | Opens an ImGui frame (mouse, window size, view) |
| `imguiEndFrame` | Closes the ImGui frame and draws it with bgfx |

### `platform/input.h`

| Función | Para qué sirve |
| --- | --- |
| `readKeyboard` | Reads keyboard, mouse and controller each frame and turns them into the game's inputs |
| `controllerInUse` | The controller is the last thing used (a button, or a stick or trigger pushed past its dead zone): the mouse cursor is hidden until the mouse moves again (thread-safe) |
| `gameInputBlocked` | True while the game must not take any input: the F1 window (or the debug item window) is open, or it just closed and something that was held (the Esc, Enter, click or B that closed it) has not been released yet |
| `resetWindowResizeFlag` | Reset the window resize flag (call after handling resize) |
| `initController` | Opens the first controller connected (and the ones connected later) |
| `shutdownController` | Closes the controller |
| `handleControllerDeviceEvent` | A controller was connected or disconnected |

### `platform/osystem.h`

| Función | Para qué sirve |
| --- | --- |
| `osystem_init` | Starts the system layer: window, graphics, sound, controller and ImGui |
| `osystem_drawBackground` | Draws the 320x200 background (or the player's picture) at the back of the scene |
| `osystem_startOfFrame` | Start of a game frame: waits for the render thread and opens the frame |
| `osystem_endOfFrame` | End of a game frame: draws what is pending and the text layer, and shows the frame |
| `osystem_setPalette` | Sets the game palette (256 RGB colours) and clears the screen fade |
| `osystem_setPalette` | Sets the game palette (256 RGB colours) and clears the screen fade |
| `osystem_setDarkRoom` | Darkens the palette for a room without light (and back) |
| `osystem_drawDarkness` | Dark room (lightOff): covers the scene drawn so far with black; the text layer is drawn after it and the glass of this frame is skipped |
| `osystem_flip` | Draws the 3D primitives still pending |
| `osystem_CopyBlockPhys` | Sends a rectangle of the 320x200 screen to the graphics card |
| `osystem_playSampleFromName` | Plays a sound effect read from a file by its name |
| `osystem_playSample` | Plays a sound effect from memory (the game's VOC data) |
| `osystem_playLoopingSample` | Plays a sound effect in a loop (ambient sound; one at a time) |
| `osystem_stopSample` | Stops every sound effect |
| `osystem_stopLoopingSamples` | Stops the looping sound effects |
| `osystem_isSamplePlaying` | True while a sound effect is playing |
| `osystem_playVO` | Plays a narrator voice by its file name (loose file or GAME.GOG) |
| `osystem_playVocByIndex` | Plays the narrator voice of a book or message by its number |
| `osystem_playVocPage` | Plays the narrator voice of one page of a book |
| `osystem_stopVO` | Stops the narrator voice |
| `osystem_preDecodeVocPage` | VOC pre-decode cache for seamless page turns during book reading |
| `osystem_clearVocCache` | Forgets the voices decoded in the background |
| `osystem_findNarrators` | The narrators with voices: the folders of Voices in the working folder |
| `osystem_createMask` | Makes the texture of a foreground mask from its 320x200 shape |
| `osystem_drawMask` | Draws a foreground mask of the camera over the objects behind it |
| `osystem_startFrame` | Opens a frame of the renderer (creates its resources the first time) |
| `osystem_stopFrame` | Empty: the renderer needs nothing at the end of a frame |
| `osystem_setClip` | Sets the drawing clip rectangle (320x200 coordinates) |
| `osystem_clearClip` | Removes the drawing clip rectangle |
| `osystem_fillPoly` | Adds a filled polygon of a 3D model (plain, noise, ramp or glass by polyType) |
| `osystem_draw3dLine` | Adds a 3D line |
| `osystem_draw3dQuad` | Adds the four edges of a 3D quad as lines |
| `osystem_drawSphere` | Adds a sphere of a 3D model (a disc facing the camera) |
| `osystem_drawPoint` | Adds a point of a 3D model (a small disc facing the camera) |
| `osystem_flushPendingPrimitives` | Sends the polygons, spheres and lines of the frame to the graphics card |
| `osystem_drawUILayer` | Draws the text layer (uiLayer) over the scene |
| `osystem_playTrack` | Plays a CD music track (loose file or GAME.GOG) |
| `osystem_stopTrack` | Stops the CD music |
| `osystem_isTrackPlaying` | True while a CD music track (CD, loose file or GAME.GOG) is still playing |
| `osystem_playAdlib` | Starts the AdLib music output (Jack in the Dark) |
| `osystem_updateSceneSnapshot` | Scene preview capture for pause menu |
| `osystem_captureScenePreview` | Freezes the scene snapshot for the pause menu preview |
| `osystem_finalizeScenePreview` | Marks the pause menu preview as ready |
| `osystem_getScenePreviewData` | The pixels of the pause menu preview, or nullptr |
| `osystem_getScenePreviewWidth` | Width of the pause menu preview (0 if none) |
| `osystem_getScenePreviewHeight` | Height of the pause menu preview (0 if none) |
| `osystem_freezeSceneForMenu` | Freeze/unfreeze scene snapshot for menu backgrounds |
| `osystem_unfreezeSceneForMenu` | The scene snapshot follows the game again (menu closed) |
| `osystem_drawFrozenSceneBackground` | Draws the frozen scene behind a menu |
| `osystem_windowToGame` | Window position (pixels) to the 320x200 game screen, through the scene rectangle (graphics.aspect leaves black bars); false before the first frame |
| `osystem_getSceneRect` | Rectangle of the window (pixels) where the play area is drawn: x, y, width, height |
| `osystem_suspendScreenEffects` | Plain screens (title armadillo, intro screens, books) suspend the screen effects without changing the player's options; calls nest (BUGS.md C15) |
| `osystem_setDepthSlice` | Limits the depth drawn (-1: all), used to draw objects in front of or behind a mask |
| `osystem_depthSlicedThisFrame` | True if a depth slice was used this frame |
| `osystem_drawDeferredGlass` | Draws the glass polygons of the frame (Original mode), after the scene |
| `osystem_takeFrozenSceneDrawn` | True if the frozen image was drawn since the last call (screen effects reuse its ambient occlusion) |
| `osystem_isSceneSnapshotLive` | True while the scene snapshot follows the game (not frozen by a menu) |
| `osystem_canFadeScreen` | Brightness of the final image, 0 (black) to 1 (normal). Fades use it so the whole picture (background, models, masks) changes together. |
| `osystem_setScreenFade` | Sets the screen fade (0 black, 1 normal) |
| `osystem_setObjectStencilMode` | Kind of object being drawn, for the background-incrust rule: 0 = none, 1 = moving actor, 2 = incrusted (static) object. See rendererBGFX.cpp. |

### `platform/osystemAL.h`

| Función | Para qué sirve |
| --- | --- |
| `osystemAL_init` | Starts the sound engine (SoLoud) and its two volume groups |
| `osystemAL_udpate` | Applies the volumes and frees finished sounds (each frame) |
| `osystemAL_getSampleRate` | Sample rate of the sound output |

### `platform/platformHooks.h`

| Función | Para qué sirve |
| --- | --- |
| `g_onGraphicsReady` | bgfx and ImGui are ready (end of initBgfxGlue). The program (Fitd/fitd.cpp) points it to registerRemasterHooks, which loads the menu fonts and fills the other hooks. |
| `g_onDrawMenus` | Inside the ImGui frame, after everything else: the menus drawn on top (F1 and the debug windows) |
| `g_onKeyPressed` | A key with no meaning of its own was pressed (SDL scancode): F9, F10... |
| `g_onOptionsAction` | The "Options window" action (F1, Home, Back by default) was pressed |
| `g_isMenuCapturingInput` | True while a menu owns keyboard, mouse and controller: the game receives no input |
| `g_onFrameStart` | Each frame, before drawing: pictures saved or removed are picked up |
| `g_getBackgroundPicture` | The picture for the screen on show; false: the game's own background |
| `g_getMaskPicture` | The player's picture of a mask of the game (invalid handle: none); originalShape: 16:10 like the scene, otherwise the shape of the picture |
| `g_onScreenUploaded` | The 320x200 screen was sent to the graphics card |
| `g_onMaskCreated` | A mask of the game was made (its 320x200 shape and rectangle) |

### `platform/rendererBGFXInternal.h`

| Función | Para qué sirve |
| --- | --- |
| `submitImageBackground` | Submits an RGBA picture quad (imageBackground_ps, with the fade level) |

### `platform/screenEffects.h`

| Función | Para qué sirve |
| --- | --- |
| `init` | Creates the offscreen images and the effects' programs for a scene of width x height |
| `shutdown` | Deletes the offscreen images and programs |
| `resize` | Remakes the offscreen images for a new scene size |
| `beginScene` | The game draws the scene (view 0) into the offscreen image |
| `endScene` | Applies the enabled effects and draws the result on the screen. modelsDrawn: 3D models were drawn this frame (ambient occlusion from their depth). frozenImageDrawn: the frame shows the last image again (BUGS.md V14, menus): its ambient occlusion is reused. Neither: none. |
| `suspend` | Screens that must look plain (title armadillo, intro screens, books) suspend the effects; nested calls are counted. The player's options in g_settings are never changed for this (they were, and a save or the F1 window during those screens lost them, BUGS.md C15). |
| `setScreenFade` | Brightness of the final image, 0 (black) to 1 (normal); used by fades |
| `getMainColorTexture` | The offscreen image of the scene |
| `getSceneFrameBuffer` | The framebuffer the scene is drawn into |
| `getSceneCopy` | Copy of the scene the glass reads (rendererBGFX.cpp, osystem_drawDeferredGlass) |
| `getWidth` | Width of the scene image |
| `getHeight` | Height of the scene image |
| `createTargets` | Creates the offscreen images (scene, copies, bloom levels, SSAO) |
| `destroyTargets` | Deletes the offscreen images |
| `renderAmbientOcclusion` | false when there is no camera yet |
| `renderAntialiasing` | FXAA on the 3D models: m_sceneColor -> m_aaColor |
| `renderBloom` | Makes the bloom: bright parts of the painted backgrounds, blurred down and up |
| `renderComposite` | Copies the scene to the screen with the chosen effects and the fade |

### `platform/settings.h`

| Función | Para qué sirve |
| --- | --- |
| `loadSettings` | Reads aitd_remaster.cfg (defaults for anything missing) |
| `saveSettings` | Writes the options and the controls to aitd_remaster.cfg |

## core: El juego de 1992

### `core/AITD1.h`

| Función | Para qué sirve |
| --- | --- |
| `startAITD1` | AITD1's start: introduction screens, start menu, character choice and the game |
| `AITD1_ReadBook` | Opens a book or letter of AITD1 with its picture and narrator voice |

### `core/AITD2.h`

| Función | Para qué sirve |
| --- | --- |
| `startAITD2` | AITD2's start (inherited from FITD, not played yet) |
| `drawInventoryAITD2` | Draws AITD2's inventory frame |
| `redrawInventorySpriteAITD2` | Redraws the picture of AITD2's inventory |
| `AITD2_ReadBook` | Opens a book or letter of AITD2 |

### `core/AITD3.h`

| Función | Para qué sirve |
| --- | --- |
| `startAITD3` | AITD3's start (inherited from FITD, not played yet) |
| `drawInventoryAITD3` | Draws AITD3's inventory frame |

### `core/JACK.h`

| Función | Para qué sirve |
| --- | --- |
| `startJACK` | Jack in the Dark's start: the game from its first floor and room |
| `JACK_ReadBook` | Opens a book or letter of Jack in the Dark |

### `core/actorList.h`

| Función | Para qué sirve |
| --- | --- |
| `sortActorList` | Puts Index[0..NbAffObjets) in drawing order, far to near (BUGS.md V22) |

### `core/aitdBox.h`

| Función | Para qué sirve |
| --- | --- |
| `AffBigCadre` | Draws a text box with its frame centred at (x, y) and sets the text area inside it |

### `core/anim.h`

| Función | Para qué sirve |
| --- | --- |
| `InitAnim` | Starts an animation on the current object (once, repeated or all once; animInfo: the next one); false if it cannot |
| `SetAnimObjet` | Puts a body in the pose of one frame of an animation |
| `SetInterAnimObjet` | Puts a body in the pose between two frames by the time passed; returns 1 when the frame ends |
| `GetNbFramesAnim` | Number of frames of an animation |
| `StockInterAnim` | Keeps the current pose of a body as the start of the next interpolation |
| `ResetStartAnim` | Restarts the interpolation timer of a body |
| `GereAnim` | Advances the animation of the current object and moves it by its animation step |
| `InitCopyBox` | Sets the screens used to copy rectangles (the original's screen pointers) |

### `core/anim2d.h`

| Función | Para qué sirve |
| --- | --- |
| `resetAnim2D` | Stops every 2D animation |
| `load2dAnims` | Loads the 2D animations of a camera (ANIMnn.PAK) |
| `startAnim2d` | Starts a 2D animation |
| `handleAnim2d` | Advances and draws the 2D animations |

### `core/animAction.h`

| Función | Para qué sirve |
| --- | --- |
| `GereFrappe` | Carries out the action of the current object's animation at its frame: hit, shot or throw |

### `core/cheats.h`

| Función | Para qué sirve |
| --- | --- |
| `applyCheats` | After each object's script (mainLoop.cpp): puts back the life a hit took |

### `core/common.h`

| Función | Para qué sirve |
| --- | --- |
| `getCVarsIdx` | Position of a game variable (CVar of DEFINES.ITD) in the CVars table |
| `getCVarsIdx` | Position of a game variable (CVar of DEFINES.ITD) in the CVars table |

### `core/coreHooks.h`

| Función | Para qué sirve |
| --- | --- |
| `g_onCameraBackgroundReady` | A camera background was loaded: the picture (320x200 palette indexes) and where it comes from (PAK name and index). Custom backgrounds use it. |
| `g_onScreenPictureShown` | A full-screen picture (title, book page, character choice...) is about to be shown; the core loads and draws it as always. Custom backgrounds use it. |
| `g_onStartMenuDrawn` | A frame of the start menu was drawn (the controller help goes over it). |
| `g_onGameLoopTick` | Once per loop of the game, on the game thread, before the objects' scripts (debug cheats act here). |
| `g_onObjectDraw` | An object of the camera is about to be drawn at this position (world units), rotation (1024 = a full turn) and size (renderer.h); listIndex is its place in ListObjets. A listener may change how it is drawn (a visual fix: the game keeps the real position) and returns false to skip drawing it this frame. |
| `g_isGameFrozenLive` | True while a tool freezes the game but needs the scene drawn every frame (object editor): PlayWorld then only redraws, with the clock stopped. |
| `g_isOptionsWindowOpen` | True while an options window over the game is open (F1): the game waits. |

### `core/debugger.h`

| Función | Para qué sirve |
| --- | --- |
| `debugger_draw` | Draws FITD's debug menu and windows (Debug builds; empty elsewhere) |
| `debugger_draw` | Draws FITD's debug menu and windows (Debug builds; empty elsewhere) |

### `core/floor.h`

| Función | Para qué sirve |
| --- | --- |
| `LoadEtage` | Loads a floor (ETAGEnn.PAK): its rooms, cameras and zones |

### `core/font.h`

| Función | Para qué sirve |
| --- | --- |
| `SetFont` | Chooses the font and the colour to write with |
| `SetFontSpace` | Sets the space between words and between letters |
| `ExtGetSizeFont` | Width in pixels of a text with the current font |
| `PrintFont` | Writes a text at (x, y) on a 320x200 surface |
| `SelectedMessage` | Writes text number `index` highlighted (selected menu entry) |
| `SimpleMessage` | Writes text number `index` at (x, y) in a colour |

### `core/gameTime.h`

| Función | Para qué sirve |
| --- | --- |
| `SaveTimerAnim` | Stops the game clock (menus, books); calls nest |
| `RestoreTimerAnim` | Lets the game clock run again from where it stopped |
| `setGameClock` | Sets the game clock (loading a game) |
| `gameClockForSave` | The game clock to save: the time it stopped at if it is stopped |

### `core/hqr.h`

| Función | Para qué sirve |
| --- | --- |
| `HQR_Get` | Entry `index` of a resource list, loaded from its PAK if it is not in memory yet |
| `HQR_Init` | Creates an empty list of memory blocks (size in bytes, number of entries) |
| `HQ_Malloc` | Reserves a block of `size` bytes in a list; returns its number |
| `HQ_PtrMalloc` | The memory of block number `index` |
| `HQ_Free_Malloc` | Frees block number `index` |
| `HQR_Free` | Empties and deletes a resource list |
| `HQR_InitRessource` | Creates a resource list read from a PAK (bodies, animations...) |
| `HQR_Reset` | Empties a resource list (everything is loaded again when asked for) |
| `configureHqrHero` | Changes the PAK a resource list reads from (the hero's bodies) |
| `createBodyFromPtr` | Builds a body (3D model) from its data in a PAK entry |

### `core/hybrid.h`

| Función | Para qué sirve |
| --- | --- |
| `readEntites` | Reads the entities of a hybrid object (AITD2 and later) |
| `readSprites` | Reads the sprites of a hybrid object |
| `readAnimations` | Reads the animations of a hybrid object |
| `AffHyb` | Draws entity `index` of a hybrid object at (X, Y) |

### `core/inventory.h`

| Función | Para qué sirve |
| --- | --- |
| `processInventory` | The inventory screen: choose an object and an action |

### `core/launchOptions.h`

| Función | Para qué sirve |
| --- | --- |
| `parseLaunchOptions` | Reads the parameters above; unknown parameters are left to other parsers. |
| `applyLaunchOptions` | Applies the options: game selection and language. Call after loadSettings() and before the graphics and game data are set up. |

### `core/main.h`

| Función | Para qué sirve |
| --- | --- |
| `menuWaitVSync` | Empty: the original waited for the screen refresh between menu frames |
| `AffSpfI` | Draws sprite number `param` of `gfx` at (x, y), skipping its colour 0 (transparent) |
| `GereDec` | The scenery zones the current object stands in: room change, floor change and other zone triggers |
| `executeFoundLife` | Runs the script of an object when it is found ("found" life of the world object) |
| `resetFoundObjectChronos` | The chronometers of the objects run from the hand start again (after a load) |
| `InitView` | Sets up the view of the current camera: projection, angles, position, visible rooms and the objects to draw |
| `GereSwitchCamera` | Switches camera when the followed object enters another camera's zone |
| `GenereActiveList` | Builds the list of active objects (those in the current room or the rooms seen from the camera) |
| `GenereAffList` | Builds the list of objects to draw (NbAffObjets, Index) and puts it in drawing order |
| `AllRedraw` | Redraws the whole screen: background, objects, masks and texts |
| `CheckObjectCol` | Finds the objects whose collision box touches zvPtr (up to 3); returns how many |
| `checkLineProjectionWithActors` | Moves a box along a line from (X, Y, Z) at angle beta and returns the first object it hits (-1 none, -2 a wall) |
| `AsmCheckListCol` | Counts the fixed collision boxes of the room that touch zvPtr (fills hardColTable) |
| `DeleteInventoryObjet` | Takes an object out of the inventory |
| `walkStep` | Turns a step of animation (animMoveX/Z) by the object's angles |
| `throwStoppedAt` | Places a thrown object where it stopped, on the floor near (x, z) |
| `processActor2Sub` | The scenery zone (sceZone) at (x, y, z) in the room, or nullptr |
| `LoadWorld` | Loads the world objects (OBJETS.ITD): every object of the game with its starting state |
| `initVars` | Resets the game variables for a new game (inventory, flags, game over...) |
| `flushScreen` | Clears the 320x200 screen and the text layer |
| `parseAllSaves` | Save and load screen: lists the slots with their date and returns the chosen one (-1 none) |
| `getTextFromIdx` | The text entry number `index` of the language's texts |
| `cleanupAndExit` | Closes everything (controller, sound, music, graphics) and ends the program |
| `drawProjectedBox` | Debug: draws a 3D box as six projected faces |
| `playSequence` | Plays an animated sequence (cinematic) with its fades |
| `fileExists` | True if the file exists (path as given) |
| `dataFileExists` | in the game data folder (homePath) |
| `OpenProgram` | Program start of the original: screen, resources, texts, palette |
| `freeAll` | Frees the game's resource lists before exiting |
| `startGame` | Starts a game on the given floor and room: loads the world, the floor and runs the main loop |
| `AffRect` | fast recode. No RE |
| `CopyZV` | Copies a collision box |
| `AdjustZV` | Moves a collision box from the coordinates of one room to those of another |
| `take` | Puts a world object in the inventory (as the script instruction TAKE) |
| `FoundObjet` | An object was found: shows it and asks the player to take it or not |
| `hit` | Starts a hit: plays the animation and sets the frame, range and force of the blow |
| `CubeIntersect` | True if two collision boxes overlap |
| `PutAtObjet` | Puts an object at another object's position (room, floor and coordinates) |
| `SetClip` | Sets the drawing clip rectangle on the 320x200 screen |
| `drawZv` | Debug: draws a collision box as projected lines |
| `drawZv` | Debug builds (FITD_DEBUGGER): an object's box and a room's zones (debugDraw.cpp) |
| `drawSceZone` | Debug: draws the scenery zones of a room |
| `drawHardCol` | Debug: draws the fixed collision boxes of a room |
| `readBook` | Opens a book or letter: picture, pages and narrator voice |
| `Lire` | The reader of books and documents: shows the pages of text `index` in the rectangle and turns them |
| `SetProjection` | Sets the camera projection: screen centre, perspective and field of view |
| `SetAngleCamera` | Sets the camera angles (1024 steps per turn) |
| `SetPosCamera` | Sets the camera position |
| `GiveDistance2D` | Distance between two points on the floor plane (x, z) |
| `InitDeplacement` | Sets how the current object moves: still, controlled by the player, following an object or a track |
| `InitRealValue` | Starts a value that goes from beta to newBeta in `param` ticks (rotations, speeds) |
| `updateActorRotation` | The current value of an interpolated value (InitRealValue) |
| `deleteObject` | Removes a world object from the room (and from the screen) |
| `removeFromBGIncrust` | Takes an object out of the background (it is drawn as a moving object again) |
| `makeMessage` | Shows message number messageIdx at the bottom of the screen |
| `drawTextOverlay` | Draws the messages at the bottom of the screen and keeps them for their time |
| `GereCollision` | Corrects a move so the object does not go into a fixed collision box (slides along it) |
| `addActorToBgInscrust` | Pastes an object into the background (drawn once, not every frame) |
| `setupPointRotate` | Sets the angles for pointRotate |
| `pointRotate` | Turns a point by the angles of setupPointRotate |
| `Open` | Opens a file of the game data folder (homePath) |
| `isInViewList` | True if the room is seen from the current camera |
| `loadCamera` | Loads the camera picture (CAMERAnn.PAK) into the background |
| `loadMask` | Loads the foreground masks of a camera (Jack and later: MASKnn.PAK) |
| `createAITD1Mask` | Builds the foreground masks of the current camera (AITD1: polygons of the camera data) |
| `InitViewedRoomList` | Builds the list of rooms seen from the current camera |
| `DeleteObjet` | Removes an active object from the list of objects |
| `loadPalette` | Loads the game palette (ITD_RESS) |
| `isBgOverlayRequired` | True if a box on the floor (X1..X2, Z1..Z2) is inside one of the mask zones: the mask must be drawn over the object |

### `core/mainLoop.h`

| Función | Para qué sirve |
| --- | --- |
| `PlayWorld` | The main loop of a game: each frame runs objects, scripts, cameras and rooms and draws, until the game ends |
| `waitForOptionsWindow` | The game waits while the F1 window is open (called by process_events) |
| `rememberDarkRoom` | Rooms found dark (LM_LIGHT): entering one again is dark from its first frame, before its scripts run again (BUGS.md V23) |
| `roomAssumedDark` | True if the current room is dark (no lamp light) |

### `core/modernStick.h`

| Función | Para qué sirve |
| --- | --- |
| `updateModernStick` | Each frame, after localJoyD is read (mainLoop.cpp): while the left stick steers the player, turns its tank directions into "forward" or a turn on the spot towards the stick, for the scripts' animations |
| `modernStickTurn` | In processTrack for the player under manual control: turns it towards the stick and returns true, or returns false when the stick is not steering (then the tank controls turn it) |

### `core/music.h`

| Función | Para qué sirve |
| --- | --- |
| `initMusicDriver` | Starts the AdLib music driver (Jack in the Dark) |
| `musicUpdate` | Fills a sound buffer with the AdLib emulation (called by the mixer) |
| `playMusic` | Plays a music: CD track (AITD1) or AdLib (Jack) |
| `callMusicUpdate` | Advances the AdLib music by one step |
| `destroyMusicDriver` | Stops the AdLib emulation |
| `fadeMusic` | Fades the music in or out |
| `stopMusic` | Silences the music, CD track and AdLib alike (BUGS.md C33) |
| `isMusicOver` | True when the music that was playing has ended (CD track or AdLib) |

### `core/object.h`

| Función | Para qué sirve |
| --- | --- |
| `InitObjet` | Creates an active object with its body, animation, collision box and position; returns its number |

### `core/pak.h`

| Función | Para qué sirve |
| --- | --- |
| `loadPak` | Loads an entry of a PAK into new memory (decompressed) |
| `LoadPak` | Loads an entry of a PAK into the given memory |
| `getPakSize` | Size of an entry of a PAK once decompressed |
| `PAK_getNumFiles` | Number of entries of a PAK |

### `core/palette.h`

| Función | Para qué sirve |
| --- | --- |
| `paletteFill` | Fills a palette with one colour |
| `setPalette` | Sets a palette on screen |
| `copyPalette` | Copies a palette |
| `copyPalette` | Copies a palette |
| `convertPaletteIfRequired` | Converts the palette of Jack and later games from 6-bit to 8-bit colours |
| `SetLevelDestPal` | Makes the palette `coef`/256 of the way to black (fades) |

### `core/renderer.h`

| Función | Para qué sirve |
| --- | --- |
| `transformPoint` | Turns and moves a point by the camera |
| `AffObjet` | Draws a 3D model at a position and angles: projects it and sends its polygons to the renderer |
| `setNextObjectSize` | Size of the next object AffObjet draws; put back ObjectSize() afterwards |
| `setCurrentBodyNum` | The body about to be drawn (for the rules of special bodies) |
| `computeScreenBox` | Projects a model and computes the screen rectangle it covers (BBox3D) |

### `core/room.h`

| Función | Para qué sirve |
| --- | --- |
| `getRoomData` | The raw data of a room of the current floor |
| `ChangeSalle` | Enters another room of the floor: room data, cameras that see it, object positions relative to it |
| `getNumberOfRoom` | Number of rooms of the current floor |

### `core/save.h`

| Función | Para qué sirve |
| --- | --- |
| `saveFilePath` | Path of a save file of slot `slot` (".ITD", or ".png" for its picture), in the saves folder |
| `loadSave` | Reads a save file (both the GOG layout and older ones) |
| `restoreSave` | The load screen: chooses a slot and loads it |
| `makeSave` | The save screen: chooses a slot and writes the game (GOG layout for AITD1) |

### `core/screen.h`

| Función | Para qué sirve |
| --- | --- |
| `setupScreen` | Creates the 320x200 screen buffers |

### `core/sprite.h`

| Función | Para qué sirve |
| --- | --- |
| `read` | Reads the sprites of a hybrid object's data |
| `AffSpr` | Draws a sprite at (X, Y) on a screen |

### `core/startupMenu.h`

| Función | Para qué sirve |
| --- | --- |
| `DrawMenu` | Draws the start menu with an entry selected |
| `MainMenu` | The start menu: new game, load or quit; returns the choice |

### `core/systemMenu.h`

| Función | Para qué sirve |
| --- | --- |
| `processSystemMenu` | The 1992 pause menu: continue, save, load, music, sound, detail, quit |
| `scaleDownImage` | Shrinks a 320x200 picture into the save preview box |

### `core/tatou.h`

| Función | Para qué sirve |
| --- | --- |
| `make3dTatou` | AITD1's start screen: the turning 3D armadillo (Tatou) until a key is pressed |
| `process_events` | Between two game frames: shows the frame, starts the next one, advances the game clock and waits while F1 is open |
| `FastCopyScreen` | Copies a whole 320x200 screen |
| `Rotate` | Turns the point (y, z) by angle x (1024 steps per turn) |
| `FadeInPhys` | Fades the screen in from black |
| `FadeOutPhys` | Fades the screen out to black |
| `playSound` | Plays sound effect number `num` (LISTSAMP) |
| `playSoundLooping` | Plays sound effect number `num` in a loop |
| `playSoundThen` | Plays a sound effect and then another one |
| `updateNextSample` | Starts the sound effect waiting after the current one (playSoundThen) |
| `setCameraTarget` | Moves the camera towards a position and angles in `time` ticks |
| `startChrono` | Starts a chronometer (keeps the current time) |
| `evalChrono` | Ticks passed since startChrono |

### `core/track.h`

| Función | Para qué sirve |
| --- | --- |
| `processTrack` | Moves the current object by its movement mode: player controls, following an object or a track |
| `GereManualRot` | Turns the player with the left and right controls |

### `core/vars.h`

| Función | Para qué sirve |
| --- | --- |
| `readNextArgument` | Reads the next 16-bit value of the running script |
| `updateShaking` | Advances the screen shake |
| `stopShaking` | Stops the screen shake |
| `setupShaking` | Starts a screen shake (script instruction SHAKING) |
| `pauseShaking` | Removes the shake offset while paused |

### `core/zv.h`

| Función | Para qué sirve |
| --- | --- |
| `getZvCube` | Collision box of a body as a cube (the largest side on every axis) |
| `GiveZVObjet` | Collision box of a body as stored in its data |
| `getZvMax` | Collision box of a body big enough for any turn |
| `getZvRot` | Collision box of a body turned by its angles |
| `makeDefaultZV` | A default collision box (200 x 200 x 200 around the origin) |

## scripts: La lógica del juego

### `scripts/evalVar.h`

| Función | Para qué sirve |
| --- | --- |
| `evalVar` | Reads and evaluates the next value of the running script (AITD1 and Jack) |
| `evalVar2` | Reads and evaluates the next value of the running script (AITD2 and later) |

### `scripts/life.h`

| Función | Para qué sirve |
| --- | --- |
| `processLife` | Runs one frame of script number lifeNum for the current object (the interpreter) |
| `InitSpecialObjet` | Creates a special object (shot, thrown object, blood...) at a position |
| `animMove` | Chooses the walking, running, turning or still animation from the controls |
| `doRealZv` | Computes an object's collision box from its body in its current pose |
| `setStage` | Moves the current object to another floor and room |
| `fire` | Shoots: animation, frame of the shot and the hit it does |
| `put` | Puts a world object at a position (room, floor, angles) |
| `getHardClip` | The fixed collision box the current object touches |
| `throwObj` | Throws an object: animation, frame of the throw and the object's flight |
| `appendFormated` | Adds formatted text to the script trace (debug.logLifeScripts) |

### `scripts/lifeSafety.h`

| Función | Para qué sirve |
| --- | --- |
| `checkObjectAfterLifeOpcode` | Puts back a valid animation, body, frame or speed after a script instruction |

### `scripts/native/nativeLife.h`

| Función | Para qué sirve |
| --- | --- |
| `getNativeLifeScript` | Look up whether a native override exists for the given life script number. Returns the function pointer if registered, or nullptr if bytecode should be used. |
| `getNativeLifeScriptCount` | Returns the total number of registered native life script overrides. |
| `initNativeLifeScripts` | Initialize all native life script registrations (call during startup). |
| `dumpLifeScript` | Dump a life script's bytecode to console as readable pseudo-C. If lifeNum is -1, dumps all life scripts. |
| `dumpAllLifeScripts` | Dump all life scripts to console. |
| `generateNativeLifeScriptC` | Generate compilable C code for a single life script using nativeLifeHelpers.h |
| `generateAllNativeLifeScripts` | Generate compilable C code for all life scripts to nativeLifeScripts_generated.cpp |
| `validateNativeLifeScriptsAgainstDump` | Validate registered native scripts against a LISTLIFE_dump.txt reference file. Any script whose live decompilation differs from the reference is removed from the registry so the bytecode interpreter runs instead. |
| `registerNativeLifeScriptIfSafe` | Register a native C function only if the script contains no risky opcodes (animations, rotations, movement, combat, spatial changes, etc.). Safe scripts are pure logic/branch/variable/flag/message/audio scripts. |

## remaster: Extras propios

### `remaster/config/debugCheats.h`

| Función | Para qué sirve |
| --- | --- |
| `debugCheatsToggle` | Window thread (input.cpp): F9 opens or closes the item window |
| `debugCheatsWindowOpen` | The item window is open: the game waits and gets no input |
| `debugCheatsDraw` | Inside the active ImGui frame (bgfxGlue.cpp, next to the F1 window) |
| `debugCheatsRegister` | Registers the game loop hook (remasterHooks.cpp) |

### `remaster/config/remasterHooks.h`

| Función | Para qué sirve |
| --- | --- |
| `registerRemasterHooks` | Connects the remaster to the engine: menu fonts, platform hooks (menus, keys) and core hooks. Called once bgfx and ImGui are ready (g_onGraphicsReady, set by Fitd/fitd.cpp). |

### `remaster/graphics/customBackgrounds.h`

| Función | Para qué sirve |
| --- | --- |
| `customBackgroundsRegister` | Registers the core hooks (camera, full-screen picture) and the platform hooks the renderer asks (platformHooks.h: the picture on show, the masks, the screen and masks the game made, the check for saved files) |
| `customBackgroundsExportMasksStart` | Exports every mask of the game (AITD1), read from the floors' camera data without entering the cameras: backgrounds_export/masks (320x200, white = covers, ready to enlarge in a batch) and backgrounds_export/masks_guides. Progress as the background export (0 idle, 1 working, 2 done, 3 failed) |
| `customBackgroundsExportMasksState` | State of the mask export (0 idle, 1 working, 2 done, 3 failed) |
| `customBackgroundsExportMasksCount` | Number of masks exported |
| `customBackgroundsExportBusy` | An export is running: the game stays paused (the mask export uses the core's polygon filler, which is not reentrant) |
| `customBackgroundsExportStart` | Exports every camera background in a background thread; progress through customBackgroundsExportState (0 idle, 1 working, 2 done, 3 failed) |
| `customBackgroundsExportState` | State of the background export (0 idle, 1 working, 2 done, 3 failed) |
| `customBackgroundsExportCount` | Number of backgrounds exported |

### `remaster/graphics/objectFixes.h`

| Función | Para qué sirve |
| --- | --- |
| `objectFixesRegister` | Loads the fixes of the running game and registers the draw hook (remasterHooks.cpp) |
| `objectFixesMutex` | Guards everything below (the draw hook runs on the game thread) |
| `objectFixes` | Every visual fix of the current game |
| `objectFixIsEmpty` | True if a fix changes nothing |
| `findObjectFix` | The fix of an object (by floor and world index, or floor, room and body), or nullptr |
| `objectFixesSeen` | The objects drawn lately, by their place in ListObjets (NUM_MAX_OBJECT) |
| `objectFixesSetBlinking` | The object (place in ListObjets) that blinks to be found, -1 none |
| `objectFixesSave` | Writes the fixes into the project's file for this game (the next build compiles them in); returns false if it cannot |
| `objectFixesFileName` | Name of the fixes file of the current game (aitd1.cfg or jack.cfg) |

### `remaster/ui/advancedOptions.h`

| Función | Para qué sirve |
| --- | --- |
| `advancedOptionsIsOpen` | Thread-safe: F1 arrives on SDL's event thread, the window is drawn on the game/render thread |
| `advancedOptionsPausesGame` | The game waits (g_isOptionsWindowOpen): the window is open or an export started from it is still running |
| `advancedOptionsToggle` | Opens or closes the F1 window |
| `advancedOptionsCapturingInput` | The controls window is waiting for a new input (thread-safe) |
| `advancedOptionsDraw` | Called inside the active ImGui frame, just before imguiEndFrame() |

### `remaster/ui/controllerHint.h`

| Función | Para qué sirve |
| --- | --- |
| `controllerHintRegister` | Registers the start menu hook (remasterHooks.cpp) |

### `remaster/ui/menuFonts.h`

| Función | Para qué sirve |
| --- | --- |
| `menuFontsLoad` | Adds the fonts to ImGui's atlas and rebuilds its texture (after imguiCreate) |
| `menuFonts` | The fonts and the scale they were made at (1 if not loaded: ImGui's font) |
| `menuFontsScale` | Scale the menu fonts were made at |

### `remaster/ui/objectEditor.h`

| Función | Para qué sirve |
| --- | --- |
| `objectEditorToggle` | Window thread (input.cpp): F10 opens or closes the editor (debug mode only) |
| `objectEditorIsOpen` | The editor is open: the game is frozen and gets no input |
| `objectEditorDraw` | Inside the active ImGui frame (bgfxGlue.cpp, next to the F1 window) |
| `objectEditorRegister` | Registers the freeze hook (remasterHooks.cpp) |

## Shared: Compartido por el juego y el launcher

### `Shared/cdImage.h`

| Función | Para qué sirve |
| --- | --- |
| `path` | — |
| `fileCount` | — |
| `tracks` | — |
| `sectorSize` | Bytes of one raw sector (2352) or 2048 for an .iso: where a track starts |

### `Shared/narrators.h`

| Función | Para qué sirve |
| --- | --- |
| `name` | — |
| `imagePath` | The image the voices come from, or empty (loose files) |

### `Shared/ui/launcherScreen.h`

| Función | Para qué sirve |
| --- | --- |
| `u` | Size in pixels of `units` of the screen design |

### `Shared/ui/menuBase.h`

| Función | Para qué sirve |
| --- | --- |
| `px` | Size in pixels of interface units (scaled to the screen) |
| `px` | Size in pixels of interface units (scaled to the screen) |
| `portrait` | True for the vertical layout (a tall window or a phone) |
| `pressHeight` | Height of anything pressed: the given one, never smaller than a finger after a touch |

## Launcher: Launcher.exe

### `Launcher/gameCover.h`

| Función | Para qué sirve |
| --- | --- |
| `loadGameCover` | game: 0 = AITD1, 1 = AITD2, 2 = AITD3 |
| `freeGameCover` | Frees the textures of a cover |

### `Launcher/launcherSettings.h`

| Función | Para qué sirve |
| --- | --- |
| `launcherFolder` | Folder that contains Launcher.exe (and Tatou.exe), UTF-8, with trailing separator |
| `logOpen` | Opens launcher.log next to Launcher.exe |
| `logLine` | Writes a line to launcher.log |
| `loadLauncherSettings` | Reads launcher.cfg |
| `saveLauncherSettings` | Writes launcher.cfg |
| `isAitd1DataFolder` | The chosen folder can be a copy of the data (LISTBOD2.PAK in it, Jack in its JACK subfolder) or a GOG or Steam installation (GAME.GOG and GAME.INS, or GAME.INST on Steam; AITD1 in INDARK, Jack in JACK; the INDARK folder itself may be chosen too). A GOG installation is never written to: aitd_remaster.cfg goes to the user folder (Shared/userFolder.h), as the game's saves and log. |
| `isGogInstallFolder` | True if the folder is a GOG or Steam installation |
| `hasJackData` | True if Jack in the Dark is there too (PERE.PAK) |
| `findAitd1Narrators` | The narrators with voices in AITD1's working folder (Voices: in UserData\AITD1, or in the copy itself in debug mode; Shared/narrators.h) |
| `setLauncherDebugMode` | Debug mode (launcher.cfg "debugMode = 1", only by hand): with a copy of the data, the game writes inside the copy; otherwise everything goes to UserData next to the launcher (Shared/userFolder.h) |
| `prepareUserData` | Creates UserData with each game's Saves, Backgrounds, Masks and Exports folders (and AITD1's Voices); called when the launcher opens |
| `findGameLanguages` | Language PAKs present in the data folder, in a fixed order |
| `readGameConfig` | Language and advanced options of the data folder's aitd_remaster.cfg |
| `writeGameConfig` | Writes the launcher's options in the game's aitd_remaster.cfg, keeping every other line |
| `launchGame` | Starts Tatou.exe with the default settings and the chosen language, in the AITD1 data folder: game "aitd1", or "jack" (Tatou enters the JACK subfolder and shares the AITD1 configuration) |
