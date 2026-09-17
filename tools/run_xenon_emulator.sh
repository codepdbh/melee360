#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMU="$ROOT/tools/xenon-emulator/build/m360-gfx/Xenon"
ELF="$ROOT/dist/xenon-emulator.elf"
RUN_HOME="$ROOT/build-x360/xenon-home"
CONFIG="$RUN_HOME/.local/share/Xenon/config.toml"

[[ -x "$EMU" ]] || {
    echo '[M360][EMU][ERROR] graphical Xenon emulator is not built' >&2
    exit 1
}
[[ -f "$ELF" ]] || {
    echo '[M360][EMU][ERROR] run tools/build_xenon_emulator_elf.sh first' >&2
    exit 1
}

mkdir -p "$RUN_HOME/.local/share"
if [[ ! -f "$CONFIG" ]]; then
    timeout 2 env XENON_GUI_ONLY=1 HOME="$RUN_HOME" "$EMU" >/dev/null 2>&1 || true
fi
[[ -f "$CONFIG" ]] || {
    echo '[M360][EMU][ERROR] emulator did not generate config.toml' >&2
    exit 1
}

escaped_elf=${ELF//\\/\\\\}
escaped_elf=${escaped_elf//\"/\\\"}
sed -i -E 's/^(ElfLoader[[:space:]]*=[[:space:]]*).*/\1true/' "$CONFIG"
sed -i -E 's/^(Simulate1BL[[:space:]]*=[[:space:]]*).*/\1true/' "$CONFIG"
sed -i -E 's/^(UARTSystem[[:space:]]*=[[:space:]]*).*/\1"print"/' "$CONFIG"
sed -i -E "s|^(ElfBinary[[:space:]]*=[[:space:]]*).*|\1\"$escaped_elf\"|" "$CONFIG"

echo '[M360][EMU] launching working PowerPC integration build'
exec env HOME="$RUN_HOME" "$EMU"
