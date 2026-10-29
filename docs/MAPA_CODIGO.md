# Mapa del código

Qué hay en cada carpeta y para qué sirve cada archivo de **Alone in the Dark: Classic Reborn**. Lo que se borró y por qué está en `docs/DESCARTADO.md`; los fallos y sus arreglos, en `docs/baseline/BUGS.md`. Las funciones públicas de cada archivo, una por una, están en `docs/DICCIONARIO.md` (generado con `tools/makeDictionary.py`). El orden de capas se comprueba con `tools/checkLayers.py`.

Actualizado el 2026-10-27, al terminar la limpieza (bloques A, B y C).

## 1. Las capas

El motor (`TatouSource/FitdLib/`) está dividido en capas. Cada capa usa solo las de **debajo**:

```
  Fitd/fitd.cpp        el programa (Tatou.exe): arranca el motor y conecta el remaster
        │
  remaster/            extras propios: F1, fondos personalizados, correcciones de objetos,
        │              editor F10, trucos F9
        │   (se registra en los ganchos de core y platform)
  core/ + scripts/     el juego de 1992: salas, objetos, animaciones, inventario, partidas,
        │              y el intérprete de los scripts
  platform/            el sistema: ventana, dibujo con bgfx, efectos de imagen, audio,
        │              teclado y mando, archivos, opciones guardadas
  util/                herramientas sueltas que no dependen de nada: tipos, registro,
                       descompresión, tablas, hilos
```

Fuera del motor, dos proyectos más y una carpeta compartida:

```
  Launcher/            Launcher.exe (Windows): anfitrión de la pantalla del launcher
  Shared/, Shared/ui/  solo cabeceras, usadas por el juego y por el launcher
                       (menús, textos, controles, volumen); no dependen del motor
```

**Reglas:**

- Una capa nunca incluye cabeceras de las de arriba. Cuando una capa de abajo tiene que avisar a una de arriba, lo hace con un **gancho**: un puntero a función que empieza vacío y que la capa de arriba rellena si quiere responder. Si nadie lo rellena, el motor hace lo del juego original.
  - `core/coreHooks.h`: lo que anuncia el juego (fondo de cámara listo, imagen de pantalla completa mostrada, objeto a punto de dibujarse, pausa del juego, imagen del bucle...).
  - `platform/platformHooks.h`: lo que anuncia el sistema (gráficos listos, momento de dibujar los menús, tecla pulsada, acción de opciones, ¿algún menú usa los controles?, imagen de fondo del jugador, máscaras, pantalla subida...).
  - El remaster se conecta en `remaster/config/remasterHooks.cpp` (`registerRemasterHooks`), que el programa enchufa en `g_onGraphicsReady` (`Fitd/fitd.cpp`).
- **Excepción aceptada:** `platform/` usa los tipos y variables del juego (`core/common.h`). Es el diseño de FITD: su capa de sistema dibuja y suena con los datos del juego. Nunca usa el remaster.
- `util/` no depende de nada (los tipos `u8`...`s32` están en `util/types.h`).
- `CMakeLists.txt` de `FitdLib` compila **todo** `.cpp` de sus carpetas (`GLOB_RECURSE`) y cada carpeta está en la lista de inclusión: los archivos se incluyen por su nombre, sin ruta. Al agregar, mover o borrar un archivo hay que volver a generar la solución con CMake.
- Los nombres del motor original se mantienen en francés (`AffObjet`, `LoadEtage`...): ver el glosario (sección 4).

## 2. Archivo por archivo

Cuando un archivo tiene `.cpp` y `.h`, va en una sola fila.

### util/: herramientas sueltas

| Archivo | Para qué sirve |
| --- | --- |
| `types.h` | Tipos de números por tamaño (`u8`, `u16`, `u32`, `s8`, `s16`, `s32`), los de los archivos del juego. |
| `consoleLog.h` | Etiquetas del registro: cada mensaje empieza con la de su sistema (`[CFG]`, `[LIFE]`, `[BGFX]`...). |
| `logFile.cpp/.h` | Todo lo que el motor escribe (`printf`) va a `tatou.log`, al instante; la sesión anterior queda en `tatou.prev.log`. |
| `exceptionHandler.h` | Registro de cierres y corrupciones de memoria en `crash_log.txt` (Windows). |
| `safeDelete.h` | Borrado protegido de recursos (lo usa `hqr.cpp` al vaciar sus listas). |
| `unpack.cpp/.h` | Descompresión de las entradas de los `.PAK` (el "explode" de PKWARE del juego y deflate con zlib). |
| `cosTable.cpp/.h` | Tabla de cosenos del motor (los giros del juego se miden en 1024 pasos por vuelta). |
| `jobSystem.cpp/.h`, `jobSystemInit.h` | Hilos de trabajo: hoy decodifican las voces `.VOC` sin frenar el juego. |
| `platform_compat.h` | Sustitutos de funciones solo de Microsoft (`fopen_s`, `strcpy_s`...) para otros compiladores (Linux, Fase 7). |
| `version.h` | Fecha y hora de compilación, escritas en el registro al arrancar. |

