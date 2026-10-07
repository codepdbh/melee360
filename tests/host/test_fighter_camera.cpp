#include <cassert>
#include <cstdio>
#include <cmath>
#include <initializer_list>
struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
struct UnkFloat6_Camera { Vec3 x0, xC; };
enum { CmSubjectState_Active = 0 };
struct CmSubject {
    int state;
    bool on_ledge;
    float facing_dir;
    struct Extents { Vec2 h; Vec3 v; } ext, target_ext;
    Vec3 pos, bone_pos;
};
struct FighterData { UnkFloat6_Camera* x3C; };
struct HSD_GObj;
struct Fighter {
    FighterData* ft_data;
    CmSubject* x890_cameraBox;
    Vec3 x34_scale, cur_pos;
    float facing_dir;
    bool x221F_b3;
    void (*cam_cb)(HSD_GObj*);
    struct { float model_scaling; } co_attrs;
    bool is_metal, is_always_metal;
    int metal_timer, metal_health;
};
struct HSD_JObj { Vec3 scale; };
struct HSD_GObj { Fighter* user_data; HSD_JObj* hsd_obj; };
typedef HSD_GObj Fighter_GObj;
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define GET_JOBJ(gobj) ((gobj)->hsd_obj)
#define DP(type, value) ((type*)(value))
#define HSD_ASSERTMSG(line, predicate, message) assert(predicate)
struct M360MatchStage { float camX, camY; };
static M360MatchStage testStage = { 5, 10 };
const M360MatchStage* M360_MatchStageData() { return &testStage; }
static float zoom = 1.5f;
float M360_MatchFixedZoom() { return zoom; }
float Stage_GetBlastZoneTopOffset() { return 110; }
float Stage_GetCamBoundsTopOffset() { return 60; }
static unsigned boneCalls, commonCalls, callbackCalls;
static float collisionScale;
void HSD_JObjSetScale(HSD_JObj* joint, const Vec3* scale) { joint->scale = *scale; }
void ftCo_800D105C(void*) {}
void ft_80081C88(void*, float size) { collisionScale = size; }
void ftLib_800866DC(HSD_GObj* gobj, Vec3* out) {
    ++boneCalls; *out = gobj->user_data->cur_pos; out->y += 7;
}
void ftCommon_8008021C(HSD_GObj*) { ++commonCalls; }
void TestCallback(HSD_GObj*) { assert(commonCalls == callbackCalls + 1); ++callbackCalls; }
#pragma warning(push)
#pragma warning(disable: 4552 4555)
#include "fighter_camera_original.h"
#pragma warning(pop)
static void Near(float a, float b) { assert(std::fabs(a-b) < 0.0001f); }
int main()
{
    UnkFloat6_Camera data = { { 10, 20, -15 }, { 30, -20, 5 } };
    FighterData attrs = { &data };
    CmSubject camera = {};
    camera.state = 99;
    Fighter fighter = {};
    fighter.ft_data = &attrs; fighter.x890_cameraBox = &camera;
    fighter.x34_scale = { 2, 2, 1 }; fighter.cur_pos = { 40, 100, 3 };
    fighter.facing_dir = 1; fighter.cam_cb = TestCallback;
    fighter.co_attrs.model_scaling = 1.25f;
    HSD_JObj joint = {};
    HSD_GObj gobj = { &fighter, &joint };
    ftCamera_80076064(&fighter);
    assert(camera.state == CmSubjectState_Active);
    Near(camera.pos.y, 120); Near(camera.target_ext.h.x, -30); Near(camera.target_ext.h.y, 60);
    Near(camera.target_ext.v.x, 60); Near(camera.ext.v.y, -40); Near(camera.bone_pos.y, 120);
    fighter.facing_dir = -1;
    fighter.cur_pos.y = 80;
    camera.on_ledge = true;
    ftCamera_UpdateCameraBox(&gobj);
    Near(camera.target_ext.h.x, -60); Near(camera.target_ext.h.y, 30);
    Near(camera.pos.y, 100); Near(camera.bone_pos.y, 87);
    Near(camera.ext.h.x, -30); // Current extents are eased by the camera manager.
    assert(!camera.on_ledge && boneCalls == 1);
    ftCamera_800762F4(&gobj);
    assert(boneCalls == 2);
    ftCamera_80076320(&gobj);
    Near(camera.pos.x, 20); Near(camera.pos.y, 110); // Offset-aware original ratio.
    Vec3 center;
    Stage_UnkSetVec3TCam_Offset(&center);
    Near(center.x, 5); Near(center.y, 10); Near(center.z, 0);
    for (float scale : { 0.5f, 1.0f, 2.0f }) {
        fighter.x34_scale.y = scale;
        ftCamera_80076064(&fighter);
        Near(camera.ext.v.x, 30 * scale);
        Near(camera.pos.y, 80 + 10 * scale);
    }
    Fighter_UnkCallCameraCallback_8006D9EC(&gobj);
    assert(commonCalls == 1 && callbackCalls == 1);
    fighter.x221F_b3 = true;
    Fighter_UnkCallCameraCallback_8006D9EC(&gobj);
    assert(commonCalls == 1 && callbackCalls == 1);
    fighter.x221F_b3 = false; fighter.cam_cb = nullptr;
    Fighter_UnkCallCameraCallback_8006D9EC(&gobj);
    assert(commonCalls == 2 && callbackCalls == 1);
    Fighter_UpdateModelScale(&gobj);
    Near(joint.scale.x, 2.5f); Near(joint.scale.y, 2.5f); Near(joint.scale.z, 2.5f);
    fighter.x34_scale.z = 0.75f;
    Fighter_UpdateModelScale(&gobj);
    Near(joint.scale.x, 0.75f); Near(joint.scale.y, 2.5f);
    for (float size : { 0.5f, 1.0f, 2.0f }) {
        M360_FighterSetEncounter(&gobj, size, 1);
        Near(fighter.x34_scale.x, size); Near(fighter.x34_scale.y, size);
        Near(fighter.x34_scale.z, 1); Near(collisionScale, size);
        Near(joint.scale.x, size * 1.25f); Near(joint.scale.z, size * 1.25f);
        Near(camera.ext.v.x, size * 30);
        assert(fighter.is_metal && fighter.is_always_metal);
    }
    puts("PASS: original camera extents, facing, scale, bone updates, stage offsets and callback guards/order");
}
