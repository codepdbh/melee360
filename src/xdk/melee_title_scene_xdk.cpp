#include <xtl.h>

#include "hsdjobj_xdk_compat.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"

extern "C" {
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
}

namespace {

struct TitleModelSymbols {
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matAnim;
    HSD_ShapeAnimJoint* shapeAnim;
};

HSD_JObj* s_titleModels[2];

void CountTree(HSD_JObj* jobj, unsigned* joints, unsigned* dobjs)
{
    for (HSD_JObj* node = jobj; node && *joints < 10000; node = node->next) {
        ++*joints;
        if (!(node->flags & (JOBJ_SPLINE | JOBJ_PTCL))) {
            for (HSD_DObj* dobj = node->u.dobj; dobj; dobj = dobj->next)
                ++*dobjs;
        }
        if (!(node->flags & JOBJ_INSTANCE))
            CountTree(node->child, joints, dobjs);
    }
}

void* Resolve(const char* symbol, MeleeTitleSceneStatus* status)
{
    void* address = M360_GetHsdPublic(symbol);
    if (address)
        ++status->resolvedSymbols;
    return address;
}

} // namespace

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status)
{
    if (!status)
        return false;
    ZeroMemory(status, sizeof(*status));
    status->expectedSymbols = 12;

    TitleModelSymbols title;
    title.joint = static_cast<HSD_Joint*>(
        Resolve("TtlMoji_Top_joint", status));
    title.anim = static_cast<HSD_AnimJoint*>(
        Resolve("TtlMoji_Top_animjoint", status));
    title.matAnim = static_cast<HSD_MatAnimJoint*>(
        Resolve("TtlMoji_Top_matanim_joint", status));
    title.shapeAnim = static_cast<HSD_ShapeAnimJoint*>(
        Resolve("TtlMoji_Top_shapeanim_joint", status));

    Resolve("ScTitle_cam_int1_camera", status);
    Resolve("ScTitle_scene_lights", status);
    Resolve("ScTitle_fog", status);

    TitleModelSymbols background;
    background.joint = static_cast<HSD_Joint*>(
        Resolve("TtlBg_Top_joint", status));
    background.anim = static_cast<HSD_AnimJoint*>(
        Resolve("TtlBg_Top_animjoint", status));
    background.matAnim = static_cast<HSD_MatAnimJoint*>(
        Resolve("TtlBg_Top_matanim_joint", status));
    background.shapeAnim = static_cast<HSD_ShapeAnimJoint*>(
        Resolve("TtlBg_Top_shapeanim_joint", status));
    Resolve("TitleMark_sobjdesc", status);

    status->symbolsResolved =
        status->resolvedSymbols == status->expectedSymbols;
    if (!status->symbolsResolved || !title.joint || !background.joint)
        return false;

    s_titleModels[0] = HSD_JObjLoadJoint(title.joint);
    s_titleModels[1] = HSD_JObjLoadJoint(background.joint);
    status->modelCount = (s_titleModels[0] ? 1u : 0u) +
                         (s_titleModels[1] ? 1u : 0u);
    status->modelsLoaded = status->modelCount == 2;
    if (!status->modelsLoaded)
        return false;

    HSD_JObjAddAnimAll(s_titleModels[0], title.anim, title.matAnim,
                       title.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[1], background.anim, background.matAnim,
                       background.shapeAnim);
    HSD_JObjReqAnimAll(s_titleModels[0], 0.0f);
    HSD_JObjReqAnimAll(s_titleModels[1], 0.0f);
    HSD_JObjAnimAll(s_titleModels[0]);
    HSD_JObjAnimAll(s_titleModels[1]);
    status->animationsBound = true;

    CountTree(s_titleModels[0], &status->jointCount,
              &status->displayObjectCount);
    CountTree(s_titleModels[1], &status->jointCount,
              &status->displayObjectCount);
    return true;
}
