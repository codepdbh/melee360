#include <math.h>
#include <stdarg.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_Damage.h>
#include <melee/pl/player.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Ottotto.h>
#include <melee/cm/types.h>
#include <melee/ft/kinds/ftMario/ftmario.h>
#include <melee/ft/kinds/ftMario/ftmariospecialhi.h>
#include <melee/ft/kinds/ftMario/ftmariospeciallw.h>
#include <melee/ft/kinds/ftMario/ftmariospecialn.h>
#include <melee/ft/kinds/ftMario/ftmariospecials.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lbarchive.h>
#include <melee/pl/plstale.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/mtx.h>
#pragma warning(pop)

#include "match_xdk.h"

bool lb_8000B074(HSD_JObj* jobj);
void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);

extern void M360_AudioSfx(unsigned sfxId, unsigned volume, unsigned pan);

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

typedef struct M360MotionEntry {
    int msid;
    MotionState state;
} M360MotionEntry;

extern const M360MotionEntry M360_MotionTable[];
extern const unsigned M360_MotionTableCount;

u32 HSD_GObj_80390EB8(s32 i);
void HSD_GObjProc_8038FED4(HSD_GObjProc* proc);

enum {
    kMaxJoints = 128,
    kMaxDObjs = 256,
    kMaxAnims = 512,
    kMaxFighters = 2,
    kMaxHitboxes = 4,
    kLinkFighter = 2,
    kFighterStates = 0x80
};

typedef struct M360Hitbox {
    int active;
    int bone;
    float damage;
    float size;
    float angle;
    float kbGrowth;
    float kbBase;
    Vec3 offset;
    Vec3 pos;
} M360Hitbox;

typedef struct M360Fighter {
    Fighter fighter;
    HSD_GObj* gobj;
    HSD_JObj* joints[kMaxJoints];
    HSD_Joint* jointDescs[kMaxJoints];
    unsigned jointCount;
    HSD_DObj* dobjs[kMaxDObjs];
    unsigned dobjCount;
    FtPartsVisLookup* vis[5];
    FighterBone parts[kMaxJoints];
    int port;
    int modelIdx;
    M360Hitbox hitboxes[kMaxHitboxes];
    int hitTarget;
    float hitlag;
    unsigned flinchFrames;
    int traceMotion;
    int dead;
    u32 datAttrs[0x140]; /* fighter_dat_attrs_alloc_data storage */
    CmSubject cameraSubject;
} M360Fighter;


static ftData* s_ftData;
static HSD_Joint* s_modelJoint;
static HSD_MatAnimJoint* s_modelMatAnim;
static unsigned char* s_animImage;
static unsigned s_animImageSize;
static FigaTree* s_trees[kMaxAnims];
static M360Fighter s_fighters[kMaxFighters];
static StaleMoveTable s_staleTables[6];
static unsigned s_hitCount;
static unsigned s_cpuLevel = 1;
static MotionState s_commonStates[ftCo_MS_Count];
static MotionState s_fighterStates[0x80];
static Vec3 s_playerPos[6];
static float s_playerFacing[6];
static int s_playerCpu[6];
static unsigned s_unported[64];
static const char* s_unportedName[64];
static unsigned s_unportedCount;

static void BuildMotionTables(void);

static void Unported(const char* name)
{
    unsigned i;
    for (i = 0; i < s_unportedCount; ++i)
        if (s_unportedName[i] == name)
            return;
    if (s_unportedCount < 64) {
        s_unportedName[s_unportedCount] = name;
        s_unported[s_unportedCount++] = 1;
        M360_MatchTrace(name, 0);
    }
}

static M360Fighter* Owner(HSD_GObj* gobj)
{
    return (M360Fighter*) gobj->user_data;
}

static void* LoadArchiveSymbol(const char* file, const char* symbol)
{
    unsigned size = 0;
    unsigned char* image = M360_ReadDiscFile(file, &size);
    void* archive = image ? M360_ArchiveOpen(image, size) : NULL;
    M360_MatchTrace(file, size);
    return archive ? M360_ArchiveFind(archive, symbol) : NULL;
}

HSD_Archive* M360_ArchiveLoadSymbolsV(const char* filename, void* symbols, va_list args)
{
    unsigned size = 0;
    unsigned char* image = M360_ReadDiscFile(filename, &size);
    void* archive = image ? M360_ArchiveOpen(image, size) : NULL;
    void** out = symbols;
    M360_MatchTrace(filename, size);
    while (out) {
        const char* name = va_arg(args, const char*);
        *out = archive ? M360_ArchiveFind(archive, name) : NULL;
        out = va_arg(args, void**);
    }
    return archive;
}

StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 slot)
{
    return &s_staleTables[slot < 6 && slot >= 0 ? slot : 0];
}

