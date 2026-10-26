# Original swept floor query - 2026-10-26

## Source and build

Runtime commit: `77df96207dc752dc9b6803d6631ba5ab95ffe36f`.
Pinned upstream: `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`.
Normal build is clean; BootToMatch, scripted input, call tracing and the
experimental race are all disabled.

| Artifact | SHA-256 |
|---|---|
| Normal XEX | `B11DC9ADF6279278515729CAF85AC2B6B6500B69FA277D973CD45BC992C756E9` |
| Normal map | `4BF92772394BBC19A7F65F1B7FB08627EA1F91F8D7F797AF3210E1BE05D353BA` |
| Match diagnostic XEX | `B7F3266A9957480DD4FB68339BE8DA09BBAF1175021DCD320AB0FB25D62A8EC0` |
| Match diagnostic map | `194F9C1D01EC69DC8F95B7CAD55DE2EB161FEC22BCD85491C547F01B5042E79B` |

The diagnostic is a separate scripted-input harness built from this normal
build. It is not the normal live-input executable. The published alpha.3
package remains at `1f60296` and does not contain these October 26 changes.

## Runtime changes

The generated `mpCheckFloor` preserves original directional intersection,
nearest-contact selection, callbacks, line/group filters, floor offsets,
flags and normals. Native map-group access replaces the original linked
collision cache, retaining authored group order. Invalid native ranges are
rejected or clipped; the original bounding-cache optimization is omitted.
Original `mpLib_8004ED5C`, `mpLineIntersection`, `mpLineIntersectionH` and
`mpColl_80044838_Floor` are compiled with adapted map access where required.

Native landing now queries the swept motion and calls the original final
floor projection instead of comparing only the destination's floor height.
This rejects a false descending-slope landing from below the floor and
retains a genuine diagonal crossing that the old endpoint test missed.

## Host checks

- `test_floor_query.ps1`: 2,280,960 comparisons with the actual original
  query, including slopes, directions, offsets, ranges, callbacks, nearest
  contacts and output metadata. Both actual native landing regressions pass.
- `test_collision_projection.ps1`: original projection comparisons and
  native ground/edge/teeter integration.
- `test_surface_speed.ps1`: original floor-below queries and native surface
  filtering/speed checks.
- `test_match_rules.ps1` and `test_collision_lifecycle.ps1`: original mode
  predicates, rules and collision initialization remain covered.

## Native checks

All **34 bounded Xenia checks pass** for the normal build and its paired
match diagnostic:

| Check | Cases | Extent |
|---|---:|---|
| Normal opening with live input | 1 | 1800 loop frames, 900 movie frames |
| Original teeter and teeter wait | 1 | 1200 frames, both motion states |
| Random items across all 17 supported VS stages | 17 | 1200 frames each; 88 creations in total |
| Item frequency Off | 1 | 1200 frames; zero creations |
| Seven four-CPU roster batches | 7 | 600 frames each; 26 primary fighters plus Nana |
| Zelda/Sheik transformation cases | 2 | 1800 frames each; repeated transformations |
| Life reset and collision reinitialization | 1 | 1800 frames |
| Classic giant and metal | 2 | 1200 frames each; encounter entry and combat |
| Adventure wireframes and Giga Bowser | 2 | 1200 frames each; encounter entry and combat |

All gameplay cases have zero decode failures, zero audio-history mismatches,
no missing required markers and no watchdog. The normal opening has zero
movie/history errors, no watchdog and no scripted-input marker.

Evidence is under `build-x360/oct26-floor-query`, with one `result.json` per
case. `validation-summary.json` checks all 34 results against the exact
normal or diagnostic XEX/map pair above, verifies the clean build flags,
checks the current normal binary/map hashes, and verifies item creation
counts including Off. The source commit remains the build identity even
when later documentation commits are added to main.

## Limits

This is not the full original contact pipeline. Animated ECB sampling and
substeps, moving-line remapping, runtime collision-joint ordering and other
native ceiling/wall queries still need work. The existing floor-skip
heuristic is also still native. Bounded headless Xenia traces cannot prove
visual parity, full campaign completion or Xbox 360 hardware performance.

The [upstream review](UPSTREAM_AUDIT_20261026.md) records newly available
changes separately; fetching references did not update the compiled base.
