"""Unpacks a game executable compressed with LZEXE 0.90 (AITD2.EXE of the GOG
and CD versions), or takes one that is not (AITD3.EXE, Borland C), and
disassembles it, to study how the original engine did
something. It reads the player's own copy; the result is the game's code and
stays out of the repository (the output folder may not be inside it).

    python tools/disasmExe.py <game>/AITD2.EXE <output folder>

Writes <name>.bin (the unpacked program: its code and data as loaded, from
linear address 0) and, if GNU objdump is there (or in WSL on Windows),
<name>.asm, 16-bit x86 in Intel syntax. Addresses in the .asm are linear: a
far call to SEG:OFF is at SEG*16+OFF. What is known of AITD2.EXE is in
docs/AITD2_EXE.md, and of AITD3.EXE in docs/AITD3_EXE.md.
"""
import os
import shutil
import struct
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))


def unpack_lzexe(data):
    """The program packed by LZEXE 0.90, without its relocations."""
    if data[0x1c:0x20] != b'LZ09':
        raise ValueError('not an LZEXE 0.90 program (no LZ09 mark)')
    header_paragraphs = struct.unpack_from('<H', data, 8)[0]
    pos = header_paragraphs * 16
    out = bytearray()

    def word():
        nonlocal pos
        value = data[pos] | (data[pos + 1] << 8)
        pos += 2
        return value

    def byte():
        nonlocal pos
        value = data[pos]
        pos += 1
        return value

    bits = word()
    count = 16

    def bit():
        nonlocal bits, count
        value = bits & 1
        count -= 1
        if count == 0:
            bits = word()
            count = 16
        else:
            bits >>= 1
        return value

    while True:
        if bit():
            out.append(byte())
            continue
        if not bit():
            length = bit() * 2
            length += bit()
            length += 2
            span = byte() - 0x100
        else:
            low = byte()
            high = byte()
            span = (low | ((high & 0xF8) << 5) | 0xE000) - 0x10000
            length = high & 7
            if length == 0:
                length = byte()
                if length == 0:
                    break
                if length == 1:
                    continue
                length += 1
            else:
                length += 2
        for _ in range(length):
            out.append(out[span])
    return bytes(out)


def mz_image(data):
    """The program of an unpacked MZ executable (AITD3.EXE), without its relocations."""
    if data[:2] != b'MZ':
        raise ValueError('not a DOS executable (no MZ mark)')
    last_page, pages, _, header_paragraphs = struct.unpack_from('<4H', data, 2)
    end = pages * 512 - ((512 - last_page) if last_page else 0)
    return data[header_paragraphs * 16:end]


def objdump_command(binary):
    """objdump of the system, or of WSL on Windows."""
    args = ['-D', '-b', 'binary', '-m', 'i8086', '-M', 'intel']
    if shutil.which('objdump'):
        return ['objdump'] + args + [binary]
    if os.name == 'nt' and shutil.which('wsl'):
        drive, rest = os.path.splitdrive(os.path.abspath(binary))
        wsl_path = '/mnt/' + drive[0].lower() + rest.replace('\\', '/')
        return ['wsl', 'objdump'] + args + [wsl_path]
    return None


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    exe, out_folder = sys.argv[1], os.path.abspath(sys.argv[2])
    inside = os.path.splitdrive(out_folder)[0].lower() == os.path.splitdrive(REPO)[0].lower() \
        and os.path.commonpath([out_folder, REPO]) == REPO
    if inside:
        print('The output folder is inside the repository: the game code must stay out of it')
        return 1
    os.makedirs(out_folder, exist_ok=True)
    name = os.path.splitext(os.path.basename(exe))[0].lower()

    with open(exe, 'rb') as f:
        data = f.read()
    program = unpack_lzexe(data) if data[0x1c:0x20] == b'LZ09' else mz_image(data)
    binary = os.path.join(out_folder, name + '.bin')
    with open(binary, 'wb') as f:
        f.write(program)
    print('%s: %d bytes' % (binary, len(program)))

    command = objdump_command(binary)
    if not command:
        print('No objdump (GNU binutils): only the unpacked program was written')
        return 0
    listing = os.path.join(out_folder, name + '.asm')
    with open(listing, 'wb') as f:
        subprocess.run(command, stdout=f, check=True)
    print(listing)
    return 0


if __name__ == '__main__':
    sys.exit(main())
