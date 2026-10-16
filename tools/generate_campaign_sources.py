"""Slice original Classic selection and 1P difficulty/ally code for XDK."""
import argparse
from pathlib import Path
import re
from generate_menu_sources import functions

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'upstream/melee-pc/src/melee/gm'


def declaration(text, marker):
    start = text.index(marker)
    cursor = text.index('{', start) + 1
    depth = 1
    while depth:
        if text[cursor] == '{': depth += 1
        if text[cursor] == '}': depth -= 1
        cursor += 1
    return text[start:text.index(';', cursor) + 1]


def generate(output):
    classic = (BASE / 'gmclassic.c').read_text(encoding='utf-8')
    types = classic[classic.index('typedef struct gmClassicMatchup {'):
                    classic.index('gmClassicIntroData gmClassicIntroDataBuffer;')]
    result = types + '\n' + declaration(classic, 'static gmClassic_803DDEC8Data gmClassic_803DDEC8 =')
    result += '\nstatic u8 gm_804908A0[112];\n'
    inventory = functions(classic)
    for name in ('gm_804D4318', 'gm_804D4320', 'gm_804D4328'):
        result += '\n' + declaration(classic, 'static gmClassicMatchupData ' + name)
    for name in ('gmClassic_InitMatchupOrder', 'gmClassic_GetMatchupCount',
                 'gmClassic_801B2BA4', 'gmClassic_801B2D54'):
        body = inventory[name][1]
        if name == 'gmClassic_801B2D54':
            # Retail assumes every ground exists. Report unavailable native
            # candidates instead of entering its four intentional panic loops.
            body, count = re.subn(r'for \(;;\) \{\s*\}', 'return NULL;', body)
            if count != 4: raise ValueError('Classic availability guard changed')
        result += '\n' + body + '\n'
    for filename, typename, table, names in (
        ('gm_17EB.c', 'AllstarStageEntry', 'lbl_803D85F0',
         ('gm_8017EB3C', 'gm_8017EB64', 'gm_8017EB98', 'gm_8017EBCC',
          'gm_8017EC00', 'gm_8017EC50', 'gm_8017ECA0', 'gm_8017ECD4',
          'gm_8017ED08', 'gm_8017ED3C', 'gm_8017ED8C')),
        ('gm_17E4.c', 'AdventureStageEntry', 'lbl_803D7AC0',
         ('gm_8017E500', 'gm_8017E528', 'gm_8017E578', 'gm_8017E5C8',
          'gm_8017E5FC', 'gm_8017E630', 'gm_8017E664', 'gm_8017E6B4',
          'gm_8017E704', 'gm_8017E738', 'gm_8017E76C'))):
        source = (BASE / filename).read_text(encoding='utf-8')
        start = source.index('typedef struct ' + typename)
        end = source.index('ASSERT_SIZE', start)
        result += '\n' + source[start:end] + declaration(source, typename + ' ' + table)
        fs = functions(source)
        result += '\n' + '\n'.join(fs[n][1] for n in names)
    source = (BASE / 'gm_17C0.c').read_text(encoding='utf-8')
    result += '\n' + declaration(source, 'typedef struct RegClearCharEntry')
    fs = functions(source)
    for name in ('pick_random_ckind', 'fn_8017D9C0', 'gm_8017DB88',
                 'fn_8017DD7C', 'fn_8017DE54', 'gm_8017CE34_SetupColors'):
        result += '\n' + fs[name][1] + '\n'
    result += '\n' + declaration((BASE / 'gmregcommon.c').read_text(encoding='utf-8'),
                                  'u8 lbl_803D79F0[]')
    result += '\n' + functions((BASE / 'gmregcommon.c').read_text(encoding='utf-8'))['gmRegSetupEnemyColorTable'][1]
    fs = functions((BASE / 'gm_1B03.c').read_text(encoding='utf-8'))
    result += '\n' + fs['gm_801B05F4'][1] + '\n' + fs['gm_SetupHumanPlayer'][1]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('/* Generated original game code; do not edit. */\n#ifndef NUM_OPPONENTS\n#define NUM_OPPONENTS 3\n#endif\n' + result, encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
