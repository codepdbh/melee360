# Remaining work for playable Melee

The XDK executable has a quick-match route from VS mode and a native match
loop. Scripted Xenia input verifies movement, attacks, hits, damage, stock loss,
respawn, Mario's cape, CPU actions, music and SFX submissions. Interactive
controller play on a physical Xbox 360 remains unverified.

## Verified current path

The normal build starts at the opening movie, then title, original menu,
native fighter/stage selection and match. Diagnostic `-BootToMatch` and
`-InputScript` builds are opt-in and must not replace the live-input release.

### Experimental Underground Maze (2026-10-06)

The current normal XEX adds a twentieth Adventure phase with the original
GrNSr.dat course, six symbols, randomized Triforce room, Link encounters and
seven-minute timer. Collision switching restores original disabled traversal
segments, and entrance trigger dimensions use the original half extents.
Xenia verified three Link-room victories and a Triforce clear after a timeout
and retry (`adventure-maze-drop-pulse-20261006`, 445 seconds). All rooms in one
attempt and the full updated campaign remain unverified. Maze hazards and
transition animations remain missing; F-Zero race and mountain climb are absent.
Current normal SHA-256:
`CFBA92ACDC18193E0627EA66AFD334035B5CDF55B7A5D6C3BFECFFA10BE936E0`.
The full normal rebuild now includes original grounded body inclination and
foot IK (`ft_0899.c`, `lb_020A.c`), authored leg descriptors and priority-7
processing. Host checks include both leg chains and small/giant model sizes;
matrix application is mocked. Four diagnostic Xenia combat cases pass 1800
frames, and goal reruns clear maze and Brinstar escape
(`ground-pose-native-20261007`, `ground-pose-courses-20261009`). This is not a
full campaign, visual parity or physical-console verification.
This build restores only traversal groups 0-7 after a maze fight, preserving
the original disabled state of other arena groups. The preceding camera build
reached loop frame 10,200 and presented 1,922 opening frames without movie
errors, ADPCM history mismatches or a watchdog alert
(`normal-fighter-camera-runtime-trace-20261007.txt`).
It also uses the original grab wall-occlusion query; host tests verify the
ECB center coordinates and wall-side choice. The complete runtime grab
interaction across all stages remains unverified.
Original moving-surface velocity remapping and its four collision queries now
replace constant-zero stubs. Host comparisons with the upstream math pass;
a four-CPU/items Fountain test reached frame 1800 without a watchdog. The full
original collision engine and unimplemented stage hazards remain pending.
Original stage material tables now restore friction and sound/effect queries
for footsteps, landings and ground bounces. Host tests cover all 1420 table
entries; Mario/Zelda and four-CPU Fountain tests reach frame 1800. Audible
and visual parity of these effects still require comparison with the original.
Original fighter camera callbacks and model scaling now run, including
independent width handling. Link/Young Link, small DK and the Easy Bowser
final pass native tests. Camera subject framing/easing, visual parity and a
complete campaign with these latest changes remain unverified.
Authored next/previous floor links now replace constant-minus-one queries;
338,688 host comparisons match the original selection math. Native collision
toggles still collapse hidden/disabled state, and the original collision
engine is unfinished. Original stage identity lookup now includes all 286
entries and preserves campaign variants. The combined diagnostic passes
Fountain/Team Kirby at frame 1800, finishes a short Ness/DK match, and clears
the seeded maze and Brinstar escape goals at frames 4190 and 2287.
These goal tests cover individual phases, not the full campaign or all maze
variants (`stage-identity-final-courses-20261007`).
The current normal XEX reaches loop frame 4500 and presents 2338 opening
frames with no movie errors, ADPCM history mismatches or watchdog alert;
its image dump succeeds (`normal-stage-identity-runtime-trace-20261007.txt`).
Physical console behavior and visual comparison remain unverified.
The nineteen-phase results below describe preceding builds.

## Fighter and item fixes (2026-10-05)

### Adventure route implementation and validation

`generate_adventure_matchups.py` extracts the original Adventure encounters,
stage identifiers and time limits. The flow now covers nineteen encounter
phases, including giant fighters, metal Mario/Luigi, male/female wireframes
and Giga Bowser. Campaign enemies share a team; victory requires every enemy
stock to be exhausted. Continue restores three player lives at the same
encounter. Classic still has five normal encounters.

Mushroom Kingdom starts with the player alone. Its authored checkpoint starts
ten Yoshis across three active fighter slots (4/3/3 lives). Replacements spawn
at the checkpoint; camera and blast bounds stay local until all ten are beaten.
The gate then opens and the authored finish marker ends the phase. A controller
pilot completed this route in Xenia at CPU level 3: 9,900 frames, 199 hits,
ten enemy eliminations and two player falls, with a life remaining
(`adventure-route-and-escape-20261005/adventure-route-pilot`).

Brinstar escape is a solo, forty-second climb on the original GrNZr stage.
Landing on collision group 1 completes it; timeout removes one life and retries
the phase. Host regressions and a 1,200-frame Xenia smoke run pass. The controller pilot
also completed the full climb at frame 2,287 without a fall or lost life
(`escape-precise-20261005/adventure-escape-pilot`). The trace then loaded the
following Kirby encounter. It used normal PAD inputs, without position, damage,
stock or timer changes. The CSV's frame 2,100 is the last periodic sample;
`match.adventure.escape_goal: 2287` records the actual finish.

The seventeen-phase build preceding the wireframe insertion passed 1,200-frame
Xenia smoke runs for each phase (`build-x360/adventure-final-20261005`). The
eighteen-phase build passed the wireframe, metal and giant Bowser runs, but
Giga Bowser and the scripted Jigglypuff course run exited after a player death.
An instrumented rerun subsequently reached 1,800 frames for both cases and
completed their first player respawns (`adventure-rebirth-trace-20261005`).
The earlier exits were not reproduced; their cause is not yet established.

A longer course run exposed a separate checkpoint load failure: Yoshi's
OnLoad callback read an uninitialized native `x5AC` visibility descriptor.
The fighter loader now populates it from the selected costume, with the
original costume fallback, before invoking character callbacks. A host
regression covers those pointers and flags. The corrected Yoshi/Pichu Xenia
match recorded eleven hits and two stock losses without a freeze
(`yoshi-init-fix-20261005`). This is not a completed Adventure playthrough.

