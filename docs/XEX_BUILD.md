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
