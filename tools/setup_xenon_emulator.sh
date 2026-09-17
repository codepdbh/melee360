#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="$ROOT/tools/xenon-emulator"
PATCH="$ROOT/patches/xenon-emulator/0001-direct-elf-without-nand.patch"
PIN="0284bbe6c8125935d97bf54ab3132089c6c65c8b"

if [[ ! -d "$REPO/.git" ]]; then
    git clone --recursive https://github.com/xenon-emu/xenon.git "$REPO"
fi

current="$(git -C "$REPO" rev-parse HEAD)"
if [[ "$current" != "$PIN" ]]; then
    echo "[M360][EMU][ERROR] expected Xenon commit $PIN, found $current" >&2
    exit 1
fi
git -C "$REPO" submodule update --init --recursive

if git -C "$REPO" apply --check --reverse --unidiff-zero "$PATCH"; then
    echo '[M360][EMU] compatibility patch already applied'
elif git -C "$REPO" apply --check --unidiff-zero "$PATCH"; then
    git -C "$REPO" apply --unidiff-zero "$PATCH"
    echo '[M360][EMU] applied direct-ELF compatibility patch'
else
    echo '[M360][EMU][ERROR] local emulator changes conflict with the patch' >&2
    exit 1
fi

cmake -S "$REPO" -B "$REPO/build/m360-headless" \
    -DCMAKE_BUILD_TYPE=Release -DGFX_ENABLED=OFF \
    -DXENON_USE_SYSTEM_DEPS=OFF
cmake --build "$REPO/build/m360-headless" -j2

cmake -S "$REPO" -B "$REPO/build/m360-gfx" \
    -DCMAKE_BUILD_TYPE=Release -DGFX_ENABLED=ON
cmake --build "$REPO/build/m360-gfx" -j2

echo '[M360][EMU] headless and graphical emulator builds are ready'
