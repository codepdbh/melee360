# Upstream review - 2026-10-26

Public references for `999sian/melee-pc` were queried and the existing checkout
ran `git fetch --no-tags origin master`. The compiled base remains clean at
`6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`; fetching did not merge source.

| Reference | Observed revision |
|---|---|
| master | `0fb0af3304d3469308b5574369a6ec86f145632f` |
| main | `1627cf10cf6afd116b2442d3d2baf325915b6961` |
| Previous master review | `0cc24fe` (October 19) |

The interval `0cc24fe..0fb0af3` contains 21 commits and changes to 92 files.
All nine affected game-source diffs were reviewed, alongside platform commit
subjects. There are no new `mp/` changes in this interval.

## Findings

- [Super Scope and Multi-Man records](https://github.com/999sian/melee-pc/commit/1c544cd)
  replace GameCube offset overlays with named fields to avoid corrupting
  64-bit structures. Xenon uses 32-bit pointers; these do not address the
  native landing solver corrected in this checkpoint.
- [Mushroom Kingdom block](https://github.com/999sian/melee-pc/commit/60c8dfd)
  reads its Item JObj by field instead of the GameCube `0xDD8` offset.
  Retain this fix when integrating that callback and its objects/enemies.
  The reported upstream crash is specific to the larger 64-bit layout;
  this change alone does not complete Adventure's course.
- [Credits initialization](https://github.com/999sian/melee-pc/commit/6ee1f71)
  clears all 198 `staffInfo` entries instead of one. This is useful for future
  credits integration. The native build does not compile `gmstaffroll.c`
  or offer the complete original ending yet.
- [Widescreen magnifier bars](https://github.com/999sian/melee-pc/commit/171f68f)
  change EFB copying and camera flags in the PC/Aurora renderer. The XDK port
  uses a separate renderer; copying the flag would not implement the Xenos
  magnifier.
- Other changes add PC scene diagnostics and boot/CPU selection options,
  Android/browser shaders, and macOS packaging. They do not provide native
  Xbox campaign or collision implementations.

No new backports were applied in this review. The original floor-query work
in `77df962` is extracted from the pinned base and tested separately.
The earlier sound-bank backport remains documented in the
[October 16 audit](UPSTREAM_AUDIT_20261016.md).
