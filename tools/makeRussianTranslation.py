#!/usr/bin/env python3
# Makes the Russian translations of Alone in the Dark 1 and 2 built into the
# game (TatouSource/FitdLib/assets/translations/aitd1-russian.txt and
# aitd2-russian.txt, read by core/translation.cpp) from the translations of
# the Old-Games.ru translation bureau (www.old-games.ru: Alone 1, 2011;
# Alone 2, 2013; "Alone_in_the_Dark_RUS.exe" and "Alone_in_the_Dark_2_RUS.exe",
# opened with 7-Zip into a folder) and the player's GOG edition.
#
# A translation changes the texts (its ENGLISH.PAK), letters of the game's
# font, and texts drawn in pictures and in animated sequences. The file
# keeps only that: the texts, the letters drawn with # and ., and the
# pixels that change (compressed); the game puts them over the player's
# own files. What else differs in a patch comes from the floppy version it
# was made for (Alone 2: models, sounds), not from the translation, and is
# left out.
#
# Usage: python tools/makeRussianTranslation.py aitd1|aitd2 <folder of the
#        translation's files> <folder of the GOG data>
import base64
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pak  # noqa: E402

FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'TatouSource', 'FitdLib', 'assets',
                      'translations')

GAMES = {
    'aitd1': {
        'title': 'Alone in the Dark',
        'year': 2011,
        'credits': 'Its credits are the last pages of text 19 (the book shown at the start).',
        'font': 5,
        'pictures': {'ITD_RESS': (4, 13)},
        'sequences': (),
        'texts': None,
    },
    'aitd2': {
        'title': 'Alone in the Dark 2',
        'year': 2013,
        'credits': 'Its credits are at the end of the final credits (ITD_RESS 57).',
        'font': 1,
        'pictures': {'ITD_RESS': (14, 15, 16, 17, 51, 52, 53, 54, 55, 56, 57, 58), 'CAMERA04': (13, 14)},
        'sequences': ('ITRO', 'MER', 'LAST'),
        # The floppy version has no text 4 of the GOG (CD) edition: the
        # others come one place later; text 4 is translated here
        'texts': lambda n: n if n < 4 else n + 1,
        'extra': {4: '#CВидение ...\n{1A}\n'},
    },
}

HEADER = """\
# Russian translation of {title}, built into Alone in the Dark: Classic
# Reborn. Translation by the Old-Games.ru translation bureau ({year}),
# www.old-games.ru: thank you! {credits}
#
# Made by tools/makeRussianTranslation.py from their translation; read by
# core/translation.cpp only when the game language is Russian.
#   @@ text N            a text of the language PAK, in UTF-8 (DOS code page
#                        866 in the game); {{XX}} is a byte that is not a letter
#   @@ glyph XX W        a letter of the game's font: W columns, then one line
#                        of # and . for each of its rows
#   @@ patch PAK N SIZE  the pixels that change in that entry (of that size)
#   @@ frame PAK N       the pixels that change in that frame of a sequence
#                        (once the frame is unpacked, 320x200)
#   A patch or a frame is base64 of zlib data: runs of a 32-bit offset, a
#   16-bit length and the new bytes.
"""


def find(folder, name):
    for file in os.listdir(folder):
        if file.upper() == name.upper():
            return os.path.join(folder, file)
    sys.exit(f'no {name} in {folder}')


def escape(raw):
    """A text of the game as lines of UTF-8 (\\r\\n is a new line)."""
    out = []
    for part in raw.split(b'\r\n'):
        line = ''
        for byte in part:
            if byte < 32 or byte == ord('{'):
                line += '{%02X}' % byte
            else:
                line += bytes([byte]).decode('cp866')
        out.append(line)
    return '\n'.join(out)


def unescape(text):
    out = bytearray()
    for n, line in enumerate(text.split('\n')):
        if n:
            out += b'\r\n'
        i = 0
        while i < len(line):
            if line[i] == '{':
                out.append(int(line[i + 1:i + 3], 16))
                i += 4
            else:
                out += line[i].encode('cp866')
                i += 1
    return bytes(out)


def font(data):
    """First code, height and the letters (width, rows) of a font (core/font.cpp)."""
    align = struct.unpack_from('<h', data, 0)[0]
    height = data[2]
    row_bytes = data[3] or struct.unpack_from('<h', data, 4)[0]
    table = struct.unpack_from('>H', data, 6)[0] - (align & 0xFF) * 2
    letters = []
    for code in range(256):
        # GOG's tables can end before the last codes: no letter
        value = struct.unpack_from('>H', data, table + code * 2)[0] if table + code * 2 + 2 <= len(data) else 0
        width, start = value >> 12, value & 0xFFF
        rows = []
        for row in range(height):
            rows.append(''.join('#' if data[8 + row * row_bytes + ((start + x) >> 3)] & (0x80 >> ((start + x) & 7))
                                else '.' for x in range(width)))
        letters.append((width, rows))
    return align & 0xFF, height, letters


