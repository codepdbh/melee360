#include "hsdjobj_xdk_compat.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include "aobj.h"
#include "class.h"
#include "cobj.h"
#include "displayfunc.h"
#include "dobj.h"
#include "fobj.h"
#include "id.h"
#include "jobj.h"
#include "list.h"
#include "mtx.h"
#include "object.h"
#include "robj.h"
#include "spline.h"
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
HSD_JObj* HSD_JObjGetPrev(HSD_JObj* jobj);

static int fail_count = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "[M360][JOBJ][FAIL] %s (%s:%d)\n", #cond,        \
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
                    "[M360][JOBJ][FAIL] %s=%.9g vs %s=%.9g (%s:%d)\n", #a,   \
                    a_, #b, b_, __FILE__, __LINE__);                         \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

#define EPS 1e-4
#define HALF_PI 1.57079632679489661923f
#define QUARTER_PI 0.78539816339744830962f

static void check_mtx(const char* what, Mtx m, const f32* e, int line)
{
    int i;
    for (i = 0; i < 12; i++) {
        double got = (double) m[i / 4][i % 4];
        if (!(fabs(got - (double) e[i]) <= EPS)) {
            fprintf(stderr,
                    "[M360][JOBJ][FAIL] %s[%d][%d]=%.9g expected %.9g "
                    "(%s:%d)\n",
                    what, i / 4, i % 4, got, (double) e[i], __FILE__, line);
            fail_count++;
        }
    }
}

#define CHECK_MTX(m, ...)                                                    \
    do {                                                                     \
        const f32 e_[12] = { __VA_ARGS__ };                                  \
        check_mtx(#m, m, e_, __LINE__);                                      \
    } while (0)

static Vec3 v3(f32 x, f32 y, f32 z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static void set_srt(HSD_JObj* j, Vec3 s, Vec3 r, Vec3 t)
{
    HSD_JObjSetScale(j, &s);
    HSD_JObjSetRotationX(j, r.x);
    HSD_JObjSetRotationY(j, r.y);
    HSD_JObjSetRotationZ(j, r.z);
    HSD_JObjSetTranslate(j, &t);
}

static void set_t(HSD_JObj* j, f32 x, f32 y, f32 z)
{
    Vec3 t = v3(x, y, z);
    HSD_JObjSetTranslate(j, &t);
}

static void clean_all(HSD_JObj* j)
{
    while (j != NULL) {
        j->flags &= ~JOBJ_MTX_DIRTY;
        if (!(j->flags & JOBJ_INSTANCE)) {
            clean_all(j->child);
        }
        j = j->next;
    }
}

static int dirty(HSD_JObj* j)
{
    return (j->flags & JOBJ_MTX_DIRTY) != 0;
}

static u32 nb_jobj(void)
{
    return (u32) HSD_CLASS_INFO(&hsdJObj)->head.nb_exist;
}

static u32 using_count(HSD_ObjAllocData* d)
{
    return HSD_ObjAllocGetUsing(d);
}

static u8 g_streams[64][10];
static int g_stream_n;

static void put_f32(u8* dst, f32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    dst[0] = (u8) (bits & 0xFF);
    dst[1] = (u8) ((bits >> 8) & 0xFF);
    dst[2] = (u8) ((bits >> 16) & 0xFF);
    dst[3] = (u8) ((bits >> 24) & 0xFF);
}

static u8* ramp_stream(f32 v0, f32 v1)
{
    u8* s = g_streams[g_stream_n++];
    s[0] = 0x12;
    put_f32(s + 1, v0);
    s[5] = 10;
    put_f32(s + 6, v1);
    return s;
}

typedef struct Ramp {
    HSD_FObjDesc f[10];
    int n;
    HSD_AObjDesc a;
    HSD_AnimJoint aj;
} Ramp;

static Ramp g_ramps[8];
static int g_ramp_n;

static Ramp* ramp_new(void)
{
    Ramp* r = &g_ramps[g_ramp_n++];
    memset(r, 0, sizeof(*r));
    return r;
}

static void ramp_add(Ramp* r, u8 type, f32 v0, f32 v1)
{
    HSD_FObjDesc* d = &r->f[r->n];
    memset(d, 0, sizeof(*d));
    d->length = 10;
    d->type = type;
    d->frac_value = HSD_A_FRAC_FLOAT;
    d->frac_slope = HSD_A_FRAC_FLOAT;
    d->ad = ramp_stream(v0, v1);
    if (r->n > 0) {
        r->f[r->n - 1].next = d;
    }
    r->n++;
}

static HSD_AnimJoint* ramp_joint(Ramp* r, u32 aflags, u32 jflags)
{
    r->a.flags = aflags;
    r->a.end_frame = 10.0f;
    r->a.fobjdesc = &r->f[0];
    r->a.obj_id = 0;
    r->aj.aobjdesc = &r->a;
    r->aj.flags = jflags;
    return &r->aj;
}

static void step(HSD_JObj* j, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        HSD_JObjAnim(j);
    }
}

static void test_dolphin_prims(void)
{
    Mtx m, inv, prod, r;
    Vec3 v, axis;
    u32 ok;
    int i, j;

    static const f32 rz90_t[12] = { 0.0f, -1.0f, 0.0f, 10.0f, 1.0f, 0.0f,
                                    0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f };
    for (i = 0; i < 12; i++) {
        m[i / 4][i % 4] = rz90_t[i];
    }

    v = v3(1.0f, 2.0f, 3.0f);
    PSMTXMultVec(m, &v, &axis);
    CHECK_NEAR(axis.x, 8.0, EPS);
    CHECK_NEAR(axis.y, 2.0, EPS);
    CHECK_NEAR(axis.z, 3.0, EPS);
    PSMTXMultVec(m, &v, &v);
    CHECK_NEAR(v.x, 8.0, EPS);
    CHECK_NEAR(v.y, 2.0, EPS);
    CHECK_NEAR(v.z, 3.0, EPS);

    v = v3(1.0f, 2.0f, 3.0f);
    axis = v3(4.0f, 5.0f, 6.0f);
    PSVECAdd(&v, &axis, &v);
    CHECK(v.x == 5.0f && v.y == 7.0f && v.z == 9.0f);

    ok = PSMTXInverse(m, inv);
    CHECK(ok == 1);
    CHECK_MTX(inv, 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 10.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
    PSMTXConcat(m, inv, prod);
    CHECK_MTX(prod, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
    PSMTXInverse(m, m);
    CHECK_MTX(m, 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 10.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    PSMTXScale(m, 2.0f, 4.0f, 5.0f);
    m[0][3] = 1.0f;
    m[1][3] = 1.0f;
    m[2][3] = 1.0f;
    ok = PSMTXInverse(m, inv);
    CHECK(ok == 1);
    CHECK_MTX(inv, 0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.25f, 0.0f, -0.25f, 0.0f,
              0.0f, 0.2f, -0.2f);

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            m[i][j] = 0.0f;
            inv[i][j] = 7.0f;
        }
    }
    CHECK(PSMTXInverse(m, inv) == 0);
    CHECK(inv[0][0] == 7.0f && inv[2][3] == 7.0f);

    axis = v3(0.0f, 0.0f, 2.0f);
    PSMTXRotAxisRad(r, &axis, HALF_PI);
    CHECK_MTX(r, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              1.0f, 0.0f);
    axis = v3(1.0f, 0.0f, 0.0f);
    PSMTXRotAxisRad(r, &axis, HALF_PI);
    CHECK_MTX(r, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
              0.0f, 0.0f);
    axis = v3(1.0f, 1.0f, 1.0f);
    PSMTXRotAxisRad(r, &axis, 2.0f * 3.14159265358979323846f / 3.0f);
    CHECK_MTX(r, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
              0.0f, 0.0f);
}

static HSD_JObjInfo hsdTestJObj;
static void TestJObjInfoInit(void);
static HSD_JObjInfo hsdTestJObj = { TestJObjInfoInit };

static void TestJObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&hsdTestJObj), HSD_CLASS_INFO(&hsdJObj),
                     (char*) "test_library", (char*) "test_jobj",
                     sizeof(HSD_JObjInfo), sizeof(HSD_JObj));
}

static void test_class_and_alloc(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdJObj);
    HSD_ClassInfo* tinfo = HSD_CLASS_INFO(&hsdTestJObj);
    HSD_JObj* j = HSD_JObjAlloc();
    int i;
    int k;

    CHECK(j != NULL);
    CHECK(HSD_CLASS_METHOD(j) == info);
    CHECK(strcmp(info->head.class_name, "hsd_jobj") == 0);
    CHECK(strcmp(info->head.library_name, "sysdolphin_base_library") == 0);
    CHECK(info->head.obj_size == (s16) sizeof(HSD_JObj));
    CHECK(info->head.info_size == (s16) sizeof(HSD_JObjInfo));
    CHECK(info->head.parent == &hsdObj);
    CHECK(hsdIsDescendantOf(info, &hsdClass));
    CHECK(hsdIsDescendantOf(info, &hsdObj));
    CHECK(!hsdIsDescendantOf(&hsdObj, info));
    CHECK(hsdObjIsDescendantOf(&j->object, info));
    CHECK(nb_jobj() == 1);

    CHECK(info->init == JObjInit);
    CHECK(info->release == JObjRelease);
    CHECK(info->amnesia == JObjAmnesia);
    CHECK(HSD_JOBJ_INFO(&hsdJObj)->make_mtx == HSD_JObjMakeMatrix);
    CHECK(HSD_JOBJ_INFO(&hsdJObj)->make_pmtx == HSD_JObjMakePositionMtx);
    CHECK(HSD_JOBJ_INFO(&hsdJObj)->disp == HSD_JObjDispSub);
    CHECK(HSD_JOBJ_INFO(&hsdJObj)->load == JObjLoad);
    CHECK(HSD_JOBJ_INFO(&hsdJObj)->release_child == JObjReleaseChild);

    CHECK(j->flags == JOBJ_MTX_DIRTY && HSD_JObjGetFlags(j) == 0x40);
    CHECK(j->scale.x == 1.0f && j->scale.y == 1.0f && j->scale.z == 1.0f);
    CHECK(j->rotate.x == 0.0f && j->rotate.y == 0.0f && j->rotate.z == 0.0f &&
          j->rotate.w == 0.0f);
    CHECK(j->translate.x == 0.0f && j->translate.y == 0.0f &&
          j->translate.z == 0.0f);
    CHECK(j->next == NULL && j->parent == NULL && j->child == NULL);
    CHECK(j->u.dobj == NULL && j->scl == NULL && j->envelopemtx == NULL);
    CHECK(j->aobj == NULL && j->robj == NULL && j->id == 0);
    CHECK(ref_CNT(j) == 0 && iref_CNT(j) == 0);
    for (i = 0; i < 3; i++) {
        for (k = 0; k < 4; k++) {
            CHECK(j->mtx[i][k] == 0.0f);
        }
    }
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == 0);
    CHECK(HSD_JObjGetFlags(NULL) == 0);

    HSD_JObjSetDefaultClass(tinfo);
    j = HSD_JObjAlloc();
    CHECK(HSD_CLASS_METHOD(j) == tinfo && tinfo->head.nb_exist == 1);
    CHECK(hsdIsDescendantOf(tinfo, &hsdJObj));
    CHECK(j->flags == JOBJ_MTX_DIRTY && j->scale.x == 1.0f);
    CHECK(nb_jobj() == 0);
    HSD_JObjUnref(j);
    CHECK(tinfo->head.nb_exist == 0);

    HSD_JObjSetDefaultClass(NULL);
    j = HSD_JObjAlloc();
    CHECK(HSD_CLASS_METHOD(j) == info);
    HSD_JObjUnref(j);

    HSD_JObjSetDefaultClass(tinfo);
    tinfo->amnesia(tinfo);
    j = HSD_JObjAlloc();
    CHECK(HSD_CLASS_METHOD(j) == info);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == 0);
}

