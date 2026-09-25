/* Native game-rule state for the original in-match HUD (melee/if: ifall.c,
 * ifstatus.c, ifstock.c, iftime.c, if_2F6E.c, if_2F72.c). The original reads
 * the VS scene controller built by gmvs.c; the port fills one from its own
 * match rules and ticks its timer fields from the match frame count. */
#include <stdlib.h>
#include <string.h>

#include <melee/gm/types.h>
#include <melee/gr/ground.h>
#include <melee/gr/stage.h>
#include <melee/gr/types.h>
#include <melee/if/ifall.h>
#include <melee/if/ifstatus.h>
#include <melee/if/iftime.h>
#include <melee/lb/types.h>
#include <melee/mn/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/lb/lbarchive.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>

#include "match_xdk.h"

void ifStatus_802F6EA4(int arg0, int arg1, int arg2, int arg3, Event arg4, Event arg5);
void ifStatus_802F6D10(s32 player_idx);

void ftLib_80086824(void);
void fn_801A1134(void);
void gm_801A0FEC(s32 slot, u8 flag);
void gm_801A10FC(int slot);
void ftLib_800868A4(void);

static VsSceneController s_controller;
static int s_hudActive;
static unsigned s_hudTotalFrames;
/* Countdown/"GO!" and "GAME!"/"TIME!" sequencing (gmvs.c fn_8016B7F8,
 * fn_8016B784, ifStatus_802F7034). */
static int s_fightStarted;
static unsigned s_fightStartFrame;
static unsigned s_lastFrame;
static int s_endShown;
static int s_endDone;

StartMeleeRules* gm_GetStartMeleeRules(void)
{
    return &s_controller.start;
}

VsSceneController* gmVs_GetSceneController(void)
{
    return &s_controller;
}

VsSceneState* gmVs_GetSceneState(void)
{
    return &s_controller.state;
}

/* gm_1601.c: HUD panel colour index and palette. */
u32 gm_80160854(u8 slot, u8 team, u8 is_teams, u8 slot_type)
{
    static const u8 kTeamHuman[5] = { 0, 1, 3, 2, 4 };
    static const u8 kTeamCpu[5] = { 5, 6, 8, 7, 4 };
    if (is_teams)
        return team < 5 ? (slot_type == Gm_PKind_Human ? kTeamHuman[team] : kTeamCpu[team]) : 0;
    if (slot_type != Gm_PKind_Human)
        return 4;
    return slot < 4 ? slot : 0;
}

GXColor gm_80160968(u32 arg0)
{
    static const GXColor kColors[9] = {
        { 0x99, 0x1A, 0x1A, 0xFF }, { 0x33, 0x33, 0x80, 0xFF }, { 0x80, 0x66, 0x00, 0xFF },
        { 0x1A, 0x66, 0x1A, 0xFF }, { 0x66, 0x66, 0x66, 0xFF }, { 0x66, 0x4D, 0x4D, 0xFF },
        { 0x4D, 0x4D, 0x66, 0xFF }, { 0x66, 0x59, 0x33, 0xFF }, { 0x4D, 0x66, 0x4D, 0xFF },
    };
    return kColors[(u8) arg0 < 9 ? (u8) arg0 : 0];
}

void gm_8016895C(HSD_JObj* jobj, DynamicModelDesc* desc, int idx)
{
    DiscU32* anims = DP(DiscU32, desc->anims);
    DiscU32* matanims = DP(DiscU32, desc->matanims);
    DiscU32* shapeanims = DP(DiscU32, desc->shapeanims);
    HSD_JObjAddAnimAll(jobj,
                       anims ? (HSD_AnimJoint*) (uintptr_t) anims[idx].v : NULL,
                       matanims ? (HSD_MatAnimJoint*) (uintptr_t) matanims[idx].v : NULL,
                       shapeanims ? (HSD_ShapeAnimJoint*) (uintptr_t) shapeanims[idx].v : NULL);
}

/* Stock/emblem icon frame for a character and costume (gm_1601.c). */
f32 gm_80168B34(CharacterKind ckind, int fkind, int costume)
{
    int base;
    if (ckind == CKind_GKoops)
        return 58.0f;
    if (ckind == CKind_Boy || ckind == CKind_Girl)
        return 26.0f;
    if (ckind == CKind_MasterH)
        return 28.0f;
    if (ckind == CKind_CrezyH)
        return 27.0f;
    if (ckind == CKind_Zelda || ckind == CKind_Seak)
        base = fkind == Ft_Kind_Seak ? 0x19 : 0x12;
    else if (ckind == ChKind_Sandbag)
        return 59.0f;
    else if (ckind == ChKind_Popo)
        base = 0xE;
    else if (ckind > CKind_Seak)
        base = ckind - 1;
    else
        base = ckind;
    return (f32) (base + costume * 30);
}

