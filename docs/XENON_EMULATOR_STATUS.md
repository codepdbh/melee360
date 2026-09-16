# Xenon Emulator status

The pinned emulator explicitly describes itself as early-stage and currently
lists XeLL, Linux, and LK as its working programs. Direct libxenon application
compatibility is not claimed, so emulator execution is not yet treated as a
substitute for hardware validation.

| Feature | Emulator | Hardware | Notes |
|---|---|---|---|
| Boot | Not tested | Not tested | ELF32 produced; loader experiment pending |
| Console | Not tested | Not tested | Uses verified libxenon `console_init` API |
| Memory | Not tested | Not tested | Platform test checks 128-byte aligned allocation |
| Video | Not tested | Not tested | Uses `xenos_init(VIDEO_MODE_AUTO)` |
| Xenos | Unknown | Not tested | Emulator README does not promise libxenon compatibility |
| Input | Unknown | Not tested | USB/controller polling is implemented |
| Audio | Pending | Pending | No audio code yet |
| USB | Unknown | Not tested | Initialized by platform test |
| Filesystem | Pending | Pending | Must remain read-only for the ISO |

