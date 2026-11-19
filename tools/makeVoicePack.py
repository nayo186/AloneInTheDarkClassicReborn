#!/usr/bin/env python3
# Makes a narrator pack for Alone in the Dark (TatouSource/Shared/narrators.h):
# one .voice file with the voice files of a narrator folder (BBSSNN.ogg, .wav
# or .voc) gathered one after the other without compression, so the game reads
# each one straight from it. The pack goes in UserData\AITD1\Voices; its name
# is the narrator's name in the list (Spanish.voice is shown as "Spanish").
#
# Usage: python tools/makeVoicePack.py <narrator folder> <pack.voice>
#   e.g. python tools/makeVoicePack.py "UserData/AITD1/Voices/Spanish" Spanish.voice
import os
import sys
import zipfile

EXTENSIONS = ('.ogg', '.wav', '.voc')


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: makeVoicePack.py <narrator folder> <pack.voice>')
    folder, output = sys.argv[1], sys.argv[2]
    names = sorted(n for n in os.listdir(folder)
                   if n.lower().endswith(EXTENSIONS) and len(os.path.splitext(n)[0]) == 6 and n[:6].isdigit())
    if not any(n.startswith('000000') for n in names):
        sys.exit('no 000000 file in ' + folder + ': the game would not list this narrator')
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_STORED) as pack:
        for name in names:
            pack.write(os.path.join(folder, name), name)
    print(f'{output}: {len(names)} voice files, {os.path.getsize(output) / 1e6:.1f} MB')


if __name__ == '__main__':
    main()