float gm_80168BF8(int slot)
{
    return gm_80168B34(Player_GetPlayerCharacter(slot), Player_80036394(slot),
                       (int) Player_GetCostumeId(slot));
}

/* 1P, multi-man, coin and team-count queries: none apply to the port's VS. */
s32 gm_80169394(void) { return 0; }
int gm_801694A0(HSD_GObj* gobj) { (void) gobj; return 0; }
UNK_T gm_80169520(void) { return NULL; }
UNK_T gm_80169530(void) { return NULL; }
bool gm_8016A1F8(void) { return false; }
bool gm_8016A944(void) { return false; }
bool gm_IsMultimanSmashMode(void) { return false; }

static u8 s_gm8046B668[0x40];

UNK_T gm_8016A97C(void)
{
    return s_gm8046B668 + 0x1C;
}

struct lbl_8046B668_t* gm_8016A98C(void)
{
    return (struct lbl_8046B668_t*) s_gm8046B668;
}

s32 gm_8016AEC8(void)
{
    return s_controller.start.sd_penalty;
}

u32 gm_8016AEEC(void)
{
    return s_controller.state.timer_seconds;
}

u16 gm_8016AF0C(void)
{
    u16 centi = (u16) (99.0f * s_controller.state.unk_2C / 59.0f);
    if (!s_controller.start.timer_counts_up)
        centi = (u16) (99 - centi);
    return centi;
}

bool gm_8016B110(void)
{
    return s_controller.start.timer_counts_up;
}

bool gm_8016B184(void)
{
    return s_controller.state.is_singleplayer == 1;
}

bool gm_8016B1A8(void)
{
    return s_controller.start.x9 == 1;
}

void gm_8016B8D4(s32 slot, u8 slot_type)
{
    if (slot >= 0 && slot < GM_MAX_PLAYERS) {
        s_controller.state.fighters[slot].x1 = 0x82;
        s_controller.state.fighters[slot].slot_type = slot_type;
    }
}

void Ground_801C1DE4(s32* a, s32* b)
{
    *a = stage_info.x6D4;
    *b = stage_info.x6D2;
}

/* pc/widescreen: the Xbox output keeps the original 4:3 HUD layout. */
int pc_get_hud_mode(void) { return 0; }
float pc_widescreen_hud_player_x(int i, int n, float x) { (void) i; (void) n; return x; }
float pc_widescreen_hud_timer_x(float x) { return x; }

/* lbspdisplay.c: HUD camera from its scene description. */
HSD_CObj* lb_80013B14(HSD_CameraDescPerspective* desc)
{
    HSD_CObj* cobj = HSD_CObjLoadDesc((HSD_CObjDesc*) desc);
    Scissor scissor;
    if (HSD_CObjGetProjectionType(cobj) == 1 && HSD_CObjGetAspect(cobj) == 1.18f)
        HSD_CObjSetAspect(cobj, 1.2173333f);
    HSD_CObjGetScissor(cobj, &scissor);
    if (scissor.right > 640)
        scissor.right = 640;
    if (scissor.bottom > 480)
        scissor.bottom = 480;
    HSD_CObjSetScissor(cobj, &scissor);
    return cobj;
}

/* Name tags, hazard arrows, coin/bonus counters and the offscreen magnifier
 * are not ported; their show/hide/init hooks from ifall.c are no-ops. */
/* ---- Player pointers (ifnametag.c without the SisLib name text) ----
 * ScInfPnm_scene_models frames: 0-15 are 1P-4P in four colours, 16-19 CP.
 * The pointer shows during the countdown and whenever ftLib_80086F4C says
 * the fighter asks for it, at name_tag_height above the fighter. */
