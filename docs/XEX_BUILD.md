# XEX build path

MELEE360 has two distinct executable targets:

- `xenon.elf` is the current LibXenon build and runs through XeLL on an
  RGH/JTAG console.
- `default.xex` will be the XDK build used for Xenia and development-kit style
  loaders.

An ELF cannot simply be renamed or passed directly to `imagexex`. The XEX
image builder requires an Xbox 360 PowerPC PE produced with the matching C/C++
toolchain, headers and import libraries.

Check a local installation from PowerShell:

```powershell
./tools/check_xdk.ps1
```

A complete result needs all four components reported as `OK`:

- `imagexex.exe`
- the Xbox 360 PowerPC C/C++ compiler
- XTL headers such as `xtl.h`
- the Xbox kernel import library such as `xboxkrnl.lib`

The repository never vendors SDK binaries, headers, keys or libraries. They
remain local and must come from an installation the developer is authorized
to use.

## Current machine

The detected SDK is version `2.0.21256.3`, installed as `Full`. The PowerPC
compiler, XTL headers, Xbox import libraries, linker and `imagexex.exe` all pass
the local toolchain check.

Build the native D3D9 bootstrap from PowerShell:

```powershell
./tools/build_xex.ps1
```

This produces `dist/default.xex`. When exactly one legal ISO exists under
`iso/`, the build creates an ignored hardlink at `dist/melee.iso` so Xenia can
expose it as `game:\melee.iso` without duplicating the 1.4 GB image. The XEX
mounts the GameCube FST, decodes `opening.bnr`, reads `GmTtAll.dat`, runs the
original `HSD_ArchiveParse`, applies its relocation table and resolves the
first public title root. It then constructs the title JObj/MObj/PObj/TObj
graph, decodes its first tiled GameCube texture and translates the title PObj
GX display lists into a D3D9 triangle list. It is a
real Xbox 360 PowerPC XEX importing `xam.xex` and `xboxkrnl.exe`. The original
menu scene and a fixed two-Mario Battlefield quick-match loop are now linked;
the latter is still an integration prototype and has not passed interactive
movement/combat validation. See [remaining gameplay gaps](PLAYABLE_PORT_GAPS.md).

Controller input also passes through the original
`upstream/melee-pc/src/sysdolphin/baselib/controller.c`. The Xbox adapter
implements `PADRead` over XInput, after which the original HSD code performs
stick clamping and normalization plus button trigger, release and repeat state.
The prototype reads `HSD_PadGameStatus`, rather than consuming XInput directly.

The prototype combat sandbox uses the original `lb_8000D148` segment-distance
routine from `upstream/melee-pc/src/melee/lb/lb_00CE.c`. The quick-match
fighter bridge currently checks relocated hit capsules against transformed
hurtbox segments and applies provisional knockback; full original damage-state
behavior remains pending. The original helper is included unchanged through a
C-linkage XDK wrapper; only its CRT-conflicting private `expf`/`powf` symbol
names are remapped during compilation.

Run it with a local Xenia Canary build:

```powershell
./tools/run_xenia.ps1 -XeniaPath C:\path\to\xenia_canary.exe
```

The launcher enables Xenia's keyboard-as-controller mode and applies these
bindings:

- `1` + `3`: cycle GX mesh / title texture / disc banner (LB+RB together)
- `WASD`: move
- `J/K/L/I`: GameCube A/B/X/Y
- `Enter`: Start/confirm
- `1/3`: LB shield / RB grab (GameCube Z; together they also cycle title diagnostics)
- `Q/E`: LT/RT shield
- `Arrow keys`: C-stick; `Ctrl` + `WASD`: D-pad

An Xbox-compatible controller maps A/B/X/Y by matching face-button labels,
Start to confirm, LB/LT to GameCube L, RT to GameCube R, and RB to GameCube Z
(grab). X/Y jump during combat. LB+RB title-resource diagnostics are retained in the title viewer.
Pass `-DisableKeyboard` if keyboard emulation should remain disabled.

This interactive executable now runs the original menu scene and offers a VS
quick-match path, but gameplay remains incomplete. Use the title-resource
viewer notes below when inspecting the diagnostic title view.

This is an interactive native title-resource milestone, not a claim that
Melee gameplay has been ported. The first PObj GX display-list translation is
running; the next graphics step is binding each PObj to its own material,
texture and TEV state, followed by the original camera and scene callbacks.
GX mesh mode binds the first TObj of each material and samples its original
UVs. A 64-entry cache uploads decoded images once; unavailable textures fall
back to the atlas white texel. Multiple texture stages, TEV, animated texture
selection and texture matrices remain unimplemented.

Runtime diagnostics append boot completion, decoded vertex count and the
results of frames 1 and 120 to `game:\runtime-trace.txt`. The
`tools/run_xenia.ps1` launcher enables `--allow_game_relative_writes=true` so
Xenia can write the trace under ignored `dist/`. Without write access, tracing
is skipped.
View IDs are 0 (banner), 1 (texture), and 2 (mesh). A successful `Present`
result verifies submission, not visual correctness.

The corrected draw order submits the opaque backdrop/UI batch before the
title mesh. The mesh is rebuilt from animated JObj transforms each frame in
mesh mode. Perspective camera descriptors now supply eye, target, field of
view, aspect and depth limits for the preview; unsupported descriptors retain
the XY fit fallback. Camera animation remains pending; per-PObj texture binding
currently covers the first texture only.
Near/far clipping currently rejects crossing triangles instead of splitting
them, and the preview still lacks depth buffering and perspective-correct UVs.
Runtime tracing includes mesh hashes at frames 1 and 120
to check that animation reaches submitted vertex data.

## Graphics pipeline

`tools/build_xex.ps1` compiles the HLSL sources in `src/xdk/shaders` with the
Xbox 360 shader compiler. The generated microcode headers are placed under
`build-x360/xdk` and embedded in the XEX; they are not committed.

`src/xdk/sprite_renderer.cpp` owns the D3D9 vertex shader, pixel shader and
vertex declaration. It batches the UI into `D3DPT_QUADLIST` and submits the
translated Melee title geometry separately as `D3DPT_TRIANGLELIST`, with alpha
blending. The translator handles GX triangles, quads, triangle strips and fans
with direct, 8-bit-indexed or 16-bit-indexed vertex attributes. Display-list
parsing is bounded by HAL's 32-byte-block `n_display` length and validates the
complete vertex payload before decoding a primitive. Only the
initial back-buffer clear remains a D3D clear operation.

The pixel shader now samples a `D3DFMT_LIN_A8R8G8B8` sprite atlas. For this
repository milestone, the atlas is generated at runtime and contains original
placeholder fighter and training-dummy artwork plus a white texel used for
solid-color geometry. This validates UV coordinates, point sampling and alpha
transparency without distributing copyrighted game artwork.

The build also generates `dist/assets/sprite_atlas.png`. At runtime the XEX
loads it from `game:\assets\sprite_atlas.png` through D3DX9 and reports
`EXTERNAL ATLAS: OK` in the HUD. If the file is absent, rendering continues
with the equivalent in-memory fallback atlas.

The atlas currently contains separate original frames for idle, movement,
airborne and attack states. The render loop selects a frame from gameplay
state, flips UV coordinates for facing direction and emits short-lived alpha
blended particles when the reconstructed segment hit test connects.
