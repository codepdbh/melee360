#include "hsdjobj_xdk_compat.h"

extern "C" {
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/robj.h>
}

extern "C" {

M360JObjStubState g_m360JObjStub;

/* ---- M20 STUB SECTION: cobj and displayfunc are not ported ---------------
 * displayfunc.c is a GX renderer (zlist, immediate-mode GX, TEV state), so
 * the entry points jobj.c calls are counting no-ops. */

static void RecordVMtx(MtxPtr vmtx)
{
    g_m360JObjStub.lastVMtx = vmtx;
    if (vmtx != NULL) {
        for (int i = 0; i < 12; ++i) {
            g_m360JObjStub.lastVMtxCopy[i] = vmtx[i / 4][i % 4];
        }
    }
}

HSD_CObj* HSD_CObjGetCurrent(void)
{
    g_m360JObjStub.cobjGetCurrent++;
    return static_cast<HSD_CObj*>(g_m360JObjStub.currentCObj);
}

void HSD_JObjDisp(HSD_JObj* jobj, MtxPtr vmtx, HSD_TrspMask flags,
                  u32 rendermode)
{
    g_m360JObjStub.disp++;
    g_m360JObjStub.lastJObj = jobj;
    RecordVMtx(vmtx);
    g_m360JObjStub.lastFlags = static_cast<u32>(flags);
    g_m360JObjStub.lastRenderMode = rendermode;
}

void HSD_JObjDispSub(HSD_JObj* jobj, MtxPtr vmtx, MtxPtr pmtx,
                     HSD_TrspMask trsp_mask, u32 rendermode)
{
    (void) pmtx;
    g_m360JObjStub.dispSub++;
    g_m360JObjStub.lastJObj = jobj;
    RecordVMtx(vmtx);
    g_m360JObjStub.lastFlags = static_cast<u32>(trsp_mask);
    g_m360JObjStub.lastRenderMode = rendermode;
}

void HSD_JObjMakePositionMtx(HSD_JObj* jobj, Mtx vmtx, Mtx pmtx)
{
    (void) vmtx;
    (void) pmtx;
    g_m360JObjStub.makePositionMtx++;
    g_m360JObjStub.lastJObj = jobj;
}

} /* extern "C" */