The current live-input XEX was built without diagnostic boot/input flags:
SHA-256 `E6F9B2A761702F361160919FF6F4BB8F82D1EAB1148462065987C24D8DE1FEC4`.

Ceiling collision now measures the six ECB bones named by the original
fighter collision descriptor, with the original two-unit margin and a scaled
fallback for absent descriptors. Previously it used the camera framing extent.
Host regression checks the authored bones, origin, invalid indices and fallback.
The complete climb passed with this correction and the revised input pilot.

The following nineteen-phase smoke runs reached 1,800 match frames for Samus
and Kirby (`adventure-escape-neighbors-20261005`). Giga Bowser ended earlier:
the trace records player victory, campaign completion and a return to the menu.
The harness labels it SHORT because its 1,800-frame target was not reached;
this run establishes the final encounter's clear/menu path, not a full campaign.
Kirby, Pokemon and wireframe replacement slots now enter at the arena spawn,
without a player rebirth platform. Host tests cover a defeated/replaced enemy
in each of those three teams. They still reuse slots rather than the original
enemy generator.


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

### Original encounter rules added on 2026-10-06

The two Donkey Kongs use the original half-size modifier (`gm_801B4768`).
Clearing Mushroom Kingdom with a displayed seconds digit of 2 selects Luigi
in place of Mario in the next encounter (`gm_801B44A0`, `gm_801B461C`); the
Luigi cutscene and unlock/save logic are still absent. The port reads the same
whole displayed seconds used by the native HUD, rather than rounded-up time.
Giant Kirby is skipped after a Team Kirby clear lasting more than thirty whole
seconds (`gm_801B4C5C`). Clear time is captured before the result animation,
so waiting on the result does not change eligibility. Host regressions cover
the whole-second boundary, delayed results, Luigi selection and small DK scale.
These conditional transitions still need their own end-to-end Xenia checks.

Enemy CPU levels now use the original per-slot `gm_8017E5C8` table, including
different levels for paired and metal opponents. Team replacements retain
their encounter level. Giga Bowser is selected only at Normal or higher
(current selector level 5+) with accumulated match time strictly below 64,800
frames, matching `gm_8017E7FC`. Finished attempts count toward total time;
flag-0x80 scenes are excluded, result animations do not add time, retries
retain totals, and a fresh campaign resets them. Host regressions cover these
boundaries. Current-build Xenia validation also confirms both difficulty
branches: CPU-3 Bowser victory returns to the menu without Giga, and CPU-5
Bowser victory loads scene 92 with two fighters and native Giga CPU level 7.
The eighteen-minute runtime threshold is covered by host tests, not an actual
eighteen-minute playthrough. Total time currently covers ported encounters;
the three missing courses cannot contribute until they are implemented.
CPU behaviors also come from `gm_8017E630`: Mario/Peach, small DKs, Fox and
Ness retain their individual original patterns. Generated waves choose behavior
23 or 24 with the original three-to-one probability; permanent metal forces
behavior 27. Respawns restore both behavior and level. The Adventure selector
shows Very Easy/Easy/Normal/Hard/Very Hard and LB advances one difficulty.

The Kirby, Pokemon and wireframe wave builds each passed 1,800-frame Xenia
runs (`adventure-wave-replacements-20261005`), with 42, 41 and 18 hits.
The runs establish combat stability, but do not independently prove a full wave
clear or replacement cycle; the latter is currently covered by host rules.
On October 6 the new original-ratio/CPU build also cleared the full fifteen-
Kirby wave with the input pilot: 12,600 sampled frames, 94 hits and seventeen
total falls (fifteen enemies and two player lives), with native replacement
traces and `match.result.winner: 0`. Evidence:
`build-x360/adventure-original-wave-clears-20261006/adventure-kirby-wave-pilot`.
The same build cleared all twelve Pokemon: 6,300 sampled frames, 76 hits,
fourteen falls (twelve enemies and two player lives), and player victory.
The wireframe pilot did not clear its wave in the allotted time; it remained
active and performed replacements but lost repeated player attempts. These
runs preceded the additional original-CPU-behavior and Giga-eligibility changes;
validation of the current build is in `adventure-original-ai-validation-20261006`.
The October 6 full-campaign pilots were intentionally stopped for further
rebuilds; their termination verdicts do not establish a game crash. They did
not complete the campaign. A subsequent continuous pilot cleared all seventeen
mandatory available Easy phases; see the evidence below. The complete original
campaign, including the three missing courses, remains unverified.

The preceding original-AI build's matrix (`adventure-original-ai-validation-20261006`)
passed nine of ten cases. Small DK, Giant Kirby, Ness and metal Mario/Luigi
combat runs passed. Mushroom cleared at a last sample of 5,100 frames with
45 hits and one player fall. All fifteen Kirby cleared at 8,700 sampled frames
with 75 hits and two player falls. Pokemon cleared on its second attempt:
6,300 maximum sampled frames, 72 maximum per-match hits and 26 falls across
both attempts; the winning attempt defeated all twelve enemies and lost two
player lives. The final difficulty branches passed as described above.
Wireframe replacement and repeated Continue remained stable, but its pilot
did not win in the allotted time. The revised main-floor PAD pilot subsequently
cleared all fifteen wireframes at 3,900 sampled frames, with 43 hits, thirteen
replacement traces and zero player falls
(`adventure-wireframe-main-floor-20261006`).

The continuous Easy pilot (`adventure-full-main-floor-20261006`) cleared all
seventeen mandatory available phases in 918 seconds, with two Continues and
seven player falls. It finished Mushroom Kingdom, Brinstar escape and Bowser;
Giant Kirby and Giga were skipped by the original conditions. The harness
stopped at match.campaign.preview_complete, before confirming menu return in
that continuous run. The separate Easy Bowser run verifies menu return.
This pilot used the preceding 0794227E build.

