# Published alpha.3 validation — 2026-10-23

## Release and source identity

- [Published prerelease](https://github.com/codepdbh/melee360/releases/tag/v0.1.0-alpha.3).
- Published at `2026-10-23T20:00:51Z`; not a draft.
- Tag and build commit: `1f6029621519769a47ce62832da370fcfc34b3b1`.
- Pinned upstream: `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`.
- Normal build recorded clean; BootToMatch, scripted input, call tracing
  and experimental race are all disabled.

Normal XEX SHA-256:
`fbd60ba51f0e9c37e5b518972dcc53be949b7e662a41371e60e8cf3c8c1539e1`.

Normal map SHA-256:
`26c5336c1dcf25691afaa2854a71d49afeb17c442401b72d4187c8c781bf404b`.

## Package

[MELEE360-v0.1.0-alpha.3-xbox360.zip](https://github.com/codepdbh/melee360/releases/download/v0.1.0-alpha.3/MELEE360-v0.1.0-alpha.3-xbox360.zip)
is **3,330,462 bytes**. SHA-256:
`176b6d4e449a2b9dbc2eaf57096a6be5a4e2875b9f0c89625368df6d03ad3e2f`.

It contains exactly six files: `default.xex`, `assets/sprite_atlas.png`,
`INSTRUCCIONES.md`, `AVISOS.txt`, `BUILD.json`, `SHA256SUMS.txt`.
No ISO, extracted disc assets, SDK, maps, traces or diagnostic scripts are
included. The packager reads back and hashes every ZIP entry. The embedded
executable matches the tested normal XEX, and the embedded guide identifies
alpha.3. The separate `SHA256SUMS.txt` release asset hashes the complete ZIP.

Both assets were downloaded from the GitHub draft and compared with their
local counterparts before publication. GitHub then confirmed `draft=false`,
`prerelease=true`, the two asset sizes, and the tag's exact commit above.

## Fresh native checks

All **10 bounded Xenia checks pass** for this build and its paired
diagnostics:

| Check | Cases | Extent |
|---|---:|---|
| Normal opening, live-input executable | 1 | 1800 loop frames, 900 movie frames |
| Original teeter / teeter wait | 1 | 1200 frames, both original motion states |
| Random items on Battlefield and Corneria | 2 | 1200 frames each; 5 and 6 creations |
| Item frequency Off | 1 | 1200 frames; no random creations |
| Classic giant and Adventure wireframes | 2 | 1200 frames each; live encounter entry |
| Original menus into VS, Classic and Adventure | 3 | 3000, 2400 and 2400 frames |

The normal opening has zero movie/history errors, no watchdog and no script
marker. All nine gameplay/menu cases have zero decode/history errors and no
watchdog. Diagnostics are separately built input harnesses, not the XEX
distributed in the ZIP. Their build records point to the normal identities
above; `build-x360/alpha3-validation/validation-summary.json` checks every
result's XEX/map pair, case count and package identity.

Earlier 33-case and 14-case reports used older XEX files. They are preserved
in `STATUS.md` and are not counted as fresh validation of the published ZIP.
The runtime source is unchanged from the edge-fix checkpoint; intervening
commits contain documentation and the reproducible teeter test.

## Limits

These checks do not complete the original ECB/contact pipeline, stage hazards,
Classic bonus stages/bosses, Adventure courses/enemy systems, remaining menus
or presentation. They do not prove full campaign wins, visual parity,
crash-free unrestricted play or physical Xbox 360 compatibility. Installation
and controls are in [INSTALAR_XBOX360.md](INSTALAR_XBOX360.md).
