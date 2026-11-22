#!/usr/bin/env python3
# Reads the entries of a PAK of the Alone in the Dark games, as the engine
# does (TatouSource/FitdLib/core/pak.cpp): stored (0), imploded (1, the
# "explode" of util/unpack.cpp) or deflated (4).
#
# As a module: entries(path) -> list of bytes, one per entry.
# As a script: python tools/pak.py <file.PAK>  (lists the entries)
import struct
import sys
import zlib


def _tree(src, pos, n):
    """Bit lengths of a code (PAK_get_tree): count/length pairs."""
    count = src[pos] + 1
    pos += 1
    lengths = []
    for _ in range(count):
        j = src[pos]
        pos += 1
        lengths += [(j & 0xF) + 1] * (((j & 0xF0) >> 4) + 1)
    if len(lengths) != n:
        raise ValueError('bad code in an imploded entry')
    return lengths, pos


def _codes(lengths, values=None, extras=None):
    """(length, code) -> (value, extra bits), the codes in the order the
    engine gives them (PAK_huft_build: by length, counting backwards)."""
    table = {}
    code = 0
    for k in range(1, 17):
        for symbol, length in enumerate(lengths):
            if length != k:
                continue
            value = values[symbol] if values else symbol
            extra = extras[symbol] if extras else 0
            table[(k, code)] = (value, extra)
            j = 1 << (k - 1)
            while code & j:
                code ^= j
                j >>= 1
            code ^= j
    return table


class _Bits:
    def __init__(self, data, pos):
        self.data, self.pos, self.buffer, self.count = data, pos, 0, 0

    def need(self, n):
        while self.count < n:
            byte = self.data[self.pos] if self.pos < len(self.data) else 0
            self.pos += 1
            self.buffer |= byte << self.count
            self.count += 8

    def take(self, n):
        self.need(n)
        value = self.buffer & ((1 << n) - 1)
        self.buffer >>= n
        self.count -= n
        return value

    def decode(self, table):
        # The engine looks codes up with the bits inverted (~b)
        code = 0
        for k in range(1, 17):
            code |= (self.take(1) ^ 1) << (k - 1)
            found = table.get((k, code))
            if found:
                return found
        raise ValueError('bad code in an imploded entry')


_LEN2 = list(range(2, 66))
_LEN3 = list(range(3, 67))
_EXTRA = [0] * 63 + [8]
_DIST4 = [1 + 64 * i for i in range(64)]
_DIST8 = [1 + 128 * i for i in range(64)]


def explode(src, size, flags):
    pos = 0
    literals = None
    if flags & 4:
        lengths, pos = _tree(src, pos, 256)
        literals = _codes(lengths)
        lengths, pos = _tree(src, pos, 64)
        length_codes = _codes(lengths, _LEN3, _EXTRA)
    else:
        lengths, pos = _tree(src, pos, 64)
        length_codes = _codes(lengths, _LEN2, _EXTRA)
    lengths, pos = _tree(src, pos, 64)
    if flags & 2:
        low_bits, dist_codes = 7, _codes(lengths, _DIST8, [0] * 64)
    else:
        low_bits, dist_codes = 6, _codes(lengths, _DIST4, [0] * 64)
    bits = _Bits(src, pos)
    out = bytearray()
    while len(out) < size:
        if bits.take(1):
            out.append(bits.decode(literals)[0] if literals else bits.take(8))
            continue
        distance = bits.take(low_bits)
        distance += bits.decode(dist_codes)[0]
        length, extra = bits.decode(length_codes)
        if extra:
            length += bits.take(8)
        for _ in range(length):
            source = len(out) - distance
            out.append(out[source] if source >= 0 else 0)
    return bytes(out[:size])


def entries(path):
    data = open(path, 'rb').read()
    count = struct.unpack_from('<I', data, 4)[0] // 4 - 1
    result = []
    for index in range(count):
        offset = struct.unpack_from('<I', data, (index + 1) * 4)[0]
        if offset == 0:
            break
        skip = struct.unpack_from('<I', data, offset)[0]
        pos = offset + (skip if skip else 4)
        disc, size, flag, info5, name = struct.unpack_from('<IIBBH', data, pos)
        pos += 12 + name
        raw = data[pos:pos + disc]
        if flag == 0:
            result.append(raw[:size])
        elif flag == 1:
            result.append(explode(raw, size, info5))
        elif flag == 4:
            result.append(zlib.decompressobj(-15).decompress(raw)[:size])
        else:
            raise ValueError(f'entry {index}: unknown compression {flag}')
    return result


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('usage: pak.py <file.PAK>')
    for i, entry in enumerate(entries(sys.argv[1])):
        print(i, len(entry), '%08X' % zlib.crc32(entry))
