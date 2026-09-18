#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build-x360/host-tests/test_memory"

mkdir -p "$(dirname "$OUT")"
cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -I"$ROOT/upstream/melee-pc/src" \
    -I"$ROOT/upstream/melee-pc/src/sdk_include" \
    -include "$ROOT/src/xdk/memory_xdk_compat.h" \
    -c "$ROOT/upstream/melee-pc/src/sysdolphin/baselib/memory.c" \
    -o "$ROOT/build-x360/host-tests/memory.o"
c++ -std=c++17 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -c "$ROOT/src/xdk/memory_xdk.cpp" \
    -o "$ROOT/build-x360/host-tests/memory_xdk.o"
cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -I"$ROOT/upstream/melee-pc/src/sysdolphin/baselib" \
    -I"$ROOT/upstream/melee-pc/src" \
    -I"$ROOT/upstream/melee-pc/src/sdk_include" \
    -include "$ROOT/src/xdk/memory_xdk_compat.h" \
    "$ROOT/tests/host/test_memory.c" \
    -c -o "$ROOT/build-x360/host-tests/test_memory.o"
c++ "$ROOT/build-x360/host-tests/memory.o" \
    "$ROOT/build-x360/host-tests/memory_xdk.o" \
    "$ROOT/build-x360/host-tests/test_memory.o" \
    -o "$OUT"
"$OUT"
