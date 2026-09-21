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
- M12 GameCube FST reader: complete; validated against the legal GALE01 image
- M13 Minimal Dolphin DVD API: complete (`DVDOpen`, `DVDFastOpen`, sync/async read)
- M14 HAL archive loading: complete for `GmTtAll.dat` (`HSD_ArchiveParse` and public-symbol lookup)
- M15 HSD heap allocator: complete (`sysdolphin/baselib/memory.c`, `HSD_MemAlloc`/`HSD_Free`)
- M16 HSD class/hash/debug/objalloc/id cluster: complete (`hash.c`, `debug.c`, `class.c`, `object.c`, `objalloc.c`, `id.c`); real `HSD_Assert`/`OSPanic` bridging
- M17 HSD gobj (game object) cluster: complete (`list.c`, `gobjobject.c`, `gobjuserdata.c`, `gobjproc.c`, `gobjplink.c`, `gobjgxlink.c`, `gobjinit.c`, `gobj.c`); cobj/fog/jobj/lobj not ported yet, stubbed as opaque types plus no-op render/teardown callbacks
- M18 HSD math/animation-data foundation: complete (`mtx.c`, `quatlib.c`, `spline.c`, `fobj.c`, `random.c`); Dolphin `PSMTX*`/`PSVEC*` primitives implemented as scalar float math; `util.c` deferred to M19
- M19 HSD animation-object layer: complete (`aobj.c`, `dobj.c`, `robj.c`, plus the GX-free `util.c` and `bytecode.c` they need); real jobj/pobj/mobj/tobj/wobj/fog/cobj/lobj headers compile unmodified against real `GXEnum.h`/`GXStruct.h`; jobj/mobj/pobj functions are counting stubs (`HSD_JObjLoadJoint`/`SetupMatrixSub`/`MakeMatrix`, all `HSD_MObj*`/`HSD_PObj*` entry points dobj calls) plus a stand-in `hsdJObj` class and verbatim `HSD_JObjGetFlags`/`Unref`/`UnrefThis`; on-target self-test lives in `hsdanim_xdk.cpp` because `gobj_xdk_compat.h` short-circuits `jobj.h`
- M20 XDK native disc boot: complete in Xenia Canary (`GALE01`, 1,212-entry FST, `opening.bnr` RGB5A3 decode)
- M21 XDK HAL archive relocation: complete in Xenia Canary (381,781-byte `GmTtAll.dat`, 1,909 relocations, first public root `ScTitle_cam_int1_camanim`)
- M22 Title scene graph load: XEX resolves the 12 symbols requested by `gmtitle.c`, constructs both real JObj trees, binds their animation graphs and reports joint/DObj counts on screen
- M23 Title render-data graph: DObj now retains runtime MObj/PObj/TObj objects, material and PE data, GX attribute/display-list pointers, texture image descriptors, palettes and LOD/TEV metadata; XEX reports real material, polygon and texture counts

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

The portable GCM reader mounts the filesystem table directly from a raw ISO,
resolves nested paths, bounds-checks names and reads file extents. Its host
test validated 1,212 entries and the `BNR1` header of `opening.bnr`; the same
implementation is linked into the Xbox 360 ELF.

The first Dolphin DVD compatibility slice now runs on the same GCM reader.
Host validation opens `/opening.bnr` as entry 537 through
`DVDConvertPathToEntrynum`/`DVDFastOpen`, reads it with `DVDReadPrio`, and
checks its `BNR1` signature. These symbols are present in the PowerPC link map.

The Xbox build now compiles the upstream HAL `archive.c` implementation through
a Xenon compatibility wrapper. On boot it reads `GmTtAll.dat` from the legal
ISO, applies all 1,909 pointer relocations and resolves its first public root.
Host validation independently checks the 381,781-byte archive layout, 13
public roots and first symbol (`ScTitle_cam_int1_camanim`).

The same original parser is now also compiled into `default.xex`. Xenia Canary
has visibly confirmed `GMTTALL.DAT RELOCATED`, `MELEE MODULES LINKED`, and a
resolved public title root while displaying the banner decoded directly from
the user's ISO. No ISO or extracted Nintendo asset is copied into the repo.

The Xbox build now compiles the upstream `sysdolphin/baselib/memory.c` HSD
allocator directly, backed by a real first-fit, coalescing free-list heap
over a static 24 MiB arena supplied through `HSD_GetHeap`/`OSAllocFromHeap`/
`OSFreeToHeap`. Host validation exercises alignment, read/write of allocated
memory, mixed free/reallocate patterns and a 4,000-iteration burn-in; the
same original `HSD_MemAlloc`/`HSD_Free` now back the boot self-test that
reports `MELEE MODULES ...... LINKED/OK`.