### platform/: el sistema

| Archivo | Para qué sirve |
| --- | --- |
| `osystem.h` | Lista de funciones del sistema que usa el juego (`osystem_*`): dibujo, sonido, tiempo, pantalla. |
| `osystemSDL.cpp` | Arranque del programa (`FitdInit`): carpeta del juego, registro, ventana SDL, hilos; el bucle de eventos y `dataFileExists`. |
| `bgfxGlue.cpp/.h` | Ventana y bgfx: creación, tamaño, pantalla completa, rectángulo de la escena, inicio y fin de cada imagen, ImGui. |
| `rendererBGFX.cpp`, `rendererBGFXInternal.h` | El dibujador: fondo de 320×200, máscaras, polígonos, esferas, líneas, cristal, capa de textos, fundidos. |
| `sceneCapture.cpp` | Foto de la escena 3D para el menú de pausa y fondo congelado detrás de los menús. |
| `screenEffects.cpp/.h` | La escena se dibuja fuera de pantalla y se copia con los efectos opcionales (FXAA, bloom, grano, SSAO) y el fundido. |
| `imguiBGFX.cpp/.h` | Dear ImGui dibujado con bgfx (los menús). |
| `menuMouse.h` | Ayudas de ratón para los menús del juego de 1992 (lista, botones, clic). |
| `input.cpp/.h` | Teclado, ratón y mando con SDL: F11, Alt+Intro, cursor oculto con mando, bloqueo de entradas con un menú abierto. |
| `controls.cpp/.h` | Asignaciones de controles en uso; cada imagen convierte teclado y mando en las entradas de 1992 (`JoyD`, `key`, `Click`) y lee el stick izquierdo para el movimiento moderno (`controlsLeftStick`, `controller.modernStick`). |
| `settings.cpp/.h` | Las opciones (`g_settings`), guardadas en `aitd_remaster.cfg`: pantalla, efectos, animación, volumen, idioma, controles y ajustes de desarrollador. |
| `platformHooks.cpp/.h` | Los ganchos del sistema (ver sección 1). |
| `gameFolder.cpp/.h` | Carpetas: datos (copia o instalación de GOG/Steam), carpeta de trabajo (`UserData` o la copia en modo depuración), Jack en `JACK`. |
| `fileAccess.cpp/.h` | Cargar archivos `.ITD` y entradas de `.PAK` en memoria, y el error fatal si falta uno. |
| `gogImage.cpp/.h` | La música y las voces de la imagen del CD de GOG (`GAME.GOG`, leída con `Shared/cdImage.h`), sin extraer nada. |
| `osystemAL.cpp/.h` | Audio con SoLoud: efectos, voces (narrador del juego o de `Voices`, página por página), música del CD, dos grupos de volumen (música / voces y efectos). |
| `osystemAL_adlib.cpp` | Música AdLib de Jack in the Dark, como fuente de sonido de SoLoud. |
| `fmopl.cpp/.h` | Emulador del chip AdLib (OPL2) de MAME, para la música de Jack. |
| `vocDecoder.cpp/.h` | Convierte los archivos de voz `.VOC` (Creative) a WAV para SoLoud. |
| `reshadecompat.h` | Diagnóstico: avisa en el registro si hay un ReShade instalado que pueda interferir. |

### core/: el juego de 1992

