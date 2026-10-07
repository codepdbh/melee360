# Development status

## Original fighter camera and model scaling — 2026-10-07

The build compiles original `ftcamera.c` directly. The original fighter camera
callback runs at GObj priority 0x12, with its dormant guard and common-update
ordering. Camera subjects initialize on spawn and encounter size changes;
stage zoom/center queries use the native stage's original archive values.
Original `Fighter_UpdateModelScale` replaces its stub. Encounter size updates
the original x/y fields while z retains its independent flat-mode width
meaning. The native animation scale adapter calls original
`ftCommon_GetModelScale`. The global camera manager, easing and match framing
still need further porting.

Host tests check the actual original functions, both facings, normal/tiny/giant
sizes, camera-bone updates, the offset-aware death ratio, camera callback
guards/order and the separate width override. `fighter-camera-native-20261007`
passes 1800-frame Link/Kirby (27 hits), Young Link/Ice Climbers (33 hits), and
small-DK encounter (24 hits, two falls) tests. The Very Hard Bowser test clears
Bowser at frame 1430, loads Giga, samples Giga at frame 517 and then returns
through game-over/menu; verdict SHORT because the target was 1800. No
watchdog alert. It does not prove a Giga clear or a sustained 1800-frame Giga
fight. The dedicated Easy final pilot passes in 62 seconds, with 30 hits,
mode-4 completion and return to menu (`fighter-camera-bowser-final-20261007`).
Diagnostic SHA-256:
`7B1F9AA28074C17DC98373B66BE843409D954256C08E7492E2568BC0DF1145DC`.
Current normal twenty-phase XEX SHA-256:
`516B2F8DC625ED7F9C688C5DFFECCB93361D2144E546A1D14D6C8266A78E7538`.
The 903-second campaign continuation below uses the preceding material build,
before these camera/scale changes.
A fresh normal boot reached loop frame 10,200 and presented 1,922 movie
frames with zero movie errors, zero ADPCM history mismatches and no watchdog
alert (`normal-fighter-camera-runtime-trace-20261007.txt`). This checks opening
playback only; visual comparison and physical console verification remain pending.

## Original surface materials — 2026-10-07

The native build now extracts the original 71 stage material tables, twenty
entries each, from `mplib.c`. Original friction queries, the Ice Climbers
friction exception, footstep/landing sound and effect queries, and all three
fighter wrappers replace constant returns and unported stubs. Only stage
ownership is adapted through `M360_MatchGroundKind`; query bodies and values
come from the original source. Optional diagnostic tracing is excluded from
the normal build. The complete original collision engine remains unfinished.

`test_surface_materials.ps1` passes all 1420 stage/material combinations,
low-byte indexing, sentinel/default behavior, airborne/missing-floor guards,
four bounce sound entries and the Ice Climbers exception. An ISO audit of
all 21 native stage archives finds only material indices 0–19:
`surface-materials-iso-audit-20261007.json`.

`surface-materials-native-20261007` passes two Xenia tests at frame 1800:
Mario vs Zelda on Battlefield (24 hits, one fall), and four CPUs/items on
Fountain of Dreams (77 hits). No watchdog alert; audio is enabled. Fountain
trace identifies GrKind 12/material 10: original step sound 335/effect 30007,
landing sound 526/effect 30008. These are selection/submission checks,
not listening or visual parity tests. Both runs retain one SFX miss.
Diagnostic SHA-256:
`A5C28D98A46EDF29208CC743ABD4DB54D754B3DAFB1C3E5A191249FB77C055CE`.
The preceding normal twenty-phase material XEX has SHA-256
`0A0C6FD5D7050F69502A1D22F6744CE53252C37E302E15DC04BCCB7FF98B9104`.
A fresh normal-boot smoke run reached loop frame 2700 and presented 1448
opening frames, with zero movie errors/history mismatches and no watchdog:
`normal-surface-materials-runtime-trace-20261007.txt`. No menu interaction or
physical Xbox test was performed. The campaign follow-up
`adventure-materials-campaign-20261007` completed with verdict OK in 903
seconds, three Continues, maze goal 4190, escape 2291, race 3974 and final
mode-4 completion/return to menu. It starts at the maze, uses original seed
12000 and Very Easy, and excludes optional Giant Kirby/Giga. The first four
phases, other maze variants and a physical console remain unverified by
this run. Diagnostic SHA-256 is the A5C28D98 build above.

