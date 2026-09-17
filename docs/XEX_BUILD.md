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

This produces `dist/default.xex`. The current interactive bootstrap renders a
small D3D9 arena with player movement, jumping, gravity, a training dummy,
damage, attacking and reset handling. The damage path directly compiles and
links the original `upstream/melee-pc/src/melee/lb/lbtime.c` source through a
small XDK time compatibility layer. It is a real Xbox 360 PowerPC XEX importing
`xam.xex` and `xboxkrnl.exe`; it is not the full game yet.

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
bindings for the prototype are:

- `A` / `D`: move
- `;`: jump (Xbox A)
- `L`: attack (Xbox X)
- `X`: reset (Start)
- `P`: exit (Xbox Y)

An Xbox-compatible controller uses the left stick or D-pad, A, X, Start and Y.
Pass `-DisableKeyboard` if keyboard emulation should remain disabled.

This arena is a native input/physics/rendering integration milestone, not a
claim that Melee gameplay has been ported. Game subsystems will continue to
move behind the shared platform boundary incrementally.

## Graphics pipeline

`tools/build_xex.ps1` compiles the HLSL sources in `src/xdk/shaders` with the
Xbox 360 shader compiler. The generated microcode headers are placed under
`build-x360/xdk` and embedded in the XEX; they are not committed.

`src/xdk/sprite_renderer.cpp` owns the D3D9 vertex shader, pixel shader and
vertex declaration. It batches all UI glyphs, arena geometry, gradients and
character primitives into one `D3DPT_QUADLIST` submission per frame with alpha
blending. Only the initial back-buffer clear remains a D3D clear operation.

The pixel shader now samples a `D3DFMT_LIN_A8R8G8B8` sprite atlas. For this
repository milestone, the atlas is generated at runtime and contains original
placeholder fighter and training-dummy artwork plus a white texel used for
solid-color geometry. This validates UV coordinates, point sampling and alpha
transparency without distributing copyrighted game artwork.

The build also generates `dist/assets/sprite_atlas.png`. At runtime the XEX
loads it from `game:\assets\sprite_atlas.png` through D3DX9 and reports
`EXTERNAL ATLAS: OK` in the HUD. If the file is absent, rendering continues
with the equivalent in-memory fallback atlas.