| Archivo | Para qué sirve |
| --- | --- |
| `common.h` | Cabecera general: incluye todo lo del juego; casi todos los archivos del core la incluyen. |
| `config.h` | Opciones de compilación (`FITD_DEBUGGER` en Debug) y cabeceras del sistema. |
| `vars.cpp/.h` | Variables globales y estructuras de datos del juego (objetos, salas, cámaras, inventario...). |
| `coreHooks.cpp/.h` | Los ganchos del juego (ver sección 1). |
| `main.cpp/.h` | `FitdMain`: arranque del motor (opciones, parámetros, gráficos, juego) y funciones sueltas del original (mensajes, golpes, recorte, salida). |
| `launchOptions.cpp/.h` | Parámetros del launcher: `--game aitd1\|jack` y `--lang`. |
| `startup.cpp` | Inicio: variables del juego (`DEFINES.ITD`), textos del idioma, paletas, cronómetro del objeto en la mano. |
| `game.cpp` | Detección del juego, inicio de partida, pantalla de guardar y cargar (con fecha en números pequeños). |
| `world.cpp` | Carga del mundo (objetos del mundo, variables) al empezar una partida. |
| `cheats.cpp/.h` | Trucos de F1 (`g_settings.cheats`, ninguno actúa con los trucos apagados): vida infinita repone lo que quita un golpe (AITD1 `vars[21]`, Jack `vars[6]`); si un script pone la vida en 0 (trampa) la muerte sigue. |
| `mainLoop.cpp/.h` | Bucle principal: cada imagen procesa objetos, scripts, cámaras, salas y dibuja. |
| `modernStick.cpp/.h` | Stick izquierdo moderno: el personaje camina hacia donde apunta el stick según la cámara (proyecta el paso de cada ángulo con la cámara actual) y conserva la dirección al cambiar de cámara; la cruceta y el teclado siguen tipo tanque. |
| `AITD1.cpp/.h`, `AITD1_Tatou.cpp` | Lo propio de AITD1: pantallas de introducción, elección de personaje, libros; el armadillo 3D del menú de inicio. |
| `JACK.cpp/.h` | Lo propio de Jack in the Dark (inicio). |
| `AITD2.cpp/.h`, `AITD3.cpp/.h` | Lo propio de AITD2 y AITD3 heredado de FITD (inicio, inventario, libros). Aún no se juegan: base para cuando se agreguen sus datos. |
| `floor.cpp/.h` | Carga de un piso (`ETAGEnn.PAK`): salas, cámaras, zonas. |
| `room.cpp/.h` | Datos de las salas y cámaras; cambio de sala. |
| `camera.cpp` | Cámaras: cambio de cámara, máscaras de primer plano, salas visibles desde cada cámara. |
| `actors.cpp` | Listas de objetos activos y visibles, vista de la cámara, quitar objetos. |
| `actorList.cpp/.h` | Orden de dibujo de los objetos, del más lejano al más cercano, según la cámara (V22). |
| `object.cpp/.h` | Crear un objeto en la sala con su modelo y animación. |
| `anim.cpp/.h` | Animaciones: fotogramas, interpolación, suavizado opcional de poses. |
| `animAction.cpp/.h` | Acciones de animación: golpes, disparos, lanzar objetos, daño. |
| `anim2d.cpp/.h` | Animaciones 2D sobre el fondo (Jack y siguientes). |
| `hybrid.cpp/.h` | Objetos híbridos 2D/3D (AITD2 y siguientes). |
| `track.cpp/.h` | Recorridos: los caminos que siguen los personajes (`LISTTRAK.PAK`). |
| `collision.cpp` | Colisiones con el decorado y entre objetos, cambio de cámara al cruzar zonas, objetos lanzados. |
| `zv.cpp/.h` | Cajas de colisión (ZV): calcular, rotar, comparar. |
| `renderer.cpp/.h` | Proyección 3D del motor original: transforma los modelos y prepara sus polígonos para el dibujador. |
| `sceneDraw.cpp` | Dibujo de la escena: fondo, objetos en orden, máscaras encima, redibujado. |
| `polys.cpp`, `lines.cpp` | Relleno de polígonos y líneas en el búfer de 320×200 (máscaras de AITD1, depuración). |
| `screen.cpp/.h` | Búferes de pantalla de 320×200. |
| `sprite.cpp/.h` | Sprites (marcos, iconos). |
| `palette.cpp/.h` | Paleta de colores: carga, fundidos, conversión. |
| `font.cpp/.h` | Fuente de 1992: medir y escribir texto. |
| `aitdBox.cpp/.h` | Cuadros de texto con su marco. |
| `book.cpp` | Libros y cartas: páginas y paso de página. |
| `inventory.cpp/.h` | Inventario: lista de objetos y acciones (usar, abrir, leer, lanzar...). |
| `tatou.cpp/.h` | Funciones de base del original: fundidos de pantalla, copia rápida de pantalla, cronómetros, giro de un punto, cámara y reproducción de sonidos. |
| `startupMenu.cpp/.h` | Menú de inicio (nueva partida, cargar, salir). |
| `systemMenu.cpp/.h` | Menú de pausa de 1992 (continuar, guardar, cargar, música, sonido, detalle, salir). |
| `save.cpp/.h` | Guardar y cargar partidas (AITD1 en el formato de GOG; reconoce los formatos por el tamaño de los bloques). |
| `gameTime.cpp/.h` | Reloj del juego (60 marcas por segundo), cronómetros, reloj de la partida al guardar y cargar. |
| `music.cpp/.h` | Música: pistas del CD (AITD1) y AdLib (Jack). |
| `sequence.cpp/.h` | Secuencias animadas (cinemáticas). |
| `shaking.cpp` | Temblor de la pantalla. |
| `pak.cpp/.h` | Lectura de los archivos `.PAK` y sus entradas. |
| `hqr.cpp/.h` | Listas de recursos cargados (modelos, animaciones) con su memoria. |
| `debugger.cpp/.h`, `debugDraw.cpp` | Depurador de FITD (solo en Debug, `FITD_DEBUGGER`): zonas, cajas, cámara desde arriba. |

