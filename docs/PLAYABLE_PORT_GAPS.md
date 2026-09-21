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
- Geometry is fitted in XY to a diagnostic panel. Original camera projection,
  per-object textures, GX TEV effects, depth and skinning remain incomplete.
- Scene transitions, fighter loading, action states, stage collision, combat,
  match rules and game audio must be integrated and tested together.

## Acceptance criteria

## Gameplay compiler probe

Run `tools/probe_gameplay_xdk.ps1` with XEDK configured. It compiles fighter,
Wait, title and menu-animation translation units independently into the ignored
`build-x360/gameplay-probe` directory. It intentionally fails until all three
compile; it neither links them into the XEX nor replaces the working binary.
The probe-only compatibility header preserves static assertions.

Title and menu-animation units now compile in C after supplying Predicate and
OSCalendarTime declarations. The current fighter C++ probes still fail on C/C++ structure-name differences in
`lb/types.h`, fighter layout assertions involving Pikachu state types, and
other fighter declarations. C compilation also
fails on enum constant initializers. Switching language mode alone is not a
solution; a reviewed source/header adaptation is required before linking.

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
