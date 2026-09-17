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

This produces `dist/default.xex`. The initial bootstrap renders a MELEE360
status screen, an animated progress bar and controller-exit handling. It is a
real Xbox 360 PowerPC XEX importing `xam.xex` and `xboxkrnl.exe`; it is not the
full game yet.

Run it with a local Xenia Canary build:

```powershell
./tools/run_xenia.ps1 -XeniaPath C:\path\to\xenia_canary.exe
```

Game subsystems will be moved behind the shared platform boundary
incrementally after this bootstrap is validated in Xenia.
