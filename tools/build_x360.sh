#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$ROOT/tests/xbox360"

stage() { printf '\n[M360][BUILD] %s\n' "$*"; }
fail() { printf '[M360][BUILD][ERROR] %s\n' "$*" >&2; exit 1; }

mkdir -p "$ROOT/build-x360" "$ROOT/dist"
"$ROOT/tools/generate_xenos_shaders.sh"

DOCKER=()
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
    DOCKER=(docker)
elif [[ -x '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' ]] && \
     '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' info >/dev/null 2>&1; then
    DOCKER=('/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe')
fi

if [[ -n "${DEVKITXENON:-}" && -x "$DEVKITXENON/bin/xenon-gcc" ]]; then
    stage "native toolchain: $DEVKITXENON"
    make -C "$TEST_DIR" clean all
elif ((${#DOCKER[@]})); then
    stage 'official free60/libxenon Docker toolchain'
    MOUNT_ROOT="$ROOT"
    [[ "${DOCKER[0]}" == *.exe ]] && MOUNT_ROOT="$(wslpath -w "$ROOT")"
    "${DOCKER[@]}" run --rm -v "$MOUNT_ROOT:/project" -w /project/tests/xbox360 \
        free60/libxenon:latest sh -lc 'make clean all'
else
    fail 'No usable Xenon toolchain. Run tools/setup_xenon.sh.'
fi

[[ -f "$ROOT/build-x360/melee360-test.elf32" ]] || fail 'link completed without expected ELF32 output'
cp "$ROOT/build-x360/melee360-test.elf32" "$ROOT/dist/xenon.elf"
stage 'created build-x360/melee360-test.elf32 and dist/xenon.elf'
file "$ROOT/build-x360/melee360-test.elf32" || true
