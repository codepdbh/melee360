# Porting analysis

## Current architecture

`melee-pc` is the correct primary base. Its root CMake build compiles
`src/melee` and `src/sysdolphin` as C into `melee_game`, adds
`src/pc/vtxarray.c`, and links that library to Aurora's GX, GD, VI, PAD, SI,
MTX, OS, DVD, CARD, and THP compatibility libraries. `src/pc` supplies the
desktop executable, launcher, disc opening, updater, compatibility shims, and
other host integration.

| Area | Role | Xbox 360 decision |
|---|---|---|
| `src/melee` | Reconstructed game logic | Reuse incrementally |
| `src/sysdolphin` | GameCube/Dolphin SDK-facing game support | Reuse where Aurora supplies semantics |
| `src/sdk_include` | SDK-compatible declarations | Reuse, audit ABI/endian assumptions |
| `src/pc` | Desktop launcher/runtime and host shims | Split; retain only portable compatibility pieces |
| `src/Runtime` | Runtime support | Audit per source before reuse |
| `src/thp` | Original PPC THP path | Initially disable movies; revisit later |
| `extern/aurora/include` and GX/GD state logic | Public compatibility boundary | Preserve as the preferred Melee-to-renderer boundary |
| `extern/aurora/lib/gx`, `gd`, FIFO/state parsing | GX implementation | Reuse state interpretation where it is backend-neutral |
| `extern/aurora/lib/gfx`, `webgpu`, `dawn` | WebGPU renderer and presentation | Replace with a Xenos backend |
| Aurora SDL window/device/main loop | Desktop/mobile platform | Replace with libxenon video/input/timing |
| RmlUi/ImGui launcher | Desktop UI | Eliminate from Xbox runtime |

## Important constraints found

- The game build currently defines `TARGET_PC=1` and `MELEE_PC=1`; these
  assumptions cannot simply be copied to Xbox 360.
- Upstream deliberately uses GCC's `scalar_storage_order("big-endian")` for
  disc structures. Xbox 360 is already big-endian, so every PC byte-order
  shim must be audited rather than retained blindly.
- Upstream uses C11 for game code and C++20 for Aurora. The official current
  Xenon toolchain is GCC 16.2.0, so language support is adequate; library and
  memory constraints remain to be measured.
- Aurora's public GX API is separable in concept, but its current graphics
  implementation is strongly coupled to WebGPU types. A Xenos backend is a
  real renderer port, not a build-system switch.

## Integration boundary

MELEE360 keeps its platform API in `src/common/platform.h`, with the libxenon
implementation in `src/xbox360`. Game code must not call libxenon directly.
The next integration step is to extend this boundary for read-only files,
video presentation, analog input, audio submission, and logging, then compile
a small selected subset of `melee_game` without the desktop executable.