def runs(old, new):
    """The changed bytes as compressed runs, in lines of base64."""
    data = bytearray()
    i = 0
    while i < len(new):
        if old[i] == new[i]:
            i += 1
            continue
        # a run goes on over short stretches of equal bytes (cheaper than a
        # new run), up to 6 of them
        start, end, j = i, i + 1, i + 1
        while j < len(new) and j - start < 0xFFFF:
            if old[j] != new[j]:
                end = j + 1
            elif j - end >= 6:
                break
            j += 1
        data += struct.pack('<IH', start, end - start) + new[start:end]
        i = end
    text = base64.b64encode(zlib.compress(bytes(data), 9)).decode('ascii')
    return [text[k:k + 76] for k in range(0, len(text), 76)]


def unpack_frame(src, dest):
    """A frame of a sequence over the previous one (unapckSequenceFrame)."""
    i, d = 0, 0
    code = src[i]
    i += 1
    while code:
        if code == 1:
            n, c = 1, src[i]
            i += 1
        elif code == 2:
            n, c = 2, src[i]
            i += 1
        elif code == 3:
            n, c = src[i], src[i + 1]
            i += 2
        else:
            n, c = struct.unpack_from('<H', src, i)[0], src[i + 2]
            i += 3
        if c:
            dest[d:d + n] = bytes([c]) * n
        d += n
        code = src[i]
        i += 1


def frames(entries):
    """The frames of a sequence as the game shows them (core/sequence.cpp)."""
    screen = bytearray(entries[0][768:768 + 64000])
    out = [bytes(screen)]
    for entry in entries[1:]:
        if struct.unpack_from('<I', entry, 0)[0] < 64000:
            unpack_frame(entry[4:], screen)
        else:
            screen[:] = entry[:64000]
        out.append(bytes(screen))
    return entries[0][:768], out


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in GAMES:
        sys.exit('usage: makeRussianTranslation.py aitd1|aitd2 <folder of the translation> <folder of the GOG data>')
    game, patch_folder, gog_folder = sys.argv[1], sys.argv[2], sys.argv[3]
    info = GAMES[game]
    lines = [HEADER.format(**info)]

    # Texts, as the GOG edition numbers them
    texts = {}
    for index, raw in enumerate(pak.entries(find(patch_folder, 'ENGLISH.PAK'))):
        texts[info['texts'](index) if info['texts'] else index] = escape(raw)
    texts.update(info.get('extra', {}))
    if len(texts) != len(pak.entries(find(gog_folder, 'ENGLISH.PAK'))):
        sys.exit('the texts do not match the GOG edition\'s')
    for index in sorted(texts):
        text = texts[index]
        if escape(unescape(text)) != text or any(line.startswith('@@') for line in text.split('\n')):
            sys.exit(f'text {index} cannot be written back as it was')
        lines.append(f'@@ text {index}')
        lines.append(text)

    # Letters of the font
    gog_res = pak.entries(find(gog_folder, 'ITD_RESS.PAK'))
    patch_res = pak.entries(find(patch_folder, 'ITD_RESS.PAK'))
    first, height, gog_letters = font(gog_res[info['font']])
    first_r, height_r, patch_letters = font(patch_res[info['font']])
    if (first, height) != (first_r, height_r):
        sys.exit('the two fonts are not alike')
    # The codes under the first one are no letters: their entries of the
    # table fall on the end of the font's last row
    changed = [c for c in range(first, 256) if gog_letters[c] != patch_letters[c]]
    for code in changed:
        width, rows = patch_letters[code]
        lines.append('@@ glyph %02X %d' % (code, width))
        lines += rows if width else []

    # Pictures
    pictures = 0
    for name, indexes in info['pictures'].items():
        gog = gog_res if name == 'ITD_RESS' else pak.entries(find(gog_folder, name + '.PAK'))
        new = patch_res if name == 'ITD_RESS' else pak.entries(find(patch_folder, name + '.PAK'))
        for index in range(len(new)):
            if (name == 'ITD_RESS' and index == info['font']) or gog[index] == new[index]:
                continue
            if index not in indexes or len(gog[index]) != len(new[index]):
                sys.exit(f'{name} entry {index} changes in an unexpected way')
            lines.append(f'@@ patch {name} {index} {len(gog[index])}')
            lines += runs(gog[index], new[index])
            pictures += 1

    # Sequences
    changed_frames = 0
    for name in info['sequences']:
        palette, gog = frames(pak.entries(find(gog_folder, name + '.PAK')))
        palette_r, new = frames(pak.entries(find(patch_folder, name + '.PAK')))
        if palette != palette_r or len(gog) != len(new):
            sys.exit(f'the sequence {name} is not the same')
        for index, (a, b) in enumerate(zip(gog, new)):
            if a != b:
                lines.append(f'@@ frame {name} {index}')
                lines += runs(a, b)
                changed_frames += 1

    lines.append('@@ end')
    output = os.path.join(FOLDER, f'{game}-russian.txt')
    os.makedirs(FOLDER, exist_ok=True)
    with open(output, 'w', encoding='utf-8', newline='\n') as file:
        file.write('\n'.join(lines) + '\n')
    print(f'{len(texts)} texts, {len(changed)} letters, {pictures} pictures, {changed_frames} frames -> '
          f'{os.path.normpath(output)} ({os.path.getsize(output) // 1024} KB)')


if __name__ == '__main__':
    main()