The live build now initializes landing, walk and shield animation lengths
from the original cached FigaTree data, following Fighter_Create_Inline2.
The ftData_80085E50 bridge previously returned NULL and all five duration
fields remained zero. Falcon special landing consequently advanced extremely
slowly. Host regressions and a focused Xenia run now verify three landings
exiting exactly thirty fight frames later, alongside walk/shield transitions
(`fighter-animation-lengths-runtime-20261006/special-landing-falcon`).
The timing-corrected build (3EF652AD, before reserved-part creation) passed a continuous Easy run
(`adventure-full-animation-lengths-20261006`) in 977 seconds, with three
Continues and ten player falls. All seventeen mandatory available phases
cleared. Brinstar finished at fight frame 2,287; mode.preview_complete: 4
and loop.flow_state: 2 confirm completion and menu return in the same run.
Diagnostic XEX SHA-256:
`479E2D4464B30CDCC224051EC3B486C3443E39B36DB09FE32C91DE1F28BB0364`.
The diagnostic build shares native gameplay objects with the live build;
only boot and ordinary PAD input automation differ. The three missing
courses, original generators, cinematics, saves and physical console remain
outside this proof.

Adventure now imports the original per-encounter attack and defense ratios
from `gm_17E4.c` and feeds them into the native fighter hit calculations.
Previously both player accessors always returned 1.0, making wave enemies
much harder to knock out than intended. The generator counts every original
GS_VS scene so omitted courses do not shift later encounter statistics.
The current CPU selector maps levels 1–2/3–4/5–6/7–8/9 to the five original
difficulties. Player 1 and non-Adventure matches retain normal ratios;
sentinel escape and unavailable low-difficulty Giga rows use neutral values.
Host regressions cover player isolation, Yoshi/Kirby difficulty values and
escape/VS fallback. The playable XEX was rebuilt on October 6; runtime wave
validation passed for the ten-Yoshi Mushroom route at CPU 3 with Jigglypuff:
gate opened, finish reached, and Mario/Peach loaded. The last pre-clear sample
was frame 3,600 with 26 hits; the trace records eleven total falls, including
ten Yoshi and one player. Evidence:
`build-x360/adventure-original-ratios-20261006/adventure-route-pilot`.
The first harness summary reports zero hits because the next match reset its
counter before sampling; the raw trace preserves 26. Future summaries retain
the maximum per-match hit count, matching their existing frame-count behavior.

The per-life reset also restores permanent metal state and reapplies native
scaled attributes. Temporary metal is still cleared by the common life reset.

Remaining Adventure work includes original enemy generators and wave rules,
the maze, race and climb scenes, cinematics,
results, unlocks and persistent saves. Metal physics are present; the
original reflective metal appearance remains incomplete. Physical Xbox 360
testing is still required.

The native fighter skeleton now preserves the reserved bone positions from
`Fighter_804D6540`. Kirby's 46 costume joints occupy 59 logical positions;
his five authored hurtboxes now resolve to valid joints. Dense animation
traversal retains a mapping to those logical positions. The previous Xenia
panic at loop frame 205 no longer occurs: the scripted Kirby/Link run reached
match frame 6900 with 28 hits and three falls.

`lb_80014498` now performs the original color-overlay reset. Recycled item
memory previously retained an invalid color-animation index because this
function was a stub. The Zelda/Din's Fire regression previously froze after
match frame 4800; after the reset fix it reached frame 9600 with 44 hits.
These traces are in `build-x360/soak-fixed-20261005`.

Costume texture-animation tables now map authored texture indices onto the
loaded materials, including the fallback to costume zero. Frame selection,
reset, callback dispatch and frozen texture animation rates are implemented.
`tools/test_fighter_parts.ps1` exercises this code, the sparse bone mapping
and color-overlay cleanup with actual extracted native functions. Visual
confirmation of character expressions remains pending.

The previously stubbed CPU spacing recovery (`ftCo_800A0098`) now queues
the original nine controller commands, checked against the retail PPC routine.
Fox and Marth previously stopped fighting after 27 hits; the same two-CPU
test now completes all three requested matches. Its final match records 63
hits, with four falls across the run (`build-x360/soak-cpu-20261005`). A separate
scripted-fall test also completes three matches, exercising repeated teardown
and fighter reload (`build-x360/soak-textures-20261005`).

The interactive Xenia launcher explicitly enables audio and selects XAudio2;
`-Mute` silences it. Soak tests retain silent operation by default, with
`-Audio` enabling output. Submitted PCM and advancing playback counters do
not independently verify what a listener hears on their output device.

## Native lifecycle fixes (2026-10-02)

SFX now return voice tokens instead of placeholder handles. Individual stops,
track key-off, completion queries and pitch changes reach XAudio2; reused slots
invalidate old tokens, restore neutral pitch and prefer completed voices over
interrupting active ones. Scene transitions stop effects from the previous scene.
The original fighter hit-sound pitch variation is restored. Eight simultaneous
native voices and the simplified SEM stream resolver remain limitations.

Texture cache identity includes dimensions, format and palette metadata, with
LRU eviction, retry after failed upload and release on scene transitions. Host
regressions exercise these rules using the actual cache implementation. This
does not complete GX materials, lighting or TEV.

Unimplemented menu leaves display a temporary notice and resume navigation;
unsupported scene exits rebuild their current submenu so it remains usable.
Those destinations are still unimplemented. Match music now uses the original
HPS filename table and the stage archive's base StageParam row (VS/1P columns),
with separate character-selection music. Alternate music and the full campaign
stage sequence remain pending.

## Local VS usability (2026-10-02)

VS results now accept A for a direct rematch, retaining the selected roster,
colors, stage, rules and current human slots (including mid-match joins).
The replacement still uses the scene teardown path described below; a failed
fighter load returns to selection instead of entering a broken rematch.
Any human player's Start button can pause/resume. A disconnected human pad
pauses the simulation once the initial countdown ends and blocks resume until
all missing pads reconnect. Reconnection leaves the match paused until Start;
another connected human can leave via B. The UI identifies the first missing
pad, and the pause panel records the player who pressed Start.

