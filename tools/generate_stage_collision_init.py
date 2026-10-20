"""Extract unconditional collision disables from reviewed original stage startup."""
import argparse
from pathlib import Path
import re
from generate_menu_sources import functions

SOURCES = (
    ('grcastle.c', 'grCastle_801CD37C', 'Gr_Kind_Castle', 12),
    ('grkongo.c', 'grKongo_801D523C', 'Gr_Kind_Kongo', 2),
    ('grcorneria.c', 'grCorneria_801DD350', 'Gr_Kind_Corneria', 6),
    ('grpstadium.c', 'grStadium_OnInit', 'Gr_Kind_PStadium', 6),
)


def generate(output):
    root = Path(__file__).resolve().parent.parent
    rows = []
    for filename, name, kind, expected in SOURCES:
        source = (root / 'upstream/melee-pc/src/melee/gr' / filename).read_text(encoding='utf-8')
        body = functions(source)[name][1]
        calls = list(re.finditer(r'\bmpLib_80057BC0\(\s*(\d+)\s*\)\s*;', body))
        if len(calls) != expected:
            raise ValueError(f'Review changed startup collision calls in {name}')
        for call in calls:
            prefix = re.sub(r'/\*[\s\S]*?\*/|//[^\n]*', '', body[:call.start()])
            if prefix.count('{') - prefix.count('}') != 1:
                raise ValueError(f'Conditional collision call requires review: {name}')
            rows.append(f'    {{ {kind}, {int(call[1])} }}, /* {name} */')
    text = '\n'.join([
        '/* Generated from pinned original startup functions; do not edit. */',
        '#ifndef M360_STAGE_COLLISION_INITIAL_H', '#define M360_STAGE_COLLISION_INITIAL_H',
        'static const struct { int kind; unsigned group; } g_m360StageInitialDisabled[] = {',
        *rows, '};',
        '#define M360_STAGE_INITIAL_DISABLED_COUNT (sizeof(g_m360StageInitialDisabled) / sizeof(g_m360StageInitialDisabled[0]))',
        '#endif', '',
    ])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding='ascii')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
