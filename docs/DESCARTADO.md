# Código y documentos descartados

Qué se borró, para qué servía y por qué ya no hace falta en este proyecto.

Casi todo lo de esta lista viene de Alone in the Dark: Re-Haunted (https://github.com/spacefarergames/AloneInTheDarkReHaunted), el remaster de Spacefarer Retro Remasters del que nace este proyecto, y de FITD, de yaz0r. Gracias a su trabajo este proyecto pudo existir. Si algo se quitó no es por su calidad: este proyecto tiene un objetivo más acotado (el juego de 1992 con sus gráficos originales) y mantiene solo lo que usa, para que el código sea fácil de entender. Varias de esas ideas (efectos de imagen, ventana de opciones, fondos de mayor resolución) siguen aquí, rehechas con el mismo espíritu.

Lo borrado sigue en el historial de Git del proyecto y en el repositorio de Re-Haunted.

## Efectos de imagen del remaster (2026-10-25, BUGS.md C13)

- **`remaster/graphics/postProcessing.cpp/.h`** y sus shaders (`postprocess_vs`, `brightpass_ps`, `blur_ps`, `composite_ps`, `ssao_ps`, `ssao_blur_ps`, `ssgi_ps`, `ssgi_blur_ps` y los duplicados sin usar `fs_*.sc`, `selective_noise_ps.sc`): bloom, grano, SSAO, SSGI y corrección de color del remaster. Sustituidos por versiones propias en `remaster/graphics/screenEffects.cpp`. Los viejos tenían fallos: el grano iba más rápido o más lento según los FPS, el SSAO leía mal la profundidad y "hervía", y en las imágenes repintadas (V14) desaparecía.
- **`remaster/graphics/lightProbes.cpp/.h`** (sondas de luz): se iniciaban al arrancar, pero ningún código las usaba.
- **`docs/SSGI_LIGHT_PROBES_IMPLEMENTATION.md`**: guía del SSGI y las sondas de luz anteriores.
- **Opciones de `aitd_remaster.cfg`** que dejaron de existir: `postprocessing.bloomThreshold`, `bloomIntensity`, `bloomPasses`, `filmGrainIntensity`, `ssaoRadius`, `ssaoIntensity`, `ssgi*`, `lightProbes*`, `colorGrading`, `exposure`, `contrast`, `saturation`, `temperature`, `shadowLift`, `highlightRolloff`, `animation.poseSmoothingStrength`, `graphics.filtering` y `graphics.wallDepth`. Las dos últimas no hacían nada.

## Ventana de opciones F1 del remaster (2026-10-25, BUGS.md C13)

- **`remaster/ui/remasterOptions.cpp/.h`**: ventana F1 en inglés, con pestañas de opciones HD, controles y diagnósticos. Estaba desactivada en modo Original y el juego seguía corriendo detrás. Sustituida por `remaster/ui/advancedOptions.cpp`, con las mismas opciones que el launcher: pausa el juego, guarda al cerrar y está en 5 idiomas.

## Fondos HD de cámara del remaster, etapa 1 (2026-10-26)

Sustituidos por el módulo propio `remaster/graphics/customBackgrounds.cpp` (fondos y máscaras personalizados, BUGS.md C16). Se borran para no arrastrar código ajeno que ya dio problemas; el modo HD ya no se ofrece en el launcher. Guardado en el historial hasta el commit `7dcdbd47`.

- **`remaster/graphics/backgroundDump.cpp/.h`** y la opción `backgrounds.dump`: exportaba los fondos al arrancar (`backgrounds_dump/CAMERAxx_Fyy_Czz.png`) con la paleta del momento, que daba colores equivocados (C5, C16). Sustituido por "Exportar fondos" en F1.
- **Volcado y sustitución de máscaras del remaster** (`dumpMaskToPng`, `loadReplacementMask` en `rendererBGFX.cpp`; carpetas `masks_dump/` y `masks_hd/`; opciones `masks.dump` y `masks.load`): solo funcionaba con fondos HD y usaba otros nombres. Sustituido por "Exportar máscaras" y las máscaras personalizadas.
- **Fondos HD de cámara** (en `remasterHooks.cpp`: `loadCameraHDBackground`, `unloadFloorHDBackgrounds`, `preloadFloorBackgrounds`; en `hdBackground.cpp`: la caché de precarga por piso) y los ganchos del core que solo servían para eso: `g_onCameraBackgroundLoaded` (`camera.cpp`), `g_onFloorLoaded` y `g_onFloorUnloaded` (`floor.cpp`). Cargaban `backgrounds_hd/CAMERAxx_zzz.png` solo en modo HD.
- **Hierba animada** (`remaster/graphics/bgAnimGrassMask.cpp/.h`, `osystem_drawAnimatedGrassMask` en `rendererBGFX.cpp` y el gancho `g_onSceneActorsDrawn` de `sceneDraw.cpp`): máscara animada para un fondo HD animado concreto (`CAMERA07_004`); sin fondos HD de cámara no tiene uso.

Lo que quedaba se borró en la etapa 2 (sección siguiente).

## Pantallas HD del remaster, etapa 2 (2026-10-26)

Antes de borrar se rehízo lo útil desde cero: "imágenes personalizadas" para las pantallas completas (título, libros, elección de personaje, fin de partida...; gancho `g_onScreenPictureShown`, en `customBackgrounds.cpp`) y la ayuda de controles con mando (`remaster/ui/controllerHint.cpp`, gancho `g_onStartMenuDrawn`). En modo Original nada de lo borrado se veía, salvo la ayuda de controles. Guardado en el historial hasta el commit `95ac536d`.

- **`remaster/graphics/menuBackgrounds.cpp`** (≈1700 líneas): retratos HD de la elección de personaje (`Carnby.png`, `Emily.png`, variable `g_portraitOverlayChoice`), fondos HD del menú de pausa, guardar/cargar (con vista previa HD de la partida), inventario y objeto encontrado, arte del menú inicial y de la selección de idioma, marco a pantalla completa (`FullScreenFrame.png`), lupa de pistas (`osystem_drawHotspotOverlay`), viñeta (`osystem_drawVignette`) y la ayuda de controles vieja. Todo eran imágenes de `backgrounds_hd/` dibujadas encima del juego.
- **`remaster/graphics/hdBackground.cpp/.h`**: cargador de imágenes HD (archivo `backgrounds_hd.hda` o PNG sueltos, fondos animados por fotogramas, marcos). El atlas de modelos no lo usaba para leer (ya usa stb); solo le preguntaba si el HD estaba activo, ahora lee `graphics.hdBackgrounds` directamente.
- **`remaster/graphics/hdBackgroundRenderer.cpp/.h`**: textura RGBA del fondo HD (`g_hdBackgroundTexture`, `g_currentBackgroundIsHD`), fondos animados y la luz de la lámpara sobre fondos HD (`setHDBackgroundLanternUniforms`; el bloque que la calculaba en `lanternLighting.cpp` también se quitó).
- **`remaster/graphics/resourceGC.cpp/.h`**: liberación diferida de fondos HD animados grandes; sin ellos no tiene uso.
- **Shaders:** `maskHDBackground_ps` (máscaras sobre fondo HD) y el bloque de luz de lámpara de `maskBackground_ps` (solo actuaba con fondo HD). `hdBackground_ps` se sustituyó por `imageBackground_ps`, propio y simple (imagen RGBA por el nivel de fundido), que dibuja la escena congelada detrás de los menús y los fotogramas de secuencia sustituidos por PNG.
- **Ganchos del core:** `g_onShowHDScreenImage`, `g_onUseStandardBackground`, `g_onClearAnimatedBackground`, `g_onPauseBackgroundAnimation`, `g_isHDBackgroundActive`, `g_setHDBackgroundActive` (y sus funciones `core...`) y `g_onDetailLevelChanged`. El core ahora solo avisa con `coreScreenPictureShown`. `g_onOpenControlsMenu` ya no recibe "modo HD".
- **Ramas HD del core:** marcos alternativos en el menú de pausa, inventario, objeto encontrado, guardar/cargar y controles (`useHDBG`, `useHDInventoryBG`, `useHD`); copia del marco al `uiLayer` en la elección de personaje (`CopyFrameBorderToUILayer`); texto de libros sobre fondo HD; `scaleDownImageHD`.
- **Pantalla de mapa** (`processMapScreen`, tecla TAB, `osystem_loadMapTexture`/`drawMapImage`/`destroyMapTexture`): solo del remaster (desactivada en modo Original) y mostraba planos HD `ETAGExx.png`.
- **Opción de detalle Low/High del remaster** (`applyDetailLevel`): cambiaba fondos HD, fuentes TTF y postprocesado a la vez. La opción "Detalle" del menú de pausa es del juego de 1992 y se queda; el menú inicial vuelve a dibujar siempre su marco original.
- **`asyncLoadHDBackground`** (`util/asyncLoader.cpp`): función vacía que nadie llamaba. Y las etiquetas de registro `HDBG`, `HDBGR` y `GC` de `consoleLog.h`.

Se mantienen: `hdArchive.cpp` (lo usan el audio y el atlas de modelos) y los fotogramas de secuencia sustituidos por PNG (`sequence.cpp`, opción `sequences.load`), que no dependían de lo borrado; quedan para revisar más adelante. La detección de pistas (`isHotspotOverlayVisible` en `mainLoop.cpp`) sigue en el código sin dibujarse; si se quieren pistas visuales se rehará como módulo propio.

## Despliegue en Steam/GOG, overlay de Steam y comprobador de actualizaciones (2026-10-26)

Pensados para la versión publicada del remaster original; en Classic Reborn el juego se abre con su propio launcher y nunca escribe en la instalación del jugador. Guardado en el historial hasta el commit `f68f4b5f`. Si se quiere compatibilidad con Steam o GOG Galaxy, se hará con código propio (ROADMAP).

- **`TatouSource/DOSBoxStub/`** (`DOSBox.exe`): programa diminuto que sustituía el `DOSBOX\DOSBox.exe` de la instalación de Steam/GOG para que el botón "Jugar" de esas plataformas abriera `..\INDARK\Tatou.exe` en vez del emulador. No era un emulador. Se compilaba en cada build sin usarse (4,7 MB).
- **`remaster/config/gameDataCopy.cpp/.h`** (909 líneas): si faltaban los datos, buscaba la instalación de Steam (app 548090), GOG o el CD y copiaba los `.PAK`/`.ITD` a la carpeta del juego; además desplegaba el remaster dentro de la instalación de GOG con el `DOSBox.exe` falso (`deployRemasterToInstall`). Estaba apagado por `gamedata.steamless = true`; ahora el launcher elige la carpeta de datos y el plan es leer de `GAME.GOG`.
- **Opción `gamedata.steamless`**: solo servía para apagar lo anterior y el overlay de Steam. Si aparece en un `aitd_remaster.cfg` viejo, se ignora.
- **`remaster/config/steamOverlay.cpp/.h`** (237 líneas): cargaba `steam_api64.dll` para el overlay de Steam (Mayús+Tab).
- **`remaster/config/updateChecker.cpp/.h`** (210 líneas): en cada arranque se conectaba a `api.github.com`, consultaba las versiones del proyecto original (spacefarergames/AloneInTheDarkReHaunted) y, si había una más nueva, abría el navegador en su página de descargas. No tenía sentido en el fork y se conectaba a internet sin avisar. Se rehízo de cero, solo en el launcher (2026-10-30, `Launcher/updater.cpp`): mira las versiones de este proyecto en GitHub y las instala él mismo, sin tocar las opciones ni las partidas; el juego sigue sin conectarse.
- Las etiquetas de registro `STEAM` y `DATA` de `consoleLog.h`.

## MSAA del remaster original (2026-10-26)

- **`graphics.msaa` / `msaaLevel`** (`getMsaaResetFlags` en `platform/bgfxGlue.cpp`): pedía varias muestras por píxel en la imagen final de la pantalla, solo en modo HD. No suavizaba nada: la escena se dibuja antes en una imagen fuera de pantalla sin varias muestras (ya anotado en BUGS.md C13). El suavizado de bordes es ahora el FXAA de las opciones avanzadas (`postprocessing.antialiasing`), que basta para estos polígonos lisos; por eso tampoco se hará el MSAA x2/x4/x8 propio. Si aparece `graphics.msaa` en un `aitd_remaster.cfg` viejo, se ignora.

## Controles del remaster original (2026-10-26)

Sustituidos por un sistema propio desde cero (`Shared/controlsBindings.h`, `platform/controls.cpp`). Guardado en el historial hasta el commit `369f1ca8`.

- **`remaster/ui/controlsMenu.cpp/.h`** (≈740 líneas): submenú "Controles" del menú de pausa, solo en modo HD (en modo Original no se podía cambiar ningún control). Una sola tecla y un solo botón por acción, guardados como números de SDL (`controls.key.up = 82`); W A S D, Enter, Esc y Tab estaban fijos en `input.cpp`.
- **La traducción de mando y teclado en `platform/input.cpp`** (`updateController`, la parte de teclado de `readKeyboard`, `ControllerConfig`): pasa a `platform/controls.cpp`, con varias entradas por acción (teclas, botones, gatillos y direcciones de los sticks).
- **El giro rápido de 180° (Q / E, LB / RB) y el mapa (Tab, Back)**: el juego de 1992 no los tiene. El giro lo había añadido el remaster dentro del core (`GereManualRot` en `core/track.cpp`, bits `0x10` / `0x20` de `JoyD`), que vuelve a ser la función original; el mapa nunca hizo nada en AITD1.
- **Las claves `controls.key.*` y `controls.pad.*`** de `aitd_remaster.cfg`: se ignoran; ahora es una línea legible por acción (`controls.run = key:Left Shift, pad:leftstick, axis:righttrigger+`).

## Código muerto del dibujo de objetos (2026-10-27)

- **`drawSceneObjects()`** (`core/sceneDraw.cpp`, declarada en `core/main.h`): una copia vieja del bucle de `AllRedraw` que dibuja los objetos de la cámara. Nadie la llamaba; el juego siempre dibuja con `AllRedraw`.
- **`renderHardColDepthPass()`** (`core/renderer.cpp`, `platform/osystem.h`): función vacía ("not yet implemented") que solo llamaba `drawSceneObjects`.

## Pantalla completa con doble clic (2026-10-27)

- **Doble clic izquierdo en la ventana** (`readKeyboard` en `platform/input.cpp`): cambiaba entre ventana y pantalla completa. Molestaba al hacer clic en las ventanas de prueba (editor de objetos, F10) y nadie lo usaba; quedan F11, Alt+Intro y la opción "Pantalla completa" de F1.

## Botones para abrir las carpetas de imágenes (2026-10-27)

- **"Abrir carpeta de mis imágenes" y "Abrir carpeta de exportación"** en F1, con su "(?)" de rutas, y las rutas del "(?)" de Fondos personalizados en el launcher: las carpetas son fijas y se explicarán en el README general. Se borraron con ellos `platform/openFolder.cpp/.h` (`osystem_openFolder`), `customBackgroundsFolderPath` y las otras tres funciones de rutas de `customBackgrounds.cpp`, `pictureFolders` del launcher y sus textos (`OpenCustomFolder`, `OpenExportFolder`, `PictureFoldersHelp`, `PictureFolders`).

## Limpieza de la solución (2026-10-27, bloque A)

- **`Assets/`** (680 MB, 3 552 archivos): fondos, máscaras y texturas HD del remaster original (hechos con el arte del juego, no nuestros). Nada los usaba desde que se borró su HD (más arriba); los fondos propios van por `Fondos`/`Mascaras`.
- **`tools/`** (`build_hda_archive`, `unpack_hda_archive`) y `rebuild_hda_archive.bat/.ps1`, `FitdLib/pack_audio.ps1`: herramientas del remaster para empaquetar sus archivos `.hda`.
- **Soporte de Nintendo Switch y UWP**: ramas de los `CMakeLists.txt`, `cmake/NintendoSwitch.toolchain.cmake`, `Package.appxmanifest`, `deploy_uwp.bat` (nunca hubo código para ellos).
- **`FITD.sln/.slnx`** guardadas a mano (la solución se genera en `build/`), **`CMakePresets.json`** (apuntaba a una carpeta `vs2026` inexistente) y `FitdLib/readme.txt`.
- **Proyectos de terceros que no se usan**: `SDL3_test` (`SDL_TEST_LIBRARY OFF`), la zlib compartida y sus ejemplos `example` y `minigzip` (ahora solo `zlibstatic`), y las herramientas de bgfx para modelos, texturas y `bin2c`.
- **`FitdLib/util/zlib.h` y `zconf.h`**: copia vieja de las cabeceras de zlib, distinta de la biblioteca enlazada.

## Modo HD del remaster original (2026-10-27, bloque B)

El launcher siempre arrancaba el modo Original y no había forma de elegir el HD: todo lo que solo funcionaba en él era código que nunca se ejecutaba. Regla del proyecto: lo que no se usa y no está en F1 se borra; si se quiere algo así en el futuro, se hace de cero.

- **El modo en sí**: `isOriginalMode`, `applyOriginalModeAfterInit`, `--mode` (Tatou.exe y el launcher), `gamedata.mode`, `applyOriginalModeOverrides` y la copia de los valores HD en la configuración.
- **Sombras** (`remaster/graphics/shadows.cpp/.h`, ganchos `g_onShadowPrepass`, `g_onDrawBlobShadow`, `g_onDrawPlanarShadow`, `g_onDrawWallPlanarShadow`, `g_planarShadowEnabled`, `g_wallShadowEnabled`, `g_submitShadowStencil`, `hidePlanarShadow`).
- **Sangre** (`bloodParticles.cpp/.h`, gancho `g_onActorHit`) y **polvo** (`dustParticles.cpp/.h`, shader `particle`).
- **Lámpara** (`lanternLighting.cpp/.h`, ~1 700 líneas, shaders `lantern_bloom` y `lantern_shadow`, ganchos `g_onHeldLampDrawn` y `g_onMenuOverlayChanged`): solo dibujaba el brillo que tapaba la lámpara (BUGS.md V7), ya apagado; no había ninguna luz de lámpara visible.
- **Letras TTF** (`remaster/ui/fontTTF.cpp/.h`, ganchos `g_onDrawText`, `g_onTextCleared`, `g_onMenuStateChanged`, `g_onMenuSelectionChanged`, opciones `font.*`): las reemplaza el "Texto mejorado".
- **Modelos con textura** (`modelAtlas.cpp/.h`, `modelAtlasRender.cpp/.h`, `hdArchive.cpp/.h`, `appArchive.h`, ganchos `g_onPolyDrawn`, `g_onBodySelected`, `g_onPrepareTatouAtlas`, `g_onTatouAtlasActive`, `g_onModelAtlasReset`): sus texturas estaban en `Assets/`. Las esferas y los puntos se dibujan siempre con los colores de la paleta (`addBillboard` en `rendererBGFX.cpp`).
- **`audio.hda`**: el lector de audio empaquetado de `osystemAL.cpp` (el audio sale de archivos sueltos o de `GAME.GOG`).
- **Menú de pausa del remaster**: las opciones Pantalla, Controles y Pistas (y el gancho `g_onOpenControlsMenu`) y el fondo oscurecido ("borroso"); queda el menú de 1992.
- **Franjas de cine** (letterbox) y **animación de pasar página** del libro (`sceneEffects.cpp`).
- **Pistas de objetos cercanos** (la lupa, `detectNearbyInteractiveHotspot`) y la **mira láser** (`drawLaserSight`).
- **Ilustraciones del menú de inicio** (`graphics.useArtwork`) y **fotogramas HD de las secuencias** (`sequences.dump/load`, `osystem_setSequenceHDFrame`).
- **Heurísticas de polígonos** del remaster en `renderer.cpp` (recorte de detalles cercanos, geometría flotante).
- **Parches de scripts** que solo servían a lo anterior: sombras de las gallinas y de los cuerpos 5/6/55/56, polvo del coche, rana del principio, aceite de la lámpara.
- **Configuración** (`configRemaster`, reescrita desde cero): fuera `controller.*` (valores fijos de `ControlsAnalog`), `graphics.hdBackgrounds`, `backgroundScale`, `blurredMenu`, `menuBlurAmount`, `gameplay.hints`, `graphics.useArtwork`, `postprocessing.vignette*`, `music.external`, `music.folder`, `sequences.*`, `gamedata.jackmode` (Jack se reconoce por `PERE.PAK`) y `debug.originalDrawOrder` (con él, la comparación de 1992 de `actorList.cpp`).

## Funciones y archivos sin uso (2026-10-27, bloque B, parte 2)

Búsqueda en todo el código propio (`FitdLib`, `Shared`, `Launcher`, `Fitd`) de funciones definidas cuyo nombre no aparece en ningún otro lugar, o solo en su declaración, repetida hasta que no quedó ninguna. Se dejan a propósito: `fmopl.cpp` (emulador de AdLib de terceros, completo) y `strncpy_s` de `util/platform_compat.h` (para Linux).

- **Archivos enteros**: `util/asyncLoader.*` (caché de precarga de PAK: `preloadPak` nunca se llamaba, así que `loadPak` siempre la encontraba vacía), `platform/osystemAL_mp3.*` (reproductor MP3 dentro de un `#if 1 / #else` vacío), `platform/videoMode.*` y la variable `videoMode`, `util/threadCode.*` y `util/version.cpp` (vacíos), `util/endianess.h`, `util/baseTypes.h`, `util/memoryManager.h`, `core/triangle.h` y `Fitd/resource.h` (nadie los incluía).
- **Restos del OpenGL de FITD**: `osystem_initGL` y `osystem_preinigGL` (cuerpo en `#if 0`), `osystem_initBuffer`, `backBuffer`, `g_screenWidth/Height`, `gameScreenToViewport` y unas cuentas sin uso en `osystem_setClip`; `osystem_refreshFrontTextureBuffer` (vacía, con sus 3 llamadas), `osystem_drawBlackScreen`, `osystem_fillPolyTextured`, `osystem_drawMaskStencilPrepass`, `renderGameWindow`, `loadBgfxShader`, `osystem_delay`, `osystem_updateImage`, `OPL_musicPlayer`.
- **Bordes HD de los cuadros de texto** (`drawHDFrameBorders` en `aitdBox.cpp`, en `#if 0`): el cuadro se dibuja siempre con los sprites de 1992.
- **ImGui del ejemplo de bgfx**: los bloques `USE_ENTRY` (siempre 0) y `#if 0` de `imguiBGFX.cpp`, `imguiDestroy`, `imguiRGBA`.
- **Sueltas**: `drawConverZones`, `drawConverZone`, `drawMaskZones`, `drawMaskZone`, `drawRoomZvLine`, `drawZone`, `drawOverlayZone`, `drawAAQuad` (dibujos de depuración que nunca se llamaban), `primFunctionDefault`, `defaultRenderFunction`, `getSaveEntry`, `ensureSequenceDirectory`, `blitScreenTatou`, `dumpPak`, `reloadLanguage`, `deleteBgfxGlue`, `osystem_isVOPlaying`, `osystem_releaseScenePreview`, `menuMouseHitTwoButtons`, `PAK_Error`, `READ_LE_S32`/`READ_BE_U32`/`READ_BE_S32`, `applyBytecodeLifecyclePatches` (vacía) con su enumeración, `dumpBytecodePatches`, `getBytecodePatches`, `registerNativeLifeScript` (el código generado usa `registerNativeLifeScriptIfSafe`).
- **Gancho `g_onShutdown`**: nadie se registraba (antes liberaba los fondos HD).
- **Registro**: `util/consoleLog.h` reescrito con solo las etiquetas que se usan (de 210 a 101 líneas); fuera las de TTF, HD, etc., los códigos de color (ya estaban vacíos) y `enableConsoleColors`.

## Parches de scripts y arreglos apagados (2026-10-27, bloque B, parte 3)

`scripts/bytecodePatches.cpp` (1 266 líneas, más su documentación de 418) tenía 11 "parches" que se ejecutaban después de cada instrucción de script. Revisados uno por uno:

- **Se quedan** (ahora en `scripts/lifeSafety.cpp`, 90 líneas, una función llamada desde `processLife`): los cuatro que corrigen un valor imposible: animación inexistente, fotograma pasado del final, cuerpo inexistente y velocidad o modo de movimiento fuera de rango. Nunca aparecieron en los registros, pero evitan que el motor lea fuera de sus datos.
- **Fuera, solo escribían en el registro**: validar el puntero del objeto (la condición ya exigía que fuera válido, así que nunca hacía nada), comprobar recursos y comprobar las coordenadas de la caja de choque. También el aviso "Monitoring actor N for movement issues", que salía en cada partida sin significar nada.
- **Fuera, apagados con interruptores en `core/workarounds.h`** (todos a 0; el archivo también se borró): gallinas zombi atascadas tras la ventana (`ENEMY_STUCK_WORKAROUNDS`, con su detección de puntos de ruta inalcanzables y velocidad 0 en `track.cpp`), jugador atascado en las cámaras 04_000 y 05_000 (`PLAYER_STUCK_CAMERA_WORKAROUNDS`), sacar al jugador de las paredes al cambiar de sala (`PLAYER_WALL_WARP_WORKAROUND`, `fixPlayerStuckInWall`) y cortar los sonidos de la introducción en la primera cámara (`INTRO_SOUND_STOP_WORKAROUND`). Si alguno de esos fallos aparece al jugar, se investiga y se arregla de nuevo con su causa.
- **Fuera, restos de las sombras**: esconder la sombra de la gallina zombi hasta que rompe la ventana, más variables sueltas de la rana de la introducción, los cuerpos sin sombra y el polvo del coche.
- El sistema de registro de parches (condición + acción, antes y después de cada instrucción) se reemplazó por la llamada directa. Diez archivos del core incluían `bytecodePatches.h` sin usarlo.

## Restos encontrados al escribir el mapa (2026-10-27, bloque D)

- **Modelos con textura**: el dibujador todavía vaciaba en cada imagen una lista de vértices con textura que nadie llenaba (`texturedVertices`, `g_activeModelTexture`, `getTexturedShader`) y el shader `textured_ps`; en `core/renderer.cpp`, la marca `s_modelHasTexturedPrims`, que nadie leía.
- **Shader `vs_fullscreen.sc`**: no estaba registrado en ningún programa.
- **`util/exceptionHandler.cpp`**: vacío (todo está en su `.h`).
- **`platform/reshadecompat.h`**: manejador de cierres que reemplazaba al nuestro (BUGS.md C31).

## Documentos y archivos viejos (2026-10-28, bloque E)

Antes de preparar la versión pública se quitaron los archivos que ya no eran ciertos o no servían:

- **Documentos del remaster original**: `REMASTER.md`, `ARCHITECTURE.md` y `BUILDING.md` (describían el modo HD y una compilación para Linux que hoy no funciona), los 22 informes de `docs/upstream/`, `docs/TTF_FONT_README.md` (las letras TTF ya se habían borrado en el bloque B), `docs/NATIVE_SCRIPTS_QUICK_REFERENCE.md` y `docs/GAMEPLAY_TESTING_CHECKLIST.md` (la explicación vigente de los scripts nativos está en `scripts/native/nativeLife.h`). `README.md` y `CONTRIBUTING.md` se reescriben.
- **Capturas de `docs/baseline/`** (7 imágenes del juego de fallos viejos, varios de sistemas ya borrados: sombras, menú del modo HD): `BUGS.md` conserva sus nombres, como en los fallos posteriores.
- **Scripts**: `build/vs2022.bat` y `vs2026.bat` (buscaban Visual Studio 2019 por error; la solución se genera con el comando de CMake), `run_linux.sh` (rutas del PC del autor original) y `build_linux.sh` (Linux se hará en la Fase 7).
- **`TatouSource/LICENSE`**: copia idéntica de la licencia de la raíz.

## Intentos descartados

- **Bordes suaves y ajustados por color en las máscaras con fondos personalizados (aparcado, 2026-10-26):** con un fondo nuevo, la máscara de 320×200 de una silla dejaba un anillo dentado de suelo sobre el personaje. Se probó, solo con fondos personalizados: (1) promediar la máscara en un píxel original y fundir el borde; (2) en la franja del borde, decidir por color si cada punto es del objeto o del suelo (primero con el color más parecido, luego con la media de cada lado y el color suavizado); (3) que cada punto decida junto a 4 vecinos para quitar los puntitos que dejaba un suelo con vetas. Mejoró mucho, pero quedaban algunos puntitos. Se aparca para probar antes las máscaras propias en alta definición, que dan el borde exacto. Código en la rama `feature/soft-mask-edges` (commit `d88296f7`, sin unir a `main`); puede servir para las máscaras que no se hagan a mano.

- **Cristal "multiplicador" (V20, primer intento):** multiplicaba lo que hay detrás por el color del cristal. La ventana del desván quedaba bien, pero el cristal de la lámpara, que está sobre el tapete verde, salía negro. Se cambió por la regla exacta de la paleta del original (`glass_ps`). Estaba guardado en un `stash` y en la rama `fix/transparent-polygons`, ya borrados.

## Personaje tapado por el cristal en el borde de la ventana del desván (V22, pendiente)

Tres intentos que no cambiaron nada visible. Se deshicieron y el problema queda anotado en BUGS.md V22.

- **Fórmula de las zonas de cámara:** en `testCrossProduct` (`core/collision.cpp`) el denominador usa `yAC` donde la fórmula de cruce de segmentos lleva `yAB`. Se corrigió, pero la cámara seguía sin cambiar en el borde de la ventana. Un registro que comparaba las dos fórmulas en cada paso dio exactamente el mismo resultado: con la línea horizontal que usa `isInPoly` la errata no influye. Se dejó como estaba; puede que el original calcule igual.
- **Registro de zonas de cámara (diagnóstico, no un arreglo):** mostró que las zonas son las del juego y que se solapan junto a esa pared (cámara 0 hasta x = -60, cámara 2 desde x = -14). Se queda la cámara actual mientras el centro de la caja siga en su zona, así que llegando por la derecha se mantiene la cámara 0 hasta -60. La caja del personaje toca la pared (z 499 contra 500). Quitado tras usarlo.
- **Orden de dibujo con cajas que se cruzan:** en `sortCompareFunction` (`core/actorList.cpp`), si las cajas de dos objetos se solapaban en X y en Z, se ordenaban por la distancia del centro (lo que el código dejó comentado como "hack", `flag = 0`). No cambió nada: el registro del orden mostró que la caja del personaje (z 442..499) y la de la ventana (z 500..510) no llegan a cruzarse, así que ese caso no se daba. La ventana queda delante por la comparación de bordes en Z, que es lo que falta entender.
- **No dibujar polígonos muy finos (V15):** quitaba polígonos de menos de un píxel de grosor. Las rayitas seguían y desaparecían trozos del personaje. Rama borrada; la solución fueron las máscaras ensanchadas.
