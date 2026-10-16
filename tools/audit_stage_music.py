"""Audit stage-authored BGM references in the user's disc without extracting assets."""
import argparse
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def u32(data, offset):
    return struct.unpack_from('>I', data, offset)[0]


def disc_files(image):
    image.seek(0x424)
    offset, size = struct.unpack('>II', image.read(8))
    image.seek(offset)
    fst = image.read(size)
    count = u32(fst, 8)
    strings = count * 12
    directories = [(count, '')]
    files = {}
    for index in range(1, count):
        while index >= directories[-1][0]:
            directories.pop()
        entry, offset, size = struct.unpack_from('>III', fst, index * 12)
        start = strings + (entry & 0xFFFFFF)
        name = fst[start:fst.index(b'\0', start)].decode('ascii')
        path = directories[-1][1] + name
        if entry >> 24:
            directories.append((size, path + '/'))
        else:
            files[path] = offset, size
    return files


def read_file(image, files, path):
    offset, size = files[path]
    image.seek(offset)
    data = image.read(size)
    if len(data) != size:
        raise ValueError(f'Truncated file: {path}')
    return data


def stage_rows(archive):
    _, data_size, relocs, public, external = struct.unpack_from('>5I', archive)
    data = archive[32:32 + data_size]
    exports = 32 + data_size + relocs * 4
    strings = exports + (public + external) * 8
    for index in range(public):
        address, name = struct.unpack_from('>II', archive, exports + index * 8)
        start = strings + name
        symbol = archive[start:archive.index(b'\0', start)]
        if symbol == b'grGroundParam':
            rows, count = struct.unpack_from('>II', data, address + 0xB0)
            if not 0 < count < 1024 or rows + count * 0x64 > len(data):
                raise ValueError('Invalid stage music rows')
            return [dict(zip(('stkind', 'one_player', 'one_player_alt', 'vs', 'vs_alt',
                              'alternate_rule', 'alternate_chance'),
                             struct.unpack_from('>Iiiiihh', data, rows + i * 0x64)))
                    for i in range(count)]
    raise ValueError('Missing grGroundParam')


def inspect_hps(data):
    if data[:8] != b' HALPST\0':
        raise ValueError('Invalid HPS magic')
    rate, channels = struct.unpack_from('>II', data, 8)
    if not rate or channels not in (1, 2):
        raise ValueError('Invalid HPS format')
    offset, samples, visited = 0x80, 0, {}
    while offset not in visited:
        if offset < 0x80 or offset + 0x20 > len(data):
            raise ValueError('Invalid HPS block address')
        visited[offset] = samples
        size, end, following = struct.unpack_from('>III', data, offset)
        if (not size or size % (channels * 8) or offset + 0x20 + size > len(data)
                or end < 2 or end >= size * 2 // channels):
            raise ValueError('Invalid HPS block size')
        nibbles = end + 1
        samples += nibbles // 16 * 14 + max(0, nibbles % 16 - 2)
        if following == 0xFFFFFFFF:
            return dict(rate=rate, channels=channels, blocks=len(visited),
                        frames=samples, loop=None)
        offset = following
    return dict(rate=rate, channels=channels, blocks=len(visited),
                frames=samples, loop=visited[offset])


def audit(iso):
    metadata = (ROOT / 'upstream/melee-pc/src/melee/lb/lbaudio_ax.static.h').read_text(encoding='utf-8')
    match = re.search(r'static const char\* hps_files\[\] = \{(.*?)\n\};', metadata, re.S)
    if not match:
        raise ValueError('Original HPS file table not found')
    tracks = re.findall(r'"([^"]+)"', match[1])
    source = (ROOT / 'src/xdk/match_scene_xdk.c').read_text(encoding='utf-8')
    table = re.search(r'static const M360StageDesc s_stages\[\] = \{(.*?)\n\};', source, re.S)
    if not table:
        raise ValueError('Native stage registry not found')
    stages = re.findall(r'\{\s*"([^"]+)",\s*"([^"]+)"', table[1])
    if not stages:
        raise ValueError('Empty stage registry')
    result = dict(stages=[], tracks={})
    with iso.open('rb') as image:
        files = disc_files(image)
        for name, archive in stages:
            rows = stage_rows(read_file(image, files, archive))
            # The current VS bridge uses row zero; 1P uses its exact StKind row.
            if rows[0]['vs'] < 0:
                raise ValueError(f'No primary VS music: {name}')
            for row in rows:
                for key in ('one_player', 'one_player_alt', 'vs', 'vs_alt'):
                    bgm = row[key]
                    if bgm in (-1, -2):
                        continue  # Authored sentinel: absent alternative / character theme.
                    if not 0 <= bgm < len(tracks):
                        raise ValueError(f'Invalid BGM {bgm}: {name}, {row["stkind"]}, {key}')
                    path = 'audio/' + tracks[bgm]
                    if path not in result['tracks']:
                        result['tracks'][path] = inspect_hps(read_file(image, files, path))
            result['stages'].append(dict(name=name, archive=archive,
                                         primary_vs=tracks[rows[0]['vs']], rows=rows))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, default=ROOT / 'iso/MeleeUSAv1.02.iso')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    result = audit(args.iso)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.header:
        args.header.parent.mkdir(parents=True, exist_ok=True)
        paths = ',\n'.join('    ' + json.dumps(p) for p in sorted(result['tracks']))
        args.header.write_text('static const char* stageMusicFiles[] = {\n' + paths + '\n};\n', encoding='ascii')
    print(f'PASS: {len(result["stages"])} stage archives; '
          f'{sum(len(s["rows"]) for s in result["stages"])} authored rows; '
          f'{len(result["tracks"])} HPS files present and structurally valid')
