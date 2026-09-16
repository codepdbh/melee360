# Xenon Emulator status

The pinned emulator explicitly describes itself as early-stage and currently
lists XeLL, Linux, and LK as its working programs. Direct libxenon application
compatibility is not claimed, so emulator execution is not yet treated as a
substitute for hardware validation.

| Feature | Emulator | Hardware | Notes |
|---|---|---|---|
| Boot | Blocked in emulator initialization | Not tested | Direct ELF loader selected, but emulator crashes while constructing SFCX without a NAND image |
| Console | Not tested | Not tested | Uses verified libxenon `console_init` API |
| Memory | ELF not reached | Not tested | Platform test checks 128-byte aligned allocation |
| Video | Not tested | Not tested | Uses `xenos_init(VIDEO_MODE_AUTO)` |
| Xenos | Unknown | Not tested | Emulator README does not promise libxenon compatibility |
| Input | ELF not reached | Not tested | Full buttons, sticks, and triggers are implemented |
| Audio | ELF not reached | Not tested | 48 kHz stereo PCM test tone is implemented |
| USB | Unknown | Not tested | Initialized by platform test |
| Filesystem | ELF not reached | Not tested | FAT enumeration and ISO header access are read-only |

## Direct ELF experiment

A headless emulator build (`GFX_ENABLED=OFF`) completed successfully in WSL.
The generated configuration was changed only to enable `ElfLoader`, point
`ElfBinary` at `melee360-test.elf32`, enable the emulator's documented
`Simulate1BL` mode, and select printable UART output. The process exits with a
segmentation fault before logging `ELF32 Header found`: current construction
still attempts to initialize SFCX from a NAND image, and the absent NAND path
leaves that path unusable. MELEE360 will not fabricate or redistribute NAND
data to bypass this.

Reproduction: `./tools/test_xenon_emulator.sh 10`.
