#include "hsdanim_xdk_compat.h"

extern "C" {
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/robj.h>
}

extern "C" {
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/id.h>

u32 HSD_GetNbBits(u32 arg0);
s32 HSD_Index2PosNrmMtx(u32 arg0);
}

#if defined(_XBOX)
typedef char M360_AssertAObjSize[(sizeof(HSD_AObj) == 0x1C) ? 1 : -1];
typedef char M360_AssertDObjSize[(sizeof(HSD_DObj) == 0x18) ? 1 : -1];
typedef char M360_AssertRObjSize[(sizeof(HSD_RObj) == 0x1C) ? 1 : -1];
typedef char M360_AssertJObjSize[(sizeof(HSD_JObj) == 0x88) ? 1 : -1];
typedef char M360_AssertRObjDescSize[(sizeof(HSD_RObjDesc) == 0xC) ? 1 : -1];
typedef char M360_AssertAObjDescSize[(sizeof(HSD_AObjDesc) == 0x10) ? 1 : -1];
typedef char M360_AssertDObjDescSize[(sizeof(HSD_DObjDesc) == 0x10) ? 1 : -1];
#endif

extern "C" {

M360AnimStubState g_m360AnimStub;

/* ---- M19 STUB SECTION: mobj/pobj are not ported yet ---------------------
 * Every function below stands in for an unported neighbour and is a
 * counting no-op. jobj is real since M20 (jobj.c). */

void HSD_MObjSetCurrent(HSD_MObj* mobj)
{
    g_m360AnimStub.mobjSetCurrent++;
    g_m360AnimStub.lastPtr = mobj;
}

void HSD_MObjRemoveAnimByFlags(HSD_MObj* mobj, u32 flags)
{
    g_m360AnimStub.mobjRemoveAnimByFlags++;
    g_m360AnimStub.lastPtr = mobj;
    g_m360AnimStub.lastFlags = flags;
}

void HSD_MObjAddAnim(HSD_MObj* mobj, HSD_MatAnim* matanim)
{
    g_m360AnimStub.mobjAddAnim++;
    g_m360AnimStub.lastPtr = mobj;
    g_m360AnimStub.lastPtr2 = matanim;
}

void HSD_MObjReqAnimByFlags(HSD_MObj* mobj, f32 startframe, u32 flags)
{
    g_m360AnimStub.mobjReqAnimByFlags++;
    g_m360AnimStub.lastPtr = mobj;
    g_m360AnimStub.lastFrame = startframe;
    g_m360AnimStub.lastFlags = flags;
}

void HSD_MObjAnim(HSD_MObj* mobj)
{
    g_m360AnimStub.mobjAnim++;
    g_m360AnimStub.lastPtr = mobj;
}

HSD_MObj* HSD_MObjLoadDesc(HSD_MObjDesc* mobjdesc)
{
    g_m360AnimStub.mobjLoadDesc++;
    g_m360AnimStub.lastPtr = mobjdesc;
    return g_m360AnimStub.mobjLoadDescResult;
}

void HSD_MObjRemove(HSD_MObj* mobj)
{
    g_m360AnimStub.mobjRemove++;
    g_m360AnimStub.lastPtr = mobj;
}

void HSD_PObjRemoveAnimAllByFlags(HSD_PObj* pobj, u32 flags)
{
    g_m360AnimStub.pobjRemoveAnimAllByFlags++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastFlags = flags;
}

void HSD_PObjAddAnimAll(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
    g_m360AnimStub.pobjAddAnimAll++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastPtr2 = shapeanim;
}

void HSD_PObjReqAnimAllByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
    g_m360AnimStub.pobjReqAnimAllByFlags++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastFrame = startframe;
    g_m360AnimStub.lastFlags = flags;
}

void HSD_PObjAnimAll(HSD_PObj* pobj)
{
    g_m360AnimStub.pobjAnimAll++;
    g_m360AnimStub.lastPtr = pobj;
}

HSD_PObj* HSD_PObjLoadDesc(HSD_PObjDesc* pobjdesc)
{
    g_m360AnimStub.pobjLoadDesc++;
    g_m360AnimStub.lastPtr = pobjdesc;
    return g_m360AnimStub.pobjLoadDescResult;
}

void HSD_PObjRemoveAll(HSD_PObj* pobj)
{
    g_m360AnimStub.pobjRemoveAll++;
    g_m360AnimStub.lastPtr = pobj;
}

void HSD_PObjResolveRefsAll(HSD_PObj* pobj, HSD_PObjDesc* desc)
{
    g_m360AnimStub.pobjResolveRefsAll++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastPtr2 = desc;
}

} /* extern "C" */