## Original moving-surface queries — 2026-10-07

Removed the `mpGetSpeed` placeholder and constant-zero collision surface
queries. The native endpoint adapter now runs the original `mpRemap2d` math,
with the original four `mpCollGetSpeed*` wrappers and ECB query point.
Inactive collision lines are rejected, matching the original enabled/hidden
line check. Host tests compare against the actual upstream helper for slopes,
vertical/reversed lines, endpoint clamping and the original degenerate branch.
Static surfaces return success with zero speed; missing/disabled lines leave
the output unchanged, as in the original. This is not the complete original
collision engine or a port of unimplemented stage hazards.

The XDK build links with the replacement stub removed. Xenia's four-CPU,
items-enabled Fountain of Dreams test reached frame 1800 with 99 hits and no
watchdog alert (`surface-speed-native-20261007`). This is integration evidence,
not a native measurement of every moving surface. The preceding normal
twenty-phase `dist/default.xex` has SHA-256
`98899CEF25563ED34BD28EEE904EABE4FBE0EE002453748BFFFFDB75B6556682`.
Its fresh normal-boot trace reached loop frame 3000 and presented 1696 opening
frames, with zero movie errors, zero ADPCM history mismatches and no hang:
`normal-surface-speed-runtime-trace-20261007.txt`. This does not verify menu
interaction, visual/audio fidelity or physical Xbox behavior.

## Seeded Adventure continuation — 2026-10-07

`maze-connected-floor-campaign-20261006` completed in Xenia: verdict OK,
721 seconds, original seed 12000, maze goal at frame 3502, escape at 2291,
race at 3974, final mode-4 completion and return to menu. The run includes
two Continues and no watchdog alert. It starts at the maze, omits the first
four phases, uses Very Easy and does not exercise optional Giant Kirby or
Giga Bowser. It therefore does not prove a complete twenty-one-phase campaign
from the beginning, all randomized maze rooms, or resolution of every earlier
render stall. Diagnostic SHA-256:
`4B847DE445051706B4248C7B5A3CB1617D277E62C95F11B3AA8C8E4259BDE2CB`.
Normal live-input `dist/default.xex` remains twenty phases;
the diagnostic's race and bot are excluded from that executable.

## Underground Maze source progress — 2026-10-06

The current playable XEX includes a twentieth Adventure phase:
Underground Maze, using GrNSr.dat, six symbols, a randomized Triforce room,
Link encounters, room collision switching and a seven-minute timer. Original
traversal segments 51, 79, 101, 102, 115, 116 and 131 are disabled outside
combat, and trigger half extents match Ground_801C3DB4. Symbols hide during
combat and cleared rooms remain hidden. Host match and flow rules pass.
The Xenia pilot defeated Link in rooms 5, 4 and 3, timed out while navigating
to room 2, then reached the Triforce on retry (445 seconds, 50 hits, four falls;
`build-x360/adventure-maze-drop-pulse-20261006`). This establishes a maze clear
with retry, not traversal of every room or a complete twenty-phase campaign.
Original maze hazards and transition animations are still missing.

The preceding normal-boot/live-input twenty-phase maze build has SHA-256
`C7272C2962895FEBC3A02F6A2ECAB7CA28F0FAFF42A249F9ADCC9984A7C9727C`.
This build also restores the original grab wall-occlusion query. Host tests
verify ECB centers and both wall directions. A normal-boot Xenia smoke run
presented 1,194 opening frames with no movie errors or watchdog hang
(`build-x360/normal-maze-collision-runtime-trace-20261006.txt`); audio counters are
submission evidence, not a listening test. This does not verify menu input.
The preceding nineteen-phase E6F9B2A7 build is retained in
`build-x360/playable-reserved-parts-20261006.xex`. Existing continuous
seventeen-phase Easy results below refer to earlier builds without this course.

