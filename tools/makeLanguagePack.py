#!/usr/bin/env python3
# Makes a language pack for Alone in the Dark 3 (TatouSource/Shared/
# languageFolder.h): one .language file with the files of a language folder
# (the language PAK, LISTSAMP.PAK, LISTLIFE.PAK and the others, and the CD
# tracks as NN.ogg or TrackNN.ogg) gathered one after the other without
# compression, so the game reads each one straight from it. The pack goes in
# UserData\AITD3\Languages, named as the folder would be (Spanish.language).
#
# Usage: python tools/makeLanguagePack.py <language folder> <pack.language>
#   e.g. python tools/makeLanguagePack.py "UserData/AITD3/Languages/Spanish" Spanish.language
import os
import sys
import zipfile

PAKS = ('FRANCAIS.PAK', 'ENGLISH.PAK', 'ESPAGNOL.PAK', 'DEUTSCH.PAK', 'ITALIANO.PAK')


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: makeLanguagePack.py <language folder> <pack.language>')
    folder, output = sys.argv[1], sys.argv[2]
    names = sorted(n for n in os.listdir(folder) if os.path.isfile(os.path.join(folder, n)))
    if not any(n.upper() in PAKS for n in names):
        sys.exit('no language PAK (ESPAGNOL.PAK, FRANCAIS.PAK...) in ' + folder)
    for needed in ('LISTSAMP.PAK', 'LISTLIFE.PAK'):
        if needed not in (n.upper() for n in names):
            sys.exit(f'no {needed} in {folder}: the game would not use this language')
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_STORED, allowZip64=False) as pack:
        for name in names:
            pack.write(os.path.join(folder, name), name)
    print(f'{output}: {len(names)} files, {os.path.getsize(output) / 1e6:.1f} MB')


if __name__ == '__main__':
    main()
