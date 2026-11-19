#!/usr/bin/env python3
# Makes a narrator pack (TatouSource/Shared/narrators.h): one .voice file with
# the files of a narrator folder gathered one after the other without
# compression, so the game reads each one straight from it. Its name is the
# narrator's name in the list (Spanish.voice is shown as "Spanish").
#   Alone in the Dark: BBSSNN.ogg, .wav or .voc; the pack goes in
#   UserData\AITD1\Voices.
#   Alone in the Dark 2: books.cfg, TrackNN (or TrackNNN) and SampleNNN (.ogg, .wav or
#   .voc); the pack goes in UserData\AITD2\Voices.
#
# Usage: python tools/makeVoicePack.py <narrator folder> <pack.voice>
#   e.g. python tools/makeVoicePack.py "UserData/AITD1/Voices/Spanish" Spanish.voice
import os
import re
import sys
import zipfile

EXTENSIONS = ('.ogg', '.wav', '.voc')


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: makeVoicePack.py <narrator folder> <pack.voice>')
    folder, output = sys.argv[1], sys.argv[2]
    files = os.listdir(folder)
    if 'books.cfg' in files:
        # Alone in the Dark 2
        names = ['books.cfg'] + sorted(n for n in files if n.lower().endswith(EXTENSIONS) and
                                       re.fullmatch(r'(Track\d{2,3}|Sample\d{3})', os.path.splitext(n)[0]))
    else:
        names = sorted(n for n in files
                       if n.lower().endswith(EXTENSIONS) and len(os.path.splitext(n)[0]) == 6 and n[:6].isdigit())
        if not any(n.startswith('000000') for n in names):
            sys.exit('no 000000 file (Alone 1) or books.cfg (Alone 2) in ' + folder +
                     ': the game would not list this narrator')
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_STORED) as pack:
        for name in names:
            pack.write(os.path.join(folder, name), name)
    print(f'{output}: {len(names)} files, {os.path.getsize(output) / 1e6:.1f} MB')


if __name__ == '__main__':
    main()
