#include "hsdanim_xdk_compat.h"

extern "C" {
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/memory.h>
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

static void M360_MObjUpdate(void* object, enum_t type, HSD_ObjData* value)
{
    HSD_MObj* mobj = static_cast<HSD_MObj*>(object);
    if (!mobj || !mobj->mat || !value)
        return;
    switch (type) {
    case HSD_A_M_AMBIENT_R: mobj->mat->ambient.r = (u8) (255.0f * value->fv); break;
    case HSD_A_M_AMBIENT_G: mobj->mat->ambient.g = (u8) (255.0f * value->fv); break;
    case HSD_A_M_AMBIENT_B: mobj->mat->ambient.b = (u8) (255.0f * value->fv); break;
    case HSD_A_M_DIFFUSE_R: mobj->mat->diffuse.r = (u8) (255.0f * value->fv); break;
    case HSD_A_M_DIFFUSE_G: mobj->mat->diffuse.g = (u8) (255.0f * value->fv); break;
    case HSD_A_M_DIFFUSE_B: mobj->mat->diffuse.b = (u8) (255.0f * value->fv); break;
    case HSD_A_M_SPECULAR_R: mobj->mat->specular.r = (u8) (255.0f * value->fv); break;
    case HSD_A_M_SPECULAR_G: mobj->mat->specular.g = (u8) (255.0f * value->fv); break;
    case HSD_A_M_SPECULAR_B: mobj->mat->specular.b = (u8) (255.0f * value->fv); break;
    case HSD_A_M_ALPHA: mobj->mat->alpha = 1.0f - value->fv; break;
    case HSD_A_M_PE_REF0:
        if (mobj->pe) mobj->pe->ref0 = (u8) (255.0f * value->fv);
        break;
    case HSD_A_M_PE_REF1:
        if (mobj->pe) mobj->pe->ref1 = (u8) (255.0f * value->fv);
        break;
    case HSD_A_M_PE_DSTALPHA:
        if (mobj->pe) mobj->pe->dst_alpha = (u8) (255.0f * value->fv);
        break;
    }
}

static void M360_PObjUpdate(void* object, enum_t type, HSD_ObjData* value)
{
    HSD_PObj* pobj = static_cast<HSD_PObj*>(object);
    if (!pobj || !pobj->u.shape_set || !value || type < HSD_A_S_W0)
        return;
    HSD_ShapeSet* shape = pobj->u.shape_set;
    if (shape->flags & SHAPESET_ADDITIVE) {
        const unsigned index = static_cast<unsigned>(type - HSD_A_S_W0);
        if (shape->blend.bp && index < shape->nb_shape)
            shape->blend.bp[index] = value->fv;
    } else {
        shape->blend.bl = value->fv;
    }
}

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
    if (mobj && (flags & MOBJ_ANIM)) {
        HSD_AObjRemove(mobj->aobj);
        mobj->aobj = NULL;
    }
}

void HSD_MObjAddAnim(HSD_MObj* mobj, HSD_MatAnim* matanim)
{
    g_m360AnimStub.mobjAddAnim++;
    g_m360AnimStub.lastPtr = mobj;
    g_m360AnimStub.lastPtr2 = matanim;
    if (!mobj)
        return;
    HSD_AObjRemove(mobj->aobj);
    mobj->aobj = matanim ? HSD_AObjLoadDesc(matanim->aobjdesc) : NULL;
    if (matanim)
        HSD_TObjAddAnimAll(mobj->tobj, matanim->texanim);
}

void HSD_MObjReqAnimByFlags(HSD_MObj* mobj, f32 startframe, u32 flags)
{
    g_m360AnimStub.mobjReqAnimByFlags++;
    g_m360AnimStub.lastPtr = mobj;
    g_m360AnimStub.lastFrame = startframe;
    g_m360AnimStub.lastFlags = flags;
    if (mobj && (flags & MOBJ_ANIM))
        HSD_AObjReqAnim(mobj->aobj, startframe);
    if (mobj)
        HSD_TObjReqAnimAllByFlags(mobj->tobj, startframe, flags);
}

void HSD_MObjAnim(HSD_MObj* mobj)
{
    g_m360AnimStub.mobjAnim++;
    g_m360AnimStub.lastPtr = mobj;
    if (mobj)
        HSD_AObjInterpretAnim(mobj->aobj, mobj, M360_MObjUpdate);
    if (mobj)
        HSD_TObjAnimAll(mobj->tobj);
}