static void test_ref_counting(void)
{
    HSD_JObj* j;
    HSD_JObj* c;
    HSD_JObj* a;
    HSD_JObj* b;
    u32 base = nb_jobj();

    HSD_JObjRef(NULL);
    HSD_JObjRefThis(NULL);
    HSD_JObjUnref(NULL);
    HSD_JObjUnrefThis(NULL);

    j = HSD_JObjAlloc();
    HSD_JObjRef(j);
    HSD_JObjRef(j);
    CHECK(ref_CNT(j) == 2 && iref_CNT(j) == 0);
    HSD_JObjUnref(j);
    CHECK(ref_CNT(j) == 1 && nb_jobj() == base + 1);
    HSD_JObjUnref(j);
    CHECK(ref_CNT(j) == 0 && nb_jobj() == base + 1);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == base);

    j = HSD_JObjAlloc();
    HSD_JObjRefThis(j);
    CHECK(iref_CNT(j) == 1);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == base + 1 && ref_CNT(j) == -1 && iref_CNT(j) == 1);
    CHECK(HSD_OBJ(j)->ref_count == HSD_OBJ_NOREF);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == base + 1 && ref_CNT(j) == -1 && iref_CNT(j) == 1);
    HSD_JObjUnrefThis(j);
    CHECK(nb_jobj() == base);

    j = HSD_JObjAlloc();
    HSD_JObjRefThis(j);
    HSD_JObjRefThis(j);
    HSD_JObjRef(j);
    HSD_JObjUnrefThis(j);
    CHECK(iref_CNT(j) == 1 && nb_jobj() == base + 1);
    HSD_JObjUnrefThis(j);
    CHECK(iref_CNT(j) == 0 && ref_CNT(j) == 1 && nb_jobj() == base + 1);
    HSD_JObjUnrefThis(j);
    CHECK(iref_CNT(j) == 0 && ref_CNT(j) == 1 && nb_jobj() == base + 1);
    HSD_JObjUnref(j);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == base);

    j = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    HSD_JObjAddChild(j, c);
    HSD_JObjRefThis(j);
    HSD_JObjUnref(j);
    CHECK(nb_jobj() == base + 1 && j->child == NULL);
    HSD_JObjUnrefThis(j);
    CHECK(nb_jobj() == base);

    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    CHECK(HSD_JObjGetCurrent() == NULL);
    HSD_JObjSetCurrent(a);
    CHECK(HSD_JObjGetCurrent() == a && ref_CNT(a) == 1);
    HSD_JObjSetCurrent(b);
    CHECK(HSD_JObjGetCurrent() == b && ref_CNT(a) == 0 && ref_CNT(b) == 1);
    HSD_JObjSetCurrent(b);
    CHECK(HSD_JObjGetCurrent() == b && ref_CNT(b) == 1);
    HSD_JObjSetCurrent(NULL);
    CHECK(HSD_JObjGetCurrent() == NULL && ref_CNT(b) == 0);
    HSD_JObjSetCurrent(a);
    HSD_CLASS_INFO(&hsdJObj)->amnesia(HSD_CLASS_INFO(&hsdJObj));
    CHECK(HSD_JObjGetCurrent() == NULL && ref_CNT(a) == 1);
    HSD_CLASS_INFO(&hsdJObj)->head.nb_exist = 2;
    HSD_OBJ(a)->ref_count = 0;
    HSD_JObjUnref(a);
    HSD_JObjUnref(b);
    CHECK(nb_jobj() == 0);
}

typedef struct WalkLog {
    int n;
    HSD_JObj* j[16];
    s32 type[16];
    f32** args;
} WalkLog;

static WalkLog g_walk;

static void walk_cb(HSD_JObj* j, f32** args, s32 type)
{
    if (g_walk.n < 16) {
        g_walk.j[g_walk.n] = j;
        g_walk.type[g_walk.n] = type;
        g_walk.n++;
    }
    g_walk.args = args;
}

static void test_tree(void)
{
    u32 base = nb_jobj();
    HSD_JObj *p, *a, *b, *c, *q, *x, *n, *k1, *k2, *g1, *g2, *g3;
    HSD_JObj* ret;
    f32 marker = 1.0f;
    f32* marker_ptr = &marker;

    p = HSD_JObjAlloc();
    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    q = HSD_JObjAlloc();
    HSD_JObjAddChild(NULL, a);
    HSD_JObjAddChild(p, NULL);
    CHECK(p->child == NULL && a->parent == NULL);
    HSD_JObjAddChild(p, a);
    HSD_JObjAddChild(p, b);
    HSD_JObjAddChild(p, c);
    CHECK(p->child == a && a->next == b && b->next == c && c->next == NULL);
    CHECK(a->parent == p && b->parent == p && c->parent == p);
    CHECK(HSD_JObjGetChild(p) == a && HSD_JObjGetNext(a) == b &&
          HSD_JObjGetParent(c) == p);
    CHECK(HSD_JObjGetChild(NULL) == NULL && HSD_JObjGetNext(NULL) == NULL &&
          HSD_JObjGetParent(NULL) == NULL);
    CHECK(HSD_JObjGetPrev(b) == a && HSD_JObjGetPrev(c) == b);
    CHECK(HSD_JObjGetPrev(a) == NULL && HSD_JObjGetPrev(p) == NULL &&
          HSD_JObjGetPrev(NULL) == NULL);

    CHECK(HSD_JObjReparent(NULL, q) == NULL);
    ret = HSD_JObjReparent(b, q);
    CHECK(ret == c && p->child == a && a->next == c && c->next == NULL);
    CHECK(b->parent == q && q->child == b && b->next == NULL);
    ret = HSD_JObjReparent(a, q);
    CHECK(ret == c && p->child == c && q->child == b && b->next == a &&
          a->parent == q && a->next == NULL);
    ret = HSD_JObjReparent(a, NULL);
    CHECK(ret == NULL && a->parent == NULL && a->next == NULL &&
          q->child == b && b->next == NULL);
    HSD_JObjAddChild(q, a);
    HSD_JObjRemoveAll(q);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    p = HSD_JObjAlloc();
    k1 = HSD_JObjAlloc();
    k2 = HSD_JObjAlloc();
    n = HSD_JObjAlloc();
    HSD_JObjAddChild(p, k1);
    HSD_JObjAddChild(p, k2);
    HSD_JObjAddNext(NULL, n);
    HSD_JObjAddNext(k1, NULL);
    CHECK(n->parent == NULL && n->child == NULL);
    HSD_JObjAddNext(k1, n);
    CHECK(p->child == n && n->parent == p && n->child == k1);
    CHECK(k1->next == k2 && k1->parent == n && k2->parent == n);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    x = HSD_JObjAlloc();
    n = HSD_JObjAlloc();
    g1 = HSD_JObjAlloc();
    HSD_JObjAddChild(n, g1);
    HSD_JObjAddNext(x, n);
    CHECK(n->parent == NULL && n->child == g1 && g1->next == x);
    CHECK(x->parent == n && g1->parent == n && x->next == NULL);
    HSD_JObjRemoveAll(n);
    CHECK(nb_jobj() == base);

    p = HSD_JObjAlloc();
    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g1 = HSD_JObjAlloc();
    HSD_JObjAddChild(p, a);
    HSD_JObjAddChild(p, b);
    HSD_JObjAddChild(a, g1);
    ret = HSD_JObjRemove(a);
    CHECK(ret == g1 && p->child == g1 && g1->next == b && g1->parent == p);
    CHECK(nb_jobj() == base + 4);
    ret = HSD_JObjRemove(b);
    CHECK(ret == NULL && g1->next == NULL && p->child == g1);
    CHECK(nb_jobj() == base + 3);
    ret = HSD_JObjRemove(g1);
    CHECK(ret == NULL && p->child == NULL);
    CHECK(HSD_JObjRemove(NULL) == NULL);
    HSD_JObjRemoveAll(NULL);
    HSD_JObjRemoveAll(c);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    p = HSD_JObjAlloc();
    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g1 = HSD_JObjAlloc();
    g2 = HSD_JObjAlloc();
    g3 = HSD_JObjAlloc();
    HSD_JObjAddChild(p, a);
    HSD_JObjAddChild(p, b);
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(b, g1);
    HSD_JObjAddChild(b, g2);
    HSD_JObjAddChild(c, g3);
    CHECK(nb_jobj() == base + 7);
    HSD_JObjRemoveAll(b);
    CHECK(nb_jobj() == base + 2 && p->child == a && a->next == NULL);
    HSD_JObjRemoveAll(a);
    CHECK(p->child == NULL && nb_jobj() == base + 1);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    p = HSD_JObjAlloc();
    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    HSD_JObjAddChild(p, a);
    HSD_JObjAddChild(p, b);
    HSD_JObjAddChild(p, c);
    HSD_JObjUnref(b);
    CHECK(nb_jobj() == base + 3 && a->next == c && p->child == a);
    HSD_JObjUnref(a);
    CHECK(nb_jobj() == base + 2 && p->child == c);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    x = HSD_JObjAlloc();
    n = HSD_JObjAlloc();
    HSD_JObjRef(n);
    x->flags |= JOBJ_INSTANCE;
    x->child = n;
    HSD_JObjUnref(x);
    CHECK(nb_jobj() == base + 1 && ref_CNT(n) == 0);
    HSD_JObjUnref(n);
    CHECK(nb_jobj() == base);

    p = HSD_JObjAlloc();
    a = HSD_JObjAlloc();
    b = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    HSD_JObjAddChild(p, a);
    HSD_JObjAddChild(a, b);
    HSD_JObjAddChild(b, c);
    g_walk.n = 0;
    HSD_JObjWalkTree(p, walk_cb, &marker_ptr);
    CHECK(g_walk.n == 4 && g_walk.args == &marker_ptr);
    CHECK(g_walk.j[0] == p && g_walk.type[0] == 0);
    CHECK(g_walk.j[1] == a && g_walk.type[1] == 1);
    CHECK(g_walk.j[2] == b && g_walk.type[2] == 1);
    CHECK(g_walk.j[3] == c && g_walk.type[3] == 1);
    q = HSD_JObjAlloc();
    HSD_JObjAddChild(a, q);
    g_walk.n = 0;
    HSD_JObjWalkTree(p, walk_cb, &marker_ptr);
    CHECK(g_walk.n == 5 && g_walk.j[3] == c && g_walk.type[3] == 1);
    CHECK(g_walk.j[4] == q && g_walk.type[4] == 2);
    g_walk.n = 0;
    HSD_JObjWalkTree(NULL, walk_cb, &marker_ptr);
    HSD_JObjWalkTree(p, NULL, &marker_ptr);
    CHECK(g_walk.n == 0);
    b->flags |= JOBJ_INSTANCE;
    g_walk.n = 0;
    HSD_JObjWalkTree(p, walk_cb, &marker_ptr);
    CHECK(g_walk.n == 4);
    b->flags &= ~JOBJ_INSTANCE;
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);
}