static const float kTagFrame[4] = { 0.0f, 5.0f, 10.0f, 15.0f };
static HSD_WObjDesc s_tagEye = { 0, { 0.0f, 0.0f, 300.0f }, 0 };
static HSD_WObjDesc s_tagInterest = { 0, { 0.0f, 0.0f, 0.0f }, 0 };
static HSD_CameraDescFrustum s_tagCamDesc;
static HSD_GObj* s_tagCamera;
static HSD_GObj* s_tags[GM_MAX_PLAYERS];
static u8 s_tagSlot[GM_MAX_PLAYERS];
static int s_tagsForced;
static int s_tagsHidden;

HSD_GObj* Camera_80030A50(void);
bool ftLib_80086F4C(HSD_GObj* gobj);
float ftLib_80086F80(HSD_GObj* gobj);
Vec3* lbVector_WorldToScreen(HSD_CObj* cobj, const Vec3* pos3d, Vec3* screenCoords, int d);

static void TagCameraRender(HSD_GObj* gobj, int pass)
{
    (void) pass;
    if (ifAll_IsHUDHidden() || s_tagsHidden)
        return;
    if (HSD_CObjSetCurrent(GET_COBJ(gobj))) {
        HSD_CObjEraseScreen(GET_COBJ(gobj), 0, 0, 1);
        HSD_GObj_80390ED0(gobj, 7);
        HSD_CObjEndCurrent();
    }
}

/* fn_802FCC44 */
static void TagProc(HSD_GObj* gobj)
{
    const int slot = *(u8*) gobj->user_data;
    HSD_JObj* jobj = gobj->hsd_obj;
    HSD_GObj* fighter = Player_GetEntity(slot);
    HSD_GObj* camera = Camera_80030A50();
    Fighter* fp = fighter ? GET_FIGHTER(fighter) : NULL;
    Vec3 pos, screen;
    if (!fp || !camera || fp->x221F_b3 || fp->invisible ||
        (s_controller.start.is_stock && !Player_GetStocks(slot)) ||
        !(s_tagsForced || ftLib_80086F4C(fighter))) {
        HSD_JObjSetFlags(HSD_JObjGetChild(jobj), JOBJ_HIDDEN);
        return;
    }
    HSD_JObjClearFlags(HSD_JObjGetChild(jobj), JOBJ_HIDDEN);
    pos = fp->cur_pos;
    pos.y += ftLib_80086F80(fighter) - 2.5f;
    lbVector_WorldToScreen(GET_COBJ(camera), &pos, &screen, 0);
    HSD_JObjSetTranslateX(jobj, screen.x);
    HSD_JObjSetTranslateY(jobj, -screen.y);
}

static void TagUserDataKeep(void* data)
{
    (void) data;
}

static void TagsCreate(void)
{
    DiscU32* models = NULL;
    DynamicModelDesc* d;
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matanim;
    HSD_ShapeAnimJoint* shapeanim;
    int i;
    memset(s_tags, 0, sizeof(s_tags));
    s_tagCamera = NULL;
    lbArchive_LoadSections(*ifAll_GetArchive(), (void**) &models, "ScInfPnm_scene_models", 0);
    if (!models || !(d = DP(DynamicModelDesc, models[0].v)))
        return;
    joint = DP(HSD_Joint, d->joint);
    anim = d->anims ? (HSD_AnimJoint*) (uintptr_t) DP(DiscU32, d->anims)[0].v : NULL;
    matanim = d->matanims ? (HSD_MatAnimJoint*) (uintptr_t) DP(DiscU32, d->matanims)[0].v : NULL;
    shapeanim = d->shapeanims ? (HSD_ShapeAnimJoint*) (uintptr_t) DP(DiscU32, d->shapeanims)[0].v : NULL;
    memset(&s_tagCamDesc, 0, sizeof(s_tagCamDesc));
    s_tagCamDesc.projection_type = 3;
    s_tagCamDesc.viewport.xmax = 640;
    s_tagCamDesc.viewport.ymax = 480;
    s_tagCamDesc.scissor.right = 640;
    s_tagCamDesc.scissor.bottom = 480;
    s_tagCamDesc.eyepos = &s_tagEye;
    s_tagCamDesc.interest = &s_tagInterest;
    s_tagCamDesc.nnear = 0.1f;
    s_tagCamDesc.ffar = 32768.0f;
    s_tagCamDesc.top = 0.0f;
    s_tagCamDesc.bottom = -480.0f;
    s_tagCamDesc.left = 0.0f;
    s_tagCamDesc.right = 640.0f;
    s_tagCamera = GObj_Create(0xE, 15, 0);
    HSD_GObjObject_80390A70(s_tagCamera, HSD_GObj_CameraKind,
                            lb_80013B14((HSD_CameraDescPerspective*) &s_tagCamDesc));
    GObj_SetupGXLinkMax(s_tagCamera, TagCameraRender, 6);
    s_tagCamera->gxlink_prios = 0x200;
    for (i = 0; i < 4; ++i) {
        HSD_GObj* gobj;
        HSD_JObj* jobj;
        if (Player_GetPlayerSlotType(i) == Gm_PKind_NA)
            continue;
        gobj = GObj_Create(HSD_GOBJ_CLASS_UI, 15, 0);
        jobj = HSD_JObjLoadJoint(joint);
        HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, jobj);
        GObj_SetupGXLink(gobj, HSD_GObj_JObjCallback, 9, 0);
        HSD_JObjSetScaleX(jobj, 10.0f);
        HSD_JObjSetScaleY(jobj, 10.0f);
        HSD_JObjSetScaleZ(jobj, 10.0f);
        HSD_JObjAddAnimAll(jobj, anim, matanim, shapeanim);
        HSD_JObjReqAnimAll(jobj, Player_GetPlayerSlotType(i) == Gm_PKind_Human ? kTagFrame[i] : 18.0f);
        HSD_JObjAnimAll(jobj);
        s_tagSlot[i] = (u8) i;
        GObj_InitUserData(gobj, 0, TagUserDataKeep, &s_tagSlot[i]);
        HSD_GObj_SetupProc(gobj, TagProc, 17);
        s_tags[i] = gobj;
    }
}

