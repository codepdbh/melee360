#include <math.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/gr/types.h>
#include <melee/mp/types.h>
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#pragma warning(pop)

#include "match_xdk.h"
#include "melee_classic_matchups.h"
#include "melee_adventure_matchups.h"

typedef union AnimFn {
    void (*fn)(void);
    void* ptr;
} AnimFn;

static void* AnimCallback(void (*fn)(void))
{
    AnimFn u;
    u.fn = fn;
    return u.ptr;
}

void HSD_GObj_JObjCallback(HSD_GObj* gobj, int arg1);
void HSD_GObj_LObjCallback(HSD_GObj* gobj, int unused);
void HSD_GObj_80390ED0(HSD_GObj* gobj, u32 mask);
void HSD_GObj_80390FC0(void);
void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);
void it_8026D018(void);
s32 HSD_Randi(s32 max);
void HSD_GObj_RunProcs(void);
void M360_StageSetKind(int grkind);
void M360_FighterHangInfo(void* gobj, unsigned* out);
int M360_InputScriptHolding(void);
unsigned M360_HeapLargestFree(void);
unsigned M360_HeapUsed(void);
void M360_AudioStopAllSfx(void);
void M360_HsdRenderClearTextures(void);

volatile unsigned g_m360Crumb;
volatile unsigned g_m360CrumbDetail;

void M360_MatchHangInfo(unsigned* out)
{
    HSD_GObjProc* proc = HSD_GObj_CurrentInvokedProc;
    out[0] = g_m360Crumb;
    out[1] = proc ? (unsigned) (uintptr_t) proc->on_invoke : 0;
    out[2] = proc && proc->gobj ? proc->gobj->p_link : 99;
    out[3] = g_m360CrumbDetail;
    out[4] = out[5] = out[6] = out[7] = out[8] = out[9] = out[10] = out[11] = 0;
    if (proc && proc->gobj && proc->gobj->p_link == 8)
        M360_FighterHangInfo(proc->gobj, &out[4]);
}

static unsigned DObjChainLength(HSD_DObj* d)
{
    unsigned n = 0;
    while (d && n < 1000) {
        d = d->next;
        ++n;
    }
    return n;
}

extern char it_mobj[];

static void TraceClassChain(HSD_ClassInfo* c)
{
    int i;
    for (i = 0; c && i < 6; ++i, c = c->head.parent) {
        M360_MatchTrace("hang.class.ptr", (unsigned) (uintptr_t) c);
        M360_MatchTrace("hang.class.size", (unsigned) c->head.obj_size);
        M360_MatchTrace("hang.class.flags", (unsigned) c->head.flags);
    }
}

void M360_MatchHangItems(void)
{
    HSD_GObj* g;
    TraceClassChain((HSD_ClassInfo*) it_mobj);
    TraceClassChain((HSD_ClassInfo*) &hsdMObj);
    for (g = HSD_GObjPLinkHead[9]; g; g = g->next) {
        HSD_JObj* j = g->hsd_obj;
        unsigned steps = 0, worst = 0;
        while (j && steps < 4000) {
            const unsigned len = DObjChainLength(j->u.dobj);
            if (len > worst)
                worst = len;
            ++steps;
            if (j->child)
                j = j->child;
            else if (j->next)
                j = j->next;
            else {
                while (j->parent && !j->parent->next && steps < 4000) {
                    j = j->parent;
                    ++steps;
                }
                j = j->parent ? j->parent->next : NULL;
            }
        }
        M360_MatchTrace("hang.item.gobj", (unsigned) (uintptr_t) g);
        M360_MatchTrace("hang.item.kind", g->user_data ? *(unsigned*) ((char*) g->user_data + 0x10) : 0xFFFFFFFFu);
        M360_MatchTrace("hang.item.jobj_steps", steps);
        M360_MatchTrace("hang.item.dobj_chain", worst);
    }
}

static void TraceHeap(const char* used, const char* largest)
{
    M360_MatchTrace(used, M360_HeapUsed());
    M360_MatchTrace(largest, M360_HeapLargestFree());
}

enum {
    kLinkLight = 0,
    kLinkStage = 1,
    kLinkFighter = 2,
    kMaxMapGObjs = 16,
    kMaxFighters = 4,
    kRespawnFrames = 60,
    kStartingStocks = 3,
    kGameModeClassic = 3,
    kGameModeAdventure = 4,
    kCampaignRounds = 5
};

enum { kPhaseSelect, kPhaseFight };

typedef struct CamGlobals {
    float x0, x4, x8, xC, x10, x14, x18, x1C, x20, x24, x28, x2C, x30, x34, x38, x3C, x40, x44;
} CamGlobals;

/* First 18 values of cm_803BCCA0 (melee/cm/camera.c). */
static const CamGlobals kCam = {
    83.0f, 1000.0f, -30.0f, 5.0f, -7.0f, 17.5f, -17.5f, 0.0f, 0.0682f,
    60.0f, 120.0f, 0.05f, 0.1f, 120.0f, 900.0f, 0.15f, 38.0f, 0.1f
};
/* cm_803BCB64 (melee/cm/camera.c). */
static const float kCamNear = 0.1f;
static const float kCamFar = 16384.0f;
static const float kCamFov = 30.0f;
static const float kCamAspect = 1.2173333f;
/* cm_803BCB9C (melee/cm/camera.c). */
static const float kTrackWeight[5] = { 0.0f, 1.5f, 1.32f, 1.16f, 1.0f };

typedef struct CamTransform {
    Vec3 interest, targetInterest, position, targetPosition;
    float fov, targetFov;
} CamTransform;

typedef struct CamBounds {
    float xMin, xMax, yMin, yMax, zPos;
    int subjects;
} CamBounds;

/* Stages whose original ground code only creates and animates map GObjs;
 * the ids follow each stage's OnInit (grbattle.c, grlast.c, groldpupupu.c,
 * grstory.c, grizumi.c, groldyoshi.c, grshrine.c, groldkongo.c,
 * grgarden.c). Moving platforms and hazards (Kongo's barrel, Japes' water
 * and Klaptraps) are not simulated: collision uses the lines as authored. */
typedef struct M360GrJoint {
    short group, mapId, joint;
} M360GrJoint;

/* Compiled-in StageData::joints tables (collision group, map GObj, joint). */
static const M360GrJoint kLastJoints[] = { { 0, 3, 0 } };
static const M360GrJoint kIzumiJoints[] = { { 0, 3, 1 }, { 1, 3, 2 }, { 2, 3, 3 } };
static const M360GrJoint kOldKongoJoints[] = { { 0, 3, 1 }, { 1, 3, 2 } };
static const M360GrJoint kCastleJoints[] = { { 4, 6, 1 }, { 5, 6, 4 } };
static const M360GrJoint kKongoJoints[] = { { 2, 10, 19 }, { 3, 10, 22 }, { 5, 10, 43 }, { 6, 10, 44 } };
static const M360GrJoint kZebesJoints[] = { { 1, 6, 21 }, { 4, 6, 14 }, { 3, 6, 1 } };
static const M360GrJoint kCorneriaJoints[] = { { 3, 3, 0 }, { 4, 3, 0 } };

typedef struct M360StageDesc {
    const char* name;
    const char* file;
    int gobjs[9];
    int hidden;
    const M360GrJoint* joints;
    int jointCount;
    int grkind;
} M360StageDesc;

static const M360StageDesc s_stages[] = {
    { "BATTLEFIELD", "GrNBa.dat", { 0, 3, 1, 6, -1 }, 3, NULL, 0, Gr_Kind_Battle },
    { "FINAL DESTINATION", "GrNLa.dat", { 0, 1, 2, 3, -1 }, -1, kLastJoints, 1, Gr_Kind_Last },
    { "DREAM LAND", "GrOp.dat", { 0, 3, 7, 5, 4, 6, 1, 8, -1 }, -1, NULL, 0, Gr_Kind_OldPupupu },
    { "YOSHIS STORY", "GrSt.dat", { 0, 1, 3, 2, -1 }, -1, NULL, 0, Gr_Kind_Story },
    { "FOUNTAIN OF DREAMS", "GrIz.dat", { 0, 1, 3, -1 }, -1, kIzumiJoints, 3, Gr_Kind_Izumi },
    { "YOSHIS ISLAND 64", "GrOy.dat", { 0, 1, 4, 5, 2, 3, -1 }, -1, NULL, 0, Gr_Kind_OldYoshi },
    { "HYRULE TEMPLE", "GrSh.dat", { 0, 1, 2, -1 }, -1, NULL, 0, Gr_Kind_Shrine },
    { "KONGO JUNGLE 64", "GrOk.dat", { 0, 3, 1, 2, -1 }, -1, kOldKongoJoints, 2, Gr_Kind_OldKongo },
    { "JUNGLE JAPES", "GrGd.dat", { 0, 4, 5, 6, 1, 3, 2, -1 }, -1, NULL, 0, Gr_Kind_Garden },
    { "PEACHS CASTLE", "GrCs.dat", { 0, 4, 3, 6, -1 }, -1, kCastleJoints, 2, Gr_Kind_Castle },
    { "KONGO JUNGLE", "GrKg.dat", { 0, 10, 5, 3, 6, 4, -1 }, -1, kKongoJoints, 4, Gr_Kind_Kongo },
    { "BRINSTAR", "GrZe.dat", { 0, 1, 6, 4, 8, -1 }, -1, kZebesJoints, 3, Gr_Kind_Zebes },
    { "GREEN GREENS", "GrGr.dat", { 0, 2, 6, 5, 4, -1 }, -1, NULL, 0, Gr_Kind_Greens },
    { "CORNERIA", "GrCn.dat", { 7, 3, 8, 9, 4, 11, -1 }, -1, kCorneriaJoints, 2, Gr_Kind_Corneria },
    { "POKEMON STADIUM", "GrPs.dat", { 0, 1, 2, -1 }, -1, NULL, 0, Gr_Kind_PStadium },
    { "ONETT", "GrOt.dat", { 0, 2, 5, 4, 3, -1 }, -1, NULL, 0, Gr_Kind_Onett },
    { "MUTE CITY", "GrMc.dat", { 0, 30, 36, 37, -1 }, -1, NULL, 0, Gr_Kind_MuteCity },
    { "MUSHROOM KINGDOM ROUTE", "GrNKr.dat", { 2, 0, 1, 3, -1 }, -1, NULL, 0, Gr_Kind_KinokoRoute },
    { "BRINSTAR ESCAPE", "GrNZr.dat", { 0, 1, 2, -1 }, -1, NULL, 0, Gr_Kind_ZebesRoute },
    { "UNDERGROUND MAZE", "GrNSr.dat", { 0, 4, 2, -1 }, -1, NULL, 0, Gr_Kind_ShrineRoute },
    /* Original road and finish/checkpoint markers. Dynamic cars are pending. */
    { "F-ZERO GRAND PRIX", "GrNBr.dat", { 0, 32, -1 }, -1, NULL, 0, Gr_Kind_BigBlueRoute },
};

enum { kStageCount = sizeof(s_stages) / sizeof(s_stages[0]) };

typedef struct M360StageArchive {
    void* archive;
    UnkStageDat* mapHead;
    MapCollData* coll;
    GroundParam* param;
    DiscU32* plit;
    int state;
} M360StageArchive;

static M360StageArchive s_stageArchives[kStageCount];
static unsigned s_stageIndex;
static unsigned s_builtStage = ~0u;
static unsigned s_stocks = 4;
/* Rule: 0 is stock mode, otherwise a timed match of this many minutes where
 * lives are unlimited and the score is KOs minus falls. */
static const unsigned kTimeOptions[] = { 0, 2, 3, 4, 5, 8 };
enum { kTimeOptionCount = sizeof(kTimeOptions) / sizeof(kTimeOptions[0]) };
static unsigned s_timeOption;
static int s_score[kMaxFighters];
static int s_draw;
static int s_suddenDeath;
static float s_selPrevSub;
static unsigned s_cpuLevel = 3;
/* Item switch: -1 off, 0-4 very low to very high (gm item_freq). */
static int s_itemFreq = -1;
static s32 s_itemCounts[35];
static float s_itemScale;
static void* s_archive;
static UnkStageDat* s_mapHead;
static MapCollData* s_coll;
static GroundParam* s_param;
static DiscU32* s_plit;
static HSD_JObj* s_points[256];
static HSD_JObj* s_models[kMaxMapGObjs];
static int s_modelIds[kMaxMapGObjs];
static unsigned s_modelCount;
static HSD_GObj* s_cameraGObj;
static HSD_CObj* s_cobj;
static HSD_LObj* s_lobj;
static void* s_fighters[kMaxFighters];
static unsigned s_fighterCount;
static unsigned s_respawn[kMaxFighters];
static unsigned s_stocksLost[kMaxFighters];
static unsigned s_stocksRemaining[kMaxFighters];
static int s_human[kMaxFighters];
static int s_matchOver;
static unsigned s_winner;
static M360MatchStage s_stage;
static CamTransform s_cam;
static int s_loaded;
static int s_active;
static int s_paused;
static unsigned s_disconnectedControllers;
static int s_rematchPending;
static int s_holdTraced;
static unsigned s_frame;
static unsigned s_gameMode = 2;
static unsigned s_campaignRound;
static unsigned s_campaignStocks = kStartingStocks;
static unsigned s_classicMatchups[kCampaignRounds];
static unsigned s_classicStKind = ~0u;
static int s_campaignRetryPending;
static int s_campaignTimedOut;
static unsigned s_adventureLives[4];
static unsigned s_campaignClearFrames;
static unsigned s_adventureElapsedFrames;
static int s_adventureLuigi;
static int s_adventureCheckpoint;
static unsigned s_mazeGoal, s_mazeVisited, s_mazeFinishFrames;
static int s_mazeRoom = -1;
static Vec3 s_mazePoints[6], s_mazeSpawns[6];
static HSD_JObj* s_mazeSymbols[6];
static float s_mazeRadiusX, s_mazeRadiusY;
static int InitMaze(void);
static float s_routeBlast[4], s_routeFightBlast[4];
static float s_routeCamera[4], s_routeFightCamera[4];
static unsigned s_loadFailedSlot;
static float s_scale = 1.0f;
static float s_tilt, s_pan, s_camX20, s_camX24, s_zoomRate, s_maxDepth;
static float s_trackRatio, s_fixedZoom, s_trackSmooth;
static HSD_WObjDesc s_eyeDesc, s_interestDesc;
static HSD_CameraDescPerspective s_camDesc;
static int s_phase;
static unsigned s_selKind[kMaxFighters];
static unsigned s_selCostume[kMaxFighters] = { 0, 3, 1, 2 };
static unsigned s_slotCount = 2;
static unsigned s_selReady[kMaxFighters];
static int s_selHuman[kMaxFighters];
static float s_selPrevStick[kMaxFighters];
static int s_selProbeP2;
static M360MatchAutoConfig s_auto;
static unsigned s_autoPlayed;
static unsigned s_autoResultFrames;