The earlier twenty-phase diagnostic build also passed the following Zelda
encounter (`adventure-maze-followup-smoke-20261006`): 3,900 sampled frames,
41 hits and two falls. The continuous Easy run in
`build-x360/adventure-full-maze-20261006` cleared the maze after a retry but
reported a render watchdog stall on entering scene 18 (Zelda), at loop frame
42,779. It did not complete the campaign. The isolated scene 25 smoke test
in `zelda-post-maze-isolation-20261006` actually tests Samus, despite its
directory name; it does not validate the frozen transition. The maze-to-Zelda
transition is under investigation.

A second run starting at the maze (`maze-transition-native-20261006`, isolated
21-phase diagnostic) reported a render watchdog at frame 11,413 before clearing
the maze. The counter subsequently advanced to 11,417, so the alert alone does
not establish a permanent lock. The harness still classifies this as FREEZE;
the instrumented follow-up observes recovery without weakening that verdict.
The instrumented run `maze-render-detail-20261006` subsequently reached 18,900
sampled maze frames without a watchdog, but did not reach the Triforce before
its time budget: verdict SHORT. This does not reproduce or resolve the earlier
stalls. The diagnostic pilot now ignores roofs above the destination's head,
with a regression based on the maze's upper corridor; native validation follows.

Source review found that exploration incorrectly re-enabled all six arena
collision groups after a fight. The original `grShrineRoute` restores only
traversal joints 0-7 and preserves the selected arena's state. `MazeRoomBounds`
now follows that rule; host tests check that other arenas stay disabled after
successive room clears. `maze-navigation-transition-20261006` was intentionally
stopped after identifying this defect (its harness CRASH is process termination,
not an observed game crash). The corrected native test
`maze-traversal-transition-20261006` reached and cleared room 2 without a
watchdog, then was intentionally stopped to improve diagnostic navigation
below the upper ledge (harness CRASH again denotes owned process termination).
The pilot now retains its ceiling detour until close to the destination height;
the continuing test is `maze-clearance-transition-20261006`.
That test was intentionally stopped when traces showed the diagnostic stick
falling into its dead zone eight units short of the overhang edge. Steering now
keeps a minimum magnitude until within two units, with tests in both directions.
The native continuation is `maze-steering-transition-20261006`; the three
superseded pilot XEX/EXE pairs were removed, retaining maps and traces.
The subsequent ramp pilot ran for 929 seconds without a watchdog but did not
reach the Triforce: `maze-ramp-transition-20261006`, SHORT. Original collision
restoration is now included in the normal twenty-phase XEX above. Diagnostic
navigation uses the entrance's 35-unit half extent with a five-unit margin;
the Very Easy test `maze-margin-very-easy-20261006` finished SHORT, with
25,080 sampled frames, 48 hits, 36 falls and no recorded watchdog. It did not
clear the maze. The reproducible original-RNG diagnostic
`maze-seeded-campaign-20261006` applied seed 12000, selected room 3, cleared
the maze at frame 3502 and entered Zelda. Zelda advanced to 9900 sampled
frames without a watchdog, but the diagnostic pilot stayed on a side ledge.
The run was intentionally stopped; its CRASH verdict is owned process
termination, not an observed game crash. The pilot now chooses connected
floor spans rather than isolated segments; host tests cover reversed endpoints,
disconnected ledges, inactive surfaces and excluded pass-through platforms.
Native follow-up: `maze-connected-floor-campaign-20261006`, diagnostic SHA-256
`4B847DE445051706B4248C7B5A3CB1617D277E62C95F11B3AA8C8E4259BDE2CB`.
This seeded route does not cover all six rooms.
The continuous
campaign and the earlier maze-to-Zelda watchdog remain unvalidated.

## F-Zero course prototype — 2026-10-06

