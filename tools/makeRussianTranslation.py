#!/usr/bin/env python3
# Makes the Russian translation of Alone in the Dark built into the game
# (TatouSource/FitdLib/assets/translations/aitd1-russian.txt, read by
# core/translation.cpp) from the translation of the Old-Games.ru
# translation bureau (2011, www.old-games.ru, "Alone_in_the_Dark_RUS.exe",
# opened with 7-Zip) and the GOG edition's ITD_RESS.PAK.
#
# The translation changes the texts (its ENGLISH.PAK) and, in ITD_RESS.PAK,
# the game's font (67 letters: the Russian alphabet and Ё),
# the plate of the frames (sprite 8 of entry 4) and the title of the book
# of the start menu (entry 13). The file keeps only that: the texts, the
# letters drawn with # and ., and the bytes that change in the two pictures;
# the game puts them over the player's own files.
#
# Usage: python tools/makeRussianTranslation.py <ENGLISH.PAK of the
#        translation> <itd_ress.pak of the translation> <ITD_RESS.PAK of GOG>
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pak  # noqa: E402

OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'TatouSource', 'FitdLib', 'assets',
                      'translations', 'aitd1-russian.txt')
FONT, FRAMES, TITLE = 5, 4, 13

HEADER = """\
# Russian translation of Alone in the Dark, built into Alone in the Dark:
# Classic Reborn. Translation by the Old-Games.ru translation bureau (2011),
# www.old-games.ru: thank you! Its credits are the last pages of text 19.
#
# Made by tools/makeRussianTranslation.py from their translation; read by
# core/translation.cpp only when the game language is Russian.
#   @@ text N        a text of the language PAK, in UTF-8 (DOS code page
#                    866 in the game); {XX} is a byte that is not a letter
#   @@ glyph XX W    a letter of the game's font: W columns, 16 rows of # and .
#   @@ patch ITD_RESS N SIZE   bytes that change in that entry (of that size):
#                    offset and the new bytes
"""


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
    """Height and the letters (width, rows) of a font (core/font.cpp)."""
    align = struct.unpack_from('<h', data, 0)[0]
    height = data[2]
    row_bytes = data[3] or struct.unpack_from('<h', data, 4)[0]
    table = struct.unpack_from('>H', data, 6)[0] - (align & 0xFF) * 2
    letters = []
    for code in range(256):
        # The table of GOG's font ends before the last code (255): no letter
        value = struct.unpack_from('>H', data, table + code * 2)[0] if table + code * 2 + 2 <= len(data) else 0
        width, start = value >> 12, value & 0xFFF
        rows = []
        for row in range(height):
            rows.append(''.join('#' if data[8 + row * row_bytes + ((start + x) >> 3)] & (0x80 >> ((start + x) & 7))
                                else '.' for x in range(width)))
        letters.append((width, rows))
    return height, letters


def patch(old, new):
    """Lines 'offset hex' for the bytes of new that differ from old."""
    lines = []
    i = 0
    while i < len(new):
        if old[i] == new[i]:
            i += 1
            continue
        start = i
        while i < len(new) and i - start < 32 and old[i] != new[i]:
            i += 1
        lines.append('%d %s' % (start, new[start:i].hex().upper()))
    return lines


def main():
    if len(sys.argv) != 4:
        sys.exit('usage: makeRussianTranslation.py <ENGLISH.PAK> <itd_ress.pak of the translation> <ITD_RESS.PAK of GOG>')
    texts = pak.entries(sys.argv[1])
    russian = pak.entries(sys.argv[2])
    gog = pak.entries(sys.argv[3])
    if len(russian) != len(gog):
        sys.exit('the two ITD_RESS.PAK have different entries')
    lines = [HEADER]
    for index, raw in enumerate(texts):
        text = escape(raw)
        if unescape(text) != raw or any(line.startswith('@@') for line in text.split('\n')):
            sys.exit(f'text {index} cannot be written back as it was')
        lines.append(f'@@ text {index}')
        lines.append(text)
    height, gog_letters = font(gog[FONT])
    russian_height, russian_letters = font(russian[FONT])
    if height != 16 or russian_height != 16:
        sys.exit('unexpected font height')
    # The codes under 32 are no letters: their entries of the table fall on
    # the end of the font's last row (core/translation.cpp, addGlyphs)
    changed = [c for c in range(32, 256) if gog_letters[c] != russian_letters[c]]
    for code in changed:
        width, rows = russian_letters[code]
        lines.append('@@ glyph %02X %d' % (code, width))
        lines += rows if width else []
    for index in range(len(gog)):
        if index == FONT or gog[index] == russian[index]:
            continue
        if index not in (FRAMES, TITLE) or len(gog[index]) != len(russian[index]):
            sys.exit(f'ITD_RESS entry {index} changes in an unexpected way')
        lines.append(f'@@ patch ITD_RESS {index} {len(gog[index])}')
        lines += patch(gog[index], russian[index])
    lines.append('@@ end')
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, 'w', encoding='utf-8', newline='\n') as file:
        file.write('\n'.join(lines) + '\n')
    print(f'{len(texts)} texts, {len(changed)} letters -> {os.path.normpath(OUTPUT)}')


if __name__ == '__main__':
    main()
