# Original directional ceiling and wall queries - 2026-10-26

## Implementation

Runtime source commit: `b4e3c274ce5fffa16fc9a448417f7b3bbbfdffba`.
The pinned upstream remains `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`.

The normal build records a clean source tree and has BootToMatch, scripted
input, call tracing and the experimental race disabled.

| Artifact | SHA-256 |
|---|---|
| Normal XEX | `30E7D7AD08EE07469FA8E6C0951736F9C81D66314EED5838B1709C66211C78CB` |
| Normal map | `EC219152C652CC45D6DBA8EC2FD69CD2A6A720A2C1D22918F00EF55777C396F8` |
| Match diagnostic XEX | `592B61AB873200C002C8D8698CCAD000C1658735A8CBD286236C48AA87FC8447` |
| Match diagnostic map | `CA8D9566A239114AD14FE3092979204FBC2833187F9C9D989D744C086B5F92EF` |

The diagnostic is separately built with scripted input and BootToMatch.
Its provenance points to the exact normal build above.

`mpCheckCeiling`, `mpCheckLeftWall` and `mpCheckRightWall` now come from the
original `mplib.c`. This removes the shared symmetric segment intersection
that accepted crossings from either side. The original query bodies retain
direction checks, horizontal/vertical intersection tolerances, nearest-hit
selection, group exclusions, normals, flags and optional output semantics.
The original `mpLineIntersectionV` is compiled alongside its horizontal and
general counterparts.

`generate_surface_queries.py` adapts map access to the native transformed
line endpoints and authored map-group order, as the floor adapter does.
It guards expected original fragments and rejects leftover cache structures.
The bounding cache is omitted. Invalid native static/dynamic ranges are
guarded and oversized ranges clipped to existing line storage.

## Host validation

`tools/test_surface_queries.ps1` compares the generated functions with the
actual original functions extracted independently from the pinned source.
All **5,854,464 comparisons pass**, covering three surface types, horizontal
or vertical surfaces, both slope directions, disabled/empty/wrong-kind lines,
static/dynamic/shared ranges, skip/only filters, nearest selection and all
16 optional-output combinations. Explicit reverse-side crossing regressions
and malformed native dynamic ranges also pass.

The existing surface-speed fixture now uses valid original ceiling/wall
winding and actual surface-group ranges. It previously labeled a horizontal
floor segment as a wall, which the symmetric approximation accepted. The
corrected fixture retains its skip/only assertions.

The floor query (2,280,960 comparisons), surface projections (88,416),
floor-below query (47,840), native ground/edge adapters, match rules and
collision lifecycle suites also pass with the new generator in use.

## Fresh native validation

All **20 bounded Xenia checks pass** for this build and its paired diagnostic.

| Check | Cases | Extent |
|---|---:|---|
| Normal opening with live input | 1 | 1800 loop frames, 901 movie frames |
| Original teeter and teeter wait | 1 | 1200 frames, both motion states |
| Random items: Battlefield, Hyrule Temple, Corneria | 3 | 1200 frames each; 5, 7 and 5 creations |
| Item frequency Off | 1 | 1200 frames; no random creations |
| Seven four-CPU roster batches | 7 | 600 frames each; 26 primary fighters plus Nana |
| Zelda/Sheik transformations | 2 | 1800 frames each; repeated transformations |
| Life reset / collision reinitialization | 1 | 1800 frames |
| Classic giant and metal | 2 | 1200 frames each; entry and combat |
| Adventure wireframes and Giga Bowser | 2 | 1200 frames each; entry and combat |

All gameplay checks have zero decode failures, zero audio-history mismatches,
no missing required markers and no watchdog. The opening has zero
movie/history errors, no watchdog and no scripted-input marker.

Evidence root: `build-x360/oct26-surfaces`. The local
`validation-summary.json` checks every result against its exact XEX/map pair,
requires all 20 cases, checks the normal binary/map hashes and clean build
flags, and confirms 17 random item creations across the selected stages
with none in Off. Earlier test counts are not added to this build's total.

These are headless bounded traces, not visual or auditory parity checks.
Campaign cases do not establish full campaign completion, and execution on
physical Xbox 360 hardware remains unverified.

## Scope and remaining work

This checkpoint replaces low-level query APIs used by original collision
callers, including item code and edge guards. It does not replace the full
fighter solver: `AirWalls` still samples a native fixed midpoint and
`AirCeilings` still has a native endpoint-height loop. Full animated ECB
sampling/substeps, original contact response, moving-line remapping and
runtime joint ordering remain pending. Do not describe this as complete
ceiling/wall collision parity for characters.

The earlier [34-case floor checkpoint](VALIDATION_20261026.md) identifies
a different executable. The published alpha.3 also remains unchanged.