int M360_FighterLoad(void)
{
    Fighter_LoadCommonData();
    s_ftData = LoadArchiveSymbol("PlMr.dat", "ftDataMario");
    s_modelJoint = LoadArchiveSymbol("PlMrNr.dat", "PlyMario5K_Share_joint");
    if (!p_ftCommonData || !s_ftData || !s_modelJoint)
        return 0;
    s_modelMatAnim = LoadArchiveSymbol("PlMrNr.dat", "PlyMario5K_Share_matanim_joint");
    s_animImage = M360_ReadDiscFile("PlMrAJ.dat", &s_animImageSize);
    M360_MatchTrace("fighter.aj.bytes", s_animImageSize);
    M360_MatchTrace("fighter.attr.gravity_x1000",
               (unsigned) (s_ftData->x0->gravity * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.walk_max_x1000",
               (unsigned) (s_ftData->x0->walk_max_vel * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.max_jumps", (unsigned) s_ftData->x0->max_jumps);
    BuildMotionTables();
    M360_MatchTrace("fighter.kb.xF4_x1000", (unsigned) (p_ftCommonData->xF4 * 1000.0f));
    M360_MatchTrace("fighter.kb.xF8_x1000", (unsigned) (p_ftCommonData->xF8 * 1000.0f));
    M360_MatchTrace("fighter.kb.x110_x1000", (unsigned) (p_ftCommonData->x110 * 1000.0f));
    M360_MatchTrace("fighter.kb.x114_x1000", (unsigned) (p_ftCommonData->x114 * 1000.0f));
    M360_MatchTrace("fighter.kb.x118_x1000", (unsigned) (p_ftCommonData->x118 * 1000.0f));
    M360_MatchTrace("fighter.kb.x11C_x1000", (unsigned) (p_ftCommonData->x11C * 1000.0f));
    M360_MatchTrace("fighter.kb.x120_x1000", (unsigned) (p_ftCommonData->x120 * 1000.0f));
    M360_MatchTrace("fighter.kb.x154_x1000", (unsigned) (p_ftCommonData->x154 * 1000.0f));
    M360_MatchTrace("fighter.attr.weight_x1000", (unsigned) (s_ftData->x0->weight * 1000.0f));
    return s_animImage != NULL;
}

void M360_FighterResetMatch(void)
{
    s_hitCount = 0;
    memset(s_fighters, 0, sizeof(s_fighters));
}

static FigaTree* LoadTree(int anim)
{
    struct Fighter_WaitAnimData* entry;
    unsigned char* copy;
    void* archive;
    if (anim < 0 || anim >= kMaxAnims)
        return NULL;
    if (s_trees[anim])
        return s_trees[anim];
    entry = &s_ftData->xC[anim];
    if (!entry->x8 || (unsigned) entry->x4 + (unsigned) entry->x8 > s_animImageSize)
        return NULL;
    copy = HSD_MemAlloc(entry->x8);
    if (!copy)
        return NULL;
    memcpy(copy, s_animImage + entry->x4, entry->x8);
    archive = M360_ArchiveOpen(copy, entry->x8);
    if (!archive)
        return NULL;
    s_trees[anim] = M360_ArchiveFind(archive, M360_ArchiveFirstSymbol(archive));
    return s_trees[anim];
}

static void CollectJoints(M360Fighter* f, HSD_JObj* jobj, HSD_Joint* desc)
{
    while (jobj && desc && f->jointCount < kMaxJoints) {
        HSD_DObj* dobj;
        f->joints[f->jointCount] = jobj;
        f->jointDescs[f->jointCount] = desc;
        ++f->jointCount;
        if (union_type_dobj(jobj))
            for (dobj = jobj->u.dobj; dobj && f->dobjCount < kMaxDObjs; dobj = dobj->next)
                f->dobjs[f->dobjCount++] = dobj;
        if (!(jobj->flags & JOBJ_INSTANCE))
            CollectJoints(f, jobj->child, desc->child);
        jobj = jobj->next;
        desc = desc->next;
    }
}

static void SetVisGroup(M360Fighter* f, int idx, int showIndex)
{
    FtPartsVisLookup* lookup = f->vis[idx];
    int j, k;
    if (!lookup)
        return;
    for (j = 0; j < lookup[0].x0; ++j) {
        TempS* group = &lookup[0].x4[j];
        for (k = 0; k < group->x0; ++k) {
            const unsigned d = group->x4[k];
            if (d >= f->dobjCount)
                continue;
            if (j == showIndex)
                HSD_DObjClearFlags(f->dobjs[d], DOBJ_HIDDEN);
            else
                HSD_DObjSetFlags(f->dobjs[d], DOBJ_HIDDEN);
        }
    }
}

static void FighterRender(HSD_GObj* gobj, int pass)
{
    M360Fighter* f = Owner(gobj);
    if (f->dead)
        return;
    SetVisGroup(f, 1, -1);
    SetVisGroup(f, 4, -1);
    SetVisGroup(f, 0, f->modelIdx);
    HSD_JObjDispAll(gobj->hsd_obj, NULL, HSD_GObj_80390EB8(pass), 0);
}

static void ResetJoints(M360Fighter* f)
{
    unsigned i;
    const unsigned special = s_ftData->x8->x10;
    HSD_JObjRemoveAnimAllByFlags(f->joints[0], 1);
    for (i = 1; i < f->jointCount; ++i) {
        HSD_JObj* j = f->joints[i];
        HSD_Joint* d = f->jointDescs[i];
        j->rotate.x = d->rotation.x;
        j->rotate.y = d->rotation.y;
        j->rotate.z = d->rotation.z;
        if (i == special) {
            const float s = 1.0f / f->fighter.co_attrs.model_scaling;
            j->scale.x = j->scale.y = j->scale.z = s;
        } else {
            j->scale.x = d->scale.x;
            j->scale.y = d->scale.y;
            j->scale.z = d->scale.z;
        }
        j->translate.x = d->position.x;
        j->translate.y = d->position.y;
        j->translate.z = d->position.z;
        HSD_JObjClearFlags(j, JOBJ_USE_QUATERNION);
        if (!(j->flags & JOBJ_MTX_INDEP_SRT))
            HSD_JObjSetMtxDirty(j);
    }
}

static void ApplyTree(M360Fighter* f, FigaTree* tree)
{
    s8* nodes = tree->nodes;
    FigaTrack* tracks = tree->tracks;
    unsigned i;
    for (i = 0; i < f->jointCount && *nodes != -1; ++i) {
        lbAnim_8001E6D8(f->joints[i], tree, tracks, *nodes);
        tracks += *nodes;
        ++nodes;
    }
}

static float ModelScale(Fighter* fp)
{
    return fp->x34_scale.y * fp->co_attrs.model_scaling;
}

static void AnimAdvance(M360Fighter* f)
{
    Fighter* fp = &f->fighter;
    HSD_JObj* transN = f->jointCount > 1 ? f->joints[1] : NULL;
    unsigned i;
    if (fp->anim_id == -1)
        return;
    HSD_AObjInitEndCallBack();
    for (i = 0; i < f->jointCount; ++i) {
        HSD_JObj* j = f->joints[i];
        if (fp->x594_b0 && j == transN) {
            Vec3 zero = { 0.0f, 0.0f, 0.0f };
            const float scale = ModelScale(fp);
            HSD_JObjAnim(j);
            fp->x698 = fp->x68C_transNPos;
            HSD_JObjGetTranslation(j, &fp->x68C_transNPos);
            fp->x68C_transNPos.x *= scale;
            fp->x68C_transNPos.y *= scale;
            fp->x68C_transNPos.z *= scale;
            fp->x6B0 = fp->x6A4_transNOffset;
            fp->x6A4_transNOffset.x = fp->x68C_transNPos.x - fp->x698.x;
            fp->x6A4_transNOffset.y = fp->x68C_transNPos.y - fp->x698.y;
            fp->x6A4_transNOffset.z = fp->x68C_transNPos.z - fp->x698.z;
            HSD_JObjSetTranslate(j, &zero);
        } else {
            HSD_JObjAnim(j);
        }
    }
    HSD_AObjInvokeCallBacks();
    for (i = 0; i < f->jointCount; ++i)
        if (f->joints[i]->aobj) {
            fp->cur_anim_frame = f->joints[i]->aobj->curr_frame;
            break;
        }
}

void ftAnim_8006E9B4(Fighter_GObj* gobj)
{
    AnimAdvance(Owner(gobj));
}

void ftAnim_8006EBA4(Fighter_GObj* gobj)
{
    AnimAdvance(Owner(gobj));
    ftAction_80073240(gobj);
}

void ftAnim_80070758(HSD_JObj* jobj)
{
    if (jobj)
        HSD_JObjRemoveAnimAllByFlags(jobj, 1);
}

bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    unsigned i;
    for (i = 0; i < f->jointCount; ++i) {
        HSD_AObj* aobj = f->joints[i]->aobj;
        if (aobj && !(aobj->flags & AOBJ_NO_ANIM))
            return true;
    }
    return false;
}

void ftAnim_8006F0FC(Fighter_GObj* gobj, float anim_rate)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_ForeachAnim(GET_JOBJ(gobj), JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetRate), AOBJ_ARG_AF, anim_rate);
    fp->frame_speed_mul = anim_rate;
}

void ftAnim_SetAnimRate(Fighter_GObj* gobj, float anim_rate)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->x2223_b0) {
        fp->x8A0_unk = anim_rate;
        return;
    }
    ftAnim_8006F0FC(gobj, anim_rate);
}

float ftAnim_8006F484(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    unsigned i;
    for (i = 0; i < f->jointCount; ++i)
        if (f->joints[i]->aobj)
            return f->joints[i]->aobj->end_frame;
    return 0.0f;
}

void ftAnim_8006EDD0(Fighter* fp, int arg1, float arg8, float arg9)
{
    (void) fp; (void) arg1; (void) arg8; (void) arg9;
    Unported("fighter.unported.anim_blend");
}

void ftAnim_8006FE9C(Fighter* fp, Fighter_Part start, float a, float b)
{
    (void) fp; (void) start; (void) a; (void) b;
}