static int IsCampaign(void);
static void StartFight(void);
void M360_FighterDrawHitboxes(void);

/* Hitbox/hurtbox overlay, toggled with X while paused; drawn last on the
 * world effect link so it sits over the scene. */
static int s_debugHitboxes;

static void DebugRender(HSD_GObj* gobj, int pass)
{
    (void) gobj;
    if (s_debugHitboxes && pass == 2)
        M360_FighterDrawHitboxes();
}

static unsigned TimeMinutes(void)
{
    if (IsCampaign() || s_suddenDeath)
        return 0;
    return s_auto.enabled ? s_auto.timeMinutes : kTimeOptions[s_timeOption];
}

void M360_MatchSetAutoConfig(const M360MatchAutoConfig* config)
{
    s_auto = *config;
    s_autoPlayed = 0;
}

/* Applies the unattended setup instead of the select phase. */
static void ApplyAutoConfig(void)
{
    unsigned i;
    const unsigned count = M360_FighterKindCount();
    s_slotCount = s_auto.players < 2 ? 2 : (s_auto.players > kMaxFighters ? kMaxFighters : s_auto.players);
    for (i = 0; i < kMaxFighters; ++i) {
        s_selKind[i] = s_auto.kinds[i] < count ? s_auto.kinds[i] : 0;
        s_selCostume[i] = s_auto.costumes[i] % M360_FighterCostumeCount(s_selKind[i]);
        s_selReady[i] = 1;
        s_selHuman[i] = 0;
    }
    s_selHuman[0] = s_auto.humanP1 != 0;
    s_stocks = s_auto.stocks ? s_auto.stocks : 1;
    s_itemFreq = s_auto.items < -1 ? -1 : (s_auto.items > 4 ? 4 : s_auto.items);
    s_cpuLevel = s_auto.cpuLevel ? s_auto.cpuLevel : 1;
    M360_FighterSetCpuLevel(s_cpuLevel);
    s_selProbeP2 = 0;
    s_autoResultFrames = 0;
    M360_MatchTrace("auto.match.index", s_autoPlayed + 1);
    M360_MatchTrace("auto.stage", s_stageIndex);
    M360_MatchTrace("auto.players", s_slotCount);
    M360_MatchTrace("auto.time", s_auto.timeMinutes);
    M360_MatchTrace("auto.items", (unsigned) (s_itemFreq + 1));
    TraceHeap("auto.heap.used", "auto.heap.largest_free");
    StartFight();
}

static float DegToRad(float d) { return d * 0.017453292f; }

static void CollectPoints(HSD_JObj* root, HSD_Joint* joint)
{
    struct MapJointRemapEntry* entry = s_mapHead->unk0;
    HSD_JObj* order[128];
    unsigned count = 0;
    HSD_JObj* j = root;
    int i;
    while (j && count < 128) {
        order[count++] = j;
        if (j->child)
            j = j->child;
        else {
            while (j && !j->next && j != root)
                j = j->parent;
            if (!j || j == root)
                break;
            j = j->next;
        }
    }
    for (i = 0; i < s_mapHead->unk4; ++i, ++entry) {
        DiscS16* pairs;
        int k;
        if (entry->joint != joint)
            continue;
        pairs = entry->pairs;
        for (k = 0; k < entry->pair_count; ++k) {
            const int index = pairs[k * 2].v;
            const int slot = pairs[k * 2 + 1].v;
            if (index >= 0 && (unsigned) index < count && slot >= 0 && slot < 256)
                s_points[slot] = order[index];
        }
    }
}

static int PointPosition(int slot, Vec3* out)
{
    if (!s_points[slot])
        return 0;
    lb_8000B1CC(s_points[slot], NULL, out);
    return 1;
}

/* Collision groups bound to animated map joints (Ground_InitMapColl,
 * mpLib_800552B0/80055E9C): each frame the group's vertices are moved by the
 * joint's world matrix and the native lines are rebuilt from them. */
enum { kMaxCollBinds = 128, kMaxCollVerts = 2048 };
typedef struct M360CollBind {
    int group;
    HSD_JObj* jobj;
} M360CollBind;
static M360CollBind s_collBinds[kMaxCollBinds];
static unsigned s_collBindCount;
static float s_vtxX[kMaxCollVerts], s_vtxY[kMaxCollVerts];

/* Depth-first joint walk of mpLib_800552B0: index 0 is the first child and
 * instance joints are not descended into. */
static HSD_JObj* FindCollJoint(HSD_JObj* root, int z)
{
    HSD_JObj* j = root ? root->child : NULL;
    int i = 0;
    while (j && i != z) {
        ++i;
        if (!(j->flags & JOBJ_INSTANCE) && j->child) {
            j = j->child;
            continue;
        }
        if (j->next) {
            j = j->next;
            continue;
        }
        for (;;) {
            if (!j->parent) {
                j = NULL;
            } else if (j->parent->next) {
                j = j->parent->next;
            } else {
                j = j->parent;
                continue;
            }
            break;
        }
    }
    return j;
}

static void AddCollBind(int group, HSD_JObj* model, int z)
{
    HSD_JObj* j = FindCollJoint(model, z);
    if (!j || group < 0 || group >= s_coll->joint_count || s_collBindCount >= kMaxCollBinds)
        return;
    s_collBinds[s_collBindCount].group = group;
    s_collBinds[s_collBindCount++].jobj = j;
    M360_MatchTrace("match.stage.coll_bind", (unsigned) group);
}

static void BindMapColl(const M360StageDesc* stage, int id, HSD_JObj* model)
{
    struct UnkStageDat_x8_t* desc = MAP_GOBJ_DESC(s_mapHead, id);
    GrJoint* g = desc->unk20;
    int i;
    for (i = 0; g && i < desc->unk24; ++i)
        AddCollBind(g[i].x, model, g[i].z);
    for (i = 0; i < stage->jointCount; ++i)
        if (stage->joints[i].mapId == id)
            AddCollBind(stage->joints[i].group, model, stage->joints[i].joint);
}

static void SetupMatrixChain(HSD_JObj* jobj)
{
    HSD_JObj* chain[32];
    int n = 0;
    while (jobj && n < 32) {
        chain[n++] = jobj;
        jobj = jobj->parent;
    }
    while (n > 0)
        HSD_JObjSetupMatrix(chain[--n]);
}

static void UpdateMapColl(void)
{
    const MapJoint* groups = s_coll->joints;
    unsigned b, i;
    if (!s_collBindCount || (unsigned) s_coll->vert_count > kMaxCollVerts)
        return;
    for (b = 0; b < s_collBindCount; ++b) {
        const MapJoint* mj = &groups[s_collBinds[b].group];
        HSD_JObj* j = s_collBinds[b].jobj;
        MtxPtr m;
        int v;
        if (HSD_JObjGetFlags(j) & JOBJ_HIDDEN)
            continue;
        SetupMatrixChain(j);
        m = HSD_JObjGetMtxPtr(j);
        for (v = mj->vtx_start; v < mj->vtx_start + mj->vtx_count && v < s_coll->vert_count; ++v) {
            const float x = s_coll->verts[v].x, y = s_coll->verts[v].y;
            s_vtxX[v] = m[0][0] * x + m[0][1] * y + m[0][3];
            s_vtxY[v] = m[1][0] * x + m[1][1] * y + m[1][3];
        }
    }
    for (i = 0; i < s_stage.lineCount; ++i) {
        M360StageLine* l = &s_stage.lines[i];
        l->px0 = l->x0;
        l->py0 = l->y0;
        l->px1 = l->x1;
        l->py1 = l->y1;
        l->x0 = s_vtxX[l->v0];
        l->y0 = s_vtxY[l->v0];
        l->x1 = s_vtxX[l->v1];
        l->y1 = s_vtxY[l->v1];
    }
}

static HSD_JObj* CreateMapGObj(const M360StageDesc* stage, int id)
{
    struct UnkStageDat_x8_t* desc;
    HSD_Joint* joint;
    HSD_Joint scaleJoint;
    HSD_JObj* model;
    HSD_JObj* root;
    HSD_GObj* gobj;
    DiscU32* aj;
    DiscU32* ma;
    DiscU32* sa;
    u8* loop;
    if (id >= s_mapHead->unkC || s_modelCount >= kMaxMapGObjs)
        return NULL;
    desc = MAP_GOBJ_DESC(s_mapHead, id);
    joint = desc->unk0;
    model = HSD_JObjLoadJoint(joint);
    CollectPoints(model, joint);
    memset(&scaleJoint, 0, sizeof(scaleJoint));
    scaleJoint.scale.x = scaleJoint.scale.y = scaleJoint.scale.z = s_scale;
    root = HSD_JObjLoadJoint(&scaleJoint);
    HSD_JObjAddNext(model, root);
    aj = desc->unk4;
    ma = desc->unk8;
    sa = desc->unkC;
    HSD_JObjAddAnimAll(model, aj ? (HSD_AnimJoint*) (uintptr_t) aj[0].v : NULL,
                       ma ? (HSD_MatAnimJoint*) (uintptr_t) ma[0].v : NULL,
                       sa ? (HSD_ShapeAnimJoint*) (uintptr_t) sa[0].v : NULL);
    HSD_JObjReqAnimAll(model, 0.0f);
    loop = (u8*) desc->x28;
    if (loop && loop[0])
        HSD_ForeachAnim(model, JOBJ_TYPE, 0x77A4, AnimCallback((void (*)(void)) HSD_AObjSetFlags), AOBJ_ARG_AU, AOBJ_LOOP);
    HSD_JObjAnimAll(model);
    BindMapColl(stage, id, model);
    gobj = GObj_Create(HSD_GOBJ_CLASS_STAGE, 5, 0);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, root);
    GObj_SetupGXLink(gobj, HSD_GObj_JObjCallback, kLinkStage, 0);
    s_modelIds[s_modelCount] = id;
    s_models[s_modelCount++] = model;
    return root;
}

static void LoadCollision(void)
{
    MapLine* lines = s_coll->lines;
    DiscVec2* verts = s_coll->verts;
    int i;
    s_stage.lineCount = 0;
    for (i = 0; i < s_coll->line_count && s_stage.lineCount < M360_MAX_STAGE_LINES; ++i) {
        M360StageLine* out = &s_stage.lines[s_stage.lineCount++];
        out->x0 = verts[lines[i].v0_idx].x * s_scale;
        out->y0 = verts[lines[i].v0_idx].y * s_scale;
        out->x1 = verts[lines[i].v1_idx].x * s_scale;
        out->y1 = verts[lines[i].v1_idx].y * s_scale;
        out->kind = lines[i].hi_flags;
        out->flags = lines[i].lo_flags;
        out->v0 = lines[i].v0_idx;
        out->v1 = lines[i].v1_idx;
        out->px0 = out->x0;
        out->py0 = out->y0;
        out->px1 = out->x1;
        out->py1 = out->y1;
    }
    for (i = 0; i < s_coll->vert_count && i < kMaxCollVerts; ++i) {
        s_vtxX[i] = verts[i].x * s_scale;
        s_vtxY[i] = verts[i].y * s_scale;
    }
    s_collBindCount = 0;
}

static void LoadBounds(void)
{
    Vec3 a, b, c;
    int i;
    s_stage.camLeft = -170.0f; s_stage.camRight = 170.0f;
    s_stage.camTop = 120.0f; s_stage.camBottom = -60.0f;
    s_stage.camX = s_stage.camY = 0.0f;
    if (PointPosition(0x95, &a) && PointPosition(0x96, &b) && PointPosition(0x94, &c)) {
        s_stage.camLeft = (a.x < b.x ? a.x : b.x) - c.x;
        s_stage.camRight = (a.x < b.x ? b.x : a.x) - c.x;
        s_stage.camBottom = (a.y < b.y ? a.y : b.y) - c.y;
        s_stage.camTop = (a.y < b.y ? b.y : a.y) - c.y;
        s_stage.camX = c.x;
        s_stage.camY = c.y;
    }
    s_stage.blastLeft = -250.0f; s_stage.blastRight = 250.0f;
    s_stage.blastTop = 200.0f; s_stage.blastBottom = -100.0f;
    if (PointPosition(0x97, &a) && PointPosition(0x98, &b)) {
        s_stage.blastLeft = (a.x < b.x ? a.x : b.x) - s_stage.camX;
        s_stage.blastRight = (a.x < b.x ? b.x : a.x) - s_stage.camX;
        s_stage.blastBottom = (a.y < b.y ? a.y : b.y) - s_stage.camY;
        s_stage.blastTop = (a.y < b.y ? b.y : a.y) - s_stage.camY;
    }
    for (i = 0; i < 4; ++i) {
        if (PointPosition(i, &a)) {
            s_stage.spawnX[i] = a.x;
            s_stage.spawnY[i] = a.y;
        } else {
            s_stage.spawnX[i] = (i & 1 ? 1.0f : -1.0f) * 30.0f;
            s_stage.spawnY[i] = 20.0f;
        }
        /* Rebirth platform points are ground points 4-7 (Stage_80224E38). */
        if (PointPosition(4 + i, &a)) {
            s_stage.rebirthX[i] = a.x;
            s_stage.rebirthY[i] = a.y;
        } else {
            s_stage.rebirthX[i] = 0.0f;
            s_stage.rebirthY[i] = s_stage.spawnY[i] + 30.0f;
        }
    }
}