namespace {

bool SelfTestNear(float a, float b, float eps)
{
    const float d = a - b;
    return d <= eps && d >= -eps;
}

bool SelfTestMtxIs(const Mtx m, const float (&e)[12], float eps)
{
    for (int i = 0; i < 12; ++i) {
        if (!SelfTestNear(m[i / 4][i % 4], e[i], eps)) {
            return false;
        }
    }
    return true;
}

const float kHalfPi = 1.57079632679489661923f;

bool SelfTestJObjTree()
{
    HSD_VecInitAllocData();
    HSD_MtxInitAllocData();
    HSD_AObjInitAllocData();
    HSD_RObjInitAllocData();

    HSD_JObj* root = HSD_JObjAlloc();
    HSD_JObj* a = HSD_JObjAlloc();
    HSD_JObj* c = HSD_JObjAlloc();
    if (root == NULL || a == NULL || c == NULL) {
        return false;
    }

    Vec3 rootPos = { 10.0f, 0.0f, 0.0f };
    Vec3 kidPos = { 1.0f, 0.0f, 0.0f };
    HSD_JObjSetTranslate(root, &rootPos);
    HSD_JObjSetRotationZ(root, kHalfPi);
    HSD_JObjSetTranslate(a, &kidPos);
    HSD_JObjSetTranslate(c, &kidPos);
    HSD_JObjAddChild(root, a);
    HSD_JObjAddChild(root, c);
    const bool linkOk = root->child == a && a->next == c && c->parent == root &&
                        HSD_JObjGetChild(root) == a &&
                        HSD_JObjGetNext(a) == c && c->next == NULL;

    HSD_JObjSetupMatrix(a);
    const float expectA[12] = { 0.0f, -1.0f, 0.0f, 10.0f, 1.0f, 0.0f,
                                0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f };
    const bool matrixOk = SelfTestMtxIs(a->mtx, expectA, 1e-4f) &&
                          !(a->flags & JOBJ_MTX_DIRTY) &&
                          !(root->flags & JOBJ_MTX_DIRTY) &&
                          (c->flags & JOBJ_MTX_DIRTY) != 0;

    static unsigned char stream[10] = { 0x12, 0x00, 0x00, 0x00, 0x00, 10,
                                        0x00, 0x00, 0xC8, 0x42 };
    static HSD_FObjDesc fdesc;
    fdesc.next = NULL;
    fdesc.length = sizeof(stream);
    fdesc.startframe = 0.0f;
    fdesc.type = HSD_A_J_TRAX;
    fdesc.frac_value = HSD_A_FRAC_FLOAT;
    fdesc.frac_slope = HSD_A_FRAC_FLOAT;
    fdesc.dummy0 = 0;
    fdesc.ad = stream;
    static HSD_AObjDesc adesc;
    adesc.flags = 0;
    adesc.end_frame = 10.0f;
    adesc.fobjdesc = &fdesc;
    adesc.obj_id = 0;
    static HSD_AnimJoint anim;
    anim.child = NULL;
    anim.next = NULL;
    anim.aobjdesc = &adesc;
    anim.robj_anim = NULL;
    anim.flags = 0;

    HSD_JObjAddAnim(a, &anim, NULL, NULL);
    HSD_JObjReqAnim(a, 0.0f);
    for (int k = 0; k < 4; ++k) {
        HSD_JObjAnim(a);
    }
    const bool animDirty = (a->flags & JOBJ_MTX_DIRTY) != 0;
    HSD_JObjSetupMatrix(a);
    const bool animOk = animDirty && SelfTestNear(a->translate.x, 30.0f, 1e-3f) &&
                        SelfTestNear(a->mtx[0][3], 10.0f, 1e-3f) &&
                        SelfTestNear(a->mtx[1][3], 30.0f, 1e-3f) &&
                        SelfTestNear(a->mtx[2][3], 0.0f, 1e-3f);

    HSD_JObjRemoveAll(root);
    const bool freeOk = hsdJObj.parent.parent.head.nb_exist == 0 &&
                        HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 0 &&
                        HSD_ObjAllocGetUsing(HSD_AObjGetAllocData()) == 0 &&
                        HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0;

    return linkOk && matrixOk && animOk && freeOk;
}

bool SelfTestJObjLoad()
{
    static HSD_Joint rootJoint;
    static HSD_Joint kidJoint;
    static DiscMtx envelope;
    memset(&rootJoint, 0, sizeof(rootJoint));
    memset(&kidJoint, 0, sizeof(kidJoint));
    memset(&envelope, 0, sizeof(envelope));
    envelope.m[0][0] = envelope.m[1][1] = envelope.m[2][2] = 1.0f;
    envelope.m[0][3] = 7.0f;

    rootJoint.flags = JOBJ_SKELETON_ROOT;
    rootJoint.child = &kidJoint;
    rootJoint.rotation.z = kHalfPi;
    rootJoint.scale.x = rootJoint.scale.y = rootJoint.scale.z = 1.0f;
    rootJoint.position.x = 10.0f;
    rootJoint.mtx = &envelope;
    kidJoint.scale.x = kidJoint.scale.y = kidJoint.scale.z = 1.0f;
    kidJoint.position.x = 1.0f;

    HSD_JObj* jobj = HSD_JObjLoadJoint(&rootJoint);
    if (jobj == NULL || jobj->child == NULL) {
        return false;
    }
    HSD_JObj* kid = jobj->child;
    const uintptr_t rootId = reinterpret_cast<uintptr_t>(&rootJoint);
    const bool treeOk = kid->parent == jobj && kid->next == NULL &&
                        (jobj->flags & JOBJ_SKELETON_ROOT) != 0 &&
                        jobj->id == rootId &&
                        HSD_IDGetDataFromTable(NULL, rootId, NULL) == jobj &&
                        jobj->envelopemtx != NULL &&
                        SelfTestNear(jobj->envelopemtx[0][3], 7.0f, 1e-6f);

    HSD_JObjSetupMatrix(kid);
    const float expectKid[12] = { 0.0f, -1.0f, 0.0f, 10.0f, 1.0f, 0.0f,
                                  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f };
    const bool matrixOk = SelfTestMtxIs(kid->mtx, expectKid, 1e-4f);

    HSD_JObjRemoveAll(jobj);
    s32 found = 1;
    HSD_IDGetDataFromTable(NULL, rootId, &found);
    const bool freeOk = found == 0 &&
                        hsdJObj.parent.parent.head.nb_exist == 0 &&
                        HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == 0 &&
                        HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 0;

    return treeOk && matrixOk && freeOk;
}

} // namespace

extern "C" int M360_HsdJObjStubDispCount(void)
{
    return g_m360JObjStub.disp;
}

extern "C" int M360_HsdJObjSelfTest(void)
{
    return SelfTestJObjTree() && SelfTestJObjLoad() ? 1 : 0;
}
