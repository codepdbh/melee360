# Items, aspect ratio and stage points — 2026-10-20

## Changes

- Restore original `db_EnableItemSpawns` / disable / query functions. The old
  query stub always returned zero, so the original random item timer never ran.
- Compile original `Ground_801C2D24` and `Stage_80224FDC`, with native relocated
  point lookup, original fallback logic and original RNG call bounds.
- Use the original player-count multiplier for item weights, independently of
  Damage Ratio; preserve item mask, frequency Off and special-item rules.
- Render the world into 1280×720 with the authored camera aspect multiplied
  by 4/3. Vertical framing and character proportions are preserved. Original
  HUD/menu/movie composition remains centered at its authored proportions.
- Hide collision for hidden animated models while retaining explicit course
  enable/disable state. Restoring visibility must not restore disabled lines.
- Extract four original startup disable lists instead of maintaining manual
  copies: Castle 12 groups, Kongo 2, Corneria 6, Stadium 6. These lists already
  existed in the port; this is consolidation, not 181 newly repaired segments.
- Load Corneria's missing map 0, which owns its 20 authored item/player,
  camera and blast point remaps. The original stage also creates this model
  from `grCorneria_801DD674`.

## Reproducers and host checks

The initial native item run completed 3600 frames, 81 hits and 16 real random
item creations with zero decode/history errors or watchdogs. This used an
earlier diagnostic image, not the final normal executable.

The subsequent stage regression passed stages 0–12, then correctly failed
Corneria: five random spawn attempts, no item creation. An observational
diagnostic isolated `Stage_80224FDC` returning false because the point table
was empty. The previous native stage catalog did not load map 0, so player
positions and bounds also fell back to generic values. This failure remains
archived under `build-x360/menu-port/oct20-playable/items/stage-13`.

Host match rules cover item enable/disable, weight independence, original RNG
bounds/fallbacks, presentation geometry, hidden collision state and the 26
startup groups. Existing stage binding, collision lifecycle, moving surface,
fighter camera, campaign and flow checks pass. Flow mocks now use seven
Classic combats and twenty normal Adventure phases. The disc point audit
covers all 21 stage archives, explicitly retaining three pending Brinstar
item markers tied to its unported lava skeleton. Audio resolves 4035 streams
in their own banks; original music tests cover 162 authored rows.

## Final native validation

The clean normal build is from `a6ffa110e360a433956b1748c463d8c25713b811`,
pinned upstream `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`, built at
`2026-10-20T13:37:21Z`. BootToMatch, scripted input, call tracing and experimental
race flags are all off.

Normal XEX SHA-256:
`8A45B674E47E7D4A43FB9AF5CAC5F94FFA8DB676F11D2AF5079C1B8DAD79B327`.

Normal map SHA-256:
`1F1BA82715E3FA6D83807092B56F2DF51713CCE232DDCCAAB6F3FD2205026BD7`.

All **40 bounded native checks pass** on diagnostics paired to the normal
build above. Evidence root: `build-x360/menu-port/oct20-playable-final`;
`validation-summary.json` checks every result's XEX/map pair against its
recorded normal build. Diagnostic images must not replace `dist/default.xex`.

| Check | Cases | Extent / result |
|---|---:|---|
| Random items on all supported VS stages | 17 | 1200 frames each; 87 actual random item creations |
| Item frequency Off | 1 | 1200 frames; zero random attempts/creations |
| Classic giant/metal; Adventure wireframes/Giga | 4 | 1200 frames each; encounter entry and live combat |
| Original menus: VS, Stock Rules, Classic, Adventure, results return | 5 | 2400–4500 frames; expected route markers |
| PAD-driven Adventure goals | 2 | Mushroom Kingdom and Brinstar escape reach their next phases |
| Normal opening without scripted input | 1 | 1800 loop frames; 918 movie frames |
| Full selectable roster and collision life boundaries | 10 | Seven four-CPU batches at 600 frames; transformations and life reset at 1800 frames |

All gameplay checks record zero decode failures and audio-history mismatches;
all 40 have no watchdog. The normal opening records zero movie errors and no
script marker. These are bounded checks, not complete campaign playthroughs.

Corneria's corrected 1200-frame reproducer loads 20 authored points, creates
five random items and records zero blast-zone falls. The preceding image
recorded no random items and two falls under the same starting configuration.
This does not establish full original stage behavior or eliminate every fall.

The roster batches cover all 26 selectable primary fighters, varied costumes
and Nana as Popo's follower; each batch actually creates random items
(7 total). Both Zelda/Sheik transformation cases and Fox's life
reset reinitialize the original collision data. These checks do not prove
every move/item interaction or full collision parity.

Diagnostic XEX / map SHA-256 pairs:

- `match-image`: `AD00E9CB359A3A3A9C1235DE056BAD5794FE1F5F290986D25BCE2EA98CFA8999` / `4C3B9F1CD83AE45C348C5E3EF563AC203923C3BAE021C4131EA135EA05172F99`.
- `menu-image`: `00991807BDF22CBC19DBD1AABDE08315EFA3304B581773D4863BC88737C25521` / `E122B99D8BE0275958A96649584041344069DE5FAEF877883BE06BA3E4E0138B`.

## Release distinction

The [published alpha.2](https://github.com/codepdbh/melee360/releases/tag/v0.1.0-alpha.2)
was verified through GitHub's release API on October 20. It contains XEX
`B9D328102D8ED71B13E29CC7BE35AE84A3B58E0D5CFEFC671E67A5CD19EA37F1`,
not the new executable above. Its ZIP is 3,330,297 bytes, SHA-256
`1540c76a370a496071bc46b312eddcbf93439b0f88f34504e8d35db091197c91`.
The previously documented local alpha.2 candidate is also a different build.
The current `dist/default.xex` and this report do not update an existing GitHub
release asset.

## Limits

These changes do not complete original per-frame fighter ECB/contact solving,
including the native fixed-height wall test and origin-based landing search.
The user's report of displaced collisions on all stages in published alpha.2
remains open; these tests do not measure rendered contact alignment.
They also do not complete
stage state machines, hazards or Stadium/Mute City transitions. Classic still
omits bonus courses and bosses. Adventure remains partial, with skipped courses,
missing original enemy/hazard systems and incomplete presentation/endings.
Several menu leaves and original results remain unported. A scripted encounter
entry is not a complete campaign. Visual parity and physical Xbox 360 play are
not verified by the host mathematics and headless Xenia checks.