/* ifall.c show/hide hooks for the pointers (un_802FD450/45C) and their
 * teardown (un_802FD468). */
void un_802FD450(void) { s_tagsHidden = 1; }
void un_802FD45C(void) { s_tagsHidden = 0; }
void un_802FD468(void)
{
    int i;
    for (i = 0; i < GM_MAX_PLAYERS; ++i)
        if (s_tags[i]) {
            HSD_GObjFree(s_tags[i]);
            s_tags[i] = NULL;
        }
    if (s_tagCamera)
        HSD_GObjFree(s_tagCamera);
    s_tagCamera = NULL;
}
void un_802FD4C8(void)
{
    s_tagsForced = 0;
    s_tagsHidden = 0;
    TagsCreate();
}
void un_802FD668(void) {}
void un_802FD674(void) {}
void un_802FD704(void) {}
void un_802FD90C(void) {}
void un_802FD910(void) {}
void un_802FD91C(void) {}
void un_802FE260(void) {}
void un_802FE390(void) {}
void un_802FF190(void) {}
void un_802FF1B4(void) {}
void un_802FF364(s32 slot) { (void) slot; }
void un_802FF498(void) {}
void un_802FF4FC(void) {}
void un_802FF570(void) {}
void un_802FF620(void) {}
void ifMagnify_802FC750(void) {}
void ifMagnify_802FC870(void) {}
void ifMagnify_802FC8E8(void) {}
void ifMagnify_802FC940(void) {}

/* fn_802F36B8 (ifall.c) plus a depth clear: the HUD camera draws over the
 * finished 3D scene, and the port's depth buffer still holds that scene. */
static void HudCameraRender(HSD_GObj* gobj, int pass)
{
    (void) pass;
    if (ifAll_IsHUDHidden())
        return;
    if (HSD_CObjSetCurrent(GET_COBJ(gobj))) {
        HSD_CObjEraseScreen(GET_COBJ(gobj), 0, 0, 1);
        HSD_GObj_80390ED0(gobj, 0x7);
        HSD_CObjEndCurrent();
    }
}

/* End of "GO!" (fn_8016B784). */
static void HudGoDone(int idx)
{
    (void) idx;
    s_controller.state.hud_enabled = 1;
}

/* End of the "3, 2, 1" countdown (fn_8016B7F8): release the fighters and
 * show "GO!". */
static void HudCountdownDone(int idx)
{
    (void) idx;
    ftLib_800868A4();
    s_tagsForced = 0;
    s_fightStarted = 1;
    s_fightStartFrame = s_lastFrame;
    ifStatus_802F6EA4(4, -1, -1, 0, NULL, (Event) HudGoDone);
    M360_MatchTrace("hud.go", s_lastFrame);
}

static void HudEndDone(int idx)
{
    (void) idx;
    s_endDone = 1;
}