static void SetCollisionGroupEnabled(unsigned group, int enabled)
{
    const MapJoint* joint;
    int starts[5], counts[5], kind, line;
    if (group >= (unsigned) s_coll->joint_count) return;
    joint = &s_coll->joints[group];
    starts[0] = joint->floor_start; counts[0] = joint->floor_count;
    starts[1] = joint->ceiling_start; counts[1] = joint->ceiling_count;
    starts[2] = joint->right_wall_start; counts[2] = joint->right_wall_count;
    starts[3] = joint->left_wall_start; counts[3] = joint->left_wall_count;
    starts[4] = joint->dynamic_start; counts[4] = joint->dynamic_count;
    for (kind = 0; kind < 5; ++kind)
        for (line = starts[kind]; line >= 0 && line < starts[kind] + counts[kind] &&
             (unsigned) line < s_stage.lineCount; ++line)
            s_stage.lines[line].kind = enabled ? s_coll->lines[line].hi_flags : 0;
}

static void DisableCollisionGroup(unsigned group)
{
    SetCollisionGroupEnabled(group, 0);
}

static float CamLeft(void) { return s_stage.camLeft + s_stage.camX; }
static float CamRight(void) { return s_stage.camRight + s_stage.camX; }
static float CamTop(void) { return s_stage.camTop + s_stage.camY; }
static float CamBottom(void) { return s_stage.camBottom + s_stage.camY; }

static void ClampToCamBounds(Vec3* p)
{
    if (p->x < CamLeft()) p->x = CamLeft();
    if (p->x > CamRight()) p->x = CamRight();
    if (p->y > CamTop()) p->y = CamTop();
    if (p->y < CamBottom()) p->y = CamBottom();
}

/* Camera_8002958C: subject framing bounds. */
static void CamComputeBounds(CamBounds* bounds)
{
    float minX = 3.4e38f, minY = 3.4e38f, maxX = -3.4e38f, maxY = -3.4e38f;
    float mult, z, zf;
    unsigned i;
    int n = 0;
    for (i = 0; i < s_fighterCount; ++i)
        if (s_fighters[i] && !s_respawn[i] && (s_stocksRemaining[i] || TimeMinutes()))
            ++n;
    mult = (n < 5 ? kTrackWeight[n] : 1.0f) * s_trackRatio;
    for (i = 0; i < s_fighterCount; ++i) {
        Vec3 base, test;
        float left, right, up, down;
        if (!s_fighters[i] || s_respawn[i] || (!s_stocksRemaining[i] && !TimeMinutes()))
            continue;
        M360_FighterCameraBox(s_fighters[i], &base.x, &base.y, &left, &right, &up, &down);
        base.z = 0.0f;
        ClampToCamBounds(&base);
        test = base;
        test.x = left * mult + base.x;
        ClampToCamBounds(&test);
        if (test.x < minX) minX = test.x;
        if (test.x > maxX) maxX = test.x;
        test.x = right * mult + base.x;
        ClampToCamBounds(&test);
        if (test.x < minX) minX = test.x;
        if (test.x > maxX) maxX = test.x;
        test.y = down * mult + base.y;
        ClampToCamBounds(&test);
        if (test.y < minY) minY = test.y;
        if (test.y > maxY) maxY = test.y;
        test.y = up * mult + base.y;
        ClampToCamBounds(&test);
        if (test.y < minY) minY = test.y;
        if (test.y > maxY) maxY = test.y;
    }
    if (!n) {
        minX = s_stage.camX - 40.0f; maxX = s_stage.camX + 40.0f;
        minY = s_stage.camY - 40.0f; maxY = s_stage.camY + 40.0f;
    }
    z = fabsf(s_cam.position.z);
    zf = z < 80.0f ? 0.0f : (z > 5000.0f ? 1.0f : (z - 80.0f) / 4920.0f);
    bounds->xMin = minX;
    bounds->yMin = minY - (390.0f * zf + 10.0f);
    bounds->xMax = maxX;
    bounds->yMax = maxY;
    bounds->subjects = n;
    bounds->zPos = z;
}

/* Camera_80029CF8: target interest/position from the framing bounds. */
static void CamTarget(const CamBounds* b)
{
    const float dx = b->xMax - b->xMin, dy = b->yMax - b->yMin;
    const float spread = dx > dy ? dx : dy;
    float t, base, angle, yAngle, tanU, tanD, distY, yOff, xCenter, tanR, tanL;
    float distX, xOff, dist;
    if (spread > kCam.x28) t = kCam.x20;
    else if (spread < kCam.x24) t = kCam.x1C;
    else t = (kCam.x20 - kCam.x1C) * ((spread - kCam.x24) / (kCam.x28 - kCam.x24)) + kCam.x1C;
    base = ((b->yMin - s_stage.camY) + (b->yMax - s_stage.camY)) * (0.5f - t) + s_stage.camY;
    angle = -DegToRad((base + kCam.x8) * s_camX24);
    if (angle > DegToRad(kCam.xC)) angle = DegToRad(kCam.xC);
    if (angle < DegToRad(kCam.x10)) angle = DegToRad(kCam.x10);
    angle += DegToRad(s_pan);
    yAngle = angle;
    tanU = tanf(0.5f * DegToRad(s_cam.fov) + angle);
    tanD = tanf(0.5f * DegToRad(s_cam.fov) - angle);
    distY = dy / (tanU + tanD);
    yOff = distY * tanf(yAngle);
    s_cam.targetInterest.y = yOff + (b->yMax - distY * tanU);
    xCenter = 0.5f * (b->xMin + b->xMax);
    angle = -DegToRad((xCenter - s_stage.camX) * s_camX20);
    if (angle > DegToRad(kCam.x14)) angle = DegToRad(kCam.x14);
    if (angle < DegToRad(kCam.x18)) angle = DegToRad(kCam.x18);
    tanR = kCamAspect * tanf(0.5f * DegToRad(s_cam.fov) - angle);
    tanL = kCamAspect * tanf(0.5f * DegToRad(s_cam.fov) + angle);
    distX = dx / (tanR + tanL);
    xOff = kCamAspect * (distX * tanf(angle));
    s_cam.targetInterest.x = (b->xMax - distX * tanR) - xOff;
    s_cam.targetInterest.z = 0.0f;
    dist = distY > distX ? distY : distX;
    if (dist < s_zoomRate) dist = s_zoomRate;
    if (dist > s_maxDepth) dist = s_maxDepth;
    s_cam.targetPosition.x = s_cam.targetInterest.x + xOff;
    s_cam.targetPosition.y = s_cam.targetInterest.y - yOff;
    s_cam.targetPosition.z = s_cam.targetInterest.z + dist;
}

/* Camera_80029AAC and Camera_80029C88: smoothing toward the targets. */
static void CamFollow(const CamBounds* b)
{
    float spread, follow, lerp, scale;
    if (b->subjects) {
        const float dx = b->xMax - b->xMin, dy = b->yMax - b->yMin;
        spread = dx > dy ? dx : dy;
    } else {
        spread = 99999.0f;
    }
    if (spread > kCam.x38) follow = kCam.x30;
    else if (spread < kCam.x34) follow = kCam.x2C;
    else follow = ((spread - kCam.x34) / (kCam.x38 - kCam.x34)) * (kCam.x30 - kCam.x2C) + kCam.x2C;
    lerp = follow * s_trackSmooth;
    if (lerp > 1.0f) lerp = 1.0f;
    if (lerp < 0.0001f) lerp = 0.0001f;
    s_cam.interest.x += (s_cam.targetInterest.x - s_cam.interest.x) * lerp;
    s_cam.interest.y += (s_cam.targetInterest.y - s_cam.interest.y) * lerp;
    scale = kCam.x3C * s_trackSmooth;
    if (scale > 1.0f) scale = 1.0f;
    s_cam.position.x += (s_cam.targetPosition.x - s_cam.position.x) * scale;
    s_cam.position.y += (s_cam.targetPosition.y - s_cam.position.y) * scale;
    s_cam.position.z += (s_cam.targetPosition.z - s_cam.position.z) * scale;
}

/* Camera_RequestQuake (camera.c): the original shakes through a stage quake
 * GObj animation; this port offsets the camera by a decaying jitter sized by
 * the quake kind (1 loop, 2 small, 3 medium, 4 large). The signature uses int
 * for CmQuakeKind and void* for the epicenter, which the fighter and effect
 * code pass unchanged. */
static int s_quakeFrames[5];
static unsigned s_quakeSeed = 0x1234567u;

void Camera_RequestQuake(int kind, void* pos)
{
    (void) pos;
    if (kind < 1 || kind >= 5)
        return;
    s_quakeFrames[kind] = kind == 1 ? 10 : 22;
}

void Camera_StopQuake(int kind)
{
    if (kind >= 0 && kind < 5)
        s_quakeFrames[kind] = 0;
}

static float QuakeAmplitude(void)
{
    static const float kAmp[5] = { 0.0f, 1.0f, 1.2f, 2.2f, 3.6f };
    float amp = 0.0f;
    int i;
    for (i = 1; i < 5; ++i)
        if (s_quakeFrames[i]) {
            const float a = kAmp[i] * (i == 1 ? 1.0f : s_quakeFrames[i] / 22.0f);
            if (a > amp)
                amp = a;
        }
    return amp;
}

static float QuakeJitter(void)
{
    s_quakeSeed = s_quakeSeed * 1664525u + 1013904223u;
    return (float) ((s_quakeSeed >> 8) & 0xFFFF) / 32767.5f - 1.0f;
}

static void CamApply(void)
{
    const float amp = QuakeAmplitude();
    Vec3 interest = s_cam.interest;
    Vec3 eye = s_cam.position;
    if (amp > 0.0f) {
        const float dx = amp * QuakeJitter();
        const float dy = amp * QuakeJitter();
        interest.x += dx;
        interest.y += dy;
        eye.x += dx;
        eye.y += dy;
    }
    HSD_CObjSetFov(s_cobj, s_cam.fov);
    HSD_CObjSetInterest(s_cobj, &interest);
    HSD_CObjSetEyePosition(s_cobj, &eye);
}

static void CamUpdate(int snap)
{
    CamBounds bounds;
    int i;
    for (i = 0; i < 5; ++i)
        if (s_quakeFrames[i])
            --s_quakeFrames[i];
    CamComputeBounds(&bounds);
    s_cam.targetFov = kCam.x40;
    s_cam.fov += (s_cam.targetFov - s_cam.fov) * kCam.x44;
    if (snap)
        s_cam.fov = s_cam.targetFov;
    CamTarget(&bounds);
    if (snap) {
        s_cam.interest = s_cam.targetInterest;
        s_cam.position = s_cam.targetPosition;
    } else {
        CamFollow(&bounds);
    }
    CamApply();
}

static void CameraRender(HSD_GObj* gobj, int pass)
{
    (void) pass;
    if (HSD_CObjSetCurrent(gobj->hsd_obj)) {
        HSD_SetEraseColor(0, 0, 0, 255);
        HSD_CObjEraseScreen(gobj->hsd_obj, 1, 0, 1);
        HSD_FogSet(NULL);
        HSD_GObj_80390ED0(gobj, 7);
        HSD_CObjEndCurrent();
    }
}

static void CreateCamera(void)
{
    memset(&s_camDesc, 0, sizeof(s_camDesc));
    memset(&s_eyeDesc, 0, sizeof(s_eyeDesc));
    memset(&s_interestDesc, 0, sizeof(s_interestDesc));
    s_eyeDesc.pos.z = 100.0f;
    s_camDesc.projection_type = 1;
    s_camDesc.viewport.xmax = 640;
    s_camDesc.viewport.ymax = 480;
    s_camDesc.scissor.right = 640;
    s_camDesc.scissor.bottom = 480;
    s_camDesc.eyepos = &s_eyeDesc;
    s_camDesc.interest = &s_interestDesc;
    s_camDesc.nnear = kCamNear;
    s_camDesc.ffar = kCamFar;
    s_camDesc.fov = kCamFov;
    s_camDesc.aspect = kCamAspect;
    s_cobj = HSD_CObjLoadDesc((HSD_CObjDesc*) &s_camDesc);
    s_cameraGObj = GObj_Create(19, 20, 0);
    HSD_GObjObject_80390A70(s_cameraGObj, HSD_GObj_CameraKind, s_cobj);
    GObj_SetupGXLinkMax(s_cameraGObj, CameraRender, 0);
    /* Items draw on GX link 6 (item.c), world effects on 7 (efLib_Init).
     * Link 8 holds screen-space particles, drawn by the IfAll HUD camera
     * (links 8, 10 and 11). */
    s_cameraGObj->gxlink_prios = (1 << kLinkLight) | (1 << kLinkStage) | (1 << kLinkFighter) | (1 << 6) |
                                  (1 << 7);
    memset(&s_cam, 0, sizeof(s_cam));
    s_cam.fov = kCamFov;
}

static void CreateLights(void)
{
    HSD_GObj* gobj = GObj_Create(12, 3, 0);
    s_lobj = lb_80011AC4(s_plit);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_LightKind, s_lobj);
    GObj_SetupGXLink(gobj, HSD_GObj_LObjCallback, kLinkLight, 0);
    HSD_LObjReqAnimAll(s_lobj, 0.0f);
}

