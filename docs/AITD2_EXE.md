# AITD2.EXE: lo que se sabe del motor original

Notas para quien quiera comparar el motor con el *Alone in the Dark 2* de 1993. El código del juego no está en el repositorio, ni su desensamblado: cada uno lo genera con su propia copia.

```
python tools/disasmExe.py "<carpeta del juego>/INDARK2/AITD2.EXE" <carpeta fuera del repositorio>
```

`AITD2.EXE` (GOG y CD) es un programa de DOS de 16 bits, compilado con Borland C++ 1991 y comprimido con LZEXE 0.90. La herramienta lo descomprime (`aitd2.bin`, 167 936 bytes) y, si hay `objdump` de GNU (o en WSL en Windows), lo desensambla (`aitd2.asm`). Las direcciones de esta página son **lineales**, como en el `.asm`: una llamada lejana a `SEG:OFF` va a `SEG*16+OFF`.

## Datos

| Qué | Dónde | Notas |
|---|---|---|
| Lista de objetos (`ListObjets`) | puntero lejano en `ds:AACB` | 50 objetos de `0xB4` (180) bytes |
| Campos de un objeto | | `+02` modelo, `+04` tipo (flags), `+08` caja (X1, X2, Y1, Y2, Z1, Z2), `+14` recuadro en pantalla, `+1C`/`+20` posición X/Z en la sala, `+30` sala, `+62`/`+66` paso X/Z de la animación |
| Cámara actual | `ds:ABA9` | índice en la tabla de punteros lejanos de `ds:A350` |
| Datos de una cámara | | `+12` número de salas vistas; desde `+14`, una entrada de `0x10` bytes por sala: `+0` sala, `+2` máscaras, `+6` animaciones 2D, `+8` zonas vistas (como `core/floor.cpp`) |
| Altura del suelo | `ds:0A94` | la pone `SET_GROUND` (rutina en `C4D0`, opcode 94 de la tabla `cs:29CF` del intérprete, segmento `9E00`); 100 al empezar la partida |
| Lista de dibujo | `ds:A1BE`, cuántos en `ds:A25E` | la arma `GenereAffList`; se copia a `ds:A132` para ordenarla |

## Rutinas

| Dirección | Qué hace | En nuestro código |
|---|---|---|
| `14031` | si la cámara ve la sala del objeto; deja el índice de la sala vista en `ds:AD90` | `checkActorInRoom` (`core/actors.cpp`) |
| `14088` | si el punto del objeto (posición + paso) cae en una zona vista de esa sala; **sin zonas, no se ve** | `isActorInSeenPart` (`core/actors.cpp`); nosotros sí lo dibujamos sin zonas (BUGS.md A35) |
| `14523` | `GenereAffList`: mete en la lista los objetos con modelo de una sala vista cuyo punto se ve, y marca como quietos (`+04` \|= 8) los que no se animan | `GenereAffList` (`core/actors.cpp`); la prueba de la caja es nuestra (A32) |
| `045F8` | coloca cada animación 2D en el orden de dibujo, justo después del objeto más cercano que cabe en su zona (entradas `0x8000` + número) | `core/anim2d.cpp` |
| `15261` (`1509:01D1`) | prueba de zona: la caja del objeto en décimas cabe entera en uno de los rectángulos | `isBgOverlayRequired` (AITD1), `drawBgOverlay` (`core/sceneDraw.cpp`) |
| `10803` | las máscaras de un objeto: recorte a su recuadro en pantalla y cada máscara cuya zona lo contiene | `drawBgOverlay` (`core/sceneDraw.cpp`) |
| `19F46` (`18D9:11B6`) | fija la altura de corte del renderizador (escribe el valor dentro del propio código, en `19C8E` y `19EBA`): los puntos del modelo más bajos que ella se marcan como fuera (-10000) | `setWaterSurface` (`core/renderer.cpp`, BUGS.md A38) |
| `10C38` | antes de dibujar cada objeto quieto le pasa la altura de corte (10000 si tiene `AF_WATER` (`+04` & 0x400), si no el suelo; los demás objetos, igual en `1141C`); además repinta el fondo: copia el fondo de la cámara y pinta en él los objetos quietos, en el orden de la lista, cada uno seguido de sus máscaras; el resultado pasa a ser el fondo | `drawLaterStillMasks` (`core/sceneDraw.cpp`, BUGS.md A35) |

Cuando se encuentre algo nuevo, se añade aquí con su dirección y el archivo nuestro que lo hace.