The opt-in `-ExperimentalRace` build adds scene 58 as a twenty-first phase;
normal builds retain twenty phases. The experimental course uses original
GrNBr.dat terrain and markers, solo start, a 240-second timer, rebirth
checkpoints 4-7 and the original 15/2000 finish half extents. Host match/flow
rules pass and the XDK diagnostic prototype compiles. Losing the final life
in a solo course now loses the attempt instead of showing a draw; host tests
verify Continue restores three lives in Mushroom, maze, escape and race.
Dynamic cars and traffic
collisions remain absent. Native traversal reached all three checkpoints and
the finish in Xenia (`build-x360/race-native-20261006`): goal at match frame
3,974, 70 seconds, one fall and recovery, no watchdog stall. This establishes
the terrain/checkpoint/finish path, not the missing traffic gameplay. This prototype
is isolated in `build-x360/race-prototype-objects/race.xex` and has not replaced
the current twenty-phase `dist/default.xex` or its active campaign test.

A normal-boot/live-input candidate also links successfully at
`build-x360/race-prototype-objects/playable-traversal.xex`, SHA-256
`C13FEB2C1DCCF229CF9A7E2850ECD1A806DED23BF4CC5C3AC80B2C5AA48D31C0`.
It has not been promoted while the maze render stalls are being investigated.

Do not treat the preliminary inspection of floor 62 (a high boundary ledge)
as the starting floor. Authored floor 63 spans the player start at approximately
y=49, below spawn y=51.5; its archive geometry is retained without a substitute.

## Storage cleanup — 2026-10-06

Removed fifteen obsolete diagnostic ISO hardlinks and fifteen superseded
diagnostic XEX files, retaining their traces. Removed four generated ASAN
emulator build directories (1.63 GiB).
Removed fifteen obsolete October 5 executable/map snapshots (35.7 MiB),
and eleven superseded prototype binary/map files (32.4 MiB),
retaining current and October 6 reference builds. Source, Git repositories, the current
XEX and recent evidence remain. A subsequent full file scan measured
5.70 GiB across 20,239 files; subtracting the two repeated ISO directory entries
gives 2.98 GiB. This is a sum of file lengths, not allocated disk clusters.
Three ISO paths remain: the source image, normal runtime
and current diagnostic slot; these refer to one physical NTFS file. The removed
ISO links reduced Explorer's summed size, not physical ISO storage. The manifest
is `build-x360/storage-cleanup-20261006.csv`.

## Adventure combat rules — 2026-10-06

The playable `dist/default.xex` now applies the original Adventure attack and
defense percentages per encounter and difficulty. Previously the player ratio
accessors returned 1.0 for every fighter. The original table makes wave enemies
easier to launch and adjusts their attack strength. Player 1, VS and Classic
retain normal ratios. CPU levels 1–2, 3–4, 5–6, 7–8 and 9 currently map to the
five original Adventure difficulties.

The CPU-3 Jigglypuff route pilot passed again in Xenia: ten Yoshi defeated,
gate opened, Mushroom Kingdom exit reached, then Mario/Peach loaded. Its last
pre-clear sample was 3,600 match frames and 26 hits, with eleven total falls
across the route (ten enemies and one player). Evidence is in
`build-x360/adventure-original-ratios-20261006/adventure-route-pilot`.
Host tests cover original ratio values, player isolation and neutral fallbacks.
Live XEX SHA-256:
`E6F9B2A761702F361160919FF6F4BB8F82D1EAB1148462065987C24D8DE1FEC4`.
Enemy CPU levels now come from the original per-slot difficulty table and are
restored on replacement. Original CPU behavior types and the wave generator's
three-to-one random behavior choice are also connected. The selector now shows
the five named Adventure difficulties and LB advances one difficulty.
The Giga Bowser branch requires Normal or higher
(current CPU selector level 5+) and total match time strictly below eighteen
minutes. Time includes finished attempts, excludes flag-0x80 scenes, and is
captured before results. Host regressions cover the exact time boundary,
result delay, fresh campaign reset and retry retention. Runtime validation of
both final branches passed in Xenia: Easy Bowser returns to the menu, while
Normal Bowser loads Giga with two fighters and CPU level 7. The exact
eighteen-minute boundary remains a host test. The accumulated time covers
the ported phases; missing courses do not contribute yet.
The complete original campaign and physical Xbox execution remain unverified.