/* Archives relocate in place, so each stage file is opened once. */
static int LoadStageArchive(unsigned index)
{
    M360StageArchive* a = &s_stageArchives[index];
    if (!a->state) {
        unsigned size = 0;
        unsigned char* image = M360_ReadDiscFile(s_stages[index].file, &size);
        M360_MatchTrace("match.stage.bytes", size);
        a->state = -1;
        if (image && (a->archive = M360_ArchiveOpen(image, size)) != NULL) {
            a->mapHead = M360_ArchiveFind(a->archive, "map_head");
            a->coll = M360_ArchiveFind(a->archive, "coll_data");
            a->param = M360_ArchiveFind(a->archive, "grGroundParam");
            a->plit = M360_ArchiveFind(a->archive, "map_plit");
            if (a->mapHead && a->coll && a->param && a->plit)
                a->state = 1;
        }
        M360_MatchTrace("match.stage.index", index);
        M360_MatchTrace("match.stage.map_gobjs", a->mapHead ? (unsigned) a->mapHead->unkC : 0);
        M360_MatchTrace("match.stage.coll_lines", a->coll ? (unsigned) a->coll->line_count : 0);
    }
    if (a->state < 0)
        return 0;
    s_archive = a->archive;
    s_mapHead = a->mapHead;
    s_coll = a->coll;
    s_param = a->param;
    s_plit = a->plit;
    s_scale = s_param->y;
    s_tilt = s_param->x8;
    s_pan = (float) s_param->x14;
    s_camX20 = s_param->x1C;
    s_camX24 = s_param->x18;
    s_zoomRate = (float) s_param->xC;
    s_maxDepth = (float) s_param->x10;
    s_trackSmooth = s_param->x28;
    s_trackRatio = s_param->x20;
    s_fixedZoom = s_param->x24;
    LoadCollision();
    return 1;
}

int M360_MatchLoad(void)
{
    if (s_loaded)
        return 1;
    if (!LoadStageArchive(0))
        return 0;
    M360_MatchTrace("match.fighter.loaded", M360_FighterLoad());
    s_loaded = 1;
    return 1;
}

static void FreeAllGObjs(void)
{
    int link;
    M360_HudStop();
    for (link = 0; link < 64; ++link) {
        HSD_GObj* gobj = HSD_GObjPLinkHead[link];
        while (gobj) {
            HSD_GObj* next = gobj->next;
            HSD_GObjFree(gobj);
            gobj = next;
        }
    }
    s_modelCount = 0;
    s_lobj = NULL;
    s_cameraGObj = NULL;
}

/* Ground_801C28CC/801C2AE8 over the stage's first StageParam row: per-kind
 * random item weights and the spawn frequency scale. */
static StageParam* MatchStageParam(void)
{
    StageParam* rows;
    int i;
    if (!s_param || !s_param->stage_params || s_param->stage_param_count <= 0)
        return NULL;
    rows = (StageParam*) (uintptr_t) s_param->stage_params;
    if (IsCampaign() && s_classicStKind != ~0u) {
        for (i = 0; i < s_param->stage_param_count; ++i)
            if ((unsigned) rows[i].stkind == s_classicStKind)
                return &rows[i];
        return NULL;
    }
    return &rows[0];
}

static void LoadItemTable(void)
{
    StageParam* row;
    int j;
    memset(s_itemCounts, 0, sizeof(s_itemCounts));
    s_itemScale = 0.0f;
    row = MatchStageParam();
    if (!row)
        return;
    for (j = 0; j < 35; ++j)
        s_itemCounts[j] = s_param->x6A[j] * row->x1A[j];
    s_itemScale = (0.01f * s_param->x68) * (0.01f * row->x18);
}

s32* Ground_801C2AD8(void)
{
    return s_itemCounts;
}

float Ground_801C2AE8(StKind stkind)
{
    (void) stkind;
    return s_itemScale;
}

/* Stage_80224FDC: a random item spawn point (ground points 0x7F-0x93). */
bool Stage_80224FDC(Vec3* out)
{
    int tries;
    for (tries = 0; tries < 21; ++tries)
        if (PointPosition(0x7F + HSD_Randi(21), out))
            return true;
    for (tries = 0x7F; tries < 0x94; ++tries)
        if (PointPosition(tries, out))
            return true;
    return false;
}

s32 gm_8016AE80(void)
{
    return s_active && !IsCampaign() ? s_itemFreq : -1;
}

f32 gm_8016AE94(void)
{
    return 1.0f;
}

u64 gm_8016AEA4(void)
{
    /* Every standard item (below It_Kind_L_Gun_Ray). */
    return ((u64) 1 << 35) - 1;
}

s32 gm_8016AEB8(void)
{
    return 0;
}

bool gm_8016B238(void)
{
    return false;
}

/* Builds the selected stage's lights, map GObjs, bounds and camera. */
static int BuildStage(unsigned index)
{
    const M360StageDesc* desc;
    unsigned i;
    if (index >= kStageCount || !LoadStageArchive(index)) {
        if (index == 0 || !LoadStageArchive(0))
            return 0;
        index = 0;
    }
    desc = &s_stages[index];
    M360_StageSetKind(desc->grkind);
    if (s_builtStage != ~0u)
        FreeAllGObjs();
    memset(s_points, 0, sizeof(s_points));
    s_modelCount = 0;
    CreateLights();
    for (i = 0; i < 9 && desc->gobjs[i] >= 0; ++i) {
        HSD_JObj* root = CreateMapGObj(desc, desc->gobjs[i]);
        if (root && desc->gobjs[i] == desc->hidden)
            HSD_JObjSetFlagsAll(root, JOBJ_HIDDEN);
    }
    UpdateMapColl();
    LoadBounds();
    if (desc->grkind == Gr_Kind_KinokoRoute || desc->grkind == Gr_Kind_ZebesRoute ||
        desc->grkind == Gr_Kind_ShrineRoute || desc->grkind == Gr_Kind_BigBlueRoute) {
        float minX = s_stage.spawnX[0], maxX = minX;
        float minY = s_stage.spawnY[0], maxY = minY;
        Vec3 marker;
        s_routeFightCamera[0] = s_stage.camLeft; s_routeFightCamera[1] = s_stage.camRight;
        s_routeFightCamera[2] = s_stage.camBottom; s_routeFightCamera[3] = s_stage.camTop;
        /* grKinokoRoute_80207634 establishes blast offsets at 1.5 times
         * the authored camera offsets. Use those in the fixed Yoshi arena. */
        s_routeFightBlast[0] = s_stage.camLeft * 1.5f;
        s_routeFightBlast[1] = s_stage.camRight * 1.5f;
        s_routeFightBlast[2] = s_stage.camBottom * 1.5f;
        s_routeFightBlast[3] = s_stage.camTop * 1.5f;
        for (i = 0; i < s_stage.lineCount; ++i) {
            const M360StageLine* line = &s_stage.lines[i];
            if (line->x0 < minX) minX = line->x0;
            if (line->x1 < minX) minX = line->x1;
            if (line->x0 > maxX) maxX = line->x0;
            if (line->x1 > maxX) maxX = line->x1;
            if (line->y0 < minY) minY = line->y0;
            if (line->y1 < minY) minY = line->y1;
            if (line->y0 > maxY) maxY = line->y0;
            if (line->y1 > maxY) maxY = line->y1;
        }
        /* Route archives encode an initial camera window rather than the
         * whole course. A fixed VS blast window kills P1 at the start. */
        s_stage.camX = s_stage.camY = 0.0f;
        s_stage.camLeft = minX - 100.0f; s_stage.camRight = maxX + 100.0f;
        s_stage.camBottom = minY - 80.0f; s_stage.camTop = maxY + 80.0f;
        s_stage.blastLeft = minX - 200.0f; s_stage.blastRight = maxX + 200.0f;
        s_stage.blastBottom = minY - 150.0f; s_stage.blastTop = maxY + 200.0f;
        s_routeBlast[0] = s_stage.blastLeft; s_routeBlast[1] = s_stage.blastRight;
        s_routeBlast[2] = s_stage.blastBottom; s_routeBlast[3] = s_stage.blastTop;
        s_routeCamera[0] = s_stage.camLeft; s_routeCamera[1] = s_stage.camRight;
        s_routeCamera[2] = s_stage.camBottom; s_routeCamera[3] = s_stage.camTop;
        if (PointPosition(0xBD, &marker)) M360_MatchTrace("match.adventure.checkpoint_x", (unsigned) (int) marker.x);
        if (PointPosition(0x99, &marker)) M360_MatchTrace("match.adventure.goal_x", (unsigned) (int) marker.x);
    }
    /* Inactive collision groups from each original OnInit; retain indices. */
    if (desc->grkind == Gr_Kind_Castle) {
        DisableCollisionGroup(0); DisableCollisionGroup(1); DisableCollisionGroup(2);
        for (i = 6; i <= 14; ++i) DisableCollisionGroup(i);
    } else if (desc->grkind == Gr_Kind_Kongo) {
        DisableCollisionGroup(0); DisableCollisionGroup(1);
    } else if (desc->grkind == Gr_Kind_Corneria) {
        for (i = 0; i <= 2; ++i) DisableCollisionGroup(i);
        for (i = 5; i <= 7; ++i) DisableCollisionGroup(i);
    } else if (desc->grkind == Gr_Kind_PStadium) {
        DisableCollisionGroup(0); DisableCollisionGroup(1); DisableCollisionGroup(2);
        DisableCollisionGroup(3); DisableCollisionGroup(5); DisableCollisionGroup(7);
    }
    LoadItemTable();
    M360_FighterBuildIslands();
    CreateCamera();
    s_builtStage = index;
    s_stageIndex = index;
    CamUpdate(1);
    return 1;
}

float M360_MatchStageScale(void)
{
    return s_scale;
}

void M360_MatchCameraVectors(float* interest, float* eye)
{
    interest[0] = s_cam.interest.x;
    interest[1] = s_cam.interest.y;
    interest[2] = s_cam.interest.z;
    eye[0] = s_cam.position.x;
    eye[1] = s_cam.position.y;
    eye[2] = s_cam.position.z;
}

/* camera.c: the gameplay camera GObj (world-to-screen for the HUD tags). */
HSD_GObj* Camera_80030A50(void)
{
    return s_cameraGObj;
}

unsigned M360_MatchSlotStocks(unsigned slot)
{
    return slot < kMaxFighters && s_fighters[slot] ? s_stocksRemaining[slot] : 0;
}

unsigned M360_MatchStageCount(void)
{
    return kStageCount - 4;
}

const char* M360_MatchStageName(unsigned index)
{
    return index < kStageCount ? s_stages[index].name : "";
}

int M360_MatchBgmId(void)
{
    StageParam* row = MatchStageParam();
    if (!row)
        return -1;
    /* The base stage row precedes its event/1P variants. Ground_801C24F8
     * selects xC for VS and x4 for 1P; alternate/random tracks remain pending. */
    return IsCampaign() ? row->x4 : (int) row->xC;
}

int M360_MatchGroundKind(void)
{
    return s_stages[s_stageIndex].grkind;
}

float M360_MatchFixedZoom(void)
{
    return s_fixedZoom;
}

void M360_MatchEnter(void)
{
    unsigned i;
    const int rematch = s_rematchPending && !IsCampaign();
    const int retry = s_campaignRetryPending && IsCampaign();
    s_rematchPending = 0;
    s_campaignRetryPending = 0;
    if (!M360_MatchLoad())
        return;
    M360_FighterResetMatch();
    s_builtStage = ~0u;
    if (s_auto.enabled && !IsCampaign())
        s_stageIndex = s_auto.stage < kStageCount ? s_auto.stage : 0;
    M360_MatchTrace("match.enter.step", 1);
    if (!BuildStage(s_stageIndex))
        return;
    M360_MatchTrace("match.enter.step", 3);
    s_fighterCount = 0;
    memset(s_fighters, 0, sizeof(s_fighters));
    s_matchOver = 0;
    s_loadFailedSlot = 0;
    s_winner = 0;
    s_draw = 0;
    s_campaignTimedOut = 0;
    s_suddenDeath = 0;
    s_paused = 0;
    s_disconnectedControllers = 0;
    s_frame = 0;
    s_active = 1;
    memset(s_selReady, 0, sizeof(s_selReady));
    if (!rematch) {
        s_selHuman[0] = 1;
        for (i = 1; i < kMaxFighters; ++i)
            s_selHuman[i] = 0;
    }
    if (IsCampaign())
        s_slotCount = 2;
    s_selProbeP2 = !IsCampaign() && !rematch;
    memset(s_selPrevStick, 0, sizeof(s_selPrevStick));
    s_selPrevSub = 0.0f;
    for (i = 0; i < kMaxFighters; ++i)
        s_selCostume[i] %= M360_FighterCostumeCount(s_selKind[i]);
    s_phase = kPhaseSelect;
    if (s_auto.enabled && IsCampaign()) {
        s_selKind[0] = s_auto.kinds[0] < M360_FighterKindCount() ? s_auto.kinds[0] : 0;
        s_selCostume[0] = s_auto.costumes[0];
        s_selHuman[0] = s_auto.humanP1 != 0;
        s_cpuLevel = s_auto.cpuLevel;
        M360_FighterSetCpuLevel(s_cpuLevel);
        StartFight();
    } else if (rematch || retry || (IsCampaign() && s_campaignRound > 0)) {
        for (i = 0; i < s_slotCount; ++i)
            s_selReady[i] = 1;
        StartFight();
    } else if (s_auto.enabled && !IsCampaign()) {
        ApplyAutoConfig();
    }
    CamUpdate(1);
    M360_MatchTrace("match.mode", s_gameMode);
    M360_MatchTrace("match.campaign.round", s_campaignRound + 1);
    M360_MatchTrace("match.enter.blast_left", (unsigned) (int) s_stage.blastLeft);
    M360_MatchTrace("match.enter.blast_bottom", (unsigned) (int) s_stage.blastBottom);
    for (i = 0; i < 4; ++i) {
        M360_MatchTrace("match.enter.spawn_x", (unsigned) (int) s_stage.spawnX[i]);
        M360_MatchTrace("match.enter.spawn_y", (unsigned) (int) s_stage.spawnY[i]);
    }
}

