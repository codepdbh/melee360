# Remaining work for playable Melee

The XDK executable now has a quick-match route from VS mode and a native match
loop. It is still an integration prototype, not a verified playable match.
Archive loading, fighter count and successful `Present` calls do not establish
that the game responds correctly to a player or resolves combat correctly.

## Verified current path

`main.cpp` runs the original menu scene through the shared HSD renderer. VS
selection enters `M360_MatchEnter`, which loads Battlefield (`GrNBa.dat`),
creates stage/camera objects and loads two Mario fighters from the user's ISO.
The match loop advances stage and fighter animation, updates selected original
common motion states, checks stage collision and blast zones, and renders the
scene. A local `dist/runtime-trace.txt` reached match frame 900 with two
fighters and successful frame presentation. In that run P1 remained in Wait
at `(0, 0)`, P2 damage stayed at zero, and the hit count stayed at zero. No
input-driven match or successful hit is claimed by that trace.

The current Xenia runtime trace records the original menu exiting with
`GM_CLASSIC`, selecting mode 3, loading both fighters, and finding/playing
`audio/vl_battle.hps` on arena entry. This only verifies that the menu launches
the shared VS arena. The current five-round HUD/return controls are a scaffold;
the original Classic/Adventure character roster, stage sequence, round setup,
and results progression are not implemented. Do not treat either mode as
playable yet.

The `ft_PlaySFX` bridge resolves each game sound through `audio/smash2.sem`, finds the referenced SSM voice in the ISO, decodes its DSP-ADPCM sample, and submits it to XAudio2. The SEM table resolves 4,016 of its 4,035 streams to SSM entries; a small remainder points outside the available SSM ranges. Background HPS music remains a separate working path.

The 2026-09-23 noise fix corrects three faults: treating the first sample byte
as a DSP frame header, overwriting mono samples while expanding them to stereo,
and writing into a buffer before its previous XAudio2 voice had stopped reading
it. The shared decoder now uses nibble addresses, the voice's initial predictor
and history, and the inclusive AX end address. PCM is decoded into separate
scratch channels; an active voice is destroyed before reusing its output buffer.

Host validation compared 21 mono/stereo SSM entries from `main`, `mario`, `fox`,
`falco`, and `1padv` against vgmstream r2117: all 413,827 compared PCM values
matched. Vgmstream trims 1-3 final samples differently; the port retains the
inclusive AX endpoint. Clips still have a 65,536-frame cap. A host harness
compiled the actual XDK audio source with an XAudio2 test double: 32 submissions
preserved the decoded channels, pan and volume, and 24 active buffer replacements
left the old PCM untouched until voice destruction. The rebuilt HPS test and
nibble-boundary regression checks pass. Local validation scripts/results are in
`build-x360/audio-validation/`. These checks do not establish audible playback
in Xenia or on console; that still needs an in-game listening check.

## Original common actions (2026-09-24)

`tools/build_match_xdk.ps1` now also compiles the original shield, dodge,
grab/throw, captured, ledge, teeter, taunt, special-fall and rebound units and
Mario's special moves (`ftmario.c`, `ftmariospecial{n,s,hi,lw}.c`). Their
motion-table entries are extracted with the others; the Mario table fills the
fighter-specific state list and the `ftData_Special*` dispatch slots.

Native glue additions, all over the loaded Battlefield collision lines:

- air/ground collision variants used by these states, including landing
  checks for a released or thrown victim that only have a `CollData`;
- ledge detection from the fighter's original ledge-snap box (`ftData` x44)
  on ledge-flagged floor ends while falling, feeding `ftCliffCommon_80081298`;
- the Wait/Walk edge stop that raises `Collide_Edge` for teeter;
- floor endpoint/distance/connection queries and a segment raycast.

Still missing for these actions: item objects (fireball, cape, tether
items, item throws out of shield), the shield bubble effect, the stick-driven
shield tilt (second animation skeleton), wall-bound ledge edge cases, moving
ledges and non-Mario character hooks, which are traced as
`fighter.unported.*`. A successful ledge grab traces `fighter.cliff.catch`.

### Roster, select and respawn (2026-09-24)