void ftAnim_8006FF74(Fighter* fp, Fighter_Part start)
{
    (void) fp; (void) start;
}

bool ftAnim_8006F368(Fighter* fp, Fighter_Part part)
{
    return fp->parts[part].joint && lb_8000B074(fp->parts[part].joint);
}

/* ftAnim_8006DF0C: keep a grabbed fighter's special joint on its hip. */
void ftAnim_8006DF0C(Fighter* fp)
{
    if (fp->x2221_b2) {
        HSD_JObj* root = fp->parts[0].joint;
        HSD_JObj* special = fp->parts[DP(struct ftData_x8, fp->ft_data->x8)->x10].joint;
        Mtx mtx;
        Vec3 pos0, vec;
        DISC_VEC3_GET(pos0, p_ftCommonData->x808);
        lb_8000B1CC(fp->parts[ftParts_GetBoneIndex(fp, FtPart_HipN)].joint, &pos0, &vec);
        HSD_MtxInverse(HSD_JObjGetMtxPtr(root), mtx);
        PSMTXMultVec(mtx, &vec, &vec);
        HSD_JObjSetTranslate(special, &vec);
    }
}

/* Costume/part animation hooks from the Mario motion table (metal and
 * vitamin swaps); the native part-visibility path does not model them. */
void ftAnim_80070C48(Fighter_GObj* gobj, s32 arg)
{
    (void) gobj; (void) arg;
}

void ftAnim_80070CC4(Fighter_GObj* gobj, int arg)
{
    (void) gobj; (void) arg;
}

void ftAnim_80070FB4(Fighter_GObj* gobj, s32 a, s32 b)
{
    (void) gobj; (void) a; (void) b;
}

/* The shield-tilt pose blends a second skeleton (x8AC_animSkeleton) that the
 * native animation path does not build yet; the Guard animation itself still
 * plays through the normal motion tree. */
void ftAnim_8006F4C8(Fighter* fp, bool do_blending, FigaTree* tree)
{
    (void) fp; (void) do_blending; (void) tree;
}

void ftAnim_8006FA58(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    (void) fp; (void) part; (void) joint;
}

void ftAnim_8006FB88(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    (void) fp; (void) part; (void) joint;
}

void ftAnim_80070010(Fighter* fp, Fighter_Part start, float t, float t_inv, HSD_Joint* joint)
{
    (void) fp; (void) start; (void) t; (void) t_inv; (void) joint;
}

void ftAnim_80070108(Fighter* fp, Fighter_Part start, float t, float t_inv, HSD_Joint* joint)
{
    (void) fp; (void) start; (void) t; (void) t_inv; (void) joint;
}

void ftAnim_80070710(HSD_JObj* jobj, float frame)
{
    (void) jobj; (void) frame;
}

FigaTree* ftData_80085E50(Fighter* fp, enum_t msid)
{
    (void) fp; (void) msid;
    return NULL;
}

static HSD_JObj* FindAnimatedJObj(HSD_JObj* jobj)
{
    for (; jobj; jobj = jobj->next) {
        HSD_JObj* child;
        if (jobj->aobj)
            return jobj;
        if (!(jobj->flags & JOBJ_INSTANCE) && (child = FindAnimatedJObj(jobj->child)) != NULL)
            return child;
    }
    return NULL;
}

float lbGetJObjEndFrame(HSD_JObj* jobj)
{
    jobj = FindAnimatedJObj(jobj);
    return jobj ? jobj->aobj->end_frame : 0.0f;
}

void ftData_80085CD8(Fighter* fp, Fighter* src, enum_t msid)
{
    (void) src;
    fp->x590 = LoadTree(msid);
    fp->x597_bits = fp->kind;
}

void ftAnim_8006EBE8(HSD_GObj* gobj, float anim_start, float anim_rate,
                     float anim_blend_frames)
{
    M360Fighter* f = Owner(gobj);
    Fighter* fp = &f->fighter;
    HSD_JObj* root = f->joints[0];
    (void) anim_blend_frames;
    ResetJoints(f);
    if (fp->x590)
        ApplyTree(f, fp->x590);
    HSD_JObjReqAnimAllByFlags(root, 1, anim_start);
    if (fp->x594_b1_loop)
        HSD_ForeachAnim(root, JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetFlags), AOBJ_ARG_AU, AOBJ_LOOP);
    HSD_ForeachAnim(root, JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetRate), AOBJ_ARG_AF, anim_rate);
    fp->x8A4_animBlendFrames = 0.0f;
    fp->x8A8_anim_frame = 0.0f;
}

static void UnknownMotion_Anim(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    M360_MatchTrace("fighter.unported.motion", (unsigned) fp->motion_id);
    ftCommon_8007D92C(gobj);
}

static void BuildMotionTables(void)
{
    MotionState unknown;
    unsigned i;
    memset(&unknown, 0, sizeof(unknown));
    unknown.anim_id = -1;
    unknown.anim_cb = UnknownMotion_Anim;
    for (i = 0; i < ftCo_MS_Count; ++i)
        s_commonStates[i] = unknown;
    for (i = 0; i < kFighterStates; ++i)
        s_fighterStates[i] = unknown;
    for (i = 0; i < M360_MotionTableCount; ++i)
        s_commonStates[M360_MotionTable[i].msid] = M360_MotionTable[i].state;
    for (i = 0; i < ftMr_MS_SelfCount && i < kFighterStates; ++i)
        s_fighterStates[i] = ftMr_Init_MotionStateTable[i];
    ftData_SpecialN[Ft_Kind_Mario] = ftMr_SpecialN_Enter;
    ftData_SpecialS[Ft_Kind_Mario] = ftMr_SpecialS_Enter;
    ftData_SpecialHi[Ft_Kind_Mario] = ftMr_SpecialHi_Enter;
    ftData_SpecialLw[Ft_Kind_Mario] = ftMr_SpecialLw_Enter;
    ftData_SpecialAirN[Ft_Kind_Mario] = ftMr_SpecialAirN_Enter;
    ftData_SpecialAirS[Ft_Kind_Mario] = ftMr_SpecialAirS_Enter;
    ftData_SpecialAirHi[Ft_Kind_Mario] = ftMr_SpecialAirHi_Enter;
    ftData_SpecialAirLw[Ft_Kind_Mario] = ftMr_SpecialAirLw_Enter;
}

static int FloorAt(float x, float yTop, float yBottom, int allowPlatform, float* y,
                   int* line)
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned i;
    int best = -1;
    float bestY = -3.4e38f;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, ly;
        if (!(l->kind & M360_LINE_FLOOR))
            continue;
        if (!allowPlatform && (l->flags & M360_LINE_PLATFORM))
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (x < lo || x > hi || hi - lo < 1e-4f)
            continue;
        t = (x - l->x0) / (l->x1 - l->x0);
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (ly <= yTop && ly >= yBottom && ly > bestY) {
            bestY = ly;
            best = (int) i;
        }
    }
    if (best < 0)
        return 0;
    *y = bestY;
    *line = best;
    return 1;
}

int M360_MatchGroundBelow(float x, float y, float depth, float* groundY, unsigned* line)
{
    int l;
    if (!FloorAt(x, y, y - depth, 1, groundY, &l))
        return 0;
    *line = (unsigned) l;
    return 1;
}

