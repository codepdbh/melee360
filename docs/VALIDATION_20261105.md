# Fighter ECB sampling and air-surface filters - 2026-11-05

## Changes

The native ceiling response samples the six collision bones using the
original `mpColl_LoadECB_JObj` and `mpColl_80042384` functions. The adapter
uses a temporary copy of collision data, preserving the live buffers until
the full original solver is integrated. It samples relative to the current
root transform because this port commits the fighter root position after
collision processing. A locked bottom is preserved as in the original loader.

`AirWalls` and `AirCeilings` now reject lines marked empty and respect the
collision data's skipped and required stage-joint filters. These functions
remain native approximations; the change does not implement original ECB
substeps, wall response, moving-line remapping or the complete air solver.

`tools/test_fighter_ecb_top.ps1` extracts the actual original sampling code
and native callers. It checks animation changes, root translation, locked
bottom, fallback for a missing bone, preservation of live data, real native
ceiling contact, and both group filters and empty lines for air contacts.
The existing collision-lifecycle, floor-query and surface-query suites pass.

## First build: ECB sampling

- Source commit: `059b8cdd265fc9898b112731a7e581b4f8dd1776`.
- Normal XEX SHA-256: `4B3FD9740EE4A7D95E630A026A3E5891EC03F61DFD23C9AF1590EA5F1CCDDE6E`.
- Normal map SHA-256: `07C6F7C2EA7D8E896197CC098B840C191F927AEC766DDA04953118416DED3E93`.
- Paired diagnostic XEX/map SHA-256: `2576C49F400B531E6DC8A826BC2B3F9677A9963EAC64FDF4636FBEB422C90C8B` /
  `C62D277580D696BA82C5090C3134CADEDD896ED30F49960BB97024F8F74AFD54`.

All 19 bounded native Xenia checks pass: normal opening; teeter; items in
Hyrule Temple and Corneria plus Off; seven four-CPU roster batches; two
Zelda/Sheik transformations; life reset; Classic giant/metal; Adventure
wireframes/Giga. The two stage cases create 12 random items and Off creates
none. The opening reaches 1800 loop/900 movie frames with zero movie/history
errors. Gameplay cases have zero decode/history errors, no missing markers
and no watchdog. Identities and results were checked before the second build
in `build-x360/nov05-ecb-top/validation-summary.json`.

## Second build: ECB sampling and air-surface filters

- Source commit: `e43d908951f52ddec69cb705b9fc6864bd8151c9`.
- Normal XEX SHA-256: `CC9D94B1D24E30DB353658D930A925997E9857CE685938FB4C70C9715E503CF1`.
- Normal map SHA-256: `41D16217245C238ECFD0069F45A7746246933D9047CAC66B9520E31D42E486AE`.
- Paired diagnostic XEX/map SHA-256: `1BBFA650BFEC3599A80A9194D1BA9A1B66CBECDCA2AF40828C00AFA6A2B03258` /
  `C948DA3E45E115E4446D5523FDE4229D0D3E6FE3F54D489585C1791183B7D504`.

All seven bounded native checks pass: normal opening (1800 loop/900 movie
frames), teeter (1200 frames), items in Hyrule Temple and Corneria plus Off,
Classic giant and Adventure wireframes (1200 frames each). The stage cases
created 12 items and Off created none. All gameplay cases report zero
decode/history errors, no missing markers and no watchdog; opening reports
zero movie/history errors, no watchdog and no scripted-input marker. Identities
and results are checked in `build-x360/nov05-ecb-filters/validation-summary.json`.
The 19 cases above belong to the first build and are not counted here.

## Build provenance

Both builds record the tree as dirty because the pre-existing tracked
`dist/.gitkeep` deletion was preserved. Source edits for each build were
committed before compiling; upstream was clean and pinned to
`6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`. BootToMatch, scripted input,
call tracing and experimental race are disabled in the normal executables.
Diagnostics are separate scripted builds.

Bounded headless checks do not prove visual/audio parity, completion of a
campaign or performance on physical Xbox 360 hardware. Alpha.3 remains the
previous published package and does not contain these changes.