static void test_trsp_bits(void)
{
    u32 base = nb_jobj();
    HSD_JObj *g, *p, *c, *d;

    g = HSD_JObjAlloc();
    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    d = HSD_JObjAlloc();
    HSD_JObjAddChild(g, p);
    c->flags |= JOBJ_OPA | JOBJ_TEXEDGE;
    HSD_JObjAddChild(p, c);
    CHECK((p->flags & JOBJ_ROOT_MASK) == (JOBJ_ROOT_OPA | JOBJ_ROOT_TEXEDGE));
    CHECK((g->flags & JOBJ_ROOT_MASK) == (JOBJ_ROOT_OPA | JOBJ_ROOT_TEXEDGE));
    d->flags |= JOBJ_XLU;
    HSD_JObjAddChild(p, d);
    CHECK((p->flags & JOBJ_ROOT_MASK) == JOBJ_ROOT_MASK);
    CHECK((g->flags & JOBJ_ROOT_MASK) == JOBJ_ROOT_MASK);
    CHECK((c->flags & JOBJ_ROOT_MASK) == 0);

    HSD_JObjReparent(d, NULL);
    CHECK((p->flags & JOBJ_ROOT_MASK) == (JOBJ_ROOT_OPA | JOBJ_ROOT_TEXEDGE));
    CHECK((g->flags & JOBJ_ROOT_MASK) == JOBJ_ROOT_MASK);
    HSD_JObjReparent(c, NULL);
    CHECK((p->flags & JOBJ_ROOT_MASK) == 0);
    CHECK((g->flags & JOBJ_ROOT_MASK) == JOBJ_ROOT_MASK);

    RecalcParentTrspBits(NULL);
    HSD_JObjRemoveAll(g);
    HSD_JObjRemoveAll(c);
    HSD_JObjRemoveAll(d);
    CHECK(nb_jobj() == base);
}

static void test_flags_and_dirty(void)
{
    u32 base = nb_jobj();
    HSD_JObj *p, *c1, *c2, *g, *i, *x;
    HSD_RObj* dummy_robj = (HSD_RObj*) &base;
    Quaternion q;
    Quaternion qr;
    Vec3 vs;
    Vec3 vr;

    p = HSD_JObjAlloc();
    c1 = HSD_JObjAlloc();
    c2 = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    HSD_JObjAddChild(p, c1);
    HSD_JObjAddChild(p, c2);
    HSD_JObjAddChild(c1, g);

    CHECK(dirty(p) && dirty(c1) && dirty(c2) && dirty(g));
    clean_all(p);
    CHECK(!HSD_JObjMtxIsDirty(p) && !HSD_JObjMtxIsDirty(g));
    HSD_JObjSetTranslateX(c1, 5.0f);
    CHECK(c1->translate.x == 5.0f);
    CHECK(!dirty(p) && dirty(c1) && dirty(g) && !dirty(c2));
    clean_all(p);
    HSD_JObjSetTranslateY(p, 1.0f);
    CHECK(dirty(p) && dirty(c1) && dirty(c2) && dirty(g));

    clean_all(p);
    c1->flags |= JOBJ_MTX_INDEP_SRT;
    HSD_JObjSetTranslateX(c1, 9.0f);
    CHECK(c1->translate.x == 9.0f && !dirty(c1) && !dirty(g));
    c1->flags &= ~JOBJ_MTX_INDEP_SRT;

    g->flags |= JOBJ_MTX_INDEP_PARENT;
    HSD_JObjSetTranslateX(c1, 3.0f);
    CHECK(dirty(c1) && !dirty(g));
    g->flags &= ~JOBJ_MTX_INDEP_PARENT;

    clean_all(p);
    c2->flags |= JOBJ_USER_DEF_MTX;
    HSD_JObjSetTranslateZ(p, 2.0f);
    CHECK(dirty(c2) && !HSD_JObjMtxIsDirty(c2) && HSD_JObjMtxIsDirty(p));
    c2->flags &= ~JOBJ_USER_DEF_MTX;

    clean_all(p);
    p->flags |= JOBJ_MTX_DIRTY;
    HSD_JObjSetTranslateX(p, 4.0f);
    CHECK(!dirty(c1) && !dirty(c2) && !dirty(g));
    HSD_JObjCheckDepend(c1);
    CHECK(dirty(c1) && !dirty(g) && !dirty(c2));
    HSD_JObjCheckDepend(g);
    CHECK(dirty(g));
    HSD_JObjCheckDepend(NULL);
    clean_all(p);
    HSD_JObjCheckDepend(c1);
    CHECK(!dirty(c1));
    c1->robj = dummy_robj;
    HSD_JObjCheckDepend(c1);
    CHECK(dirty(c1));
    c1->robj = NULL;
    clean_all(p);
    c1->flags |= JOBJ_JOINT1;
    HSD_JObjCheckDepend(c1);
    CHECK(dirty(c1));
    c1->flags &= ~JOBJ_JOINT;
    clean_all(p);
    c1->flags |= JOBJ_JOINT2;
    HSD_JObjCheckDepend(c1);
    CHECK(dirty(c1));
    c1->flags &= ~JOBJ_JOINT;
    clean_all(p);
    c1->flags |= JOBJ_EFFECTOR;
    HSD_JObjCheckDepend(c1);
    CHECK(dirty(c1));
    c1->flags &= ~JOBJ_JOINT;
    clean_all(p);
    p->flags |= JOBJ_MTX_DIRTY;
    c2->flags |= JOBJ_USER_DEF_MTX;
    HSD_JObjCheckDepend(c2);
    CHECK(dirty(c2) && !HSD_JObjMtxIsDirty(c2));
    c2->flags &= ~(JOBJ_USER_DEF_MTX | JOBJ_MTX_DIRTY);
    c2->flags |= JOBJ_USER_DEF_MTX | JOBJ_MTX_INDEP_PARENT;
    HSD_JObjCheckDepend(c2);
    CHECK(!dirty(c2));
    c2->flags &= ~(JOBJ_USER_DEF_MTX | JOBJ_MTX_INDEP_PARENT);

    clean_all(p);
    HSD_JObjSetFlags(p, JOBJ_HIDDEN);
    CHECK((p->flags & JOBJ_HIDDEN) && !dirty(p) && !dirty(c1));
    HSD_JObjSetFlags(p, JOBJ_CLASSICAL_SCALE);
    CHECK((p->flags & JOBJ_CLASSICAL_SCALE) && dirty(p) && dirty(c1) &&
          dirty(g) && dirty(c2));
    clean_all(p);
    HSD_JObjSetFlags(p, JOBJ_CLASSICAL_SCALE | JOBJ_LIGHTING);
    CHECK(!dirty(p) && (p->flags & JOBJ_LIGHTING));
    HSD_JObjSetFlags(p, JOBJ_TEXGEN);
    CHECK(dirty(p) && dirty(g));
    clean_all(p);
    HSD_JObjClearFlags(p, JOBJ_CLASSICAL_SCALE);
    CHECK(!(p->flags & JOBJ_CLASSICAL_SCALE) && !dirty(p));
    HSD_JObjClearFlags(p, JOBJ_CLASSICAL_SCALE);
    CHECK(dirty(p) && dirty(c1));
    clean_all(p);
    HSD_JObjClearFlags(p, JOBJ_TEXGEN | JOBJ_LIGHTING);
    CHECK(!dirty(p) && !(p->flags & (JOBJ_TEXGEN | JOBJ_LIGHTING)));
    HSD_JObjSetFlags(p, JOBJ_CLASSICAL_SCALE);
    clean_all(p);
    HSD_JObjClearFlags(p, JOBJ_HIDDEN);
    CHECK(dirty(p));
    clean_all(p);
    HSD_JObjSetFlags(p, JOBJ_SKELETON);
    CHECK(dirty(p));
    HSD_JObjClearFlags(p, JOBJ_SKELETON | JOBJ_CLASSICAL_SCALE);
    clean_all(p);

    HSD_JObjSetFlagsAll(p, JOBJ_HIDDEN);
    CHECK((p->flags & JOBJ_HIDDEN) && (c1->flags & JOBJ_HIDDEN) &&
          (c2->flags & JOBJ_HIDDEN) && (g->flags & JOBJ_HIDDEN));
    HSD_JObjClearFlagsAll(c1, JOBJ_HIDDEN);
    CHECK((p->flags & JOBJ_HIDDEN) && !(c1->flags & JOBJ_HIDDEN) &&
          (c2->flags & JOBJ_HIDDEN) && !(g->flags & JOBJ_HIDDEN));
    HSD_JObjClearFlagsAll(p, JOBJ_HIDDEN);
    HSD_JObjSetFlagsAll(NULL, JOBJ_HIDDEN);
    HSD_JObjClearFlagsAll(NULL, JOBJ_HIDDEN);

    x = HSD_JObjAlloc();
    i = HSD_JObjAlloc();
    HSD_JObjRef(x);
    i->flags |= JOBJ_INSTANCE;
    i->child = x;
    HSD_JObjSetFlagsAll(i, JOBJ_HIDDEN);
    CHECK((i->flags & JOBJ_HIDDEN) && !(x->flags & JOBJ_HIDDEN));
    clean_all(i);
    x->flags &= ~JOBJ_MTX_DIRTY;
    HSD_JObjSetTranslateX(i, 1.0f);
    CHECK(dirty(i) && !dirty(x));
    HSD_JObjUnref(i);
    HSD_JObjUnref(x);

    clean_all(p);
    q.x = 0.1f;
    q.y = 0.2f;
    q.z = 0.3f;
    q.w = 0.4f;
    HSD_JObjSetRotation(c1, &q);
    HSD_JObjGetRotation(c1, &qr);
    CHECK(qr.x == 0.1f && qr.y == 0.2f && qr.z == 0.3f && qr.w == 0.4f);
    CHECK(dirty(c1) && dirty(g) && !dirty(p));
    clean_all(p);
    HSD_JObjSetRotationX(c1, 0.5f);
    HSD_JObjSetRotationY(c1, 0.6f);
    HSD_JObjSetRotationZ(c1, 0.7f);
    CHECK(HSD_JObjGetRotationX(c1) == 0.5f && HSD_JObjGetRotationY(c1) == 0.6f &&
          HSD_JObjGetRotationZ(c1) == 0.7f && c1->rotate.w == 0.4f);
    HSD_JObjAddRotationX(c1, 0.25f);
    HSD_JObjAddRotationY(c1, 0.25f);
    HSD_JObjAddRotationZ(c1, 0.25f);
    CHECK_NEAR(c1->rotate.x, 0.75, 1e-6);
    CHECK_NEAR(c1->rotate.y, 0.85, 1e-6);
    CHECK_NEAR(c1->rotate.z, 0.95, 1e-6);
    vs = v3(2.0f, 3.0f, 4.0f);
    HSD_JObjSetScale(c1, &vs);
    HSD_JObjGetScale(c1, &vr);
    CHECK(vr.x == 2.0f && vr.y == 3.0f && vr.z == 4.0f);
    HSD_JObjSetScaleX(c1, 5.0f);
    HSD_JObjSetScaleY(c1, 6.0f);
    HSD_JObjSetScaleZ(c1, 7.0f);
    CHECK(HSD_JObjGetScaleX(c1) == 5.0f && HSD_JObjGetScaleY(c1) == 6.0f &&
          HSD_JObjGetScaleZ(c1) == 7.0f);
    HSD_JObjAddScaleX(c1, 1.0f);
    HSD_JObjAddScaleY(c1, 1.0f);
    HSD_JObjAddScaleZ(c1, 1.0f);
    CHECK(c1->scale.x == 6.0f && c1->scale.y == 7.0f && c1->scale.z == 8.0f);
    vs = v3(1.0f, 2.0f, 3.0f);
    HSD_JObjSetTranslate(c1, &vs);
    HSD_JObjGetTranslation(c1, &vr);
    CHECK(vr.x == 1.0f && vr.y == 2.0f && vr.z == 3.0f);
    HSD_JObjSetTranslateX(c1, 4.0f);
    HSD_JObjSetTranslateY(c1, 5.0f);
    HSD_JObjSetTranslateZ(c1, 6.0f);
    CHECK(HSD_JObjGetTranslationX(c1) == 4.0f &&
          HSD_JObjGetTranslationY(c1) == 5.0f &&
          HSD_JObjGetTranslationZ(c1) == 6.0f);
    HSD_JObjAddTranslationX(c1, 1.0f);
    HSD_JObjAddTranslationY(c1, 1.0f);
    HSD_JObjAddTranslationZ(c1, 1.0f);
    CHECK(c1->translate.x == 5.0f && c1->translate.y == 6.0f &&
          c1->translate.z == 7.0f);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);
}

