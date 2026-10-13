#include <cassert>
#include <cstdio>
#include <cstdint>
using s32 = int;
using u32 = uint32_t;
constexpr u32 JOBJ_INSTANCE = 0x1000;
constexpr u32 CollJoint_TooFar = 0x1000;
struct HSD_JObj { u32 flags; HSD_JObj *child, *next, *parent; };
HSD_JObj* HSD_JObjGetChild(HSD_JObj* j) { return j->child; }
HSD_JObj* HSD_JObjGetNext(HSD_JObj* j) { return j->next; }
HSD_JObj* HSD_JObjGetParent(HSD_JObj* j) { return j->parent; }
struct CollJoint { HSD_JObj* x20; };
CollJoint groundCollJoint[4];
struct M360CollBind { int group; HSD_JObj* jobj; };
constexpr unsigned kMaxCollBinds = 4;
static M360CollBind s_collBinds[kMaxCollBinds];
static unsigned s_collBindCount;
struct MapCollData { int joint_count; };
static MapCollData collision = {4};
static MapCollData* s_coll = &collision;
struct GrJoint { short x, y, z; };
struct UnkStageDat_x8_t { GrJoint* unk20; int unk24; };
static UnkStageDat_x8_t descs[2];
static UnkStageDat_x8_t* s_mapHead = descs;
#define MAP_GOBJ_DESC(head, id) (&(head)[id])
struct M360GrJoint { short group, mapId, joint; };
struct M360StageDesc { const M360GrJoint* joints; int jointCount; };
void M360_MatchTrace(const char*, unsigned) {}
#include "stage_bindings_reference.h"
#include "stage_bindings_native.h"

int main()
{
    // The GObj owns a scale wrapper. Index zero is the archive model root,
    // not its first child (which may have a decorative rotation animation).
    HSD_JObj wrapper = {}, model = {}, decoration = {}, leaf = {}, sibling = {};
    wrapper.child = &model; model.parent = &wrapper;
    model.child = &decoration; decoration.parent = &model;
    decoration.child = &leaf; leaf.parent = &decoration;
    decoration.next = &sibling; sibling.parent = &model;
    for (unsigned instance = 0; instance < 4; ++instance) {
        model.flags = (instance & 1) ? JOBJ_INSTANCE : 0;
        decoration.flags = (instance & 2) ? JOBJ_INSTANCE : 0;
        for (int index = -1; index < 8; ++index) {
            groundCollJoint[0].x20 = nullptr;
            OriginalBind(0, &wrapper, index);
            assert(FindCollJoint(&wrapper, index) == groundCollJoint[0].x20);
        }
    }
    model.flags = decoration.flags = 0;
    assert(FindCollJoint(&wrapper, 0) == &model);
    assert(FindCollJoint(&model, 0) == &decoration); // Previous wrong caller.
    assert(FindCollJoint(nullptr, 0) == nullptr);

    GrJoint archive[] = {{0, 0, 1}, {1, 0, 2}};
    M360GrJoint compiled[] = {{0, 0, 0}, {2, 1, 3}};
    descs[0] = {archive, 2};
    descs[1] = {nullptr, 0};
    M360StageDesc stage = {compiled, 2};
    BindMapColl(&stage, 0, &wrapper);
    assert(s_collBindCount == 2);
    assert(s_collBinds[0].group == 0 && s_collBinds[0].jobj == &model);
    assert(s_collBinds[1].group == 1 && s_collBinds[1].jobj == &leaf);
    BindMapColl(&stage, 1, &wrapper);
    assert(s_collBindCount == 3 && s_collBinds[2].jobj == &sibling);
    AddCollBind(-1, &wrapper, 0);
    AddCollBind(4, &wrapper, 0);
    AddCollBind(3, &wrapper, 99);
    assert(s_collBindCount == 3);
    AddCollBind(3, &wrapper, 0);
    assert(s_collBindCount == 4);
    // Overrides work even when the array is full.
    AddCollBind(0, &wrapper, 1);
    assert(s_collBindCount == 4 && s_collBinds[0].jobj == &decoration);
    std::puts("PASS: original stage joint indices, instance traversal and binding overrides");
}