namespace {

bool SelfTestNear(float a, float b, float eps)
{
    const float d = a - b;
    return d <= eps && d >= -eps;
}
struct AnimProbe {
    int calls;
    float value;
};

void SelfTestAnimUpdate(void* obj, enum_t type, HSD_ObjData* fval)
{
    AnimProbe* probe = static_cast<AnimProbe*>(obj);
    (void) type;
    probe->calls++;
    probe->value = fval->fv;
}

struct RObjProbe {
    int count;
    int type[6];
    float value[6][3];
};

void SelfTestRObjUpdate(void* obj, enum_t type, HSD_ObjData* fval)
{
    RObjProbe* probe = reinterpret_cast<RObjProbe*>(
        static_cast<HSD_JObj*>(obj)->id);
    if (probe->count < 6) {
        probe->type[probe->count] = static_cast<int>(type);
        if (fval != NULL) {
            probe->value[probe->count][0] = fval->p.x;
            probe->value[probe->count][1] = fval->p.y;
            probe->value[probe->count][2] = fval->p.z;
        }
        probe->count++;
    }
}

float SelfTestExpFunc(void* args)
{
    const float* a = static_cast<const float*>(args);
    return a[0] * 2.0f + a[1];
}

bool SelfTestHsdAnim()
{
    HSD_AObjInitAllocData();
    HSD_RObjInitAllocData();

    static unsigned char stream[10] = { 0x12, 0x00, 0x00, 0x00, 0x00, 10,
                                        0x00, 0x00, 0xC8, 0x42 };
    static HSD_FObjDesc fdesc;
    fdesc.next = NULL;
    fdesc.length = sizeof(stream);
    fdesc.startframe = 0.0f;
    fdesc.type = 12;
    fdesc.frac_value = HSD_A_FRAC_FLOAT;
    fdesc.frac_slope = HSD_A_FRAC_FLOAT;
    fdesc.dummy0 = 0;
    fdesc.ad = stream;
    static HSD_AObjDesc adesc;
    adesc.flags = AOBJ_LOOP;
    adesc.end_frame = 10.0f;
    adesc.fobjdesc = &fdesc;
    adesc.obj_id = 0;

    bool aobjOk = false;
    HSD_AObj* aobj = HSD_AObjLoadDesc(&adesc);
    if (aobj != NULL) {
        AnimProbe probe = { 0, 0.0f };
        bool stepOk = aobj->flags == (AOBJ_NO_ANIM | AOBJ_LOOP);
        HSD_AObjReqAnim(aobj, 0.0f);
        for (int k = 0; k < 10; ++k) {
            probe.calls = 0;
            HSD_AObjInterpretAnim(aobj, &probe, SelfTestAnimUpdate);
            stepOk = stepOk && probe.calls == 1 &&
                     SelfTestNear(probe.value, 10.0f * k, 1e-3f);
        }
        probe.calls = 0;
        HSD_AObjInterpretAnim(aobj, &probe, SelfTestAnimUpdate);
        stepOk = stepOk && probe.calls == 1 &&
                 SelfTestNear(probe.value, 0.0f, 1e-3f) &&
                 SelfTestNear(aobj->curr_frame, 0.0f, 1e-4f) &&
                 (aobj->flags & AOBJ_REWINDED) != 0;
        HSD_AObjRemove(aobj);
        aobjOk = stepOk && HSD_ObjAllocGetUsing(HSD_AObjGetAllocData()) == 0 &&
                 HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0;
    }

    bool dobjOk = false;
    {
        HSD_DObj* d1 = HSD_DObjAlloc();
        HSD_DObj* d2 = HSD_DObjAlloc();
        if (d1 != NULL && d2 != NULL) {
            d1->next = d2;
            HSD_DObjSetFlags(d1, 0x12);
            HSD_DObjModifyFlags(d1, 0x4, 0xE);
            const bool flagsOk = HSD_DObjGetFlags(d1) == 0x14;
            const bool classOk =
                HSD_CLASS_METHOD(d1) == HSD_CLASS_INFO(&hsdDObj) &&
                hsdDObj.parent.head.nb_exist == 2;
            HSD_DObjRemoveAll(d1);
            dobjOk = flagsOk && classOk && hsdDObj.parent.head.nb_exist == 0;
        }
    }

    static HSD_JObj target;
    static HSD_JObj jobjA;
    static HSD_JObj jobjB;
    static RObjProbe robjProbe;
    memset(&target, 0, sizeof(target));
    memset(&jobjA, 0, sizeof(jobjA));
    memset(&jobjB, 0, sizeof(jobjB));
    memset(&robjProbe, 0, sizeof(robjProbe));
    PSMTXIdentity(target.mtx);
    PSMTXIdentity(jobjA.mtx);
    PSMTXIdentity(jobjB.mtx);
    target.id = reinterpret_cast<uintptr_t>(&robjProbe);
    jobjA.mtx[0][3] = 2.0f;
    jobjA.mtx[1][3] = 4.0f;
    jobjA.mtx[2][3] = 6.0f;
    jobjB.mtx[0][3] = 4.0f;
    jobjB.mtx[1][3] = 8.0f;
    jobjB.mtx[2][3] = 10.0f;

    bool robjPosOk = false;
    {
        HSD_RObj* r1 = HSD_RObjAlloc();
        HSD_RObj* r2 = HSD_RObjAlloc();
        if (r1 != NULL && r2 != NULL) {
            r1->flags = 0x80000000u | REFTYPE_JOBJ | 1;
            r1->u.jobj = &jobjA;
            r1->next = r2;
            r2->flags = 0x80000000u | REFTYPE_JOBJ | 1;
            r2->u.jobj = &jobjB;
            HSD_RObjUpdateAll(r1, &target, SelfTestRObjUpdate);
            robjPosOk = robjProbe.count == 2 && robjProbe.type[0] == 0x35 &&
                        SelfTestNear(robjProbe.value[0][0], 3.0f, 1e-4f) &&
                        SelfTestNear(robjProbe.value[0][1], 6.0f, 1e-4f) &&
                        SelfTestNear(robjProbe.value[0][2], 8.0f, 1e-4f) &&
                        robjProbe.type[1] == 0x38;
            r1->u.jobj = NULL;
            r2->u.jobj = NULL;
            r1->flags = REFTYPE_LIMIT;
            r2->flags = REFTYPE_LIMIT;
            HSD_RObjRemoveAll(r1);
        }
    }

    bool robjExpOk = false;
    {
        static HSD_RvalueList rvalues[2];
        rvalues[0].flags = 0x10 | 0x20;
        rvalues[0].joint = reinterpret_cast<HSD_Joint*>(301);
        rvalues[1].flags = 0;
        rvalues[1].joint = NULL;
        static HSD_ExpDesc expDesc;
        expDesc.func = reinterpret_cast<void*>(SelfTestExpFunc);
        expDesc.rvalue = rvalues;
        static unsigned char bytecode[] = { 0x02, 0x00, 0x00, 0x06, 0x40, 0x20,
                                            0x00, 0x00, 0x19, 0x02, 0x00, 0x01,
                                            0x17, 0x01 };
        static HSD_ByteCodeExpDesc bcDesc;
        bcDesc.bytecode = bytecode;
        bcDesc.rvalue = rvalues;
        static HSD_RObjDesc descs[2];
        descs[0].next = &descs[1];
        descs[0].flags = REFTYPE_EXP | 5;
        descs[0].u.exp = &expDesc;
        descs[1].next = NULL;
        descs[1].flags = REFTYPE_BYTECODE | 6;
        descs[1].u.bcexp = &bcDesc;

        jobjA.translate.x = 7.0f;
        jobjA.translate.y = 1.0f;
        HSD_IDInsertToTable(NULL, 301, &jobjA);
        HSD_RObj* list = HSD_RObjLoadDesc(&descs[0]);
        if (list != NULL && list->next != NULL) {
            HSD_RObjResolveRefsAll(list, descs);
            HSD_RObjSetFlags(list, 0x80000000u);
            HSD_RObjSetFlags(list->next, 0x80000000u);
            robjProbe.count = 0;
            HSD_RObjUpdateAll(list, &target, SelfTestRObjUpdate);
            robjExpOk = robjProbe.count == 2 && robjProbe.type[0] == 5 &&
                        SelfTestNear(robjProbe.value[0][0], 15.0f, 1e-3f) &&
                        robjProbe.type[1] == 6 &&
                        SelfTestNear(robjProbe.value[1][0], 18.5f, 1e-3f) &&
                        list->u.exp.nb_args == 2 &&
                        jobjA.object.ref_count_individual == 2;
            HSD_RObjRemoveAll(list);
            robjExpOk = robjExpOk &&
                        jobjA.object.ref_count_individual == 0 &&
                        HSD_ObjAllocGetUsing(HSD_RObjGetAllocData()) == 0 &&
                        HSD_ObjAllocGetUsing(HSD_RvalueObjGetAllocData()) == 0;
        }
        HSD_IDRemoveByIDFromTable(NULL, 301);
    }

    const bool utilOk = HSD_GetNbBits(0x80000001u) == 2 &&
                        HSD_Index2PosNrmMtx(5) == 15;

    return aobjOk && dobjOk && robjPosOk && robjExpOk && utilOk;
}

} // namespace

extern "C" int M360_HsdAnimSelfTest(void)
{
    return SelfTestHsdAnim() ? 1 : 0;
}
