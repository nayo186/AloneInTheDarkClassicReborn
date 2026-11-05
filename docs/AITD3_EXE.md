# AITD3.EXE: lo que se sabe del motor original

Notas para quien quiera comparar el motor con el *Alone in the Dark 3* de 1994. El código del juego no está en el repositorio, ni su desensamblado: cada uno lo genera con su propia copia.

```
python tools/disasmExe.py "<carpeta del juego>/INDARK3/AITD3.EXE" <carpeta fuera del repositorio>
```

`AITD3.EXE` (GOG) es un programa de DOS de 16 bits compilado con Borland C++ 1991, **sin comprimir** (a diferencia de `AITD2.EXE`). La herramienta toma su programa (`aitd3.bin`, 202 208 bytes) y lo desensambla (`aitd3.asm`). Las direcciones de esta página son **lineales**, como en el `.asm`: una llamada lejana a `SEG:OFF` va a `SEG*16+OFF`. El segmento de datos (`ds`) es `22E1`.

Su motor es el de AITD2 casi sin cambios: muchas rutinas son iguales instrucción por instrucción (ver la última tabla).

## Datos

| Qué | Dónde | Notas |
|---|---|---|
| Reloj del juego | interrupción del temporizador en `222FF`, divisor 19886 | 60 marcas por segundo; `ds:140C` |
| Variables de juego (CVars) | `ds:E0E6`, 70 | usa la 0 a la 15: 0 sonido de página, 1 y 5-8 y 10 modelos de los destellos, 2 la variable con la vida del personaje, 3 sonido de golpe, 13 el personaje, 14 el tipo de inventario, 15 la intro en marcha (`AITD3KnownCVars`, BUGS.md T12) |
| Dificultad | `ds:B3A5`, 3 valores | vida, golpes de Carnby, golpes de los enemigos; 0 bajo, 1 medio, 2 alto (BUGS.md T3) |
| Colores de los textos | `ds:C64` | los 16 primeros de la paleta, en 0-63; se ponen tras cargar cada imagen (BUGS.md T10) |
| Duración de cada pista de música | `ds:2EE` | en segundos, una por música (BUGS.md T2) |
| Valores de una partida | tabla en `ds:94` | entradas de 8 bytes (puntero lejano y tamaño): la dificultad, sala, piso, cámara, objetivo, objeto de la cámara, número de objetos, las dos listas de objetos del mundo (7 a 10, con su tamaño calculado en `2D4C`), las variables (140), los objetos en la mano, cuántos objetos, las dos listas de 50, estado y reloj como AITD2, y tres bloques más (`B3AB`, `B39B`, `B085`); cifrado xor 0x35 en `2D0D` (BUGS.md T11) |
| Prioridad de cada sonido | `PRIORITY.ITD` | un canal: un sonido de igual o más prioridad ocupa el canal (`96B8`, `971A`; BUGS.md T9) |

## Rutinas

| Dirección | Qué hace | En nuestro código |
|---|---|---|
| `9783` | música N: pista del CD N + 1, con la duración de `ds:2EE` | `playMusic` (`core/music.cpp`, BUGS.md T2) |
| `181BB` | al dejar el juego: para el CD y los sonidos | `PlayWorld` (`core/mainLoop.cpp`, BUGS.md T4) |
| `15D8D` | secuencia: 5 marcas por imagen | `playSequence` (`core/sequence.cpp`, BUGS.md T5) |
| `7E01` | `PICTURE`: 1 s sin teclas y luego su tiempo; Esc en la intro la termina | `scripts/life.cpp` (BUGS.md T5) |
| `10F33`, `1129E` | menú de dificultad | `core/startupMenu.cpp` (BUGS.md T3) |
| `11205` | menú de inicio; a las 900 marcas sin tocar vuelve la intro (`11286`) | `MainMenu` (`core/startupMenu.cpp`, BUGS.md T7) |
| `106E2` | inventario: marco y dibujos por tipo, escena oscurecida (`1596F`) bajo el marco (`159AD`), textos en los colores 64 y 1 | `core/AITD3.cpp`, `core/inventory.cpp` (BUGS.md T10) |
| `D4F7` | objeto encontrado: sin peso, 40 objetos, no ofrece uno ya cogido (0x8000) | `FoundObjet` (`core/inventory.cpp`, BUGS.md T11) |
| `BB13` y siguientes | la fuerza de un golpe según la dificultad | `difficultyHitForce` (`core/AITD3.cpp`, BUGS.md T3) |
| `166BF` | tabla de tipos de destello (3 a 8) | `AITD3KnownCVars` (BUGS.md T12) |
| `13576`, `13530` | menú de pausa: nueve filas desde y 37 cada 14; "Intermediate sequence" (`ds:C5E`) enciende las animaciones 3D de los menús (`15174` y siguientes) | `core/systemMenu.cpp` (BUGS.md T16) |
| `78D4`, `17DD6` | `READ`: carta (imagen 2), libro (3) o cuaderno (4), con las zonas de texto de AITD2 y los colores de texto de `ds:C64` | `AITD3_ReadBook` (`core/AITD3.cpp`, BUGS.md T17) |
| `7917` | `READ_ON_PICTURE`: imagen con barrido, colores de texto de `ds:C64` y el lector | `scripts/life.cpp` (BUGS.md T17) |
| `1756F` | `Lire`, el lector de textos; voz del narrador por página desde la tabla de `ds:31C` (entradas de 30 bytes: hasta 10 páginas de segundos y pista) | `AITD3_playBookVoice` (BUGS.md T17) |
| `ds:31C` (CD español) | el `AITD3.EXE` del CD español es igual al de GOG salvo los segundos de esa tabla (31 bytes): mismas pistas, leídas por su narrador | `s_bookVoicesAITD3Spanish` (`core/AITD3.cpp`) |

## Igual que AITD2

Rutinas comparadas con `AITD2.EXE` (`docs/AITD2_EXE.md`) por la forma de sus instrucciones (BUGS.md T13):

| AITD2 | AITD3 | Parecido | Qué es |
|---|---|---|---|
| `14031` | `1856D` | igual | si la cámara ve la sala del objeto |
| `14088` | `185C4` | igual | zonas vistas |
| `15261` | `19955` | igual | prueba de zona |
| `44D4` (`364C`) | `9B46` | igual | `GereAnim` (BUGS.md T8) |
| `10C38` | `140D5` | 99 % | objetos quietos y su corte en el suelo |
| `E0AD` (`DFC3`) | `11617` | 95 % | recorridos (seguir a un objeto) |
| `14523` (`1421C`) | `18758` | 94 % | lista de dibujo |
| `10803` | `15617` | lo mismo, escrito en ensamblador | máscaras de un objeto: las de su sala cuya zona lo contiene |
| `6E1E` | `D4F7` | 79 % | objeto encontrado (cambia el peso y el límite) |

Cuando se encuentre algo nuevo, se añade aquí con su dirección y el archivo nuestro que lo hace.
