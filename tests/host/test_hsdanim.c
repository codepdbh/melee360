#include "hsdanim_xdk_compat.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include "aobj.h"
#include "bytecode.h"
#include "class.h"
#include "cobj.h"
#include "dobj.h"
#include "fobj.h"
#include "fog.h"
#include "id.h"
#include "jobj.h"
#include "list.h"
#include "lobj.h"
#include "mobj.h"
#include "mtx.h"
#include "object.h"
#include "pobj.h"
#include "robj.h"
#include "tobj.h"
#include "util.h"
#include "wobj.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4054 4055)
#endif

extern void M360_HSD_HeapInit(void);
extern int M360_HsdAnimSelfTest(void);

static int fail_count = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "[M360][ANIM][FAIL] %s (%s:%d)\n", #cond,        \
                    __FILE__, __LINE__);                                     \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                \
    do {                                                                     \
        double a_ = (double) (a);                                            \
        double b_ = (double) (b);                                            \
        if (!(fabs(a_ - b_) <= (double) (eps))) {                            \
            fprintf(stderr,                                                  \
                    "[M360][ANIM][FAIL] %s=%.9g vs %s=%.9g (%s:%d)\n", #a,   \
                    a_, #b, b_, __FILE__, __LINE__);                         \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

#define EPS 1e-4

static void put_f32(u8* dst, f32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    dst[0] = (u8) (bits & 0xFF);
    dst[1] = (u8) ((bits >> 8) & 0xFF);
    dst[2] = (u8) ((bits >> 16) & 0xFF);
    dst[3] = (u8) ((bits >> 24) & 0xFF);
}

typedef struct UpdateLog {
    int calls;
    int last_type;
    f32 last_value;
    f32 by_type[16];
} UpdateLog;

static void on_update(void* obj, enum_t type, HSD_ObjData* fval)
{
    UpdateLog* log = (UpdateLog*) obj;
    log->calls++;
    log->last_type = (int) type;
    log->last_value = fval->fv;
    if (type >= 0 && type < 16) {
        log->by_type[type] = fval->fv;
    }
}

static void fill_fdesc(HSD_FObjDesc* d, u8* stream, u32 length, u8 type,
                       u8 frac_value, u8 frac_slope, HSD_FObjDesc* next)
{
    memset(d, 0, sizeof(*d));
    d->next = next;
    d->length = length;
    d->startframe = 0.0f;
    d->type = type;
    d->frac_value = frac_value;
    d->frac_slope = frac_slope;
    d->ad = stream;
}

static void fill_adesc(HSD_AObjDesc* a, HSD_FObjDesc* f, u32 flags,
                       f32 end_frame, u32 obj_id)
{
    memset(a, 0, sizeof(*a));
    a->flags = flags;
    a->end_frame = end_frame;
    a->fobjdesc = f;
    a->obj_id = obj_id;
}

static u8 g_lin[10];
static u8 g_con[] = { 0x11, 0x80, 0x01, 5, 0x00, 0xFF, 3 };
static u8 g_key[] = { 0x16, 5, 4, 9 };

static void init_streams(void)
{
    g_lin[0] = 0x12;
    put_f32(g_lin + 1, 0.0f);
    g_lin[5] = 10;
    put_f32(g_lin + 6, 100.0f);
}

static HSD_AObj* make_lin_aobj(u32 flags, f32 end_frame)
{
    HSD_FObjDesc fd;
    HSD_AObjDesc ad;
    fill_fdesc(&fd, g_lin, sizeof(g_lin), TYPE_JOBJ, HSD_A_FRAC_FLOAT,
               HSD_A_FRAC_FLOAT, NULL);
    fill_adesc(&ad, &fd, flags, end_frame, 0);
    return HSD_AObjLoadDesc(&ad);
}

static u32 using_count(HSD_ObjAllocData* d)
{
    return HSD_ObjAllocGetUsing(d);
}

static void test_util(void)
{
    GXColor a = { 255, 128, 0, 51 };
    GXColor b = { 255, 255, 100, 255 };
    GXColor out;

    CHECK(HSD_GetNbBits(0) == 0);
    CHECK(HSD_GetNbBits(1) == 1);
    CHECK(HSD_GetNbBits(0xFF) == 8);
    CHECK(HSD_GetNbBits(0x80000001u) == 2);
    CHECK(HSD_GetNbBits(0xFFFFFFFFu) == 32);
    CHECK(HSD_GetNbBits(0x10101010u) == 4);

    CHECK(HSD_Index2PosNrmMtx(0) == 0);
    CHECK(HSD_Index2PosNrmMtx(1) == 3);
    CHECK(HSD_Index2PosNrmMtx(5) == 15);
    CHECK(HSD_Index2PosNrmMtx(9) == 27);

    HSD_MulColor(&a, &b, &out);
    CHECK(out.r == 255 && out.g == 128 && out.b == 0 && out.a == 51);
    b.r = 128;
    b.g = 64;
    b.b = 200;
    b.a = 128;
    HSD_MulColor(&a, &b, &out);
    CHECK(out.r == (255 * 128) / 255);
    CHECK(out.g == (128 * 64) / 255);
    CHECK(out.b == 0);
    CHECK(out.a == (51 * 128) / 255);

    CHECK(HSD_identityMtx[0][0] == 1.0f && HSD_identityMtx[1][1] == 1.0f &&
          HSD_identityMtx[2][2] == 1.0f && HSD_identityMtx[0][1] == 0.0f &&
          HSD_identityMtx[1][3] == 0.0f);
}

static void test_bytecode(void)
{
    f32 args[3];
    u8 mul_add[] = { 0x02, 0x00, 0x00, 0x06, 0x40, 0x20, 0x00, 0x00, 0x19,
                     0x02, 0x00, 0x01, 0x17, 0x01 };
    u8 sub_neg[] = { 0x02, 0x00, 0x00, 0x02, 0x00, 0x01, 0x18, 0x09, 0x01 };
    u8 fn[] = { 0x02, 0x00, 0x02, 0x16, 0x01 };
    u8 dup_pop[] = { 0x02, 0x00, 0x00, 0x3C, 0x00, 0x17, 0x02, 0x00, 0x01,
                     0x05, 0x01, 0x01 };
    u8 clamp[] = { 0x02, 0x00, 0x00, 0x02, 0x00, 0x01, 0x22, 0x01 };
    u8 branch[] = { 0x06, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x06, 0x06,
                    0x3F, 0x80, 0x00, 0x00, 0x01, 0x06, 0x40, 0x00, 0x00,
                    0x00, 0x01 };

    args[0] = 4.0f;
    args[1] = 3.0f;
    args[2] = 9.0f;

    CHECK_NEAR(HSD_ByteCodeEval(mul_add, args, 3), 4.0 * 2.5 + 3.0, 1e-6);
    CHECK_NEAR(HSD_ByteCodeEval(sub_neg, args, 3), -(4.0 - 3.0), 1e-6);
    CHECK_NEAR(HSD_ByteCodeEval(fn, args, 3), 3.0, 1e-6);
    CHECK_NEAR(HSD_ByteCodeEval(dup_pop, args, 3), 8.0, 1e-6);
    CHECK_NEAR(HSD_ByteCodeEval(clamp, args, 3), 3.0, 1e-6);
    CHECK(HSD_ByteCodeEval(NULL, args, 3) == 0.0f);
    CHECK_NEAR(HSD_ByteCodeEval(branch, args, 3), 2.0, 1e-6);
    branch[4] = 0x00;
    CHECK_NEAR(HSD_ByteCodeEval(branch, args, 3), 1.0, 1e-6);
}

static void test_aobj_basics(void)
{
    HSD_AObj* a;
    HSD_FObj* f;
    HSD_ObjAllocData* pool = HSD_AObjGetAllocData();

    CHECK(pool != NULL);
    CHECK(using_count(pool) == 0);

    a = HSD_AObjAlloc();
    CHECK(a != NULL);
    CHECK(using_count(pool) == 1);
    CHECK(a->flags == AOBJ_NO_ANIM);
    CHECK(a->framerate == 1.0f);
    CHECK(a->curr_frame == 0.0f && a->rewind_frame == 0.0f &&
          a->end_frame == 0.0f);
    CHECK(a->fobj == NULL && a->hsd_obj == NULL);
    CHECK(HSD_AObjGetFlags(a) == AOBJ_NO_ANIM);
    CHECK(HSD_AObjGetFlags(NULL) == 0);

    HSD_AObjSetFlags(a, 0xFFFFFFFFu);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP | AOBJ_NO_UPDATE));
    HSD_AObjClearFlags(a, 0xFFFFFFFFu);
    CHECK(a->flags == AOBJ_NO_ANIM);
    HSD_AObjSetFlags(a, AOBJ_LOOP);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP));
    HSD_AObjSetFlags(a, AOBJ_FIRST_PLAY | AOBJ_REWINDED);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP));
    HSD_AObjClearFlags(a, AOBJ_NO_ANIM);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP));
    HSD_AObjClearFlags(a, AOBJ_LOOP);
    CHECK(a->flags == AOBJ_NO_ANIM);
    HSD_AObjSetFlags(NULL, AOBJ_LOOP);
    HSD_AObjClearFlags(NULL, AOBJ_LOOP);

    HSD_AObjSetRate(a, 0.25f);
    HSD_AObjSetRewindFrame(a, 3.0f);
    HSD_AObjSetEndFrame(a, 42.5f);
    CHECK(a->framerate == 0.25f && a->rewind_frame == 3.0f &&
          a->end_frame == 42.5f);
    CHECK(HSD_AObjGetEndFrame(a) == 42.5f);
    HSD_AObjSetCurrentFrame(a, 7.0f);
    CHECK(a->curr_frame == 0.0f);
    a->flags = 0;
    HSD_AObjSetCurrentFrame(a, 7.0f);
    CHECK(a->curr_frame == 7.0f && HSD_AObjGetCurrFrame(a) == 7.0f);
    CHECK(a->flags == AOBJ_FIRST_PLAY);
    HSD_AObjSetRate(NULL, 1.0f);
    HSD_AObjSetRewindFrame(NULL, 1.0f);
    HSD_AObjSetEndFrame(NULL, 1.0f);
    HSD_AObjSetCurrentFrame(NULL, 1.0f);

    CHECK(using_count(HSD_FObjGetAllocData()) == 0);
    f = HSD_FObjAlloc();
    HSD_AObjSetFObj(a, f);
    CHECK(a->fobj == f && using_count(HSD_FObjGetAllocData()) == 1);
    f = HSD_FObjAlloc();
    HSD_AObjSetFObj(a, f);
    CHECK(a->fobj == f && using_count(HSD_FObjGetAllocData()) == 1);
    HSD_AObjSetFObj(a, NULL);
    CHECK(a->fobj == NULL && using_count(HSD_FObjGetAllocData()) == 0);
    HSD_AObjSetFObj(NULL, NULL);

    HSD_AObjFree(NULL);
    HSD_AObjRemove(NULL);
    f = HSD_FObjAlloc();
    a->fobj = f;
    HSD_AObjRemove(a);
    CHECK(using_count(pool) == 0);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);

    HSD_AObjInitEndCallBack();
    HSD_AObjInvokeCallBacks();
    _HSD_AObjForgetMemory(NULL, NULL);
}

