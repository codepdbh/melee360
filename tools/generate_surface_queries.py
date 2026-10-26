"""Adapt original ceiling/wall queries to the native transformed map view."""
import argparse
from pathlib import Path
import re
from generate_menu_sources import functions

NAMES = ('mpCheckCeiling', 'mpCheckLeftWall', 'mpCheckRightWall')


def generate(output):
    root = Path(__file__).resolve().parent.parent
    parsed = functions((root/'upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8'))
    bodies = []
    for name, field, kind in zip(NAMES, ('ceiling', 'left_wall', 'right_wall'),
                                 ('CEILING', 'LEFT_WALL', 'RIGHT_WALL')):
        text = parsed[name][1]
        def replace(old, new):
            nonlocal text
            if old not in text:
                raise ValueError(f'Original {name} changed: {old}')
            text = text.replace(old, new)
        ceiling = name == 'mpCheckCeiling'
        line = 'line_r26' if ceiling else 'line'
        count = 'var_r25' if ceiling else 'count'
        dynamic = 'var_r24' if ceiling else 'dynamic_count'
        replace('CollJoint* joint;', 'int group;')
        replace('bool already_checked;', '')
        text = re.sub(r'PAD_STACK\(\d+\);', '', text)
        replace('already_checked = mpCheckedBounding();', '')
        text, n = re.subn(r'if \(!already_checked\) \{\s*mpBoundingCheck2\(ax, ay, bx, by\);\s*\}', '', text)
        if n != 1: raise ValueError(f'Bounding setup changed in {name}')
        replace('for (joint = jointListStart; joint != NULL; joint = joint->next)',
                'for (group = 0; group < (int) M360_MatchMapJointCount(); ++group)')
        replace('if (joint->flags & CollJoint_TooFar) {\n            continue;\n        }', '')
        replace('joint - groundCollJoint', 'group')
        replace('MapJoint* j_inner;', 'const MapJoint* j_inner;')
        replace('j_inner = joint->inner;', 'j_inner = M360_MatchMapJoint((unsigned) group);')
        replace(f'CollLine* {line};', f'const M360StageLine* {line};')
        replace(f'{count} = j_inner->{field}_count;',
                f'{count} = j_inner->{field}_count;\n'
                f'        if (j_inner->{field}_start < 0 || (unsigned) j_inner->{field}_start >= M360_MatchStageData()->lineCount) {count} = 0;\n'
                f'        else if ({count} > (int) M360_MatchStageData()->lineCount - j_inner->{field}_start) {count} = (int) M360_MatchStageData()->lineCount - j_inner->{field}_start;')
        replace(f'&groundCollLine[j_inner->{field}_start]', f'&groundCollLine[{count} > 0 ? j_inner->{field}_start : 0]')
        replace(f'if ({dynamic} != 0)', f'if ({dynamic} > 0)')
        replace(f'{count} = {dynamic};',
                f'if (j_inner->dynamic_start < 0 || (unsigned) j_inner->dynamic_start >= M360_MatchStageData()->lineCount) continue;\n'
                f'            {count} = {dynamic};\n'
                f'            if ({count} > (int) M360_MatchStageData()->lineCount - j_inner->dynamic_start) {count} = (int) M360_MatchStageData()->lineCount - j_inner->dynamic_start;')
        replace('joint->inner->dynamic_start', 'j_inner->dynamic_start')
        original_kind = {'CEILING':'Ceiling', 'LEFT_WALL':'LeftWall', 'RIGHT_WALL':'RightWall'}[kind]
        replace(f'{line}->flags & CollLine_{original_kind}', f'{line}->kind & M360_LINE_{kind}')
        replace(f'{line}->flags & LINE_FLAG_EMPTY', f'{line}->kind & LINE_FLAG_EMPTY')
        if ceiling:
            replace(f'                !({line}->flags & LINE_FLAG_ENABLED) ||\n', '')
        else:
            replace(f'                {line}->flags & LINE_FLAG_ENABLED &&\n', '')
            replace('MapLine* inner = line->x0;\n                CollVtx* v0 = &groundCollVtx[inner->v0_idx];\n                CollVtx* v1 = &groundCollVtx[inner->v1_idx];', '')
            for i in (0, 1):
                for axis in ('x', 'y'):
                    replace(f'v{i}->pos.{axis}', f'line->{axis}{i}')
        replace(f'{line}->x0->lo_flags', f'{line}->flags')
        replace(f'{line} - groundCollLine', f'(int) ({line} - M360_MatchStageData()->lines)')
        replace('groundCollLine', 'M360_MatchStageData()->lines')
        replace('if (!already_checked) {\n        mpUncheckBounding();\n    }', '')
        replace('F32_MAX', 'FLT_MAX')
        if any(word in text for word in ('groundColl', 'CollJoint', 'CollLine', 'CollVtx', 'MapLine', 'already_checked')):
            raise ValueError(f'Unadapted query state in {name}')
        bodies.append(text)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('/* Original directional surface queries; native map access, no bounding cache. */\n' +
                      '\n\n'.join(bodies) + '\n', encoding='ascii')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
