#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$ROOT/tests/xbox360"
TARGET="$ROOT/build-x360/melee360-emulator"

mkdir -p "$ROOT/build-x360" "$ROOT/dist"
"$ROOT/tools/generate_xenos_shaders.sh"

if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
    docker run --rm -v "$ROOT:/project" -w /project/tests/xbox360 \
        free60/libxenon:latest sh -lc \
        'make clean && make all EMULATOR=1 TARGET=/project/build-x360/melee360-emulator'
elif [[ -x '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' ]] && \
     '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' info >/dev/null 2>&1; then
    WIN_ROOT="$(wslpath -w "$ROOT")"
    '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' run --rm \
        -v "$WIN_ROOT:/project" -w /project/tests/xbox360 \
        free60/libxenon:latest sh -lc \
        'make clean && make all EMULATOR=1 TARGET=/project/build-x360/melee360-emulator'
else
    echo '[M360][EMU][ERROR] Docker/libxenon toolchain is unavailable' >&2
    exit 1
fi

[[ -f "$TARGET.elf32" ]] || {
    echo '[M360][EMU][ERROR] emulator ELF was not produced' >&2
    exit 1
}
cp "$TARGET.elf32" "$ROOT/dist/xenon-emulator.elf"
echo '[M360][EMU] created dist/xenon-emulator.elf'