static void test_aobj_stepping(void)
{
    UpdateLog log;
    HSD_AObj* a;
    int k;

    memset(&log, 0, sizeof(log));

    a = make_lin_aobj(0, 10.0f);
    CHECK(a != NULL);
    CHECK(a->flags == AOBJ_NO_ANIM);
    CHECK(a->end_frame == 10.0f && a->rewind_frame == 0.0f);
    CHECK(a->fobj != NULL && a->fobj->ad_head == g_lin);

    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 0 && a->curr_frame == 0.0f);

    HSD_AObjReqAnim(a, 0.0f);
    CHECK(a->flags == AOBJ_FIRST_PLAY);
    CHECK(HSD_FObjGetState(a->fobj) == 1);
    for (k = 0; k <= 10; k++) {
        log.calls = 0;
        HSD_AObjInterpretAnim(a, &log, on_update);
        CHECK(log.calls == 1);
        CHECK(log.last_type == TYPE_JOBJ);
        CHECK_NEAR(log.last_value, 10.0 * k, 1e-3);
        CHECK_NEAR(a->curr_frame, (double) k, 0.0);
        if (k < 10) {
            CHECK(a->flags == 0);
        }
    }
    CHECK(a->flags == AOBJ_NO_ANIM);
    CHECK(HSD_FObjGetState(a->fobj) == 0);
    log.calls = 0;
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 0);
    HSD_AObjInterpretAnim(NULL, &log, on_update);
    HSD_AObjRemove(a);

    a = make_lin_aobj(0, 10.0f);
    HSD_AObjSetRate(a, 0.5f);
    HSD_AObjReqAnim(a, 0.0f);
    for (k = 0; k <= 8; k++) {
        log.calls = 0;
        HSD_AObjInterpretAnim(a, &log, on_update);
        CHECK(log.calls == 1);
        CHECK_NEAR(a->curr_frame, 0.5 * k, 1e-6);
        CHECK_NEAR(log.last_value, 10.0 * 0.5 * k, 1e-3);
    }
    HSD_AObjReqAnim(a, 4.0f);
    CHECK(a->curr_frame == 4.0f && a->flags == AOBJ_FIRST_PLAY);
    log.calls = 0;
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 1);
    CHECK_NEAR(log.last_value, 40.0, 1e-3);
    CHECK(a->curr_frame == 4.0f);
    HSD_AObjRemove(a);

    a = make_lin_aobj(0, 10.0f);
    HSD_AObjReqAnim(a, 0.0f);
    HSD_AObjInterpretAnim(a, &log, on_update);
    HSD_AObjSetCurrentFrame(a, 6.0f);
    CHECK(a->curr_frame == 6.0f);
    CHECK(a->flags == AOBJ_FIRST_PLAY);
    log.calls = 0;
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 1);
    CHECK_NEAR(log.last_value, 60.0, 1e-3);
    HSD_AObjStopAnim(a, &log, on_update);
    CHECK(a->flags == AOBJ_NO_ANIM);
    CHECK(HSD_FObjGetState(a->fobj) == 0);
    HSD_AObjSetCurrentFrame(a, 2.0f);
    CHECK(a->curr_frame == 6.0f);
    HSD_AObjStopAnim(NULL, &log, on_update);
    HSD_AObjRemove(a);

    a = make_lin_aobj(AOBJ_NO_UPDATE, 10.0f);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_NO_UPDATE));
    HSD_AObjReqAnim(a, 0.0f);
    CHECK(a->flags == (AOBJ_NO_UPDATE | AOBJ_FIRST_PLAY));
    log.calls = 0;
    for (k = 0; k < 5; k++) {
        HSD_AObjInterpretAnim(a, &log, on_update);
    }
    CHECK(log.calls == 0);
    CHECK_NEAR(a->curr_frame, 4.0, 0.0);
    HSD_AObjRemove(a);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);
}

static void test_aobj_loop(void)
{
    UpdateLog log;
    HSD_AObj* a;
    int k;

    memset(&log, 0, sizeof(log));

    a = make_lin_aobj(AOBJ_LOOP, 10.0f);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP));
    HSD_AObjReqAnim(a, 0.0f);
    CHECK(a->flags == (AOBJ_LOOP | AOBJ_FIRST_PLAY));
    for (k = 0; k < 10; k++) {
        log.calls = 0;
        HSD_AObjInterpretAnim(a, &log, on_update);
        CHECK(log.calls == 1);
        CHECK_NEAR(log.last_value, 10.0 * k, 1e-3);
        CHECK((a->flags & AOBJ_REWINDED) == 0);
    }
    log.calls = 0;
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 1);
    CHECK_NEAR(a->curr_frame, 0.0, 1e-6);
    CHECK_NEAR(log.last_value, 0.0, 1e-3);
    CHECK((a->flags & AOBJ_REWINDED) != 0);
    CHECK((a->flags & AOBJ_NO_ANIM) == 0);
    for (k = 1; k < 10; k++) {
        log.calls = 0;
        HSD_AObjInterpretAnim(a, &log, on_update);
        CHECK(log.calls == 1);
        CHECK_NEAR(log.last_value, 10.0 * k, 1e-3);
        CHECK((a->flags & AOBJ_REWINDED) == 0);
    }
    HSD_AObjRemove(a);

    a = make_lin_aobj(AOBJ_LOOP, 10.0f);
    HSD_AObjSetRewindFrame(a, 4.0f);
    HSD_AObjReqAnim(a, 0.0f);
    for (k = 0; k <= 10; k++) {
        HSD_AObjInterpretAnim(a, &log, on_update);
    }
    CHECK_NEAR(a->curr_frame, 4.0, 1e-6);
    CHECK_NEAR(log.last_value, 40.0, 1e-3);
    CHECK((a->flags & AOBJ_REWINDED) != 0);
    HSD_AObjSetRate(a, 3.0f);
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK_NEAR(a->curr_frame, 7.0, 1e-6);
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK_NEAR(a->curr_frame, 4.0, 1e-6);
    HSD_AObjRemove(a);

    a = make_lin_aobj(AOBJ_LOOP, 10.0f);
    HSD_AObjSetRewindFrame(a, 10.0f);
    HSD_AObjReqAnim(a, 0.0f);
    for (k = 0; k <= 10; k++) {
        HSD_AObjInterpretAnim(a, &log, on_update);
    }
    CHECK_NEAR(a->curr_frame, 10.0, 0.0);
    CHECK((a->flags & AOBJ_REWINDED) != 0);
    HSD_AObjRemove(a);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);
}

static void test_aobj_fobj_shapes(void)
{
    UpdateLog log;
    HSD_FObjDesc d2, d1;
    HSD_AObjDesc ad;
    HSD_AObj* a;
    int k;

    memset(&log, 0, sizeof(log));

    fill_fdesc(&d2, g_key, sizeof(g_key), 5, HSD_A_FRAC_U8, 0, NULL);
    fill_fdesc(&d1, g_lin, sizeof(g_lin), 12, HSD_A_FRAC_FLOAT,
               HSD_A_FRAC_FLOAT, &d2);
    fill_adesc(&ad, &d1, 0, 10.0f, 0);
    a = HSD_AObjLoadDesc(&ad);
    CHECK(a->fobj != NULL && a->fobj->next != NULL &&
          a->fobj->next->next == NULL);
    HSD_AObjReqAnim(a, 0.0f);

    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 2);
    CHECK_NEAR(log.by_type[12], 0.0, 1e-3);
    CHECK_NEAR(log.by_type[5], 5.0, 1e-6);
    for (k = 1; k <= 3; k++) {
        log.calls = 0;
        HSD_AObjInterpretAnim(a, &log, on_update);
        CHECK(log.calls == 1);
        CHECK(log.last_type == 12);
        CHECK_NEAR(log.last_value, 10.0 * k, 1e-3);
    }
    log.calls = 0;
    HSD_AObjInterpretAnim(a, &log, on_update);
    CHECK(log.calls == 2);
    CHECK_NEAR(log.by_type[5], 9.0, 1e-6);
    CHECK_NEAR(log.by_type[12], 40.0, 1e-3);
    HSD_AObjRemove(a);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);
}

static HSD_JObj* new_jobj(void)
{
    HSD_JObj* j = (HSD_JObj*) hsdNew(HSD_CLASS_INFO(&hsdJObj));
    PSMTXIdentity(j->mtx);
    return j;
}

static void test_jobj_class_and_refs(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdJObj);
    HSD_JObj* j;
    HSD_JObj* kid;

    j = new_jobj();
    CHECK(j != NULL);
    CHECK(HSD_CLASS_METHOD(j) == info);
    CHECK(strcmp(info->head.class_name, "hsd_jobj") == 0);
    CHECK(info->head.obj_size == (s16) sizeof(HSD_JObj));
    CHECK(info->head.parent == &hsdObj);
    CHECK(hsdIsDescendantOf(info, &hsdClass));
    CHECK(hsdObjIsDescendantOf(&j->object, info));
    CHECK(info->head.nb_exist == 1);
    CHECK(ref_CNT(j) == 0 && iref_CNT(j) == 0);

    HSD_JObjRefThis(j);
    CHECK(iref_CNT(j) == 1);
    HSD_JObjUnrefThis(j);
    CHECK(iref_CNT(j) == 0 && info->head.nb_exist == 1);
    HSD_JObjUnrefThis(NULL);

    HSD_JObjRefThis(j);
    HSD_OBJ(j)->ref_count = HSD_OBJ_NOREF;
    HSD_JObjUnrefThis(j);
    CHECK(info->head.nb_exist == 0);

    j = new_jobj();
    HSD_JObjUnref(j);
    CHECK(info->head.nb_exist == 0);

    j = new_jobj();
    kid = new_jobj();
    HSD_JObjAddChild(j, kid);
    ref_INC(j);
    ref_INC(j);
    HSD_JObjUnref(j);
    CHECK(ref_CNT(j) == 1 && info->head.nb_exist == 2);
    HSD_JObjUnref(j);
    CHECK(ref_CNT(j) == 0 && info->head.nb_exist == 2 && j->child == kid);
    HSD_JObjRefThis(j);
    HSD_JObjUnref(j);
    CHECK(info->head.nb_exist == 1 && iref_CNT(j) == 1 && ref_CNT(j) == -1);
    CHECK(j->child == NULL);
    HSD_JObjUnrefThis(j);
    CHECK(info->head.nb_exist == 0);
    HSD_JObjUnref(NULL);
    CHECK(HSD_JObjGetFlags(NULL) == 0);
}

static HSD_Joint g_obj_joint;