static void test_matrices(void)
{
    u32 base = nb_jobj();
    u32 vec_base = using_count(HSD_VecGetAllocData());
    HSD_JObj *p, *c, *g;
    Mtx keep;
    int i;

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    set_srt(p, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(10, 0, 0));
    set_t(c, 1, 0, 0);
    set_t(g, 0, 2, 0);
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(c, g);
    clean_all(p);
    p->flags |= JOBJ_MTX_DIRTY;
    c->flags |= JOBJ_MTX_DIRTY;
    g->flags |= JOBJ_MTX_DIRTY;
    HSD_JObjSetupMatrix(g);
    CHECK(!dirty(p) && !dirty(c) && !dirty(g));
    CHECK_MTX(p->mtx, 0, -1, 0, 10, 1, 0, 0, 0, 0, 0, 1, 0);
    CHECK_MTX(c->mtx, 0, -1, 0, 10, 1, 0, 0, 1, 0, 0, 1, 0);
    CHECK_MTX(g->mtx, 0, -1, 0, 8, 1, 0, 0, 1, 0, 0, 1, 0);
    CHECK(p->scl != NULL && c->scl != NULL && g->scl != NULL);
    CHECK(p->scl->x == 1.0f && g->scl->z == 1.0f);

    for (i = 0; i < 12; i++) {
        keep[i / 4][i % 4] = g->mtx[i / 4][i % 4];
    }
    g->mtx[0][3] = 123.0f;
    HSD_JObjSetupMatrix(g);
    CHECK(g->mtx[0][3] == 123.0f);
    g->mtx[0][3] = keep[0][3];

    HSD_JObjSetTranslateX(c, 3.0f);
    CHECK(!dirty(p) && dirty(c) && dirty(g));
    CHECK(HSD_JObjGetMtxPtr(g) == g->mtx);
    CHECK_MTX(c->mtx, 0, -1, 0, 10, 1, 0, 0, 3, 0, 0, 1, 0);
    CHECK_MTX(g->mtx, 0, -1, 0, 8, 1, 0, 0, 3, 0, 0, 1, 0);

    p->mtx[1][3] = 55.0f;
    HSD_JObjSetupMatrixSub(p);
    CHECK(p->mtx[1][3] == 0.0f && !dirty(p));

    c->flags |= JOBJ_USER_DEF_MTX;
    HSD_JObjSetTranslateX(p, 11.0f);
    c->mtx[0][3] = 77.0f;
    HSD_JObjSetupMatrix(c);
    CHECK(c->mtx[0][3] == 77.0f);
    c->flags &= ~JOBJ_USER_DEF_MTX;
    HSD_JObjSetTranslateX(p, 10.0f);

    HSD_JObjCopyMtx(c, p->mtx);
    CHECK_MTX(c->mtx, 0, -1, 0, 10, 1, 0, 0, 0, 0, 0, 1, 0);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    set_srt(p, v3(1, 1, 1), v3(HALF_PI, 0, 0), v3(1, 2, 3));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 1, 0, 0, 1, 0, 0, -1, 2, 0, 1, 0, 3);
    set_srt(p, v3(1, 1, 1), v3(0, HALF_PI, 0), v3(0, 0, 0));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0);
    set_srt(p, v3(1, 1, 1), v3(HALF_PI, HALF_PI, 0), v3(1, 2, 3));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, 1, 0, 1, 0, 0, -1, 2, -1, 0, 0, 3);
    set_srt(p, v3(1, 1, 1), v3(0, HALF_PI, HALF_PI), v3(0, 0, 0));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, -1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0);
    set_srt(p, v3(2, 3, 4), v3(0, 0, 0), v3(0, 0, 0));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0);
    set_srt(p, v3(2, 3, 4), v3(0, 0, HALF_PI), v3(0, 0, 0));
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, -3, 0, 0, 2, 0, 0, 0, 0, 0, 4, 0);
    CHECK(p->scl != NULL && p->scl->x == 2.0f && p->scl->y == 3.0f &&
          p->scl->z == 4.0f);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    set_srt(p, v3(2, 3, 4), v3(0, 0, 0), v3(1, 1, 1));
    set_t(c, 1, 0, 0);
    set_t(g, 0, 0, 1);
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(c, g);
    HSD_JObjSetupMatrix(g);
    CHECK_MTX(c->mtx, 2, 0, 0, 3, 0, 3, 0, 1, 0, 0, 4, 1);
    CHECK(c->scl->x == 2.0f && c->scl->y == 3.0f && c->scl->z == 4.0f);
    CHECK_MTX(g->mtx, 2, 0, 0, 3, 0, 3, 0, 1, 0, 0, 4, 5);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    set_srt(p, v3(2, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    set_srt(c, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(0, 0, 0));
    HSD_JObjAddChild(p, c);
    HSD_JObjSetupMatrix(c);
    CHECK_MTX(c->mtx, 0, -1, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0);
    CHECK(c->scl->x == 2.0f && c->scl->y == 1.0f && c->scl->z == 1.0f);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    set_srt(p, v3(2, 2, 2), v3(0, 0, 0), v3(0, 0, 0));
    set_srt(c, v3(3, 3, 3), v3(0, 0, 0), v3(0, 0, 0));
    set_t(g, 1, 0, 0);
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(c, g);
    HSD_JObjSetupMatrix(g);
    CHECK_MTX(c->mtx, 6, 0, 0, 0, 0, 6, 0, 0, 0, 0, 6, 0);
    CHECK(c->scl->x == 6.0f && c->scl->y == 6.0f && c->scl->z == 6.0f);
    CHECK_MTX(g->mtx, 6, 0, 0, 6, 0, 6, 0, 0, 0, 0, 6, 0);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    set_srt(p, v3(2, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    set_srt(c, v3(3, 3, 3), v3(0, 0, HALF_PI), v3(0, 0, 0));
    HSD_JObjAddChild(p, c);
    HSD_JObjSetupMatrix(c);
    CHECK_MTX(c->mtx, 0, -3, 0, 0, 6, 0, 0, 0, 0, 0, 3, 0);
    CHECK(c->scl->x == 6.0f && c->scl->y == 3.0f && c->scl->z == 3.0f);
    HSD_JObjSetFlags(c, JOBJ_CLASSICAL_SCALE);
    HSD_JObjSetupMatrix(c);
    CHECK_MTX(c->mtx, 0, -3, 0, 0, 6, 0, 0, 0, 0, 0, 3, 0);
    CHECK(c->scl->x == 2.0f && c->scl->y == 1.0f && c->scl->z == 1.0f);
    CHECK(c->scl != p->scl);
    HSD_JObjClearFlags(c, JOBJ_CLASSICAL_SCALE);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    set_srt(p, v3(2, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    set_srt(c, v3(3, 3, 3), v3(0, 0, 0), v3(0, 0, 0));
    c->flags |= JOBJ_CLASSICAL_SCALE;
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(c, g);
    HSD_JObjSetupMatrix(g);
    CHECK(g->scl->x == 2.0f && g->scl->y == 1.0f && g->scl->z == 1.0f);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base + 3);
    p->flags |= JOBJ_CLASSICAL_SCALE;
    HSD_JObjSetMtxDirtySub(p);
    HSD_JObjSetupMatrix(g);
    CHECK(p->scl == NULL && c->scl == NULL && g->scl != NULL && g->scl->x == 1.0f);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base + 1);
    CHECK_MTX(c->mtx, 6, 0, 0, 0, 0, 3, 0, 0, 0, 0, 3, 0);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    set_srt(p, v3(-1, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    set_srt(c, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(1, 0, 0));
    HSD_JObjAddChild(p, c);
    HSD_JObjSetupMatrix(c);
    CHECK_MTX(p->mtx, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);
    CHECK(p->scl->x == -1.0f && c->scl->x == -1.0f && c->scl->y == 1.0f);
    CHECK_MTX(c->mtx, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 1, 0);
    set_srt(c, v3(-2, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    HSD_JObjSetupMatrix(c);
    CHECK(c->scl->x == 2.0f && c->scl->y == 1.0f);
    CHECK_MTX(c->mtx, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    p->flags |= JOBJ_USE_QUATERNION;
    {
        Quaternion q;
        Vec3 s = v3(2, 2, 2);
        Vec3 t = v3(1, 2, 3);
        q.x = 0.0f;
        q.y = 0.0f;
        q.z = (f32) sin((double) QUARTER_PI);
        q.w = (f32) cos((double) QUARTER_PI);
        HSD_JObjSetScale(p, &s);
        HSD_JObjSetTranslate(p, &t);
        HSD_JObjSetRotation(p, &q);
    }
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, -2, 0, 1, 2, 0, 0, 2, 0, 0, 2, 3);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    set_srt(p, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(0, 5, 0));
    set_t(c, 1, 0, 0);
    {
        HSD_AObj* ao = HSD_AObjAlloc();
        ao->hsd_obj = HSD_OBJ(p);
        HSD_JObjRef(p);
        c->aobj = ao;
        HSD_JObjSetupMatrix(c);
        CHECK_MTX(c->mtx, 1, 0, 0, 0, 0, 1, 0, 6, 0, 0, 1, 0);
        CHECK(!dirty(p) && !dirty(c));
        c->aobj = NULL;
        HSD_AObjRemove(ao);
        CHECK(ref_CNT(p) == 0 && nb_jobj() == base + 2);
    }
    HSD_JObjRemoveAll(c);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    set_srt(p, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(10, 0, 0));
    HSD_JObjAddChild(p, c);
    set_srt(c, v3(1, 1, 1), v3(0.3f, 0.4f, 0.5f), v3(1, 0, 0));
    HSD_JObjSetupMatrix(c);
    {
        HSD_ObjData val;
        val.p.x = 0.0f;
        set_srt(c, v3(7, 7, 7), v3(1, 1, 1), v3(99, 99, 99));
        c->flags &= ~JOBJ_MTX_DIRTY;
        JObjUpdateFunc(c, 0x38, &val);
        CHECK_NEAR(c->translate.x, 1.0, EPS);
        CHECK_NEAR(c->translate.y, 0.0, EPS);
        CHECK_NEAR(c->translate.z, 0.0, EPS);
        CHECK(c->rotate.x == 1.0f && c->scale.x == 7.0f);
        JObjUpdateFunc(c, 0x37, &val);
        CHECK_NEAR(c->rotate.x, 0.3, EPS);
        CHECK_NEAR(c->rotate.y, 0.4, EPS);
        CHECK_NEAR(c->rotate.z, 0.5, EPS);
        CHECK(c->scale.x == 7.0f && c->translate.x < 1.5f);
        JObjUpdateFunc(c, 0x39, &val);
        CHECK_NEAR(c->scale.x, 1.0, EPS);
        CHECK_NEAR(c->scale.y, 1.0, EPS);
        CHECK_NEAR(c->scale.z, 1.0, EPS);
        set_srt(c, v3(7, 7, 7), v3(1, 1, 1), v3(99, 99, 99));
        JObjUpdateFunc(c, 0x36, &val);
        CHECK_NEAR(c->translate.x, 1.0, EPS);
        CHECK_NEAR(c->rotate.z, 0.5, EPS);
        CHECK_NEAR(c->scale.y, 1.0, EPS);
        JObjUpdateFunc(p, 0x36, &val);
        CHECK_NEAR(p->translate.x, 10.0, EPS);
        CHECK_NEAR(p->rotate.z, HALF_PI, EPS);
    }
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base);
}

typedef struct DptclLog {
    int n;
    int a;
    int lo;
    int hi;
    HSD_JObj* j;
} DptclLog;

static DptclLog g_dptcl;

static void dptcl_cb(int a, int lo, int hi, HSD_JObj* jobj)
{
    g_dptcl.n++;
    g_dptcl.a = a;
    g_dptcl.lo = lo;
    g_dptcl.hi = hi;
    g_dptcl.j = jobj;
}

static void upd(HSD_JObj* j, int type, f32 fv)
{
    HSD_ObjData d;
    d.fv = fv;
    JObjUpdateFunc(j, type, &d);
}

static void test_update_func(void)
{
    u32 base = nb_jobj();
    HSD_JObj *j, *k;
    HSD_ObjData d;
    HSD_Spline spline;
    static DiscVec3 cv[5];
    static DiscF32 seg[3];
    HSD_JObj* path;
    HSD_AObj* ao;

    j = HSD_JObjAlloc();
    k = HSD_JObjAlloc();
    HSD_JObjAddChild(j, k);
    clean_all(j);

    upd(j, HSD_A_J_TRAX, 1.5f);
    upd(j, HSD_A_J_TRAY, 2.5f);
    upd(j, HSD_A_J_TRAZ, 3.5f);
    upd(j, HSD_A_J_ROTX, 0.1f);
    upd(j, HSD_A_J_ROTY, 0.2f);
    upd(j, HSD_A_J_ROTZ, 0.3f);
    CHECK(j->translate.x == 1.5f && j->translate.y == 2.5f &&
          j->translate.z == 3.5f);
    CHECK(j->rotate.x == 0.1f && j->rotate.y == 0.2f && j->rotate.z == 0.3f);
    CHECK(dirty(j) && dirty(k));

    upd(j, HSD_A_J_SCAX, 2.0f);
    upd(j, HSD_A_J_SCAY, -3.0f);
    upd(j, HSD_A_J_SCAZ, 4.0f);
    CHECK(j->scale.x == 2.0f && j->scale.y == -3.0f && j->scale.z == 4.0f);
    upd(j, HSD_A_J_SCAX, 0.0005f);
    upd(j, HSD_A_J_SCAY, -0.0005f);
    upd(j, HSD_A_J_SCAZ, 0.0f);
    CHECK(j->scale.x == 1e-3f && j->scale.y == 1e-3f && j->scale.z == 1e-3f);
    upd(j, HSD_A_J_SCAX, 1e-3f);
    CHECK(j->scale.x == 1e-3f);
    upd(j, HSD_A_J_SCAX, -0.002f);
    CHECK(j->scale.x == -0.002f);

    j->flags |= JOBJ_HIDDEN | JOBJ_MTX_INDEP_SRT;
    k->flags |= JOBJ_HIDDEN;
    clean_all(j);
    upd(j, HSD_A_J_TRAX, 8.0f);
    CHECK(j->translate.x == 8.0f && !dirty(j) && !dirty(k));
    j->flags &= ~JOBJ_MTX_INDEP_SRT;
    upd(j, HSD_A_J_NODE, 1.0f);
    CHECK(!(j->flags & JOBJ_HIDDEN) && (k->flags & JOBJ_HIDDEN));
    upd(j, HSD_A_J_NODE, 0.5f);
    CHECK((j->flags & JOBJ_HIDDEN) && (k->flags & JOBJ_HIDDEN));
    upd(j, HSD_A_J_BRANCH, 0.75f);
    CHECK(!(j->flags & JOBJ_HIDDEN) && !(k->flags & JOBJ_HIDDEN));
    upd(j, HSD_A_J_BRANCH, 0.0f);
    CHECK((j->flags & JOBJ_HIDDEN) && (k->flags & JOBJ_HIDDEN));
    upd(j, HSD_A_J_NODE, 0.75f);
    CHECK(!(j->flags & JOBJ_HIDDEN) && (k->flags & JOBJ_HIDDEN));

    d.iv = 0;
    JObjUpdateFunc(NULL, HSD_A_J_TRAX, &d);
    upd(j, 99, 1.0f);
    upd(j, HSD_A_J_SETBYTE3, 1.0f);
    upd(j, HSD_A_J_SETFLOAT7, 1.0f);
    upd(j, 0x29, 1.0f);
    upd(j, 0x2A, 1.0f);
    upd(j, 0x28, 1.0f);

    HSD_JObjSetDPtclCallback(dptcl_cb);
    d.iv = (0x123456 << 6) | 5;
    JObjUpdateFunc(j, 0x28, &d);
    CHECK(g_dptcl.n == 1 && g_dptcl.a == 0 && g_dptcl.lo == 5 &&
          g_dptcl.hi == 0x123456 && g_dptcl.j == j);
    d.iv = 0x3F;
    JObjUpdateFunc(k, 0x28, &d);
    CHECK(g_dptcl.n == 2 && g_dptcl.lo == 0x3F && g_dptcl.hi == 0 &&
          g_dptcl.j == k);
    HSD_JObjSetDPtclCallback(NULL);
    JObjUpdateFunc(k, 0x28, &d);
    CHECK(g_dptcl.n == 2);

    d.p.x = 1.0f;
    d.p.y = 2.0f;
    d.p.z = 3.0f;
    JObjUpdateFunc(k, 0x32, &d);
    d.p.x = 4.0f;
    d.p.y = 5.0f;
    d.p.z = 6.0f;
    JObjUpdateFunc(k, 0x33, &d);
    d.p.x = 7.0f;
    d.p.y = 8.0f;
    d.p.z = 9.0f;
    JObjUpdateFunc(k, 0x34, &d);
    d.p.x = 10.0f;
    d.p.y = 11.0f;
    d.p.z = 12.0f;
    JObjUpdateFunc(k, 0x35, &d);
    CHECK_MTX(k->mtx, 1, 4, 7, 10, 2, 5, 8, 11, 3, 6, 9, 12);

    memset(&spline, 0, sizeof(spline));
    memset(cv, 0, sizeof(cv));
    cv[1].x = 10.0f;
    cv[2].x = 10.0f;
    cv[2].y = 20.0f;
    seg[0].v = 0.0f;
    seg[1].v = 1.0f / 3.0f;
    seg[2].v = 1.0f;
    spline.type = 0;
    spline.numcv = 3;
    spline.cv = cv;
    spline.totalLength = 30.0f;
    spline.segLength = seg;

    path = HSD_JObjAlloc();
    path->flags |= JOBJ_SPLINE;
    path->u.spline = &spline;
    ao = HSD_AObjAlloc();
    ao->hsd_obj = HSD_OBJ(path);
    HSD_JObjRef(path);
    j->aobj = ao;
    upd(j, HSD_A_J_PATH, 0.5f);
    CHECK_NEAR(j->translate.x, 10.0, EPS);
    CHECK_NEAR(j->translate.y, 5.0, EPS);
    CHECK_NEAR(j->translate.z, 0.0, EPS);
    upd(j, HSD_A_J_PATH, 1.0f / 6.0f);
    CHECK_NEAR(j->translate.x, 5.0, EPS);
    CHECK_NEAR(j->translate.y, 0.0, EPS);
    upd(j, HSD_A_J_PATH, -2.0f);
    CHECK_NEAR(j->translate.x, 0.0, EPS);
    CHECK_NEAR(j->translate.y, 0.0, EPS);
    upd(j, HSD_A_J_PATH, 7.0f);
    CHECK_NEAR(j->translate.x, 10.0, EPS);
    CHECK_NEAR(j->translate.y, 20.0, EPS);
    j->aobj = NULL;
    HSD_AObjRemove(ao);
    HSD_JObjUnref(path);

    HSD_JObjRemoveAll(j);
    CHECK(nb_jobj() == base);
}

static void test_anim(void)
{
    u32 base = nb_jobj();
    u32 aobj_base = using_count(HSD_AObjGetAllocData());
    u32 fobj_base = using_count(HSD_FObjGetAllocData());
    HSD_JObj *p, *c, *q, *g;
    Ramp* r;
    Ramp* r2;
    HSD_AObj* ao;
    HSD_AObj* first;
    int i;

    p = HSD_JObjAlloc();
    r = ramp_new();
    ramp_add(r, HSD_A_J_TRAX, 0.0f, 100.0f);
    ramp_add(r, HSD_A_J_TRAY, 0.0f, -50.0f);
    ramp_add(r, HSD_A_J_TRAZ, 5.0f, 15.0f);
    ramp_add(r, HSD_A_J_ROTZ, 0.0f, HALF_PI * 2.0f);
    ramp_add(r, HSD_A_J_SCAX, 1.0f, 3.0f);
    ramp_add(r, HSD_A_J_SCAY, 1.0f, 2.0f);
    ramp_add(r, HSD_A_J_SCAZ, 2.0f, 0.5f);
    HSD_JObjAddAnim(p, ramp_joint(r, 0, 0), NULL, NULL);
    CHECK(p->aobj != NULL);
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base + 1);
    CHECK(using_count(HSD_FObjGetAllocData()) == fobj_base + 7);

    clean_all(p);
    HSD_JObjAnim(p);
    CHECK(p->translate.x == 0.0f && !dirty(p));

    HSD_JObjReqAnim(p, 0.0f);
    step(p, 1);
    CHECK_NEAR(p->translate.x, 0.0, EPS);
    CHECK_NEAR(p->translate.z, 5.0, EPS);
    CHECK_NEAR(p->scale.x, 1.0, EPS);
    CHECK_NEAR(p->scale.z, 2.0, EPS);
    CHECK(dirty(p));
    step(p, 5);
    CHECK_NEAR(p->translate.x, 50.0, EPS);
    CHECK_NEAR(p->translate.y, -25.0, EPS);
    CHECK_NEAR(p->translate.z, 10.0, EPS);
    CHECK_NEAR(p->rotate.z, HALF_PI, EPS);
    CHECK_NEAR(p->scale.x, 2.0, EPS);
    CHECK_NEAR(p->scale.y, 1.5, EPS);
    CHECK_NEAR(p->scale.z, 1.25, EPS);
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, -1.5f, 0, 50, 2, 0, 0, -25, 0, 0, 1.25f, 10);
    step(p, 4);
    CHECK_NEAR(p->translate.x, 90.0, EPS);
    step(p, 1);
    CHECK_NEAR(p->translate.x, 100.0, EPS);
    CHECK_NEAR(p->translate.y, -50.0, EPS);
    CHECK_NEAR(p->rotate.z, 2.0 * HALF_PI, EPS);
    CHECK_NEAR(p->scale.z, 0.5, EPS);
    step(p, 3);
    CHECK_NEAR(p->translate.x, 100.0, EPS);

    HSD_JObjReqAnim(p, 5.0f);
    step(p, 1);
    CHECK_NEAR(p->translate.x, 50.0, EPS);
    step(p, 2);
    CHECK_NEAR(p->translate.x, 70.0, EPS);

    r2 = ramp_new();
    ramp_add(r2, HSD_A_J_TRAX, 200.0f, 300.0f);
    HSD_JObjAddAnim(p, ramp_joint(r2, 0, 0), NULL, NULL);
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base + 1);
    CHECK(using_count(HSD_FObjGetAllocData()) == fobj_base + 1);
    HSD_JObjReqAnim(p, 0.0f);
    step(p, 3);
    CHECK_NEAR(p->translate.x, 220.0, EPS);
    HSD_JObjReqAnimByFlags(p, 0, 0.0f);
    step(p, 1);
    CHECK_NEAR(p->translate.x, 230.0, EPS);
    HSD_JObjAddAnim(p, NULL, NULL, NULL);
    CHECK(p->aobj != NULL);
    HSD_JObjAddAnim(NULL, ramp_joint(r2, 0, 0), NULL, NULL);
    HSD_JObjRemoveAnim(p);
    CHECK(p->aobj == NULL);
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base &&
          using_count(HSD_FObjGetAllocData()) == fobj_base);
    step(p, 1);
    CHECK_NEAR(p->translate.x, 230.0, EPS);
    HSD_JObjAddAnim(p, ramp_joint(r2, 0, 0), NULL, NULL);
    HSD_JObjRemoveAnimByFlags(p, 0);
    CHECK(p->aobj != NULL);
    HSD_JObjRemoveAnimByFlags(p, 1);
    CHECK(p->aobj == NULL);
    HSD_JObjRemoveAnim(NULL);
    HSD_JObjReqAnim(NULL, 0.0f);
    HSD_JObjAnim(NULL);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    r = ramp_new();
    ramp_add(r, HSD_A_J_ROTX, 0.0f, 2.0f * HALF_PI);
    ramp_add(r, HSD_A_J_ROTY, 0.0f, -2.0f * HALF_PI);
    HSD_JObjAddAnim(p, ramp_joint(r, 0, 0), NULL, NULL);
    HSD_JObjReqAnim(p, 0.0f);
    step(p, 6);
    CHECK_NEAR(p->rotate.x, HALF_PI, EPS);
    CHECK_NEAR(p->rotate.y, -HALF_PI, EPS);
    HSD_JObjSetupMatrix(p);
    CHECK_MTX(p->mtx, 0, -1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0);
    HSD_JObjRemoveAll(p);

    p = HSD_JObjAlloc();
    c = HSD_JObjAlloc();
    g = HSD_JObjAlloc();
    q = HSD_JObjAlloc();
    set_t(c, 1, 0, 0);
    set_t(g, 0, 0, 0);
    HSD_JObjAddChild(p, c);
    HSD_JObjAddChild(c, g);
    HSD_JObjAddChild(p, q);
    {
        HSD_AnimJoint aj_p, aj_c, aj_g, aj_q;
        Ramp *rp, *rc, *rg;
        rp = ramp_new();
        rc = ramp_new();
        rg = ramp_new();
        ramp_add(rp, HSD_A_J_ROTZ, 0.0f, 2.0f * HALF_PI);
        ramp_add(rc, HSD_A_J_TRAX, 0.0f, 10.0f);
        ramp_add(rg, HSD_A_J_TRAY, 0.0f, 20.0f);
        aj_p = *ramp_joint(rp, 0, 0);
        aj_c = *ramp_joint(rc, 0, 1);
        aj_g = *ramp_joint(rg, 0, 0);
        memset(&aj_q, 0, sizeof(aj_q));
        aj_p.child = &aj_c;
        aj_c.child = &aj_g;
        aj_c.next = &aj_q;
        HSD_JObjAddAnimAll(p, &aj_p, NULL, NULL);
        CHECK(p->aobj != NULL && c->aobj != NULL && g->aobj != NULL &&
              q->aobj == NULL);
        CHECK(!(p->flags & JOBJ_CLASSICAL_SCALE) &&
              (c->flags & JOBJ_CLASSICAL_SCALE) &&
              !(g->flags & JOBJ_CLASSICAL_SCALE));
        CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base + 3);
    }
    HSD_JObjAnimAll(p);
    CHECK(p->rotate.z == 0.0f && c->translate.x == 1.0f);
    HSD_JObjReqAnimAll(p, 0.0f);
    for (i = 0; i < 6; i++) {
        HSD_JObjAnimAll(p);
    }
    CHECK_NEAR(p->rotate.z, HALF_PI, EPS);
    CHECK_NEAR(c->translate.x, 5.0, EPS);
    CHECK_NEAR(g->translate.y, 10.0, EPS);
    HSD_JObjSetupMatrix(g);
    CHECK_MTX(c->mtx, 0, -1, 0, 0, 1, 0, 0, 5, 0, 0, 1, 0);
    CHECK_MTX(g->mtx, 0, -1, 0, -10, 1, 0, 0, 5, 0, 0, 1, 0);
    HSD_JObjRemoveAnimAll(p);
    CHECK(p->aobj == NULL && c->aobj == NULL && g->aobj == NULL);
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base);
    HSD_JObjAnimAll(NULL);
    HSD_JObjRemoveAnimAll(NULL);
    HSD_JObjReqAnimAll(NULL, 0.0f);

    {
        Ramp* rc = ramp_new();
        HSD_AnimJoint* ajc;
        ramp_add(rc, HSD_A_J_TRAX, 0.0f, 10.0f);
        ajc = ramp_joint(rc, 0, 0);
        HSD_JObjAddAnimAll(p, ajc, NULL, NULL);
        HSD_JObjReqAnimAll(p, 0.0f);
        HSD_JObjReqAnimAllByFlags(p, 0, 4.0f);
        HSD_JObjAnimAll(p);
        HSD_JObjAnimAll(p);
        CHECK_NEAR(p->translate.x, 1.0, EPS);
        HSD_JObjRemoveAnimAllByFlags(p, 0);
        CHECK(p->aobj != NULL);
        HSD_JObjRemoveAnimAllByFlags(p, 1);
        CHECK(p->aobj == NULL);
    }
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);

    {
        HSD_FObjDesc fd[3];
        HSD_AObjDesc ad;
        u8 t;
        static const u8 order_in[3] = { HSD_A_J_TRAX, TYPE_JOBJ, HSD_A_J_TRAY };
        for (t = 0; t < 3; t++) {
            memset(&fd[t], 0, sizeof(fd[t]));
            fd[t].length = 10;
            fd[t].type = order_in[t];
            fd[t].ad = ramp_stream(0.0f, 1.0f);
            fd[t].next = t < 2 ? &fd[t + 1] : NULL;
        }
        memset(&ad, 0, sizeof(ad));
        ad.end_frame = 10.0f;
        ad.fobjdesc = &fd[0];
        first = HSD_AObjLoadDesc(&ad);
        CHECK(first->fobj->obj_type == HSD_A_J_TRAX &&
              first->fobj->next->obj_type == TYPE_JOBJ &&
              first->fobj->next->next->obj_type == HSD_A_J_TRAY);
        JObjSortAnim(first);
        CHECK(first->fobj->obj_type == TYPE_JOBJ &&
              first->fobj->next->obj_type == HSD_A_J_TRAX &&
              first->fobj->next->next->obj_type == HSD_A_J_TRAY &&
              first->fobj->next->next->next == NULL);
        JObjSortAnim(first);
        CHECK(first->fobj->obj_type == TYPE_JOBJ &&
              first->fobj->next->obj_type == HSD_A_J_TRAX);
        HSD_AObjRemove(first);
        fd[1].type = HSD_A_J_TRAZ;
        first = HSD_AObjLoadDesc(&ad);
        JObjSortAnim(first);
        CHECK(first->fobj->obj_type == HSD_A_J_TRAX &&
              first->fobj->next->obj_type == HSD_A_J_TRAZ &&
              first->fobj->next->next->obj_type == HSD_A_J_TRAY);
        HSD_AObjRemove(first);
        JObjSortAnim(NULL);
        ao = HSD_AObjAlloc();
        JObjSortAnim(ao);
        HSD_AObjRemove(ao);
    }
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base &&
          using_count(HSD_FObjGetAllocData()) == fobj_base);
}

