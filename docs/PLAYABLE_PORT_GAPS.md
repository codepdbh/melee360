# Remaining work for playable Melee

The current XDK executable is a title-resource viewer. It loads original disc
data and runs selected reconstructed HAL modules, but does not run a match.
Successful linking, archive relocation and Present calls are not evidence of
working Melee gameplay.

## Verified current path

`main.cpp` loads `GmTtAll.dat`, constructs JObj trees, advances HAL animation,
decodes display lists and submits a preview to D3D9. The latest guest trace
reports 8,130 vertices and successful Present results at frames 1 and 120.
The mesh hashes differ between those frames, confirming changing mesh data.
The HUD vertex capacity was increased and quad submissions are chunked.

## Missing integration

- `tools/build_xex.ps1` does not compile the original title scene `gmtitle.c`
  or fighter gameplay modules. The current main loop is a viewer loop.
- `src/xdk/hsdjobj_xdk.cpp` still provides placeholder HSD draw callbacks and
  current-camera handling. The preview renderer bypasses these callbacks.
- The preview uses original camera descriptor values and a first-texture binding
  per PObj. Full CObj handling, GX TEV effects, depth and skinning remain incomplete.
- Scene transitions, fighter loading, action states, stage collision, combat,
  match rules and game audio must be integrated and tested together.

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
