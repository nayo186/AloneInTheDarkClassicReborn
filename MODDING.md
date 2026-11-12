# Modding guide

This guide is for anyone who wants to change how the games look or sound: new backgrounds, redrawn scenes, other music, a narrator in another language. Everything you add goes into the **`UserData`** folder, next to `Launcher.exe`. The game files are never changed: to go back to the original, remove your files.

The games: *Alone in the Dark* (folder `AITD1`), *Jack in the Dark* (`JACK`), *Alone in the Dark 2* (`AITD2`) and *Alone in the Dark 3* (`AITD3`).

## What you can change

| What | Games | Where it goes | Section |
| --- | --- | --- | --- |
| Backgrounds (the painted scenes behind the characters) | all | `UserData\<game>\Backgrounds` | [Backgrounds](#backgrounds) |
| Full-screen pictures (title, books, letters, game over...) | all | `UserData\<game>\Backgrounds` | [Full-screen pictures](#full-screen-pictures) |
| Masks (the parts of a background that pass in front of the characters) | all | `UserData\<game>\Masks` | [Masks](#masks) |
| Animated sequences (intros, cut scenes), picture by picture | *Jack*, *AITD2*, *AITD3* | `UserData\<game>\Backgrounds\sequences` | [Animated sequences](#animated-sequences) |
| Music (the CD tracks) | all but *Jack* | `UserData\<game>` | [Music](#music) |
| The narrator's voice (books, letters, notes) | *AITD1*, *AITD2*, *AITD3* | `UserData\AITD1\Voices`, `UserData\AITD2`, `UserData\AITD3` | [Narrator](#narrator) |
| Languages of *Alone in the Dark 3* | *AITD3* | `UserData\AITD3\Languages` | [Languages](#languages) |

**Not yet:** the 3D models and their textures, the sound effects and the texts. Changing the 3D models (exporting them to a common format and bringing them back) is an idea for the future.

## Where your files go

```
UserData/
├── AITD1/                       Alone in the Dark
│   ├── Backgrounds/             backgrounds and full-screen pictures (PNG or JPG)
│   ├── Masks/                   masks (PNG)
│   ├── Exports/                 what F1 exports: the originals, to use as a base
│   ├── Voices/<narrator>/       narrators, one folder each
│   └── 01.ogg ... 16.ogg        music (only the tracks you replace)
├── JACK/                        Jack in the Dark: Backgrounds/, Masks/, Exports/,
│   └── Backgrounds/sequences/   and its animated sequences
├── AITD2/                       Alone in the Dark 2: as AITD1, with sequences,
│                                music and narrator files in the folder itself
└── AITD3/                       Alone in the Dark 3: as AITD2, and
    └── Languages/<language>/    other languages
```

The launcher creates these folders the first time it opens. A mod is shared as these same folders: whoever uses it copies them into their own `UserData` (see [Sharing a mod](#sharing-a-mod)).

## Before you start

1. Switch on **Custom backgrounds** in **F1** (or in the launcher, with its **Options** card). Backgrounds, full-screen pictures, masks and sequences are only used while it is on.
2. In F1, use **Export backgrounds**, **Export masks** and, in *Jack*, *AITD2* and *AITD3*, **Export sequences**. They write the game's own pictures, at their original 320×200, to `UserData\<game>\Exports`: they are your starting point, and their names are the names your files must have.
3. Keep F1's export running until it says it is done: it goes through every floor of the game.

While the game runs, a picture you save is shown at once, no restart needed. `tatou.log` (in `UserData\<game>`) names each of your pictures when it is used, for example `Custom background CAMERA10_000.png (2880x2160, ...)`, and warns about any it cannot use.

## Backgrounds

**Name:** `CAMERAnn_iii`, as exported: `nn` is the floor (the game's `CAMERAnn.PAK` file) and `iii` the picture in it, one per camera. Example: `CAMERA00_000.png`.

**Format:** PNG or JPG (`.png`, `.jpg`, `.jpeg`). JPG is much lighter for big pictures.

**Size:** any, as long as it is not narrower than 4:3:

- **16:10**, the shape of the original 320×200 (1280×800, 2560×1600...), or **4:3** (1600×1200...): the whole picture is the play area;
- **wider than 4:3** (16:9...): its 4:3 centre is the play area and the sides fill the black bars in fullscreen.

A picture narrower than 4:3 is not used (`tatou.log` says so).

**Example:** to repaint the first room of *Alone in the Dark*, take `Exports\CAMERA00_000.png`, enlarge or repaint it, and save it as `UserData\AITD1\Backgrounds\CAMERA00_000.png` (or `.jpg`).

**Keep the layout.** The characters walk on the floor of the original picture, and the objects stand where they stood: your picture must keep the walls, floor and furniture in the same places, or the characters will float or walk through things.

In *Alone in the Dark*, some cameras change their picture after the sorcerer dies: those pictures are `ITD_RESS_nnn` (exported too) and are replaced the same way.

## Full-screen pictures

The pictures that fill the screen (the title, the books and letters, the notebook, the character choice, the game over...) are named `ITD_RESS_nnn`, after the game's `ITD_RESS.PAK` file and their number in it. F1's **Export backgrounds** writes them with the backgrounds. They follow the same rules: same name in `Backgrounds`, PNG or JPG, any size from 4:3.

## Masks

A mask says which part of a background passes in front of the characters: a table, a column, a door frame. The game makes its masks from the original 320×200 picture; when your background moves or redraws such an object, its mask must follow it.

**Name:** `CAMERAnn_iii_mask_r_m.png`, as exported: the background it belongs to, then two numbers the game gives each mask (the room seen from that camera and the mask in that room). Use the exported names as they are.

**What you get from the export:** in `Exports\masks`, each mask as a black and white picture; in `Exports\masks_guides`, the same mask over its background, to see which part it is.

**How it works:**

- **White** is the part that covers the characters; black lets them be seen. The edge is where the grey is halfway, and the game smooths it by one pixel of the screen.
- **PNG only** (a mask's edges must be exact; JPG would blur them).
- **Size:** the shape of the original (16:10, like 320×200, e.g. 1280×800) places it over the scene as the original mask; the shape of your background (e.g. the same 16:9 size as your picture) places it over your background. The simplest is to make it the same size as your background.

**Example:** your new `CAMERA00_000.png` moved a table. Take `Exports\masks\CAMERA00_000_mask_0_1.png`, paint the table white where it now is (and black where it no longer is) at your background's size, and save it as `UserData\AITD1\Masks\CAMERA00_000_mask_0_1.png`.

Masks only matter when your picture moves or redraws an object in front of the characters: with the same layout, the game's own masks keep working.

## Animated sequences

In *Jack in the Dark*, *Alone in the Dark 2* and *Alone in the Dark 3*, the intro and the cut scenes are animated sequences: a series of full-screen pictures. Each one can be redrawn picture by picture.

**Export:** F1's **Export sequences** writes each sequence to `Exports\sequences\<NAME>\`, one picture per frame: `000.png`, `001.png`...

**Replace:** save your frame with the same name in `Backgrounds\sequences\<NAME>\` (PNG or JPG, any size from 4:3). Example: `UserData\AITD2\Backgrounds\sequences\<NAME>\005.png` replaces the sixth frame of that sequence. Frames you do not replace keep the original, so you can redraw only some of them.

## Music

The music of *Alone in the Dark*, *Alone in the Dark 2* and *Alone in the Dark 3* is the CD's audio tracks. Each one can be replaced by a file of yours.

**Name:** the track's number minus 1, with two digits: `01` is the first music track (CD track 2; track 1 of the CD is the data), then `02`, `03`...

**Format:** `.ogg`, `.wav` or `.mp3`.

**Where:** in the game's folder of `UserData`: `UserData\AITD1\05.ogg`, `UserData\AITD2\12.ogg`, `UserData\AITD3\03.mp3`... A track without a file of yours plays as always. (A copy of the game with its tracks as files, `01.wav`..., in its own folder works too; a file of yours in `UserData` is played before it.)

**Which number is which music:** play the game: `tatou.log` notes each track it plays, with the name of its file:

```
[AL] Music: CD track 12 from the CD image
```

means that this music is CD track 12, so your file is `11.ogg` (or `.wav`, `.mp3`). With a file of yours, the line says `Music: CD track 12 from its file (11.ogg, .wav or .mp3)`.

*Alone in the Dark 2* and *3* also read their narrator from CD tracks: those files are explained in [Narrator](#narrator).

*Jack in the Dark* has no CD music: its music is AdLib, and cannot be replaced yet.

## Narrator

- **Alone in the Dark:** each narrator is a folder of `UserData\AITD1\Voices`, with one file per page of the books, letters and notes. The names, formats and the steps to make and choose one are in [Narrators](README.md#narrators) in the README.
- **Alone in the Dark 2 and 3:** the narrator is read from CD tracks; a file of yours in `UserData\AITD2` or `UserData\AITD3` replaces a page. The tables of pages and files are in the README: [Alone in the Dark 2](README.md#alone-in-the-dark-2-1) and [Alone in the Dark 3](README.md#alone-in-the-dark-3-1).

## Languages

*Alone in the Dark 3* can be played in a language from a folder of `UserData\AITD3\Languages`, with the texts, voices and narrator of that language: see [Other languages in Alone in the Dark 3](README.md#other-languages-in-alone-in-the-dark-3) in the README.

## Sharing a mod

- **Share only your own work:** pictures you painted or made, recordings you made. **Never** the files of the game, its pictures exported as they are, its music or its voices: everyone needs their own copy of the game.
- **Share the folders as they go in `UserData`**, so a player can copy them as they are. For example, a pack of backgrounds for *Alone in the Dark*:

  ```
  My HD backgrounds/
  └── AITD1/
      ├── Backgrounds/   CAMERA00_000.jpg, CAMERA00_001.jpg...
      └── Masks/         CAMERA00_000_mask_0_1.png...
  ```

- **Say which game and which edition** it is made for (GOG, Steam, a CD of a given language): the pictures and tracks are the same in all of them, but a player will want to know.
- A short text with what it changes and how to remove it (delete the files) helps players.