The preceding ratio/CPU-level build fully cleared Kirby's fifteen enemies
(12,600 sampled frames, 94 hits) and Pokemon's twelve (6,300 frames, 76 hits)
in Xenia. Both runs consumed two player lives. The wireframe pilot remained
stable but did not win within its allotted time. Current behavior/eligibility
build validation is recorded separately; these results do not establish a
complete campaign playthrough.

The preceding original-AI build (`adventure-original-ai-validation-20261006`) passed
nine cases: four encounter combat runs, Mushroom's complete ten-Yoshi route,
Kirby's complete fifteen-enemy wave, Pokemon's twelve-enemy wave (after one
Continue), and both final difficulty branches. Wireframe combat/replacements
remained stable but its automated pilot did not win. The revised main-floor input pilot then cleared all fifteen wireframes in
3,900 sampled frames with 43 hits and no player falls
(`adventure-wireframe-main-floor-20261006`). It uses normal PAD inputs.

A continuous Easy pilot cleared all seventeen mandatory available phases,
including Mushroom Kingdom, Brinstar escape and final Bowser
(`adventure-full-main-floor-20261006`): 918 seconds, two Continues and seven
player falls. Giant Kirby and Giga were skipped by the original eligibility
rules. This run stopped at the final match completion marker; final menu
return is independently verified by the Easy Bowser test. It used the
preceding 0794227E build. The maze, race and mountain courses remain absent.

The current build also restores the original landing, walk and shield
animation-length initialization and the real cached ftData animation lookup.
Previously these lengths were zero, causing special landings to last thousands
of frames. A focused Falcon Xenia run verified three special landings ending
exactly thirty fight frames later, plus walking and shield transitions
(`fighter-animation-lengths-runtime-20261006/special-landing-falcon`).
Host fighter-parts regressions pass. The animation-length build (3EF652AD, before reserved-part creation) passed a continuous
Easy sequence (`adventure-full-animation-lengths-20261006`): all seventeen
mandatory available phases, three Continues and ten player falls in 977 seconds.
Brinstar finished at fight frame 2,287. Both mode.preview_complete: 4 and
loop.flow_state: 2 confirm completion and menu return in this same run.
The diagnostic XEX hash is
`479E2D4464B30CDCC224051EC3B486C3443E39B36DB09FE32C91DE1F28BB0364`;
it shares the native gameplay objects with the live build and adds ordinary
PAD input automation. This does not establish all roster behavior or the
complete original campaign.


### Reserved fighter joints and Link shield (2026-10-06)

An extended Link/Kirby test exposed a reproducible panic at fight frame 2,460.
Link's passive shield used reserved part 68, whose joint remained NULL because
ftParts_800753D4 was a stub. Young Link uses the corresponding reserved part 72.
The native bridge now loads the original descriptor, selects its authored
joint, inserts it with the original attachment type and registers the part.
The renderer shares the resulting joint between its two animation paths.
Removal clears appended display references before releasing the joint.
Host regressions cover all four attachment types, sibling parent relationships,
preorder descriptor selection and instance guards. Both extended Xenia runs
passed 3,600 frames: Link recorded 53 hits and Young Link 37
(`reserved-parts-runtime-20261006`). Peach also passed 5,700 frames with 46 hits
(`animation-lengths-extended-20261006`); the Link freeze in that earlier matrix
was the diagnostic evidence for this fix. Final cleanup of display references
was added after the two successful runs. An actual-function host regression
passes removal of multiple appended displays, retention of unrelated parts,
repeated removal and invalid-entry guards; the Link runtime tests do not
independently exercise that removal path. Complete Kirby hats/copy abilities remain unfinished.
The final live-input XEX includes this fix; the continuous seventeen-phase
proof above was recorded in the preceding animation-length build.

## Native campaign work — 2026-10-05

