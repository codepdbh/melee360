"""Extract original surface materials and fighter queries for the native port."""
import argparse
import pathlib
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parent.parent
source = root / 'upstream/melee-pc/src/melee'
mp = (source / 'mp/mplib.c').read_text(encoding='utf-8')
ft = (source / 'ft/ft_081B.c').read_text(encoding='utf-8')
coll = (source / 'mp/mpcoll.c').read_text(encoding='utf-8')

def function(text, signature):
    start = text.index(signature)
    cursor = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[cursor] == '{') - (text[cursor] == '}')
        cursor += 1
    return text[start:cursor]

structs = mp[mp.index('struct mpLib_803BF248_t_x4 {'):mp.index('/* 458868 */')]
tables = mp[mp.index('struct mpLib_803BF248_t_x4 mpLib_803BD3D8 ='):mp.index('extern DiscVec2 mpLib_803BF718')]
assert len(re.findall(r'\{ Gr_Kind_\w+, &mpLib_', tables)) == 71
gr_enum = re.search(r'typedef enum GrKind \{.*?\} GrKind;',
                   (source / 'gr/forward.h').read_text(encoding='utf-8'), re.S)[0]
pieces = ['/* Generated from mplib.c, mpcoll.c and ft_081B.c; diagnostic tracing is optional. */',
          '#ifndef M360_SURFACE_MATERIAL_HOST',
          '#include <melee/mp/mplib.h>', '#include <melee/mp/mpcoll.h>',
          '#include <melee/ft/ft_081B.h>', '#include <melee/ft/types.h>',
          '#include <melee/ft/inlines.h>',
          '#include <melee/gr/forward.h>', '#include "match_xdk.h"',
          '#else', gr_enum, '#endif', structs, tables]
pieces.append('''#ifdef M360_SURFACE_MATERIAL_TRACE
extern void M360_MatchTrace(const char*, unsigned);
static void TraceSurfaceMaterial(int mode, unsigned flags, int sound, int count, int effect)
{
    static unsigned seen[3][71];
    int ground = M360_MatchGroundKind();
    unsigned material = (u8) flags;
    if (mode < 0 || mode > 2 || ground < 0 || ground >= 71 || material >= 20) return;
    if (seen[mode][ground] & (1u << material)) return;
    seen[mode][ground] |= 1u << material;
    M360_MatchTrace("surface.query.ground", (unsigned) ground);
    M360_MatchTrace("surface.query.material", material);
    M360_MatchTrace("surface.query.mode", (unsigned) mode);
    M360_MatchTrace("surface.query.sound", (unsigned) sound);
    M360_MatchTrace("surface.query.count", (unsigned) count);
    M360_MatchTrace("surface.query.effect", (unsigned) effect);
}
#endif''')
for signature in ['float mpLib_800569EC(', 'int* mpLib_80056A1C(',
                  'int mpLib_80056A54(', 'int* mpLib_80056A8C(',
                  'int mpLib_80056AC4(', 'int* mpLib_80056AFC(', 'int mpLib_80056B34(']:
    # Stage ownership belongs to the native scene adapter. Game queries and
    # their low-byte material indexing retain their original implementation.
    pieces.append(function(mp, signature).replace('stage_info.grkind', 'M360_MatchGroundKind()'))
pieces.append(function(coll, 'float mpColl_8004CA6C('))
for signature in ['float ft_GetGroundFrictionMultiplier(', 'bool ft_80084A80(',
                  'bool ft_80084BFC(', 'bool ft_80084C38(', 'bool ft_80084C74(']:
    body = function(ft, signature)
    if signature == 'bool ft_80084A80(':
        body = body.replace('return true;', '''#ifdef M360_SURFACE_MATERIAL_TRACE
            TraceSurfaceMaterial(arg0, temp_r26, arg1[0], arg2[0], arg3[0]);
#endif
            return true;''')
    pieces.append(body)
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text('\n\n'.join(pieces) + '\n', encoding='ascii')