static void test_aobj_loaddesc_objid(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdJObj);
    HSD_FObjDesc fd;
    HSD_AObjDesc ad;
    HSD_AObj* a;
    HSD_JObj* j;
    HSD_JObj* fresh;

    CHECK(HSD_AObjLoadDesc(NULL) == NULL);

    fill_fdesc(&fd, g_lin, sizeof(g_lin), TYPE_JOBJ, HSD_A_FRAC_FLOAT,
               HSD_A_FRAC_FLOAT, NULL);
    fill_adesc(&ad, &fd, AOBJ_LOOP | AOBJ_FIRST_PLAY | AOBJ_NO_UPDATE, 12.5f,
               0);
    a = HSD_AObjLoadDesc(&ad);
    CHECK(a->flags == (AOBJ_NO_ANIM | AOBJ_LOOP | AOBJ_NO_UPDATE));
    CHECK(a->end_frame == 12.5f && a->rewind_frame == 0.0f);
    CHECK(a->hsd_obj == NULL);
    HSD_AObjRemove(a);

    j = new_jobj();
    HSD_IDInsertToTable(NULL, 0x1234, j);
    fill_adesc(&ad, &fd, 0, 5.0f, 0x1234);
    a = HSD_AObjLoadDesc(&ad);
    CHECK(a->hsd_obj == HSD_OBJ(j));
    CHECK(ref_CNT(j) == 1);
    CHECK(info->head.nb_exist == 1);
    HSD_AObjRemove(a);
    CHECK(ref_CNT(j) == 0 && info->head.nb_exist == 1);
    HSD_IDRemoveByIDFromTable(NULL, 0x1234);

    memset(&g_obj_joint, 0, sizeof(g_obj_joint));
    g_obj_joint.scale.x = 1.0f;
    g_obj_joint.scale.y = 1.0f;
    g_obj_joint.scale.z = 1.0f;
    if ((uintptr_t) &g_obj_joint <= 0xFFFFFFFFu) {
        s32 found = 1;
        fill_adesc(&ad, &fd, 0, 5.0f, (u32) (uintptr_t) &g_obj_joint);
        a = HSD_AObjLoadDesc(&ad);
        fresh = (HSD_JObj*) a->hsd_obj;
        CHECK(fresh != NULL && HSD_CLASS_METHOD(fresh) == info);
        CHECK(fresh->id == (uintptr_t) &g_obj_joint);
        CHECK(HSD_IDGetDataFromTable(NULL, (uintptr_t) &g_obj_joint, NULL) ==
              fresh);
        CHECK(ref_CNT(fresh) == 0);
        CHECK(info->head.nb_exist == 2);
        HSD_AObjRemove(a);
        CHECK(info->head.nb_exist == 1);
        HSD_IDGetDataFromTable(NULL, (uintptr_t) &g_obj_joint, &found);
        CHECK(found == 0);
    } else {
        printf("[M360][ANIM] obj_id joint load skipped: host joint address "
               "does not fit the 32-bit obj_id field\n");
    }
    hsdDelete(j);
    CHECK(info->head.nb_exist == 0);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);
}

typedef struct FLog {
    int n;
    HSD_AObj* aobj[64];
    void* obj[64];
    int type[64];
    f32 f;
    u32 d;
    void* v;
} FLog;

static FLog fl;

static void fl_add(HSD_AObj* a, void* o, int t)
{
    if (fl.n < 64) {
        fl.aobj[fl.n] = a;
        fl.obj[fl.n] = o;
        fl.type[fl.n] = t;
        fl.n++;
    }
}

static void cb_A(HSD_AObj* a)
{
    fl_add(a, NULL, 0);
}
static void cb_AF(HSD_AObj* a, f32 f)
{
    fl_add(a, NULL, 0);
    fl.f = f;
}
static void cb_AV(HSD_AObj* a, void* v)
{
    fl_add(a, NULL, 0);
    fl.v = v;
}
static void cb_AU(HSD_AObj* a, u32 d)
{
    fl_add(a, NULL, 0);
    fl.d = d;
}
static void cb_AO(HSD_AObj* a, void* o)
{
    fl_add(a, o, 0);
}
static void cb_AOT(HSD_AObj* a, void* o, HSD_Type t)
{
    fl_add(a, o, (int) t);
}
static void cb_AOF(HSD_AObj* a, void* o, f32 f)
{
    fl_add(a, o, 0);
    fl.f = f;
}
static void cb_AOV(HSD_AObj* a, void* o, void* v)
{
    fl_add(a, o, 0);
    fl.v = v;
}
static void cb_AOU(HSD_AObj* a, void* o, u32 d)
{
    fl_add(a, o, 0);
    fl.d = d;
}
static void cb_AOTF(HSD_AObj* a, void* o, HSD_Type t, f32 f)
{
    fl_add(a, o, (int) t);
    fl.f = f;
}
static void cb_AOTV(HSD_AObj* a, void* o, HSD_Type t, void* v)
{
    fl_add(a, o, (int) t);
    fl.v = v;
}
static void cb_AOTU(HSD_AObj* a, void* o, HSD_Type t, u32 d)
{
    fl_add(a, o, (int) t);
    fl.d = d;
}

static void* zalloc(size_t n)
{
    void* p = calloc(1, n);
    if (p == NULL) {
        abort();
    }
    return p;
}

typedef struct Graph {
    HSD_AObj* a[22];
    HSD_JObj *root, *c1, *c2;
    HSD_DObj *d1, *d2;
    HSD_MObj* m1;
    HSD_TObj *t1, *t2;
    HSD_PObj* p1;
    HSD_ShapeSet* s1;
    HSD_RObj *r1, *r2, *r3;
    HSD_WObj *w1, *w2, *w3, *w4, *w5;
    HSD_CObj* cobj;
    HSD_LObj *l1, *l2;
    HSD_Fog* fog;
} Graph;

static Graph g;

static void build_graph(void)
{
    int i;
    memset(&g, 0, sizeof(g));
    for (i = 1; i <= 21; i++) {
        g.a[i] = HSD_AObjAlloc();
    }
    g.root = zalloc(sizeof(HSD_JObj));
    g.c1 = zalloc(sizeof(HSD_JObj));
    g.c2 = zalloc(sizeof(HSD_JObj));
    g.d1 = zalloc(sizeof(HSD_DObj));
    g.d2 = zalloc(sizeof(HSD_DObj));
    g.m1 = zalloc(sizeof(HSD_MObj));
    g.t1 = zalloc(sizeof(HSD_TObj));
    g.t2 = zalloc(sizeof(HSD_TObj));
    g.p1 = zalloc(sizeof(HSD_PObj));
    g.s1 = zalloc(sizeof(HSD_ShapeSet));
    g.r1 = zalloc(sizeof(HSD_RObj));
    g.r2 = zalloc(sizeof(HSD_RObj));
    g.r3 = zalloc(sizeof(HSD_RObj));
    g.w1 = zalloc(sizeof(HSD_WObj));
    g.w2 = zalloc(sizeof(HSD_WObj));
    g.w3 = zalloc(sizeof(HSD_WObj));
    g.w4 = zalloc(sizeof(HSD_WObj));
    g.w5 = zalloc(sizeof(HSD_WObj));
    g.cobj = zalloc(sizeof(HSD_CObj));
    g.l1 = zalloc(sizeof(HSD_LObj));
    g.l2 = zalloc(sizeof(HSD_LObj));
    g.fog = zalloc(sizeof(HSD_Fog));

    g.root->aobj = g.a[1];
    g.root->robj = g.r1;
    g.root->u.dobj = g.d1;
    g.root->child = g.c1;
    g.c1->aobj = g.a[10];
    g.c1->next = g.c2;
    g.c2->aobj = g.a[11];
    g.r1->aobj = g.a[2];
    g.r1->next = g.r2;
    g.r2->aobj = g.a[3];
    g.d1->aobj = g.a[4];
    g.d1->mobj = g.m1;
    g.d1->pobj = g.p1;
    g.d1->next = g.d2;
    g.d2->aobj = g.a[9];
    g.m1->aobj = g.a[5];
    g.m1->tobj = g.t1;
    g.t1->aobj = g.a[6];
    g.t1->next = g.t2;
    g.t2->aobj = g.a[7];
    g.p1->flags = POBJ_SHAPEANIM;
    g.p1->u.shape_set = g.s1;
    g.s1->aobj = g.a[8];

    g.w1->aobj = g.a[12];
    g.w1->robj = g.r3;
    g.r3->aobj = g.a[13];
    g.cobj->aobj = g.a[14];
    g.w2->aobj = g.a[15];
    g.w3->aobj = g.a[16];
    g.cobj->eyepos = g.w2;
    g.cobj->interest = g.w3;
    g.l1->aobj = g.a[17];
    g.w4->aobj = g.a[18];
    g.w5->aobj = g.a[19];
    g.l1->position = g.w4;
    g.l1->interest = g.w5;
    g.l1->next = g.l2;
    g.l2->aobj = g.a[20];
    g.fog->aobj = g.a[21];
}

static void destroy_graph(void)
{
    int i;
    for (i = 1; i <= 21; i++) {
        HSD_AObjFree(g.a[i]);
    }
    free(g.root);
    free(g.c1);
    free(g.c2);
    free(g.d1);
    free(g.d2);
    free(g.m1);
    free(g.t1);
    free(g.t2);
    free(g.p1);
    free(g.s1);
    free(g.r1);
    free(g.r2);
    free(g.r3);
    free(g.w1);
    free(g.w2);
    free(g.w3);
    free(g.w4);
    free(g.w5);
    free(g.cobj);
    free(g.l1);
    free(g.l2);
    free(g.fog);
}

static int fl_matches(const int* idx, int count)
{
    int i;
    if (fl.n != count) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (fl.aobj[i] != g.a[idx[i]]) {
            return 0;
        }
    }
    return 1;
}

static void run_foreach(void* obj, HSD_Type type, HSD_TypeMask mask)
{
    fl.n = 0;
    HSD_ForeachAnim(obj, type, mask, (void*) cb_A, AOBJ_ARG_A);
}