### scripts/: la lógica del juego

| Archivo | Para qué sirve |
| --- | --- |
| `life.cpp/.h` | Intérprete de los scripts "Life" (`LISTLIFE.PAK`): puertas, enemigos, objetos, eventos. |
| `evalVar.cpp/.h` | Valores que leen los scripts (distancias, colisiones, objetos encontrados, variables...). |
| `lifeMacroTable.cpp` | Tabla de nombres de las instrucciones de los scripts. |
| `lifeSafety.cpp/.h` | Comprobaciones tras cada instrucción: animación, modelo, fotograma y velocidad válidos. |
| `native/` | Scripts traducidos a C++ para leerlos y estudiarlos (explicado en `native/nativeLife.h`). El juego siempre usa el intérprete. |
| `native/nativeLife.cpp/.h` | Registro, volcado a texto (`LISTLIFE_dump.txt`) y generador del C++. |
| `native/nativeLifeHelpers.h`, `nativeLifeHelpersImpl.cpp` | Funciones que llama el código generado (una por instrucción). |
| `native/nativeLifeScripts_AITD1.cpp` | Registra los scripts generados de AITD1. |
| `native/nativeLifeScripts_generated.cpp` | Los 563 scripts de AITD1 en C++ (generado: no se edita a mano). |

### remaster/: extras propios

| Archivo | Para qué sirve |
| --- | --- |
| `config/remasterHooks.cpp/.h` | Conecta el remaster con el motor: fuentes, menús, teclas F9/F10 y los ganchos de cada sistema. |
| `config/debugCheats.cpp/.h` | F9 (modo depuración, herramientas de desarrollo): ventana para darse objetos. |
| `graphics/customBackgrounds.cpp/.h` | Fondos y máscaras del jugador en lugar de los originales; exportar todos los fondos y máscaras. |
| `graphics/objectFixes.cpp/.h` | Correcciones visuales de objetos 3D (posición, giro, tamaño), cargadas de `assets/objectFixes/`. |
| `ui/advancedOptions.cpp/.h` | Ventana F1: las opciones del launcher dentro del juego, con Controles y Audio. |
| `ui/menuFonts.cpp/.h` | Fuente Georgia de los menús, nítida a cualquier tamaño de ventana. |
| `ui/controllerHint.cpp/.h` | Ayuda de botones del mando en el menú de inicio. |
| `ui/objectEditor.cpp/.h` | F10 (modo depuración, herramientas de desarrollo): editor de las correcciones de objetos. |

### Shared/ y Shared/ui/: lo que comparten juego y launcher