HSD_MObj* HSD_MObjLoadDesc(HSD_MObjDesc* mobjdesc)
{
    g_m360AnimStub.mobjLoadDesc++;
    g_m360AnimStub.lastPtr = mobjdesc;
    if (!mobjdesc)
        return NULL;
    HSD_MObj* mobj = static_cast<HSD_MObj*>(HSD_MemAlloc(sizeof(HSD_MObj)));
    if (!mobj)
        return NULL;
    memset(mobj, 0, sizeof(*mobj));
    mobj->rendermode = mobjdesc->rendermode | RENDER_TOON;
    mobj->tobj = HSD_TObjLoadDesc(mobjdesc->texdesc);
    if (mobjdesc->mat) {
        mobj->mat = static_cast<HSD_Material*>(
            HSD_MemAlloc(sizeof(HSD_Material)));
        if (mobj->mat)
            memcpy(mobj->mat, mobjdesc->mat, sizeof(HSD_Material));
    }
    if (mobjdesc->pedesc) {
        mobj->pe = static_cast<HSD_PEDesc*>(HSD_MemAlloc(sizeof(HSD_PEDesc)));
        if (mobj->pe)
            memcpy(mobj->pe, mobjdesc->pedesc, sizeof(HSD_PEDesc));
    }
    return mobj;
}

void HSD_MObjRemove(HSD_MObj* mobj)
{
    g_m360AnimStub.mobjRemove++;
    g_m360AnimStub.lastPtr = mobj;
    if (!mobj)
        return;
    HSD_AObjRemove(mobj->aobj);
    HSD_TObjRemoveAll(mobj->tobj);
    HSD_Free(mobj->pe);
    HSD_Free(mobj->mat);
    HSD_Free(mobj);
}

HSD_TObj* HSD_TObjLoadDesc(HSD_TObjDesc* desc)
{
    if (!desc)
        return NULL;
    HSD_TObj* tobj = static_cast<HSD_TObj*>(HSD_MemAlloc(sizeof(HSD_TObj)));
    if (!tobj)
        return NULL;
    memset(tobj, 0, sizeof(*tobj));
    tobj->next = HSD_TObjLoadDesc(desc->next);
    tobj->id = desc->id;
    tobj->src = desc->src;
    tobj->rotate.x = desc->rotate.x;
    tobj->rotate.y = desc->rotate.y;
    tobj->rotate.z = desc->rotate.z;
    tobj->rotate.w = 1.0f;
    tobj->scale.x = desc->scale.x;
    tobj->scale.y = desc->scale.y;
    tobj->scale.z = desc->scale.z;
    tobj->translate.x = desc->translate.x;
    tobj->translate.y = desc->translate.y;
    tobj->translate.z = desc->translate.z;
    tobj->wrap_s = desc->wrap_s;
    tobj->wrap_t = desc->wrap_t;
    tobj->repeat_s = desc->repeat_s;
    tobj->repeat_t = desc->repeat_t;
    tobj->flags = desc->blend_flags;
    tobj->blending = desc->blending;
    tobj->magFilt = desc->magFilt;
    tobj->imagedesc = desc->imagedesc;
    if (desc->tlutdesc) {
        tobj->tlut = static_cast<HSD_Tlut*>(HSD_MemAlloc(sizeof(HSD_Tlut)));
        if (tobj->tlut) {
            tobj->tlut->lut = desc->tlutdesc->lut;
            tobj->tlut->fmt = desc->tlutdesc->fmt;
            tobj->tlut->tlut_name = desc->tlutdesc->tlut_name;
            tobj->tlut->n_entries = desc->tlutdesc->n_entries;
        }
    }
    if (desc->lod) {
        tobj->lod = static_cast<HSD_TexLODDesc*>(
            HSD_MemAlloc(sizeof(HSD_TexLODDesc)));
        if (tobj->lod)
            memcpy(tobj->lod, desc->lod, sizeof(HSD_TexLODDesc));
    }
    if (desc->tev) {
        tobj->tev = static_cast<HSD_TObjTev*>(
            HSD_MemAlloc(sizeof(HSD_TObjTev)));
        if (tobj->tev)
            memcpy(tobj->tev, desc->tev, sizeof(HSD_TObjTev));
    }
    PSMTXIdentity(tobj->mtx);
    return tobj;
}

