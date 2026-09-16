# Upstreams

Pinned on 2026-09-16. The directories are independent Git repositories and
are intentionally ignored by the MELEE360 root repository.

| Component | URL | Branch | Commit | License / status | Purpose |
|---|---|---|---|---|---|
| melee-pc | https://github.com/999sian/melee-pc.git | `master` | `6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac` | Mixed: game decompilation has no redistribution license; project additions GPL-3.0; see upstream `LICENSE.md` | Primary port base |
| doldecomp/melee | https://github.com/doldecomp/melee.git | `master` | `6f6e1ade7c8f723d78c3cdb8f2ca96efd6885dc6` | No repository-wide license found | Decompilation reference |
| libxenon | https://github.com/Free60Project/libxenon.git | `master` | `a333adef440f28b436a667be0a4f014afce6349d` | BSD-style libxenon license; toolchain contains separately licensed code | Bare-metal Xbox 360 platform |
| xenon-examples | https://github.com/Free60Project/xenon-examples.git | `master` | `82f13ad013d5a792e417612c55388ff3006fcf04` | README identifies examples as BSD | Verified API/build references |
| XeLL Reloaded | https://github.com/Free60Project/xell-reloaded.git | `master` | `a36ed6b7dae940e47e472a20fd14b0616ddc8cef` | Mixed/individual source notices; no root license file found | Real-hardware loading reference |
| Xenon Emulator | https://github.com/xenon-emu/xenon.git | `main` | `0284bbe6c8125935d97bf54ab3132089c6c65c8b` | GPL-2.0 | Early boot and CPU testing |
| Aurora (vendored by melee-pc) | recorded by `extern/aurora/UPSTREAM_COMMIT` | n/a | See vendored file | MIT | GX compatibility and PC runtime |

The Xenon Emulator checkout has 19 recursively initialized submodules. The
other pinned repositories currently declare no root submodules. Run
`tools/update_upstreams.sh` to fetch and report available changes without
changing any checkout.