static void SetFloor(Fighter* fp, int line)
{
    const M360StageLine* l = &M360_MatchStageData()->lines[line];
    float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
    const float len = sqrtf(nx * nx + ny * ny);
    if (len > 0.0f) {
        nx /= len;
        ny /= len;
    }
    fp->coll_data.floor.index = line;
    fp->coll_data.floor.flags = l->flags;
    fp->coll_data.floor.normal.x = nx;
    fp->coll_data.floor.normal.y = ny;
    fp->coll_data.floor.normal.z = 0.0f;
}

u32 mpLineGetFlags(int line_id)
{
    const M360MatchStage* st = M360_MatchStageData();
    if (line_id < 0 || (unsigned) line_id >= st->lineCount)
        return 0;
    return st->lines[line_id].flags;
}

bool mpColl_IsOnPlatform(CollData* coll)
{
    return (mpLineGetFlags(coll->floor.index) & LINE_FLAG_PLATFORM) != 0;
}

void mpUpdateFloorSkip(CollData* coll)
{
    coll->floor_skip = coll->floor.index;
}

void mpClearFloorSkip(CollData* coll)
{
    coll->floor_skip = -1;
}

static int GroundStep(HSD_GObj* gobj, int stopAtEdge)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const M360MatchStage* st = M360_MatchStageData();
    const float step = 4.0f;
    float y;
    int line;
    fp->coll_data.env_flags &= ~(Collide_LeftEdge | Collide_RightEdge);
    if (FloorAt(fp->cur_pos.x, fp->cur_pos.y + step, fp->cur_pos.y - step, 1, &y, &line)) {
        fp->cur_pos.y = y;
        SetFloor(fp, line);
        fp->coll_data.env_flags |= Collide_FloorHug;
        return 1;
    }
    if (stopAtEdge && fp->coll_data.floor.index >= 0 &&
        (unsigned) fp->coll_data.floor.index < st->lineCount) {
        const M360StageLine* l = &st->lines[fp->coll_data.floor.index];
        const float lo = l->x0 < l->x1 ? l->x0 : l->x1;
        const float hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (fp->cur_pos.x < lo) {
            fp->cur_pos.x = lo;
            fp->coll_data.env_flags |= Collide_LeftEdge;
        } else {
            fp->cur_pos.x = hi;
            fp->coll_data.env_flags |= Collide_RightEdge;
        }
        fp->cur_pos.y = l->y0 + (l->y1 - l->y0) * ((fp->cur_pos.x - l->x0) / (l->x1 - l->x0));
        fp->gr_vel = 0.0f;
        fp->self_vel.x = 0.0f;
        return 1;
    }
    return 0;
}

GroundOrAir ft_80082708(Fighter_GObj* gobj)
{
    return (GroundOrAir) GroundStep(gobj, 0);
}

/* mpColl_8004A678_Floor: a fighter drifting off a floor end without a hard
 * tilt (|stick x| < 0.75) toward it stops on the edge and flags Collide_Edge,
 * which the callers turn into Ottotto (teeter). */
static int GroundStepEdge(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const int line = fp->coll_data.floor.index;
    const float stick = fp->input.lstick[0].x;
    Vec3 left, right;
    fp->coll_data.env_flags &= ~Collide_Edge;
    if (GroundStep(gobj, 0))
        return 1;
    if (!mpLib_80054ED8(line))
        return 0;
    mpFloorGetLeft(line, &left);
    mpFloorGetRight(line, &right);
    if (fp->cur_pos.x <= left.x && fp->facing_dir < 0.0f && stick > -0.75f) {
        fp->cur_pos.x = left.x;
        fp->cur_pos.y = left.y;
    } else if (fp->cur_pos.x >= right.x && fp->facing_dir > 0.0f && stick < 0.75f) {
        fp->cur_pos.x = right.x;
        fp->cur_pos.y = right.y;
    } else {
        return 0;
    }
    fp->coll_data.env_flags |= Collide_Edge;
    fp->gr_vel = 0.0f;
    fp->self_vel.x = 0.0f;
    return 0;
}

void ft_800843FC(Fighter_GObj* gobj)
{
    if (GroundStepEdge(gobj) || ftCo_8009A3C8(gobj))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_800848DC(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 0))
        cb(gobj);
}

bool ft_80084C74(Fighter_GObj* gobj, int* arg1, int* arg2, int* arg3)
{
    (void) gobj; (void) arg1; (void) arg2; (void) arg3;
    return false;
}

bool ft_80084CE4(Fighter* attacker, Fighter* victim)
{
    (void) attacker; (void) victim;
    return false;
}

bool ft_800827A0(Fighter_GObj* gobj)
{
    return GroundStep(gobj, 1);
}

void ft_80084280(Fighter_GObj* gobj)
{
    if (GroundStepEdge(gobj) || ftCo_8009A3C8(gobj))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_800844EC(Fighter_GObj* gobj)
{
    if (GroundStep(gobj, 0))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_80083F88(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

void ft_80084104(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 1))
        ftCo_Fall_Enter(gobj);
}

/* Finds the highest floor line crossed while moving from `from` to `to`,
 * honoring the platform-drop skip line and an optional per-line filter. */
static int LandBetween(Fighter* fp, const Vec3* from, Vec3* to,
                       bool (*land)(Fighter_GObj*, int))
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned i;
    int best = -1;
    float bestY = -3.4e38f;
    if (fp->coll_data.floor_skip >= 0 && (unsigned) fp->coll_data.floor_skip < st->lineCount) {
        const M360StageLine* l = &st->lines[fp->coll_data.floor_skip];
        const float lo = l->x0 < l->x1 ? l->x0 : l->x1;
        const float hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (to->x < lo || to->x > hi || to->y < (l->y0 < l->y1 ? l->y0 : l->y1) - 12.0f)
            fp->coll_data.floor_skip = -1;
    }
    if (to->y > from->y)
        return 0;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, ly;
        if (!(l->kind & M360_LINE_FLOOR))
            continue;
        if ((int) i == fp->coll_data.floor_skip)
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (to->x < lo || to->x > hi || hi - lo < 1e-4f)
            continue;
        t = (to->x - l->x0) / (l->x1 - l->x0);
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (from->y >= ly - 0.01f && to->y <= ly && ly > bestY) {
            if (land && !land(fp->gobj, (int) i))
                continue;
            bestY = ly;
            best = (int) i;
        }
    }
    if (best < 0)
        return 0;
    to->y = bestY;
    SetFloor(fp, best);
    return 1;
}

static int AirStep(HSD_GObj* gobj, bool (*land)(Fighter_GObj*, int))
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;
    if (!LandBetween(fp, &fp->prev_pos, &fp->cur_pos, land))
        return 0;
    fp->coll_data.cur_pos = fp->cur_pos;
    return 1;
}

static void AirWalls(Fighter* fp)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float mid = fp->cur_pos.y + 8.0f;
    unsigned i;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, lx;
        if (!(l->kind & (M360_LINE_LEFT_WALL | M360_LINE_RIGHT_WALL)))
            continue;
        lo = l->y0 < l->y1 ? l->y0 : l->y1;
        hi = l->y0 < l->y1 ? l->y1 : l->y0;
        if (mid < lo || mid > hi || hi - lo < 1e-4f)
            continue;
        t = (mid - l->y0) / (l->y1 - l->y0);
        lx = l->x0 + (l->x1 - l->x0) * t;
        if ((l->kind & M360_LINE_LEFT_WALL) && fp->prev_pos.x <= lx && fp->cur_pos.x > lx) {
            fp->cur_pos.x = lx;
            if (fp->self_vel.x > 0.0f) fp->self_vel.x = 0.0f;
        } else if ((l->kind & M360_LINE_RIGHT_WALL) && fp->prev_pos.x >= lx && fp->cur_pos.x < lx) {
            fp->cur_pos.x = lx;
            if (fp->self_vel.x < 0.0f) fp->self_vel.x = 0.0f;
        }
    }
}

