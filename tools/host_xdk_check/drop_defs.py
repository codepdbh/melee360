#!/usr/bin/env python3
"""Remove top-level definitions of the given names from a C file (used to
retire native stubs once the original unit links): drop_defs.py FILE NAME..."""
import re, sys
path, names = sys.argv[1], set(sys.argv[2:])
lines = open(path).read().split('\n')
out, i, removed = [], 0, []
def item_name(text):
    m = re.match(r'\s*(?:UNPORTED_BOOL|UNPORTED_VOID|SILENT_VOID|ITEM_SWING_STATE)\((\w+)', text)
    if m:
        return m.group(1)
    m = re.match(r'\s*[A-Za-z_][\w\s\*]*?[\s\*](\w+)\s*(\(|;|\[|=)', text)
    return m.group(1) if m else None
while i < len(lines):
    line = lines[i]
    if not line.strip() or line.startswith((' ', '\t', '#', '/*', '*', '//', '}')):
        out.append(line); i += 1; continue
    j, depth, text = i, 0, ''
    while j < len(lines):
        text += lines[j] + '\n'
        depth += lines[j].count('{') - lines[j].count('}')
        s = lines[j].rstrip()
        j += 1
        if depth == 0 and (s.endswith('}') or s.endswith(';') or s.endswith(')') and not s.endswith(',')):
            if s.endswith(')') and j < len(lines) and lines[j].startswith('{'):
                continue
            break
    name = item_name(text)
    if name in names:
        removed.append(name)
    else:
        out.extend(lines[i:j])
    i = j
open(path, 'w').write('\n'.join(out))
print('removed', len(removed), 'missing', sorted(names - set(removed)))