static HSD_Joint* mk_joints(int n)
{
    HSD_Joint* j = (HSD_Joint*) calloc((size_t) n, sizeof(HSD_Joint));
    int i;
    if (j == NULL) {
        abort();
    }
    for (i = 0; i < n; i++) {
        j[i].scale.x = 1.0f;
        j[i].scale.y = 1.0f;
        j[i].scale.z = 1.0f;
    }
    return j;
}

static void test_load_joint(void)
{
    u32 base = nb_jobj();
    u32 mtx_base = using_count(HSD_MtxGetAllocData());
    u32 vec_base = using_count(HSD_VecGetAllocData());
    u32 id_base = using_count(HSD_IDGetAllocData());
    HSD_Joint* jt = mk_joints(7);
    HSD_Joint *root = &jt[0], *a = &jt[1], *a1 = &jt[2], *b = &jt[3],
              *inst = &jt[4];
    DiscMtx env;
    HSD_JObj *jr, *ja, *ja1, *jb, *ji;
    HSD_DObjDesc dd[2];
    HSD_RObjDesc rd;
    HSD_DiscSList sl[2];
    HSD_Spline spl;
    int mobj0, pobj0, mrem0, prem0;

    memset(&env, 0, sizeof(env));
    env.m[0][0] = 1.0f;
    env.m[1][1] = 1.0f;
    env.m[2][2] = 1.0f;
    env.m[0][3] = 7.0f;
    env.m[1][3] = 8.0f;
    env.m[2][3] = 9.0f;

    root->flags = JOBJ_SKELETON_ROOT | JOBJ_OPA;
    root->rotation.z = HALF_PI;
    root->position.x = 10.0f;
    root->child = a;
    root->mtx = &env;
    a->child = a1;
    a->next = b;
    a->position.x = 1.0f;
    a->scale.x = 2.0f;
    a->scale.y = 3.0f;
    a->scale.z = 4.0f;
    a->class_name = (char*) "test_jobj";
    b->rotation.x = 0.25f;
    a1->position.y = 2.0f;
    a1->flags = JOBJ_XLU;
    b->next = inst;
    b->position.z = -1.0f;
    inst->flags = JOBJ_INSTANCE;
    inst->child = a1;

    memset(dd, 0, sizeof(dd));
    dd[0].next = &dd[1];
    b->u.dobjdesc = dd;
    memset(&rd, 0, sizeof(rd));
    rd.flags = REFTYPE_LIMIT | 8;
    rd.u.limit = 3.0f;
    b->robjdesc = &rd;

    mobj0 = g_m360AnimStub.mobjLoadDesc;
    pobj0 = g_m360AnimStub.pobjLoadDesc;
    mrem0 = g_m360AnimStub.mobjRemove;
    prem0 = g_m360AnimStub.pobjRemoveAll;
    jr = HSD_JObjLoadJoint(root);
    CHECK(jr != NULL && nb_jobj() == base + 5);
    ja = jr->child;
    CHECK(ja != NULL);
    ja1 = ja->child;
    jb = ja->next;
    CHECK(ja1 != NULL && jb != NULL);
    ji = jb->next;
    CHECK(ji != NULL && ji->next == NULL);

    CHECK(jr->parent == NULL && jr->next == NULL);
    CHECK(ja->parent == jr && jb->parent == jr && ji->parent == jr);
    CHECK(ja1->parent == ja && ja1->next == NULL && jb->child == NULL);
    CHECK(HSD_CLASS_METHOD(ja) == HSD_CLASS_INFO(&hsdJObj));
    CHECK(jr->flags == (JOBJ_MTX_DIRTY | JOBJ_SKELETON_ROOT | JOBJ_OPA));
    CHECK(ja1->flags == (JOBJ_MTX_DIRTY | JOBJ_XLU));
    CHECK((ji->flags & JOBJ_INSTANCE) != 0);
    CHECK(ji->child == ja1 && ref_CNT(ja1) == 1 && ji->u.dobj == NULL);
    CHECK(jr->id == (uintptr_t) root && ja->id == (uintptr_t) a &&
          ja1->id == (uintptr_t) a1 && ji->id == (uintptr_t) inst);
    CHECK(HSD_IDGetDataFromTable(NULL, (uintptr_t) root, NULL) == jr);
    CHECK(HSD_IDGetDataFromTable(NULL, (uintptr_t) a1, NULL) == ja1);
    CHECK(using_count(HSD_IDGetAllocData()) == id_base + 5);

    CHECK(jr->rotate.x == 0.0f && jr->rotate.z == HALF_PI &&
          jr->translate.x == 10.0f && jr->scale.x == 1.0f);
    CHECK(ja->rotate.x == 0.0f && ja->scale.x == 2.0f &&
          ja->scale.y == 3.0f && ja->scale.z == 4.0f &&
          ja->translate.x == 1.0f && jb->rotate.x == 0.25f);
    CHECK(jb->translate.z == -1.0f && jb->robj != NULL && jb->robj->next == NULL);
    CHECK((jb->robj->flags & 0xFFFFFFF) == 8 && jb->robj->u.limit == 3.0f);
    CHECK(jr->envelopemtx != NULL && jr->envelopemtx != (MtxPtr) env.m);
    CHECK(jr->envelopemtx[0][0] == 1.0f && jr->envelopemtx[1][3] == 8.0f &&
          jr->envelopemtx[2][3] == 9.0f && jr->envelopemtx[0][1] == 0.0f);
    CHECK(ja->envelopemtx == NULL && ja->scl == NULL && jr->scl == NULL);
    CHECK(using_count(HSD_MtxGetAllocData()) == mtx_base + 1);
    CHECK(jr->mtx[0][0] == 1.0f && jr->mtx[1][1] == 1.0f &&
          jr->mtx[2][2] == 1.0f && jr->mtx[0][3] == 0.0f);

    CHECK(jb->u.dobj != NULL && jb->u.dobj->next != NULL &&
          jb->u.dobj->next->next == NULL);
    CHECK(g_m360AnimStub.mobjLoadDesc == mobj0 + 2 &&
          g_m360AnimStub.pobjLoadDesc == pobj0 + 2);
    CHECK(ja->u.dobj == NULL && ja1->u.dobj == NULL);

    HSD_JObjSetupMatrix(ja1);
    CHECK_MTX(ja->mtx, 0, -3, 0, 10, 2, 0, 0, 1, 0, 0, 4, 0);
    CHECK_MTX(ja1->mtx, 0, -3, 0, 4, 2, 0, 0, 1, 0, 0, 4, 0);
    CHECK(ja->scl != NULL && ja->scl->x == 2.0f && ja->scl->y == 3.0f);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base + 3);
    HSD_JObjRemoveAll(jr);
    CHECK(nb_jobj() == base);
    CHECK(g_m360AnimStub.mobjRemove == mrem0 + 2 &&
          g_m360AnimStub.pobjRemoveAll == prem0 + 2);
    CHECK(using_count(HSD_IDGetAllocData()) == id_base);
    CHECK(using_count(HSD_MtxGetAllocData()) == mtx_base);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base);
    CHECK(HSD_IDGetDataFromTable(NULL, (uintptr_t) root, NULL) == NULL);

    memset(sl, 0, sizeof(sl));
    memset(&spl, 0, sizeof(spl));
    sl[0].next = &sl[1];
    sl[0].data = 5;
    sl[1].data = 6;
    root->flags = JOBJ_PTCL;
    root->child = NULL;
    root->mtx = NULL;
    root->u.ptcl = sl;
    jr = HSD_JObjLoadJoint(root);
    CHECK(jr != NULL && (jr->flags & JOBJ_PTCL) && jr->u.ptcl == sl);
    CHECK(sl[0].data == 0x80000005u && sl[1].data == 0x80000006u);
    HSD_JObjRemoveAll(jr);
    root->flags = JOBJ_SPLINE;
    root->u.spline = &spl;
    jr = HSD_JObjLoadJoint(root);
    CHECK(jr != NULL && jr->u.spline == &spl && jr->u.dobj == (HSD_DObj*) &spl);
    HSD_JObjRemoveAll(jr);
    CHECK(nb_jobj() == base);
    HSD_JObjLoadJoint(NULL);

    root->flags = 0;
    root->u.dobjdesc = NULL;
    root->rotation.z = 0.0f;
    root->position.x = 0.0f;
    root->child = a;
    jr = HSD_JObjLoadJoint(root);
    ja = jr->child;
    ja->translate.x = 55.0f;
    ja->next->rotate.x = 9.0f;
    ja->rotate.x = 5.0f;
    ja->scale.x = 5.0f;
    jr->translate.z = 6.0f;
    clean_all(jr);
    HSD_JObjResetRST(jr, root);
    CHECK(jr->translate.z == 0.0f && ja->translate.x == 1.0f &&
          ja->rotate.x == 0.0f && ja->scale.x == 2.0f &&
          ja->next->rotate.x == 0.25f);
    CHECK(dirty(jr) && dirty(ja));
    HSD_JObjResetRST(NULL, root);
    HSD_JObjResetRST(jr, NULL);
    JObjResetRST(NULL, root);
    HSD_JObjRemoveAll(jr);
    CHECK(nb_jobj() == base);
    free(jt);
}

