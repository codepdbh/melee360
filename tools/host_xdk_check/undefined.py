#!/usr/bin/env python3
"""List symbols referenced by the match objects that none of them define.
Usage: undefined.py <obj>... ; prints sorted names (one per line)."""
import subprocess, sys
defined, undef, dup = set(), {}, {}
for obj in sys.argv[1:]:
    out = subprocess.run(['/usr/lib/llvm-18/bin/llvm-nm', obj], capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == 'U':
            undef.setdefault(parts[1], set()).add(obj.rsplit('/', 1)[-1])
        elif len(parts) == 3 and parts[1] in 'TDBRC':
            if parts[1] != 'C' and parts[2] in defined:
                dup.setdefault(parts[2], []).append(obj.rsplit('/', 1)[-1])
            defined.add(parts[2])
for name in sorted(set(undef) - defined):
    print(name.lstrip('_'), ' '.join(sorted(undef[name])))
for name, objs in sorted(dup.items()):
    print('DUPLICATE', name, ' '.join(objs), file=sys.stderr)
