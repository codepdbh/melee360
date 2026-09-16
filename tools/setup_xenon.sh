#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${DEVKITXENON:-$ROOT/.devkitxenon}"
MODE="${1:---check}"

log() { printf '[M360][SETUP] %s\n' "$*"; }
fail() { printf '[M360][SETUP][ERROR] %s\n' "$*" >&2; exit 1; }

case "$(uname -s)" in Linux) ;; *) fail 'Run this script on Linux or WSL.' ;; esac

if command -v xenon-gcc >/dev/null 2>&1; then
    log "native compiler: $(command -v xenon-gcc)"
    xenon-gcc --version | head -1
    exit 0
fi

DOCKER=()
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
    DOCKER=(docker)
elif [[ -x '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' ]] && \
     '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' info >/dev/null 2>&1; then
    DOCKER=('/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe')
fi

if ((${#DOCKER[@]})); then
    log 'official free60/libxenon Docker toolchain is available'
    "${DOCKER[@]}" image inspect free60/libxenon:latest >/dev/null 2>&1 || "${DOCKER[@]}" pull free60/libxenon:latest
    "${DOCKER[@]}" run --rm free60/libxenon:latest sh -lc 'printf "DEVKITXENON=%s\n" "$DEVKITXENON"; xenon-gcc --version | head -1'
    exit 0
fi

if [[ "$MODE" != '--install-native' ]]; then
    fail 'No compiler found. Enable Docker integration in WSL or rerun with --install-native.'
fi

for tool in git make gcc g++ wget patch flex bison makeinfo; do
    command -v "$tool" >/dev/null 2>&1 || fail "missing host dependency: $tool"
done

log "building the native toolchain in $PREFIX (this can take a long time)"
export PREFIX
export PARALLEL="${PARALLEL:--j$(nproc)}"
"$ROOT/third_party/libxenon/toolchain/build-xenon-toolchain" toolchain
"$ROOT/third_party/libxenon/toolchain/build-xenon-toolchain" libxenon
log "export DEVKITXENON='$PREFIX'"
log 'export PATH="$PATH:$DEVKITXENON/bin:$DEVKITXENON/usr/bin"'
