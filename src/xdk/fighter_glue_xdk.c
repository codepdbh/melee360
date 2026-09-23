#include <math.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/lb/lbanim.h>
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
#pragma warning(pop)

#include "match_xdk.h"

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
    kLinkFighter = 2
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
} M360Fighter;

ftCommonData* p_ftCommonData;
struct Fighter_804D651C_t* Fighter_804D651C;
struct Fighter_804D6520_t* Fighter_804D6520;
struct Fighter_804D6524_t* Fighter_804D6524;
DiscU32* ftPartsTable;

static DiscU32* s_commonData;
static ftData* s_ftData;
static HSD_Joint* s_modelJoint;
static HSD_MatAnimJoint* s_modelMatAnim;
static unsigned char* s_animImage;
static unsigned s_animImageSize;
static FigaTree* s_trees[kMaxAnims];
static M360Fighter s_fighters[kMaxFighters];
static unsigned s_hitCount;
static unsigned s_unported[64];
static const char* s_unportedName[64];
static unsigned s_unportedCount;

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

int M360_FighterLoad(void)
{
    s_commonData = LoadArchiveSymbol("PlCo.dat", "ftLoadCommonData");
    s_ftData = LoadArchiveSymbol("PlMr.dat", "ftDataMario");
    s_modelJoint = LoadArchiveSymbol("PlMrNr.dat", "PlyMario5K_Share_joint");
    if (!s_commonData || !s_ftData || !s_modelJoint)
        return 0;
    s_modelMatAnim = LoadArchiveSymbol("PlMrNr.dat", "PlyMario5K_Share_matanim_joint");
    s_animImage = M360_ReadDiscFile("PlMrAJ.dat", &s_animImageSize);
    M360_MatchTrace("fighter.aj.bytes", s_animImageSize);
    p_ftCommonData = (ftCommonData*) (uintptr_t) s_commonData[0].v;
    ftPartsTable = (DiscU32*) (uintptr_t) s_commonData[4].v;
    Fighter_804D6524 = (struct Fighter_804D6524_t*) (uintptr_t) s_commonData[12].v;
    Fighter_804D6520 = (struct Fighter_804D6520_t*) (uintptr_t) s_commonData[13].v;
    Fighter_804D651C = (struct Fighter_804D651C_t*) (uintptr_t) s_commonData[14].v;
    M360_MatchTrace("fighter.attr.gravity_x1000",
               (unsigned) (s_ftData->x0->gravity * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.walk_max_x1000",
               (unsigned) (s_ftData->x0->walk_max_vel * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.max_jumps", (unsigned) s_ftData->x0->max_jumps);
    return s_animImage != NULL;
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

void ftAnim_8006EBA4(Fighter_GObj* gobj)
{
    AnimAdvance(Owner(gobj));
    ftAction_80073240(gobj);
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

static const MotionState* FindMotion(int msid)
{
    unsigned i;
    for (i = 0; i < M360_MotionTableCount; ++i)
        if (M360_MotionTable[i].msid == msid)
            return &M360_MotionTable[i].state;
    return NULL;
}

void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId msid,
                               MotionFlags flags, f32 anim_start,
                               f32 anim_speed, f32 anim_blend,
                               Fighter_GObj* arg3)
{
    M360Fighter* f = Owner(gobj);
    Fighter* fp = &f->fighter;
    HSD_JObj* jobj = GET_JOBJ(gobj);
    const MotionState* state = FindMotion(msid);
    struct Fighter_WaitAnimData* anim;
    (void) anim_blend;
    (void) arg3;
    if (!state) {
        M360_MatchTrace("fighter.unported.motion", (unsigned) msid);
        state = FindMotion(fp->ground_or_air == GA_Air ? ftCo_MS_Fall : ftCo_MS_Wait);
        msid = fp->ground_or_air == GA_Air ? ftCo_MS_Fall : ftCo_MS_Wait;
    }
    fp->motion_id = msid;
    fp->facing_dir1 = fp->facing_dir;
    HSD_JObjSetTranslate(jobj, &fp->cur_pos);
    if (!(flags & Ft_MF_SkipHit)) {
        unsigned i;
        for (i = 0; i < kMaxHitboxes; ++i)
            f->hitboxes[i].active = 0;
    }
    if (!(flags & Ft_MF_SkipModel) && fp->x221D_b2)
        ftParts_80074A8C(gobj);
    if (!(flags & Ft_MF_Unk19))
        fp->x2222_b2 = 0;
    fp->x221F_b3 = 0;
    fp->x2219_b1 = 0;
    fp->x2219_b2 = 0;
    fp->x221A_b7 = 0;
    fp->x221B_b0 = 0;
    fp->x221C_b3 = 0;
    fp->x221D_b5 = 0;
    fp->x2218_b6 = 0;
    fp->x221C_b4 = 0;
    fp->x221D_b7 = 0;
    fp->x221E_b1 = 0;
    fp->x221E_b2 = 0;
    fp->x221F_b1 = 0;
    fp->x221E_b5 = 0;
    fp->x221E_b6 = 0;
    fp->x2220_b3 = 0;
    fp->x2220_b7 = 0;
    fp->x2223_b0 = 0;
    fp->x2222_b3 = 0;
    fp->x2224_b5 = 0;
    fp->x2225_b1 = 0;
    fp->x2225_b4 = 0;
    fp->x2224_b4 = false;
    if (!(flags & Ft_MF_KeepFastFall))
        fp->fall_fast = 0;
    if (fp->ground_or_air == GA_Ground) {
        fp->x2221_b5 = false;
        fp->x2221_b7 = true;
        fp->x2221_b6 = true;
        fp->x2224_b1 = false;
        fp->x2227_b1 = false;
        fp->x213C = -1;
    }
    if (msid != 0xE && msid != 0xF && msid != 0x10 && msid != 0x11)
        fp->hitlag_mul = 0.0f;
    fp->x2222_b7 = 0;
    fp->x100 = (flags & Ft_MF_UnkUpdatePhys) ? 0.0f : -1.0f;
    fp->lstick_angle = 0.0f;
    HSD_JObjSetRotationX(jobj, 0.0f);
    HSD_JObjSetRotationY(jobj, (float) M_PI_2 * fp->facing_dir);
    HSD_JObjSetRotationZ(jobj, 0.0f);
    fp->x2225_b3 = state->x9_b0;
    fp->anim_id = state->anim_id;
    fp->frame_speed_mul = anim_speed;
    fp->x8A0_unk = anim_speed;
    fp->cur_anim_frame = anim_start - fp->frame_speed_mul;
    fp->x898_unk = 0.0f;
    if (fp->anim_id != -1) {
        anim = &fp->x24[fp->anim_id];
        fp->x594_s32 = anim->x10_animCurrFlags;
        if (!(flags & Ft_MF_SkipAnim)) {
            ftData_80085CD8(fp, fp, fp->anim_id);
            fp->x3E4_fighterCmdScript.u = anim->xC;
            fp->x3E4_fighterCmdScript.loop_count = 0;
            if (anim_start) {
                if (fp->x590)
                    ftAnim_8006EBE8(gobj, anim_start - anim_speed, anim_speed, 0.0f);
                AnimAdvance(f);
                fp->x3E4_fighterCmdScript.timer = -anim_start;
            } else {
                if (fp->x590)
                    ftAnim_8006EBE8(gobj, anim_start, anim_speed, 0.0f);
                fp->x3E4_fighterCmdScript.timer = 0.0f;
            }
            AnimAdvance(f);
            if (fp->x594_b0) {
                if (!anim_start) {
                    fp->x6B0.x = fp->x6B0.y = fp->x6B0.z = 0.0f;
                    fp->x6A4_transNOffset.x = fp->x6A4_transNOffset.y =
                        fp->x6A4_transNOffset.z = 0.0f;
                    fp->x698 = fp->x68C_transNPos;
                } else if (!(flags & Ft_MF_SkipAnimVel) && fp->ground_or_air == GA_Ground) {
                    const float v = fp->x6A4_transNOffset.z * fp->facing_dir;
                    fp->self_vel.x = v;
                    fp->gr_vel = v;
                }
            }
            if (flags & Ft_MF_UpdateCmd)
                ftAction_8007349C(gobj);
            else if (anim_start)
                ftAction_80073354(gobj);
            else
                ftAction_80073240(gobj);
        } else {
            fp->anim_id = -1;
        }
    }
    if (fp->anim_id == -1) {
        fp->x594_s32 = 0;
        HSD_JObjRemoveAnimAllByFlags(jobj, 1);
        fp->x3E4_fighterCmdScript.u = NULL;
    }
    fp->anim_cb = state->anim_cb;
    fp->input_cb = state->input_cb;
    fp->phys_cb = state->phys_cb;
    fp->coll_cb = state->coll_cb;
    fp->cam_cb = state->cam_cb;
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
    return st->lines[line_id].flags | st->lines[line_id].kind;
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
    return GroundStep(gobj, 0) ? GA_Ground : GA_Air;
}

bool ft_800827A0(Fighter_GObj* gobj)
{
    return GroundStep(gobj, 1);
}

void ft_80084280(Fighter_GObj* gobj)
{
    if (GroundStep(gobj, 0))
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

static int AirStep(HSD_GObj* gobj, bool (*land)(Fighter_GObj*, int))
{
    Fighter* fp = GET_FIGHTER(gobj);
    const M360MatchStage* st = M360_MatchStageData();
    unsigned i;
    int best = -1;
    float bestY = -3.4e38f;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;
    if (fp->cur_pos.y > fp->prev_pos.y)
        return 0;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, ly;
        if (!(l->kind & M360_LINE_FLOOR))
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (fp->cur_pos.x < lo || fp->cur_pos.x > hi || hi - lo < 1e-4f)
            continue;
        t = (fp->cur_pos.x - l->x0) / (l->x1 - l->x0);
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (fp->prev_pos.y >= ly - 0.01f && fp->cur_pos.y <= ly && ly > bestY) {
            if (land && !land(gobj, (int) i))
                continue;
            bestY = ly;
            best = (int) i;
        }
    }
    if (best < 0)
        return 0;
    fp->cur_pos.y = bestY;
    fp->coll_data.cur_pos = fp->cur_pos;
    SetFloor(fp, best);
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

void ft_800831CC(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
}

void ft_800835B0(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
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

static void UpdateInput(Fighter* fp)
{
    HSD_PadStatus* pad = &HSD_PadGameStatus[fp->x618_player_id];
    const int cpu = fp->x618_player_id >= 4;
    ftCommonData* cd = p_ftCommonData;
    fp->input.lstick[1] = fp->input.lstick[0];
    fp->input.cstick[1] = fp->input.cstick[0];
    fp->input.triggers[1] = fp->input.triggers[0];
    fp->input.held_buttons[1] = fp->input.held_buttons[0];
    fp->input.lstick[0].x = cpu ? 0.0f : pad->nml_stickX;
    fp->input.lstick[0].y = cpu ? 0.0f : pad->nml_stickY;
    fp->input.cstick[0].x = cpu ? 0.0f : pad->nml_subStickX;
    fp->input.cstick[0].y = cpu ? 0.0f : pad->nml_subStickY;
    fp->input.triggers[0] = cpu ? 0.0f
        : (pad->nml_analogL > pad->nml_analogR ? pad->nml_analogL : pad->nml_analogR);
    if (!cpu && fp->input.lstick[0].x == 0.0f) {
        if (pad->button & HSD_PAD_DPADLEFT)
            fp->input.lstick[0].x = -0.5f;
        else if (pad->button & HSD_PAD_DPADRIGHT)
            fp->input.lstick[0].x = 0.5f;
    }
    if (fabsf(fp->input.lstick[0].x) <= cd->horizontal_stick_deadzone)
        fp->input.lstick[0].x = 0.0f;
    if (fabsf(fp->input.lstick[0].y) <= cd->vertical_stick_deadzone)
        fp->input.lstick[0].y = 0.0f;
    if (fabsf(fp->input.cstick[0].x) <= cd->horizontal_stick_deadzone)
        fp->input.cstick[0].x = 0.0f;
    if (fabsf(fp->input.cstick[0].y) <= cd->vertical_stick_deadzone)
        fp->input.cstick[0].y = 0.0f;
    if (fp->input.triggers[0] <= cd->analog_shoulder_deadzone)
        fp->input.triggers[0] = 0.0f;
    fp->input.held_buttons[0] = cpu ? 0 : pad->button & ~(HSD_PAD_DPADLEFT | HSD_PAD_DPADRIGHT);
    if (fp->input.held_buttons[0] & (HSD_PAD_L | HSD_PAD_R)) {
        fp->input.held_buttons[0] |= HSD_PAD_LR;
        fp->input.triggers[0] = 1.0f;
    } else if (fp->input.triggers[0]) {
        fp->input.held_buttons[0] |= HSD_PAD_LR;
    }
    if (fp->input.held_buttons[0] & HSD_PAD_Z) {
        fp->input.held_buttons[0] |= HSD_PAD_LR | HSD_PAD_A;
        fp->input.triggers[0] = cd->z_press_analog_value;
    }
    fp->input.pressed_buttons = fp->input.held_buttons[0] &
        (fp->input.held_buttons[1] ^ fp->input.held_buttons[0]);
    fp->input.released_buttons = fp->input.held_buttons[1] &
        (fp->input.held_buttons[1] ^ fp->input.held_buttons[0]);
    if (++fp->x676_x > 0xFE) fp->x676_x = 0xFE;
    if (fp->input.lstick[0].x >= cd->horizontal_stick_smash_deadzone ||
        fp->input.lstick[0].x <= -cd->horizontal_stick_smash_deadzone) {
        const int same = fp->input.lstick[0].x > 0.0f
            ? fp->input.lstick[1].x >= cd->horizontal_stick_smash_deadzone
            : fp->input.lstick[1].x <= -cd->horizontal_stick_smash_deadzone;
        if (same) {
            if (++fp->x670_timer_lstick_tilt_x > 0xFE) fp->x670_timer_lstick_tilt_x = 0xFE;
            if (++fp->x673 > 0xFE) fp->x673 = 0xFE;
            if (++fp->x679_x > 0xFE) fp->x679_x = 0xFE;
        } else {
            fp->x676_x = 0;
            fp->x673 = 0;
            fp->x670_timer_lstick_tilt_x = 0;
            fp->x2228_b7 = fp->input.lstick[0].x > 0.0f;
        }
    } else {
        fp->x679_x = 0xFE;
        fp->x673 = 0xFE;
        fp->x670_timer_lstick_tilt_x = 0xFE;
    }
    if (++fp->x677_y > 0xFE) fp->x677_y = 0xFE;
    if (fp->input.lstick[0].y >= cd->vertical_stick_smash_deadzone ||
        fp->input.lstick[0].y <= -cd->vertical_stick_smash_deadzone) {
        const int same = fp->input.lstick[0].y > 0.0f
            ? fp->input.lstick[1].y >= cd->vertical_stick_smash_deadzone
            : fp->input.lstick[1].y <= -cd->vertical_stick_smash_deadzone;
        if (same) {
            if (++fp->x671_timer_lstick_tilt_y > 0xFE) fp->x671_timer_lstick_tilt_y = 0xFE;
            if (++fp->x674 > 0xFE) fp->x674 = 0xFE;
            if (++fp->x67A_y > 0xFE) fp->x67A_y = 0xFE;
        } else {
            fp->x677_y = 0;
            fp->x674 = 0;
            fp->x671_timer_lstick_tilt_y = 0;
            fp->x2229_b0 = fp->input.lstick[0].y < 0.0f;
        }
    } else {
        fp->x67A_y = 0xFE;
        fp->x674 = 0xFE;
        fp->x671_timer_lstick_tilt_y = 0xFE;
    }
    if (fp->input.pressed_buttons & HSD_PAD_A) {
        fp->x683 = fp->x67C;
        fp->x67C = 0;
    } else if (fp->x67C < 0xFF) {
        ++fp->x67C;
    }
    if (fp->input.pressed_buttons & HSD_PAD_B) fp->x67D = 0;
    else if (fp->x67D < 0xFF) ++fp->x67D;
    if (fp->input.pressed_buttons & HSD_PAD_XY) fp->x67E = 0;
    else if (fp->x67E < 0xFF) ++fp->x67E;
}

static void ProcAnim(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->pos_delta.x = fp->cur_pos.x - fp->prev_pos.x;
    fp->pos_delta.y = fp->cur_pos.y - fp->prev_pos.y;
    fp->pos_delta.z = fp->cur_pos.z - fp->prev_pos.z;
    fp->prev_pos = fp->cur_pos;
    if (Owner(gobj)->hitlag > 0.0f)
        return;
    ftAnim_8006EBA4(gobj);
    if (fp->anim_cb)
        fp->anim_cb(gobj);
}

static void ProcInput(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    UpdateInput(fp);
    if (Owner(gobj)->hitlag > 0.0f)
        return;
    if (fp->input_cb)
        fp->input_cb(gobj);
}

static void ProcUpdate(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    M360Fighter* f = Owner(gobj);
    if (f->hitlag > 0.0f) {
        f->hitlag -= 1.0f;
        return;
    }
    if (fp->phys_cb)
        fp->phys_cb(gobj);
    if (fp->x8c_kb_vel.x != 0.0f || fp->x8c_kb_vel.y != 0.0f) {
        if (fp->ground_or_air == GA_Air) {
            const float angle = atan2f(fp->x8c_kb_vel.y, fp->x8c_kb_vel.x);
            const float mag = sqrtf(fp->x8c_kb_vel.x * fp->x8c_kb_vel.x +
                                    fp->x8c_kb_vel.y * fp->x8c_kb_vel.y);
            if (mag < p_ftCommonData->x204_knockbackFrameDecay) {
                fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = 0.0f;
            } else {
                fp->x8c_kb_vel.x -= p_ftCommonData->x204_knockbackFrameDecay * cosf(angle);
                fp->x8c_kb_vel.y -= p_ftCommonData->x204_knockbackFrameDecay * sinf(angle);
            }
        } else {
            fp->x8c_kb_vel.x *= 0.9f;
            if (fabsf(fp->x8c_kb_vel.x) < 0.05f)
                fp->x8c_kb_vel.x = 0.0f;
            fp->x8c_kb_vel.y = 0.0f;
        }
    }
    fp->gr_vel += fp->xE4_ground_accel_1 + fp->xE8_ground_accel_2;
    fp->xE4_ground_accel_1 = fp->xE8_ground_accel_2 = 0.0f;
    fp->self_vel.x += fp->x74_self_accel.x;
    fp->self_vel.y += fp->x74_self_accel.y;
    fp->self_vel.z += fp->x74_self_accel.z;
    fp->x74_self_accel.x = fp->x74_self_accel.y = fp->x74_self_accel.z = 0.0f;
    fp->cur_pos.x += fp->self_vel.x + fp->x8c_kb_vel.x;
    fp->cur_pos.y += fp->self_vel.y + fp->x8c_kb_vel.y;
}

static void ProcMap(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (Owner(gobj)->hitlag > 0.0f)
        return;
    HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
    if (fp->coll_cb)
        fp->coll_cb(gobj);
    HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
}

static void ProcFinish(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
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
    fp->x618_player_id = (u8) (port < 0 ? 4 : port);
    fp->x34_scale.x = fp->x34_scale.y = fp->x34_scale.z = 1.0f;
    fp->x18 = ftCo_MS_Count;
    fp->anim_id = -1;
    fp->gobj = gobj;
    fp->x670_timer_lstick_tilt_x = fp->x671_timer_lstick_tilt_y = 0xFE;
    fp->x679_x = fp->x67A_y = 0xFE;
    fp->x673 = fp->x674 = 0xFE;
    fp->x67C = fp->x67D = fp->x67E = 0xFF;
    fp->coll_data.floor.index = -1;
    fp->dmg.x18C8 = -1;
    for (i = 0; i < (int) ARRAY_SIZE(fp->x8B0); ++i) {
        fp->x8B0[i].x10 = -1;
        fp->x8B0[i].x11 = -1;
    }
    scale.x = scale.y = scale.z = ModelScale(fp);
    HSD_JObjSetScale(root, &scale);
    f->modelIdx = 0;
    ftParts_80074A4C(gobj, 0, 0);
    HSD_GObj_SetupProc(gobj, ProcAnim, 1);
    HSD_GObj_SetupProc(gobj, ProcInput, 3);
    HSD_GObj_SetupProc(gobj, ProcUpdate, 4);
    HSD_GObj_SetupProc(gobj, ProcMap, 6);
    HSD_GObj_SetupProc(gobj, ProcFinish, 9);
    M360_FighterRespawn(gobj, x, y);
    fp->facing_dir = facing;
    ProcFinish(gobj);
    M360_MatchTrace("fighter.spawn.joints", f->jointCount);
    M360_MatchTrace("fighter.spawn.dobjs", f->dobjCount);
    return gobj;
}

void M360_FighterRespawn(void* handle, float x, float y)
{
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    fp->cur_pos.x = fp->prev_pos.x = x;
    fp->cur_pos.y = fp->prev_pos.y = y;
    fp->cur_pos.z = fp->prev_pos.z = 0.0f;
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    fp->gr_vel = 0.0f;
    fp->dmg.x1830_percent = 0.0f;
    fp->x1968_jumpsUsed = 1;
    fp->ground_or_air = GA_Air;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.floor.index = -1;
    Owner(gobj)->hitlag = 0.0f;
    ftCo_Fall_Enter(gobj);
}

unsigned M360_MatchPadTriggered(void)
{
    return HSD_PadGameStatus[0].trigger;
}

unsigned M360_MatchPadHeld(void)
{
    return HSD_PadGameStatus[0].button;
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

float ftColl_80079AB0(Fighter* fp, HitCapsule* hit, u32 unk_count, float arg3,
                      float attack, float defense, float weight);
void ftColl_8007AD18(Fighter* fp, HitCapsule* hit);
void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);

static float SegmentDistance(const Vec3* p, const Vec3* a, const Vec3* b)
{
    Vec3 ab, ap;
    float t, len;
    ab.x = b->x - a->x; ab.y = b->y - a->y; ab.z = b->z - a->z;
    ap.x = p->x - a->x; ap.y = p->y - a->y; ap.z = p->z - a->z;
    len = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    t = len > 0.0f ? (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / len : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    ap.x -= ab.x * t; ap.y -= ab.y * t; ap.z -= ab.z * t;
    return sqrtf(ap.x * ap.x + ap.y * ap.y + ap.z * ap.z);
}

static int HurtOverlap(M360Fighter* target, const HitCapsule* hit)
{
    Fighter* tfp = &target->fighter;
    ftData_x30* hurt = tfp->ft_data->x30;
    const float scale = ModelScale(tfp);
    int i;
    for (i = 0; hurt && i < hurt->count; ++i) {
        ftHurtboxInit* init = &hurt->inits[i];
        Vec3 a, b;
        if ((unsigned) init->bone_idx >= target->jointCount)
            continue;
        a.x = init->a_offset.x; a.y = init->a_offset.y; a.z = init->a_offset.z;
        b.x = init->b_offset.x; b.y = init->b_offset.y; b.z = init->b_offset.z;
        lb_8000B1CC(target->joints[init->bone_idx], &a, &a);
        lb_8000B1CC(target->joints[init->bone_idx], &b, &b);
        if (SegmentDistance(&hit->x4C, &a, &b) <= hit->scale + init->scale * scale)
            return 1;
    }
    return 0;
}

static void ApplyHit(M360Fighter* attacker, M360Fighter* target, HitCapsule* hit)
{
    Fighter* afp = &attacker->fighter;
    Fighter* tfp = &target->fighter;
    float kb, angle, speed;
    tfp->dmg.x1838_percentTemp = hit->damage;
    kb = ftColl_80079AB0(tfp, hit, hit->unk_count, 1.0f, 1.0f, 1.0f, tfp->co_attrs.weight);
    tfp->dmg.x1830_percent += hit->damage;
    if (tfp->dmg.x1830_percent > 999.0f)
        tfp->dmg.x1830_percent = 999.0f;
    tfp->dmg.x1838_percentTemp = 0.0f;
    tfp->dmg.kb_applied = kb;
    angle = hit->kb_angle == 361 ? (tfp->ground_or_air == GA_Air ? 45.0f : 0.0f)
                                 : (float) hit->kb_angle;
    speed = kb * p_ftCommonData->x100;
    tfp->facing_dir = afp->cur_pos.x < tfp->cur_pos.x ? -1.0f : 1.0f;
    tfp->x8c_kb_vel.x = speed * cosf(angle * 0.017453292f) * afp->facing_dir;
    tfp->x8c_kb_vel.y = speed * sinf(angle * 0.017453292f);
    tfp->self_vel.x = tfp->self_vel.y = 0.0f;
    tfp->gr_vel = 0.0f;
    if (tfp->x8c_kb_vel.y > 0.0f) {
        ftCommon_8007D5D4(tfp);
        ftCo_Fall_Enter(target->gobj);
    }
    hit->x44 = 1;
    ++s_hitCount;
    M360_MatchTrace("fighter.hit.damage", (unsigned) hit->damage);
    M360_MatchTrace("fighter.hit.knockback_x100", (unsigned) (kb * 100.0f));
    M360_MatchTrace("fighter.hit.target_percent", (unsigned) tfp->dmg.x1830_percent);
}

void M360_FighterResolveHits(void* attackerHandle, void* targetHandle)
{
    M360Fighter* pair[2];
    int k;
    unsigned i;
    pair[0] = Owner((HSD_GObj*) attackerHandle);
    pair[1] = Owner((HSD_GObj*) targetHandle);
    for (k = 0; k < 2; ++k) {
        M360Fighter* a = pair[k];
        M360Fighter* t = pair[1 - k];
        for (i = 0; i < ARRAY_SIZE(a->fighter.x914); ++i) {
            HitCapsule* hit = &a->fighter.x914[i];
            if (hit->state == HitCapsule_Disabled)
                continue;
            ftColl_8007AD18(&a->fighter, hit);
            if (!hit->x44 && HurtOverlap(t, hit))
                ApplyHit(a, t, hit);
        }
    }
}

unsigned M360_FighterHitCount(void)
{
    return s_hitCount;
}