static void AirCeilings(Fighter* fp)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float height = fp->ft_data && fp->ft_data->x3C
        ? fp->ft_data->x3C->xC.x * fp->x34_scale.y : 12.0f;
    unsigned i;
    if (fp->cur_pos.y <= fp->prev_pos.y)
        return;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, lx, ly;
        if (!(l->kind & M360_LINE_CEILING))
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (hi - lo < 1e-4f)
            continue;
        t = (fp->cur_pos.x - l->x0) / (l->x1 - l->x0);
        if (t < 0.0f || t > 1.0f)
            continue;
        lx = l->x0 + (l->x1 - l->x0) * t;
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (fp->prev_pos.y + height <= ly && fp->cur_pos.y + height >= ly) {
            fp->cur_pos.y = ly - height;
            if (fp->self_vel.y > 0.0f) fp->self_vel.y = 0.0f;
            if (fp->x8c_kb_vel.y > 0.0f) fp->x8c_kb_vel.y = 0.0f;
            fp->coll_data.ceiling.index = (s16) i;
            (void) lx;
            return;
        }
    }
}

/* Native counterpart of mpColl_80044164/800443C4: looks for a ledge-flagged
 * floor end inside the fighter's ledge-snap box (ftData x44) while falling. */
static int FindLedge(Fighter* fp, int left, int* ledge)
{
    const M360MatchStage* st = M360_MatchStageData();
    const ftData_x44_t* snap = DP(ftData_x44_t, fp->ft_data->x44);
    const float scale = fp->x34_scale.y;
    const float snapX = snap->ledge_snap_x * scale;
    const float snapY = snap->ledge_snap_y * scale;
    const float half = 0.5f * snap->ledge_snap_height * scale;
    const Vec3* prev = &fp->prev_pos;
    const Vec3* cur = &fp->cur_pos;
    const float bottom = (cur->y < prev->y ? cur->y : prev->y) + snapY - half;
    const float top = (cur->y < prev->y ? prev->y : cur->y) + snapY + half;
    float lo, hi;
    unsigned i;
    if (left) {
        lo = prev->x < cur->x ? prev->x : cur->x;
        hi = snapX + (prev->x < cur->x ? cur->x : prev->x);
    } else {
        lo = (prev->x < cur->x ? prev->x : cur->x) - snapX;
        hi = prev->x < cur->x ? cur->x : prev->x;
    }
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float ex, ey;
        if (!(l->kind & M360_LINE_FLOOR) || !(l->flags & LINE_FLAG_LEDGE) || (int) i == fp->coll_data.floor_skip)
            continue;
        if (left == (l->x0 < l->x1)) {
            ex = l->x0; ey = l->y0;
        } else {
            ex = l->x1; ey = l->y1;
        }
        if (ex < lo || ex > hi || ey < bottom || ey > top)
            continue;
        if ((left && cur->x >= ex) || (!left && cur->x <= ex) || cur->y >= ey)
            continue;
        *ledge = (int) i;
        return 1;
    }
    return 0;
}

static int TryCliff(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int ledge;
    fp->coll_data.env_flags &= ~Collide_LedgeGrabMask;
    if (fp->x2064_ledgeCooldown || fp->x2224_b2 || fp->cur_pos.y >= fp->prev_pos.y ||
        !fp->ft_data || !fp->ft_data->x44)
        return 0;
    if (fp->facing_dir >= 0.0f && FindLedge(fp, 1, &ledge)) {
        fp->coll_data.env_flags |= Collide_LeftLedgeGrab;
        fp->coll_data.ledge_id_left = ledge;
    } else if (fp->facing_dir <= 0.0f && FindLedge(fp, 0, &ledge)) {
        fp->coll_data.env_flags |= Collide_RightLedgeGrab;
        fp->coll_data.ledge_id_right = ledge;
    } else {
        return 0;
    }
    if (ftCliffCommon_80081298(gobj)) {
        M360_MatchTrace("fighter.cliff.catch", (unsigned) ledge);
        return 1;
    }
    return 0;
}

void ft_800831CC(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_800835B0(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
    else
        TryCliff(gobj);
}

bool ft_80081DD4(Fighter_GObj* gobj)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    return AirStep(gobj, NULL) != 0;
}

bool ft_80081F2C(Fighter_GObj* gobj)
{
    return ft_80081DD4(gobj);
}

bool ft_80082084(Fighter_GObj* gobj)
{
    return ft_80081DD4(gobj);
}

void ft_8008370C(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (ft_80081DD4(gobj))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_80083318(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_80083464(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_80082D40(Fighter_GObj* gobj, f32 arg1)
{
    (void) arg1;
    if (ft_80081DD4(gobj)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp))
            ft_8008A2BC(gobj);
        else
            ftCo_Landing_Enter_Basic(gobj);
    }
}

void ft_80082C74(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, NULL))
        cb(gobj);
}

void ft_80082F28(Fighter_GObj* gobj)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, NULL)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp)) {
            ft_8008A2BC(gobj);
            return;
        }
        ftCo_Landing_Enter_Basic(gobj);
    } else {
        TryCliff(gobj);
    }
}

void ft_80083090(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_800845B4(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

void ft_800847D0(Fighter_GObj* gobj, ftCollisionBox* box)
{
    (void) box;
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

/* Air collision used by grab/throw states (mpColl_800477E0 in ft_80082578):
 * reports floor contact through env_flags like the original. */
static int AirCollide(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->coll_data.env_flags &= ~Collide_FloorMask;
    AirWalls(fp);
    AirCeilings(fp);
    if (!AirStep(gobj, NULL))
        return 0;
    fp->coll_data.env_flags |= Collide_FloorHug;
    return 1;
}

GroundOrAir ft_80081D0C(Fighter_GObj* gobj)
{
    return AirCollide(gobj) ? GA_Air : GA_Ground;
}

bool ft_800824A0(Fighter_GObj* gobj, ftCollisionBox* ecb)
{
    (void) ecb;
    return AirCollide(gobj) != 0;
}

bool ft_80082888(Fighter_GObj* gobj, ftCollisionBox* ecb)
{
    (void) ecb;
    return GroundStep(gobj, 0) != 0;
}

void ft_80083910(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (AirCollide(gobj))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_80083B68(Fighter_GObj* gobj)
{
    AirCollide(gobj);
}

bool ft_800821DC(Fighter_GObj* gobj)
{
    return AirCollide(gobj) != 0;
}

void mpLib_80053ECC_Floor(int line_id, Vec* vec)
{
    const M360StageLine* l;
    if (!mpLib_80054ED8(line_id))
        return;
    l = &M360_MatchStageData()->lines[line_id];
    vec->x = l->x0 < l->x1 ? l->x0 : l->x1;
    vec->y = l->x0 < l->x1 ? l->y0 : l->y1;
    vec->z = 0.0f;
}

void mpFloorGetLeft(int line_id, Vec3* vec)
{
    mpLib_80053ECC_Floor(line_id, vec);
}

void mpFloorGetRight(int line_id, Vec3* vec)
{
    mpLib_80053DA4_Floor(line_id, vec);
}

void mpLib_80053DA4_Floor(int line_id, Vec3* vec)
{
    const M360StageLine* l;
    if (!mpLib_80054ED8(line_id))
        return;
    l = &M360_MatchStageData()->lines[line_id];
    vec->x = l->x0 < l->x1 ? l->x1 : l->x0;
    vec->y = l->x0 < l->x1 ? l->y1 : l->y0;
    vec->z = 0.0f;
}

void ft_80083C00(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (AirCollide(gobj))
        cb(gobj);
}

void ft_80083CE4(Fighter_GObj* gobj, bool (*cb1)(Fighter_GObj*, int), HSD_GObjEvent cb2)
{
    Fighter* fp = GET_FIGHTER(gobj);
    AirWalls(fp);
    AirCeilings(fp);
    if (AirStep(gobj, cb1))
        cb2(gobj);
}

void ft_8008403C(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 0))
        cb(gobj);
}

void ft_800841B8(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 1))
        cb(gobj);
}

