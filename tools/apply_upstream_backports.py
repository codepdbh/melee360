"""Apply reviewed upstream fixes to generated XDK sources; keep the pin clean."""
import argparse
import json
from pathlib import Path

MANIFEST = Path(__file__).with_name('upstream_backports.json')


def apply_backport(name, source):
    patch = json.loads(MANIFEST.read_text(encoding='utf-8'))[name]
    for old, new in patch['edits']:
        if source.count(old) != 1:
            raise ValueError(f'{name}: expected one original fragment from {patch["source"]}')
        source = source.replace(old, new, 1)
    return source


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--name', required=True)
    p.add_argument('--input', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    args = p.parse_args()
    args.output.write_text(apply_backport(args.name, args.input.read_text(encoding='utf-8')), encoding='utf-8')
