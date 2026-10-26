"""Keep original mpCheckFloor math and selection over native active map groups."""
import argparse
from pathlib import Path
from generate_menu_sources import functions


def generate(output):
    root = Path(__file__).resolve().parent.parent
    source = (root/'upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8')
    text = functions(source)['mpCheckFloor'][1]
    def replace(old, new):
        nonlocal text
        if old not in text: raise ValueError('Original floor query changed: '+old)
        text = text.replace(old, new)
    replace('CollJoint* joint;', 'int group;')
    replace('bool already_checked;', '')
    replace('PAD_STACK(8);', '')
    replace('already_checked = mpCheckedBounding();\n    if (!already_checked) {\n        mpBoundingCheck2(ax, ay, bx, by);\n    }',
            '/* Active native lines are scanned without the original bounding cache. */')
    replace('for (joint = jointListStart; joint != NULL; joint = joint->next)',
            'for (group = 0; group < (int) M360_MatchMapJointCount(); ++group)')
    replace('CollLine* line_r26;', 'const M360StageLine* line_r26;')
    replace('MapJoint* j_inner;', 'const MapJoint* j_inner;')
    replace('if (joint->flags & CollJoint_TooFar) {\n            continue;\n        }', '')
    replace('joint - groundCollJoint', 'group')
    replace('j_inner = joint->inner;', 'j_inner = M360_MatchMapJoint((unsigned) group);')
    replace('joint->inner->dynamic_start', 'j_inner->dynamic_start')
    replace('var_r25 = j_inner->floor_count;',
            'var_r25 = j_inner->floor_count;\n        if (j_inner->floor_start < 0 || (unsigned) j_inner->floor_start >= M360_MatchStageData()->lineCount) var_r25 = 0;\n        else if (var_r25 > (int) M360_MatchStageData()->lineCount - j_inner->floor_start) var_r25 = (int) M360_MatchStageData()->lineCount - j_inner->floor_start;')
    replace('line_r26 = &groundCollLine[j_inner->floor_start];',
            'line_r26 = &groundCollLine[var_r25 > 0 ? j_inner->floor_start : 0];')
    replace('var_r25 = var_r24;',
            'if (j_inner->dynamic_start < 0 || (unsigned) j_inner->dynamic_start >= M360_MatchStageData()->lineCount) continue;\n            var_r25 = var_r24;\n            if (var_r25 > (int) M360_MatchStageData()->lineCount - j_inner->dynamic_start) var_r25 = (int) M360_MatchStageData()->lineCount - j_inner->dynamic_start;')
    replace('if (var_r24 != 0)', 'if (var_r24 > 0)')
    replace('ssize_t line_offset;', 'int line_index;')
    replace('if (line_id_skip == (line_offset = (intptr_t) line_r26 -\n                                               (intptr_t) groundCollLine) /\n                                    (ssize_t) sizeof(CollLine))',
            'line_index = (int) (line_r26 - M360_MatchStageData()->lines);\n            if (line_id_skip == line_index)')
    replace('if (!(line_r26->flags & CollLine_Floor) ||\n                !(line_r26->flags & LINE_FLAG_ENABLED) ||\n                line_r26->flags & LINE_FLAG_EMPTY)',
            'if (!(line_r26->kind & M360_LINE_FLOOR) ||\n                line_r26->kind & LINE_FLAG_EMPTY)')
    replace('line_offset / (ssize_t) sizeof(CollLine)', 'line_index')
    replace('line_r26->x0->lo_flags', 'line_r26->flags')
    replace('line_r26 - groundCollLine', '(int) (line_r26 - M360_MatchStageData()->lines)')
    replace('groundCollLine', 'M360_MatchStageData()->lines')
    replace('if (!already_checked) {\n        mpUncheckBounding();\n    }', '')
    replace('F32_MAX', 'FLT_MAX')
    if any(word in text for word in ('groundColl', 'CollJoint', 'CollLine', 'already_checked')):
        raise ValueError('Unadapted floor query state')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('/* Generated original floor query: native map view, no bounding cache. */\n'+text+'\n',encoding='ascii')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    generate(parser.parse_args().output)
