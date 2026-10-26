"""Adapt four original surface projections to the native transformed map view."""
import argparse
from pathlib import Path
import re
from generate_menu_sources import functions

NAMES = ('mpLib_8004DD90_Floor', 'mpLib_8004E090_Ceiling',
         'mpLib_8004E398_LeftWall', 'mpLib_8004E684_RightWall')


def generate(output):
    root = Path(__file__).resolve().parent.parent
    source = (root / 'upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8')
    parsed = functions(source)
    bodies = []
    for name in NAMES + ('mpLib_8004ED5C',):
        text = parsed[name][1]
        if name == 'mpLib_8004ED5C':
            text = text.replace('CollLine* line = mpLineGetCollLine(line_id);',
                                'const M360StageLine* line = &M360_MatchStageData()->lines[line_id];')
            for i in (0, 1):
                text = text.replace(f'int i{i};', '').replace(f'i{i} = line->x0->v{i}_idx;', '')
                for axis in ('x', 'y'):
                    text = text.replace(f'groundCollVtx[i{i}].pos.{axis}', f'line->{axis}{i}')
        text = re.sub(r'LINEID_CHECK\(\d+, line_id\);',
                      'if (!mpLib_80054ED8(line_id)) return -1;', text)
        text = re.sub(r'CollLine\* (\w+) = &groundCollLine\[line_id\];',
                      r'const M360StageLine* \1 = &M360_MatchStageData()->lines[line_id];', text)
        text = re.sub(r'groundCollLine\[(\w+)\]\.flags & CollLine_(\w+)',
                      lambda m: f'M360_MatchStageData()->lines[{m[1]}].kind & M360_LINE_' +
                      {'Floor': 'FLOOR', 'Ceiling': 'CEILING', 'LeftWall': 'LEFT_WALL', 'RightWall': 'RIGHT_WALL'}[m[2]], text)
        text = text.replace('groundCollLine[line_id].x0->lo_flags',
                            'M360_MatchStageData()->lines[line_id].flags')
        text = re.sub(r'groundCollVtx\[groundCollLine\[line_id\]\.x0->v([01])_idx\]\.pos\.([xy])',
                      lambda m: f'M360_MatchStageData()->lines[line_id].{m[2]}{m[1]}', text)
        text = re.sub(r'groundCollVtx\[(\w+)->x0->v([01])_idx\]\.pos\.([xy])',
                      lambda m: f'{m[1]}->{m[3]}{m[2]}', text)
        if any(word in text for word in ('groundColl', 'LINEID_CHECK', 'CollLine*')):
            raise ValueError(f'Unadapted map access in {name}')
        bodies.append(text)
    for name in ('mpLineIntersection', 'mpLineIntersectionH'):
        bodies.append(parsed[name][1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('/* Generated from pinned mplib.c; only map access is adapted. */\n' +
                      '\n\n'.join(bodies) + '\n', encoding='ascii')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