Host tests run the actual rematch/entry/frame functions, retaining four-slot
selection/rules after a draw and checking secondary pause, frozen frames on
disconnect, explicit reconnection/resume and exit via another controller.
The normal-boot scripted Xenia run in `dist/playable-vs-20261002` traversed
opening/title/menu/VS, selected a one-stock Mario vs Luigi fight, reached a
CPU win, accepted A for a direct rematch, then paused and exited at frame 361
of the replacement. No hang or HPS history mismatch was observed. The final
XEX is rebuilt with normal opening boot/live input and passes image dump
validation. Real multi-controller/disconnect behavior and physical Xbox
rendering/audio remain unverified. See [the play guide](JUGAR_XEX.md).

The image builder now redirects its normal stderr banner to a log and prints
it as ordinary output. This avoids a false exit code 1 when a successful
build is launched through nested Windows PowerShell with output redirection;
the imagexex process exit code still determines failure. The actual builder
block was checked in a nested PowerShell process and returned exit 0.

## Campaign scene replacement (2026-10-02)

## Classic normal encounters and timer (2026-10-02)

The five-fight Classic combat preview now draws normal opponent/stage pairs
from the original `gmClassic_803DDEC8.x0CC` pool. The build generates its 38
entries from `gmclassic.c`, resolves character-to-fighter mapping from
`player.c`, and resolves the ground from `stage.c`. Selection filters to
native supported stages/fighters, excludes the chosen player fighter, and
prefers opponents/grounds not used in previous preview rounds. The selected
pair persists across retries. Adventure's route remains provisional.

Classic looks up the actual StKind in the archive's StageParam rows, using
that variant's 1P BGM and item parameters instead of the base VS row. A
read-only audit of the user's ISO found all eight compatible normal encounter
pairs and their original variant rows, including the different Final
Destination/Temple music choices. A missing variant aborts the encounter
instead of silently using another stage's setup.

Normal Classic fights have the original 300-second limit with stock lives,
rather than VS timed scoring/unlimited respawns. Time-out removes one player
life; A retries the same encounter if any remain. Retrying round zero does
not refill lives. At zero lives, only returning to the menu is allowed.
The original HUD now accepts an independent stock/timer setting. The UI and
completion trace identify the five-fight preview; it grants no full-campaign
completion or unlocks. Original round order, teams, giant/metal encounters,
bonus stages, bosses, cinematic flow and campaign results still need porting.

Host tests execute the actual encounter selection, native roster lookup,
StageParam/BGM lookup, entry/frame/leave and retry functions. They check
original pairs, supported stages, early-stage variety, stock timer and three
time-outs to game over with same-encounter/life retention between retries.
The scripted Xenia run in `dist/classic-original-20261002` traversed the normal
opening/title/1P/Classic route, drew StKind 89 (Jungle Japes vs Donkey Kong),
started the stock HUD with 300 seconds and original BGM 50, ran 901 frames,
paused and returned to the menu without an observed hang or HPS history
mismatch. This is not a complete campaign or physical-console validation.
Final XEX: normal opening/live input, image dump checked.

## Campaign stock state (2026-10-02)

Scene replacement now tears down the current match even when the flow stays
in the match state (next campaign round or automated VS restart). Previously,
entry reset the built-stage marker before rebuilding, bypassing both teardown
paths and leaving the old fighters, cameras and other game objects alive.
`tools/test_flow_rules.ps1` runs the actual menu/match transition functions
through both five-round campaigns, completion, repeated VS restarts and exit;
it checks teardown happens before texture-cache clearing and the next entry.
`tools/test_match_rules.ps1` also runs the actual match entry, spawn, frame and
leave functions through all five rounds of each campaign with two player life
losses, respawns and a fresh-campaign reset. These are host tests with mocked
fighters/platform functions, not a complete campaign played in Xenia.
The opt-in Xenia harness in `dist/campaign-lifecycle-20261002` completed three
consecutive four-CPU VS matches (`auto.all.done: 3`) using the same scene
replacement path, without an observed hang or ADPCM history mismatch. This
does not establish visual correctness or prove long-run memory stability.
The final XEX is rebuilt with normal opening boot and live controller input.

The provisional Classic/Adventure route now starts with the original default
of three player lives, retains losses across its rounds and gives each ordinary
opponent one stock. A fresh run resets the player lives. Result prompts distinguish
round clear, game over and the final round; a draw cannot advance the campaign.
The original encounter order, bonus stages, bosses and campaign results are
still missing, so this remains a five-fight scaffold.

Stock matches ignore eliminated fighter slots and decide the result after all
fighters' blast-zone checks for the frame. This prevents repeated stock-loss
notifications while surviving players fight and prevents a false winner when
the last two fighters fall together. Timed matches retain unlimited respawns.
`tools/test_match_rules.ps1` executes the actual match frame function with mocked
fighters/HUD, covering these cases and campaign life/reset rules. Re-entering a
match now uses a single teardown through the flow state transition.

A scripted Xenia run in `dist/classic-rules-final-20261002/` traversed opening,
title, 1P menu, Classic submenu and character selection. Its fight recorded
player lives dropping to 2, 1 and 0, then winner 1 and a B-button return to the
main menu without an observed hang. It does not verify advancing an entire
campaign. The final `dist/default.xex` is rebuilt with normal boot/live input
and passes the XDK image dump check.

## Selection and load recovery (2026-10-02)

Costume selection uses each fighter's actual archive count, including wraparound
and normalization when changing characters. Duplicate fighters receive distinct
available colors with a bounded search. The UI shows the color index and count.
Changing player count clears old confirmations and removed human slots;
disconnecting a pad during selection makes that slot available as a CPU again.
Previous stick edges are reset when entering selection.

A failed fighter/costume load no longer silently substitutes Mario or the
default costume. A partial match load frees its scene, resets fighters/effects,
rebuilds the stage and returns to selection with a load-failure message. Failed
GObj/JObj allocation is checked before constructing a fighter. The automated
match harness exits a failed load instead of waiting indefinitely at selection.
Host tests execute the actual selection/start functions with injected load
failure, then verify a successful retry. A read-only audit of the user's ISO
found all 120 costume archives and their model/material-animation public roots.

The scripted Xenia run in `dist/selection-20261002/` traversed opening/title/VS,
cycled Luigi's four colors, selected two Luigi with the same requested color
and loaded `PlLgNr.dat`/`PlLgWh.dat` as distinct costumes 0/1. It reached match
frame 631, paused and returned to the menu without an observed hang. Partial
load recovery and pad-disconnect behavior are host-test evidence, not hardware
tests. The final normal-boot/live-input XEX passes `imagexex /DUMP`.

