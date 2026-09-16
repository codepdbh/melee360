#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rm -rf "$ROOT/tests/xbox360/build"
rm -f "$ROOT/build-x360/melee360-test.elf" \
       "$ROOT/build-x360/melee360-test.elf32" \
       "$ROOT/build-x360/melee360-test.map" \
       "$ROOT/dist/xenon.elf"
printf '[M360][CLEAN] Xbox 360 generated files removed\n'