static void test_disp(void)
{
    u32 base = nb_jobj();
    HSD_JObj *root, *c1, *c2, *gc, *inst, *tgt;
    HSD_CObj* cobj = (HSD_CObj*) calloc(1, sizeof(HSD_CObj));
    Mtx vm;
    int i;

    if (cobj == NULL) {
        abort();
    }
    root = HSD_JObjAlloc();
    c1 = HSD_JObjAlloc();
    c2 = HSD_JObjAlloc();
    gc = HSD_JObjAlloc();
    c1->flags |= JOBJ_OPA;
    c2->flags |= JOBJ_XLU;
    gc->flags |= JOBJ_OPA;
    HSD_JObjAddChild(c1, gc);
    HSD_JObjAddChild(root, c1);
    HSD_JObjAddChild(root, c2);
    PSMTXIdentity(vm);
    memset(&g_m360JObjStub, 0, sizeof(g_m360JObjStub));

    HSD_JObjDispAll(NULL, vm, 1, 0);
    CHECK(g_m360JObjStub.disp == 0);
    HSD_JObjDispAll(root, vm, 1, 0x1234);
    CHECK(g_m360JObjStub.disp == 2 && g_m360JObjStub.lastJObj == gc);
    CHECK(g_m360JObjStub.lastFlags == 1 && g_m360JObjStub.lastRenderMode == 0x1234);
    CHECK(g_m360JObjStub.lastVMtx == (void*) vm);
    memset(&g_m360JObjStub, 0, sizeof(g_m360JObjStub));
    HSD_JObjDispAll(root, vm, 2, 0);
    CHECK(g_m360JObjStub.disp == 1 && g_m360JObjStub.lastJObj == c2 &&
          g_m360JObjStub.lastFlags == 2);
    memset(&g_m360JObjStub, 0, sizeof(g_m360JObjStub));
    HSD_JObjDispAll(root, vm, 4, 0);
    CHECK(g_m360JObjStub.disp == 0);
    HSD_JObjDispAll(root, vm, 3, 0);
    CHECK(g_m360JObjStub.disp == 3);
    root->flags |= JOBJ_OPA | JOBJ_HIDDEN;
    memset(&g_m360JObjStub, 0, sizeof(g_m360JObjStub));
    HSD_JObjDispAll(root, vm, 1, 0);
    CHECK(g_m360JObjStub.disp == 3);
    root->flags &= ~(JOBJ_OPA | JOBJ_HIDDEN);

    inst = HSD_JObjAlloc();
    tgt = HSD_JObjAlloc();
    HSD_JObjRef(tgt);
    inst->flags |= JOBJ_INSTANCE;
    inst->child = tgt;
    tgt->flags |= JOBJ_OPA;
    set_t(inst, 3, 0, 0);
    set_t(tgt, 1, 0, 0);
    PSMTXIdentity(cobj->view_mtx);
    cobj->view_mtx[2][3] = -5.0f;
    memset(&g_m360JObjStub, 0, sizeof(g_m360JObjStub));
    g_m360JObjStub.currentCObj = cobj;
    HSD_JObjDispAll(inst, vm, 1, 7);
    CHECK(g_m360JObjStub.cobjGetCurrent == 1 && g_m360JObjStub.disp == 1);
    CHECK(g_m360JObjStub.lastJObj == tgt && g_m360JObjStub.lastRenderMode == 7);
    {
        const f32 want[12] = { 1, 0, 0, 2, 0, 1, 0, 0, 0, 0, 1, -5 };
        for (i = 0; i < 12; i++) {
            CHECK_NEAR(g_m360JObjStub.lastVMtxCopy[i], want[i], EPS);
        }
    }
    CHECK(!dirty(inst) && !dirty(tgt));
    inst->flags |= JOBJ_HIDDEN;
    HSD_JObjDispAll(inst, vm, 1, 7);
    CHECK(g_m360JObjStub.cobjGetCurrent == 1 && g_m360JObjStub.disp == 1);
    g_m360JObjStub.currentCObj = NULL;
    HSD_JObjUnref(inst);
    HSD_JObjUnref(tgt);

    HSD_JObjRemoveAll(root);
    free(cobj);
    CHECK(nb_jobj() == base);
}

