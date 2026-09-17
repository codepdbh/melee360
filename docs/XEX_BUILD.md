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

The detected SDK is version `2.0.21256.3`, installed as `Minimum`. It includes
`imagexex.exe` but not the compiler, XTL headers or Xbox import libraries.
Consequently it can inspect/build XEX images only after a suitable PE exists;
it cannot yet compile the MELEE360 XEX target.

Once the complete C/C++ feature set is available, the next target is a small
`default.xex` bootstrap that reproduces the same CPU/memory/status-screen
milestone as `dist/xenon-emulator.elf`. Game subsystems will then be moved
behind the shared platform boundary incrementally.
