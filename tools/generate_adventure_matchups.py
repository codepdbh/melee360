"""Generate native Adventure encounter metadata from the checked-out game source."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
source = root / 'upstream/melee-pc/src/melee'
text = (source / 'gm/gmadventure.c').read_text(encoding='utf-8')
table = text[text.index('struct gm_803DE650_t gm_803DE650[]'):text.index('static u8 gm_80490910')]
stage_text = (source / 'gr/stage.c').read_text(encoding='utf-8')
stage_table = re.search(r'struct StageIdMapEntry stage_id_map\[\] = \{(.*?)\n\};', stage_text, re.S)[1]
stages = re.findall(r'\{\s*(Gr_Kind_\w+),\s*0,\s*0\s*\}', stage_table)
player_text = (source / 'pl/player.c').read_text(encoding='utf-8')
player_table = re.search(r'ftMapping ftMapping_list\[ChKind_Max\] = \{(.*?)\n\};', player_text, re.S)[1]
characters = re.findall(r'/\*\s*((?:CKind|ChKind)_\w+)\s*\*/\s*\{\s*(Ft_Kind_\w+)', player_table)
character_names = dict(characters)
gr = {name: int(number, 16) for number, name in re.findall(r'/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s*(Gr_Kind_\w+)', (source / 'gr/forward.h').read_text(encoding='utf-8'))}
ft = {name: int(number, 16) for number, name in re.findall(r'/\*\s*([0-9A-Fa-f]{2})\s*\*/\s*(Ft_Kind_\w+)', (source / 'ft/forward.h').read_text(encoding='utf-8'))}
# Traversal, Brinstar escape and native fighter encounters. The race and
# climbing courses still need scene code. Giga eligibility is handled natively.
scene_ids = [1, 3, 9, 10, 17, 18, 25, 27, 33, 35, 37, 41, 43, 49, 59, 65, 81, 83, 89, 92]
records = {}
for entry in re.finditer(r'\{([^{}]+)\}', table):
    values = [value.strip() for value in entry[1].split(',') if value.strip()]
    if len(values) >= 10:
        records[int(values[0], 0)] = values
# gm_8017E48C counts ALL original GS_VS scenes, including courses which the
# native port does not yet load. Using our filtered round number shifts stats.
state_table = text[text.index('GameModeState gm_Mode_Adventure_States[]'):text.index('struct gm_803DE650_t gm_803DE650[]')]
enum_values = {'ADVENTURE_MUSHROOM_KINGDOM': 1, 'ADVENTURE_MARIO_PEACH_FIGHT': 3,
               'ADVENTURE_KIRBY_FIGHT': 33, 'ADVENTURE_TEAMKIRBY_FIGHT': 35,
               'ADVENTURE_GIANTKIRBY_FIGHT': 37}
ordinals = {}
depth = 0
start = 0
for index, char in enumerate(state_table):
    if char == '{':
        depth += 1
        if depth == 2:
            start = index
    elif char == '}':
        if depth == 2:
            block = state_table[start:index + 1]
            if 'GS_VS,' in block:
                token = re.match(r'\{\s*(\w+)', block)[1]
                scene = enum_values[token] if token in enum_values else int(token, 0)
                ordinals[scene] = len(ordinals)
        depth -= 1
stats_text = (source / 'gm/gm_17E4.c').read_text(encoding='utf-8')
stats_table = stats_text[stats_text.index('lbl_803D7AC0[110]'):stats_text.index('u8 gm_8017E48C')]
stats = []
cpu_levels = []
cpu_kinds = []
for row in re.finditer(r'\{\s*(0xff)\s*\}|\{\s*\w+,\s*\w+,\s*(\w+),\s*(\w+),\s*\{([^{}]*)\}\s*\}', stats_table):
    stats.append((0, 0) if row[1] else (int(row[2], 0), int(row[3], 0)))
    payload = [] if row[1] else [int(value.strip(), 0) for value in row[4].split(',') if value.strip()]
    payload += [0] * (20 - len(payload))
    cpu_levels.append([payload[index * 3] for index in range(3)])
    cpu_kinds.append([payload[index * 3 + 2] for index in range(3)])
assert len(stats) == 110, 'Original Adventure difficulty table changed'
assert len(ordinals) * 5 == len(stats), 'Adventure scene order does not match difficulty rows'
lines = ['/* Generated from gmadventure.c, stage.c and player.c. */',
         '#ifndef MELEE360_ADVENTURE_MATCHUPS_H', '#define MELEE360_ADVENTURE_MATCHUPS_H',
         'typedef struct M360AdventureEncounter { unsigned scene, flags, seconds, opponents, stkind; int grkind; int fighters[3]; } M360AdventureEncounter;',
         'static const M360AdventureEncounter g_m360Adventure[] = {']
for scene in scene_ids:
    row = records[scene]
    enemies = []
    for character in row[7:10]:
        if character == 'ChKind_None':
            enemies.append(-1)
        else:
            if character.startswith('0x'):
                character = characters[int(character, 0)][0]
            enemies.append(ft[character_names[character]])
    slots = sum(enemy >= 0 for enemy in enemies)
    total = int(row[3], 0) or slots
    # grKinokoRoute_80207C88 calls gm_801674C4(0x11, 0xA, 3, 0xB3, ...):
    # ten Yoshis, with three active slots. The encounter row only names slots.
    if scene == 1:
        total = 10
    lines.append('    { %d, %d, %d, %d, %d, %d, { %s } },' %
                 (scene, int(row[1], 0), int(row[2], 0), total, int(row[4], 0),
                  gr[stages[int(row[4], 0)]], ', '.join(map(str, enemies))))
lines += ['};', '/* Attack and defense percentages: Very Easy through Very Hard. */',
          'static const unsigned short g_m360AdventureRatios[][5][2] = {']
for scene in scene_ids:
    rows = stats[ordinals[scene] * 5:ordinals[scene] * 5 + 5]
    assert len(rows) == 5
    lines.append('    { %s }, /* scene %d */' %
                 (', '.join('{ %d, %d }' % pair for pair in rows), scene))
lines += ['};', '/* gm_8017E5C8: original enemy CPU levels per slot. */',
          'static const unsigned char g_m360AdventureCpuLevels[][5][3] = {']
for scene in scene_ids:
    rows = cpu_levels[ordinals[scene] * 5:ordinals[scene] * 5 + 5]
    lines.append('    { %s }, /* scene %d */' %
                 (', '.join('{ %d, %d, %d }' % tuple(row) for row in rows), scene))
lines += ['};', '/* gm_8017E630: original enemy CPU behavior per slot. */',
          'static const unsigned char g_m360AdventureCpuKinds[][5][3] = {']
for scene in scene_ids:
    rows = cpu_kinds[ordinals[scene] * 5:ordinals[scene] * 5 + 5]
    lines.append('    { %s }, /* scene %d */' %
                 (', '.join('{ %d, %d, %d }' % tuple(row) for row in rows), scene))
lines += ['};', '#define M360_ADVENTURE_LUIGI_KIND %d' % ft['Ft_Kind_Luigi'],
          '#define M360_ADVENTURE_ROUNDS (sizeof(g_m360Adventure) / sizeof(g_m360Adventure[0]))', '#endif']
pathlib.Path(sys.argv[1]).write_text('\n'.join(lines) + '\n', encoding='ascii')
