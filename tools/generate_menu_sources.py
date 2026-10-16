"""Extract original menu dependencies without changing the pinned checkout."""
import argparse
import pathlib
import re


def functions(text):
    found = {}
    # Ignore braces inside comments/literals while keeping source offsets.
    tokens = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    masked = tokens.sub(lambda m: re.sub(r'[^\n]', ' ', m[0]), text)
    pattern = re.compile(r'(?m)^((?:(?:static|inline|const|struct|enum|unsigned|signed)\s+)*[A-Za-z_]\w*(?:\s*\*)?\s+([A-Za-z_]\w*)\([^;{}]*?\))\s*\{')
    for match in pattern.finditer(masked):
        cursor = match.end()
        depth = 1
        while depth:
            if cursor >= len(masked): raise ValueError(f'Unclosed function: {match[2]}')
            if masked[cursor] == '{': depth += 1
            elif masked[cursor] == '}': depth -= 1
            cursor += 1
        found[match[2]] = (match.start(), text[match.start():cursor])
    return found


def generate(root, output):
    base = root / 'upstream/melee-pc/src'
    paths = ['melee/gm/gm_1601.c', 'melee/gm/gmmain_lib.c', 'melee/mn/mnname.c']
    sources = {p: re.sub(r'#ifdef MUST_MATCH\b(.*?)#endif\s*',
                        lambda m: m[1].split('#else', 1)[1] if '#else' in m[1] else '',
                        (base/p).read_text(encoding='utf-8'), flags=re.S) for p in paths}
    inventories = {p: functions(t) for p, t in sources.items()}
    lookup = {name: (p, body) for p, fs in inventories.items() for name, (_, body) in fs.items()}
    # Existing gameplay exports stay in their current compilation units.
    exports = set()
    exports.discard('gm_SelKindToCKind')  # replace the old identity stub
    for native in ('menu_scene_xdk.c', 'fighter_glue_xdk.c', 'match_scene_xdk.c', 'character_select_xdk.c'):
        exports.update(functions((root/'src/xdk'/native).read_text(encoding='utf-8')))
    exports.discard('gm_SelKindToCKind')
    menu_units = ('mncharsel', 'mnmainrule', 'mnruleplus', 'mnitemsw', 'mnstagesw', 'mnstagesel')
    seeds = '\n'.join((base/f'melee/mn/{name}.c').read_text(encoding='utf-8') for name in menu_units)
    selected = set()
    pending = re.findall(r'\b([A-Za-z_]\w*)\s*\(', seeds)
    pending.extend(('gm_80167BC8', 'gm_80167FC4', 'gm_SelKindToCKind', 'gm_SetupAllPlayerDefaults', 'gm_SetupRulesDefaults'))
    while pending:
        name = pending.pop()
        if name in selected or name in exports or name not in lookup:
            continue
        selected.add(name)
        pending.extend(re.findall(r'\b([A-Za-z_]\w*)\s*\(', lookup[name][1]))
    output.mkdir(parents=True, exist_ok=True)
    audio = functions((base/'melee/lb/lbaudio_ax.c').read_text(encoding='utf-8'))
    selectors = '\n\n'.join(audio[name][1] for name in ('lbAudioAx_80024030','lbAudioAx_8002411C','lbAudioAx_800243F4'))
    selectors = selectors.replace('fn_80023750(', 'M360_AudioSubmitOriginal(')
    (output/'audio_menu_select.c').write_text(
        '#include <melee/lb/lbaudio_ax.h>\n#define VOL_MAX 0x7F\n#define PAN_MID 0x40\n'
        'int M360_AudioSubmitOriginal(int,int,int,int,int);\n'+selectors+'\n', encoding='utf-8')
    for path, text in sources.items():
        fs = inventories[path]
        prologue = text[:min(start for start, _ in fs.values())]
        bodies = [body for name, (_, body) in fs.items() if name in selected]
        prologue = '#ifndef U64_MAX\n#define U64_MAX (~0ull)\n#define U32_MAX (~0u)\n#endif\n'+prologue
        (output/(pathlib.Path(path).stem+'_menu.c')).write_text(prologue+'\n\n'.join(bodies)+'\n', encoding='utf-8')
    # Keep the SIS interpreter and layout unchanged; route only GX submission
    # to native Xbox rendering instead of writing the GameCube FIFO address.
    text = (base/'sysdolphin/baselib/hsd_3A76.c').read_text(encoding='utf-8')
    text = text.replace('/* The SIS bytecode', '#include "sis_gx_xdk.h"\n\n/* The SIS bytecode', 1)
    (output/'hsd_3A76_native.c').write_text(text, encoding='utf-8')
    font = (base/'pc/discfont.c').read_text(encoding='utf-8')
    fs = functions(font)
    load = fs['load_from_dol'][1]
    start = load.index('    ptrdiff_t debug_off')
    end = load.index('    /* The sislib atlas')
    load = load[:start] + load[end:]
    prefix = '''#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
extern "C" {
#include <sysdolphin/baselib/sislib_font.h>
unsigned char* M360_ReadDiscFile(const char*, unsigned*);
}
#define SIS_GLYPH_BYTES (int)sizeof(TextGlyphTexture)
#define SIS_GLYPH_COUNT 287
'''
    (output/'discfont_sis.cpp').write_text(prefix+fs['find_bytes'][1]+'\n'+load+'''
extern "C" int M360_LoadDiscFonts(void) {
    unsigned size = 0;
    unsigned char* dol = M360_ReadDiscFile("main.dol", &size);
    bool ok = dol && load_from_dol(dol, size, "GALE01");
    _aligned_free(dol);
    return ok;
}
''', encoding='utf-8')
    print(f'Extracted {len(selected)} original menu/save accessors')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    generate(pathlib.Path(__file__).resolve().parents[1], args.output)
