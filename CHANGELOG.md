# Changelog

Alone in the Dark: Classic Reborn started on 2026-10-23 from [Alone in the Dark: Re-Haunted](https://github.com/spacefarergames/AloneInTheDarkReHaunted) by Spacefarer Retro Remasters, which in turn builds on [FITD](https://github.com/yaz0r/FITD) by yaz0r. The history of those projects is in their repositories.

This repository was published with a single starting commit, to keep it light: the early history carried large files that are no longer part of the project. Below are the changes of each release and, after them, the changes made in this fork before it was published, newest first, one line per change. The reasons behind larger removals are explained in `docs/DESCARTADO.md` and the fixes in `docs/baseline/BUGS.md`.

## v1.6.5 (2026-11-10)

- All games: with the 4:3 aspect ratio, the Integer scaling list only offers 4:3 sizes; 640 × 400 and 960 × 600, which have the shape of 16:10, looked stretched next to them. For square pixels choose 16:10 (issue #7, BUGS.md C69)

## v1.6.4 (2026-11-10)

- All games: **Integer scaling** has its own section in Advanced options. **Size** lists only the sizes that fit your screen (up to 8K), and the one you choose is used; **3D resolution** sets how sharp the 3D models are, with only the resolutions that keep every pixel the same size, down to the original 320 × 200. The normal Resolution, which integer scaling does not use, is greyed out while it is on (issue #7, BUGS.md C68)

## v1.6.3 (2026-11-10)

- All games: new **Integer scaling** option in Advanced options (off by default): every pixel of the game the same size, with black borders around, like pixel-perfect scaling in DOSBox. Automatic (the largest size that fits) or a size of your choice (issue #7)
- All games: the "stone" parts of the 3D models (some furniture, frames, clothes, doors...) have their grain of darker pixels again, as in the original game; it now stays on the object as it moves and turns (BUGS.md C67)
- All games: when the launcher cannot save your options (a read-only or protected folder, such as one guarded by an antivirus), it says so and which file, and you can play anyway with the options you had; it did not start the game (BUGS.md C66)

## v1.6.2 (2026-11-09)

Android phones without Vulkan can play: OpenGL ES works.

- Android: OpenGL ES works. Phones that cannot start Vulkan fall back to it; it showed a black screen, closed in rooms with spheres (*Alone in the Dark 2*), showed the picture upside down for an instant at camera changes and drew *Alone in the Dark 3*'s inventory and the save thumbnails black (BUGS.md C57)

## v1.6.1 (2026-11-09)

Play from your original CD, a check of the game files, controller vibration, the AdLib music of *Alone in the Dark* without the CD, infinite life in *Alone in the Dark 3*, and a modding guide.

- All games: play from your original CD as an image (`.cue` and `.bin`, also one `.bin` per track, or `.iso`), read straight from the image with its music; a CD edition we have not checked is said in red, with a warning before playing (see the README, BUGS.md C61, issue #6)
- All games: the launcher checks the files of the game folder (a copy, GOG or Steam, or a CD image) against the editions we know, with a window while it checks: a missing file or one of no known edition is said in red, with a warning before playing; a changed file can be accepted for good. `launcher.log` and `tatou.log` list each one (see the README, BUGS.md C62)
- All games: controller vibration, a little when hit or shooting (at each shot of a weapon that fires several times) and more on dying; on by default, it can be turned off in Controls (BUGS.md C63)
- Alone in the Dark 3: infinite life works too; the deaths of the story (being swallowed, a fall, the gunman of the mine) still kill (BUGS.md C63)
- Alone in the Dark: without the CD tracks, each music plays its AdLib piece, matched by ear, or silence; it played pieces of other scenes, or nothing (BUGS.md C23)
- All games: a copy of the game with its music tracks as files (`01.wav`...) in its folder plays them; they were only found in debug mode (BUGS.md C64)
- A modding guide, `MODDING.md`: backgrounds, full-screen pictures, masks, animated sequences, music and narrators, with their names and sizes, and how to share a mod. `tatou.log` names the file of each music track played

## v1.6.0 (2026-11-08)

Classic Reborn arrives on **macOS** and **Android**, both experimental: the same four games, from the same code as Windows and Linux. Reports from Mac and Android players are very welcome.

- macOS (experimental): one package for every Mac with macOS 11 or newer, Apple Silicon and Intel, drawn with Metal; the launcher updates it as on Windows and Linux (see the README, BUGS.md C56)
- Android (experimental): an app for Android 7 or newer (64-bit, Vulkan), with the launcher inside, which turns with the phone, while the game is always on its side (see the README, BUGS.md C58)
- Android: on-screen controls as an Xbox controller, with the same buttons and meaning as a controller on the PC (D-pad as tank controls, stick as the modern left stick); only the buttons with an action show, and the Controls window changes them too (BUGS.md C58)
- Android: **Import...** brings the game to the phone without a cable: choose your GOG or Steam folder, a game's folder of `UserData` from the PC or the whole `UserData`, and the app copies, in one go, the game and the languages, narrators, backgrounds, masks and saves it has (BUGS.md C58)
- Android: the launcher's updates download the new app and Android asks to install it; games, saves and options are kept (BUGS.md C58)
- All games: two more resolutions, 1440 x 1080 (4:3 at Full HD height) and the games' original 320 x 200, stretched to the screen as in 1992 (BUGS.md C53, C54)
- All games: when a file of the game is missing, a message says which one and the game closes cleanly; it closed without a word. An empty `GAME` folder is no longer taken as the game's folder (BUGS.md C55)

## v1.5.0 (2026-11-08)

*Alone in the Dark 3* has been played from start to end with no errors left: all four games (*Alone in the Dark*, *Jack in the Dark*, *Alone in the Dark 2* and *Alone in the Dark 3*) can now be played from start to end. This version adds its other languages and a portable folder for each game.

- Alone in the Dark 3: playable from start to end (README)
- Alone in the Dark 3: works with the GOG and Steam editions in all their languages (English, French, German, Italian): the launcher offers the installed one, and the narrator follows the timing of each language (BUGS.md T46, T47)
- Alone in the Dark 3: any of its languages over your installation, from a folder in `UserData\AITD3\Languages` with the files of that language (texts, voices, narrator); a folder that lacks a needed file is not offered; saves of the French version and of the others are kept apart, as in the original versions (see the README, BUGS.md T48)
- Alone in the Dark 3: the door of floor 13 and the one at the end of its corridor no longer go through the walls when they open (BUGS.md T43, T44)
- Alone in the Dark 3: as the cougar, the inventory has only the cougar's actions, as in the original; it kept Carnby's objects with the cougar's actions among them (BUGS.md T45)
- All games: each game can be kept inside `UserData`, in its own `GAME` folder (only the files the game uses are needed); the launcher takes it when no folder is chosen or the chosen one is gone (see the README, BUGS.md C52)

## v1.4.5 (2026-11-07)

More fixes for *Alone in the Dark 3*, and one for every game when loading a saved game.

- All games: loading a saved game from the pause menu no longer makes Carnby jump, aim or attack with the key that confirmed the load (BUGS.md C51)
- Alone in the Dark 3: the heartbeat of the dark cellar sounds and stops when the lighter is lit, as in the original; it was sometimes silent and sometimes never stopped (BUGS.md T40)
- Alone in the Dark 3: the left leaf of the double door of floor 9 no longer goes into the wall when it opens: it opens as far as the right one (BUGS.md T41)
- Alone in the Dark 3: putting the hat on the statue gives the ammunition and Carnby moves again at once, at his usual speed; he stayed stuck until the next scene (BUGS.md T42)

## v1.4.4 (2026-11-07)

More fixes for *Alone in the Dark 3*: a crash, the balcony and a hole in the floor.

- Alone in the Dark 3: the game no longer closes when a script takes away an object past the 30th place of the inventory, as when Carnby dies from a fall (BUGS.md T37)
- Alone in the Dark 3: the baluster of the balcony is no longer hidden by the plank and the shutter the enemy opens; it only showed where Carnby walked by (BUGS.md T38)
- Alone in the Dark 3: Carnby falls through the hole in the floor to the room below, as in the original; he only waved his arms at the edge (BUGS.md T39)

## v1.4.3 (2026-11-07)

More fixes for *Alone in the Dark 3*, and its HD pack.

- Alone in the Dark 3: the wall blown up with the dynamite is shown broken, as in the original; Carnby could walk through it but the picture kept it whole (BUGS.md T33)
- Alone in the Dark 3: bullets for the Winchester are kept when it is already loaded, with the game's own "already loaded" message; they were used up for nothing (BUGS.md T34)
- Alone in the Dark 3: the ghost that appears by the locked door when its lamp is lit can be seen; it was drawn too dark to notice (BUGS.md T35)
- Alone in the Dark 3: messages stay half a second longer, to give time to read them (BUGS.md T36)
- README: an HD pack for Alone in the Dark 3, and two pictures of it

## v1.4.2 (2026-11-06)

Fixes for *Alone in the Dark 3*: searching, the whip and the Lone Miner's shots.

- Alone in the Dark 3: Carnby takes what he searches for when it is in front of him, as in the original: the sheriff's star and the Winchester of the sheriff's office could not be taken (BUGS.md T30)
- Alone in the Dark 3: the whip works again after Carnby is shot; it stayed in a wrong stance and did nothing until he moved (BUGS.md T31)
- Alone in the Dark 3: the Lone Miner's shots make a sound, as the other gunmen's; the original game had none (BUGS.md T32)

## v1.4.1 (2026-11-06)

More for *Alone in the Dark 3*: its map, custom pictures and a fix in the barrel cellar.

- Alone in the Dark 3: Tab (Y on the controller) opens the map of the town, with the arrow blinking where Carnby is, as in the original; only while playing, not during the intro or a scene (BUGS.md T28)
- Alone in the Dark 3: its backgrounds, full-screen pictures, masks and sequences can be exported and replaced from F1, as in the other games (BUGS.md T27)
- Alone in the Dark 3: the big snake of the barrel cellar no longer shows over its barrel when it strikes (BUGS.md T29)

## v1.4.0 (2026-11-05)

*Alone in the Dark 3* can now be played in the same engine as the other games, with its own page in the launcher. It is being polished: if something behaves differently from the original, please open an issue. Its fixes follow `AITD3.EXE`, the game's own program, and are listed in `docs/baseline/BUGS.md` (T1 to T26); what is known of it is in `docs/AITD3_EXE.md`.

- Alone in the Dark 3: playable with the GOG edition (the `INDARK3` folder and its music in `MUSIC`) or a copy of its files, with `Tatou.exe --game aitd3` and its own tab in the launcher
- Alone in the Dark 3: its menus as in the original: the main menu with four entries and the difficulty, the pause menu on its option screen with "Difficulty Level", and the intro again after 15 seconds on the main menu (BUGS.md T1, T3, T7, T16)
- Alone in the Dark 3: the difficulty changes the hero's life and the hits, as in the original (BUGS.md T3)
- Alone in the Dark 3: the music plays the right CD track and stops on leaving the game; the intro keeps the original timing and Esc ends it (BUGS.md T2, T4, T5)
- Alone in the Dark 3: no dark brown box around the text of the intro's pictures, black as in the original (BUGS.md T6)
- Alone in the Dark 3: the street preacher stays down after melting, the street shows again after the bridge sequence, and his match no longer sounds in a loop (BUGS.md T8, T9)
- Alone in the Dark 3: its inventory (frames, objects and colours), with Carnby and the others in its darkened scene, no weight limit, 40 objects and the last one of the list no longer cut (BUGS.md T10, T11, T15, T19)
- Alone in the Dark 3: letters and books open, with their colours, the narrator of the CD, the page sound and the page turn; the story pictures (such as the piano's) are read with their voices (BUGS.md T17, T18, T26)
- Alone in the Dark 3: saves in the layout of the GOG edition, in both directions (BUGS.md T11)
- Alone in the Dark 3: the rules of the engine found in Alone in the Dark 2 (what each camera shows, masks, ladders, pushing, dropping objects, followers...) and its own table of game variables (BUGS.md T12, T13)
- Alone in the Dark 3: Carnby no longer stops at invisible posts (BUGS.md T22)
- Alone in the Dark 3: the gunman of the saloon's balcony shoots down at Carnby (BUGS.md T24)
- Alone in the Dark 3: the horse in the street no longer disappears behind the walls for a moment (BUGS.md T25)
- Alone in the Dark 3 in Spanish, from the original Spanish CD over the GOG edition: texts, voices of the scenes, intro and narrator, with the CD image or its files in `UserData\AITD3\Languages\Spanish` (see the README)
- All games: no flash of the screen when choosing an object in the inventory (BUGS.md T20)
- All games: the hero slides along the corners of walls and furniture instead of getting stuck (BUGS.md T21)
- All games: a thrown object always lands where the hero can reach it to pick it up again (BUGS.md T23)
- CD images in MODE2 are read too (narrators of Alone in the Dark and the language CD of Alone in the Dark 3)

## v1.3.3 (2026-11-04)

All three games are now playable from start to end: *Alone in the Dark*, *Jack in the Dark* and *Alone in the Dark 2*. One last fix for the ship of *Alone in the Dark 2*.

- AITD2: an object of the room behind a wall is no longer drawn through it: the cannon, turned after the shot, showed by the bed post of a cabin (BUGS.md A55)

## v1.3.2 (2026-11-03)

More fixes for *Alone in the Dark 2* on the ship: the barred trapdoor, the sleeping pirates, the cannon and the destroyed dormitory, and a repeated object in the inventory.

- An object given again to a character who already has it no longer shows twice in the inventory, as Carnby's "Actions" after the irons; saves that have it twice are fixed when loaded (BUGS.md A50)
- AITD2: Carnby no longer gets stuck by the barred trapdoor: its message shows once and he can walk away (BUGS.md A51)
- AITD2: entering a room starts its time again for the character who enters: the sleeping pirates of the ship no longer wake as soon as Carnby comes in (BUGS.md A52)
- AITD2: after the cannon shot the pirates' dormitory shows its destroyed pictures, as in the original (BUGS.md A53)
- AITD2: the ship's cannon keeps the side it was turned to when the fuse is put in (BUGS.md A54)

## v1.3.1 (2026-11-03)

More fixes for *Alone in the Dark 2*, in the kitchen, the ship and the irons, and a crash at a door after them.

- Alone in the Dark 2: the kitchen cupboard no longer opens and closes over and over when Grace stands in front of it (BUGS.md A45)
- The text of the inventory no longer stays over the "You find" window or over a sequence started from it (BUGS.md A46)
- Alone in the Dark 2: the kitchen's dumbwaiter door opens when Grace rings the bell (BUGS.md A47)
- Alone in the Dark 2: Carnby can take the key of the irons after a few pulls, as in the original (BUGS.md A48)
- No crash when an object without an animation changes its body, as at a door after the irons (BUGS.md A49)

## v1.3.0 (2026-11-03)

Two new options for every game: a quick turn and CRT scanlines. Also more fixes for *Alone in the Dark 2*.

- Scanlines, a new option in Effects (launcher and F1): thin dark lines over the picture, like an old CRT screen, with an intensity slider from 1 to 4, in all three games
- Quick turn, as in the Resident Evil HD remaster: back and run pressed together turn the character half round, smoothly, in all three games
- Alone in the Dark 2: the dumbwaiter's door on the ship stays open instead of opening and closing over and over (BUGS.md A43)
- Alone in the Dark 2: Carnby no longer shows for an instant when Grace fires the small cannon (BUGS.md A44)

## v1.2.3 (2026-11-03)

The found-object menu no longer sets off the character's action, and objects of the next room no longer show through the walls in *Alone in the Dark 2*.

- Choosing Take or Leave for a found object no longer makes the character do their action as well (guard, weapon up, Grace's spins) (BUGS.md A41)
- Alone in the Dark 2: objects of a room behind a wall no longer show through it, such as by the bed post in a cabin of the ship (BUGS.md A42)

## v1.2.2 (2026-11-03)

More fixes for *Alone in the Dark 2*: games saved with Grace load in the original, and Grace feeds the parrot without going round the cabin.

- Alone in the Dark 2: a game saved here with Grace loads in the original (GOG) with Grace's inventory and actions, not Carnby's (BUGS.md A39)
- Alone in the Dark 2: Grace walks straight to the parrot's perch when she feeds it, instead of going round the cabin for a minute (BUGS.md A40)

## v1.2.1 (2026-11-03)

More fixes for *Alone in the Dark 2*: objects near the camera, the car's brakes and the statue in the garden.

- Alone in the Dark 2: still objects near the camera, such as a small chest or an open door, are no longer cut by the walls in front of objects behind them (BUGS.md A36)
- Alone in the Dark 2: the brakes of the car that takes Grace away sound when it stops at the gazebo (BUGS.md A37)
- Alone in the Dark 2: the statue and the zombie rise from the lawn, as in the original, instead of coming up whole from below the screen (BUGS.md A38)

## v1.2.0 (2026-11-03)

*Alone in the Dark 2* can now be played through Grace's part: her own inventory, the car's trunk, and fixes to the pirate's story and the hall.

- Alone in the Dark 2: the narrator of the pirate's story finishes each text (BUGS.md A34)
- Alone in the Dark 2: Grace has her own inventory, apart from Carnby's (BUGS.md A34)
- Alone in the Dark 2: Grace can get into the car's trunk (BUGS.md A34)
- Alone in the Dark 2: the woman in purple waiting behind the hall doors no longer shows through the wall (BUGS.md A35)
- Developers: `tools/disasmExe.py` disassembles AITD2.EXE from your own copy, and `docs/AITD2_EXE.md` lists what is known of the original engine

## v1.1.3 (2026-11-02)

More fixes for *Alone in the Dark 2* found by players, and a safer exit from the game menu.

- Alone in the Dark 2: the cook is seen climbing the red stairs instead of vanishing at their foot (BUGS.md A32)
- Alone in the Dark 2: the story pictures with text use your own enlarged pictures too (BUGS.md A33)
- Alone in the Dark 2: no more flash of a wrongly coloured picture between the story pictures and when opening books (also Jack in the Dark) (BUGS.md A33)
- Alone in the Dark 2: the game no longer closes at the end of the pirate's story (BUGS.md A33)
- Alone in the Dark 2: Carnby moves again right after using the key on the floor 6 door, and the door opens as soon as he touches it (BUGS.md A33)
- Quitting from the game menu no longer crashes while music plays (all games) (BUGS.md A33)

## v1.1.2 (2026-11-02)

A fix for *Alone in the Dark 2* found by players.

- Alone in the Dark 2: running no longer goes through the closed door of the card room before the four diamonds are set (BUGS.md A31)

## v1.1.1 (2026-11-02)

Fixes for *Alone in the Dark 2* found by players, and an About window in the launcher.

- Alone in the Dark 2: an object is only dropped where it fits, as in the original ("There's no room" otherwise): the Jack of diamonds no longer falls to the ground in the maze (BUGS.md A28)
- Alone in the Dark 2: books, letters and notebooks fit as many lines per page as in the original (BUGS.md A29)
- Alone in the Dark 2: the door opened with the key no longer gets stuck before opening (BUGS.md A30)
- Launcher: an About button (bottom right) with the project, its author, credits, thanks to the testers and a link to GitHub

## v1.1.0 (2026-11-02)

**Alone in the Dark 2 is playable**, with the same engine, launcher and options as the first game.

- Alone in the Dark 2: its own launcher tab, data folder (GOG or Steam installation, or a copy of the files) and saves (UserData\AITD2), in the GOG edition's format: saves cross between both ways (BUGS.md A6)
- Alone in the Dark 2: menus, message and book frames, the intro's fades and text colours, the opening gate, the house front's windows; its animated sequences and pictures can be exported and replaced like the backgrounds (BUGS.md A1-A4, A10)
- Alone in the Dark 2, books and letters: the narrator reads them from the CD's tracks, as the CD version did, and each kind of paper uses its own text area (BUGS.md A18)
- Alone in the Dark 2, gameplay: the weapon in hand is the one chosen; no red bubbles on wounds; the muzzle flash; infinite life (F1); a weapon lost in the cave cannot be fired; the cave elevator works; the capture in the garden corner goes on (BUGS.md A7-A9, A11, A12, A14, A15)
- Alone in the Dark 2, drawing: the waterfall of the cave no longer covers Carnby; a pirate of another corridor of the hedge maze no longer shows over the hedges; Carnby is seen climbing the hall stairs (BUGS.md A13, A16, A25)
- Alone in the Dark 2: the scripts of the other characters run at the same pace at any frame rate, so the maze pirates no longer fire too often; they still see every event of the frames they skip, so the intro's scenes and the chest that hides the Jack of diamonds work (BUGS.md A17, A20, A27)
- Alone in the Dark 2: a closed trapdoor can no longer be climbed through; the chest scrapes when pushed (BUGS.md A19, A20)
- Alone in the Dark 2: ESC ends the intro scene, as Enter does, instead of opening the pause menu inside it (BUGS.md A21)
- Alone in the Dark 2: the save list and the pause menu fit the game's frames (BUGS.md A22)
- All games: loading a game from the pause menu (ESC), or starting or loading one from the main menu, no longer opens the inventory at once (BUGS.md A23, A24, A26)
- README: Alone in the Dark 2 in every section, its screenshots, its HD pack and how to dub its narrator
- tatou.log notes when the inventory opens and each life change of an object

## v1.0.1 (2026-10-31)

- Jack in the Dark, ending: Santa waving goodbye plays again (PERE.PAK), followed by the closing pictures; animated sequences run at the same speed at any frame rate (BUGS.md J2)
- Jack in the Dark: objects of a neighbouring room no longer show through the walls, like the dolls guarding the cell seen from the toy chest's room (BUGS.md J4)
- tatou.log notes every sound that starts, is skipped or is stopped, to find sounds that cut out

## v1.0.0 (2026-10-31)

- Ending: the burning tree plays again (END_SEQUENCE, ENDSEQ.PAK): the tree in fire colours, its flames and the screen filling with yellow, orange and red; a SHAKING lasts until SHAKING(0), as in the original, so the ground keeps shaking until the player gets away; the cavern water no longer disappears when a shake stops; the sorcerer's scream is louder (BUGS.md C50)
- Books and notes draw their illustrations (#G, ITD_RESS 9): the sacrificial dagger, the plantation map and others (BUGS.md C48)
- Messages ("The lamp is empty", "This room is dark"...) stay on screen one second longer and last the same at any frame rate (BUGS.md C49)

## v1.0.0-beta.10 (2026-10-31)

- Library: the talisman's looping sound stops when the player leaves the library or takes the talisman, instead of playing on in every room; a looping sound a script keeps asking for stops when it no longer asks (BUGS.md C47)

## v1.0.0-beta.9 (2026-10-31)

- Attic stairs: above 60 FPS (uncapped, 120/144 Hz monitors) Carnby walked back up the stairs right after going down; scripted walks now advance at the same pace at any frame rate (BUGS.md V29, issue #4)
- Dark caverns seen from above: a circle of light follows the player, as in the original, instead of a black screen; going back to the previous room no longer turns everything red (BUGS.md V28)
- Cavern water hides what is below its surface: the player from the hips down, and the water monster sinks into it (BUGS.md V26)
- No more screen shake on entering the caverns (BUGS.md V25)
- An enemy or the player reaching a wall by its corner slides along it instead of stopping: the flying enemy of the caverns flies from the start, and the water monster no longer gets stuck (BUGS.md V27)
- French narrator: the pages of Story of a Louisiana Plantation after its empty page are read in French, with their own recording (BUGS.md C46; README, Dubbing)
- tatou.log notes a change of screen shake once, not every frame

## v1.0.0-beta.8 (2026-10-30)

- Updates from the launcher: when it opens, it looks for a newer release on GitHub and offers to install it; the Update button next to the version (bottom right) looks again. Only the programs and documents are replaced, never UserData (options, saves, backgrounds) or launcher.cfg. The launcher connects to the internet only for this; the game never does (README, Updates)
- The launcher shows its version at the bottom right
- Each game can be started without the launcher, for example from a frontend: Tatou --game aitd1 or Tatou --game jack uses the folder, language and options chosen in the launcher; --data "<folder>" and --lang are optional (README, Starting a game without the launcher)

## v1.0.0-beta.7 (2026-10-30)

- Cigar smoke, next to the dining room: a grey cloud turns around the cigars, grows and shrinks, and makes the player cough and lose life while inside it; the water jug puts it out, as in the original (BUGS.md C41, C42)
- Found objects: when an object does not fit in the inventory (weight or number), the game says "You cannot carry anything else" and Enter or Esc closes it, instead of a window without the Leave and Take options that let the limit be skipped (BUGS.md C40); also in Jack in the Dark
- An object's masks (the parts of the background in front of it) cover only that object: the seated dead man in the dining room no longer disappears when the door is opened (BUGS.md C43)
- Drawing order: objects are only ordered when they meet on screen; a door no longer cuts through the player standing next to it (BUGS.md C44)
- Thin parts of the 3D models (chair bars) are drawn one game pixel wide instead of looking like a thin wire mesh (BUGS.md V24)
- tatou.log notes any drawing-order contradiction ([ORDER]), to help with bug reports

## v1.0.0-beta.6 (2026-10-29)

- Save pictures (SAVEn.png) are kept at 320x240 instead of the screen's size: about 100 KB each instead of 2-3 MB, and a little sharper in the save list (BUGS.md C39). Old pictures are replaced when that slot is saved again

## v1.0.0-beta.5 (2026-10-29)

- Linux version: ClassicReborn-<version>-linux-x64.tar.gz, for Ubuntu 22.04, Linux Mint 21, Debian 12 and newer (64-bit), with the GOG or Steam installation (Heroic, Lutris, Proton) or a copy of the game files; nothing else to install (README, On Linux)
- Camera changes with HD backgrounds no longer stall: the backgrounds of the room are decoded ahead in the background (BUGS.md L9)
- Save list: the date and time of each save, right after its slot, were hidden behind the preview (BUGS.md C38)
- The 3D models' gradients and spheres no longer read the palette in the wrong format, which Vulkan forbids (BUGS.md L7)
- Menus use Gelasio, a free font made to Georgia's measures, where the system has no Georgia (Linux)
- The packages carry Gelasio's license (Gelasio-OFL.txt)

## v1.0.0-beta.4 (2026-10-29)

- Modern left stick: the character walks where the stick points, seen from the camera, keeping the direction through camera cuts, like the Resident Evil HD remasters; the D-pad and the keys stay tank controls. Option in Controls (launcher and F1), on by default
- Cheats window in F1: "Enable cheats" and infinite life (a hit never takes the player's life; traps still kill), for AITD1 and Jack in the Dark

## v1.0.0-beta.3 (2026-10-28)

- Narrators: the French CD's narrator (from the player's own CD: voice files, .iso or .bin) or a fans' dubbing (.wav, .ogg, .voc), chosen in the Audio window, with any text language (README, Narrators)
- A book page always plays its whole narration; with some text languages the end of the first page was cut (BUGS.md C35)
- After leaving an object at your feet, the "take it?" window no longer comes back while you walk away (BUGS.md C36)
- The lamp keeps spending oil in the hand after being dropped or stored and taken again (BUGS.md C37)
- One CD image reader for the narrators and the GOG/Steam CD image

## v1.0.0-beta.2 (2026-10-28)

- Background music plays again when a piece ends: the music queued by the scripts follows (after a monster dies, after a scare) (BUGS.md C32)
- The music stops when restarting, dying or going back to the title screen (BUGS.md C33)
- Random events (ambient music and sounds, monsters) differ in every session instead of repeating (BUGS.md C34)
- Advanced options (launcher and F1): scroll bar when the window is too small to show everything

## v1.0.0-beta.1 (2026-10-28)

First public release.

## 2026-10-27

- New README and CONTRIBUTING; player folders in English (Backgrounds, Masks, Exports)
- Remove obsolete documents, game captures and scripts before the public export (block E)
- Ignore Python caches
- Dictionary of methods generated from the headers; layer checker; declarations without code removed (block D, part 2)
- Code map rewritten after the cleanup; ReShade crash handler and textured-model leftovers removed (block D, part 1)
- util depends on nothing; basic types in util/types.h, FITD debugger in core (block C, part 4)
- Custom backgrounds reach the renderer through platform hooks; render pipeline files move to platform (block C, part 3)
- Settings move to platform; platform announces menus and keys through hooks (block C, parts 1-2)
- Script patches reduced to four safety checks; switched-off workarounds removed (block B, part 3)
- Remove unused functions and files (block B, part 2)
- Native life scripts in their own folder, explained; developer settings documented
- Remove the remaster's HD mode and everything that only ran in it (block B, part 1)
- Clean the solution: only the projects and files in use (block A)
- Save list: date and time of each save in tiny digits
- AITD1 saves in the GOG layout; loading restores the clock and the lamp keeps spending oil
- The controller hides the mouse cursor in the game and F1 until the mouse moves
- Drop the buttons and paths of the picture folders (fixed folders, README later)
- Automatic help only while the controller or keyboard is in use (BUGS.md C27)
- Jack in the Dark: export every mask (MASKnn) and its full-screen pictures
- Object fixes ship inside Tatou.exe; editor and F9 only in development builds
- Object editor (F10, debug mode): visual position, turn and size fixes
- Saved fullscreen applies to every game and is saved at once (BUGS.md C26)
- Drop the double-click fullscreen switch
- Controller in every menu: sticks, B to go back, LB/RB tabs, delayed help
- Responsive F1: shared menu metrics, style and sharp launcher fonts in the game
- Responsive launcher: shared screen with landscape and portrait layouts
- "Options window" control: F1, Home and Back, editable like the others
- The game waits everywhere while F1 is open and takes no input from it
- Shared menu base (Shared/ui/menuBase.h) and controller navigation in F1
- Debug mode only by hand in launcher.cfg; drop the separate guide files
- Shorten the help texts and fit the data folder line in the launcher
- Accept the Steam installation (GAME.INST, same edition as GOG)
- Add a debug mode and a portable UserData folder next to the launcher
- Read the CD music and the voices straight from GOG's GAME.GOG
- Run from a GOG installation as well as from a copy of the data
- Add music and voices-and-effects volume controls to F1 and the launcher
- Bring the sound back to the GOG level
- Rebuild the AdLib music output for SoLoud (Jack in the Dark)
- Keep the 3D models out of the bloom
- Start Jack in the Dark from the launcher and with --game jack
- Stop looping samples on game over, floor change and full-screen pictures

## 2026-10-26

- Add a controls window to F1 and the launcher, without quick turns or map
- Rebuild the game controls from scratch, with RT to run
- Fix a stack overflow in loadPak with an index past the end of a PAK
- Add a debug item window (F9) for testing
- Make dark rooms dark, with their message (BUGS.md V23)
- Add "Enhanced text": the game's letters rebuilt at screen resolution
- Antialiasing: FXAA only; drop-down lists never cover the "(?)"
- Add antialiasing of the 3D models (FXAA) as an advanced option
- Antialias the edge of the player's masks over one screen pixel
- Export every mask of the game at once for batch enlarging
- Docs: park the soft mask edges, 3D antialiasing next
- Draw objects in a camera-based order (BUGS.md V22)
- Remove the DOSBox stub, Steam/GOG deployment, Steam overlay and updater
- Use the project's own icon for the game and the launcher
- Launcher: one page per game with embedded 3:4 covers, new name
- BUGS.md: C17 tested and resolved, C18 reviewed (not audible)
- Fix audio playing slower on 48 kHz sound devices
- Remove the remaster's HD screens (stage 2)
- Add custom full-screen pictures and rebuild the controller help
- Remove the remaster's HD camera backgrounds (stage 1)
- Add custom backgrounds and masks, and real-colour glass

## 2026-10-25

- Keep the advanced options chosen in F1 and in the launcher (C15)
- Add aspect ratio, resolution and fullscreen options
- Record the pending window cut (V22) and the discarded attempts
- Record discarded code and attempts in docs/DESCARTADO.md
- Draw glass with the original palette rule (V20)
- Draw objects in the original painter's order (V21)
- Make per-frame animations follow the game clock (V19)
- Fix the crash when closing the window with music playing (C14)
- Document the screen effects and the F1 window (C13)
- Launcher: effect options with help markers
- Rewrite the screen effects and add the F1 options window
- Stop all samples when a game starts
- Fix random crash loading the tatou model (V18)
- Document the launcher options and the game clock fix (V17)
- Launcher: default settings button and advanced options (FPS, VSync)
- Add graphics.frameLimit and graphics.vsync options
- Make the game clock follow real time, not the frame rate (V17)
- Update C1, C4 and H3 in the bug list
- Document the remaster workarounds switched off (C12)
- Switch off three more remaster workarounds (C12)
- Document the cause and fix of V16 (character select after Esc)
- Fix the character select screen after Esc during a story (V16)
- Document the modelAtlas cleanup (C2)
- Remove the unused duplicate of modelAtlas (C2)
- Document C5, C9, C10 and the decisions on V7 and V13
- Switch off the background dump by default (C5)
- Fix the crash when closing the window and the frozen window (C9, C10)
- Document the cause and fix of V15 and the resolution findings
- Grow original masks by half a 320x200 pixel (V15)
- Note V15: thin lines over walls (pending, first attempt undone)
- Document the cause and fix of the intro car flicker (V14)
- Repaint the last full image when a frame has no 3D (V14)
- Note V14: intro car sound fixed, car flicker pending
- Play the queued sample of SAMPLE_THEN and cut the looping one (V14)

## 2026-10-24

- Switch off the remaster's stuck-enemy workarounds (V4)
- Document the cause and fix of V4 (monster stuck in the window)
- Let actors already inside a wall box move out of it (V4)
- Document V13 (thrown objects) and explain V1 (lamp over the chair)
- Fix thrown objects: flight direction, landing spot and wall check (V13)
- Keep incrusted objects visible under masks of moving actors (Original mode)
- Record V12 investigation notes
- Note the lamp hidden by the table mask (V12)
- Fade the composited image so fades no longer flicker
- Fix ESC in books and inventory, faithful pause menu in Original mode
- Make the launcher the Visual Studio startup project
- Add Launcher.exe: choose game, mode and language, then start the game
- Add Original mode: the game without remaster effects
- Build Tatou.exe without a console window except in Debug
- Add launcher command-line options and remove the in-game launcher
- Document file splits and renames in the code map
- Rename meaningless font variables, Sub helpers and Lire locals
- Split remaster screens out of platform/rendererBGFX.cpp
- Move actor shadow drawing to remaster/graphics/shadows
- Split core/main.cpp into topic files
- Update docs after block C
- Fix Lire reading past the end of text when the last word wraps
- Drop unused hdBackground.h include from aitdBox
- Move model texture atlas rendering out of the core renderer
- Move camera HD background loading and floor preload behind core hooks
- Route full-screen HD background handling through core hooks
- Decouple core and life scripts from lantern lighting with core hooks
- Update docs after block A of core hooks
- Stop including the remaster config in every engine file
- Move startup options gate and controls menu behind core hooks
- Move menuMouse.h to platform
- Move the detail level toggle side effects behind a core hook
- Note pause menu scene preview (V9) and unfinished map screen (V10)
- Route systemMenu TTF text and notifications through core hooks
- Confirm H4 predates the font hook
- Note overlapping TTF text in books in High (H4)
- Decouple font from TTF fonts with a draw-text core hook
- Decouple startupMenu and main from TTF fonts with core hooks
- Decouple mainLoop and AITD1 from TTF fonts with core hooks
- Decouple inventory from TTF fonts with core hooks
- Decouple animAction from blood particles with a core hook
- Remove dead embedded-data fallbacks for game files
- Note inventory ignores ESC (V8)
- Update docs for the layered FitdLib layout
- Move engine sources into FitdLib/core and remaining helpers into util
- Note lantern lighting too bright (V7)
- Move remaster sources into FitdLib/remaster
- Move platform sources into FitdLib/platform
- Move script interpreter sources into FitdLib/scripts
- Move utility sources into FitdLib/util
- Document tatou.log and new findings C9/C10
- Write engine output to tatou.log and catch C++ exceptions
- Document heap overflow fix and ASan workflow
- Fix heap overflow in blood and smoke special objects
- Update notes: heap corruption lead and dead code removed
- Remove dead debugFont and menuFade sources
- Add code map for phase 3
- Update docs after removing embedded game data
- Remove embedded original game data
- Unify documentation
- Remove leftover modelAtlas.cpp.fixed and temp_renderer.txt
- Keep generated shader headers in LF
- Remove committed build outputs and ignore local builds
- Mark PAK count and animAction duplicates as resolved in baseline notes
- Fix PAK entry count off by one
- Update baseline notes after Low retest
- Document stuck enemy, missing blood and heap corruption
- Remove duplicate animAction sources from the build

## 2026-10-23

- Log a warning when the engine falls back to embedded game data
