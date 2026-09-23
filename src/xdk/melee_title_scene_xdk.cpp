#include <xtl.h>
#include <math.h>
#undef near
#undef far

#pragma warning(push, 3)
#include "gameplay_probe_compat.h"
extern "C" {
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <melee/mn/types.h>
float mn_8022ED6C(HSD_JObj*, AnimLoopSettings*);
float mn_8022F298(HSD_JObj*);
HSD_LObj* lb_80011AC4(DiscU32* list);
int lb_80011E24(HSD_JObj* root, HSD_JObj** result, ...);
}
#pragma warning(pop)

#include "hsd_render_xdk.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"

namespace {

struct TitleModelSymbols {
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matAnim;
    HSD_ShapeAnimJoint* shapeAnim;
};

enum { kLogo, kBackground, kFlash, kModelCount };

HSD_JObj* s_titleModels[kModelCount];
bool s_visible[kModelCount];
bool s_openingMode;
bool s_backgroundStarted;
AnimLoopSettings s_logoLoop = { 0, 1600.0f, 400.0f };
AnimLoopSettings s_backgroundLoop = { 0, 1330.0f, 130.0f };
HSD_FogDesc* s_fogDesc;
HSD_CObj* s_cobj;
HSD_Fog* s_fog;
HSD_LObj* s_lights;
HSD_TObj* s_firstTexture;

void HideJoint(HSD_JObj* root, int index)
{
    HSD_JObj* jobj = NULL;
    lb_80011E24(root, &jobj, index, -1);
    if (jobj)
        HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
}

void CountTree(HSD_JObj* jobj, MeleeTitleSceneStatus* status)
{
    for (HSD_JObj* node = jobj; node && status->jointCount < 10000; node = node->next) {
        ++status->jointCount;
        if (!(node->flags & (JOBJ_SPLINE | JOBJ_PTCL))) {
            for (HSD_DObj* dobj = node->u.dobj; dobj; dobj = dobj->next) {
                ++status->displayObjectCount;
                if (!dobj->mobj)
                    continue;
                ++status->materialCount;
                for (HSD_TObj* tobj = dobj->mobj->tobj; tobj; tobj = tobj->next) {
                    ++status->textureObjectCount;
                    if (!tobj->imagedesc || !tobj->imagedesc->image_ptr)
                        continue;
                    ++status->textureImageCount;
                    if (!status->firstTextureWidth) {
                        s_firstTexture = tobj;
                        status->firstTextureWidth = tobj->imagedesc->width;
                        status->firstTextureHeight = tobj->imagedesc->height;
                        status->firstTextureFormat =
                            static_cast<unsigned>(tobj->imagedesc->format);
                    }
                }
                for (HSD_PObj* pobj = dobj->pobj; pobj; pobj = pobj->next)
                    ++status->polygonObjectCount;
            }
        }
        if (!(node->flags & JOBJ_INSTANCE))
            CountTree(node->child, status);
    }
}

void* Resolve(const char* symbol, MeleeTitleSceneStatus* status)
{
    void* address = M360_GetHsdPublic(symbol);
    if (address)
        ++status->resolvedSymbols;
    return address;
}

void ResolveModel(const char* prefix, TitleModelSymbols* model,
                  MeleeTitleSceneStatus* status)
{
    char symbol[64];
    strcpy(symbol, prefix);
    strcat(symbol, "_Top_joint");
    model->joint = static_cast<HSD_Joint*>(Resolve(symbol, status));
    strcpy(symbol, prefix);
    strcat(symbol, "_Top_animjoint");
    model->anim = static_cast<HSD_AnimJoint*>(Resolve(symbol, status));
    strcpy(symbol, prefix);
    strcat(symbol, "_Top_matanim_joint");
    model->matAnim = static_cast<HSD_MatAnimJoint*>(Resolve(symbol, status));
    strcpy(symbol, prefix);
    strcat(symbol, "_Top_shapeanim_joint");
    model->shapeAnim = static_cast<HSD_ShapeAnimJoint*>(Resolve(symbol, status));
}

} // namespace

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status)
{
    if (!status)
        return false;
    ZeroMemory(status, sizeof(*status));
    status->expectedSymbols = 12;

    TitleModelSymbols title, background;
    ResolveModel("TtlMoji", &title, status);
    HSD_CameraDescPerspective* camera = static_cast<HSD_CameraDescPerspective*>(
        Resolve("ScTitle_cam_int1_camera", status));
    DiscU32* lights = static_cast<DiscU32*>(Resolve("ScTitle_scene_lights", status));
    s_fogDesc = static_cast<HSD_FogDesc*>(Resolve("ScTitle_fog", status));
    ResolveModel("TtlBg", &background, status);
    Resolve("TitleMark_sobjdesc", status);

    status->symbolsResolved = status->resolvedSymbols == status->expectedSymbols;
    if (!status->symbolsResolved || !title.joint || !background.joint)
        return false;

    s_titleModels[kLogo] = HSD_JObjLoadJoint(title.joint);
    s_titleModels[kBackground] = HSD_JObjLoadJoint(background.joint);
    s_titleModels[kFlash] = HSD_JObjLoadJoint(title.joint);
    status->modelCount = (s_titleModels[kLogo] ? 1u : 0u) +
                         (s_titleModels[kBackground] ? 1u : 0u);
    status->modelsLoaded = status->modelCount == 2 && s_titleModels[kFlash];
    if (!status->modelsLoaded)
        return false;

    s_cobj = HSD_CObjLoadDesc(reinterpret_cast<HSD_CObjDesc*>(camera));
    s_fog = HSD_FogLoadDesc(s_fogDesc);
    s_lights = lb_80011AC4(lights);

    HSD_JObjAddAnimAll(s_titleModels[kLogo], title.anim, title.matAnim, title.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[kBackground], background.anim,
                       background.matAnim, background.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[kFlash], title.anim, title.matAnim, title.shapeAnim);
    HSD_JObjReqAnimAll(s_titleModels[kFlash], s_logoLoop.loop_frame);
    HSD_JObjAnimAll(s_titleModels[kFlash]);
    Vec3 flashOffset = { 0.0f, -3.0f, 0.0f };
    HSD_JObjSetTranslate(s_titleModels[kFlash], &flashOffset);
    HideJoint(s_titleModels[kFlash], 3);
    HideJoint(s_titleModels[kFlash], 1);
    HideJoint(s_titleModels[kLogo], 7);
    M360_TitleEnter(false);
    status->animationsBound = true;

    CountTree(s_titleModels[kLogo], status);
    CountTree(s_titleModels[kBackground], status);
    return s_cobj != NULL;
}