static int IsCampaign(void)
{
    return s_gameMode == kGameModeClassic || s_gameMode == kGameModeAdventure;
}

unsigned M360_MatchCampaignRounds(unsigned mode)
{
    return mode == kGameModeAdventure ? (unsigned) M360_ADVENTURE_ROUNDS :
           mode == kGameModeClassic ? kCampaignRounds : 0;
}

static const M360AdventureEncounter* AdventureEncounter(void)
{
    return s_gameMode == kGameModeAdventure && s_campaignRound < M360_ADVENTURE_ROUNDS
               ? &g_m360Adventure[s_campaignRound] : NULL;
}

unsigned M360_MatchNextCampaignRound(void)
{
    unsigned next = s_campaignRound + 1;
    unsigned elapsed = s_matchOver ? s_campaignClearFrames : M360_HudFightFrames();
    const M360AdventureEncounter* encounter = AdventureEncounter();
    /* gm_801B4C5C skips Giant Kirby when the team battle took over thirty
     * whole seconds. Preserve its integer frame-to-second comparison. */
    if (encounter && encounter->scene == 35 && elapsed / 60 > 30 &&
        next < M360_ADVENTURE_ROUNDS && g_m360Adventure[next].scene == 37)
        ++next;
    /* gm_8017E7FC: Normal or higher, strictly under eighteen minutes. */
    if (encounter && encounter->scene == 89 && next < M360_ADVENTURE_ROUNDS &&
        g_m360Adventure[next].scene == 92 &&
        (s_cpuLevel < 5 || elapsed >= 64800u ||
         s_adventureElapsedFrames >= 64800u - elapsed))
        ++next;
    return next;
}

static unsigned CampaignSeconds(void)
{
    const M360AdventureEncounter* encounter = AdventureEncounter();
    return encounter ? encounter->seconds : s_gameMode == kGameModeClassic ? kClassicNormalSeconds : 0;
}

int M360_MatchIsTeams(void) { return IsCampaign(); }
int M360_MatchTeam(unsigned slot) { return IsCampaign() ? (slot == 0 ? 0 : 1) : (int) slot; }

float M360_MatchCombatRatio(unsigned slot, int defense)
{
    unsigned difficulty, percent;
    if (slot == 0 || slot >= s_slotCount || !AdventureEncounter())
        return 1.0f;
    /* The current selector exposes CPU levels 1..9. Map pairs to the five
     * original Adventure difficulties; level 9 selects Very Hard. */
    difficulty = s_cpuLevel > 0 ? (s_cpuLevel - 1) / 2 : 0;
    if (difficulty > 4) difficulty = 4;
    percent = g_m360AdventureRatios[s_campaignRound][difficulty][defense != 0];
    /* Escape has sentinel rows and Giga has no playable low difficulties. */
    return percent > 1 ? (float) percent / 100.0f : 1.0f;
}

unsigned M360_MatchCpuLevel(unsigned slot)
{
    const M360AdventureEncounter* encounter = AdventureEncounter();
    unsigned difficulty, enemy;
    /* Route NPCs are constructed before s_slotCount expands at checkpoint. */
    if (!encounter || slot == 0 || slot >= kMaxFighters)
        return s_cpuLevel;
    difficulty = s_cpuLevel > 0 ? (s_cpuLevel - 1) / 2 : 0;
    if (difficulty > 4) difficulty = 4;
    /* gm_8017CE34 uses the first entry for generated teams (flag 8). */
    enemy = (encounter->scene == 1 || (encounter->flags & 8)) ? 0 : slot - 1;
    if (enemy > 2) enemy = 2;
    return g_m360AdventureCpuLevels[s_campaignRound][difficulty][enemy];
}

unsigned M360_MatchCpuKind(unsigned slot)
{
    const M360AdventureEncounter* encounter = AdventureEncounter();
    unsigned difficulty, enemy;
    if (!encounter || slot == 0 || slot >= kMaxFighters)
        return 4; /* gm_1601.c: standard VS CPU. */
    if (encounter->flags & 4)
        return 27; /* gm_8017CE34: permanent metal behavior. */
    if (encounter->scene == 1 || (encounter->flags & 8))
        return HSD_Randi(4) == 3 ? 24 : 23; /* fn_8016A4C8 generator. */
    difficulty = s_cpuLevel > 0 ? (s_cpuLevel - 1) / 2 : 0;
    if (difficulty > 4) difficulty = 4;
    enemy = slot - 1;
    if (enemy > 2) enemy = 2;
    return g_m360AdventureCpuKinds[s_campaignRound][difficulty][enemy];
}

static unsigned StartingStocks(unsigned slot)
{
    return IsCampaign() ? (slot == 0 ? s_campaignStocks :
           s_gameMode == kGameModeAdventure ? s_adventureLives[slot] : 1) : s_stocks;
}

static int LoseStock(unsigned slot)
{
    if (!s_stocksRemaining[slot])
        return 0;
    --s_stocksRemaining[slot];
    if (IsCampaign() && slot == 0)
        s_campaignStocks = s_stocksRemaining[slot];
    return 1;
}

static int StockResult(unsigned* winner, int* draw)
{
    unsigned i, alive = 0, last = 0;
    if (IsCampaign()) {
        unsigned enemies = 0;
        const M360AdventureEncounter* encounter = AdventureEncounter();
        const int course = encounter && (encounter->scene == 1 || encounter->scene == 17 ||
                                         encounter->scene == 27 || encounter->scene == 58);
        for (i = 1; i < s_fighterCount; ++i)
            if (s_fighters[i]) enemies += s_stocksRemaining[i];
        if (s_stocksRemaining[0] && (enemies || course)) return 0;
        *winner = s_stocksRemaining[0] ? 0 : 1;
        *draw = !s_stocksRemaining[0] && !enemies && !course;
        return 1;
    }
    for (i = 0; i < s_fighterCount; ++i)
        if (s_fighters[i] && s_stocksRemaining[i]) {
            ++alive;
            last = i;
        }
    if (alive > 1)
        return 0;
    *winner = last;
    *draw = alive == 0;
    return 1;
}

static int ClassicStageIndex(unsigned matchup)
{
    unsigned i;
    for (i = 0; i < kStageCount; ++i)
        if (s_stages[i].grkind == g_m360ClassicNormal[matchup].grkind)
            return (int) i;
    return -1;
}

/* Normal Classic encounters use original stage/character pairs. Special
 * encounters and Adventure still require their own scene implementations. */
static void CampaignOpponent(void)
{
    const unsigned rosterCount = M360_FighterKindCount();
    const M360AdventureEncounter* adventure = AdventureEncounter();
    if (adventure) {
        unsigned i, slots = 0;
        memset(s_adventureLives, 0, sizeof(s_adventureLives));
        s_slotCount = 1;
        s_classicStKind = adventure->stkind;
        for (i = 0; i < 3; ++i) {
            const int fighterKind = adventure->scene == 3 && i == 0 && s_adventureLuigi
                                        ? M360_ADVENTURE_LUIGI_KIND : adventure->fighters[i];
            const int kind = M360_FighterIndexForKind(fighterKind);
            if (kind < 0) continue;
            s_selKind[++slots] = (unsigned) kind;
            s_selCostume[slots] = 0;
        }
        for (i = 1; i <= slots; ++i)
            s_adventureLives[i] = adventure->opponents / slots + (i <= adventure->opponents % slots);
        s_slotCount += slots;
        /* The Mushroom Kingdom enemies are created at their checkpoint. */
        if (adventure->scene == 1 || adventure->scene == 17 || adventure->scene == 58) s_slotCount = 1;
        return;
    }
    if (s_gameMode == kGameModeClassic && s_campaignRound < kCampaignRounds) {
        unsigned chosen = s_classicMatchups[s_campaignRound];
        if (chosen == ~0u) {
            unsigned candidates[sizeof(g_m360ClassicNormal) / sizeof(g_m360ClassicNormal[0])];
            unsigned count = 0, best = ~0u, i;
            for (i = 0; i < sizeof(g_m360ClassicNormal) / sizeof(g_m360ClassicNormal[0]); ++i) {
                const int kind = M360_FighterIndexForKind(g_m360ClassicNormal[i].fighterKind);
                unsigned previous, repeats = 0;
                if (ClassicStageIndex(i) < 0 || kind < 0 || (unsigned) kind == s_selKind[0])
                    continue;
                for (previous = 0; previous < s_campaignRound; ++previous) {
                    const unsigned used = s_classicMatchups[previous];
                    if (used == ~0u)
                        continue;
                    repeats += g_m360ClassicNormal[used].grkind == g_m360ClassicNormal[i].grkind;
                    repeats += g_m360ClassicNormal[used].fighterKind == g_m360ClassicNormal[i].fighterKind;
                }
                if (repeats < best) {
                    best = repeats;
                    count = 0;
                }
                if (repeats == best)
                    candidates[count++] = i;
            }
            if (count)
                chosen = s_classicMatchups[s_campaignRound] = candidates[HSD_Randi((int) count)];
        }
        if (chosen != ~0u) {
            s_selKind[1] = (unsigned) M360_FighterIndexForKind(g_m360ClassicNormal[chosen].fighterKind);
            s_classicStKind = g_m360ClassicNormal[chosen].stkind;
            s_selCostume[1] = 0;
            return;
        }
    }
    s_selKind[1] = rosterCount ? (s_selKind[0] + 1 + s_campaignRound) % rosterCount : 0;
    s_selCostume[1] = 0;
}

static void ResolveCostumes(void)
{
    unsigned i;
    for (i = 0; i < s_slotCount; ++i) {
        unsigned attempt;
        const unsigned colors = M360_FighterCostumeCount(s_selKind[i]);
        s_selCostume[i] %= colors;
        for (attempt = 0; attempt < colors; ++attempt) {
            unsigned j;
            for (j = 0; j < i; ++j)
                if (s_selKind[i] == s_selKind[j] && s_selCostume[i] == s_selCostume[j])
                    break;
            if (j == i)
                break;
            s_selCostume[i] = (s_selCostume[i] + 1) % colors;
        }
    }
}

static void StartFight(void)
{
    unsigned i;
    s_adventureCheckpoint = 0;
    s_campaignClearFrames = 0;
    if (IsCampaign())
        CampaignOpponent();
    if (s_gameMode == kGameModeClassic) {
        const unsigned chosen = s_campaignRound < kCampaignRounds ? s_classicMatchups[s_campaignRound] : ~0u;
        const int stage = chosen != ~0u ? ClassicStageIndex(chosen) : -1;
        if (stage < 0 || (s_builtStage != (unsigned) stage && !BuildStage((unsigned) stage)) ||
            s_stageIndex != (unsigned) stage || !MatchStageParam()) {
            M360_MatchTrace("match.classic.encounter_unavailable", s_classicStKind);
            s_active = 0;
            return;
        }
        LoadItemTable();
        M360_MatchTrace("match.classic.stkind", s_classicStKind);
        M360_MatchTrace("match.classic.opponent", s_selKind[1]);
    }
    if (s_gameMode == kGameModeAdventure) {
        const M360AdventureEncounter* encounter = AdventureEncounter();
        unsigned stage;
        if (!encounter) { s_active = 0; return; }
        for (stage = 0; stage < kStageCount; ++stage)
            if (s_stages[stage].grkind == encounter->grkind) break;
        if (stage == kStageCount || (s_builtStage != stage && !BuildStage(stage)) ||
            s_stageIndex != stage || !MatchStageParam()) {
            M360_MatchTrace("match.adventure.encounter_unavailable", encounter->scene);
            s_active = 0; return;
        }
        M360_MatchTrace("match.adventure.scene", encounter->scene);
        if (encounter->scene == 17 && !InitMaze()) {
            M360_MatchTrace("match.adventure.maze_load_failed", 1);
            s_active = 0; return;
        }
    }
    ResolveCostumes();
    s_loadFailedSlot = 0;
    s_fighterCount = 0;
    memset(s_fighters, 0, sizeof(s_fighters));
    M360_FighterEffectsInit();
    for (i = 0; i < s_slotCount; ++i) {
        const int port = s_selHuman[i] ? (int) i : -1;
        M360_FighterSelect((int) i, s_selKind[i], s_selCostume[i]);
        s_fighters[i] = M360_FighterSpawn((int) i, s_stage.spawnX[i], s_stage.spawnY[i],
                                          s_stage.spawnX[i] > 0.0f ? -1.0f : 1.0f, port);
        if (!s_fighters[i]) {
            s_loadFailedSlot = i + 1;
            M360_MatchTrace("match.load_failed.slot", i);
            M360_MatchTrace("match.load_failed.kind", s_selKind[i]);
            FreeAllGObjs();
            M360_FighterResetMatch();
            M360_AudioStopAllSfx();
            M360_HsdRenderClearTextures();
            memset(s_fighters, 0, sizeof(s_fighters));
            memset(s_selReady, 0, sizeof(s_selReady));
            s_fighterCount = 0;
            s_builtStage = ~0u;
            s_phase = kPhaseSelect;
            s_active = BuildStage(s_stageIndex) && !s_auto.enabled;
            return;
        }
        s_human[i] = port >= 0;
        s_respawn[i] = 0;
        s_stocksLost[i] = 0;
        s_stocksRemaining[i] = StartingStocks(i);
        if (s_gameMode == kGameModeAdventure && i > 0) {
            const M360AdventureEncounter* encounter = AdventureEncounter();
            /* gm_801B4768 halves the two Donkey Kongs before the giant fight. */
            M360_FighterSetEncounter(s_fighters[i], encounter->scene == 9 ? 0.5f :
                                    (encounter->flags & 2) ? 2.0f : 1.0f,
                                    (encounter->flags & 4) != 0);
        }
        s_score[i] = 0;
        if (s_fighters[i])
            s_fighterCount = i + 1;
        M360_MatchTrace("match.select.kind", s_selKind[i]);
        M360_MatchTrace("match.select.costume", s_selCostume[i]);
    }
    s_phase = kPhaseFight;
    s_frame = 0;
    s_suddenDeath = 0;
    s_draw = 0;
    /* gmvs.c: start the random item spawner once the fighters exist. */
    it_8026D018();
    {
        HSD_GObj* debug = GObj_Create(HSD_GOBJ_CLASS_STAGE, 5, 0);
        if (debug)
            GObj_SetupGXLink(debug, DebugRender, 7, 255);
    }
    /* Original in-match HUD (IfAll): damage panels, stocks, timer, "GO!". */
    M360_HudStart(s_slotCount, IsCampaign() ? s_campaignStocks : s_stocks,
                  IsCampaign() ? CampaignSeconds() : TimeMinutes() * 60u,
                  !TimeMinutes());
    CamUpdate(1);
    M360_MatchTrace("match.enter.fighters", s_fighterCount);
    TraceHeap("match.heap.used", "match.heap.largest_free");
    M360_MatchTrace("match.p2.human", s_human[1]);
}