Adventure now uses nineteen phases extracted from the original encounter table,
including Brinstar escape, enemy teams, retained player lives, Continue,
giant/metal attributes, wireframes and Giga Bowser. Mushroom Kingdom's full
route passed in Xenia with ten Yoshis: 9,900 frames, 199 hits and two player
falls at CPU level 3. The gate opened and the finish was reached with a life
remaining. Camera/blast bounds and NPC replacement were corrected for that
checkpoint. Brinstar escape has its original forty-second timer and top-platform
finish; the complete Xenia climb passed at frame 2,287 without a fall or lost
life, then loaded the next Kirby encounter. Host tests and a smoke run also pass. Permanent metal and size attributes are reapplied on respawn.
Team replacements now use arena spawns without player platforms; host regressions
cover Kirby, Pokemon and wireframes. Samus/Kirby passed 1,800-frame Xenia runs;
a separate Giga Bowser run cleared the final encounter and returned to the menu.
The complete original Adventure and physical Xbox 360 execution remain unverified.
See the October 6 section for the available Easy sequence verification.

Classic remains a five-normal-encounter preview. Original Adventure special
scenes, enemy generators, cinematic transitions,
results, unlocks and persistent saving are unfinished. The milestones below
are historical records, not a claim that those remaining features are complete.

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
- M41 Particles, rules, roster and stages: `psDispParticles` is replaced by a native drawer that submits each particle as a textured view-space quad through the D3D HSD renderer; the select phase adds timed matches (unlimited lives, KO credit from the last attacker, countdown and score HUD, sudden death on a tie); Zelda/Sheik spawn with a sleeping partner and transform through the original down special, and Popo fights with a CPU-driven Nana; fighter part animations (hand poses) play; quake requests shake the camera; Hyrule Temple, Kongo Jungle 64 and Jungle Japes join the stage list, and collision groups bound to animated map joints move with them and carry grounded fighters. Host link check only.
- M42 Item material class initialization (2026-09-29): a scripted Xenia match consistently froze when Mario spawned his cape. The XDK `HSD_MObjLoadDesc` allocated materials without a class pointer, while `hsdMObj` was an uninitialized placeholder. The original item code calls `hsdChangeClass` on these materials. The base material class is now initialized and assigned on load. The same script passed the former freeze and reached match frame 3486, with later hits, stock loss, CPU activity, audio submissions and rendering. This is Xenia validation; physical Xbox 360 behavior and full Classic/Adventure progression remain unverified.
- M43 Soak harness and first freeze fixes (2026-09-30): `tools/soak_xenia.ps1` runs a matrix of unattended matches in Xenia, one instance at a time, from a single `-BootToMatch -InputScript -CallTrace` build. The boot-to-match XEX reads `game:\match-config.txt` (stage, fighters, players, CPU level, stocks/time, items, repeat count), skips the select phase and can chain matches; `game:\input-script.txt` gains a `loop` directive. Runs are classified OK/FREEZE/CRASH/SHORT from the trace, with frame times, heap use and Xenia's host commit (peak and MB/min). `-CallTrace` compiles the C units with `/Gh`; a naked `__penter` keeps the last 64 function entries (and stack pointers), which the watchdog dumps as `hang.call`. Two freezes found this way: (1) a CPU Zelda's down special called the NULL `ftKindCalcIndiviParamTable` entry through `ftCo_800D105C`: the per-kind item, knockback and attribute tables (`ftData_OnItem*`, `OnKnockbackEnter/Exit`, `UnkMotionStates1-4`, `OnAbsorb`, `ftKindCalcIndiviParamTable`) were empty placeholders and are now copied from `ftdata.c` at build time; (2) Samus' grapple read unresolved archive externs (`ItmSamusGBeam*_shapeanim_joint` still held the raw offset 0x99EC): archives opened by the port now bind every extern to NULL after `HSD_ArchiveParse`, as `lbArchive_InitializeDAT` does. Both scripted runs now reach match frame 7500.

