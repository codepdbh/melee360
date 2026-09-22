# Real main menu plan

Research notes for replacing the placeholder MAIN MENU screen with the original
Melee main menu. Upstream paths are relative to `upstream/melee-pc/src/`.
The XEX parses the menu archive alongside the title archive, resolves 83/83
requested symbols, constructs 20 model trees with 351 joints and submits 2,982
projected vertices from the three main-scene models in Xenia. The original `mnmain` input and scene lifecycle,
texture animation, effects and visual fidelity remain pending.

## Scene flow

- Title exit: `gm/gmtitle.c` `gm_Scene_Title_OnFrame` (~241-290). Start plays
  SFX, stores buttons in exit data and calls `gm_801A4B60()`. 600 idle frames
  exit with 0, which leads to the opening movie.
- `gm/gmtitlemode.c:39-69` `onExit` calls `gm_SetPendingGameMode(GM_MENU)`.
- GM_MENU (major 1) is `gm_Mode_Menu_States` (`gm/gmmenumode.c:45-58`,
  registered at `gm/gmscdata.c:550`): preload `lbDvdPreload_2`, `onEnter`
  (73-240, allocates lbCardNew/lbSnap work, picks `menu_kind` and
  `hovered_selection`, default `MENU_KIND_MAIN`/`SEL_MAIN_1P`,
  `load_assets=1`), `onExit` (243-248).
- Scene `GS_MENU` (`gm/gmscdata.c:73-78`) is
  `{mnMain_Scene_OnFrame, mnMain_Scene_OnEnter, NULL, NULL}`. Drawing is done by
  GObj GX links only.
- Driver: `gm/gm_1A3F.c:156-220` (`gm_801A4014`); per-frame loop
  `gm/gmscene.c:281-395`: on_frame, `HSD_GObj_RunProcs`, `HSD_StartRender`,
  `HSD_GObj_80390FC0`, `HSD_VICopyXFBAsync`.
- `mnMain_Scene_OnEnter` (`mn/mnmain.c:2767-3057`) loads archives, then creates
  lights (`lb_80011AC4(MenMain_lights)`, 2750-2765), fog (`mn_8022BCF8`, 1612),
  two cameras and two SIS text contexts (`mn_8022BE34`/`mn_8022BEDC`,
  1649-1695), background (`mn_80229B2C`, 788), panel (`mn_80229DC0`, 875), a
  think GObj running `mn_803EB6B0[cur].think` (default `mn_8022DB10`), options
  (`mn_8022B3A0(0)`, 1391), and music via
  `lbAudioAx_80023F28(gmMainLib_8015ECB0())`.
- Per frame: `mnMain_Scene_OnFrame` (2732-2748) only handles L+R+Start (forces a
  return to the main menu from submenus). Main input is `mn_8022DB10`
  (2658-2730): confirm opens a submenu via `mn_8022B3A0(1)`, B sets
  `pending_mode = GM_TITLE`, Up/Down call `sfxMove`. Animation/description
  upkeep: `fn_8022AFEC` (1194-1370). No idle timeout back to the title was found
  in mnmain (not fully certain).

## Assets on the GALE01 disc

`lbFileGetFullName` (`lb/lbfile.c:60-95`) appends `.usd` for US language, so
`"MnMaAll"` resolves to `MnMaAll.usd`.

| File | Size | Public roots | Use |
|---|---|---|---|
| MnMaAll.usd | 2,155,311 | 234 | main menu |
| SdMenu.usd | 87,557 | `SIS_MenuData` | `HSD_SisLib_803A62A0(0, ...)` |
| SdToy.dat | 37,096 | `SIS_ToyData` | SIS slot 3 |
| GmEvent.dat | 10,820 | `sqEventInitDataLevelTbl` | `gm_801BA8FC` (`gm/gmevent.c:220`) |
| LbAd.dat | 14,488 | `lbAudioLoadData` | `lbAudioAx_8002392C` (`lb/lbaudio_ax.c:277`) |

`MnExtAll` is character select / tournament only. `lbArchive_LoadSymbols`
(`mnmain.c:2792-2960`) requests 83 symbols, all present: 20 model sets
(`_Top_joint`, `_Top_animjoint`, `_Top_matanim_joint`,
`_Top_shapeanim_joint`: MenMainBack, Panel, ConTop, Cursor, ConRl, CursorRl,
NmRl, CursorTr01-04, CursorRl01-05, ConIs, CursorIs, ConSs, CursorSs) plus
`ScMenMain_cam_int1_camera`, `ScMenMain_scene_lights`, `ScMenMain_fog`.

- Camera: perspective, fov 41.54, aspect 1.333, near 1, far 5000, eye (0,0,51)
  looking at the origin, 640x480 viewport.
