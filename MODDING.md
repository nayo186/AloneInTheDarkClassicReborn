# Modding guide

This guide is for anyone who wants to change how the games look or sound: new backgrounds, redrawn scenes, other music, a narrator in another language. Everything you add goes into the **`UserData`** folder, next to `Launcher.exe`. The game files are never changed: to go back to the original, remove your files.

The games: *Alone in the Dark* (folder `AITD1`), *Jack in the Dark* (`JACK`), *Alone in the Dark 2* (`AITD2`) and *Alone in the Dark 3* (`AITD3`).

There are **two ways** to add your files, and both always work:

- **Folders** (the usual way, best while you are making a mod): your files go loose in the folders of `UserData`, as this guide explains section by section. You can change one picture and see it at once.
- **One file** (best to share a finished mod): the same files gathered in a single **pack**, a `.texture` file for backgrounds and masks or a `.voice` file for a narrator. A player only has to put that one file in its place. See [Packs: one file instead of folders](#packs-one-file-instead-of-folders).

A loose file in a folder always comes before the same file in a pack, so you can still change one picture of a pack you downloaded.

## What you can change

| What | Games | Where it goes | Section |
| --- | --- | --- | --- |
| Backgrounds (the painted scenes behind the characters) | all | `UserData\<game>\Backgrounds` | [Backgrounds](#backgrounds) |
| Full-screen pictures (title, books, letters, game over...) | all | `UserData\<game>\Backgrounds` | [Full-screen pictures](#full-screen-pictures) |
| Masks (the parts of a background that pass in front of the characters) | all | `UserData\<game>\Masks` | [Masks](#masks) |
| Animated sequences (intros, cut scenes), picture by picture | *Jack*, *AITD2*, *AITD3* | `UserData\<game>\Backgrounds\sequences` | [Animated sequences](#animated-sequences) |
| Music (the CD tracks) | all but *Jack* | `UserData\<game>` | [Music](#music) |
| The narrator's voice (books, letters, notes) | *AITD1*, *AITD2*, *AITD3* | `UserData\AITD1\Voices`, `UserData\AITD2\Voices` (or `UserData\AITD2`), `UserData\AITD3` | [Narrator](#narrator) |
| Languages of *Alone in the Dark 3* | *AITD3* | `UserData\AITD3\Languages` | [Languages](#languages) |
| Backgrounds and masks in one file (texture pack) | all | `UserData\<name>.texture` | [Texture packs](#texture-packs-texture) |
| A narrator in one file (narrator pack) | *AITD1*, *AITD2* | `UserData\AITD1\Voices\<narrator>.voice`, `UserData\AITD2\Voices\<narrator>.voice` | [Narrator packs](#narrator-packs-voice) |

**Not yet:** the 3D models and their textures, the sound effects and the texts. Changing the 3D models (exporting them to a common format and bringing them back) is an idea for the future.

## Where your files go

```
UserData/
├── nayo_AITD1V2.texture         texture packs (one file with backgrounds and masks), any name
├── AITD1/                       Alone in the Dark
│   ├── Backgrounds/             backgrounds and full-screen pictures (PNG or JPG)
│   ├── Masks/                   masks (PNG)
│   ├── Exports/                 what F1 exports: the originals, to use as a base
│   ├── Voices/<narrator>/       narrators, one folder each...
│   ├── Voices/<narrator>.voice  ...or one file each (narrator packs)
│   └── 01.ogg ... 16.ogg        music (only the tracks you replace)
├── JACK/                        Jack in the Dark: Backgrounds/, Masks/, Exports/,
│   └── Backgrounds/sequences/   and its animated sequences
├── AITD2/                       Alone in the Dark 2: as AITD1, with sequences,
│   │                            music and page recordings in the folder itself
│   └── Voices/<narrator>/       whole narrators of another CD (or <narrator>.voice)
└── AITD3/                       Alone in the Dark 3: as AITD2, and
    └── Languages/<language>/    other languages
```

The launcher creates these folders the first time it opens (they stay there even if you only use packs). A mod is shared as these same folders, or as a pack: whoever uses it copies them into their own `UserData` (see [Sharing a mod](#sharing-a-mod)).

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

- **Alone in the Dark:** each narrator is a folder of `UserData\AITD1\Voices`, with one file per page of the books, letters and notes, or the same files in one `.voice` file ([Narrator packs](#narrator-packs-voice)). The names, formats and the steps to make and choose one are in [Narrators](README.md#narrators) in the README.
- **Alone in the Dark 2 and 3:** the narrator is read from CD tracks; a file of yours in `UserData\AITD2` or `UserData\AITD3` replaces a page. The tables of pages and files are in the README: [Alone in the Dark 2](README.md#alone-in-the-dark-2-1) and [Alone in the Dark 3](README.md#alone-in-the-dark-3-1).
- **Alone in the Dark 2, a whole narrator:** a narrator of another CD (another language, or a complete dubbing) is a folder of `UserData\AITD2\Voices`, or one `.voice` file there ([Narrator packs](#narrator-packs-voice)), chosen in **Audio** → **Narrator** while *Alone in the Dark 2* is the game chosen (each game keeps its own choice). It replaces the voices of the books and letters, and the characters' voices of the intro and the game; the effects and the music stay the game's. Its files:

  | File | What it is |
  | --- | --- |
  | `books.cfg` | The voice of each page: one line `text page track seconds` per page (e.g. `22 0 23 6`: page 0 of text 22 plays `Track23` for 6 seconds). A page that is not listed is silent. This file is what makes the folder a narrator. |
  | `Track23.ogg` ... `Track64.ogg` | The recordings of the pages, named after the CD track they come from. |
  | `Sample066.ogg` ... | The characters' voices, named after their number in the game's `LISTSAMP.PAK`; each one replaces the game's own. Only the voices are needed, not the effects. |

  `.ogg`, `.wav` or `.voc` files. The seconds of `books.cfg` are those of that CD's recordings: each CD has its own table, in its `AITD2.EXE`. `tatou.log` says it is used: `Narrator: French (narrator pack French.voice)`, `Narrator French: 45 pages with a voice`, and each voice of it `Playing VO: CD track 44 (narrator French)` or `Sample (narrator) 210`.

## Languages

*Alone in the Dark 3* can be played in a language from a folder of `UserData\AITD3\Languages`, with the texts, voices and narrator of that language: see [Other languages in Alone in the Dark 3](README.md#other-languages-in-alone-in-the-dark-3) in the README.

## Packs: one file instead of folders

A **pack** is a single file that holds many others, so a player does not have to copy folders or put each file in its place. It is like a box: the files are kept one after the other, **without compression** (the pictures and voices are already compressed), with a list at the end that says where each one starts. The game reads each file straight from the pack; nothing is extracted, and nothing is written into it.

Inside, it is an ordinary ZIP file with another extension, so it can be opened with 7-Zip or any ZIP program to see what it holds.

**The order** when the same file is in several places:

1. A loose file in its folder (`Backgrounds`, `Masks`, or a narrator folder) comes first.
2. Then the packs, in name order: the first one that has the file is used.

So a pack you downloaded can be corrected with one loose file, and two packs can be combined (the one whose name comes first wins where both have a picture).

**To remove a pack**, delete its file. To stop using it for a while, rename its extension (e.g. `.texture.off`).

### Texture packs (.texture)

A texture pack holds backgrounds, full-screen pictures, masks and sequences of one or more games.

- **Where:** in `UserData` itself (next to the game folders), e.g. `UserData\nayo_AITD1V2.texture`. Any name; putting the game and a version in it helps (`nayo_AITD1V2`).
- **Inside:** the same folders as `UserData`, with the same file names:

  ```
  mypack.texture
  ├── AITD1/Backgrounds/   CAMERA00_000.jpg, ITD_RESS_002.jpg...
  ├── AITD1/Masks/         CAMERA00_000_mask_0_1.png...
  ├── JACK/Backgrounds/    ... and JACK/Backgrounds/sequences/PERE/000.jpg...
  └── JACK/Masks/          ...
  ```

- **Making one:** put your files in the folders of a `UserData` as usual, then `python tools/makeTexturePack.py "<UserData folder>" mypack.texture` (it takes *Alone in the Dark* and *Jack*; add `AITD2` or `AITD3` at the end for those games). With 7-Zip: select the game folders (`AITD1`, `JACK`...), add them to a ZIP with the compression level **Store**, and rename the `.zip` to `.texture`. A compressed ZIP works too, a little slower. It must be under 4 GB.
- **Custom backgrounds** must be on (F1 or the launcher's **Advanced options**), as for loose files.
- `tatou.log` says which packs were found when the game opens: `Texture pack mypack.texture: 876 pictures and masks`.
- **The improved backgrounds** of *Alone in the Dark* and *Jack*, and those of *Alone in the Dark 2*, are texture packs the launcher can download: **Advanced options** → **Improved backgrounds (Alone 1 and Jack)** or **(Alone 2)**, on the page of each game → **Download**. It goes to `UserData` like any other pack, and **Remove** deletes it.

### Narrator packs (.voice)

A narrator pack is one narrator of *Alone in the Dark* or *Alone in the Dark 2* in a single file.

- **Where:** in `UserData\AITD1\Voices` (or `UserData\AITD2\Voices`), as `<narrator>.voice`. The file's name is the narrator's name in the **Narrator** list, as a folder's would be: `Spanish.voice` is shown as *Spanish*, `Spanish (fan dub).voice` as *Spanish (fan dub)*.
- **Inside:** the voice files of a narrator folder, with the same names (`000000.ogg`, `000100.ogg`...; see [Narrators](README.md#narrators) in the README), without folders. `000000` is needed, as in a folder. For *Alone in the Dark 2*, the files of its narrator folder (`books.cfg`, `TrackNN`, `SampleNNN`; see [Narrator](#narrator)), and `books.cfg` is needed.
- **Making one:** `python tools/makeVoicePack.py "<narrator folder>" Spanish.voice`. With 7-Zip: select the voice files (not their folder), add them to a ZIP with the compression level **Store**, and rename the `.zip` to `.voice`. Unlike texture packs, a narrator pack must not be compressed.
- A narrator folder with the same name comes before the file.
- `tatou.log` says where the narrator comes from: `Narrator: Spanish (narrator pack Spanish.voice)`.

## Sharing a mod

- **Share only your own work:** pictures you painted or made, recordings you made. **Never** the files of the game, its pictures exported as they are, its music or its voices: everyone needs their own copy of the game.
- **Share the folders as they go in `UserData`**, so a player can copy them as they are. For example, a pack of backgrounds for *Alone in the Dark*:

  ```
  My HD backgrounds/
  └── AITD1/
      ├── Backgrounds/   CAMERA00_000.jpg, CAMERA00_001.jpg...
      └── Masks/         CAMERA00_000_mask_0_1.png...
  ```

- **Or share one pack:** a `.texture` with your backgrounds and masks, or a `.voice` with your narrator ([Packs](#packs-one-file-instead-of-folders)). It is the simplest for players: one file, in one place.
- **Say which game and which edition** it is made for (GOG, Steam, a CD of a given language): the pictures and tracks are the same in all of them, but a player will want to know.
- A short text with what it changes and how to remove it (delete the files) helps players.