## GX vertex and UV fixes (2026-10-02)

Indexed NBT3 normals consume all three indices (three/six bytes for index8/16)
before the next vertex attribute, matching upstream Aurora's GX display-list
reader. The port still uses only the normal vector; tangent/binormal-dependent
bump mapping remains unsupported. Truncated tuples, absent indexed arrays and
unknown attribute types fail parsing instead of shifting the remaining stream.

The raw vertex retains TEX0 through TEX7. Each of the two supported material
texture slots selects its requested UV set before upload, so TEX2-TEX7 no longer
fall back to TEX0. The GPU vertex layout stays at 56 bytes. This does not lift
the two-texture material limit or implement the missing TEV/texgen modes.
`tools/test_audio_xdk.ps1` tests the actual parser/transform functions for NBT3
index8/16 alignment, tuple truncation, direct normals and all eight UV sets,
alongside existing texture-cache, CMPR and sound tests. A read-only audit of
9,977 costume meshes found TEX0/TEX1 and no indexed NBT3, so these fixes alone
do not explain all reported fighter appearance defects.

## Material texture passes (2026-10-02)

The two native texture slots now route diffuse/ambient, specular and extended
lightmaps independently, following the three loops in upstream `MObjMakeTExp`.
Specular textures modify the specular material term before its lighting sum,
instead of coloring the diffuse base. Extended textures can also apply after
lighting when the same texture participates in an earlier pass. A specular-only
texture is skipped if the material disables specular lighting. The shared alpha
term carries specular texture alpha operations into the final result.

The particle path resets all 13 shader boolean constants, including the new
specular routing flags. Host tests cover specular enabled/disabled and combined
lightmap flags; the XDK pixel shader compiles. A read-only Samus costume audit
found three specular texture entries on specular-enabled materials. This still
uses at most two textures, simplified TEV operations and incomplete texgen;
visual fidelity and physical Xbox output remain unverified.

The scripted Xenia run in `dist/material-passes-20261002/` traversed opening,
title and VS selection to Samus vs Mario, reached match frame 901 with four
hits, paused and returned to the main menu without an observed hang. The final
normal-boot/live-input XEX passes the XDK image dump check.

## Earlier runtime evidence

`main.cpp` runs the original menu scene through the shared HSD renderer. VS
selection enters `M360_MatchEnter`, which loads a selected stage, creates its
stage/camera objects and loads selected fighters from the user's ISO. The match
loop advances stage and fighter animation, runs common motion states, checks
stage collision and blast zones, and renders the scene. On 2026-09-29, the
40-step scripted Xenia run passed the former freeze at match frame 1000 when
Mario spawned his cape, then reached match frame 3486 with further hits and
successful frame presentation. The trace reported 88 SFX submissions and one
miss by frame 1500. It does not verify audible quality or console behavior.

The current Xenia runtime trace records the original menu exiting with
`GM_CLASSIC`, selecting mode 3, loading both fighters, and finding/playing
`audio/vl_battle.hps` on arena entry. This only verifies that the menu launches
the shared VS arena. The current five-round HUD/return controls are a scaffold;
the original Classic/Adventure character roster, stage sequence, round setup,
and results progression are not implemented. Do not treat either mode as
playable yet.

The `ft_PlaySFX` bridge resolves each game sound through `audio/smash2.sem`, finds the referenced SSM voice in the ISO, decodes its DSP-ADPCM sample, and submits it to XAudio2. The SEM table resolves 4,016 of its 4,035 streams to SSM entries; a small remainder points outside the available SSM ranges. Background HPS music remains a separate working path.

The 2026-09-23 noise fix corrects three faults: treating the first sample byte
as a DSP frame header, overwriting mono samples while expanding them to stereo,
and writing into a buffer before its previous XAudio2 voice had stopped reading
it. The shared decoder now uses nibble addresses, the voice's initial predictor
and history, and the inclusive AX end address. PCM is decoded into separate
scratch channels; an active voice is destroyed before reusing its output buffer.

Host validation compared 21 mono/stereo SSM entries from `main`, `mario`, `fox`,
`falco`, and `1padv` against vgmstream r2117: all 413,827 compared PCM values
matched. Vgmstream trims 1-3 final samples differently; the port retains the
inclusive AX endpoint. Clips still have a 65,536-frame cap. A host harness
compiled the actual XDK audio source with an XAudio2 test double: 32 submissions
preserved the decoded channels, pan and volume, and 24 active buffer replacements
left the old PCM untouched until voice destruction. The rebuilt HPS test and
nibble-boundary regression checks pass. Local validation scripts/results are in
`build-x360/audio-validation/`. These checks do not establish audible playback
in Xenia or on console; that still needs an in-game listening check.

## Sound selection and CMPR corrections (2026-09-30)

The native `ft_PlaySFX` bridge now calls the original `ft_80087D0C` selector,
with `ft_80087C70` and the original audio range/voice-threshold tables extracted
at build time. This restores size and metal sound variants and the Ice Climbers'
costume-dependent voice swap. The US language mapping remains the existing
identity bridge. Native sound handles now support individual/track stop,
completion queries and pitch adjustment, but the full SEM interpreter remains
incomplete.

SEM lookup now rejects a local sound ID at the next bank's boundary, following
`AXDriver_8038CFF4`, instead of allowing it to play another bank's stream.
`tools/test_audio_xdk.ps1` executes the original sound selectors on the host,
checks all 55 bank boundaries against the user's ISO and validates 32 XAudio2
submissions with a test double. These tests do not verify audible correctness.

CMPR decoding now matches the bundled Aurora converter's 5/8 and 3/8 gradients
and retains midpoint RGB on transparent texels. The old DXT1-style thirds and
transparent black differed from GX and could introduce dark filtered edges.
The same host command tests fixed colour vectors and partial-tile bounds.
Material/TEV completeness and visual fidelity still need in-game validation.