static void test_foreach_anim(void)
{
    static const int all[] = { 1, 4, 5, 6, 7, 8, 9, 2, 3, 10, 11 };
    static const int jonly[] = { 1, 10, 11 };
    static const int dt[] = { 4, 6, 7, 9 };
    static const int no_children[] = { 1, 4, 5, 6, 7, 8, 9, 2, 3 };
    static const int no_dobj[] = { 1, 2, 3, 10, 11 };
    static const int dobj_all[] = { 4, 5, 6, 7, 8, 9 };
    static const int mobj_all[] = { 5, 6, 7 };
    static const int tobj_all[] = { 6, 7 };
    static const int robj_all[] = { 2, 3 };
    static const int wobj_all[] = { 12, 13 };
    static const int cobj_all[] = { 14, 15, 16 };
    static const int lobj_all[] = { 17, 18, 19, 20 };
    static const int only8[] = { 8 };
    static const int only21[] = { 21 };

    build_graph();

    run_foreach(g.root, JOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(all, 11));
    run_foreach(g.root, JOBJ_TYPE, JOBJ_MASK);
    CHECK(fl_matches(jonly, 3));
    run_foreach(g.root, JOBJ_TYPE, DOBJ_MASK | TOBJ_MASK);
    CHECK(fl_matches(dt, 4));
    run_foreach(g.root, JOBJ_TYPE, POBJ_MASK);
    CHECK(fl_matches(only8, 1));
    run_foreach(g.root, JOBJ_TYPE, 0);
    CHECK(fl.n == 0);

    g.root->flags = JOBJ_INSTANCE;
    run_foreach(g.root, JOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(no_children, 9));
    g.root->flags = JOBJ_PTCL;
    run_foreach(g.root, JOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(no_dobj, 5));
    g.root->flags = JOBJ_SPLINE;
    run_foreach(g.root, JOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(no_dobj, 5));
    g.root->flags = 0;

    g.p1->flags = POBJ_SKIN;
    run_foreach(g.root, JOBJ_TYPE, POBJ_MASK);
    CHECK(fl.n == 0);
    g.p1->flags = POBJ_ENVELOPE;
    run_foreach(g.d1, DOBJ_TYPE, POBJ_MASK);
    CHECK(fl.n == 0);
    g.p1->flags = POBJ_SHAPEANIM;
    g.s1->aobj = NULL;
    run_foreach(g.d1, DOBJ_TYPE, POBJ_MASK);
    CHECK(fl.n == 0);
    g.s1->aobj = g.a[8];

    run_foreach(g.d1, DOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(dobj_all, 6));
    run_foreach(g.m1, MOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(mobj_all, 3));
    run_foreach(g.p1, POBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(only8, 1));
    run_foreach(g.t1, TOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(tobj_all, 2));
    run_foreach(g.r1, ROBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(robj_all, 2));
    run_foreach(g.w1, WOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(wobj_all, 2));
    run_foreach(g.cobj, COBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(cobj_all, 3));
    run_foreach(g.l1, LOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(lobj_all, 4));
    run_foreach(g.fog, FOG_TYPE, ALL_TYPE_MASK);
    CHECK(fl_matches(only21, 1));
    run_foreach(g.fog, FOG_TYPE, JOBJ_MASK);
    CHECK(fl.n == 0);
    run_foreach(NULL, JOBJ_TYPE, ALL_TYPE_MASK);
    CHECK(fl.n == 0);

    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AF, AOBJ_ARG_AF,
                    2.5);
    CHECK(fl.n == 1 && fl.f == 2.5f && fl.aobj[0] == g.a[21]);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AV, AOBJ_ARG_AV,
                    (void*) &g);
    CHECK(fl.n == 1 && fl.v == (void*) &g);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AU, AOBJ_ARG_AU,
                    0xDEADBEEFu);
    CHECK(fl.n == 1 && fl.d == 0xDEADBEEFu);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AO, AOBJ_ARG_AO);
    CHECK(fl.n == 1 && fl.obj[0] == g.fog);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AOT, AOBJ_ARG_AOT);
    CHECK(fl.n == 1 && fl.obj[0] == g.fog && fl.type[0] == FOG_TYPE);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AOF, AOBJ_ARG_AOF,
                    -7.25);
    CHECK(fl.n == 1 && fl.obj[0] == g.fog && fl.f == -7.25f);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AOV, AOBJ_ARG_AOV,
                    (void*) &fl);
    CHECK(fl.n == 1 && fl.obj[0] == g.fog && fl.v == (void*) &fl);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AOU, AOBJ_ARG_AOU,
                    77u);
    CHECK(fl.n == 1 && fl.obj[0] == g.fog && fl.d == 77u);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.fog, FOG_TYPE, FOG_MASK, (void*) cb_AOTF, AOBJ_ARG_AOTF,
                    0.125);
    CHECK(fl.n == 1 && fl.type[0] == FOG_TYPE && fl.f == 0.125f);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.root, JOBJ_TYPE, JOBJ_MASK, (void*) cb_AOTV, AOBJ_ARG_AOTV,
                    (void*) &g);
    CHECK(fl.n == 3 && fl.type[0] == JOBJ_TYPE && fl.obj[0] == g.root &&
          fl.v == (void*) &g);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.root, JOBJ_TYPE, JOBJ_MASK, (void*) cb_AOTU, AOBJ_ARG_AOTU,
                    0x1234u);
    CHECK(fl.n == 3 && fl.type[2] == JOBJ_TYPE && fl.obj[2] == g.c2 &&
          fl.d == 0x1234u);
    memset(&fl, 0, sizeof(fl));
    HSD_ForeachAnim(g.root, JOBJ_TYPE, JOBJ_MASK, (void*) cb_A, AOBJ_ARG_A);
    CHECK(fl_matches(jonly, 3));

    destroy_graph();
}

typedef struct UpdLog {
    int n;
    int type[16];
    int has[16];
    f32 v[16][3];
} UpdLog;

static void upd_cb(void* obj, enum_t type, HSD_ObjData* val)
{
    UpdLog* l = (UpdLog*) (((HSD_JObj*) obj)->id);
    if (l->n < 16) {
        l->type[l->n] = (int) type;
        l->has[l->n] = val != NULL;
        if (val != NULL) {
            l->v[l->n][0] = val->p.x;
            l->v[l->n][1] = val->p.y;
            l->v[l->n][2] = val->p.z;
        }
        l->n++;
    }
}

static void robj_flags_init(HSD_RObj* r, u32 flags, HSD_JObj* j)
{
    r->flags = flags;
    r->u.jobj = j;
}

static HSD_JObj* plain_jobj_at(f32 x, f32 y, f32 z)
{
    HSD_JObj* j = (HSD_JObj*) zalloc(sizeof(HSD_JObj));
    hsdIsDescendantOf(&hsdJObj, &hsdObj);
    j->object.parent.class_info = HSD_CLASS_INFO(&hsdJObj);
    j->scale.x = 1.0f;
    j->scale.y = 1.0f;
    j->scale.z = 1.0f;
    j->translate.x = x;
    j->translate.y = y;
    j->translate.z = z;
    PSMTXIdentity(j->mtx);
    j->mtx[0][3] = x;
    j->mtx[1][3] = y;
    j->mtx[2][3] = z;
    return j;
}

static void set_rotz90(Mtx m)
{
    PSMTXIdentity(m);
    m[0][0] = 0.0f;
    m[0][1] = -1.0f;
    m[1][0] = 1.0f;
    m[1][1] = 0.0f;
}

static void expect_vec(UpdLog* l, int i, int type, f32 x, f32 y, f32 z)
{
    CHECK(l->n > i);
    if (l->n > i) {
        CHECK(l->type[i] == type);
        CHECK(l->has[i] == 1);
        CHECK_NEAR(l->v[i][0], x, EPS);
        CHECK_NEAR(l->v[i][1], y, EPS);
        CHECK_NEAR(l->v[i][2], z, EPS);
    }
}

static void test_robj_constraints(void)
{
    UpdLog log;
    HSD_JObj* obj = plain_jobj_at(0.0f, 0.0f, 0.0f);
    HSD_JObj* j1 = plain_jobj_at(2.0f, 4.0f, 6.0f);
    HSD_JObj* j2 = plain_jobj_at(4.0f, 8.0f, 10.0f);
    HSD_JObj* j3 = plain_jobj_at(0.0f, 0.0f, 10.0f);
    HSD_JObj* j4 = plain_jobj_at(5.0f, 0.0f, 0.0f);
    HSD_RObj r1, r2, r3;
    Vec3 pos;
    int n;

    memset(&r1, 0, sizeof(r1));
    memset(&r2, 0, sizeof(r2));
    memset(&r3, 0, sizeof(r3));
    memset(&log, 0, sizeof(log));
    obj->id = (uintptr_t) &log;

    CHECK(HSD_RObjGetGlobalPosition(NULL, 1, &pos) == 0);

    robj_flags_init(&r1, 0x80000000u | REFTYPE_JOBJ | 1, j1);
    robj_flags_init(&r2, 0x80000000u | REFTYPE_JOBJ | 1, j2);
    r1.next = &r2;
    robj_flags_init(&r3, 0x80000000u | REFTYPE_JOBJ | 2, j3);
    r2.next = &r3;

    memset(&pos, 0, sizeof(pos));
    n = HSD_RObjGetGlobalPosition(&r1, 1, &pos);
    CHECK(n == 2);
    CHECK_NEAR(pos.x, 3.0, EPS);
    CHECK_NEAR(pos.y, 6.0, EPS);
    CHECK_NEAR(pos.z, 8.0, EPS);
    memset(&pos, 0, sizeof(pos));
    n = HSD_RObjGetGlobalPosition(&r1, 2, &pos);
    CHECK(n == 1);
    CHECK_NEAR(pos.z, 10.0, EPS);
    CHECK(HSD_RObjGetGlobalPosition(&r1, 9, &pos) == 0);
    r2.flags &= 0x7FFFFFFFu;
    CHECK(HSD_RObjGetGlobalPosition(&r1, 1, &pos) == 1);
    CHECK_NEAR(pos.x, 2.0, EPS);
    r2.flags |= 0x80000000u;

    j1->mtx[0][3] = 99.0f;
    j1->flags = JOBJ_MTX_DIRTY;
    HSD_RObjGetGlobalPosition(&r1, 1, &pos);
    CHECK(!(j1->flags & JOBJ_MTX_DIRTY));
    CHECK_NEAR(j1->mtx[0][3], 2.0, EPS);
    j1->mtx[0][3] = 99.0f;
    j1->flags = JOBJ_MTX_DIRTY | JOBJ_USER_DEF_MTX;
    HSD_RObjGetGlobalPosition(&r1, 1, &pos);
    CHECK((j1->flags & JOBJ_MTX_DIRTY) && j1->mtx[0][3] == 99.0f);
    j1->mtx[0][3] = 2.0f;
    j1->flags = 0;

    r3.flags = 0;
    r2.next = NULL;
    log.n = 0;
    HSD_RObjUpdateAll(&r1, obj, upd_cb);
    CHECK(log.n == 2);
    expect_vec(&log, 0, 0x35, 3.0f, 6.0f, 8.0f);
    CHECK(log.type[1] == 0x38 && log.has[1] == 0);
    HSD_RObjUpdateAll(NULL, obj, upd_cb);
    CHECK(log.n == 2);

    memset(&r1, 0, sizeof(r1));
    robj_flags_init(&r1, 0x80000000u | REFTYPE_JOBJ | 2, j3);
    log.n = 0;
    HSD_RObjUpdateAll(&r1, obj, upd_cb);
    CHECK(log.n == 4);
    expect_vec(&log, 0, 50, 0.0f, 0.0f, 1.0f);
    expect_vec(&log, 1, 51, 0.0f, 1.0f, 0.0f);
    expect_vec(&log, 2, 52, -1.0f, 0.0f, 0.0f);
    CHECK(log.type[3] == 55 && log.has[3] == 0);

    {
        Vec3 scl;
        scl.x = 2.0f;
        scl.y = 3.0f;
        scl.z = 4.0f;
        obj->scl = &scl;
        log.n = 0;
        HSD_RObjUpdateAll(&r1, obj, upd_cb);
        CHECK(log.n == 4);
        expect_vec(&log, 0, 50, 0.0f, 0.0f, 2.0f);
        expect_vec(&log, 1, 51, 0.0f, 3.0f, 0.0f);
        expect_vec(&log, 2, 52, -4.0f, 0.0f, 0.0f);
        obj->scl = NULL;
    }

    memset(&r2, 0, sizeof(r2));
    robj_flags_init(&r2, 0x80000000u | REFTYPE_JOBJ | 3, j4);
    r1.next = &r2;
    log.n = 0;
    HSD_RObjUpdateAll(&r1, obj, upd_cb);
    CHECK(log.n == 4);
    expect_vec(&log, 0, 50, 0.0f, 0.0f, 1.0f);
    expect_vec(&log, 1, 51, 1.0f, 0.0f, 0.0f);
    expect_vec(&log, 2, 52, 0.0f, 1.0f, 0.0f);

    obj->mtx[0][3] = 0.0f;
    free(obj);
    free(j1);
    free(j2);
    free(j3);
    free(j4);
}