- Fog: type 2, 60-250, colour #000019 (also the erase colour).
- Lights: 3 animated (ambient #4040B3, point #CCCCCC and #B3B3B3, flags 0xE).
- Scene graph: 351 joints (171 XLU), 189 DObj/MObj, 192 PObj (18
  envelope-skinned), 184 TObj (58 TObj-TEV blocks, 3 two-texture MObjs), 55 PE
  descriptors. Texture formats: I4 98, I8 43, IA4 34, CI4 5, CI8 3, CMPR 1.
  Main render modes `0x6000001C` (79) and `0x60000011` (69).
- Option labels (1P/VS/...) are textures swapped by TObj animation
  (`mn_8022F3D8(jobj, 0xC..0x13, TOBJ_MASK)`, `mnmain.c:1484-1491`,
  `mn_22EC.c:279`). Only the description line is SIS text (`mnmain.c:760-779`,
  indices `mn_803EB660 = {0x81..0x85}`).
- Font glyphs live in `main.dol` (`HSD_SisLib_FontAtlas[287]`, 512 B I4 each,
  `sysdolphin/baselib/sislib_font.h`); melee-pc extracts them by signature scan
  in `pc/discfont.c:146-162`.
- Menu SFX `sfxMove/Forward/Back` -> `lbAudioAx_80024030(2/1/0)`
  (`mn/inlines.h:52-65`) -> ids 0xAE/0xAD/0xAC (`lbaudio_ax.c:449-466`),
  probably in `audio/us/main.ssm` (unverified).

## Rendering gaps versus the title renderer

The title renderer (`src/xdk/melee_title_scene_xdk.cpp`) decodes display lists
and textures, projects on the CPU from the camera descriptor, binds the first
TObj per material and uses material diffuse x alpha. Stubbed today:
`HSD_JObjDisp/DispSub/MakePositionMtx/CObjGetCurrent` (`hsdjobj_xdk.cpp`),
CObj/LObj/Fog hooks (`gobj_xdk.cpp`), `HSD_TObjAnimAll` (`hsdanim_xdk.cpp`),
single-archive `melee_archive_xdk.cpp`.

| Need | Upstream (lines / GX calls) | Plan |
|---|---|---|
| CObj load, eye/interest, erase, current camera | `cobj.c` (1448 / 13) | port math, GX -> D3D |
| LObj load and animation (`mn_8022C068`) | `lobj.c` (1080 / 25) | port data/anim, light in shader |
| Fog | `fog.c` (259 / 10) | port, linear fog in D3D |
| TObj texture animation (labels) | `tobj.c` (1699 / 20) | port non-GX part; labels are wrong without it |
| MatAnim | `mobj.c` (645 / 1) | extend `M360_MObjUpdate` |
| TEV, multi-texture | `tev.c` (549 / 19) | a few pixel-shader modes |
| JObjDisp, XLU sort | `displayfunc.c` (614 / 37) | native `HSD_JObjDisp` emitting D3D draws |
| Envelope skinning | `pobj.c` (1320 / 29) | port |
| SIS text | `sislib.c` (614 / 0), `hsd_3A76.c` (1138 / 35) | small GX shim + DOL font extraction |

## Unresolved references (113 in the probe audit)

About 62 are artefacts or already provided: mnmain's own globals (22), GObj/pad
globals already linked (9), CRT/compiler helpers (~31). Real work:
CObj/fog/LObj (12, medium), SIS (6, medium), archive/language/GmEvent (3,
small: `lbarchive.c:160` over gcm, lbHeap -> HSD_MemAlloc), lbspdisplay and
`lb_8000B1CC` (4, small), audio (5, map to HPS streaming + SFX hook), save/game
state (12, native stand-ins over a default US save), leaf submenus (17, large,
out of scope: traced "not ported" stand-ins that stay in the menu, never
success stubs). No OS threads are needed if the gmscene loop and async lbDvd
preload are bypassed.

## Recommended approach

Link the original `mnmain.c` and all of `mn_22EC.c` (they already compile), use
the real baselib data/animation code, replace GX with a native D3D9 backend,
drive the scene with a small native driver instead of `gm_801A4D34`, and use
native stand-ins for save/card/audio/leaf submenus. Skip gmmenumode's
lbCardNew/lbSnap work by calling `mnMain_Scene_OnEnter` with
`MenuEnterData{0,0,1}`.

1. P0, assets: multi-archive relocation and 83/83 public-symbol resolution
   are running in Xenia (`menu.archive.symbols: 83`). Next: provide the original
   `lbArchive_LoadSymbols`/language/heap boundary and validate the loaded JObj
   counts before calling the menu scene.
2. P1, static real menu: the XEX now loads the JObj trees and translates their
   geometry through the title renderer using the menu camera descriptor. Next:
   inspect the visible result and replace the native preview path with the
   original `mnMain_Scene_OnEnter` lifecycle and GX-facing rendering hooks.
3. P2, faithful look: TObj animation, MatAnim, LObj lighting, fog, XLU sort,
   skinning, TEV modes.
4. P3, navigation: `mn_8022DB10` and submenu thinks on the gm_1A36 mapper, B
   back to title, SFX hook, `gmMainLib_8015ECB0` music via HPS, leaf entries
   trace `menu.leaf.unported`. Self-test: inject Down x2 + A and assert
   `mn_804A04F0` state.
5. P4, text: `sislib.c` + `hsd_3A76.c` with a GX shim, DOL font extraction,
   SdMenu/SdToy loading.
6. P5, menu SFX through the synth SSM path.

Risks: JObjDisp/TEV fidelity and XLU order, DOL font signature scan, keeping
all loads synchronous, heap headroom (24 MB arena, MnMaAll ~2.1 MB), 32-bit
`DP()` pointer discipline. Uncertain: which SSM bank holds the menu SFX, the
Rl/Is/Ss/Tr naming, any idle behaviour outside mnmain.