/* Native character select before the fight: left/right picks the fighter,
 * X/Y the costume, A confirms and B steps back. With one controller P1 also
 * picks the CPU opponent after confirming its own fighter. */
static int SelectInput(unsigned port, unsigned slot)
{
    const unsigned count = M360_FighterKindCount();
    const unsigned trig = M360_MatchPadTriggeredPort(port);
    const float x = M360_MatchPadStickXPort(port);
    const int left = (trig & 0x1u) || (x < -0.7f && s_selPrevStick[port] >= -0.7f);
    const int right = (trig & 0x2u) || (x > 0.7f && s_selPrevStick[port] <= 0.7f);
    s_selPrevStick[port] = x;
    if (s_selReady[slot]) {
        if (trig & 0x200u) {
            s_selReady[slot] = 0;
            M360_MatchTrace("match.select.unready", slot);
        }
        return 0;
    }
    if (left && count)
        s_selKind[slot] = (s_selKind[slot] + count - 1) % count;
    if (right && count)
        s_selKind[slot] = (s_selKind[slot] + 1) % count;
    s_selCostume[slot] %= M360_FighterCostumeCount(s_selKind[slot]);
    if (trig & 0x400u)
        s_selCostume[slot] = (s_selCostume[slot] + 1) % M360_FighterCostumeCount(s_selKind[slot]);
    if (trig & 0x800u)
        s_selCostume[slot] = (s_selCostume[slot] + M360_FighterCostumeCount(s_selKind[slot]) - 1) % M360_FighterCostumeCount(s_selKind[slot]);
    if (port == 0 && !IsCampaign() && (trig & 0xCu)) {
        const unsigned stageCount = M360_MatchStageCount();
        const unsigned next = (s_stageIndex + ((trig & 0x8u) ? stageCount - 1 : 1)) % stageCount;
        if (BuildStage(next))
            M360_MatchTrace("match.select.stage", s_stageIndex);
    }
    if (port == 0 && (trig & 0x40u)) {
        if (s_gameMode == kGameModeAdventure)
            s_cpuLevel = s_cpuLevel >= 9 ? 1 : ((s_cpuLevel ? s_cpuLevel - 1 : 0) / 2 + 1) * 2 + 1;
        else
            s_cpuLevel = s_cpuLevel >= 9 ? 1 : s_cpuLevel + 1;
        M360_FighterSetCpuLevel(s_cpuLevel);
        M360_MatchTrace("match.select.cpu_level", s_cpuLevel);
    }
    if (port == 0 && !IsCampaign() && (trig & 0x10u)) {
        s_stocks = s_stocks >= 9 ? 1 : s_stocks + 1;
        M360_MatchTrace("match.select.stocks", s_stocks);
    }
    if (port == 0 && !IsCampaign()) {
        /* Right stick up/down switches between stock and timed rules. */
        const float sy = M360_MatchPadSubStickYPort(0);
        const unsigned prevOption = s_timeOption;
        if (sy > 0.7f && s_selPrevSub <= 0.7f)
            s_timeOption = (s_timeOption + 1) % kTimeOptionCount;
        else if (sy < -0.7f && s_selPrevSub >= -0.7f)
            s_timeOption = (s_timeOption + kTimeOptionCount - 1) % kTimeOptionCount;
        if (s_timeOption != prevOption)
            M360_MatchTrace("match.select.time", kTimeOptions[s_timeOption]);
        s_selPrevSub = sy;
    }
    if (port == 0 && !IsCampaign() && (trig & 0x1000u)) {
        s_itemFreq = s_itemFreq >= 4 ? -1 : s_itemFreq + 1;
        M360_MatchTrace("match.select.items", (unsigned) (s_itemFreq + 1));
    }
    if (port == 0 && !IsCampaign() && (trig & 0x20u)) {
        unsigned i;
        s_slotCount = s_slotCount >= kMaxFighters ? 2 : s_slotCount + 1;
        memset(s_selReady, 0, sizeof(s_selReady));
        for (i = s_slotCount; i < kMaxFighters; ++i)
            s_selHuman[i] = 0;
        M360_MatchTrace("match.select.players", s_slotCount);
    }
    if (trig & 0x100u) {
        s_selReady[slot] = 1;
        M360_MatchTrace("match.select.ready", slot);
    } else if (trig & 0x200u) {
        unsigned prev;
        if (slot == 0)
            return 1;
        /* P1 editing a CPU slot steps back to the previous P1-picked slot. */
        for (prev = slot - 1; prev > 0 && s_selHuman[prev]; --prev)
            ;
        if (port == 0)
            s_selReady[prev] = 0;
    }
    return 0;
}

/* The slot P1 edits: itself, then each unready CPU slot in order. */
static unsigned P1Slot(void)
{
    unsigned i;
    if (!s_selReady[0] || IsCampaign())
        return 0;
    for (i = 1; i < s_slotCount; ++i)
        if (!s_selHuman[i] && !s_selReady[i])
            return i;
    return 0;
}

static int SelectFrame(void)
{
    unsigned i;
    int allReady;
    /* Pads are read by the frame loop, so a second controller is probed on
     * the first select frame rather than at match entry. */
    if (s_selProbeP2) {
        s_selProbeP2 = 0;
        if (M360_MatchControllerConnected(1))
            s_selHuman[1] = 1;
    }
    for (i = 1; i < kMaxFighters && !IsCampaign(); ++i) {
        if (s_selHuman[i] && !M360_MatchControllerConnected(i)) {
            s_selHuman[i] = 0;
            s_selReady[i] = 0;
            M360_MatchTrace("match.select.disconnect", i);
        }
        if (!s_selHuman[i] && M360_MatchControllerConnected(i) &&
            (M360_MatchPadTriggeredPort(i) & 0x1F00u)) {
            s_selHuman[i] = 1;
            s_selReady[i] = 0;
            if (s_slotCount < i + 1)
                s_slotCount = i + 1;
            M360_MatchTrace("match.select.join", i);
            return M360_MATCH_CONTINUE;
        }
    }
    if (SelectInput(0, P1Slot()))
        return M360_MATCH_TO_MENU;
    for (i = 1; i < s_slotCount; ++i)
        if (s_selHuman[i])
            SelectInput(i, i);
    for (i = 0; i < s_modelCount; ++i)
        HSD_JObjAnimAll(s_models[i]);
    UpdateMapColl();
    if (s_lobj)
        HSD_LObjAnimAll(s_lobj);
    CamUpdate(0);
    allReady = s_selReady[0];
    for (i = 1; i < s_slotCount && !IsCampaign(); ++i)
        if (!s_selReady[i])
            allReady = 0;
    if (allReady)
        StartFight();
    return M360_MATCH_CONTINUE;
}

static int OutsideBlastZone(float x, float y)
{
    return x > s_stage.blastRight + s_stage.camX || x < s_stage.blastLeft + s_stage.camX ||
           y > s_stage.blastTop + s_stage.camY || y < s_stage.blastBottom + s_stage.camY;
}

/* Ground's authored finish markers occupy 0x99..0xB2. KinokoRoute's
 * Yoshi encounter is at event marker 0xBD, before the finish. */
static void HideMapJoint(int mapId, int jointIndex)
{
    unsigned i;
    for (i = 0; i < s_modelCount; ++i) {
        if (s_modelIds[i] == mapId) {
            HSD_JObj* joint = FindCollJoint(s_models[i], jointIndex);
            if (joint) HSD_JObjSetFlagsAll(joint, JOBJ_HIDDEN);
            return;
        }
    }
}

static int CollisionGroupHasFloor(unsigned group, int line)
{
    const MapJoint* joint;
    if (!s_coll || group >= (unsigned) s_coll->joint_count || line < 0) return 0;
    joint = &s_coll->joints[group];
    return line >= joint->floor_start && line < joint->floor_start + joint->floor_count;
}

static void MazeMapJointVisible(int mapId, int jointIndex, int visible)
{
    unsigned i;
    for (i = 0; i < s_modelCount; ++i) {
        if (s_modelIds[i] == mapId) {
            HSD_JObj* joint = FindCollJoint(s_models[i], jointIndex);
            if (joint) {
                if (visible) HSD_JObjClearFlagsAll(joint, JOBJ_HIDDEN);
                else HSD_JObjSetFlagsAll(joint, JOBJ_HIDDEN);
            }
            return;
        }
    }
}

static void MazeRoomBounds(int room)
{
    static const int joints[6] = { 19, 20, 18, 17, 16, 15 };
    unsigned i;
    for (i = 0; i < 14; ++i) {
        /* grShrineRoute restores only traversal joints 0-7 after a fight.
         * Other arena joints stay disabled until their encounter begins. */
        if (room >= 0)
            SetCollisionGroupEnabled(i, i == 8u + (unsigned) room);
        else if (i < 8)
            SetCollisionGroupEnabled(i, 1);
    }
    if (room < 0) {
        /* grShrineRoute_8020B0AC opens these authored traversal walls. */
        static const unsigned walls[7] = { 51, 79, 101, 102, 115, 116, 131 };
        for (i = 0; i < 7; ++i)
            if (walls[i] < s_stage.lineCount) s_stage.lines[walls[i]].kind = 0;
    }
    MazeMapJointVisible(4, 1, room < 0);
    MazeMapJointVisible(2, 0, room < 0);
    for (i = 0; i < 6; ++i) {
        MazeMapJointVisible(4, joints[i], room < 0 || i == (unsigned) room);
        if (s_mazeSymbols[i]) {
            if (room >= 0 || (s_mazeVisited & (1u << i)))
                HSD_JObjSetFlagsAll(s_mazeSymbols[i], JOBJ_HIDDEN);
            else HSD_JObjClearFlagsAll(s_mazeSymbols[i], JOBJ_HIDDEN);
        }
    }
    if (room < 0) {
        s_stage.camX = s_stage.camY = 0.0f;
        s_stage.camLeft = s_routeCamera[0]; s_stage.camRight = s_routeCamera[1];
        s_stage.camBottom = s_routeCamera[2]; s_stage.camTop = s_routeCamera[3];
        s_stage.blastLeft = s_routeBlast[0]; s_stage.blastRight = s_routeBlast[1];
        s_stage.blastBottom = s_routeBlast[2]; s_stage.blastTop = s_routeBlast[3];
        s_stage.rebirthX[0] = s_stage.spawnX[0];
        s_stage.rebirthY[0] = s_stage.spawnY[0] + 40.0f;
    } else {
        s_stage.camX = s_mazePoints[room].x;
        s_stage.camY = s_mazePoints[room].y + 30.0f;
        s_stage.camLeft = s_routeFightCamera[0]; s_stage.camRight = s_routeFightCamera[1];
        s_stage.camBottom = s_routeFightCamera[2]; s_stage.camTop = s_routeFightCamera[3];
        s_stage.blastLeft = s_routeFightBlast[0]; s_stage.blastRight = s_routeFightBlast[1];
        s_stage.blastBottom = s_routeFightBlast[2]; s_stage.blastTop = s_routeFightBlast[3];
        s_stage.rebirthX[0] = s_mazeSpawns[room].x;
        s_stage.rebirthY[0] = s_mazeSpawns[room].y - 10.0f;
    }
    M360_FighterBuildIslands();
}

static int InitMaze(void)
{
    const float* params = (const float*) M360_ArchiveFind(s_archive, "yakumono_param");
    float symbolScale = params ? params[5] : 1.5f;
    unsigned i;
    /* Ground_801C3DB4 stores half of the authored trigger dimensions. */
    s_mazeRadiusX = params ? params[7] * s_scale * 0.5f : 35.0f;
    s_mazeRadiusY = params ? params[8] * s_scale * 0.5f : 35.0f;
    s_mazeGoal = (unsigned) HSD_Randi(6);
    s_mazeVisited = s_mazeFinishFrames = 0;
    s_mazeRoom = -1;
    for (i = 0; i < 6; ++i)
        if (!PointPosition(0xBD + i, &s_mazePoints[i]) ||
            !PointPosition(0xB3 + i, &s_mazeSpawns[i])) return 0;
    for (i = 0; i < 6; ++i) {
        HSD_JObj* symbol = CreateMapGObj(&s_stages[s_stageIndex], i == s_mazeGoal ? 3 : 1);
        if (!symbol) return 0;
        s_mazeSymbols[i] = symbol;
        HSD_JObjSetTranslate(symbol, &s_mazePoints[i]);
        HSD_JObjSetScaleX(symbol, symbolScale);
        HSD_JObjSetScaleY(symbol, symbolScale);
        HSD_JObjSetScaleZ(symbol, symbolScale);
    }
    MazeRoomBounds(-1);
    M360_MatchTrace("match.adventure.maze.goal_room", s_mazeGoal);
    M360_MatchTrace("match.adventure.maze.symbols", 6);
    return 1;
}

