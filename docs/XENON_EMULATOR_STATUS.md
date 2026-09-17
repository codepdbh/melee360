# Xenon Emulator status

The pinned emulator explicitly describes itself as early-stage and currently
lists XeLL, Linux, and LK as its working programs. Direct libxenon application
compatibility is not claimed, so emulator execution is not yet treated as a
substitute for hardware validation.

| Feature | Emulator | Hardware | Notes |
|---|---|---|---|
| Boot | Working for emulator-safe ELF | Not tested | Direct ELF loader reaches and executes `main()` without NAND |
| Console | Working through emulated UART | Not tested | Full platform report is visible in the emulator log |
| Memory | Working | Not tested | 128-byte aligned allocation and big-endian check pass |
| Video | Working milestone | Not tested | Guest ELF writes a tiled 1280x720 status screen into emulated scanout RAM |
| Xenos | Software scanout working | Not tested | Direct register initialization remains disabled in the emulator-safe build |
| Input | Deliberately disabled in emulator-safe ELF | Not tested | Full hardware implementation remains in the XeLL ELF |
| Audio | Deliberately disabled in emulator-safe ELF | Not tested | Full hardware implementation remains in the XeLL ELF |
| USB | Unknown | Not tested | Initialized by platform test |
| Filesystem | Deliberately disabled in emulator-safe ELF | Not tested | Full read-only FAT/ISO implementation remains in the XeLL ELF |

## Direct ELF execution

A patched emulator build now bypasses SFCX/NAND and ODD/HDD construction only
when its direct ELF loader is active. Null-safe RootBus access fixes the first
guest instruction crash. The emulator loads the big-endian PowerPC ELF, starts
the guest at its entry point and reaches the MELEE360 integration loop.

The emulator-safe ELF intentionally skips direct Xenos, USB and audio startup,
which the research emulator does not model sufficiently yet. The regular XeLL
ELF is unchanged and retains all hardware code.

For visible validation, the guest writes its own status screen through the
cached `0x9E000000` alias of the physical Xenos scanout surface at
`0x1E000000`. Pixels use the same 32-row tiled layout and big-endian byte order
expected by the emulator's scanout shader. The small panel in the upper-right
is host UI; the MELEE360 screen behind it is produced by PowerPC guest code.

Verified UART output includes:

```text
CPU ............... OK
POWERPC ........... OK
ENDIAN ............ BIG
MEMORY ............ OK
MELEE LBTIME ...... LINKED/OK
FRAMEBUFFER ........ MELEE360 STATUS SCREEN READY
EMULATOR .......... POWERPC CODE RUNNING
PORT STATUS ....... CPU/MEMORY/LBTIME OK
```

Build and run:

```bash
./tools/setup_xenon_emulator.sh
./tools/build_xenon_emulator_elf.sh
./tools/run_xenon_emulator.sh
```

Headless regression: `./tools/test_xenon_emulator.sh 5`.
