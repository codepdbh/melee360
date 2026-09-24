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
- Match setup is fixed to two Mario fighters on Battlefield with four stocks.
  VS rules, fighter selection, stage selection, configurable stocks/time and
  persistent results flow are not integrated. P2 uses a connected second
  controller when available; otherwise a basic approach-and-attack CPU is used.
- Stage collision now supports floor/wall checks, downward platform drop-through,
  a basic ceiling crossing stop, ledge grabs and edge teeter. ECB-based
  collision, ledge slips (MissFoot), platform one-way edge cases, moving
  geometry, hazards, items and the original full collision flags remain
  incomplete.
- Quick match now uses four stocks, reports a winner and returns to the menu
  with B; a connected second XInput controller can control P2. The stock count
  is fixed, there is no match timer, and VS setup still bypasses character/stage
  select and results persistence.
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