- M44 Sound selection and CMPR corrections (2026-09-30): `ft_PlaySFX` now calls the original `ft_80087D0C`/`ft_80087C70` selectors over the original sound range/voice-threshold tables, restoring size/metal variants and Ice Climbers costume voices. SEM lookup rejects IDs spilling into the next bank. GX CMPR now uses Aurora's 5/8 and 3/8 gradients and preserves midpoint RGB on transparent texels. `tools/test_audio_xdk.ps1` passes original-selector checks, CMPR vectors/bounds, all 55 SEM bank boundaries, 4,035 stream resolutions and 32 XAudio2 test-double submissions. A freshly built scripted Mario-vs-Fox Xenia match reached frame 3900 with 17 hits, 128 SFX submissions, one SFX miss and zero music history mismatches; no hang/crash was recorded. `dist/default.xex` was then relinked with normal boot and live input and passes `imagexex /DUMP`. Audible/visual correctness, full SEM interpretation and physical console behavior remain unverified; Classic/Adventure and complete menus remain unfinished.

- M45 Native lifecycle and menu recovery (2026-10-02): SFX have real voice tokens, individual/track stop, completion queries and pitch changes; stale handles are harmless, reused voices reset pitch and completed slots are preferred before stealing active ones. Scene transitions stop SFX and release cached textures. Texture identity includes dimensions/format/palette metadata, failed uploads can retry and a full cache evicts its least-recently-used entry. Unimplemented menu destinations display a notice and recover navigation, including pending scene exits. Match BGM uses the original HPS filename table and stage archive's base VS/1P music entry, with separate selection music. Host audio/selector/CMPR/cache regressions pass. A normal-boot scripted Xenia run traversed opening, title, menu and character select to match frame 3114 (16 hits, 124 SFX submissions, one SFX miss, zero HPS history mismatches), including pause/resume. A second run entered Special VS camera mode (unimplemented mode 10), recovered its menu and returned through VS to the main menu. The final `dist/default.xex` is relinked with live input and passes `imagexex /DUMP`. Visual/audible fidelity and physical console behavior remain unverified; complete menus, original campaign progression, alternate music and full SEM interpretation remain pending.

- M46 Stock resolution and campaign lives (2026-10-02): the native Classic/Adventure scaffold starts with the original default of three player lives, carries losses into later rounds and assigns an ordinary opponent one stock. A fresh campaign resets the lives. Stock matches stop processing eliminated slots and resolve their result after all blast-zone checks, so eliminated players do not repeatedly lose stocks and simultaneous last falls produce a draw. Draws cannot advance the campaign. Result text distinguishes round clear/game over and final completion, the select screen exposes the existing CPU-level control, and match restarts no longer tear down the scene twice. `tools/test_match_rules.ps1` compiles the actual match frame/rules with mocked fighter/HUD boundaries and passes life carry/reset, four-player elimination, simultaneous fall, winner/advance and timed respawn checks. This does not implement the original campaign encounter order, stages, bonuses or bosses.

- M47 Selection and partial-load recovery (2026-10-02): costume cycling and duplicate-fighter color assignment use each character's actual archive count, and the selection UI shows the color count. Entry resets analog edge state, changing player count clears stale confirmations/removed human slots, and a disconnected selection pad becomes a CPU slot. Fighter/costume load failure no longer silently substitutes Mario/default color; a partial match tears down, resets fighters/effects, rebuilds its stage and resumes selection with a failure message. Automated failed loads exit rather than waiting for input. Fighter GObj/JObj allocation is checked before use. `tools/test_match_rules.ps1` passes the actual costume-count, selection and start functions with injected partial failure/successful retry, alongside earlier stock/rules regressions. A read-only ISO audit found all 120 costume archives and their expected model/material-animation roots. The texture-cache cleanup entry point has C linkage for the new C scene caller.

- M48 GX vertex alignment and UV sources (2026-10-02): NBT3 indexed normals consume all three index8/index16 values before following attributes, fixing stream alignment. Truncated tuples, absent indexed arrays and unsupported attribute types reject parsing. Raw vertices retain TEX0-TEX7; the material routes its requested UV source into each of the two existing shader texture slots without expanding the 56-byte GPU vertex layout. Actual parser/transform host regressions pass with earlier cache/CMPR/audio tests. A costume mesh audit found only TEX0/TEX1 and no indexed NBT3 across 9,977 meshes; these fixes do not establish a solution to every reported visual defect. A normal-boot scripted Xenia route reached match frame 4014 with 28 hits, 181 SFX submissions, one SFX miss, pause/resume and zero HPS history mismatches; no hang was observed. The final XEX is relinked with live input and passes the image dump check. Bump mapping, complete texgen/TEV and materials with more than two textures remain pending.