static void test_robj_orientation(void)
{
    UpdLog log;
    HSD_JObj* obj = plain_jobj_at(0.0f, 0.0f, 0.0f);
    HSD_JObj* target = plain_jobj_at(0.0f, 0.0f, 0.0f);
    HSD_JObj* parent = plain_jobj_at(0.0f, 0.0f, 0.0f);
    HSD_RObj r;
    int i;

    memset(&r, 0, sizeof(r));
    memset(&log, 0, sizeof(log));
    obj->id = (uintptr_t) &log;
    obj->mtx[0][0] = 3.0f;
    obj->mtx[1][1] = 4.0f;
    obj->mtx[2][2] = 5.0f;

    set_rotz90(target->mtx);
    for (i = 0; i < 3; i++) {
        target->mtx[0][i] *= 2.0f;
        target->mtx[1][i] *= 2.0f;
        target->mtx[2][i] *= 2.0f;
    }
    target->flags = JOBJ_CLASSICAL_SCALE;
    robj_flags_init(&r, 0x80000000u | REFTYPE_JOBJ | 4, target);
    HSD_RObjUpdateAll(&r, obj, upd_cb);
    CHECK(log.n == 4);
    expect_vec(&log, 0, 0x32, 0.0f, 3.0f, 0.0f);
    expect_vec(&log, 1, 0x33, -4.0f, 0.0f, 0.0f);
    expect_vec(&log, 2, 0x34, 0.0f, 0.0f, 5.0f);
    CHECK(log.type[3] == 0x37 && log.has[3] == 0);

    log.n = 0;
    target->flags = 0;
    target->parent = parent;
    HSD_RObjUpdateAll(&r, obj, upd_cb);
    CHECK(log.n == 4);
    expect_vec(&log, 0, 0x32, 0.0f, 3.0f, 0.0f);
    expect_vec(&log, 1, 0x33, -4.0f, 0.0f, 0.0f);
    expect_vec(&log, 2, 0x34, 0.0f, 0.0f, 5.0f);

    obj->mtx[0][0] = 1.0f;
    obj->mtx[1][1] = 1.0f;
    obj->mtx[2][2] = 1.0f;
    set_rotz90(parent->mtx);
    PSMTXIdentity(target->mtx);
    target->mtx[0][0] = 0.0f;
    target->mtx[0][1] = 0.0f;
    target->mtx[0][2] = 1.0f;
    target->mtx[1][0] = 1.0f;
    target->mtx[1][1] = 0.0f;
    target->mtx[1][2] = 0.0f;
    target->mtx[2][0] = 0.0f;
    target->mtx[2][1] = 1.0f;
    target->mtx[2][2] = 0.0f;
    target->parent = parent;
    target->flags = JOBJ_CLASSICAL_SCALE;
    log.n = 0;
    HSD_RObjUpdateAll(&r, obj, upd_cb);
    CHECK(log.n == 4);
    expect_vec(&log, 0, 0x32, 0.0f, 1.0f, 0.0f);
    expect_vec(&log, 1, 0x33, 0.0f, 0.0f, 1.0f);
    expect_vec(&log, 2, 0x34, 1.0f, 0.0f, 0.0f);
    CHECK(log.type[3] == 0x37);

    free(obj);
    free(target);
    free(parent);
}

static void test_robj_limits(void)
{
    UpdLog log;
    HSD_JObj* obj = plain_jobj_at(0.0f, 0.0f, 0.0f);
    HSD_RObj r[6];
    int i;

    memset(r, 0, sizeof(r));
    memset(&log, 0, sizeof(log));
    obj->id = (uintptr_t) &log;
    obj->rotate.x = -1.0f;
    obj->rotate.y = 2.0f;
    obj->rotate.z = 0.3f;
    obj->translate.x = 5.0f;
    obj->translate.y = 1.0f;
    obj->translate.z = 1.0f;

    r[0].flags = REFTYPE_LIMIT | 1;
    r[0].u.limit = -0.5f;
    r[1].flags = REFTYPE_LIMIT | 4;
    r[1].u.limit = 1.5f;
    r[2].flags = REFTYPE_LIMIT | 7;
    r[2].u.limit = 6.0f;
    r[3].flags = REFTYPE_LIMIT | 5;
    r[3].u.limit = 0.1f;
    for (i = 0; i < 3; i++) {
        r[i].next = &r[i + 1];
    }

    obj->mtx[0][3] = -1000.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK(log.n == 0);
    CHECK_NEAR(obj->rotate.x, -0.5, 1e-6);
    CHECK_NEAR(obj->rotate.y, 1.5, 1e-6);
    CHECK_NEAR(obj->rotate.z, 0.3, 1e-6);
    CHECK_NEAR(obj->translate.x, 6.0, 1e-6);
    CHECK_NEAR(obj->mtx[0][3], 6.0, 1e-6);
    CHECK_NEAR(obj->mtx[1][3], 1.0, 1e-6);

    memset(r, 0, sizeof(r));
    obj->rotate.z = 0.3f;
    r[0].flags = REFTYPE_LIMIT | 6;
    r[0].u.limit = 0.1f;
    r[1].flags = REFTYPE_LIMIT | 8;
    r[1].u.limit = 2.0f;
    r[2].flags = REFTYPE_LIMIT | 3;
    r[2].u.limit = -5.0f;
    r[0].next = &r[1];
    r[1].next = &r[2];
    obj->mtx[0][3] = -1000.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK_NEAR(obj->rotate.z, 0.1, 1e-6);
    CHECK_NEAR(obj->translate.x, 2.0, 1e-6);
    CHECK_NEAR(obj->rotate.y, 1.5, 1e-6);
    CHECK_NEAR(obj->mtx[0][3], 2.0, 1e-6);

    memset(r, 0, sizeof(r));
    r[0].flags = REFTYPE_LIMIT | 13;
    r[0].u.limit = 100.0f;
    obj->mtx[0][3] = -1000.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK(obj->mtx[0][3] == -1000.0f);

    memset(r, 0, sizeof(r));
    r[0].flags = REFTYPE_LIMIT | 3;
    r[0].u.limit = -5.0f;
    obj->mtx[0][3] = -1000.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK_NEAR(obj->mtx[0][3], 2.0, 1e-6);

    memset(r, 0, sizeof(r));
    obj->translate.y = 1.0f;
    obj->translate.z = 1.0f;
    r[0].flags = REFTYPE_LIMIT | 11;
    r[0].u.limit = 9.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK_NEAR(obj->translate.y, 9.0, 1e-6);
    CHECK_NEAR(obj->translate.z, 1.0, 1e-6);
    r[0].flags = REFTYPE_LIMIT | 12;
    r[0].u.limit = 4.0f;
    HSD_RObjUpdateAll(&r[0], obj, upd_cb);
    CHECK_NEAR(obj->translate.y, 4.0, 1e-6);
    CHECK_NEAR(obj->translate.z, 1.0, 1e-6);

    free(obj);
}

static f32 wsum5(void* a)
{
    f32* p = (f32*) a;
    return p[0] + 2.0f * p[1] + 3.0f * p[2] + 4.0f * p[3] + 5.0f * p[4];
}

static f32 exp_five_hundred(void* a)
{
    (void) a;
    return 500.0f;
}

static u8 g_bc[] = { 0x02, 0x00, 0x00, 0x06, 0x40, 0x20, 0x00, 0x00, 0x19,
                     0x02, 0x00, 0x01, 0x17, 0x01 };

