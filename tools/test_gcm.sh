#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISO="${1:-$ROOT/iso/MeleeUSAv1.02.iso}"
OUT="$ROOT/build-x360/host-tests/test_gcm"

[[ -f "$ISO" ]] || {
    printf '[M360][GCM][ERROR] ISO not found: %s\n' "$ISO" >&2
    exit 1
}

mkdir -p "$(dirname "$OUT")"
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/common" \
    "$ROOT/src/common/gcm.c" "$ROOT/tests/host/test_gcm.c" -o "$OUT"
"$OUT" "$ISO"
