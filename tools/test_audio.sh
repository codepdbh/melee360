#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISO="${1:-$ROOT/iso/MeleeUSAv1.02.iso}"
WAV="${2:-}"
OBJ="$ROOT/build-x360/host-tests"
SRC=("$ROOT/src/common/gcm.c" "$ROOT/src/common/dsp_adpcm.c"
     "$ROOT/src/common/hps.c" "$ROOT/tests/host/test_audio.c")

[[ -f "$ISO" ]] || {
    printf '[M360][AUDIO][ERROR] ISO not found: %s\n' "$ISO" >&2
    exit 1
}
mkdir -p "$OBJ/msvc"

if command -v cc >/dev/null 2>&1; then
    cc -std=c99 -Wall -Wextra -Werror -I"$ROOT/src/common" "${SRC[@]}" \
        -lm -o "$OBJ/test_audio"
    "$OBJ/test_audio" "$ISO" ${WAV:+"$WAV"}
    exit 0
fi

VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
WSRC=""
for f in "${SRC[@]}"; do WSRC="$WSRC $(cygpath -w "$f")"; done
WOBJ="$(cygpath -w "$OBJ/msvc")"
BAT="$OBJ/msvc/test_audio.bat"
cat > "$BAT" <<BATCH
@echo off
call "$VCVARS" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /D_CRT_SECURE_NO_WARNINGS /MT /O2 /TC /W4 /WX /I$(cygpath -w "$ROOT/src/common") /Fo$WOBJ\ /Fe$WOBJ\test_audio.exe $WSRC || exit /b 1
$WOBJ\test_audio.exe "$(cygpath -w "$ISO")" ${WAV:+"$(cygpath -w "$WAV")"}
exit /b %errorlevel%
BATCH
cmd.exe //c "$(cygpath -w "$BAT")"
