"""Read authored stage collision and joint bindings from a local Melee ISO."""
import json
import math
import pathlib
import struct
import sys

root = pathlib.Path(__file__).resolve().parent.parent
with (root / 'dist/melee.iso').open('rb') as iso:
    iso.seek(0x424)
    offset, size = struct.unpack('>II', iso.read(8))
    iso.seek(offset)
    fst = iso.read(size)
    count = struct.unpack_from('>I', fst, 8)[0]
    strings = count * 12
    for i in range(1, count):
        name, offset, size = struct.unpack_from('>III', fst, i * 12)
        if name >> 24:
            continue
        start = strings + (name & 0xFFFFFF)
        if fst[start:fst.index(b'\0', start)].decode() == sys.argv[1]:
            iso.seek(offset)
            archive = iso.read(size)
            break
    else:
        raise ValueError('Stage file not found')
length, data_size, reloc, public, external = struct.unpack_from('>5I', archive)
data = archive[32:32 + data_size]
symbol_start = 32 + data_size + reloc * 4 + (public + external) * 8
symbols = {}
for i in range(public):
    address, name = struct.unpack_from('>II', archive, 32 + data_size + reloc * 4 + i * 8)
    start = symbol_start + name
    symbols[archive[start:archive.index(b'\0', start)].decode()] = address
def read(fmt, offset):
    return struct.unpack_from('>' + fmt, data, offset)
def word(offset):
    return read('I', offset)[0]
def mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]
identity = [[float(r == c) for c in range(4)] for r in range(4)]
def local(address):
    rx, ry, rz, sx, sy, sz, x, y, z = read('9f', address + 0x14)
    cx, cy, cz = math.cos(rx), math.cos(ry), math.cos(rz)
    ax, ay, az = math.sin(rx), math.sin(ry), math.sin(rz)
    return [[cy*cz*sx, (ax*ay*cz-cx*az)*sy, (cx*ay*cz+ax*az)*sz, x],
            [cy*az*sx, (ax*ay*az+cx*cz)*sy, (cx*ay*az-ax*cz)*sz, y],
            [-ay*sx, ax*cy*sy, cx*cy*sz, z], [0,0,0,1]]
def walk(address, parent, result, include=True):
    while address:
        matrix = mul(parent, local(address))
        if include:
            result.append(matrix)
        if not (word(address + 4) & 0x1000):
            walk(word(address + 8), matrix, result)
        address = word(address + 12)
        include = True
coll = symbols['coll_data']
vertices, nvertices, lines, nlines = read('4I', coll)
groups, ngroups = read('2I', coll + 0x24)
scale = read('f', symbols['grGroundParam'])[0] if 'grGroundParam' in symbols else 1.0
positions = [tuple(v * scale for v in read('2f', vertices + i * 8)) for i in range(nvertices)]
map_head = symbols['map_head']
remaps, nremaps = read('2I', map_head)
points = {}
for i in range(nremaps):
    joint, pairs, count = read('3I', remaps + i * 12)
    matrices = []
    matrix = [row[:] for row in identity]
    for axis in range(3):
        matrix[axis][axis] = scale
    walk(joint, matrix, matrices)
    for j in range(count):
        index, slot = read('2h', pairs + j * 4)
        points[slot] = [matrices[index][axis][3] for axis in range(3)]
maps, nmaps = read('2I', map_head + 8)
bindings = []
for mapid in range(nmaps):
    desc = maps + mapid * 0x34
    matrices = []
    matrix = [row[:] for row in identity]
    for axis in range(3):
        matrix[axis][axis] = scale
    walk(word(desc), matrix, matrices, False)
    binding, count = read('2I', desc + 0x20)
    for i in range(count):
        group, unused, joint = read('3h', binding + i * 6)
        bindings.append((mapid, group, joint))
        if joint < 0 or joint >= len(matrices):
            raise ValueError(('Invalid joint binding', mapid, group, joint, len(matrices)))
        matrix = matrices[joint]
        start, count = read('2h', groups + group * 0x28 + 0x24)
        for v in range(start, start + count):
            x, y = read('2f', vertices + v * 8)
            positions[v] = (matrix[0][0]*x + matrix[0][1]*y + matrix[0][3],
                            matrix[1][0]*x + matrix[1][1]*y + matrix[1][3])
floors = []
surfaces = []
for i in range(nlines):
    v0, v1, p0, n0, p1, n1, kind, flags = read('2H4h2H', lines + i * 16)
    surfaces.append(dict(line=i, start=positions[v0], end=positions[v1], kind=kind, flags=flags))
    if kind & 1:
        floors.append(dict(line=i, start=positions[v0], end=positions[v1], flags=flags))
group_ranges = [dict(group=i, ranges=read('10h', groups + i * 0x28)) for i in range(ngroups)]
print(json.dumps(dict(symbols=symbols, scale=scale, bindings=bindings,
                     points=points, groups=group_ranges, floors=floors, surfaces=surfaces), indent=2))
