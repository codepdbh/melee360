#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repos=(upstream/melee-pc upstream/doldecomp-melee third_party/libxenon reference/xenon-examples reference/xell tools/xenon-emulator)

for rel in "${repos[@]}"; do
    repo="$ROOT/$rel"
    [[ -d "$repo/.git" ]] || { printf '[M360][UPDATE] missing %s\n' "$rel"; continue; }
    printf '\n[M360][UPDATE] %s\n' "$rel"
    printf 'current:  '; git -C "$repo" rev-parse HEAD
    git -C "$repo" fetch --prune --tags
    branch="$(git -C "$repo" branch --show-current)"
    upstream="$(git -C "$repo" rev-parse --abbrev-ref '@{upstream}' 2>/dev/null || true)"
    if [[ -n "$upstream" ]]; then
        printf 'available commits (%s..%s):\n' "$branch" "$upstream"
        git -C "$repo" log --oneline --decorate "$branch..$upstream" || true
    else
        printf 'no tracking branch; no update performed\n'
    fi
done

printf '\nNo repository was modified. Update explicitly with git -C <repo> merge --ff-only <remote/branch>.\n'

