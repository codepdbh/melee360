# Development status

- M0 Workspace prepared: complete
- M1 Repositories cloned and pinned: complete
- M2 Toolchain functional: complete (official Free60 Docker image)
- M3 Official example compiled: complete (`xenon-examples/template`)
- M4 Hello World / platform test ELF: complete, awaiting hardware execution
- M5 Framebuffer: compiled using the verified Xenos example sequence; hardware test pending
- M6 Xenos triangle: compiled with shaders from the pinned BSD example; hardware test pending
- M7 Controller: four-port-capable analog/button abstraction compiled; hardware test pending
- M8 Basic audio: 48 kHz PCM test tone compiled; hardware test pending
- M9 Filesystem: FAT/device discovery and read-only ISO header probe compiled; hardware test pending
- M10 ISO located and validated: complete on host; target-side validation awaits hardware media
- M11 First reconstructed Melee module integrated: complete (`melee/lb/lbtime.c`)

The platform test now compiles CPU/endian reporting, aligned memory, a Xenos
framebuffer and test triangle, full analog controller state, a short synthetic
tone, FAT/device discovery, and read-only `GALE01` header validation.

The binary is functional by construction and links against the current
official libxenon toolchain. Physical output still needs verification on an
Xbox 360 booted through XeLL Reloaded.

The Xbox build now compiles `lbtime.c` directly from the pinned `melee-pc`
checkout. A LibXenon adapter supplies the Dolphin OS tick/calendar boundary,
and the platform test executes three original saturation helpers before
reporting `MELEE LBTIME ...... LINKED/OK`.