A rebuilt boot-to-match XEX ran scripted Mario vs Fox on Battlefield to match
frame 3900 in Xenia, recording 17 hits, 128 SFX submissions, one SFX miss and
zero music history mismatches without a recorded hang/crash. This is a runtime
regression check, not a listening or visual fidelity check. Its saved trace is
in the ignored `dist/asset-check-20260930/runtime-trace.txt`. The final
`dist/default.xex` uses normal boot and live input and passes `imagexex /DUMP`.

## Original common actions (2026-09-24)


`tools/build_match_xdk.ps1` now also compiles the original shield, dodge,
grab/throw, captured, ledge, teeter, taunt, special-fall and rebound units and
Mario's special moves (`ftmario.c`, `ftmariospecial{n,s,hi,lw}.c`). Their
motion-table entries are extracted with the others; the Mario table fills the
fighter-specific state list and the `ftData_Special*` dispatch slots.

Native glue additions, all over the loaded Battlefield collision lines:

- air/ground collision variants used by these states, including landing
  checks for a released or thrown victim that only have a `CollData`;
- ledge detection from the fighter's original ledge-snap box (`ftData` x44)
  on ledge-flagged floor ends while falling, feeding `ftCliffCommon_80081298`;
- the Wait/Walk edge stop that raises `Collide_Edge` for teeter;
- floor endpoint/distance/connection queries and a segment raycast.

Still missing for these actions: item objects (fireball, cape, tether
items, item throws out of shield), the shield bubble effect, the stick-driven
shield tilt (second animation skeleton), wall-bound ledge edge cases, moving
ledges and non-Mario character hooks, which are traced as
`fighter.unported.*`. A successful ledge grab traces `fighter.cliff.catch`.

### Roster, select and respawn (2026-09-24)

`fighter_glue_xdk.c` keeps per-kind state (`M360LoadedKind`) described by a
roster table: archive names, costume models, motion-state table, OnLoad/
OnDeath and the eight special entries per kind, taken from `ftdata.c` and each
kind's strings. The roster has Mario, Luigi, Dr. Mario, Peach, Yoshi, Bowser,
Donkey Kong, Captain Falcon, Ganondorf, Fox, Falco, Link, Young Link, Zelda,
Sheik, Samus, Pikachu, Pichu, Jigglypuff (default costume only), Ness, Marth,
Roy, Mewtwo and Mr. Game & Watch. Kirby (copy abilities load other kinds'
data) and the Ice Climbers (Nana's partner AI) are not linked.

`match_scene_xdk.c` starts each VS match with a native select phase: left/right
chooses the fighter, X/Y the costume, A confirms, B steps back or returns to
the menu. A second controller joins by pressing a button; otherwise P1 also
picks the CPU. Classic/Adventure keep P1's pick and cycle the CPU through the
roster. This is scaffolding, not the original character select screen.

After a KO, `M360_FighterRebirth` places the fighter at the camera top above
ground point 4 (offset per player) and runs the original rebirth states with
the PlCo platform accessory. Fighter voice/SFX tracks (`ft_0881.c`) now reach
the SSM player through a native `lbAudioAx_80023870`.

Known gaps for the new characters: DK's floor walk (mpLib_80056C54),
Zelda/Sheik transformation, costume hats and Kirby are unported; projectile
items now link (see Item system).

### Item system (2026-09-24)

The match build now also compiles the original item core (`melee/it/*.c`,
with `itzako.c` adapted for the XDK sin/cos macros) and all 166 item kinds,
so fighter projectiles and held items (fireballs, blasters, arrows, bombs,
turnips, eggs, PK fire/thunder, needles, Din's fire, shadow ball, G&W props,
cape, ...) run their original code. `M360_FighterLoad` loads ItCo through
`Item_80266FA8` and initializes the item allocators (`Item_80266FCC`, again at
each match). Items draw on GX link 6, which the match camera now includes.

Native glue supplies blast-zone/stage-scale queries, camera vectors, item
ground-collision variants, fixed ECB setup, line normals, multi-line checks,
one-shot SFX (`lbAudioAx_800237A8`) and `lbArchive_80017040` over the native
archive loader. Item pickup/throw by fighters, random item spawns, Pokemon,
Kirby copies, debug displays, custom item TEV/material state and moving stage
surfaces remain traced stubs. None of this has run on the XDK or Xenia yet.

### CPU AI and stages (2026-09-24)

The original CPU AI (`ftCo_0A01.c`, `ftcmdscript.c`, `ftcpuattack.c`,
`ft_3C61.c`) drives CPU fighters, initialized as VS CPU kind 4 at the selected
level (LB in the select phase, default 3). Its stage model is native:
`M360_FighterBuildIslands` rebuilds the floor islands from connected floor
lines on each stage build, and the `mpCheck*` raycasts test the matching
native lines. Stage-specific AI hooks (hazards, teams) are stubs.

The select phase also picks the stage (D-pad up/down) and stocks (RB). Nine
stages (the first six plus Hyrule Temple, Kongo Jungle 64 and Jungle Japes;
stage collision holds up to 512 lines) load from the map GObj ids their original OnInit creates. Collision groups
bound to map joints (the map GObj's `unk20` GrJoint list and the compiled-in
tables of Final Destination, Fountain of Dreams and Kongo Jungle 64) follow
their joint's world matrix every frame, as `mpLib_80055E9C` does, so
animation-driven platforms such as Randall move their lines. Platforms moved
by stage code (the Fountain of Dreams platforms) stay put, hidden joints keep
their last lines and hazards are not simulated. Grounded fighters ride a
moving floor line (`M360_FighterFollowFloors` keeps their parameter along
the line between frames). Kirby (copy-ability hats are stubs) and Popo (without Nana)
complete the roster. Popo now spawns with Nana as the slot's sub entity (a
hidden roster entry); she is always CPU driven (`Player_8003248C` reports the
sub fighter as CPU) and the original AI follows Popo with `cpu.kind` 6. Nana
is put to sleep when she leaves the blast zone or when Popo loses a stock,
and rejoins on Popo's rebirth.

Zelda and Sheik spawn together like the original Player code: the partner is
created as the slot's sub entity and put to sleep (`ftCo_800BFD04`), and
`Player_GetEntityAtIndex`/`Player_SwapTransformedStates` keep a per-slot
entity table so the original down special (`ftCommon_8007EFC8`) swaps them,
handing over position, damage, collision (`mpCopyCollData`) and held items.
The match scene follows the slot's active entity each frame. Original
`ftCamera_80076064` now initializes/resets the camera subject; `ftcamera.c`
and the original frame callback replace the earlier empty hooks. The global
camera subject manager and framing remain incomplete.