| Archivo | Para qué sirve |
| --- | --- |
| `advancedOptionsText.h` | Textos de las opciones avanzadas en cinco idiomas. |
| `audioVolume.h` | Volúmenes (0-100) y su ganancia. |
| `audioWindow.h` | Ventana "Audio" (música, voces y efectos, narrador). |
| `cdImage.h` | Lee una imagen de CD (`.iso`, `.bin`, `GAME.GOG`): sus archivos y, con su `.cue`, sus pistas. |
| `controlsBindings.h` | Acciones del juego, sus entradas, valores por defecto y su forma de texto en `aitd_remaster.cfg`. |
| `controlsWindow.h` | Ventana "Controles" para cambiar las asignaciones. |
| `narrators.h` | Narradores: cada carpeta de `Voices` con voces sueltas (`.voc`, `.wav`, `.ogg`) o una imagen del CD. |
| `userFolder.h` | Carpeta portable `UserData` junto a los ejecutables. |
| `ui/menuBase.h` | Base de todos los menús: paleta, escala por pantalla, disposición horizontal o vertical, controles para ratón, toque, teclado y mando. |
| `ui/menuGamepad.h` | El mando en los menús (sticks, botones, ayuda tras 4 segundos). |
| `ui/launcherScreen.h` | Pantalla del launcher (solo disposición y dibujo): la usa `Launcher.exe` y, en Android, el propio juego. |
| `ui/launcherText.h` | Textos del launcher en cinco idiomas. |
| `assets/aitd.ico` | Icono del proyecto. |

### Launcher/ y Fitd/

| Archivo | Para qué sirve |
| --- | --- |
| `Launcher/main.cpp` | `Launcher.exe`: ventana, fuentes, pantalla del launcher y lo que hace cada botón. |
| `Launcher/launcherSettings.cpp/.h` | `launcher.cfg`, las líneas que el launcher escribe en `aitd_remaster.cfg` y el arranque de `Tatou.exe`. |
| `Launcher/gameCover.cpp/.h` | Portadas de los juegos (compiladas dentro del exe; en grises sin datos enlazados). |
| `Launcher/folderDialog.cpp/.h` | Selector de carpeta del Explorador de Windows. |
| `Launcher/embedCovers.cmake`, `assets/covers/` | Mete las portadas en `Launcher.exe` al compilar. |
| `Fitd/fitd.cpp` | Punto de entrada de `Tatou.exe`: conecta el remaster y arranca el motor. |
| `Fitd/Tatou.rc` | Icono de `Tatou.exe`. |

### Otras carpetas de FitdLib

| Carpeta | Qué contiene |
| --- | --- |
| `shaders/` | Programas de la tarjeta de video (`.sc`) y su versión compilada para cada API en `generated/`. Para agregar uno: el `.sc`, `addShaderProgram` en `FitdLib/CMakeLists.txt` y sus entradas en `embeddedShaders.cpp` y `embeddedShadersIncludes.h`. |
| `assets/objectFixes/` | Correcciones de objetos de AITD1 y Jack (`aitd1.cfg`, `jack.cfg`), metidas en `Tatou.exe` por `embedObjectFixes.cmake`. |
| `embedded/` | Solo `ControllerHint.png` (la imagen de la ayuda del mando). Nunca datos del juego original. |

## 3. Shaders

| Programa | Para qué sirve |
| --- | --- |
| `background` | Fondo de 320×200 con la paleta. |
| `imageBackground` | Imagen en color (la escena congelada detrás de los menús). |
| `maskBackground` | Máscaras de primer plano. |
| `customBackground`, `customMask` | Fondo y máscaras del jugador. |
| `flat`, `noise`, `ramp`, `sphere` | Polígonos de los modelos: liso, con ruido, degradado y esferas o puntos. |
| `glass` | Cristales transparentes. |
| `ui`, `textSmooth` | Capa de textos del juego; "Texto mejorado". |
| `effectsComposite`, `effectsAntialias`, `effectsBloomDown/Up`, `effectsSsao`, `effectsSsaoBlur`, `effectsSides` | Efectos de imagen: composición final, FXAA, bloom, SSAO y relleno de los lados con la imagen del jugador. |

## 4. Glosario de nombres en francés

El motor conserva los nombres del código original (francés). No se renombran, pero esta tabla ayuda a leerlos.