`fighter_glue_xdk.c` keeps per-kind state (`M360LoadedKind`) described by a
roster table: archive names, costume models, motion-state table, OnLoad/
OnDeath and the eight special entries per kind, taken from `ftdata.c` and each
kind's strings. The roster has Mario, Luigi, Dr. Mario, Peach, Yoshi, Bowser,
Donkey Kong, Captain Falcon, Ganondorf, Fox, Falco, Link, Young Link, Zelda,
Sheik, Samus, Pikachu, Pichu, Jigglypuff (default costume only), Ness, Marth,
Roy, Mewtwo and Mr. Game & Watch. Kirby (copy abilities load other kinds'
data) and the Ice Climbers (Nana's partner AI) are not linked.

`match_scene_xdk.c` starts each VS match with a native select phase: left/right
chooses the fighter, X/Y the costume, A confirms, B steps back or returns to
the menu. A second controller joins by pressing a button; otherwise P1 also
picks the CPU. Classic/Adventure keep P1's pick and cycle the CPU through the
roster. This is scaffolding, not the original character select screen.

After a KO, `M360_FighterRebirth` places the fighter at the camera top above
ground point 4 (offset per player) and runs the original rebirth states with
the PlCo platform accessory. Fighter voice/SFX tracks (`ft_0881.c`) now reach
the SSM player through a native `lbAudioAx_80023870`.

Known gaps for the new characters: DK's floor walk (mpLib_80056C54),
Zelda/Sheik transformation, costume hats and Kirby are unported; projectile
items now link (see Item system).

### Item system (2026-09-24)

The match build now also compiles the original item core (`melee/it/*.c`,
with `itzako.c` adapted for the XDK sin/cos macros) and all 166 item kinds,
so fighter projectiles and held items (fireballs, blasters, arrows, bombs,
turnips, eggs, PK fire/thunder, needles, Din's fire, shadow ball, G&W props,
cape, ...) run their original code. `M360_FighterLoad` loads ItCo through
`Item_80266FA8` and initializes the item allocators (`Item_80266FCC`, again at
each match). Items draw on GX link 6, which the match camera now includes.

Native glue supplies blast-zone/stage-scale queries, camera vectors, item
ground-collision variants, fixed ECB setup, line normals, multi-line checks,
one-shot SFX (`lbAudioAx_800237A8`) and `lbArchive_80017040` over the native
archive loader. Item pickup/throw by fighters, random item spawns, Pokemon,
Kirby copies, debug displays, custom item TEV/material state and moving stage
surfaces remain traced stubs. None of this has run on the XDK or Xenia yet.

### CPU AI and stages (2026-09-24)

The original CPU AI (`ftCo_0A01.c`, `ftcmdscript.c`, `ftcpuattack.c`,
`ft_3C61.c`) drives CPU fighters, initialized as VS CPU kind 4 at the selected
level (LB in the select phase, default 3). Its stage model is native:
`M360_FighterBuildIslands` rebuilds the floor islands from connected floor
lines on each stage build, and the `mpCheck*` raycasts test the matching
native lines. Stage-specific AI hooks (hazards, teams) are stubs.

The select phase also picks the stage (D-pad up/down) and stocks (RB). Nine
stages (the first six plus Hyrule Temple, Kongo Jungle 64 and Jungle Japes;
stage collision holds up to 512 lines) load from the map GObj ids their original OnInit creates; moving
platforms, Randall and hazards are not simulated and collision uses the
authored lines. Kirby (copy-ability hats are stubs) and Popo (without Nana)
complete the roster. Popo now spawns with Nana as the slot's sub entity (a
hidden roster entry); she is always CPU driven (`Player_8003248C` reports the
sub fighter as CPU) and the original AI follows Popo with `cpu.kind` 6. Nana
is put to sleep when she leaves the blast zone or when Popo loses a stock,
and rejoins on Popo's rebirth.

Zelda and Sheik spawn together like the original Player code: the partner is
created as the slot's sub entity and put to sleep (`ftCo_800BFD04`), and
`Player_GetEntityAtIndex`/`Player_SwapTransformedStates` keep a per-slot
entity table so the original down special (`ftCommon_8007EFC8`) swaps them,
handing over position, damage, collision (`mpCopyCollData`) and held items.
The match scene follows the slot's active entity each frame. The transform
flash camera hook (`ftCamera_80076064`) is still a stub.

The right stick (up/down) switches the rule between stock and a timed match
(2, 3, 4, 5 or 8 minutes). Timed matches give unlimited lives, credit a KO to
the last fighter that hit the victim (`dmg.x18c4_source_ply`, cleared on
landing as in the original) and subtract one point per fall; the HUD shows a
countdown and each player's score. A tie at time-out starts sudden death:
the tied players respawn with one stock at 300% and the stock rules pick the
winner (no Bob-omb rain).

### Effects (2026-09-24)

