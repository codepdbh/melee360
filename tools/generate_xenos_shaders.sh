#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$ROOT/reference/xenon-examples/cube/source/main.c"
OUTPUT="$ROOT/build-x360/m360_xenos_shaders.h"

[[ -f "$SOURCE" ]] || { echo '[M360][SHADER][ERROR] pinned cube example is missing' >&2; exit 1; }
mkdir -p "$(dirname "$OUTPUT")"
{
    echo '/* Generated from pinned Free60 xenon-examples/cube BSD example. */'
    echo '#ifndef M360_XENOS_SHADERS_H'
    echo '#define M360_XENOS_SHADERS_H'
    sed -n '/^static unsigned char shader_3d_ps/,/^};/p; /^static unsigned char shader_3d_vs/,/^};/p' "$SOURCE" |
        sed 's/shader_3d_ps/m360_shader_ps/; s/shader_3d_vs/m360_shader_vs/'
    echo '#endif'
} > "$OUTPUT"
echo '[M360][SHADER] generated build-x360/m360_xenos_shaders.h from pinned reference'

