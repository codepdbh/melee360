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

Run `tools/probe_gameplay_xdk.ps1` with XEDK configured. It compiles 19 original
translation units independently into the ignored `build-x360/gameplay-probe`
directory. It fails if any unit fails; it neither links them into the XEX nor
replaces the working binary.
The probe-only compatibility header preserves static assertions.

Title, menu-animation and fighter Wait units now compile in C. The probe
generates header overlays converting MotionFlags constants to enum constants,
preserving their expressions while making them valid C constant expressions.
Original upstream files are not edited. Predicate, OSCalendarTime and
RETURN_IF are supplied by the compatibility header.

All 19 compilation probes now pass, including fighter.c, ftcommon, ftanim,
ftaction, ftcoll, ftparts, ftdata, Wait, Walk, Jump, Fall, Dash, Run, Turn,
Landing, Attack1, AttackAir, title and menu. Indexed castle
offsetof checks are expressed as base offsets plus element/member offsets in
generated overlays; the assertions remain active. Floating-point classification
uses XDK _fpclass, with explicit handling of float subnormals before promotion.
Compilation is not linking or execution of a fighter; its gameplay dependencies
still need to be integrated.

Generated source adaptations hoist declarations for the old C frontend and
explicitly cast native relocated animation-table slots. Upstream sources are
not changed. Compiler warnings still require review before runtime integration.

Run `tools/audit_gameplay_symbols.ps1` after the probe and runtime build to
inventory references absent from their object definitions. The current 19-object
probe reports 1,229 distinct symbols, saved to ignored
`build-x360/gameplay-probe/unresolved-symbols.json` with requesting modules.
This conservative object-level inventory includes SDK/library references and
unreachable functions; it is not a linker test or a count of runtime failures.
It also assumes the existing runtime objects are current. No missing gameplay
functions have been replaced with success-returning stubs.

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
