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
- M24 First real title texture: XEX decodes the common tiled GameCube texture formats (including paletted and CMPR), uploads the first `GmTtAll.dat` image to D3D9, and lets LB+RB switch it interactively with the ISO banner while title animation continues. A/Start are no longer diagnostic view shortcuts.
- M25 First real GX mesh: XEX parses each bounded PObj display list using HAL's `n_display << 5` byte length, accepts direct/index8/index16 position, color and TEX0 attributes, triangulates GX triangles/quads/strips/fans, applies the live JObj matrices and submits the result as chunked D3D9 triangle lists; the HUD reports the resulting `TRIS` count
- M26 HSD audio engine (`synth.c`/`devcom.c`): complete; real voice-alloc/priority-stealing, ADSR-style volume-ramp stepping, pitch-ratio and constant-power pan curves, and the DVD/ARAM devcom command queue (enqueue/dequeue/cancel/free-list reuse) linked and host-validated; AX/AI/AR/DVD/OS hardware entry points stubbed (`AXAcquireVoice`/`AXFreeVoice` real priority-steal pool, `AXSetVoice*` record state, `ARAlloc` real bump allocator, `ARQPostRequest`/`DVDReadAsyncPrio` synchronous stand-ins, no audible output yet)
- M27 First audible Melee music: XEX loads `/audio/menu01.hps` (main-menu BGM) from the ISO via the GCM reader, parses HALPST header/block chain, decodes stereo 32 kHz DSP-ADPCM incrementally (`src/common/hps.c`, `dsp_adpcm.c`) and streams 4x4096-frame PCM buffers through an XAudio2 source voice, looping at block 4 (7.168 s) like `synth.c`; Xenia reports all XAudio2 HRESULTs 0, ~real-time `SamplesPlayed`, and a non-zero host peak meter

- M28 Opening movie: `MvOpen.mth` from the user's ISO decodes and presents in Xenia. The latest trace reached frame 634 with zero decode errors while audio samples advanced.
- M29 Title-to-menu flow: Start reaches a temporary menu screen. `mnmain.c` compiles but the original menu scene is not yet active.
- M30 Menu archive load: the XEX relocates `MnMaAll.usd` from the user's ISO in a second HAL archive and resolves all 83 model, camera, light and fog symbols required by `mnMain_Scene_OnEnter`. Xenia reports `menu.archive.symbols: 83` and `menu.archive.ready: 1`. JObj construction and drawing remain pending.
- M31 Menu geometry: the XEX constructs 20 original JObj model trees with 351 joints and binds their animation data. A screenshot exposed that drawing every tree at once superimposed unrelated submenus. The main-scene draw now uses Back, Panel and ConTop only, following `mnMain_Scene_OnEnter`, and submits 2,982 vertices to D3D9. A local opt-in preview reached 120 frames with successful Present calls in Xenia. Cursor clones, `mnmain` input/scene lifecycle, texture animation, TEV, lighting and visual fidelity remain pending.
- M32 Menu navigation bridge: the original background JObj advances every frame. A temporary native overlay shows the five main choices and the selection driven by the original `PAD_ANY_UP`/`PAD_ANY_DOWN` input mapping; A/Start reports that the selected submenu is not ported yet instead of silently doing nothing. This is not the original `mnmain` cursor or a playable submenu. A direct cursor-clone experiment exposed white/duplicated labels without TObj/MatAnim support and was removed from the default build.
- M33 Original menu scene and nested navigation: `mnMain_Scene_OnEnter`/`OnFrame` now run with the shared HSD renderer, original menu assets, text placeholders and native leaf-scene bridges. VS mode returns a quick-match request; unsupported leaf modes report their name and resume the menu. This is not the full set of original submenus.
- M34 Quick match integration: VS mode enters a native match loop, loads `GrNBa.dat` Battlefield geometry/collision and Mario data/model/animation archives, creates two fighter objects, runs selected original common fighter motion states and updates a Melee camera. A local Xenia trace reached 900 match frames with successful rendering and reports two fighters. This verifies loading and loop execution only: that trace had no controller movement or hits, so a playable match is not yet verified.
- M35 Fighter/stage bridge: stage floor and wall collision, spawn/blast bounds, pause/return handling, animation advancement and a first hitbox-to-hurtbox overlap path are connected. Fighter glue still has many explicitly traced unported dependencies; hit reaction/knockback and input-driven state transitions need validation and further original code integration.
- M36 Initial fighter hit reaction: original Mario damage motion entries (75-91) are now included in the extracted motion table. A confirmed hit selects ground/air damage animations, adds a bounded hitlag/hitstun window and returns to Wait/Fall; upward hits leave the floor state. State callbacks and knockback timing are still native scaffolding, not the original damage module, and need an in-Xenia hit test.
- M37 Match controls/rules instrumentation: match HUD/traces now expose P1 buttons, triggers, normalized stick axes, positions and motion transitions. D-pad directions feed the fighter stick path; down drops through platform lines, ceiling crossings stop upward motion, and players can use a connected second XInput controller. Quick matches use four stocks and show a winner screen after a knockout; B returns to the menu. Inputs and actual hits still need a live Xenia session to verify.
- M38 Original common fighter actions: the quick match now links the original shield (Guard, GuardSetOff/Reflect, ShieldBreak, Furafura), dodge (Escape F/B/N, EscapeAir), grab/pummel/throw/captured, ledge (CliffCatch/Wait/Climb/Attack/Escape/Jump), teeter (Ottotto), taunt (AppealS), special fall and rebound states, plus Mario's four special moves from ftmario*.c. Native glue supplies the stage-collision entry points those states call (air/ground/edge/ledge detection, floor endpoints, raycast) over the Battlefield lines. Fireball, cape and other item objects are traced stubs; the shield bubble effect and shield-tilt skeleton blend are not drawn. These units compile and link-check on the host (tools/host_xdk_check); no XDK build or Xenia run has exercised them yet.
- M39 Roster and character select: the quick match links the original fighter units of 24 characters (all but Kirby and the Ice Climbers) and loads each kind's data, animations, costume model, motion-state table and special-move entries from a table generated from ftdata.c. A native select screen precedes the fight (fighter, costume, CPU pick, second-controller join); Classic/Adventure cycle CPU opponents per round. KOs respawn on the original rebirth platform, fighter voices/SFX go through the SSM player, and the CPU shields, grabs and recovers. Projectile and held items of the new characters are traced stubs. Host link check only; not yet built with the XDK or run in Xenia.
- M40 Items, CPU AI and stages: the original item system (core plus all 166 item kinds) and the item-handling fighter states link, so projectiles and held items run original code; the original CPU AI (ftCo_0A01.c with command scripts and attack selection) replaces the native brain over natively built floor islands and line raycasts; Kirby and Popo join the roster (26 fighters). The select phase also chooses among six stages (Battlefield, Final Destination, Dream Land, Yoshi's Story, Fountain of Dreams, Yoshi's Island 64), stocks and CPU level. Host link check only.
- M41 Particles and timed matches: `psDispParticles` is replaced by a native drawer that submits each particle as a textured view-space quad through the D3D HSD renderer, and the select phase adds timed matches (unlimited lives, KO credit from the last attacker, countdown and score HUD). Host link check only.

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