void ftCo_AirCatchHit_Coll(Fighter_GObj* gobj)
{
    if (AirCollide(gobj)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp))
            ft_8008A2BC(gobj);
        else
            ftCo_Landing_Enter_Basic(gobj);
    }
}

/* CollData-only entry points used when a grab releases or throws its victim.
 * The native stage collision works on the owning fighter, which embeds the
 * CollData. */
static Fighter* CollOwner(CollData* coll)
{
    return (Fighter*) ((char*) coll - offsetof(Fighter, coll_data));
}

void mpColl_80043670(CollData* coll)
{
    coll->x130_flags |= CollData_X130_Clear;
}

void mpColl_80043680(CollData* coll, Vec3* pos)
{
    coll->cur_pos = *pos;
    coll->prev_pos = coll->cur_pos;
    coll->last_pos = coll->prev_pos;
    coll->x130_flags |= CollData_X130_Clear;
}

static bool CollAir(CollData* coll)
{
    Fighter* fp = CollOwner(coll);
    coll->env_flags &= ~Collide_FloorMask;
    if (!LandBetween(fp, &coll->last_pos, &coll->cur_pos, NULL))
        return false;
    coll->env_flags |= Collide_FloorHug;
    return true;
}

bool mpColl_800471F8(CollData* coll)
{
    return CollAir(coll);
}

bool mpColl_800477E0(CollData* coll)
{
    return CollAir(coll);
}

bool mpColl_80048654(CollData* coll)
{
    Fighter* fp = CollOwner(coll);
    float y;
    int line;
    coll->env_flags &= ~Collide_FloorMask;
    if (!FloorAt(coll->cur_pos.x, coll->cur_pos.y + 4.0f, coll->cur_pos.y - 4.0f, 1, &y, &line))
        return CollAir(coll);
    coll->cur_pos.y = y;
    SetFloor(fp, line);
    coll->env_flags |= Collide_FloorHug;
    return true;
}

bool mpLib_80054ED8(int line_id)
{
    return line_id >= 0 && (unsigned) line_id < M360_MatchStageData()->lineCount;
}

int mpLib_8005199C_Floor(Vec3* vec, int joint_id_skip, int joint_id_only)
{
    float y;
    int line;
    (void) joint_id_skip; (void) joint_id_only;
    return FloorAt(vec->x, vec->y, -30000.0f, 1, &y, &line) ? line : -1;
}

static int LineEndsMeet(const M360StageLine* a, const M360StageLine* b)
{
    const float d0x = a->x1 - b->x0, d0y = a->y1 - b->y0;
    const float d1x = a->x0 - b->x1, d1y = a->y0 - b->y1;
    return d0x * d0x + d0y * d0y < 4.0f || d1x * d1x + d1y * d1y < 4.0f;
}

bool mpLinesConnected(int start_id, int target_id)
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned char seen[128];
    int queue[128];
    int head = 0, tail = 0;
    unsigned i;
    if (!mpLib_80054ED8(start_id) || !mpLib_80054ED8(target_id))
        return false;
    if (start_id == target_id)
        return true;
    memset(seen, 0, sizeof(seen));
    seen[start_id] = 1;
    queue[tail++] = start_id;
    while (head < tail) {
        const M360StageLine* cur = &st->lines[queue[head++]];
        for (i = 0; i < st->lineCount; ++i) {
            if (seen[i] || st->lines[i].kind != cur->kind || !LineEndsMeet(cur, &st->lines[i]))
                continue;
            if ((int) i == target_id)
                return true;
            seen[i] = 1;
            queue[tail++] = (int) i;
        }
    }
    return false;
}

/* mpCheckAllRemap: nearest stage line crossed by the segment (x0,y0)-(x1,y1). */
bool mpCheckAllRemap(Vec3* pos_out, int* line_id_out, u32* flags_out, Vec3* normal_out,
                     int joint_id_skip, int joint_id_only, float x0, float y0, float x1, float y1)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float dx = x1 - x0, dy = y1 - y0;
    float bestT = 2.0f;
    int best = -1;
    unsigned i;
    (void) joint_id_skip; (void) joint_id_only;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        const float ex = l->x1 - l->x0, ey = l->y1 - l->y0;
        const float den = dx * ey - dy * ex;
        float t, u;
        if (den > -1e-6f && den < 1e-6f)
            continue;
        t = ((l->x0 - x0) * ey - (l->y0 - y0) * ex) / den;
        u = ((l->x0 - x0) * dy - (l->y0 - y0) * dx) / den;
        if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f || t >= bestT)
            continue;
        bestT = t;
        best = (int) i;
    }
    if (best < 0)
        return false;
    if (pos_out) {
        pos_out->x = x0 + dx * bestT;
        pos_out->y = y0 + dy * bestT;
        pos_out->z = 0.0f;
    }
    if (line_id_out)
        *line_id_out = best;
    if (flags_out)
        *flags_out = st->lines[best].flags;
    if (normal_out) {
        const M360StageLine* l = &st->lines[best];
        float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
        const float len = sqrtf(nx * nx + ny * ny);
        normal_out->x = len > 0.0f ? nx / len : 0.0f;
        normal_out->y = len > 0.0f ? ny / len : 1.0f;
        normal_out->z = 0.0f;
    }
    return true;
}

int mpLib_8004DD90_Floor(int line_id, Vec3* vec, float* y_out, u32* flags_out, Vec3* normal_out)
{
    const M360StageLine* l;
    float x, t;
    if (!mpLib_80054ED8(line_id))
        return -1;
    l = &M360_MatchStageData()->lines[line_id];
    x = vec->x;
    if (x < (l->x0 < l->x1 ? l->x0 : l->x1))
        x = l->x0 < l->x1 ? l->x0 : l->x1;
    if (x > (l->x0 < l->x1 ? l->x1 : l->x0))
        x = l->x0 < l->x1 ? l->x1 : l->x0;
    t = l->x1 != l->x0 ? (x - l->x0) / (l->x1 - l->x0) : 0.0f;
    if (y_out)
        *y_out = l->y0 + (l->y1 - l->y0) * t - vec->y;
    if (flags_out)
        *flags_out = l->flags;
    if (normal_out) {
        float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
        const float len = sqrtf(nx * nx + ny * ny);
        normal_out->x = len > 0.0f ? nx / len : 0.0f;
        normal_out->y = len > 0.0f ? ny / len : 1.0f;
        normal_out->z = 0.0f;
    }
    return line_id;
}