The right stick (up/down) switches the rule between stock and a timed match
(2, 3, 4, 5 or 8 minutes). Timed matches give unlimited lives, credit a KO to
the last fighter that hit the victim (`dmg.x18c4_source_ply`, cleared on
landing as in the original) and subtract one point per fall; the HUD shows a
countdown and each player's score. A tie at time-out starts sudden death:
the tied players respawn with one stock at 300% and the stock rules pick the
winner (no Bob-omb rain).

### Effects (2026-09-24)

The effect system links from the original sources (`melee/ef/*.c`, with
`efasync.c` adapted for MSVC's pointer OR) together with the HSD particle
generator/particle/appsrt code. Each fight runs `efLib_Init` and loads the
common banks (0 and 0x1F), and each fighter loads its bank from
`ftData_UnkBytePerCharacter` at spawn; the match camera draws links 7 and 8.
`lbArchive_80017040` keeps a per-file cache and reports reloads as preloaded,
like the original lbDvd cache, so banks are not relocated twice. JObj-based
effects render through the HSD path. Particles are drawn by a native
`psDispParticles` (`src/xdk/particle_draw_xdk.c`): each particle becomes a
view-space quad (appsrt matrix or position, rotation, size, primary colour
and texture from its texture group, C4/C8 palettes included) submitted to
`M360_HsdDrawParticle`, which reuses the HSD shaders with vertex colour times
texture and alpha or additive blending. Not reproduced yet: particle forms
other than billboards (lines, points, trails), environment colour/TEV modes
and fog.

### Part animations (2026-09-24)

Hand poses and other sub-skeleton animations (`ftData` x1C, set by the
command scripts and by Fox/Falco/Ganon/Yoshi code) now play:
`ftAnim_ApplyPartAnim` adds the part's AnimJoint to the affected joints and
marks them with `flags_b5`, which keeps them out of the motion-end queries.
A motion change re-applies the motion tree to every joint; `ftAnim_80070F28`
then ends transient part anims and `ftAnim_80070E74` restores persistent ones.
The original animates a shadow skeleton and blends it in over a few frames;
here the pose switches immediately.

### Camera shake (2026-09-24)

`Camera_RequestQuake`/`Camera_StopQuake` are native in the match scene: each
quake kind (loop, small, medium, large) runs for the original frame count
(10 or 22) and offsets the eye and interest by a decaying jitter. The original
drives the offset from a stage quake GObj animation (`grLib_801C9CEC`), so the
exact motion differs.

### Hitbox overlay (2026-09-24)

While a match is paused, X toggles a debug overlay drawn on the last effect
link without depth test: hurt capsules in yellow (blue when intangible or
invincible) and active hit capsules in red, from their previous to current
position. Use it in Xenia to check capsule contact against the
`fighter.hit.*` trace events.

### Original match HUD (2026-09-24)

The in-match HUD now comes from the original interface code: `ifall.c`
(IfAll archive, HUD camera and light), `ifstatus.c` (damage panels with the
animated percent digits and their explosion on a KO), `ifstock.c` (stock
icons), `iftime.c` (timer) and `if_2F6E.c`/`if_2F72.c` ("GO!" and panel
graphics). `hud_xdk.c` fills a native VS scene controller from the port's
rules (stock or time, time limit) and ticks its timer from the match frame;
`Player_*` queries read the slot's active fighter (damage, character,
costume, stocks). The HUD camera clears depth before drawing, and GX link 8
(screen-space particles) moved from the stage camera to the HUD camera as
in the original.

The match now follows gmvs.c's sequence: the ScInfCnt countdown runs with
fighter input locked (`ftLib_80086824`), its end releases the fighters and
shows "GO!", the timer counts from there, and a stock-out or time-out plays
"GAME!"/"TIME!" with the scene still animating before the port's winner
overlay. Player pointers (1P-4P/CP) come from ScInfPnm through a native
version of ifnametag.c without SisLib name text. Pausing hides the percent
digits and timer (`ifAll_802F3394`) and shows the original GmPause panel
(`gmpause.c`). Not ported: hazard arrows, coin counters, the offscreen
magnifier bubbles (they need an EFB-to-texture copy) and the results
screen.

### Soak runs in Xenia (2026-09-30)

`tools/soak_xenia.ps1` builds (with `-Build`) or reuses `build-x360\soak.xex`
(`-BootToMatch -InputScript -CallTrace`) and runs each configuration of its
matrix in one headless Xenia instance at a time: every fighter as the
scripted P1 and as the CPU, every stage in a four-CPU free-for-all with
items, a one-minute timed match between level-1 CPUs (sudden death) and
three chained matches. The P1 script cycles ground attacks, tilts, smashes,
aerials, the four specials, shield, rolls, spot/air dodges, grabs with all
throws and a taunt. Results go to `soak-matrix.csv`/`.md` in `-OutDir`
(default `build-x360\soak`). A FREEZE lists the most recent function entries
from the `/Gh` call ring. A run that ends in Xenia's own error box is
reported as CRASH(host) and the process is killed; the harness never sends
window input. Verified so far: the Mario vs CPU Zelda and Dr. Mario vs CPU
Samus runs, which froze before the M43 fixes, reach match frame 7500.

### Host link check

The XDK is not available in every environment. `tools/host_xdk_check/`
contains a clang-based stand-in for the XDK `cl.exe` (32-bit MSVC mode, stub
CRT headers) so `tools/probe_gameplay_xdk.ps1` and `tools/build_match_xdk.ps1`
can run unchanged under PowerShell on Linux. `check_match.sh` compiles the
match objects and reports referenced symbols that neither they, the other XEX
sources (approximated by `external_defined.py`) nor the recorded baseline
define, plus duplicate definitions. It needs `upstream/melee-pc` at the pinned
commit, `pwsh`, `clang` and `llvm-nm`. Passing it means the units compile and
the link is expected to resolve; it is not an XDK build and runs no code.

## Missing integration