static void test_robj_load_and_eval(void)
{
    HSD_ObjAllocData* rp = HSD_RObjGetAllocData();
    HSD_ObjAllocData* vp = HSD_RvalueObjGetAllocData();
    HSD_JObj *j1, *j2, *j3;
    HSD_RObjDesc d[6];
    HSD_ExpDesc expd;
    HSD_ByteCodeExpDesc bcd;
    HSD_IKHintDesc ikd;
    HSD_RvalueList rl[4];
    HSD_RvalueList bl[2];
    HSD_RObj* list;
    HSD_RObj* e;
    HSD_RObj* b;
    HSD_Rvalue* rv;
    UpdLog log;
    HSD_JObj* obj = plain_jobj_at(0.0f, 0.0f, 0.0f);
    int n;

    memset(&log, 0, sizeof(log));
    obj->id = (uintptr_t) &log;

    j1 = new_jobj();
    j2 = new_jobj();
    j3 = new_jobj();
    j1->rotate.x = 0.5f;
    j1->translate.x = 7.0f;
    j2->translate.x = 9.0f;
    j3->scale.x = 2.0f;
    j3->scale.y = 3.0f;
    j3->scale.z = 4.0f;
    HSD_IDInsertToTable(NULL, 101, j1);
    HSD_IDInsertToTable(NULL, 102, j2);
    HSD_IDInsertToTable(NULL, 103, j3);

    memset(d, 0, sizeof(d));
    memset(&expd, 0, sizeof(expd));
    memset(&bcd, 0, sizeof(bcd));
    memset(rl, 0, sizeof(rl));
    memset(bl, 0, sizeof(bl));
    rl[0].flags = 0x1 | 0x10;
    rl[0].joint = (HSD_Joint*) (uintptr_t) 101;
    rl[1].flags = 0x100000;
    rl[1].joint = (HSD_Joint*) (uintptr_t) 102;
    rl[2].flags = 0x800000 | 0x1000000;
    rl[2].joint = (HSD_Joint*) (uintptr_t) 103;
    expd.func = (void*) wsum5;
    expd.rvalue = rl;
    bl[0].flags = 0x1 | 0x10;
    bl[0].joint = (HSD_Joint*) (uintptr_t) 101;
    bcd.bytecode = g_bc;
    bcd.rvalue = bl;
    ikd.bone_length = 1.25f;
    ikd.rotate_x = 0.5f;

    d[0].flags = REFTYPE_JOBJ | 1;
    d[0].u.joint = (HSD_Joint*) (uintptr_t) 101;
    d[1].flags = REFTYPE_LIMIT | 1;
    d[1].u.limit = 90.0f;
    d[2].flags = REFTYPE_LIMIT | 7;
    d[2].u.limit = 3.5f;
    d[3].flags = REFTYPE_EXP | 5;
    d[3].u.exp = &expd;
    d[4].flags = REFTYPE_BYTECODE | 5;
    d[4].u.bcexp = &bcd;
    d[5].flags = REFTYPE_IKHINT | 2;
    d[5].u.ik_hint = &ikd;
    for (n = 0; n < 5; n++) {
        d[n].next = &d[n + 1];
    }

    CHECK(HSD_RObjLoadDesc(NULL) == NULL);
    list = HSD_RObjLoadDesc(&d[0]);
    CHECK(list != NULL);
    CHECK(using_count(rp) == 6);
    CHECK(using_count(vp) == 4);
    n = 0;
    for (e = list; e != NULL; e = e->next) {
        n++;
    }
    CHECK(n == 6);
    CHECK(list->flags == (REFTYPE_JOBJ | 1) && list->u.jobj == NULL);
    e = list->next;
    CHECK(e->flags == (REFTYPE_LIMIT | 1));
    CHECK_NEAR(e->u.limit, 90.0 * 0.017453292, 1e-6);
    e = e->next;
    CHECK(e->flags == (REFTYPE_LIMIT | 7) && e->u.limit == 3.5f);
    e = e->next;
    CHECK(e->flags == (REFTYPE_EXP | 5));
    CHECK(e->u.exp.expr.func == wsum5);
    CHECK(e->u.exp.nb_args == (u32) -1 && e->u.exp.is_bytecode == 0);
    CHECK(e->u.exp.rvalue != NULL && e->u.exp.rvalue->flags == 0x11);
    CHECK(e->u.exp.rvalue->next->flags == 0x100000);
    CHECK(e->u.exp.rvalue->next->next->flags == 0x1800000);
    CHECK(e->u.exp.rvalue->next->next->next == NULL);
    CHECK(e->u.exp.rvalue->jobj == NULL);
    b = e->next;
    CHECK(b->flags == 5);
    CHECK(b->u.exp.is_bytecode == 1 && b->u.exp.expr.bytecode == g_bc);
    CHECK(b->u.exp.nb_args == (u32) -1);
    CHECK(b->u.exp.rvalue != NULL && b->u.exp.rvalue->flags == 0x11);
    CHECK(b->next->flags == (REFTYPE_IKHINT | 2));
    CHECK(b->next->u.ik_hint.bone_length == 1.25f &&
          b->next->u.ik_hint.rotate_x == 0.5f);
    CHECK(b->next->next == NULL);

    CHECK(HSD_RObjGetByType(list, REFTYPE_IKHINT, 0) == NULL);
    CHECK(HSD_RObjGetByType(list, REFTYPE_IKHINT, 2) == NULL);
    HSD_RObjSetFlags(b->next, 0x80000000u);
    CHECK(HSD_RObjGetByType(list, REFTYPE_IKHINT, 0) == b->next);
    CHECK(HSD_RObjGetByType(list, REFTYPE_IKHINT, 2) == b->next);
    CHECK(HSD_RObjGetByType(list, REFTYPE_IKHINT, 3) == NULL);
    CHECK(HSD_RObjGetByType(list, REFTYPE_LIMIT, 0) == NULL);
    CHECK(HSD_RObjGetByType(NULL, REFTYPE_IKHINT, 0) == NULL);
    HSD_RObjSetFlags(NULL, 1);

    CHECK(iref_CNT(j1) == 0 && iref_CNT(j2) == 0 && iref_CNT(j3) == 0);
    HSD_RObjResolveRefsAll(list, d);
    CHECK(e->u.exp.rvalue->jobj == j1);
    CHECK(e->u.exp.rvalue->next->jobj == j2);
    CHECK(e->u.exp.rvalue->next->next->jobj == j3);
    CHECK(b->u.exp.rvalue->jobj == j1);
    CHECK(list->u.jobj == j1);
    CHECK(iref_CNT(j1) == 3 && iref_CNT(j2) == 1 && iref_CNT(j3) == 1);

    HSD_RObjSetFlags(e, 0x80000000u);
    HSD_RObjSetFlags(b, 0x80000000u);

    {
        HSD_RObj* only = list->next->next->next;
        HSD_RObj* saved_next = only->next;
        only->next = NULL;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1);
        CHECK(log.type[0] == 5 && log.has[0] == 1);
        CHECK_NEAR(log.v[0][0],
                   0.5 * 57.29578 + 2.0 * 7.0 + 3.0 * 9.0 + 4.0 * 2.0 +
                       5.0 * 3.0,
                   2e-3);
        CHECK(only->u.exp.nb_args == 5);

        only->flags = 0x80000000u | REFTYPE_EXP | 1;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1 && log.type[0] == 1);
        CHECK_NEAR(log.v[0][0],
                   (0.5 * 57.29578 + 14.0 + 27.0 + 8.0 + 15.0) * 0.017453292,
                   1e-4);

        only->flags = 0x80000000u | REFTYPE_EXP | 3;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1 && log.type[0] == 3);
        only->flags = 0x80000000u | REFTYPE_EXP | 4;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1 && log.type[0] == 4);
        CHECK_NEAR(log.v[0][0], 0.5 * 57.29578 + 14.0 + 27.0 + 8.0 + 15.0,
                   2e-3);

        only->flags = REFTYPE_EXP | 5;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 0);

        only->flags = 0x80000000u | REFTYPE_EXP | 5;
        only->u.exp.expr.func = exp_five_hundred;
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1 && log.v[0][0] == 500.0f);
        only->u.exp.expr.func = wsum5;

        _HSD_RObjForgetMemory((void*) (uintptr_t) 0x1000, (void*) ~(uintptr_t) 0);
        log.n = 0;
        HSD_RObjUpdateAll(only, obj, upd_cb);
        CHECK(log.n == 1 && log.type[0] == 5);
        CHECK_NEAR(log.v[0][0],
                   0.5 * 57.29578 + 14.0 + 27.0 + 8.0 + 15.0, 2e-3);
        only->next = saved_next;
    }

    b->flags = 0x80000000u | 5;
    {
        HSD_RObj* ik = b->next;
        b->next = NULL;
        log.n = 0;
        HSD_RObjUpdateAll(b, obj, upd_cb);
        b->next = ik;
    }
    CHECK(log.n == 1 && log.type[0] == 5);
    CHECK_NEAR(log.v[0][0], 0.5 * 57.29578 * 2.5 + 7.0, 2e-3);
    CHECK(b->u.exp.nb_args == 2);

    rv = HSD_RvalueAlloc();
    CHECK(rv != NULL && rv->next == NULL && rv->jobj == NULL &&
          rv->flags == 0);
    HSD_RvalueRemove(rv);
    HSD_RvalueRemove(NULL);
    HSD_RvalueRemoveAll(NULL);
    HSD_RvalueResolveRefs(NULL, NULL);
    HSD_RvalueResolveRefsAll(NULL, NULL);
    HSD_RvalueResolveRefsAll(b->u.exp.rvalue, NULL);
    HSD_RObjResolveRefs(NULL, NULL);
    HSD_RObjResolveRefs(b, NULL);

    HSD_RObjRemoveAll(list);
    CHECK(using_count(rp) == 0);
    CHECK(using_count(vp) == 0);
    CHECK(iref_CNT(j1) == 0 && iref_CNT(j2) == 0 && iref_CNT(j3) == 0);

    hsdDelete(j1);
    hsdDelete(j2);
    hsdDelete(j3);
    HSD_IDRemoveByIDFromTable(NULL, 101);
    HSD_IDRemoveByIDFromTable(NULL, 102);
    HSD_IDRemoveByIDFromTable(NULL, 103);
    free(obj);
}

static void test_robj_refs(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdJObj);
    HSD_RObjDesc d;
    HSD_RObj* r;
    HSD_JObj* a = new_jobj();
    HSD_JObj* b = new_jobj();
    HSD_ObjAllocData* rp = HSD_RObjGetAllocData();
    u32 nb0 = info->head.nb_exist;

    r = HSD_RObjAlloc();
    CHECK(r != NULL && using_count(rp) == 1);
    CHECK(r->next == NULL && r->flags == 0 && r->aobj == NULL &&
          r->u.jobj == NULL);

    HSD_RObjSetConstraintObj(r, a);
    CHECK(r->u.jobj == a && iref_CNT(a) == 1);
    HSD_RObjSetConstraintObj(r, b);
    CHECK(r->u.jobj == b && iref_CNT(a) == 0 && iref_CNT(b) == 1);
    HSD_RObjSetConstraintObj(NULL, a);

    HSD_IDInsertToTable(NULL, 900, a);
    memset(&d, 0, sizeof(d));
    d.flags = REFTYPE_JOBJ;
    d.u.joint = (HSD_Joint*) (uintptr_t) 900;
    r->flags = REFTYPE_JOBJ;
    HSD_RObjResolveRefs(r, &d);
    CHECK(r->u.jobj == a && iref_CNT(a) == 1 && iref_CNT(b) == 0);
    HSD_IDRemoveByIDFromTable(NULL, 900);

    HSD_OBJ(a)->ref_count = HSD_OBJ_NOREF;
    HSD_RObjRemove(r);
    CHECK(using_count(rp) == 0);
    CHECK(info->head.nb_exist == nb0 - 1);
    HSD_RObjRemove(NULL);
    HSD_RObjRemoveAll(NULL);
    hsdDelete(b);
    CHECK(info->head.nb_exist == nb0 - 2);
}

static void test_robj_anim(void)
{
    HSD_ObjAllocData* rp = HSD_RObjGetAllocData();
    HSD_FObjDesc fd;
    HSD_AObjDesc ad;
    HSD_RObjAnimJoint aj2, aj1;
    HSD_RObj *r1, *r2, *r3;
    HSD_AObj* old;
    int k;

    fill_fdesc(&fd, g_con, sizeof(g_con), TYPE_ROBJ, (u8) (HSD_A_FRAC_S16 | 8),
               HSD_A_FRAC_FLOAT, NULL);
    fill_adesc(&ad, &fd, 0, 8.0f, 0);
    memset(&aj1, 0, sizeof(aj1));
    memset(&aj2, 0, sizeof(aj2));
    aj1.aobjdesc = &ad;
    aj1.next = &aj2;
    aj2.aobjdesc = &ad;

    r1 = HSD_RObjAlloc();
    r2 = HSD_RObjAlloc();
    r3 = HSD_RObjAlloc();
    r1->next = r2;
    r2->next = r3;
    r1->flags = REFTYPE_LIMIT;
    r2->flags = REFTYPE_LIMIT | 0x80000000u;

    HSD_RObjAddAnim(NULL, &aj1);
    HSD_RObjAddAnim(r1, NULL);
    HSD_RObjAddAnimAll(NULL, &aj1);
    HSD_RObjAddAnimAll(r1, NULL);
    CHECK(r1->aobj == NULL);

    HSD_RObjAddAnimAll(r1, &aj1);
    CHECK(r1->aobj != NULL && r2->aobj != NULL && r3->aobj == NULL);
    CHECK(r1->aobj->end_frame == 8.0f);
    CHECK(using_count(HSD_AObjGetAllocData()) == 2);
    old = r1->aobj;
    HSD_RObjAddAnim(r1, &aj1);
    CHECK(r1->aobj != NULL && using_count(HSD_AObjGetAllocData()) == 2);
    (void) old;

    HSD_RObjReqAnimByFlags(r1, 0.0f, 0x7F);
    CHECK(r1->aobj->flags == AOBJ_NO_ANIM);
    HSD_RObjReqAnimByFlags(r1, 0.0f, 0x80);
    CHECK(r1->aobj->flags == AOBJ_FIRST_PLAY);
    HSD_RObjReqAnimByFlags(r3, 0.0f, 0x80);
    HSD_RObjReqAnimByFlags(NULL, 0.0f, 0x80);
    HSD_RObjReqAnimAll(r1, 0.0f);
    CHECK(r1->aobj->flags == AOBJ_FIRST_PLAY &&
          r2->aobj->flags == AOBJ_FIRST_PLAY);
    HSD_RObjReqAnimAllByFlags(NULL, 0.0f, 0x80);

    HSD_RObjAnim(NULL);
    HSD_RObjAnimAll(NULL);

    for (k = 0; k < 5; k++) {
        HSD_RObjAnimAll(r1);
        CHECK((r1->flags & 0x80000000u) != 0);
        CHECK((r2->flags & 0x80000000u) != 0);
    }
    HSD_RObjAnimAll(r1);
    CHECK((r1->flags & 0x80000000u) == 0);
    CHECK((r2->flags & 0x80000000u) == 0);
    CHECK((r1->flags & 0x0FFFFFFFu) == 0 &&
          (r1->flags & ROBJ_TYPE_MASK) == REFTYPE_LIMIT);
    CHECK((r1->aobj->flags & AOBJ_NO_ANIM) == 0);
    HSD_RObjAnimAll(r1);
    HSD_RObjAnimAll(r1);
    CHECK((r1->aobj->flags & AOBJ_NO_ANIM) == 0);
    HSD_RObjAnimAll(r1);
    CHECK((r1->flags & 0x80000000u) == 0);
    CHECK((r1->aobj->flags & AOBJ_NO_ANIM) != 0);
    HSD_RObjAnimAll(r1);
    CHECK((r1->flags & 0x80000000u) == 0);

    HSD_RObjReqAnimAll(r1, 0.0f);
    HSD_RObjAnim(r1);
    CHECK((r1->flags & 0x80000000u) != 0);
    CHECK((r2->flags & 0x80000000u) == 0);
    HSD_RObjAnimAll(r1);
    CHECK((r2->flags & 0x80000000u) != 0);

    HSD_RObjRemoveAnimByFlags(r1, 0x7F);
    CHECK(r1->aobj != NULL);
    HSD_RObjRemoveAnimByFlags(r1, 0x80);
    CHECK(r1->aobj == NULL && r2->aobj != NULL);
    HSD_RObjRemoveAnimByFlags(NULL, 0x80);
    HSD_RObjRemoveAnimAllByFlags(NULL, 0x80);
    HSD_RObjRemoveAnimAll(r1);
    CHECK(r2->aobj == NULL);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);
    CHECK(using_count(HSD_FObjGetAllocData()) == 0);

    HSD_RObjRemoveAll(r1);
    CHECK(using_count(rp) == 0);
}