bool ft_80084A18(Fighter_GObj* gobj)
{
    const int line = GET_FIGHTER(gobj)->coll_data.floor.index;
    return line >= 0 && (unsigned) line < M360_MatchStageData()->lineCount;
}

float ft_GetGroundFrictionMultiplier(Fighter* fp)
{
    (void) fp;
    return 1.0f;
}

void ftCamera_UpdateCameraBox(HSD_GObj* gobj)
{
    (void) gobj;
}

void ftParts_80074A4C(Fighter_GObj* gobj, int model_idx, int val)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x5F4_arr[model_idx].prev = (s8) val;
    fp->x221D_b2 = true;
}

void ftParts_80074A8C(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    f->modelIdx = f->fighter.x5F4_arr[0].prev;
    f->fighter.x5F4_arr[0].idx = f->fighter.x5F4_arr[0].prev;
    f->fighter.x221D_b2 = false;
}

void ftParts_80074ACC(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    f->fighter.x5F4_arr[0].idx = -1;
    f->modelIdx = -1;
    f->fighter.x221D_b2 = false;
}

void ftParts_80074B0C(Fighter_GObj* gobj, int model_idx, int val)
{
    M360Fighter* f = Owner(gobj);
    if (model_idx == 0) {
        f->modelIdx = val;
        f->fighter.x5F4_arr[0].idx = (s8) val;
    }
}

Fighter_Part ftParts_GetBoneIndex(Fighter* fp, Fighter_Part part)
{
    FighterPartsTable* table = (FighterPartsTable*) (uintptr_t) ftPartsTable[fp->kind].v;
    return (Fighter_Part) (s8) table->part_to_joint[part];
}

void ft_PlaySFX(Fighter* fp, enum_t sfx_id, u8 sfx_vol, u8 sfx_pan)
{
    (void) fp;
    M360_AudioSfx((unsigned) sfx_id, sfx_vol, sfx_pan);
}

static void ProcHit(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const float kb = fp->dmg.kb_applied;
    const float damage = fp->dmg.x1838_percentTemp;
    Fighter_ProcessHit_8006D1EC(gobj);
    if (kb) {
        ++s_hitCount;
        M360_MatchTrace("fighter.hit.player", fp->player_id);
        M360_MatchTrace("fighter.hit.damage", (unsigned) damage);
        M360_MatchTrace("fighter.hit.knockback_x100", (unsigned) (kb * 100.0f));
        M360_MatchTrace("fighter.hit.reaction_motion", (unsigned) fp->motion_id);
        M360_MatchTrace("fighter.hit.hitlag_frames", (unsigned) fp->dmg.x195c_hitlag_frames);
        M360_MatchTrace("fighter.hitstun.frames", (unsigned) fp->mv.co.damage.x0);
        M360_MatchTrace("fighter.hit.target_percent", (unsigned) fp->dmg.x1830_percent);
        M360_MatchTrace("fighter.hit.kb_vel_x100_x", (unsigned) (int) (fp->x8c_kb_vel.x * 100.0f));
        M360_MatchTrace("fighter.hit.kb_vel_x100_y", (unsigned) (int) (fp->x8c_kb_vel.y * 100.0f));
    }
}

void ftCo_800B3900(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter* opponent = NULL;
    unsigned i;
    fp->cpu.lstick.x = fp->cpu.lstick.y = 0;
    fp->cpu.cstick.x = fp->cpu.cstick.y = 0;
    fp->cpu.buttons = 0;
    ++fp->cpu.x7C;
    for (i = 0; i < kMaxFighters; ++i)
        if (s_fighters[i].gobj && &s_fighters[i].fighter != fp)
            opponent = &s_fighters[i].fighter;
    if (!opponent || s_cpuLevel == 0)
        return;
    {
        const float dx = opponent->cur_pos.x - fp->cur_pos.x;
        const float dy = opponent->cur_pos.y - fp->cur_pos.y;
        const unsigned t = fp->cpu.x7C;
        const float edge = 68.0f;
        if (fp->motion_id >= ftCo_MS_DamageHi1 && fp->motion_id <= ftCo_MS_DamageFlyRoll)
            return;
        if (fp->ground_or_air == GA_Air && fabsf(fp->cur_pos.x) > edge - 8.0f) {
            fp->cpu.lstick.x = fp->cur_pos.x > 0.0f ? -127 : 127;
            if (fp->self_vel.y < 0.0f && fp->x1968_jumpsUsed < fp->co_attrs.max_jumps && (t & 7) == 0)
                fp->cpu.buttons |= HSD_PAD_X;
            return;
        }
        if (fabsf(dx) > 12.0f || (fabsf(dy) > 20.0f && fabsf(dx) > 4.0f))
            fp->cpu.lstick.x = dx > 0.0f ? 90 : -90;
        if (dy < -20.0f && fp->ground_or_air == GA_Ground &&
            mpColl_IsOnPlatform(&fp->coll_data) && (t & 31) < 3)
            fp->cpu.lstick.y = -127;
        if (dy > 20.0f && fabsf(dx) < 40.0f && (t & 15) < 3 &&
            (fp->ground_or_air == GA_Ground || fp->self_vel.y < 0.0f))
            fp->cpu.buttons |= HSD_PAD_X;
        if (fabsf(dx) < 16.0f && fabsf(dy) < 20.0f && (t % 45u) < 2)
            fp->cpu.buttons |= HSD_PAD_A;
    }
}

static void ProcFinish(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    M360Fighter* f = Owner(gobj);
    if (f->port == 0 && f->traceMotion != (int) fp->motion_id) {
        f->traceMotion = (int) fp->motion_id;
        M360_MatchTrace("fighter.p1.motion", (unsigned) fp->motion_id);
    }
    HSD_JObjSetRotationY(gobj->hsd_obj, (float) M_PI_2 * fp->facing_dir);
    HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
}

static void RemoveUserData(void* data)
{
    (void) data;
}