- Reproduce movement, jumping, attacks and hitstun in Xenia with recorded
  controller input. Traces now include normalized axes/buttons and P1 motion
  changes; no live Xenia run has verified that the original callbacks respond
  to those inputs. Attack availability still depends on partially stubbed
  fighter helpers.
- Validate hitlag, DI, tumble, landing and recovery behavior in Xenia. The
  damage states (`ftCo_Damage.c`, `ftCo_DamageFall.c`, the Down states) and
  `ftcoll.c` now link from the original sources.
- Verify an actual capsule/hurtbox contact in Xenia and check the new reaction
  trace events (`fighter.hit.reaction_motion`, `fighter.hitstun.frames`); the
  current saved runtime trace predates this change and contains no hit event.
- Match rules (stage, stocks or time, items, CPU level, player count) come from
  the native select phase. The original character/stage select screens and the
  persistent results flow are not integrated. P2 uses a
  connected second controller when available; otherwise a basic CPU that
  approaches, attacks, shields, grabs and recovers is used.
- Stage collision now supports floor/wall checks, downward platform drop-through,
  a basic ceiling crossing stop, ledge grabs and edge teeter. ECB-based
  collision, ledge slips (MissFoot), platform one-way edge cases, moving
  geometry, hazards, items and the original full collision flags remain
  incomplete.
- Quick match reports a winner (or a draw after time-out) and returns to the
  menu with B; connected XInput controllers join as P2-P4. VS setup still
  bypasses the original character/stage select and results persistence.
- Fighter model visibility/parts, material animation, effects, lighting and
  hitboxes need visual and behavioral validation against the original scene.
- Menu leaf screens use bridges for unsupported scenes; they do not implement
  the full original menu lifecycle or data persistence.

## Gameplay compiler probe

Run `tools/probe_gameplay_xdk.ps1` with XEDK configured. It compiles 31 original
translation units independently into the ignored `build-x360/gameplay-probe`
directory, plus a native layout test. It fails if any unit fails. The XEX build
runs this probe and links its layout test, not the fighter gameplay objects.
Original menu input is linked separately as described below.
The probe-only compatibility header preserves static assertions.

Title, menu-animation and fighter Wait units now compile in C. The probe
generates header overlays converting MotionFlags constants to enum constants,
preserving their expressions while making them valid C constant expressions.
Original upstream files are not edited. Predicate, OSCalendarTime and
RETURN_IF are supplied by the compatibility header.

All 31 original compilation probes now pass, including fighter.c, ftcommon, ftanim,
ftaction, ftcoll, ftparts, ftdata, Wait, Walk, Jump, Fall, Dash, Run, Turn,
Landing, Attack1, AttackAir, title and menu, plus ftwalkcommon, ft_081B,
ft_0892, ftchangeparam, mpcoll, mplib, lbcollision, lbvector, gmscene, gm_1A36,
gm_1A3F and mnmain. Indexed castle
offsetof checks are expressed as base offsets plus element/member offsets in
generated overlays; the assertions remain active. Floating-point classification
uses XDK _fpclass, with explicit handling of float subnormals before promotion.
Compilation is not linking or execution of a fighter; its gameplay dependencies
still need to be integrated.

Generated source adaptations hoist declarations for the old C frontend and
explicitly cast native relocated animation-table slots. Upstream sources are
not changed. Compiler warnings still require review before runtime integration.

Run `tools/audit_gameplay_symbols.ps1` after the probe and runtime build to
inventory references absent from their object definitions. It writes the current
unresolved-symbol count and requesting modules to
`build-x360/gameplay-probe/unresolved-symbols.json` with requesting modules.
This conservative object-level inventory includes SDK/library references and
unreachable functions; it is not a linker test or a count of runtime failures.
It also assumes the existing runtime objects are current. No missing gameplay
functions have been replaced with success-returning stubs. The audit now reads
the successful compilation manifest rather than every old object in the folder.
Interrupted or failed probes invalidate that manifest.

## Verified native fighter layout slice

The XDK gave the mixed u8/u16 animation bitfield at Fighter+0x596 separate
allocation units, placing x598 at 0x59C and input at 0x624. A generated header
overlay uses a single u16 allocation unit. Explicit assertions now check x598
at 0x598, input at 0x620, key movement fields and selected disc-record sizes.
They do not depend on the legacy disabled ASSERT_SIZE macro. This is not a
validation of the entire Fighter structure or every bitfield.

`gameplay_layout_probe.c` also checks the integer/bitfield animation-flag aliases
in the XEX. The latest Xenia trace reports `gameplay.layout: 1`, then successful
boot and Present at frame 120. This is a target compatibility test, not running
fighter state transitions or combat. No new playable scene is claimed.

## Original menu input integration

The XEX now initializes and runs the original gm_1A36 controller mapper after
HSD_PadRenewStatus each frame. The generated compilation unit preserves the
source prefix verbatim, omitting only gm_801A3EF4 (initialization of all game
modes). Start detection uses gm_GetButtonsTriggered's combined four-port slot.
Start still logs `input.start.pending_scene`; it does not enter mnmain yet.

The target-side test reports `menu.input.tests: 31` (all five checks passed):
Start-to-confirm mapping, held input without a second trigger, cancellation
from port four, direction mapping and held-direction repeat. It restores the
copied pad state and resets the mapper before processing live input.

Although mnmain and gmscene compile, their full lifecycle is not running. The
object inventory currently finds 113 mnmain references absent from the inspected
objects, including text, graphics, archive/audio and submenu services. Some may
be library-supplied or unreachable; this is not an executable linker test.
The existing OSContext shim is not a real Dolphin thread context and must not
be treated as a working implementation of gmscene's OS/thread services.

The XEX now invokes original `mn_8022ED6C` and `mn_8022F298` for title animation
looping. The build extracts these two function bodies verbatim from mn_22EC.c
into an ignored translation unit, because unrelated functions in that file
require scene and menu services which are not linked yet. This does not mean
the full menu or title scene is running.

## Playability checks

A playable milestone must load an original stage and fighter, advance original
fighter states, respond to movement and attacks, and resolve collision through
the ported game code. Visual verification in Xenia and guest diagnostics must
agree. A title image, moving mesh or custom combat sandbox does not satisfy it.