void HSD_TObjRemoveAnimAll(HSD_TObj* tobj)
{
    for (; tobj; tobj = tobj->next) {
        HSD_AObjRemove(tobj->aobj);
        tobj->aobj = NULL;
    }
}

void HSD_TObjAddAnimAll(HSD_TObj* tobj, HSD_TexAnim* texanim)
{
    // Texture animations are keyed by GX texture-map ID, not list position.
    // Menu labels often have several TObjs on one material and the positional
    // pairing silently bound their frame tables to the wrong texture.
    for (; tobj; tobj = tobj->next) {
        HSD_TexAnim* match = texanim;
        while (match && match->id != tobj->id)
            match = match->next;
        HSD_AObjRemove(tobj->aobj);
        tobj->aobj = match && match->aobjdesc
                         ? HSD_AObjLoadDesc(match->aobjdesc)
                         : NULL;
        tobj->imagetbl = match ? match->imagetbl : NULL;
        tobj->n_imagetbl = match ? match->n_imagetbl : 0;
    }
}

static void M360_TObjUpdate(void* object, enum_t type, HSD_ObjData* value)
{
    HSD_TObj* tobj = static_cast<HSD_TObj*>(object);
    if (!tobj || !value)
        return;
    switch (type) {
    case HSD_A_T_TIMG: {
        const int frame = static_cast<int>(value->fv);
        if (tobj->imagetbl && frame >= 0 && frame < tobj->n_imagetbl &&
            tobj->imagetbl[frame].v)
            tobj->imagedesc = reinterpret_cast<HSD_ImageDesc*>(
                static_cast<uintptr_t>(tobj->imagetbl[frame].v));
        break;
    }
    case HSD_A_T_BLEND: case HSD_A_T_TS_BLEND:
        tobj->blending = value->fv;
        break;
    case HSD_A_T_TRAU: tobj->translate.x = value->fv; break;
    case HSD_A_T_TRAV: tobj->translate.y = value->fv; break;
    case HSD_A_T_SCAU: tobj->scale.x = value->fv; break;
    case HSD_A_T_SCAV: tobj->scale.y = value->fv; break;
    case HSD_A_T_ROTX: tobj->rotate.x = value->fv; break;
    case HSD_A_T_ROTY: tobj->rotate.y = value->fv; break;
    case HSD_A_T_ROTZ: tobj->rotate.z = value->fv; break;
    }
}

void HSD_TObjReqAnimAllByFlags(HSD_TObj* tobj, f32 frame, u32 flags)
{
    if (!(flags & TOBJ_ANIM))
        return;
    for (; tobj; tobj = tobj->next)
        HSD_AObjReqAnim(tobj->aobj, frame);
}

void HSD_TObjAnimAll(HSD_TObj* tobj)
{
    for (; tobj; tobj = tobj->next)
        HSD_AObjInterpretAnim(tobj->aobj, tobj, M360_TObjUpdate);
}

void HSD_TObjRemoveAll(HSD_TObj* tobj)
{
    while (tobj) {
        HSD_TObj* next = tobj->next;
        HSD_AObjRemove(tobj->aobj);
        HSD_Free(tobj->tev);
        HSD_Free(tobj->lod);
        HSD_Free(tobj->tlut);
        HSD_Free(tobj);
        tobj = next;
    }
}

void HSD_PObjRemoveAnimAllByFlags(HSD_PObj* pobj, u32 flags)
{
    g_m360AnimStub.pobjRemoveAnimAllByFlags++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastFlags = flags;
    for (HSD_PObj* node = pobj; node; node = node->next) {
        if ((flags & POBJ_ANIM) && pobj_type(node) == POBJ_SHAPEANIM &&
            node->u.shape_set) {
            HSD_AObjRemove(node->u.shape_set->aobj);
            node->u.shape_set->aobj = NULL;
        }
    }
}

void HSD_PObjAddAnimAll(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
    g_m360AnimStub.pobjAddAnimAll++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastPtr2 = shapeanim;
    for (HSD_PObj* node = pobj; node && shapeanim;
         node = node->next, shapeanim = shapeanim->next) {
        if (pobj_type(node) == POBJ_SHAPEANIM && node->u.shape_set) {
            HSD_AObjRemove(node->u.shape_set->aobj);
            node->u.shape_set->aobj =
                HSD_AObjLoadDesc(shapeanim->aobjdesc);
        }
    }
}