void* M360_FighterSpawn(int slot, float x, float y, float facing, int port)
{
    M360Fighter* f;
    Fighter* fp;
    HSD_GObj* gobj;
    HSD_JObj* root;
    Vec3 scale;
    int i;
    if (!s_ftData || slot < 0 || slot >= kMaxFighters)
        return NULL;
    f = &s_fighters[slot];
    memset(f, 0, sizeof(*f));
    fp = &f->fighter;
    gobj = GObj_Create(HSD_GOBJ_CLASS_FIGHTER, 8, 0);
    root = HSD_JObjLoadJoint(s_modelJoint);
    HSD_JObjAddAnimAll(root, NULL, s_modelMatAnim, NULL);
    HSD_JObjReqAnimAll(root, 0.0f);
    HSD_JObjAnimAll(root);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, root);
    GObj_InitUserData(gobj, 4, RemoveUserData, f);
    GObj_SetupGXLink(gobj, FighterRender, kLinkFighter, 0);
    f->gobj = gobj;
    f->port = port;
    CollectJoints(f, root, s_modelJoint);
    for (i = 0; i < (int) f->jointCount; ++i) {
        f->parts[i].joint = f->joints[i];
        f->parts[i].x4_jobj2 = f->joints[i];
        f->parts[i].flags_b1 = true;
    }
    fp->parts = f->parts;
    for (i = 0; i < 4; ++i) {
        DiscU32(*table)[4] = (DiscU32(*)[4]) s_ftData->x8->x0.vis_table;
        f->vis[i] = (FtPartsVisLookup*) (uintptr_t) table[0][i].v;
    }
    SetVisGroup(f, 0, -1);
    SetVisGroup(f, 1, -1);
    fp->ft_data = s_ftData;
    fp->co_attrs = *s_ftData->x0;
    fp->x24 = s_ftData->xC;
    fp->x28 = (u8(*)[2]) s_ftData->x10;
    fp->kind = Ft_Kind_Mario;
    fp->x597_bits = Ft_Kind_Mario;
    fp->player_id = (u8) slot;
    fp->x618_player_id = (u8) (port < 0 ? 0 : port);
    fp->x61A_controller_index = (u8) slot;
    fp->team = (u8) slot;
    fp->cpu.kind = 0;
    s_playerCpu[slot] = port < 0 ? Gm_PKind_Cpu : Gm_PKind_Human;
    s_playerPos[slot].x = x;
    s_playerPos[slot].y = y;
    s_playerPos[slot].z = 0.0f;
    s_playerFacing[slot] = facing;
    f->traceMotion = -1;
    fp->x34_scale.x = fp->x34_scale.y = fp->x34_scale.z = 1.0f;
    fp->x18 = ftCo_MS_Count;
    fp->x1C_actionStateList = s_commonStates;
    fp->x20_actionStateList = s_fighterStates;
    fp->anim_id = -1;
    fp->gobj = gobj;
    fp->dat_attrs_backup = f->datAttrs;
    fp->x890_cameraBox = &f->cameraSubject;
    ftMr_Init_OnLoad(gobj);
    fp->x21FC_flag.byte = 1;
    fp->smash_attrs.x2135 = -1;
    fp->coll_data.floor.index = -1;
    fp->coll_data.floor_skip = -1;
    for (i = 0; i < (int) ARRAY_SIZE(fp->x8B0); ++i) {
        fp->x8B0[i].x10 = -1;
        fp->x8B0[i].x11 = -1;
    }
    scale.x = scale.y = scale.z = ModelScale(fp);
    HSD_JObjSetScale(root, &scale);
    f->modelIdx = 0;
    ftParts_80074A4C(gobj, 0, 0);
    ftColl_8007B320(gobj);
    HSD_GObj_SetupProc(gobj, Fighter_8006A1BC, 0);
    HSD_GObj_SetupProc(gobj, Fighter_8006A360, 1);
    HSD_GObj_SetupProc(gobj, Fighter_8006ABA0, 2);
    HSD_GObj_SetupProc(gobj, Fighter_Spaghetti_8006AD10, 3);
    HSD_GObj_SetupProc(gobj, Fighter_procUpdate, 4);
    HSD_GObj_SetupProc(gobj, Fighter_procMap, 6);
    HSD_GObj_SetupProc(gobj, Fighter_8006C80C, 9);
    HSD_GObj_SetupProc(gobj, Fighter_8006CB94, 0xD);
    HSD_GObj_SetupProc(gobj, ProcHit, 0xE);
    HSD_GObj_SetupProc(gobj, ProcFinish, 0x11);
    M360_FighterRespawn(gobj, x, y);
    ProcFinish(gobj);
    M360_MatchTrace("fighter.spawn.joints", f->jointCount);
    M360_MatchTrace("fighter.spawn.dobjs", f->dobjCount);
    return gobj;
}

void M360_FighterSetDead(void* handle)
{
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    Owner(gobj)->dead = 1;
    fp->x221F_b3 = true;
    ftColl_8007AFF8(gobj);
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    M360_MatchTrace("fighter.dead.player", fp->player_id);
}

void M360_FighterRespawn(void* handle, float x, float y)
{
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    Owner(gobj)->dead = 0;
    fp->x221F_b3 = false;
    s_playerPos[fp->player_id].x = x;
    s_playerPos[fp->player_id].y = y;
    Fighter_UnkInitReset_80067C98(fp);
    Fighter_ResetInputData_80068854(gobj);
    ftMr_Init_OnDeath(gobj);
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    fp->gr_vel = 0.0f;
    fp->x1968_jumpsUsed = 1;
    fp->ground_or_air = GA_Air;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.last_pos = fp->cur_pos;
    fp->coll_data.floor.index = -1;
    fp->coll_data.floor_skip = -1;
    ftCo_Fall_Enter(gobj);
}

s32 Player_GetDamage(s32 slot)
{
    (void) slot;
    return 0;
}

void Player_LoadPlayerCoords(s32 slot, Vec3* out_vec)
{
    *out_vec = s_playerPos[slot < 6 && slot >= 0 ? slot : 0];
}

f32 Player_GetFacingDirection(s32 slot)
{
    return s_playerFacing[slot < 6 && slot >= 0 ? slot : 0];
}

Gm_PKind Player_8003248C(s32 slot, bool arg1)
{
    (void) arg1;
    return (Gm_PKind) s_playerCpu[slot < 6 && slot >= 0 ? slot : 0];
}

f32 Player_GetAttackRatio(int slot)
{
    (void) slot;
    return 1.0f;
}

f32 Player_GetDefenseRatio(int slot)
{
    (void) slot;
    return 1.0f;
}

void M360_FighterSetCpuLevel(unsigned level)
{
    s_cpuLevel = level;
}

unsigned M360_MatchPadTriggered(void)
{
    return HSD_PadGameStatus[0].trigger;
}

unsigned M360_MatchPadTriggeredPort(unsigned port)
{
    return port < 4 ? HSD_PadGameStatus[port].trigger : 0;
}

void M360_FighterSetPort(void* handle, int port)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    Owner((HSD_GObj*) handle)->port = port;
    fp->x618_player_id = (u8) (port < 0 ? 0 : port);
    s_playerCpu[fp->player_id] = port < 0 ? Gm_PKind_Cpu : Gm_PKind_Human;
}

unsigned M360_MatchPadHeld(void)
{
    return HSD_PadGameStatus[0].button;
}

float M360_MatchPadX(void)
{
    return HSD_PadGameStatus[0].nml_stickX;
}

float M360_MatchPadY(void)
{
    return HSD_PadGameStatus[0].nml_stickY;
}

int M360_MatchControllerConnected(unsigned port)
{
    return port < 4 && HSD_PadGameStatus[port].err >= 0;
}

void M360_FighterGetState(void* handle, float* x, float* y, float* facing,
                          unsigned* motion, unsigned* damage)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    *x = fp->cur_pos.x;
    *y = fp->cur_pos.y;
    *facing = fp->facing_dir;
    *motion = (unsigned) fp->motion_id;
    *damage = (unsigned) (fp->dmg.x1830_percent + 0.5f);
}

void M360_FighterCameraBox(void* handle, float* x, float* y, float* left,
                           float* right, float* up, float* down)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    UnkFloat6_Camera* box = fp->ft_data->x3C;
    const float s = fp->x34_scale.y;
    const float zoom = M360_MatchFixedZoom();
    *x = fp->cur_pos.x;
    *y = fp->cur_pos.y + box->x0.x * s;
    if (fp->facing_dir == 1.0f) {
        *left = box->x0.z * s;
        *right = box->x0.y * s * zoom;
    } else {
        *left = -box->x0.y * s * zoom;
        *right = -box->x0.z * s;
    }
    *up = box->xC.x * s;
    *down = box->xC.y * s;
}

unsigned M360_FighterHitCount(void)
{
    return s_hitCount;
}