static void test_effector(void)
{
    u32 base = nb_jobj();
    HSD_JObj *p, *e;
    HSD_RObj* hint;

    p = HSD_JObjAlloc();
    e = HSD_JObjAlloc();
    set_srt(p, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(10, 0, 0));
    set_srt(e, v3(1, 1, 1), v3(0, 0, 0), v3(0, 0, 0));
    HSD_JObjAddChild(p, e);
    e->flags |= JOBJ_EFFECTOR;
    hint = HSD_RObjAlloc();
    hint->flags = 0x80000000u | REFTYPE_IKHINT;
    hint->u.ik_hint.bone_length = 4.0f;
    hint->u.ik_hint.rotate_x = 0.0f;
    p->robj = hint;
    HSD_JObjSetupMatrix(e);
    CHECK_MTX(e->mtx, 0, -1, 0, 10, 1, 0, 0, 4, 0, 0, 1, 0);
    CHECK(!dirty(e) && !dirty(p));
    HSD_JObjSetScaleX(p, 0.5f);
    CHECK(dirty(p) && dirty(e));
    HSD_JObjSetupMatrix(e);
    CHECK_MTX(e->mtx, 0, -1, 0, 10, 0.5f, 0, 0, 2, 0, 0, 1, 0);
    HSD_JObjRemoveAll(p);
    CHECK(nb_jobj() == base);
}