void M360_TitleEnter(bool openingMode)
{
    s_openingMode = openingMode;
    s_backgroundStarted = !openingMode;
    s_visible[kLogo] = !openingMode;
    s_visible[kBackground] = !openingMode;
    s_visible[kFlash] = false;
    if (!s_titleModels[kLogo] || !s_titleModels[kBackground])
        return;
    HSD_JObjReqAnimAll(s_titleModels[kLogo],
                       openingMode ? s_logoLoop.start_frame : s_logoLoop.loop_frame);
    HSD_JObjAnimAll(s_titleModels[kLogo]);
    HSD_JObjReqAnimAll(s_titleModels[kBackground],
                       openingMode ? s_backgroundLoop.start_frame : 130.0f);
    HSD_JObjAnimAll(s_titleModels[kBackground]);
}

bool M360_TitleUpdate(unsigned sceneTick)
{
    if (!s_titleModels[kLogo] || !s_titleModels[kBackground])
        return false;
    if (!s_openingMode) {
        mn_8022ED6C(s_titleModels[kLogo], &s_logoLoop);
        mn_8022ED6C(s_titleModels[kBackground], &s_backgroundLoop);
        return true;
    }
    s_visible[kFlash] = sceneTick >= 0x3B6 && sceneTick < 0x3CE;
    if (sceneTick < 0x140A)
        return false;
    s_visible[kLogo] = true;
    if (sceneTick > 5400) {
        mn_8022ED6C(s_titleModels[kLogo], &s_logoLoop);
    } else {
        HSD_JObjReqAnimAll(s_titleModels[kLogo], static_cast<float>(sceneTick - 5130));
        HSD_JObjAnimAll(s_titleModels[kLogo]);
    }
    if (s_backgroundStarted) {
        mn_8022ED6C(s_titleModels[kBackground], &s_backgroundLoop);
    } else if (mn_8022F298(s_titleModels[kLogo]) >= 270.0f) {
        s_backgroundStarted = true;
        s_visible[kBackground] = true;
    }
    return s_backgroundStarted;
}

bool M360_TitleAnyVisible(void)
{
    return s_visible[kLogo] || s_visible[kBackground] || s_visible[kFlash];
}

void M360_TitleRender(void)
{
    if (!s_cobj || !M360_TitleAnyVisible() || !HSD_CObjSetCurrent(s_cobj))
        return;
    HSD_FogSet(s_fog);
    HSD_LObj_803668EC(s_lights);
    HSD_LObjSetupInit(s_cobj);
    static const u32 passes[3] = { HSD_TRSP_OPA, HSD_TRSP_TEXEDGE, HSD_TRSP_XLU };
    static const unsigned order[kModelCount] = { kBackground, kLogo, kFlash };
    for (unsigned pass = 0; pass < 3; ++pass)
        for (unsigned model = 0; model < kModelCount; ++model)
            if (s_visible[order[model]])
                HSD_JObjDispAll(s_titleModels[order[model]], NULL, passes[pass], 0);
    HSD_CObjEndCurrent();
}

unsigned M360_TitleClearColor(void)
{
    if (!s_fogDesc)
        return 0xFF000000u;
    return 0xFF000000u | (s_fogDesc->color.r << 16) | (s_fogDesc->color.g << 8) |
           s_fogDesc->color.b;
}

bool M360_DecodeFirstTitleTexture(unsigned** pixels, unsigned* width,
                                  unsigned* height)
{
    return M360_HsdDecodeTexture(s_firstTexture, pixels, width, height);
}

void M360_FreeDecodedTitleTexture(unsigned* pixels)
{
    M360_HsdFreeDecoded(pixels);
}