- M49 Material pass routing (2026-10-02): diffuse/ambient, specular and extended texture flags map to independent shader passes, matching upstream `MObjMakeTExp` order. Specular textures now modify the specular term instead of the diffuse base; multi-pass diffuse/extended textures also apply after lighting, and specular-only textures are ignored when specular lighting is disabled. Specular alpha operations carry the shared alpha term. Both model/particle paths upload all 13 boolean constants so routing state does not leak. Host routing/vertex/cache/CMPR/audio tests pass and the updated XDK pixel shader compiles. A read-only Samus model audit found three enabled specular texture entries. A scripted opening/title/VS route loaded Samus vs Mario, reached match frame 901 with four hits, paused and returned to the main menu without an observed hang. The final XEX has normal boot/live input and passes the image dump check. Visual/physical-console correctness, more than two texture slots and full TEV/texgen remain pending.

- M50 Match replacement lifecycle (2026-10-02): next campaign rounds and automated VS restarts now leave the previous match scene before clearing cached textures and entering the replacement. Previously, same-state transitions skipped teardown and match entry reset the built-stage marker, allowing old fighters/cameras/GObjs to survive. `tools/test_flow_rules.ps1` passes both five-round campaign transitions/completion, three VS restarts and menu exits, checking release ordering. `tools/test_match_rules.ps1` passes actual entry/spawn/frame/leave sequences for five rounds of Classic and Adventure with two life losses, respawns, retained lives and new-campaign reset. Host fighters/platform functions are mocked. Xenia's opt-in `dist/campaign-lifecycle-20261002` harness completed three consecutive four-CPU VS matches with zero ADPCM history mismatches and no observed hang. It validates scene replacement, not the full original campaign or long-run memory stability. The final XEX is rebuilt with normal opening boot/live input and passes the image dump check. Original campaign encounters, bonus stages, bosses and results remain pending.

- M51 Local VS usability (2026-10-02): A at VS results starts a direct rematch retaining roster/colors/rules/current human slots. Any human can pause with Start; a disconnected human pad pauses simulation after GO, blocks resume until reconnection, and requires explicit Start to continue. Another connected human can exit the pause via B. The UI identifies a missing pad and records the player who pressed Start in the original pause panel. Actual-function host tests cover four-slot rematch after draw, retained rules/human slots, secondary pause, disconnect freeze, reconnect/resume and other-controller exit. A normal-boot scripted Xenia route reached Mario vs Luigi's one-stock result, rematched, paused and returned to the menu at replacement frame 361 without observed hang or HPS history mismatches. Final XEX: normal opening/live input, image dump checked. Physical console and actual multiple-controller behavior remain unverified. `docs/JUGAR_XEX.md` documents the playable VS route and controls. The build's imagexex banner no longer produces a false nested-PowerShell failure; the extracted actual image-builder block returned exit 0 in a nested invocation.

- M52 Classic normal encounters (2026-10-02): the five-fight combat preview uses generated original normal encounter pairs (38 source rows, eight compatible with current native stages), native fighter mapping, supported-stage filtering and preference for unused opponents/grounds. It uses the selected StKind's StageParam row for 1P BGM/item parameters. Missing variants abort instead of silently using another setup. Classic gets its original 300-second stock timer; timeout removes one life and permits retrying the same encounter while lives remain, including round-zero retry without refilling lives. The original HUD supports stock lives and timer together. Host tests pass original encounter pair/variant lookup and repeated timeout/retry to game over alongside existing rules/flow tests. An ISO audit found all eight supported rows. Xenia traversed normal boot/menu/Classic, loaded StKind 89 Jungle Japes vs Donkey Kong with HUD timer 300 and BGM 50, reached frame 901, paused and exited without observed hang/HPS history mismatch. Final XEX is normal boot/live input and image-dump checked. The five-fight UI/logs identify a preview; original teams/giant/metal/bonus/boss encounters, actual order, cinematic/results flow and Adventure progression remain pending.

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