| Francés | Español | Ejemplos en el código |
| --- | --- | --- |
| `Aff` (afficher) | dibujar / mostrar | `AffObjet`, `AffRect`, `AffBigCadre`, `NbAffObjets`, `GenereAffList` |
| `Gere` (gérer) | manejar / procesar | `GereCollision`, `GereFrappe`, `GereSwitchCamera`, `GereDec` |
| `Genere` (générer) | generar | `GenereActiveList`, `GenereAffList` |
| `Objet` / `ListObjets` | objeto / lista de objetos (actores activos) | `ListObjets`, `ListWorldObjets`, `InitSpecialObjet` |
| `Etage` | piso / planta | `LoadEtage`, `FlagChangeEtage`, `NewNumEtage`, `ETAGExx.PAK` |
| `Salle` | sala / habitación | `ChangeSalle`, `FlagChangeSalle`, `NewNumSalle` |
| `Cadre` | marco | `PtrCadre`, `AffBigCadre` |
| `Frappe` | golpe | `GereFrappe`, `FRAPPE_OK` |
| `Tir` | disparo | `WAIT_TIR_ANIM`, `DO_TIR` |
| `Deplacement` | desplazamiento / movimiento | `InitDeplacement` |
| `Glisser` | deslizar | `Glisser` |
| `Lire` | leer | `Lire` (lector de libros y documentos) |
| `Textes` | textos | `allocTextes` |
| `Perso` (personnage) | personaje | `CHOOSE_PERSO` |
| `AntiRebond` | anti-rebote (evita que una tecla cuente dos veces) | `AntiRebond` |
| `Incrust` (incrustation) | elemento pegado al fondo | `removeFromBGIncrust`, `addActorToBgInscrust` |
| `Chrono` | cronómetro | `startChrono`, `evalChrono` |
| `Life` (vie) | script de comportamiento | `life.cpp`, `LISTLIFE.PAK` |
| `Track` | pista / ruta de movimiento | `track.cpp`, `LISTTRAK.PAK` |
| `ZV` (zone volume) | caja de colisión | `zv.cpp`, `GiveZVObjet` |
| `sceZone` | zona de escenario (activa eventos) | `drawSceZone` |
| `hardCol` | colisión fija del decorado | `drawHardCol` |
| `CVars` | variables configurables del juego (de `DEFINES.ITD`) | `getCVarsIdx` |
| `Tatou` | armadillo (nombre en clave del motor) | `tatou.cpp`, `Tatou.exe` |

## 5. Sobre renombrar variables y funciones

Renombrados en la Fase 4 (nombre viejo → nuevo):

| Antes | Ahora | Dónde | Qué es |
| --- | --- | --- | --- |
| `fontVar1` | `fontDataPtr` | `font.cpp` | Inicio de los datos de la fuente |
| `fontSm1` | `fontCharHeight` | `font.cpp` | Alto de los caracteres |
| `fontSm2` | `fontRowBytes` | `font.cpp` | Bytes por fila de un carácter |
| `fontVar4` | `fontGlyphBitmaps` | `font.cpp` | Mapas de bits de los caracteres |
| `fontVar5` | `fontGlyphTable` | `font.cpp` | Tabla de desplazamiento y ancho por carácter |
| `fontSm3` | `fontDrawColor` | `font.cpp` | Color con que se dibuja |
| `fontVar6` / `fontSm7` | `fontCursorX` / `fontCursorY` | `font.cpp` | Posición donde se dibuja el texto |
| `fontSm8` / `fontSm9` | `fontPixelX` / `fontBitMask` | `font.cpp` | Píxel y bit actuales al dibujar un carácter |
| `computeAngleModificatorToPositionSub1` | `getTargetSideAtAngle` | `track.cpp` | Si el objetivo queda a la izquierda (-1), derecha (1) o delante (0) |
| `initVarsSub1` | `clearMessageTable` | `world.cpp` | Vacía la tabla de mensajes |
| `zvRotSub` | `rotatePointToAnimMove` | `zv.cpp` | Rota un punto y lo deja en `animMoveX/Y/Z` |
| `var_1BA` / `var_1C3` | `lineWidth` / `wordEndChar` | `book.cpp` (`Lire`) | Ancho de la línea / carácter que terminó la palabra |

Las demás funciones `…Sub` (`printTextSub3`, `fadeInSub1`, `drawBgOverlaySub2`…) solo aparecen en comentarios de código viejo. Quedan otras `var_N` locales (`room.cpp`, `save.cpp`, `life.cpp`…) para renombrar cuando se trabaje en esos archivos.

Recomendación para no confundirse sin perder la conexión con el motor original:

- **No renombrar las funciones y variables del motor original** (`AffObjet`, `GereCollision`, `ListObjets`…). Son los nombres de la ingeniería inversa de FITD; cambiarlos haría imposible comparar con FITD y con la documentación existente. En su lugar, usar el glosario de arriba.
- **Sí renombrar** nombres sin significado o engañosos cuando se trabaje en ellos (p. ej. `var_4`, `initVarsSub1`, `processActor2Sub`), y nombres confusos del código del remaster.
- **Nunca mezclar en el mismo commit mover archivos y renombrar.** Primero mover (Fase 4), compilar y probar; renombrar después, en tareas pequeñas.