void HSD_PObjReqAnimAllByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
    g_m360AnimStub.pobjReqAnimAllByFlags++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastFrame = startframe;
    g_m360AnimStub.lastFlags = flags;
    for (HSD_PObj* node = pobj; node; node = node->next) {
        if ((flags & POBJ_ANIM) && pobj_type(node) == POBJ_SHAPEANIM &&
            node->u.shape_set)
            HSD_AObjReqAnim(node->u.shape_set->aobj, startframe);
    }
}

void HSD_PObjAnimAll(HSD_PObj* pobj)
{
    g_m360AnimStub.pobjAnimAll++;
    g_m360AnimStub.lastPtr = pobj;
    for (HSD_PObj* node = pobj; node; node = node->next) {
        if (pobj_type(node) == POBJ_SHAPEANIM && node->u.shape_set)
            HSD_AObjInterpretAnim(node->u.shape_set->aobj, node,
                                  M360_PObjUpdate);
    }
}

HSD_PObj* HSD_PObjLoadDesc(HSD_PObjDesc* pobjdesc)
{
    g_m360AnimStub.pobjLoadDesc++;
    g_m360AnimStub.lastPtr = pobjdesc;
    if (!pobjdesc)
        return NULL;
    HSD_PObj* pobj = static_cast<HSD_PObj*>(HSD_MemAlloc(sizeof(HSD_PObj)));
    if (!pobj)
        return NULL;
    memset(pobj, 0, sizeof(*pobj));
    pobj->next = HSD_PObjLoadDesc(pobjdesc->next);
    pobj->verts = pobjdesc->verts;
    pobj->flags = pobjdesc->flags;
    pobj->n_display = pobjdesc->n_display;
    pobj->display = pobjdesc->display;
    if (pobj_type(pobj) == POBJ_SHAPEANIM && pobjdesc->u.shape_set) {
        HSD_ShapeSetDesc* source = pobjdesc->u.shape_set;
        HSD_ShapeSet* shape = static_cast<HSD_ShapeSet*>(
            HSD_MemAlloc(sizeof(HSD_ShapeSet)));
        if (shape) {
            memset(shape, 0, sizeof(*shape));
            shape->flags = source->flags;
            shape->nb_shape = source->nb_shape;
            shape->nb_vertex_index = source->nb_vertex_index;
            shape->vertex_desc = source->vertex_desc;
            shape->vertex_idx_list = source->vertex_idx_list;
            shape->nb_normal_index = source->nb_normal_index;
            shape->normal_desc = source->normal_desc;
            shape->normal_idx_list = source->normal_idx_list;
            if (shape->flags & SHAPESET_ADDITIVE) {
                shape->blend.bp = static_cast<f32*>(
                    HSD_MemAlloc(shape->nb_shape * sizeof(f32)));
                if (shape->blend.bp)
                    memset(shape->blend.bp, 0,
                           shape->nb_shape * sizeof(f32));
            }
            pobj->u.shape_set = shape;
        }
    }
    return pobj;
}

void HSD_PObjRemoveAll(HSD_PObj* pobj)
{
    g_m360AnimStub.pobjRemoveAll++;
    g_m360AnimStub.lastPtr = pobj;
    while (pobj) {
        HSD_PObj* next = pobj->next;
        if (pobj_type(pobj) == POBJ_SHAPEANIM && pobj->u.shape_set) {
            HSD_AObjRemove(pobj->u.shape_set->aobj);
            if (pobj->u.shape_set->flags & SHAPESET_ADDITIVE)
                HSD_Free(pobj->u.shape_set->blend.bp);
            HSD_Free(pobj->u.shape_set);
        }
        HSD_Free(pobj);
        pobj = next;
    }
}

void HSD_PObjResolveRefsAll(HSD_PObj* pobj, HSD_PObjDesc* desc)
{
    g_m360AnimStub.pobjResolveRefsAll++;
    g_m360AnimStub.lastPtr = pobj;
    g_m360AnimStub.lastPtr2 = desc;
    for (; pobj && desc; pobj = pobj->next, desc = desc->next) {
        if (pobj_type(pobj) == POBJ_SKIN && desc->u.joint) {
            pobj->u.jobj = static_cast<HSD_JObj*>(
                HSD_IDGetDataFromTable(NULL,
                    reinterpret_cast<uintptr_t>(desc->u.joint), NULL));
        }
    }
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
