#!/usr/bin/env bash
# Host-side validation of tools/build_match_xdk.ps1 without the XDK.
# Compiles every original fighter unit and the native glue with clang in a
# 32-bit MSVC mode, then reports symbols that the match objects reference but
# do not define. The XEX link resolves the remaining HSD/runtime names from
# the other XDK objects; compare against baseline-undefined.txt to find new
# names that would break the real link.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PWSH="${PWSH:-pwsh}"
export XEDK="$ROOT/tools/host_xdk_check/xedk"
OUT="$ROOT/build-host"
if [[ ! -f "$ROOT/build-x360/gameplay-probe/manifest.json" ]] || ! grep -q '"Complete": true' "$ROOT/build-x360/gameplay-probe/manifest.json"; then
    "$PWSH" -NoProfile -File "$ROOT/tools/probe_gameplay_xdk.ps1" >/dev/null
fi
rm -rf "$OUT/match"
"$PWSH" -NoProfile -c "& '$ROOT/tools/build_match_xdk.ps1' -Compiler '$XEDK/bin/win32/cl.exe' -Build '$OUT'" > "$OUT/match.log" 2>&1 || { grep -E "error|throw|failed" "$OUT/match.log" | head -40; exit 1; }
python3 "$ROOT/tools/host_xdk_check/undefined.py" "$OUT"/match/*.obj > "$OUT/undefined.txt" 2> "$OUT/duplicates.txt"
grep -vE "DUPLICATE (\?\?_C@|__real@)" "$OUT/duplicates.txt" || true
awk '{print $1}' "$OUT/undefined.txt" | sort > "$OUT/undefined.names"
awk '{print $1}' "$ROOT/tools/host_xdk_check/baseline-undefined.txt" | sort > "$OUT/baseline.names"
python3 "$ROOT/tools/host_xdk_check/external_defined.py" > "$OUT/external.names"
sort -u "$OUT/baseline.names" "$OUT/external.names" > "$OUT/resolved.names"
comm -23 "$OUT/undefined.names" "$OUT/resolved.names" > "$OUT/new-undefined.names"
echo "match objects: $(ls "$OUT"/match/*.obj | wc -l); undefined: $(wc -l < "$OUT/undefined.names"); new vs baseline: $(wc -l < "$OUT/new-undefined.names")"
grep -wFf "$OUT/new-undefined.names" "$OUT/undefined.txt" || true