/* gmvs.c match start (fn_8016E730 + gm_Scene_Vs_OnEnter): load IfAll, build
 * the HUD camera/light and panels, show "GO!", create the timer and one
 * damage/stock panel per slot. */
void M360_HudStart(unsigned slots, unsigned stocks, unsigned timeSeconds)
{
    StartMeleeRules* r = &s_controller.start;
    memset(&s_controller, 0, sizeof(s_controller));
    r->match_kind = timeSeconds ? MatchKind_Time : MatchKind_Stock;
    r->is_stock = !timeSeconds;
    r->timer_enabled = timeSeconds != 0;
    r->time_limit = timeSeconds;
    r->x2_6 = 1;
    r->x0_3 = 4;
    r->sd_penalty = -1;
    r->is_vs = 1;
    s_controller.state.hud_enabled = 1;
    s_controller.state.timer_seconds = timeSeconds;
    s_hudTotalFrames = timeSeconds * 60u;
    (void) slots;
    (void) stocks;
    ifAll_802F390C();
    if (ifAll_GetHUDGObj())
        ifAll_GetHUDGObj()->render_cb = HudCameraRender;
    s_fightStarted = 0;
    s_fightStartFrame = 0;
    s_lastFrame = 0;
    s_endShown = 0;
    s_endDone = 0;
    s_tagsForced = 1;
    /* Fighters ignore input until the countdown ends (ftLib_80086824). */
    ftLib_80086824();
    ifStatus_802F6EA4(3, -1, -1, 0, NULL, (Event) HudCountdownDone);
    ifTime_CreateTimers();
    ifStatus_802F665C(4);
    /* gmvs.c fn_8016E730: the GmPause panel, hidden until a pause. */
    fn_801A1134();
    s_hudActive = 1;
    M360_MatchTrace("hud.start", timeSeconds);
}

void M360_HudFrame(unsigned frame)
{
    unsigned elapsed;
    if (!s_hudActive)
        return;
    s_lastFrame = frame;
    s_controller.state.frame_count = frame;
    elapsed = M360_HudFightFrames();
    if (s_hudTotalFrames) {
        const unsigned left = elapsed < s_hudTotalFrames ? s_hudTotalFrames - elapsed : 0;
        s_controller.state.timer_seconds = left / 60u;
        s_controller.state.unk_2C = (u16) (left ? elapsed % 60u : 0);
    }
}

/* Frames of play since "GO!" (0 while the countdown runs, or always the
 * match frame when the HUD is not active). */
unsigned M360_HudFightFrames(void)
{
    if (!s_hudActive)
        return s_lastFrame;
    return s_fightStarted && s_lastFrame >= s_fightStartFrame ? s_lastFrame - s_fightStartFrame : 0;
}

int M360_HudFightStarted(void)
{
    return !s_hudActive || s_fightStarted;
}

/* Match end (ifStatus_802F7034): "TIME!" on a time-out, "GAME!" otherwise;
 * the fighters stop taking input. */
void M360_HudGameEnd(int timeout)
{
    if (!s_hudActive || s_endShown) {
        s_endDone = 1;
        return;
    }
    s_endShown = 1;
    s_controller.state.match_result = timeout ? OUTCOME_TIMEOUT : 0;
    ftLib_80086824();
    ifStatus_802F6EA4(timeout ? 0 : 5, -1, -1, 0, NULL, (Event) HudEndDone);
    M360_MatchTrace("hud.game_end", (unsigned) timeout);
}

/* gm_DoPauseChecksAndRoutine / gm_DoUnpauseChecksAndRoutine: hide the
 * percent digits and timers and show the GmPause panel for the pauser with
 * its L+R+A+START, Z and stick hints. */
void M360_HudPause(int paused, int slot)
{
    if (!s_hudActive)
        return;
    if (paused) {
        ifAll_802F3394();
        gm_801A0FEC(slot, 1 | 2 | 4);
    } else {
        ifAll_802F33CC();
        gm_801A10FC(slot);
    }
}

int M360_HudGameEndDone(void)
{
    return !s_hudActive || s_endDone;
}

/* A lost stock: the original gm_80167320 path explodes the percent and
 * removes a stock icon. */
void M360_HudStockLost(unsigned slot)
{
    if (s_hudActive)
        ifStatus_802F6D10((s32) slot);
}

void M360_HudStop(void)
{
    if (s_hudActive)
        ifAll_802F3A64();
    s_hudActive = 0;
}