static int g_setup_log_n;
static char g_setup_log[16];
static u32 g_last_rm;
static void* g_last_vmtx;
static void* g_last_pmtx;

static void fake_mobj_setup(HSD_MObj* m, u32 rm)
{
    (void) m;
    g_setup_log[g_setup_log_n++] = 'S';
    g_last_rm = rm;
}

static void fake_mobj_unset(HSD_MObj* m, u32 rm)
{
    (void) m;
    g_setup_log[g_setup_log_n++] = 'U';
    g_last_rm = rm;
}

static void fake_pobj_disp(HSD_PObj* p, Mtx v, Mtx pm, u32 rm)
{
    (void) p;
    g_setup_log[g_setup_log_n++] = 'P';
    g_last_vmtx = v;
    g_last_pmtx = pm;
    g_last_rm = rm;
}

static int g_test_load_calls;
static void TestDObjInfoInit(void);
static HSD_DObjInfo hsdTestDObj = { TestDObjInfoInit };

static int TestDObjLoad(HSD_DObj* d, HSD_DObjDesc* desc)
{
    g_test_load_calls++;
    return HSD_DOBJ_INFO(&hsdDObj)->load(d, desc);
}

static void TestDObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&hsdTestDObj), HSD_CLASS_INFO(&hsdDObj),
                     "m360_test_library", "m360_test_dobj",
                     sizeof(HSD_DObjInfo), sizeof(HSD_DObj));
    HSD_DOBJ_INFO(&hsdTestDObj)->load = TestDObjLoad;
}

static void test_dobj_class_and_lists(void)
{
    HSD_ClassInfo* ci = HSD_CLASS_INFO(&hsdDObj);
    HSD_DObj* d1;
    HSD_DObj* d2;
    HSD_DObj* d3;
    HSD_DObj* p;
    int n;

    d1 = HSD_DObjAlloc();
    CHECK(d1 != NULL);
    CHECK(HSD_CLASS_METHOD(d1) == ci);
    CHECK(strcmp(ci->head.class_name, "hsd_dobj") == 0);
    CHECK(strcmp(ci->head.library_name, "sysdolphin_base_library") == 0);
    CHECK(ci->head.obj_size == (s16) sizeof(HSD_DObj));
    CHECK(ci->head.info_size == (s16) sizeof(HSD_DObjInfo));
    CHECK(ci->head.parent == &hsdClass);
    CHECK(hsdIsDescendantOf(&hsdDObj, &hsdClass));
    CHECK(!hsdIsDescendantOf(&hsdDObj, &hsdObj));
    CHECK(ci->head.nb_exist == 1);
    CHECK(HSD_DOBJ_INFO(&hsdDObj)->disp == HSD_DObjDisp);
    CHECK(HSD_DOBJ_INFO(&hsdDObj)->load != NULL);
    CHECK(d1->next == NULL && d1->mobj == NULL && d1->pobj == NULL &&
          d1->aobj == NULL && d1->flags == 0);

    CHECK(HSD_DObjGetFlags(NULL) == 0);
    HSD_DObjSetFlags(d1, 0x12);
    HSD_DObjSetFlags(d1, DOBJ_HIDDEN);
    CHECK(HSD_DObjGetFlags(d1) == 0x13);
    HSD_DObjClearFlags(d1, 0x2);
    CHECK(HSD_DObjGetFlags(d1) == 0x11);
    HSD_DObjModifyFlags(d1, 0xF0, 0x30);
    CHECK(HSD_DObjGetFlags(d1) == 0x31);
    HSD_DObjModifyFlags(d1, 0, 0xFF);
    CHECK(HSD_DObjGetFlags(d1) == 0);
    HSD_DObjSetFlags(NULL, 1);
    HSD_DObjClearFlags(NULL, 1);
    HSD_DObjModifyFlags(NULL, 1, 1);
    HSD_DObjSetCurrent(d1);
    HSD_DObjSetCurrent(NULL);

    g_m360AnimStub.mobjRemove = 0;
    g_m360AnimStub.pobjRemoveAll = 0;
    HSD_DObjRemove(d1);
    CHECK(ci->head.nb_exist == 0);
    CHECK(g_m360AnimStub.mobjRemove == 1 && g_m360AnimStub.pobjRemoveAll == 1);
    HSD_DObjRemove(NULL);

    d1 = HSD_DObjAlloc();
    d2 = HSD_DObjAlloc();
    d3 = HSD_DObjAlloc();
    d1->next = d2;
    d2->next = d3;
    d2->aobj = HSD_AObjAlloc();
    d3->aobj = HSD_AObjAlloc();
    CHECK(ci->head.nb_exist == 3 && ci->head.nb_peak >= 3);
    g_m360AnimStub.mobjRemove = 0;
    g_m360AnimStub.pobjRemoveAll = 0;
    HSD_DObjRemoveAll(d1);
    CHECK(ci->head.nb_exist == 0);
    CHECK(g_m360AnimStub.mobjRemove == 3 && g_m360AnimStub.pobjRemoveAll == 3);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);
    HSD_DObjRemoveAll(NULL);

    d1 = HSD_DObjAlloc();
    d2 = HSD_DObjAlloc();
    d1->next = d2;
    d1->aobj = HSD_AObjAlloc();
    d2->aobj = HSD_AObjAlloc();
    HSD_DObjRemoveAnimByFlags(NULL, 2);
    HSD_DObjRemoveAnimAllByFlags(NULL, 2);
    g_m360AnimStub.pobjRemoveAnimAllByFlags = 0;
    g_m360AnimStub.mobjRemoveAnimByFlags = 0;
    HSD_DObjRemoveAnimByFlags(d1, 0x4);
    CHECK(d1->aobj != NULL);
    CHECK(g_m360AnimStub.pobjRemoveAnimAllByFlags == 1 &&
          g_m360AnimStub.mobjRemoveAnimByFlags == 1 &&
          g_m360AnimStub.lastFlags == 0x4);
    HSD_DObjRemoveAnimAllByFlags(d1, 0x2);
    CHECK(d1->aobj == NULL && d2->aobj == NULL);
    CHECK(g_m360AnimStub.pobjRemoveAnimAllByFlags == 3 &&
          g_m360AnimStub.mobjRemoveAnimByFlags == 3);
    CHECK(using_count(HSD_AObjGetAllocData()) == 0);

    g_m360AnimStub.pobjReqAnimAllByFlags = 0;
    g_m360AnimStub.mobjReqAnimByFlags = 0;
    HSD_DObjReqAnimAll(d1, 5.0f);
    CHECK(g_m360AnimStub.pobjReqAnimAllByFlags == 2 &&
          g_m360AnimStub.mobjReqAnimByFlags == 2);
    CHECK(g_m360AnimStub.lastFrame == 5.0f && g_m360AnimStub.lastFlags == 0x7FF);
    HSD_DObjReqAnimAllByFlags(d1, 2.5f, 0x14);
    CHECK(g_m360AnimStub.pobjReqAnimAllByFlags == 4 &&
          g_m360AnimStub.mobjReqAnimByFlags == 4);
    CHECK(g_m360AnimStub.lastFrame == 2.5f && g_m360AnimStub.lastFlags == 0x14);
    HSD_DObjReqAnimByFlags(d2, 1.0f, 0x1);
    CHECK(g_m360AnimStub.pobjReqAnimAllByFlags == 5);
    HSD_DObjReqAnimAll(NULL, 1.0f);
    HSD_DObjReqAnimAllByFlags(NULL, 1.0f, 1);
    HSD_DObjReqAnimByFlags(NULL, 1.0f, 1);
    CHECK(g_m360AnimStub.pobjReqAnimAllByFlags == 5);

    g_m360AnimStub.pobjAnimAll = 0;
    g_m360AnimStub.mobjAnim = 0;
    HSD_DObjAnimAll(d1);
    CHECK(g_m360AnimStub.pobjAnimAll == 2 && g_m360AnimStub.mobjAnim == 2);
    HSD_DObjAnim(d2);
    CHECK(g_m360AnimStub.pobjAnimAll == 3 && g_m360AnimStub.mobjAnim == 3);
    HSD_DObjAnim(NULL);
    HSD_DObjAnimAll(NULL);

    {
        HSD_ShapeAnimDObj sh[2];
        HSD_MatAnim ma[2];
        HSD_ShapeAnim sa[2];
        memset(sh, 0, sizeof(sh));
        memset(ma, 0, sizeof(ma));
        memset(sa, 0, sizeof(sa));
        sh[0].shapeanim = &sa[0];
        sh[0].next = &sh[1];
        sh[1].shapeanim = &sa[1];
        ma[0].next = &ma[1];

        g_m360AnimStub.pobjAddAnimAll = 0;
        g_m360AnimStub.mobjAddAnim = 0;
        HSD_DObjAddAnim(d1, &ma[0], &sh[0]);
        CHECK(g_m360AnimStub.pobjAddAnimAll == 1 &&
              g_m360AnimStub.mobjAddAnim == 1);
        CHECK(g_m360AnimStub.lastPtr == d1->mobj);
        CHECK(g_m360AnimStub.lastPtr2 == &ma[0]);
        HSD_DObjAddAnim(d1, NULL, NULL);
        CHECK(g_m360AnimStub.pobjAddAnimAll == 2 &&
              g_m360AnimStub.lastPtr2 == NULL);
        HSD_DObjAddAnim(NULL, &ma[0], &sh[0]);
        CHECK(g_m360AnimStub.pobjAddAnimAll == 2);
        HSD_DObjAddAnimAll(d1, &ma[0], &sh[0]);
        CHECK(g_m360AnimStub.pobjAddAnimAll == 4 &&
              g_m360AnimStub.mobjAddAnim == 4);
        CHECK(g_m360AnimStub.lastPtr2 == &ma[1]);
        HSD_DObjAddAnimAll(NULL, &ma[0], &sh[0]);
        CHECK(g_m360AnimStub.pobjAddAnimAll == 4);
    }

    {
        HSD_DObjDesc dd[3];
        memset(dd, 0, sizeof(dd));
        dd[0].next = &dd[1];
        dd[0].pobjdesc = (HSD_PObjDesc*) &dd[2];
        dd[1].pobjdesc = (HSD_PObjDesc*) &dd[0];
        g_m360AnimStub.pobjResolveRefsAll = 0;
        HSD_DObjResolveRefsAll(d1, dd);
        CHECK(g_m360AnimStub.pobjResolveRefsAll == 2);
        CHECK(g_m360AnimStub.lastPtr2 == dd[1].pobjdesc);
        g_m360AnimStub.pobjResolveRefsAll = 0;
        HSD_DObjResolveRefs(d1, &dd[0]);
        CHECK(g_m360AnimStub.pobjResolveRefsAll == 1 &&
              g_m360AnimStub.lastPtr2 == dd[0].pobjdesc);
        HSD_DObjResolveRefs(NULL, &dd[0]);
        HSD_DObjResolveRefs(d1, NULL);
        HSD_DObjResolveRefsAll(NULL, dd);
        HSD_DObjResolveRefsAll(d1, NULL);
        CHECK(g_m360AnimStub.pobjResolveRefsAll == 1);
    }

    HSD_DObjRemoveAll(d1);
    CHECK(ci->head.nb_exist == 0);

    p = HSD_DOBJ(hsdNew(HSD_CLASS_INFO(&hsdTestDObj)));
    n = 0;
    CHECK(p != NULL && HSD_CLASS_METHOD(p) == HSD_CLASS_INFO(&hsdTestDObj));
    CHECK(hsdIsDescendantOf(&hsdTestDObj, &hsdDObj));
    CHECK(hsdIsDescendantOf(&hsdTestDObj, &hsdClass));
    CHECK(HSD_DOBJ_INFO(&hsdTestDObj)->disp == HSD_DObjDisp);
    CHECK(HSD_CLASS_INFO(&hsdTestDObj)->release ==
          HSD_CLASS_INFO(&hsdDObj)->release);
    CHECK(ci->head.nb_exist == 0);
    CHECK(HSD_CLASS_INFO(&hsdTestDObj)->head.nb_exist == 1);
    HSD_DObjRemove(p);
    CHECK(HSD_CLASS_INFO(&hsdTestDObj)->head.nb_exist == 0);
    (void) n;
}

