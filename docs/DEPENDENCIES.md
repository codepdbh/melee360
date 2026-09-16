# Dependency classification

| Dependency / subsystem | Classification | Xbox 360 plan |
|---|---|---|
| Reconstructed Melee C code | REUSABLE | Compile incrementally with Xenon GCC |
| Aurora GX/GD state and compatibility API | REQUIRES PORT | Preserve interface; separate backend-neutral state from WebGPU rendering |
| libxenon | REUSABLE | Primary platform library |
| newlib in DEVKITXENON | REUSABLE | C runtime for the bare-metal target |
| SDL3 | PC ONLY / REPLACEABLE | Replace window, input, timing, threading uses with platform layer |
| Dawn / WebGPU | PC ONLY / REPLACEABLE | Implement Xenos renderer directly |
| Vulkan / D3D12 / Metal backends | ELIMINABLE | Do not include in Xbox runtime |
| RmlUi and desktop launcher | ELIMINABLE | No launcher in Alpha 0.1 |
| ImGui / desktop overlays | ELIMINABLE initially | Serial/on-screen diagnostic console instead |
| nod / Aurora DVD image handling | REQUIRES PORT | Reuse parsing where portable; supply read-only libxenon file I/O |
| Aurora PAD/SI | REQUIRES PORT | Map Xbox analog data without quantizing sticks/triggers |
| Aurora audio/mixer | REQUIRES PORT | Start with PCM tone and retain `M360_DISABLE_AUDIO` |
| THP/movie support | ELIMINABLE initially | Gate behind `M360_DISABLE_MOVIES` |
| OpenSSL/CURL/updater | PC ONLY | Built-in SHA-1 if needed; no updater/network in first target |

