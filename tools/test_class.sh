#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build-x360/host-tests/test_class"
OBJ="$ROOT/build-x360/host-tests"
BASELIB="$ROOT/upstream/melee-pc/src/sysdolphin/baselib"

mkdir -p "$OBJ"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -I"$ROOT/upstream/melee-pc/src" \
    -I"$ROOT/upstream/melee-pc/src/sdk_include" \
    -include "$ROOT/src/xdk/memory_xdk_compat.h" \
    -c "$ROOT/upstream/melee-pc/src/sysdolphin/baselib/memory.c" \
    -o "$OBJ/memory.o"
c++ -std=c++17 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -c "$ROOT/src/xdk/memory_xdk.cpp" \
    -o "$OBJ/memory_xdk.o"

c++ -std=c++17 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -c "$ROOT/src/xdk/hsd_class_xdk.cpp" \
    -o "$OBJ/hsd_class_xdk.o"

for f in hash debug class object objalloc id; do
    extra=()
    if [ "$f" = "debug" ]; then
        extra+=(-DTARGET_PC)
    fi
    cc -std=c11 -Wall -Wextra -Werror "${extra[@]}" \
        -I"$ROOT/src/xdk" \
        -I"$ROOT/upstream/melee-pc/src" \
        -I"$ROOT/upstream/melee-pc/src/sdk_include" \
        -include "$ROOT/src/xdk/hsd_class_xdk_compat.h" \
        -c "$BASELIB/$f.c" \
        -o "$OBJ/$f.o"
done

cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/src/xdk" \
    -I"$BASELIB" \
    -I"$ROOT/upstream/melee-pc/src" \
    -I"$ROOT/upstream/melee-pc/src/sdk_include" \
    -include "$ROOT/src/xdk/hsd_class_xdk_compat.h" \
    "$ROOT/tests/host/test_class.c" \
    -c -o "$OBJ/test_class.o"

c++ "$OBJ/memory.o" "$OBJ/memory_xdk.o" "$OBJ/hsd_class_xdk.o" \
    "$OBJ/hash.o" "$OBJ/debug.o" "$OBJ/class.o" "$OBJ/object.o" \
    "$OBJ/objalloc.o" "$OBJ/id.o" "$OBJ/test_class.o" \
    -o "$OUT"
"$OUT"
