#!/usr/bin/env python3
"""Emit traced C89 stubs for missing match symbols from their upstream
prototypes: gen_stubs.py NAME... > stubs.c. Unnamed parameters get names,
return values are zero/NULL/false. Review the output before committing."""
import glob, os, re, sys
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
src = os.path.join(root, 'upstream/melee-pc/src')
headers = glob.glob(src + '/melee/**/*.h', recursive=True) + glob.glob(src + '/sysdolphin/**/*.h', recursive=True)
text = {h: re.sub(r'/\*.*?\*/', ' ', open(h, encoding='latin-1').read(), flags=re.S) for h in headers}
QUAL = {'const', 'volatile', 'unsigned', 'signed', 'struct', 'enum', 'union', 'long', 'short'}

def find(name):
    for h, t in text.items():
        m = re.search(r'(^|\n)\s*([A-Za-z_][\w\s\*]*?)\b%s\s*\(([^;{]*?)\)\s*;' % re.escape(name), t, re.S)
        if m and 'define' not in m.group(2) and 'typedef' not in m.group(2):
            return ' '.join(m.group(2).split()), ' '.join(m.group(3).split())
    return None

def name_params(params):
    if params.strip() in ('', 'void'):
        return 'void', []
    out, names = [], []
    for i, p in enumerate(x.strip() for x in params.split(',')):
        if p == '...':
            out.append(p)
            continue
        toks = re.findall(r'[\w]+|\*', p)
        named = len(toks) >= 2 and toks[-1] != '*' and toks[-1] not in QUAL and \
            not (len(toks) == 2 and toks[0] in QUAL) and not (toks[-2] in ('struct', 'enum', 'union'))
        if named:
            out.append(p)
            names.append(toks[-1])
        else:
            out.append('%s a%d' % (p, i))
            names.append('a%d' % i)
    return ', '.join(out), names

for name in sys.argv[1:]:
    found = find(name)
    if not found:
        print('/* TODO: no prototype found for %s */' % name)
        continue
    ret, params = found
    ret = re.sub(r'^(static|extern|inline)\s+', '', ret).strip()
    plist, names = name_params(params)
    body = ' '.join('(void) %s;' % n for n in names)
    report = 'Report("fighter.unported.%s");' % name
    if ret == 'void':
        print('%s %s(%s) { %s %s }' % (ret, name, plist, body, report))
    elif ret in ('float', 'f32', 'double', 'f64'):
        print('%s %s(%s) { %s %s return 0.0f; }' % (ret, name, plist, body, report))
    else:
        print('%s %s(%s) { %s %s return (%s) 0; }' % (ret, name, plist, body, report, ret))
