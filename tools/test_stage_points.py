"""Check every native stage loads the authored point skeletons from the local ISO."""
import contextlib
import io
from pathlib import Path
import re
import runpy
import sys

root = Path(__file__).resolve().parent.parent
catalog = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'src/xdk/match_scene_xdk.c'
scene = catalog.read_text(encoding='utf-8')
stages = re.findall(r'\{ "[^"]+", "(Gr\w+\.dat)", \{([^}]+)\}', scene)
assert len(stages) == 21, 'Review stage point coverage after catalog changes'
total = 0
pending = set()
expected_pending = {('GrZe.dat', slot) for slot in (133, 135, 136)}
for filename, selected in stages:
    loaded = {int(x) for x in selected.split(',') if int(x) >= 0}
    sys.argv = [str(root / 'tools/inspect_stage_collision.py'), filename]
    with contextlib.redirect_stdout(io.StringIO()):
        archive = runpy.run_path(sys.argv[0])
    read = archive['read']
    maps, count = read('2I', archive['map_head'] + 8)
    roots = {read('I', maps + i * 0x34)[0] for i in loaded if i < count}
    remaps, count = read('2I', archive['map_head'])
    for i in range(count):
        joint, pairs, size = read('3I', remaps + i * 12)
        if not size:
            continue
        for j in range(size):
            index, slot = read('2h', pairs + j * 4)
            assert 0 <= index < 128 and 0 <= slot < 256, (filename, index, slot)
            if joint not in roots:
                assert (filename, slot) in expected_pending, f'{filename}: authored point {slot} skeleton is not loaded'
                pending.add((filename, slot))
            else:
                total += 1
assert pending == expected_pending, 'Review coverage when Brinstar lava is ported'
print(f'PASS: {len(stages)} stage camera/player skeletons and {total} point remaps loaded within native bounds')
print('PENDING: three Brinstar item markers belong to the unported moving lava skeleton')
