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

The Corneria correction is undergoing a new normal build and native regression.
The final executable hashes and bounded results will be recorded here after
that run; the preceding failed image must not be treated as its proof.

## Limits

These changes do not complete original per-frame fighter ECB/contact solving,
stage state machines, hazards or Stadium/Mute City transitions. Classic still
omits bonus courses and bosses. Adventure remains partial, with skipped courses,
missing original enemy/hazard systems and incomplete presentation/endings.
Several menu leaves and original results remain unported. A scripted encounter
entry is not a complete campaign. Visual parity and physical Xbox 360 play are
not verified by the host mathematics and headless Xenia checks.