static void test_dobj_load_and_default_class(void)
{
    HSD_ClassInfo* ci = HSD_CLASS_INFO(&hsdDObj);
    HSD_ClassInfo* ti = HSD_CLASS_INFO(&hsdTestDObj);
    HSD_DObjDesc dd[3];
    HSD_MObjDesc mdesc;
    HSD_DObj* d;
    HSD_MObj* mo = (HSD_MObj*) zalloc(sizeof(HSD_MObj));
    static char name[] = "hsd_dobj";

    memset(dd, 0, sizeof(dd));
    dd[0].next = &dd[1];
    dd[1].next = &dd[2];

    CHECK(HSD_DObjLoadDesc(NULL) == NULL);

    g_m360AnimStub.mobjLoadDesc = 0;
    g_m360AnimStub.pobjLoadDesc = 0;
    d = HSD_DObjLoadDesc(&dd[0]);
    CHECK(d != NULL && d->next != NULL && d->next->next != NULL &&
          d->next->next->next == NULL);
    CHECK(HSD_CLASS_METHOD(d) == ci && HSD_CLASS_METHOD(d->next) == ci);
    CHECK(ci->head.nb_exist == 3);
    CHECK(g_m360AnimStub.mobjLoadDesc == 3 && g_m360AnimStub.pobjLoadDesc == 3);
    CHECK(d->mobj == NULL && d->pobj == NULL && d->flags == 0);
    HSD_DObjRemoveAll(d);
    CHECK(ci->head.nb_exist == 0);

    dd[0].class_name = name;
    d = HSD_DObjLoadDesc(&dd[2]);
    CHECK(d != NULL && HSD_CLASS_METHOD(d) == ci);
    HSD_DObjRemove(d);
    d = HSD_DObjLoadDesc(&dd[0]);
    CHECK(hsdSearchClassInfo("hsd_dobj") == NULL);
    CHECK(d != NULL && HSD_CLASS_METHOD(d) == ci);
    HSD_DObjRemoveAll(d);
    dd[0].class_name = NULL;

    HSD_DObjSetDefaultClass(ti);
    g_test_load_calls = 0;
    d = HSD_DObjLoadDesc(&dd[0]);
    CHECK(g_test_load_calls == 3);
    CHECK(HSD_CLASS_METHOD(d) == ti && HSD_CLASS_METHOD(d->next) == ti);
    CHECK(ti->head.nb_exist == 3);
    HSD_DObjRemoveAll(d);
    CHECK(ti->head.nb_exist == 0);

    d = HSD_DObjAlloc();
    CHECK(HSD_CLASS_METHOD(d) == ti);
    HSD_DObjRemove(d);
    ti->amnesia(ti);
    d = HSD_DObjAlloc();
    CHECK(HSD_CLASS_METHOD(d) == ci);
    HSD_DObjRemove(d);

    HSD_DObjSetDefaultClass(ti);
    HSD_DObjSetDefaultClass(NULL);
    d = HSD_DObjAlloc();
    CHECK(HSD_CLASS_METHOD(d) == ci);
    HSD_DObjRemove(d);

    memset(&mdesc, 0, sizeof(mdesc));
    mdesc.rendermode = 0;
    dd[1].mobjdesc = &mdesc;
    g_m360AnimStub.mobjRemove = 0;
    dd[1].next = NULL;
    d = HSD_DObjLoadDesc(&dd[1]);
    CHECK(d->mobj != NULL && d->mobj->rendermode == RENDER_TOON);
    CHECK((d->flags & 0xE) == 2);
    HSD_DObjRemove(d);
    CHECK(g_m360AnimStub.mobjRemove == 1);

    mdesc.rendermode = 0x40000000;
    d = HSD_DObjLoadDesc(&dd[1]);
    CHECK((d->flags & 0xE) == 8);
    HSD_DObjRemove(d);

    mdesc.rendermode = 0x60000000 | 0x0000FFFF;
    d = HSD_DObjLoadDesc(&dd[1]);
    CHECK((d->flags & 0xE) == 4);
    HSD_DObjRemove(d);

    mdesc.rendermode = 0x02000000;
    d = HSD_DObjLoadDesc(&dd[1]);
    CHECK((d->flags & 0xE) == 2);
    HSD_DObjSetFlags(d, 0xF01);
    HSD_DObjModifyFlags(d, 8, 0xE);
    CHECK(d->flags == 0xF09);
    HSD_DObjRemove(d);
    dd[1].mobjdesc = NULL;
    free(mo);
    CHECK(ci->head.nb_exist == 0);
    CHECK(ti->head.nb_exist == 0);
}

static void test_dobj_disp(void)
{
    HSD_DObj* d = HSD_DObjAlloc();
    HSD_MObj* mo = (HSD_MObj*) zalloc(sizeof(HSD_MObj));
    HSD_PObj* p1 = (HSD_PObj*) zalloc(sizeof(HSD_PObj));
    HSD_PObj* p2 = (HSD_PObj*) zalloc(sizeof(HSD_PObj));
    HSD_MObjInfo mi;
    HSD_PObjInfo pi;
    Mtx vm, pm;

    memset(&mi, 0, sizeof(mi));
    memset(&pi, 0, sizeof(pi));
    mi.setup = fake_mobj_setup;
    mi.unset = fake_mobj_unset;
    pi.disp = fake_pobj_disp;
    mo->parent.class_info = HSD_CLASS_INFO(&mi);
    p1->parent.class_info = HSD_CLASS_INFO(&pi);
    p2->parent.class_info = HSD_CLASS_INFO(&pi);
    p1->next = p2;
    d->mobj = mo;
    d->pobj = p1;

    CHECK(HSD_DOBJ_INFO(&hsdDObj)->disp == HSD_DObjDisp);

    g_setup_log_n = 0;
    memset(g_setup_log, 0, sizeof(g_setup_log));
    g_m360AnimStub.mobjSetCurrent = 0;
    HSD_DObjDisp(d, vm, pm, 0x1234);
    CHECK(strcmp(g_setup_log, "SPPU") == 0);
    CHECK(g_last_rm == 0x1234);
    CHECK(g_last_vmtx == (void*) vm && g_last_pmtx == (void*) pm);
    CHECK(g_m360AnimStub.mobjSetCurrent == 2 && g_m360AnimStub.lastPtr == NULL);

    g_setup_log_n = 0;
    memset(g_setup_log, 0, sizeof(g_setup_log));
    HSD_DObjDisp(d, vm, pm, 0x4000000);
    CHECK(strcmp(g_setup_log, "PP") == 0);
    CHECK(g_last_rm == 0x4000000);

    g_setup_log_n = 0;
    memset(g_setup_log, 0, sizeof(g_setup_log));
    d->pobj = NULL;
    HSD_DObjDisp(d, vm, pm, 0);
    CHECK(strcmp(g_setup_log, "SU") == 0);

    d->mobj = NULL;
    d->pobj = NULL;
    HSD_DObjRemove(d);
    free(mo);
    free(p1);
    free(p2);
}

int main(void)
{
    M360_HSD_HeapInit();
    HSD_LogInit();
    HSD_ListInitAllocData();
    HSD_FObjInitAllocData();
    HSD_AObjInitAllocData();
    HSD_RObjInitAllocData();
    HSD_IDSetup();
    HSD_IDInitAllocData();
    HSD_VecInitAllocData();
    HSD_MtxInitAllocData();
    init_streams();

    printf("[M360][ANIM] object sizes host: AObj=%u DObj=%u RObj=%u "
           "(GameCube layout 28/24/28)\n",
           (unsigned) sizeof(HSD_AObj), (unsigned) sizeof(HSD_DObj),
           (unsigned) sizeof(HSD_RObj));

    test_util();
    test_bytecode();
    test_aobj_basics();
    test_aobj_stepping();
    test_aobj_loop();
    test_aobj_fobj_shapes();
    test_jobj_class_and_refs();
    test_aobj_loaddesc_objid();
    test_foreach_anim();
    test_robj_constraints();
    test_robj_orientation();
    test_robj_limits();
    test_robj_load_and_eval();
    test_robj_refs();
    test_robj_anim();
    test_dobj_class_and_lists();
    test_dobj_load_and_default_class();
    test_dobj_disp();
    CHECK(M360_HsdAnimSelfTest());

    if (fail_count) {
        fprintf(stderr, "[M360][ANIM] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][ANIM] util/bytecode/aobj/dobj/robj host validation "
           "passed\n");
    return 0;
}
