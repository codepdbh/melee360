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
real Xbox 360 PowerPC XEX importing `xam.xex` and `xboxkrnl.exe`; Melee's full
scene renderer and gameplay loop are not running yet.

Controller input also passes through the original
`upstream/melee-pc/src/sysdolphin/baselib/controller.c`. The Xbox adapter
implements `PADRead` over XInput, after which the original HSD code performs
stick clamping and normalization plus button trigger, release and repeat state.
The prototype reads `HSD_PadGameStatus`, rather than consuming XInput directly.

Attack contact is evaluated by the original `lb_8000D148` segment-distance
routine from `upstream/melee-pc/src/melee/lb/lb_00CE.c`. The source is included
unchanged through a C-linkage XDK wrapper; only its CRT-conflicting private
`expf`/`powf` symbol names are remapped during compilation.

Run it with a local Xenia Canary build:

```powershell
./tools/run_xenia.ps1 -XeniaPath C:\path\to\xenia_canary.exe
```

The launcher enables Xenia's keyboard-as-controller mode. Default Canary
bindings retained by the bootstrap are:

- `;`: cycle GX mesh / title texture / disc banner (Xbox A)
- `X`: cycle GX mesh / title texture / disc banner (Start)
- `P`: exit (Xbox Y)

An Xbox-compatible controller uses A or Start to cycle the displayed title
resource and Y to exit. When geometry was decoded successfully, the viewer
starts in GX mesh mode and the HUD shows the number of generated `TRIS`.
Pass `-DisableKeyboard` if keyboard emulation should remain disabled.

This is an interactive native title-resource milestone, not a claim that
Melee gameplay has been ported. The first PObj GX display-list translation is
running; the next graphics step is binding each PObj to its own material,
texture and TEV state, followed by the original camera and scene callbacks.
Until that binding exists, GX mesh mode deliberately samples the atlas white
texel so the authentic geometry and material/vertex colors remain visible;
decoded title imagery remains available in the separate texture mode.

Runtime diagnostics append boot completion, decoded vertex count and the
results of frames 1 and 120 to `game:\runtime-trace.txt`. For a local Xenia
diagnostic run, pass `--allow_game_relative_writes=true`; the resulting file
is under the ignored `dist/` directory. Without write access, tracing is skipped.
View IDs are 0 (banner), 1 (texture), and 2 (mesh). A successful `Present`
result verifies submission, not visual correctness.

The corrected draw order submits the opaque backdrop/UI batch before the
title mesh. The mesh is rebuilt from animated JObj transforms each frame in
mesh mode, fitted to the preview panel, without original camera projection or
per-PObj textures. Runtime tracing includes mesh hashes at frames 1 and 120
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
