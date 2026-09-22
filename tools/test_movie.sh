#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISO="${1:-$ROOT/dist/melee.iso}"
OUT="${2:-}"
OBJ="$ROOT/build-x360/host-tests/msvc"
SRC=("$ROOT/src/common/gcm.c" "$ROOT/src/common/jpeg_decode.c"
     "$ROOT/src/common/mth.c" "$ROOT/tests/host/test_movie.c")

[[ -f "$ISO" ]] || {
    printf '[M360][MOVIE][ERROR] ISO not found: %s\n' "$ISO" >&2
    exit 1
}
mkdir -p "$OBJ"
VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
WSRC=""
for f in "${SRC[@]}"; do WSRC="$WSRC $(cygpath -w "$f")"; done
WOBJ="$(cygpath -w "$OBJ")"
BAT="$OBJ/test_movie.bat"
cat > "$BAT" <<BATCH
@echo off
call "$VCVARS" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /D_CRT_SECURE_NO_WARNINGS /MT /O2 /TC /W4 /WX /I$(cygpath -w "$ROOT/src/common") /Fo$WOBJ\ /Fe$WOBJ\test_movie.exe $WSRC || exit /b 1
$WOBJ\test_movie.exe "$(cygpath -w "$ISO")" ${OUT:+"$(cygpath -w "$OUT")"}
exit /b %errorlevel%
BATCH
cmd.exe //c "$(cygpath -w "$BAT")"
