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
| Video | Not tested | Not tested | Uses `xenos_init(VIDEO_MODE_AUTO)` |
| Xenos | Unknown | Not tested | Emulator README does not promise libxenon compatibility |
| Input | Deliberately disabled in emulator-safe ELF | Not tested | Full hardware implementation remains in the XeLL ELF |
| Audio | Deliberately disabled in emulator-safe ELF | Not tested | Full hardware implementation remains in the XeLL ELF |
| USB | Unknown | Not tested | Initialized by platform test |
| Filesystem | ELF not reached | Not tested | FAT enumeration and ISO header access are read-only |

## Direct ELF execution

A patched emulator build now bypasses SFCX/NAND and ODD/HDD construction only
when its direct ELF loader is active. Null-safe RootBus access fixes the first
guest instruction crash. The emulator loads the big-endian PowerPC ELF, starts
the guest at its entry point and reaches the MELEE360 integration loop.

The emulator-safe ELF intentionally skips direct Xenos, USB and audio startup,
which the research emulator does not model sufficiently yet. The regular XeLL
ELF is unchanged and retains all hardware code.

Verified UART output includes:

```text
CPU ............... OK
POWERPC ........... OK
ENDIAN ............ BIG
MEMORY ............ OK
MELEE LBTIME ...... LINKED/OK
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