unsigned M360_MatchMazeVisited(void) { return s_mazeVisited; }

int M360_MatchMazePoint(unsigned room, float* x, float* y)
{
    if (room >= 6 || !AdventureEncounter() || AdventureEncounter()->scene != 17) return 0;
    *x = s_mazePoints[room].x; *y = s_mazePoints[room].y;
    return 1;
}

static void MazeFrame(float x, float y)
{
    unsigned i;
    if (s_mazeFinishFrames) {
        if (++s_mazeFinishFrames >= 60) {
            s_campaignClearFrames = M360_HudFightFrames();
            s_matchOver = 1; s_winner = 0; s_draw = 0;
            M360_HudGameEnd(0);
            M360_MatchTrace("match.adventure.maze_goal", s_frame);
        }
        return;
    }
    if (s_mazeRoom >= 0) {
        if (!s_stocksRemaining[1]) {
            s_mazeVisited |= 1u << s_mazeRoom;
            HSD_JObjSetFlagsAll(s_mazeSymbols[s_mazeRoom], JOBJ_HIDDEN);
            s_respawn[1] = 0;
            M360_MatchTrace("match.adventure.maze.link_defeated", (unsigned) s_mazeRoom);
            s_mazeRoom = -1;
            MazeRoomBounds(-1);
        }
        return;
    }
    for (i = 0; i < 6; ++i) {
        if ((s_mazeVisited & (1u << i)) ||
            fabsf(x - s_mazePoints[i].x) >= s_mazeRadiusX ||
            fabsf(y - s_mazePoints[i].y) >= s_mazeRadiusY) continue;
        if (i == s_mazeGoal) {
            s_mazeFinishFrames = 1;
            M360_MatchTrace("match.adventure.maze.triforce", i);
            return;
        }
        s_mazeRoom = (int) i;
        MazeRoomBounds((int) i);
        s_slotCount = 2;
        s_stage.spawnX[1] = s_mazeSpawns[i].x;
        s_stage.spawnY[1] = s_mazeSpawns[i].y;
        if (!s_fighters[1]) {
            unsigned colors = M360_FighterCostumeCount(s_selKind[1]);
            unsigned costume = s_selKind[0] == s_selKind[1] && colors > 1 ?
                (s_selCostume[0] + 1) % colors : 0;
            M360_FighterSelect(1, s_selKind[1], costume);
            s_fighters[1] = M360_FighterSpawn(1, s_mazeSpawns[i].x, s_mazeSpawns[i].y, -1.0f, -1);
            if (!s_fighters[1]) {
                s_active = 0;
                M360_MatchTrace("match.adventure.maze.link_load_failed", i);
                return;
            }
            s_fighterCount = 2;
            s_human[1] = 0;
            M360_HudAddFighter(1);
            M360_HudRefreshFighterTags();
        } else {
            M360_FighterRespawn(s_fighters[1], s_mazeSpawns[i].x, s_mazeSpawns[i].y);
        }
        s_stocksRemaining[1] = 1;
        s_stocksLost[1] = s_respawn[1] = 0;
        M360_MatchTrace("match.adventure.maze.link_spawned", i);
        return;
    }
}

static void AdventureFrame(void)
{
    const M360AdventureEncounter* encounter = AdventureEncounter();
    float x, y, facing;
    unsigned motion, damage, i;
    Vec3 point;
    if (!encounter || !s_fighters[0] ||
        !s_stocksRemaining[0] || s_respawn[0] || s_matchOver) return;
    if (encounter->scene == 27) {
        /* grZebesRoute's fn_8020B4D8 clears the escape when P1 lands on
         * collision group 1, the top exit platform, before the timer ends. */
        if (CollisionGroupHasFloor(1, M360_FighterFloorLine(s_fighters[0]))) {
            s_campaignClearFrames = M360_HudFightFrames();
            s_matchOver = 1; s_winner = 0; s_draw = 0;
            M360_HudGameEnd(0);
            M360_MatchTrace("match.adventure.escape_goal", s_frame);
        }
        return;
    }
    if (encounter->scene == 17) {
        M360_FighterGetState(s_fighters[0], &x, &y, &facing, &motion, &damage);
        MazeFrame(x, y);
        return;
    }
    if (encounter->scene == 58) {
        M360_FighterGetState(s_fighters[0], &x, &y, &facing, &motion, &damage);
        /* grBigBlueRoute_8020BF38 advances rebirth points 4-7 by crossing
         * points 5-7, independent of height. Preserve progress after a fall. */
        if (s_adventureCheckpoint < 3 && PointPosition(5 + s_adventureCheckpoint, &point) && x > point.x) {
            ++s_adventureCheckpoint;
            s_stage.rebirthX[0] = point.x; s_stage.rebirthY[0] = point.y;
            M360_MatchTrace("match.adventure.race.checkpoint", (unsigned) s_adventureCheckpoint);
        }
        /* Ground_801C3D44(0,30,4000) stores 15/2000 half extents. */
        if (PointPosition(0x99, &point) && fabsf(x - point.x) < 15.0f && fabsf(y - point.y) < 2000.0f) {
            s_campaignClearFrames = M360_HudFightFrames();
            s_matchOver = 1; s_winner = 0; s_draw = 0;
            M360_HudGameEnd(0);
            M360_MatchTrace("match.adventure.race_goal", s_frame);
        }
        return;
    }
    if (encounter->scene != 1) return;
    M360_FighterGetState(s_fighters[0], &x, &y, &facing, &motion, &damage);
    if (!s_adventureCheckpoint && PointPosition(0xBD, &point) &&
        fabsf(x - point.x) < 30.0f && fabsf(y - point.y) < 5000.0f) {
        s_adventureCheckpoint = 1;
        s_stage.camX = point.x; s_stage.camY = point.y + 30.0f;
        s_stage.camLeft = s_routeFightCamera[0]; s_stage.camRight = s_routeFightCamera[1];
        s_stage.camBottom = s_routeFightCamera[2]; s_stage.camTop = s_routeFightCamera[3];
        s_stage.blastLeft = s_routeFightBlast[0]; s_stage.blastRight = s_routeFightBlast[1];
        s_stage.blastBottom = s_routeFightBlast[2]; s_stage.blastTop = s_routeFightBlast[3];
        for (i = 1; i < 4; ++i) {
            M360_FighterSelect((int) i, s_selKind[i], i - 1);
            s_fighters[i] = M360_FighterSpawn((int) i, point.x + (float) ((int) i - 2) * 20.0f,
                                             point.y + 30.0f, -1.0f, -1);
            if (!s_fighters[i]) {
                s_active = 0;
                M360_MatchTrace("match.adventure.checkpoint_load_failed", i);
                return;
            }
            s_human[i] = 0;
            s_respawn[i] = s_stocksLost[i] = 0;
            s_stocksRemaining[i] = s_adventureLives[i];
            s_fighterCount = i + 1;
            M360_HudAddFighter(i);
        }
        s_slotCount = 4;
        M360_HudRefreshFighterTags();
        s_stage.rebirthX[0] = point.x;
        s_stage.rebirthY[0] = point.y + 50.0f;
        M360_MatchTrace("match.adventure.checkpoint", 1);
    }
    if (s_adventureCheckpoint == 1 &&
        !s_stocksRemaining[1] && !s_stocksRemaining[2] && !s_stocksRemaining[3]) {
        s_adventureCheckpoint = 2;
        /* grKinokoRoute_8020836C hides the gate joint as well as disabling
         * its collision. Keep the opened passage visible to the player. */
        HideMapJoint(3, 0x53);
        s_stage.camX = s_stage.camY = 0.0f;
        s_stage.camLeft = s_routeCamera[0]; s_stage.camRight = s_routeCamera[1];
        s_stage.camBottom = s_routeCamera[2]; s_stage.camTop = s_routeCamera[3];
        s_stage.blastLeft = s_routeBlast[0]; s_stage.blastRight = s_routeBlast[1];
        s_stage.blastBottom = s_routeBlast[2]; s_stage.blastTop = s_routeBlast[3];
        DisableCollisionGroup(0x3C); DisableCollisionGroup(0x33);
        for (i = 0x0C; i <= 0x0F; ++i) DisableCollisionGroup(i);
        M360_MatchTrace("match.adventure.checkpoint", 2);
    }
    if (s_adventureCheckpoint != 2) return;
    for (i = 0x99; i < 0xB3; ++i) {
        if (PointPosition((int) i, &point) && fabsf(x - point.x) < 15.0f &&
            fabsf(y - point.y) < 5000.0f) {
            unsigned total = CampaignSeconds() * 60u;
            unsigned played = M360_HudFightFrames();
            unsigned remaining = played < total ? (total - played) / 60u : 0;
            /* gm_801B44A0: the displayed seconds digit selects Luigi. */
            s_adventureLuigi = remaining % 10u == 2;
            M360_MatchTrace("match.adventure.luigi_selected", (unsigned) s_adventureLuigi);
            s_campaignClearFrames = played;
            s_matchOver = 1; s_winner = 0; s_draw = 0;
            M360_HudGameEnd(0);
            M360_MatchTrace("match.adventure.goal", i);
            return;
        }
    }
}

