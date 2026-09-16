#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMU="$ROOT/tools/xenon-emulator/build/m360-headless/Xenon"
ELF="$ROOT/build-x360/melee360-test.elf32"
RUN_HOME="$ROOT/build-x360/xenon-home"
CONFIG="$RUN_HOME/.local/share/Xenon/config.toml"
SECONDS_TO_RUN="${1:-10}"

[[ -x "$EMU" ]] || { echo '[M360][EMU][ERROR] headless Xenon emulator is not built' >&2; exit 1; }
[[ -f "$ELF" ]] || { echo '[M360][EMU][ERROR] platform test ELF is missing' >&2; exit 1; }
mkdir -p "$RUN_HOME/.local/share"

if [[ ! -f "$CONFIG" ]]; then
    timeout 2 env HOME="$RUN_HOME" "$EMU" >/dev/null 2>&1 || true
fi
[[ -f "$CONFIG" ]] || { echo '[M360][EMU][ERROR] emulator did not generate config.toml' >&2; exit 1; }

# The emulator owns this generated configuration. Change only the two values
# required for its documented direct ELF loader.
sed -i -E 's/^(ElfLoader[[:space:]]*=[[:space:]]*).*/\1true/' "$CONFIG"
sed -i -E 's/^(Simulate1BL[[:space:]]*=[[:space:]]*).*/\1true/' "$CONFIG"
sed -i -E 's/^(UARTSystem[[:space:]]*=[[:space:]]*).*/\1"print"/' "$CONFIG"
escaped_elf=${ELF//\\/\\\\}
escaped_elf=${escaped_elf//\"/\\\"}
sed -i -E "s|^(ElfBinary[[:space:]]*=[[:space:]]*).*|\1\"$escaped_elf\"|" "$CONFIG"

echo "[M360][EMU] running direct ELF loader for ${SECONDS_TO_RUN}s"
set +e
timeout --signal=INT "$SECONDS_TO_RUN" env HOME="$RUN_HOME" "$EMU"
status=$?
set -e
if [[ $status -ne 0 && $status -ne 124 && $status -ne 130 ]]; then
    echo "[M360][EMU][ERROR] emulator exited with status $status" >&2
    exit "$status"
fi
