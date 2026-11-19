#!/usr/bin/env python3
# Makes a texture pack (FitdLib/remaster/graphics/texturePack.h): one .texture
# file, a ZIP without compression, with the backgrounds and masks of each
# game, as the user folder has them:
#   AITD1/Backgrounds/...  AITD1/Masks/...  JACK/Backgrounds/...  JACK/Masks/...
# The pictures are PNG and JPG, already compressed: storing them as they are
# lets the game read each one straight from the file.
#
# Usage: python tools/makeTexturePack.py <UserData folder> <pack.texture> [games]
#   games: AITD1 and JACK when not given (also AITD2, AITD3)
#   e.g. python tools/makeTexturePack.py "D:/Alone in the Dark - ClassicReborn/UserData" nayo.texture
# The game reads packs under 4 GB (no ZIP64).
import os
import sys
import zipfile

DEFAULT_GAMES = ['AITD1', 'JACK']
FOLDERS = ['Backgrounds', 'Masks']
EXTENSIONS = ('.png', '.jpg', '.jpeg')


def main():
    if len(sys.argv) < 3:
        sys.exit('usage: makeTexturePack.py <UserData folder> <pack.texture> [games]')
    userData, output = sys.argv[1], sys.argv[2]
    games = [g.upper() for g in sys.argv[3:]] or DEFAULT_GAMES
    files = []
    for game in games:
        for folder in FOLDERS:
            root = os.path.join(userData, game, folder)
            if not os.path.isdir(root):
                continue
            for base, _, names in os.walk(root):
                for name in sorted(names):
                    if name.lower().endswith(EXTENSIONS):
                        path = os.path.join(base, name)
                        files.append((path, os.path.relpath(path, userData).replace(os.sep, '/')))
    if not files:
        sys.exit('no backgrounds or masks in ' + userData)

    total = 0
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_STORED) as pack:
        for path, name in sorted(files, key=lambda f: f[1]):
            pack.write(path, name)
            total += os.path.getsize(path)
    counts = {}
    for _, name in files:
        key = '/'.join(name.split('/')[:2])
        counts[key] = counts.get(key, 0) + 1
    for key in sorted(counts):
        print(f'{key}: {counts[key]} files')
    size = os.path.getsize(output)
    print(f'{output}: {len(files)} files, {size / 1e6:.1f} MB')
    if size >= 4 * 1024 ** 3:
        print('too large: the game reads packs under 4 GB')


if __name__ == '__main__':
    main()