static void test_wobj(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdWObj);
    u32 base = (u32) info->head.nb_exist;
    u32 aobj_base = using_count(HSD_AObjGetAllocData());
    HSD_WObj *w, *w2;
    HSD_WObjDesc desc;
    HSD_RObjDesc rd;
    HSD_JObj* jp;
    HSD_AObj* ao;
    Vec3 p;
    Ramp* r;
    HSD_WObjAnim anim;

    w = HSD_WObjAlloc();
    CHECK(w != NULL && HSD_CLASS_METHOD(w) == info);
    CHECK(strcmp(info->head.class_name, "had_wobj") == 0);
    CHECK(info->head.obj_size == (s16) sizeof(HSD_WObj));
    CHECK(info->head.parent == &hsdObj && (u32) info->head.nb_exist == base + 1);
    CHECK(w->flags == 0 && w->pos.x == 0.0f && w->aobj == NULL && w->robj == NULL);
    CHECK(HSD_WOBJ_INFO(&hsdWObj)->load != NULL);

    p = v3(1, 2, 3);
    HSD_WObjSetPosition(w, &p);
    CHECK(w->flags == 2 && w->pos.x == 1.0f && w->pos.z == 3.0f);
    w->flags = 3;
    HSD_WObjSetPosition(w, &p);
    CHECK(w->flags == 2);
    HSD_WObjSetPosition(NULL, &p);
    HSD_WObjSetPosition(w, NULL);
    memset(&p, 0, sizeof(p));
    HSD_WObjGetPosition(w, &p);
    CHECK(p.x == 1.0f && p.y == 2.0f && p.z == 3.0f);
    HSD_WObjGetPosition(NULL, &p);
    HSD_WObjGetPosition(w, NULL);
    HSD_WObjSetPositionX(w, 4.0f);
    HSD_WObjSetPositionY(w, 5.0f);
    HSD_WObjSetPositionZ(w, 6.0f);
    HSD_WObjGetPosition(w, &p);
    CHECK(p.x == 4.0f && p.y == 5.0f && p.z == 6.0f && (w->flags & 2));
    HSD_WObjClearFlags(w, 2);
    CHECK(w->flags == 0);
    HSD_WObjSetPositionX(NULL, 1.0f);

    jp = HSD_JObjAlloc();
    set_srt(jp, v3(1, 1, 1), v3(0, 0, HALF_PI), v3(0, 5, 0));
    ao = HSD_AObjAlloc();
    ao->hsd_obj = HSD_OBJ(jp);
    HSD_JObjRef(jp);
    w->aobj = ao;
    w->pos = v3(1, 0, 0);
    w->flags = 1;
    HSD_WObjGetPosition(w, &p);
    CHECK_NEAR(p.x, 0.0, EPS);
    CHECK_NEAR(p.y, 6.0, EPS);
    CHECK_NEAR(p.z, 0.0, EPS);
    CHECK((w->flags & 1) == 0);
    HSD_WObjGetPosition(w, &p);
    CHECK_NEAR(p.y, 6.0, EPS);
    w->pos = v3(1, 0, 0);
    w->flags = 1;
    HSD_WObjSetPositionY(w, 9.0f);
    CHECK_NEAR(w->pos.x, 0.0, EPS);
    CHECK_NEAR(w->pos.y, 9.0, EPS);
    CHECK_NEAR(w->pos.z, 0.0, EPS);
    CHECK((w->flags & 1) == 0 && (w->flags & 2));
    w->aobj = NULL;
    HSD_AObjRemove(ao);
    HSD_JObjUnref(jp);

    r = ramp_new();
    ramp_add(r, 5, 0.0f, 100.0f);
    ramp_add(r, 6, 10.0f, 30.0f);
    ramp_add(r, 7, -4.0f, 6.0f);
    anim.aobjdesc = &r->a;
    ramp_joint(r, 0, 0);
    anim.robjanim = NULL;
    HSD_WObjAddAnim(NULL, &anim);
    HSD_WObjAddAnim(w, NULL);
    HSD_WObjAddAnim(w, &anim);
    CHECK(w->aobj != NULL && using_count(HSD_AObjGetAllocData()) == aobj_base + 1);
    HSD_WObjInterpretAnim(w);
    CHECK_NEAR(w->pos.x, 0.0, 1e-6);
    CHECK(w->pos.y == 9.0f);
    HSD_WObjReqAnim(w, 0.0f);
    {
        int i;
        for (i = 0; i < 6; i++) {
            HSD_WObjInterpretAnim(w);
        }
    }
    CHECK_NEAR(w->pos.x, 50.0, EPS);
    CHECK_NEAR(w->pos.y, 20.0, EPS);
    CHECK_NEAR(w->pos.z, 1.0, EPS);
    HSD_WObjAddAnim(w, &anim);
    CHECK(using_count(HSD_AObjGetAllocData()) == aobj_base + 1);
    HSD_WObjRemoveAnim(w);
    CHECK(w->aobj == NULL && using_count(HSD_AObjGetAllocData()) == aobj_base);
    HSD_WObjRemoveAnim(NULL);
    HSD_WObjReqAnim(NULL, 0.0f);
    HSD_WObjInterpretAnim(NULL);

    memset(&desc, 0, sizeof(desc));
    desc.pos.x = 3.0f;
    desc.pos.y = 2.0f;
    desc.pos.z = 1.0f;
    memset(&rd, 0, sizeof(rd));
    rd.flags = REFTYPE_LIMIT | 8;
    rd.u.limit = 1.0f;
    desc.robjdesc = &rd;
    w2 = HSD_WObjLoadDesc(&desc);
    CHECK(w2 != NULL && HSD_CLASS_METHOD(w2) == info);
    CHECK(w2->pos.x == 3.0f && w2->pos.y == 2.0f && w2->pos.z == 1.0f);
    CHECK(w2->flags == 2 && w2->robj != NULL && w2->robj->u.limit == 1.0f);
    CHECK(HSD_WObjLoadDesc(NULL) == NULL);
    HSD_WObjInit(w2, &desc);
    CHECK(w2->robj != NULL && w2->robj->next == NULL);
    HSD_WObjInit(NULL, &desc);
    HSD_WObjInit(w2, NULL);
    desc.class_name = (char*) "had_wobj";
    HSD_WObjUnref(w2);
    w2 = HSD_WObjLoadDesc(&desc);
    CHECK(w2 != NULL && HSD_CLASS_METHOD(w2) == info);
    HSD_WObjUnref(w2);
    HSD_WObjUnref(NULL);
    CHECK((u32) info->head.nb_exist == base + 1);
    CHECK(using_count(HSD_RObjGetAllocData()) == 0);

    HSD_WObjSetDefaultClass(NULL);
    HSD_WObjSetDefaultClass(info);
    w2 = HSD_WObjAlloc();
    CHECK(HSD_CLASS_METHOD(w2) == info);
    HSD_WObjUnref(w2);
    HSD_WObjSetDefaultClass(NULL);
    HSD_WObjUnref(w);
    CHECK((u32) info->head.nb_exist == base);
}

static HSD_JObj* build_cycle_tree(void)
{
    HSD_JObj* root = HSD_JObjAlloc();
    int i, k;
    for (i = 0; i < 3; i++) {
        HSD_JObj* c = HSD_JObjAlloc();
        set_srt(c, v3(1, 2, 3), v3(0.1f, 0.2f, 0.3f), v3((f32) i, 1, 2));
        HSD_JObjAddChild(root, c);
        for (k = 0; k < 2; k++) {
            HSD_JObj* g = HSD_JObjAlloc();
            set_t(g, (f32) k, 0, 0);
            HSD_JObjAddChild(c, g);
        }
    }
    return root;
}

static void test_leak_cycle(void)
{
    u32 base = nb_jobj();
    u32 vec_base = using_count(HSD_VecGetAllocData());
    long heap1, heap2;
    HSD_JObj *root, *cur;
    int n;

    root = build_cycle_tree();
    n = 0;
    for (cur = root->child; cur != NULL; cur = cur->next) {
        HSD_JObj* g;
        for (g = cur->child; g != NULL; g = g->next) {
            HSD_JObjSetupMatrix(g);
            n++;
        }
    }
    CHECK(n == 6 && nb_jobj() == base + 10);
    CHECK(using_count(HSD_VecGetAllocData()) == vec_base + 10);
    HSD_JObjRemoveAll(root);
    CHECK(nb_jobj() == base && using_count(HSD_VecGetAllocData()) == vec_base);

    heap1 = OSCheckHeap(0);
    root = build_cycle_tree();
    for (cur = root->child; cur != NULL; cur = cur->next) {
        HSD_JObjSetupMatrix(cur->child);
    }
    HSD_JObjRemoveAll(root);
    heap2 = OSCheckHeap(0);
    CHECK(heap1 == heap2);
    CHECK(nb_jobj() == base);
    CHECK(using_count(HSD_MtxGetAllocData()) == 0);
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

    printf("[M360][JOBJ] object sizes host: JObj=%u WObj=%u Joint=%u "
           "(GameCube layout 136/32/64)\n",
           (unsigned) sizeof(HSD_JObj), (unsigned) sizeof(HSD_WObj),
           (unsigned) sizeof(HSD_Joint));

    test_dolphin_prims();
    test_class_and_alloc();
    test_ref_counting();
    test_tree();
    test_trsp_bits();
    test_flags_and_dirty();
    test_matrices();
    test_update_func();
    test_anim();
    test_load_joint();
    test_disp();
    test_effector();
    test_wobj();
    test_leak_cycle();
    CHECK(M360_HsdJObjSelfTest());
    CHECK(nb_jobj() == 0);

    if (fail_count) {
        fprintf(stderr, "[M360][JOBJ] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][JOBJ] jobj/wobj host validation passed\n");
    return 0;
}