int M360_MatchFrame(void)
{
    unsigned i;
    unsigned pausePort = 0;
    unsigned buttons = M360_MatchPadTriggered();
    const unsigned held = M360_MatchPadHeld();
    if (!s_active)
        return M360_MATCH_TO_MENU;
    if (s_phase == kPhaseSelect)
        return SelectFrame();
    if (s_matchOver && !M360_HudGameEndDone()) {
        /* "GAME!"/"TIME!" plays with the fighters frozen before the result. */
        ++s_frame;
        M360_HudFrame(s_frame);
        for (i = 0; i < s_modelCount; ++i)
            HSD_JObjAnimAll(s_models[i]);
        HSD_GObj_RunProcs();
        CamUpdate(0);
        return M360_MATCH_CONTINUE;
    }
    if (s_matchOver) {
        if (s_auto.enabled && !IsCampaign() && ++s_autoResultFrames == 120) {
            ++s_autoPlayed;
            TraceHeap("auto.end.heap.used", "auto.end.heap.largest_free");
            M360_MatchTrace("auto.match.done", s_autoPlayed);
            if (s_autoPlayed < s_auto.repeat)
                return M360_MATCH_RESTART;
            M360_MatchTrace("auto.all.done", s_autoPlayed);
        }
        if (buttons & 0x200u) {
            M360_MatchTrace("match.result.return_menu", s_winner);
            return M360_MATCH_TO_MENU;
        }
        if ((buttons & 0x100u) && !s_draw && s_winner == 0 &&
            (s_gameMode == kGameModeClassic || s_gameMode == kGameModeAdventure))
            return M360_MATCH_NEXT_ROUND;
        if ((buttons & 0x100u) && s_campaignTimedOut && s_campaignStocks) {
            s_campaignRetryPending = 1;
            return M360_MATCH_RESTART;
        }
        if ((buttons & 0x100u) && IsCampaign() && (s_draw || s_winner != 0)) {
            if (!s_campaignStocks) s_campaignStocks = kStartingStocks;
            s_campaignRetryPending = 1;
            M360_MatchTrace("match.campaign.continue", s_campaignRound);
            return M360_MATCH_RESTART;
        }
        if ((buttons & 0x100u) && !IsCampaign()) {
            for (i = 0; i < s_slotCount; ++i)
                s_selHuman[i] = s_human[i];
            s_rematchPending = 1;
            M360_MatchTrace("match.result.rematch", s_slotCount);
            return M360_MATCH_RESTART;
        }
        return M360_MATCH_CONTINUE;
    }
    s_disconnectedControllers = 0;
    for (i = 0; i < s_fighterCount; ++i) {
        if (!s_human[i])
            continue;
        if (!M360_MatchControllerConnected(i))
            s_disconnectedControllers |= 1u << i;
        if (i > 0) {
            const unsigned portButtons = M360_MatchPadTriggeredPort(i);
            if ((portButtons & 0x1000u) && !(buttons & 0x1000u))
                pausePort = i;
            buttons |= portButtons;
        }
    }
    if (s_disconnectedControllers && !s_paused && M360_HudFightStarted()) {
        s_paused = 1;
        M360_HudPause(1, 0);
        M360_MatchTrace("match.controller.disconnected", s_disconnectedControllers);
    } else if ((buttons & 0x1000u) && !s_disconnectedControllers && M360_HudFightStarted()) {
        s_paused = !s_paused;
        M360_HudPause(s_paused, (int) pausePort);
        M360_MatchTrace("match.pause", s_paused);
    }
    if (M360_InputScriptHolding()) {
        if (!s_holdTraced) {
            M360MatchStatus st;
            s_holdTraced = 1;
            M360_MatchGetStatus(&st);
            M360_MatchTrace("snap.frame", st.frame);
            for (i = 0; i < 2; ++i) {
                M360_MatchTrace(i ? "snap.p2.motion" : "snap.p1.motion", st.motion[i]);
                M360_MatchTrace(i ? "snap.p2.x" : "snap.p1.x", (unsigned) (int) st.posX[i]);
                M360_MatchTrace(i ? "snap.p2.y" : "snap.p1.y", (unsigned) (int) st.posY[i]);
                M360_MatchTrace(i ? "snap.p2.damage" : "snap.p1.damage", st.damage[i]);
                M360_MatchTrace(i ? "snap.p2.stocks" : "snap.p1.stocks", st.stocksRemaining[i]);
                if (s_fighters[i]) {
                    float fx, fy, facing;
                    unsigned fm, fd;
                    M360_FighterGetState(s_fighters[i], &fx, &fy, &facing, &fm, &fd);
                    M360_MatchTrace(i ? "snap.p2.facing_neg" : "snap.p1.facing_neg", facing < 0.0f);
                    if (!s_human[i])
                        M360_FighterTraceCpu(s_fighters[i]);
                }
            }
        }
        return M360_MATCH_CONTINUE;
    }
    s_holdTraced = 0;
    if (s_paused) {
        if (buttons & 0x400u) {
            s_debugHitboxes = !s_debugHitboxes;
            M360_MatchTrace("match.debug.hitboxes", (unsigned) s_debugHitboxes);
        }
        if ((buttons & 0x200u) || ((held & 0x160u) == 0x160u && (buttons & 0x100u))) {
            M360_MatchTrace("match.exit.menu", s_frame);
            return M360_MATCH_TO_MENU;
        }
        return M360_MATCH_CONTINUE;
    }
    ++s_frame;
    M360_HudFrame(s_frame);
    for (i = 0; i < s_modelCount; ++i)
        HSD_JObjAnimAll(s_models[i]);
    UpdateMapColl();
    if (s_collBindCount)
        M360_FighterFollowFloors();
    if (s_lobj)
        HSD_LObjAnimAll(s_lobj);
    for (i = 1; i < s_fighterCount && !IsCampaign(); ++i) {
        if (s_fighters[i] && !s_human[i] && M360_MatchControllerConnected(i) &&
            (M360_MatchPadTriggeredPort(i) & 0x1F00u)) {
            s_human[i] = 1;
            M360_FighterSetPort(s_fighters[i], (int) i);
            M360_MatchTrace("match.join", i);
        }
    }
    g_m360Crumb = 1;
    HSD_GObj_RunProcs();
    g_m360Crumb = 2;
    for (i = 0; i < s_fighterCount; ++i) {
        float x, y, facing;
        unsigned motion, damage;
        if (!s_fighters[i])
            continue;
        /* An eliminated slot stays out. Its dead pose can remain outside
         * the blast zone while the other players continue fighting. */
        if (!TimeMinutes() && !s_stocksRemaining[i])
            continue;
        /* Zelda/Sheik swap the slot's active fighter (Player_GetEntity). */
        if (M360_FighterActive((int) i))
            s_fighters[i] = M360_FighterActive((int) i);
        if (M360_FighterFollower((int) i)) {
            /* Nana is lost on her own when she leaves the blast zone. */
            void* nana = M360_FighterFollower((int) i);
            M360_FighterGetState(nana, &x, &y, &facing, &motion, &damage);
            if (OutsideBlastZone(x, y))
                M360_FighterSleep(nana);
        }
        if (s_respawn[i]) {
            if (--s_respawn[i] == 0 && (s_stocksRemaining[i] || TimeMinutes())) {
                const M360AdventureEncounter* encounter = AdventureEncounter();
                Vec3 point;
                if (i && encounter && encounter->scene == 1 && PointPosition(0xBD, &point)) {
                    /* Replace a defeated route enemy without the player's
                     * invulnerable rebirth platform. Three slots carry ten
                     * opponents until the original generator is integrated. */
                    M360_FighterRespawn(s_fighters[i], point.x + ((int) i - 2) * 20.0f,
                                       point.y + 30.0f);
                    M360_MatchTrace("match.adventure.enemy_replaced", i);
                } else if (i && encounter && encounter->opponents > 3) {
                    /* Team encounters also reuse slots for later enemies.
                     * Bring them into the arena, without a player platform. */
                    M360_FighterRespawn(s_fighters[i], s_stage.spawnX[i], s_stage.spawnY[i]);
                    M360_MatchTrace("match.adventure.wave_replaced", i);
                } else {
                    M360_FighterRebirth(s_fighters[i]);
                }
            }
            continue;
        }
        M360_FighterGetState(s_fighters[i], &x, &y, &facing, &motion, &damage);
        if (OutsideBlastZone(x, y)) {
            ++s_stocksLost[i];
            if (TimeMinutes()) {
                const int by = M360_FighterLastAttacker(s_fighters[i]);
                if (by >= 0 && (unsigned) by < s_fighterCount)
                    ++s_score[by];
                --s_score[i];
                s_respawn[i] = kRespawnFrames;
                M360_FighterSetDead(s_fighters[i]);
                M360_HudStockLost(i);
                M360_MatchTrace("match.time.ko_by", (unsigned) by);
                continue;
            }
            LoseStock(i);
            s_respawn[i] = s_stocksRemaining[i] ? kRespawnFrames : 0;
            M360_FighterSetDead(s_fighters[i]);
            M360_HudStockLost(i);
            M360_MatchTrace("match.blast_zone.fighter", i);
            M360_MatchTrace("match.stocks.remaining", s_stocksRemaining[i]);
        }
    }
    /* Resolve after every slot has been checked, so two final falls in the
     * same frame do not declare an already-eliminated player the winner. */
    AdventureFrame();
    if (!s_matchOver && !TimeMinutes()) {
        if (StockResult(&s_winner, &s_draw)) {
            s_campaignClearFrames = M360_HudFightFrames();
            s_matchOver = 1;
            M360_HudGameEnd(0);
            M360_MatchTrace(s_draw ? "match.result.draw" : "match.result.winner", s_winner);
            if (!s_draw && s_winner == 0 && IsCampaign() &&
                M360_MatchNextCampaignRound() >= M360_MatchCampaignRounds(s_gameMode))
                M360_MatchTrace("match.campaign.preview_complete", s_gameMode);
        }
    }
    if (!s_matchOver && IsCampaign() &&
        M360_HudFightFrames() >= CampaignSeconds() * 60u) {
        LoseStock(0);
        s_campaignClearFrames = M360_HudFightFrames();
        s_campaignTimedOut = 1;
        s_matchOver = 1;
        s_winner = 1;
        s_draw = 0;
        M360_HudGameEnd(1);
        M360_MatchTrace("match.classic.timeout.lives", s_campaignStocks);
    }
    if (TimeMinutes() && M360_HudFightFrames() >= TimeMinutes() * 60u * 60u) {
        unsigned best = 0;
        s_draw = 0;
        for (i = 1; i < s_fighterCount; ++i) {
            if (!s_fighters[i])
                continue;
            if (s_score[i] > s_score[best]) {
                best = i;
                s_draw = 0;
            } else if (s_score[i] == s_score[best]) {
                s_draw = 1;
            }
        }
        M360_MatchTrace("match.time.over", s_draw);
        if (s_draw) {
            /* Sudden death: the tied players return with one stock at 300%,
             * everyone else is out; the stock rules decide the winner. */
            const int top = s_score[best];
            unsigned n = 0;
            s_suddenDeath = 1;
            s_draw = 0;
            for (i = 0; i < s_fighterCount; ++i) {
                if (!s_fighters[i])
                    continue;
                s_respawn[i] = 0;
                if (s_score[i] == top) {
                    s_stocksRemaining[i] = 1;
                    M360_FighterRespawn(s_fighters[i], s_stage.spawnX[n & 3], s_stage.spawnY[n & 3]);
                    M360_FighterSetDamage(s_fighters[i], 300.0f);
                    ++n;
                } else {
                    s_stocksRemaining[i] = 0;
                    M360_FighterSetDead(s_fighters[i]);
                }
            }
            M360_MatchTrace("match.sudden_death", n);
        } else {
            s_matchOver = 1;
            s_winner = best;
            M360_HudGameEnd(1);
            M360_MatchTrace("match.result.winner", s_winner);
        }
    }
    if (s_frame % 300 == 0) {
        TraceHeap("match.heap.used", "match.heap.largest_free");
        for (i = 0; i < s_fighterCount; ++i)
            if (s_fighters[i] && !s_human[i])
                M360_FighterTraceCpu(s_fighters[i]);
    }
    CamUpdate(0);
    return M360_MATCH_CONTINUE;
}

void M360_MatchRender(void)
{
    g_m360Crumb = 4;
    if (s_active)
        HSD_GObj_80390FC0();
    g_m360Crumb = 5;
}

void M360_MatchLeave(void)
{
    const M360AdventureEncounter* encounter = AdventureEncounter();
    /* gm_8017D7AC excludes flag-0x80 events from total Adventure time.
     * Saturation preserves eligibility without wrapping on long retries. */
    if (s_active && s_matchOver && encounter && !(encounter->flags & 0x80)) {
        if (s_campaignClearFrames >= 64800u ||
            s_adventureElapsedFrames >= 64800u - s_campaignClearFrames)
            s_adventureElapsedFrames = 64800u;
        else
            s_adventureElapsedFrames += s_campaignClearFrames;
    }
    FreeAllGObjs();
    s_builtStage = ~0u;
    memset(s_fighters, 0, sizeof(s_fighters));
    s_fighterCount = 0;
    s_modelCount = 0;
    s_active = 0;
}

void M360_MatchSetMode(unsigned gameMode, unsigned round)
{
    s_gameMode = gameMode;
    s_campaignRound = round;
    if (!IsCampaign() && s_stageIndex >= M360_MatchStageCount()) s_stageIndex = 0;
    if (IsCampaign() && round == 0 && !s_campaignRetryPending) {
        unsigned i;
        s_campaignStocks = kStartingStocks;
        s_adventureLuigi = 0;
        s_adventureElapsedFrames = 0;
        s_classicStKind = ~0u;
        for (i = 0; i < kCampaignRounds; ++i)
            s_classicMatchups[i] = ~0u;
    }
}

void M360_MatchGetStatus(M360MatchStatus* status)
{
    unsigned i;
    memset(status, 0, sizeof(*status));
    status->loaded = (unsigned) s_loaded;
    status->paused = (unsigned) s_paused;
    status->disconnectedControllers = s_disconnectedControllers;
    status->campaignTimedOut = (unsigned) s_campaignTimedOut;
    if (s_gameMode == kGameModeAdventure && s_campaignRound == 0)
        status->campaignObjective = (unsigned) s_adventureCheckpoint + 1;
    if (AdventureEncounter() && AdventureEncounter()->scene == 27)
        status->campaignObjective = 4;
    if (AdventureEncounter() && AdventureEncounter()->scene == 17)
        status->campaignObjective = s_mazeRoom < 0 ? 5 : 6;
    if (AdventureEncounter() && AdventureEncounter()->scene == 58)
        status->campaignObjective = 7;
    if (IsCampaign())
        for (i = 1; i < s_fighterCount; ++i) status->campaignEnemies += s_stocksRemaining[i];
    if (IsCampaign()) {
        const unsigned played = s_phase == kPhaseSelect ? 0 : M360_HudFightFrames();
        const unsigned total = CampaignSeconds() * 60u;
        status->campaignTimeLeft = played < total ? (total - played + 59u) / 60u : 0;
    }
    status->frame = s_frame;
    status->fighters = s_fighterCount;
    status->hits = M360_FighterHitCount();
    /* The result shows once "GAME!"/"TIME!" has finished. */
    status->matchOver = (unsigned) (s_matchOver && M360_HudGameEndDone());
    status->winner = s_winner;
    status->gameMode = s_gameMode;
    status->campaignRound = s_campaignRound;
    status->campaignRounds = M360_MatchCampaignRounds(s_gameMode);
    status->inputButtons = M360_MatchPadHeld();
    status->inputTriggered = M360_MatchPadTriggered();
    status->inputX = M360_MatchPadX();
    status->inputY = M360_MatchPadY();
    status->selecting = s_active && s_phase == kPhaseSelect;
    status->stageIndex = s_stageIndex;
    status->stocks = IsCampaign() ? s_campaignStocks : s_stocks;
    status->cpuLevel = s_cpuLevel;
    status->itemFreq = (unsigned) (s_itemFreq + 1);
    status->timeMinutes = TimeMinutes();
    if (status->timeMinutes && s_phase != kPhaseSelect) {
        const unsigned total = status->timeMinutes * 3600u;
        const unsigned played = M360_HudFightFrames();
        status->timeLeft = played < total ? (total - played + 59u) / 60u : 0;
    } else {
        status->timeLeft = status->timeMinutes * 60u;
    }
    status->draw = (unsigned) s_draw;
    status->suddenDeath = (unsigned) s_suddenDeath;
    status->debugHitboxes = (unsigned) s_debugHitboxes;
    status->loadFailedSlot = s_loadFailedSlot;
    for (i = 0; i < kMaxFighters; ++i) {
        status->selectKind[i] = s_selKind[i];
        status->selectCostume[i] = s_selCostume[i];
        status->selectReady[i] = s_selReady[i];
        status->selectHuman[i] = (unsigned) s_selHuman[i];
        status->fighterKind[i] = s_fighters[i] ? M360_FighterKindIndex(s_fighters[i]) : s_selKind[i];
    }
    status->slotCount = s_slotCount;
    for (i = 0; i < s_fighterCount && i < kMaxFighters; ++i) {
        float facing;
        if (!s_fighters[i])
            continue;
        M360_FighterGetState(s_fighters[i], &status->posX[i], &status->posY[i], &facing,
                             &status->motion[i], &status->damage[i]);
        status->stocksLost[i] = s_stocksLost[i];
        status->stocksRemaining[i] = s_stocksRemaining[i];
        status->score[i] = s_score[i];
        status->human[i] = (unsigned) s_human[i];
    }
}

const M360MatchStage* M360_MatchStageData(void)
{
    return &s_stage;
}