The effect system links from the original sources (`melee/ef/*.c`, with
`efasync.c` adapted for MSVC's pointer OR) together with the HSD particle
generator/particle/appsrt code. Each fight runs `efLib_Init` and loads the
common banks (0 and 0x1F), and each fighter loads its bank from
`ftData_UnkBytePerCharacter` at spawn; the match camera draws links 7 and 8.
`lbArchive_80017040` keeps a per-file cache and reports reloads as preloaded,
like the original lbDvd cache, so banks are not relocated twice. JObj-based
effects render through the HSD path. Particles are drawn by a native
`psDispParticles` (`src/xdk/particle_draw_xdk.c`): each particle becomes a
view-space quad (appsrt matrix or position, rotation, size, primary colour
and texture from its texture group, C4/C8 palettes included) submitted to
`M360_HsdDrawParticle`, which reuses the HSD shaders with vertex colour times
texture and alpha or additive blending. Not reproduced yet: particle forms
other than billboards (lines, points, trails), environment colour/TEV modes
and fog.

### Part animations (2026-09-24)

Hand poses and other sub-skeleton animations (`ftData` x1C, set by the
command scripts and by Fox/Falco/Ganon/Yoshi code) now play:
`ftAnim_ApplyPartAnim` adds the part's AnimJoint to the affected joints and
marks them with `flags_b5`, which keeps them out of the motion-end queries.
A motion change re-applies the motion tree to every joint; `ftAnim_80070F28`
then ends transient part anims and `ftAnim_80070E74` restores persistent ones.
The original animates a shadow skeleton and blends it in over a few frames;
here the pose switches immediately.

### Camera shake (2026-09-24)

`Camera_RequestQuake`/`Camera_StopQuake` are native in the match scene: each
quake kind (loop, small, medium, large) runs for the original frame count
(10 or 22) and offsets the eye and interest by a decaying jitter. The original
drives the offset from a stage quake GObj animation (`grLib_801C9CEC`), so the
exact motion differs.

### Host link check

The XDK is not available in every environment. `tools/host_xdk_check/`
contains a clang-based stand-in for the XDK `cl.exe` (32-bit MSVC mode, stub
CRT headers) so `tools/probe_gameplay_xdk.ps1` and `tools/build_match_xdk.ps1`
can run unchanged under PowerShell on Linux. `check_match.sh` compiles the
match objects and reports referenced symbols that neither they, the other XEX
sources (approximated by `external_defined.py`) nor the recorded baseline
define, plus duplicate definitions. It needs `upstream/melee-pc` at the pinned
commit, `pwsh`, `clang` and `llvm-nm`. Passing it means the units compile and
the link is expected to resolve; it is not an XDK build and runs no code.

## Missing integration

- Reproduce movement, jumping, attacks and hitstun in Xenia with recorded
  controller input. Traces now include normalized axes/buttons and P1 motion
  changes; no live Xenia run has verified that the original callbacks respond
  to those inputs. Attack availability still depends on partially stubbed
  fighter helpers.
- Replace the provisional damage/knockback path with the original damage
  state flow and validate hitlag, DI, tumble, landing and recovery behavior.
  Damage motion IDs 75-91 now resolve through Mario's original motion/animation
  data, but their temporary callbacks and hitstun timing are native scaffolding.
- Verify an actual capsule/hurtbox contact in Xenia and check the new reaction
  trace events (`fighter.hit.reaction_motion`, `fighter.hitstun.frames`); the
  current saved runtime trace predates this change and contains no hit event.
- Match rules (stage, stocks or time, items, CPU level, player count) come from
  the native select phase. The original character/stage select screens and the
  persistent results flow are not integrated. P2 uses a
  connected second controller when available; otherwise a basic CPU that
  approaches, attacks, shields, grabs and recovers is used.
- Stage collision now supports floor/wall checks, downward platform drop-through,
  a basic ceiling crossing stop, ledge grabs and edge teeter. ECB-based
  collision, ledge slips (MissFoot), platform one-way edge cases, moving
  geometry, hazards, items and the original full collision flags remain
  incomplete.
- Quick match reports a winner (or a draw after time-out) and returns to the
  menu with B; connected XInput controllers join as P2-P4. VS setup still
  bypasses the original character/stage select and results persistence.
- Fighter model visibility/parts, material animation, effects, lighting and
  hitboxes need visual and behavioral validation against the original scene.
- Menu leaf screens use bridges for unsupported scenes; they do not implement
  the full original menu lifecycle or data persistence.

## Gameplay compiler probe

Run `tools/probe_gameplay_xdk.ps1` with XEDK configured. It compiles 31 original
translation units independently into the ignored `build-x360/gameplay-probe`
directory, plus a native layout test. It fails if any unit fails. The XEX build
runs this probe and links its layout test, not the fighter gameplay objects.
Original menu input is linked separately as described below.
The probe-only compatibility header preserves static assertions.

Title, menu-animation and fighter Wait units now compile in C. The probe
generates header overlays converting MotionFlags constants to enum constants,
preserving their expressions while making them valid C constant expressions.
Original upstream files are not edited. Predicate, OSCalendarTime and
RETURN_IF are supplied by the compatibility header.

All 31 original compilation probes now pass, including fighter.c, ftcommon, ftanim,
ftaction, ftcoll, ftparts, ftdata, Wait, Walk, Jump, Fall, Dash, Run, Turn,
Landing, Attack1, AttackAir, title and menu, plus ftwalkcommon, ft_081B,
ft_0892, ftchangeparam, mpcoll, mplib, lbcollision, lbvector, gmscene, gm_1A36,
gm_1A3F and mnmain. Indexed castle
offsetof checks are expressed as base offsets plus element/member offsets in
generated overlays; the assertions remain active. Floating-point classification
uses XDK _fpclass, with explicit handling of float subnormals before promotion.
Compilation is not linking or execution of a fighter; its gameplay dependencies
still need to be integrated.

Generated source adaptations hoist declarations for the old C frontend and
explicitly cast native relocated animation-table slots. Upstream sources are
not changed. Compiler warnings still require review before runtime integration.

Run `tools/audit_gameplay_symbols.ps1` after the probe and runtime build to
inventory references absent from their object definitions. It writes the current
unresolved-symbol count and requesting modules to
`build-x360/gameplay-probe/unresolved-symbols.json` with requesting modules.
This conservative object-level inventory includes SDK/library references and
unreachable functions; it is not a linker test or a count of runtime failures.
It also assumes the existing runtime objects are current. No missing gameplay
functions have been replaced with success-returning stubs. The audit now reads
the successful compilation manifest rather than every old object in the folder.
Interrupted or failed probes invalidate that manifest.

## Verified native fighter layout slice

The XDK gave the mixed u8/u16 animation bitfield at Fighter+0x596 separate
allocation units, placing x598 at 0x59C and input at 0x624. A generated header
overlay uses a single u16 allocation unit. Explicit assertions now check x598
at 0x598, input at 0x620, key movement fields and selected disc-record sizes.
They do not depend on the legacy disabled ASSERT_SIZE macro. This is not a
validation of the entire Fighter structure or every bitfield.

`gameplay_layout_probe.c` also checks the integer/bitfield animation-flag aliases
in the XEX. The latest Xenia trace reports `gameplay.layout: 1`, then successful
boot and Present at frame 120. This is a target compatibility test, not running
fighter state transitions or combat. No new playable scene is claimed.

## Original menu input integration

The XEX now initializes and runs the original gm_1A36 controller mapper after
HSD_PadRenewStatus each frame. The generated compilation unit preserves the
source prefix verbatim, omitting only gm_801A3EF4 (initialization of all game
modes). Start detection uses gm_GetButtonsTriggered's combined four-port slot.
Start still logs `input.start.pending_scene`; it does not enter mnmain yet.

The target-side test reports `menu.input.tests: 31` (all five checks passed):
Start-to-confirm mapping, held input without a second trigger, cancellation
from port four, direction mapping and held-direction repeat. It restores the
copied pad state and resets the mapper before processing live input.

Although mnmain and gmscene compile, their full lifecycle is not running. The
object inventory currently finds 113 mnmain references absent from the inspected
objects, including text, graphics, archive/audio and submenu services. Some may
be library-supplied or unreachable; this is not an executable linker test.
The existing OSContext shim is not a real Dolphin thread context and must not
be treated as a working implementation of gmscene's OS/thread services.

The XEX now invokes original `mn_8022ED6C` and `mn_8022F298` for title animation
looping. The build extracts these two function bodies verbatim from mn_22EC.c
into an ignored translation unit, because unrelated functions in that file
require scene and menu services which are not linked yet. This does not mean
the full menu or title scene is running.

## Playability checks

A playable milestone must load an original stage and fighter, advance original
fighter states, respond to movement and attacks, and resolve collision through
the ported game code. Visual verification in Xenia and guest diagnostics must
agree. A title image, moving mesh or custom combat sandbox does not satisfy it.
