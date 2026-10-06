#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef unsigned u32;
typedef short s16;
struct Vec3 { float x, y, z; };
struct ftData_x44_t { s16 unk0, unk2, unk4, unk6, unk8, unkA; };
struct Fighter_GObj;
typedef int FighterKind;
struct DiscU32 { uintptr_t v; };
struct Fighter_804D6540_x0_t { unsigned char x0, x1, x2, x3; };
struct Fighter_804D6540_t { Fighter_804D6540_x0_t* x0; int x4; };
DiscU32* Fighter_804D6540;
#define DP(type, pointer) ((type*) (pointer))
enum { kMaxJoints = 128 };
struct HSD_JObj { HSD_JObj* parent; HSD_JObj* child; HSD_JObj* next; unsigned dirty; };
struct HSD_Joint { unsigned flags; HSD_Joint* child; HSD_Joint* next; };
enum { JOBJ_INSTANCE = 0x1000 };
void HSD_JObjSetMtxDirty(HSD_JObj* joint) { ++joint->dirty; }
struct Bone { void* joint; void* x4_jobj2; bool flags_b1; };
struct HSD_AObj { float rate, frame; };
struct HSD_TObj { HSD_TObj* next; HSD_AObj* aobj; unsigned updates; };
struct HSD_MObj { HSD_TObj* tobj; };
struct HSD_DObj { HSD_MObj* mobj; };
struct DiscU16 { unsigned short v; };
struct ftData_x8_x8 { unsigned x8; DiscU32* xC; };
struct ftData_x8 { ftData_x8_x8 x8; };
struct FighterData { ftData_x8* x8; ftData_x44_t* x44; };
struct CostumeTObjList { unsigned n_costume_tobjs; DiscU16* x5D0; HSD_TObj* costume_tobjs[5]; };
struct FtPartsVisLookup { unsigned value; };
struct FtPartsDesc { unsigned model_num; void* vis_table; };
struct FtPartsVis { unsigned model_num; FtPartsVisLookup* xC[5]; bool cleared[5]; };
struct Fighter { FighterData* ft_data; unsigned x619_costume_id, player_id, kind; CostumeTObjList tobj_list; bool x221E_b7; FtPartsVis x5AC; Fighter_GObj* gobj; Bone* parts; Vec3 x34_scale; bool no_normal_motion, is_sandbag; float x2DC, x2E0, x2E4, x2E8, x2EC; };
struct FigaTree { float frames; };
FigaTree animationTrees[64];
unsigned animationQueries;
FigaTree* ftData_80085E50(Fighter* fighter, int animation)
{
    assert(fighter && animation >= 0 && animation < 64);
    ++animationQueries;
    return &animationTrees[animation];
}
float lbAnim_8001E8F8(FigaTree* tree) { return tree ? tree->frames : 0.0f; }
struct Fighter_GObj { Fighter* user_data; };
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
struct TextureCallbacks { void (*x0)(Fighter_GObj*); void (*x4)(Fighter_GObj*, int, float); };
TextureCallbacks ftData_UnkCallbackPairs0[2];
unsigned textureMisses;
unsigned textureFrameCallbacks, textureResetCallbacks;
void TextureFrameCallback(Fighter_GObj* object, int index, float frame)
{
    assert(object && index == 0 && frame == 3);
    ++textureFrameCallbacks;
}
void TextureResetCallback(Fighter_GObj* object)
{
    assert(object);
    ++textureResetCallbacks;
}
void M360_MatchTrace(const char*, unsigned) { ++textureMisses; }
void HSD_AObjSetRate(HSD_AObj* animation, float rate) { animation->rate = rate; }
void HSD_AObjReqAnim(HSD_AObj* animation, float frame) { animation->frame = frame; }
void HSD_TObjAnim(HSD_TObj* texture) { ++texture->updates; }
typedef unsigned char arg_t;
enum { CpuCmd_SetLstickX = 0x80, CpuCmd_SetLstickY = 0x81,
       CpuCmd_WaitFor = 0x8E, CpuCmd_LstickXForward = 0x91, CpuCmd_Done = 0x7F };
unsigned cpuCommands[9][2], cpuCommandCount;
void ftCo_800B46B8(Fighter* fighter, unsigned command, arg_t argument)
{
    assert(fighter && cpuCommandCount < 9);
    cpuCommands[cpuCommandCount][0] = command;
    cpuCommands[cpuCommandCount++][1] = argument;
}
void ftCo_800B463C(Fighter* fighter, unsigned command)
{
    ftCo_800B46B8(fighter, command, 0);
}
struct M360Fighter {
    Fighter fighter;
    void* joints[kMaxJoints];
    unsigned jointParts[kMaxJoints];
    unsigned jointCount;
    unsigned partCount;
    Bone parts[kMaxJoints];
    HSD_DObj* dobjs[kMaxJoints];
    unsigned dobjCount;
    FtPartsVisLookup* vis[5];
};
struct ColorOverlay {
    void* x8_ptr1;
    int x4_pri;
    union { void* ptr; int i; } x28_colanim;
    bool x7C_color_enable;
    bool x7C_flag2;
};
M360Fighter* Owner(Fighter_GObj* object) { return (M360Fighter*) object->user_data; }
void lb_8000B1CC(void* joint, Vec3*, Vec3* out) { *out = *(Vec3*) joint; }
#include "fighter_parts_original.h"

int main()
{
    for (unsigned type = 0; type < 4; ++type) {
        HSD_JObj parent = {}, root = {}, old = {}, sibling = {}, inserted = {};
        root.parent = &parent;
        old.parent = type < 2 ? &root : &parent;
        sibling.parent = old.parent;
        old.next = &sibling;
        if (type < 2) root.child = &old; else root.next = &old;
        AttachReservedJoint(type, &root, &inserted);
        assert((type < 2 ? root.child : root.next) == &inserted);
        assert(inserted.parent == (type < 2 ? &root : &parent));
        if (type == 0 || type == 2) {
            assert(inserted.child == &old && !inserted.next);
            assert(old.parent == &inserted && sibling.parent == &inserted);
        } else {
            assert(inserted.next == &old && !inserted.child);
            assert(old.parent == inserted.parent && sibling.parent == inserted.parent);
        }
        assert(root.dirty && inserted.dirty);
    }
    {
        HSD_Joint root = {}, child = {}, sibling = {};
        root.child = &child; child.next = &sibling;
        unsigned index = 2;
        assert(ReservedJointDescriptor(&root, &index) == &sibling);
        index = 3; assert(!ReservedJointDescriptor(&root, &index));
        root.flags = JOBJ_INSTANCE;
        index = 1; assert(!ReservedJointDescriptor(&root, &index));
    }
    puts("PASS: reserved fighter joints preserve all four attachment relationships and descriptor traversal");
    {
        Fighter fighter = {};
        animationTrees[0x23].frames = 20.0f;
        animationTrees[7].frames = 30.0f;
        animationTrees[8].frames = 24.0f;
        animationTrees[9].frames = 18.0f;
        animationTrees[0x25].frames = 40.0f;
        animationQueries = 0;
        SetupAnimationLengths(&fighter);
        assert(animationQueries == 5);
        assert(fighter.x2EC == 20.0f && fighter.x2DC == 30.0f);
        assert(fighter.x2E0 == 24.0f && fighter.x2E4 == 18.0f && fighter.x2E8 == 40.0f);
        const float rate = (0.1f + fighter.x2EC) / 30.0f;
        unsigned frames = 0;
        for (float progress = 0; progress < fighter.x2EC; progress += rate) ++frames;
        assert(frames == 30);
        fighter.no_normal_motion = true; animationQueries = 0;
        SetupAnimationLengths(&fighter);
        assert(!animationQueries && fighter.x2EC == 20.0f);
        fighter.no_normal_motion = false; fighter.is_sandbag = true;
        fighter.x2DC = fighter.x2E0 = fighter.x2E4 = fighter.x2E8 = 0;
        SetupAnimationLengths(&fighter);
        assert(animationQueries == 1 && fighter.x2EC == 20.0f && fighter.x2E8 == 0);
        puts("PASS: original landing, walking and shield animation lengths initialize with boss/sandbag guards");
    }
    {
        M360Fighter subject = {};
        Fighter_GObj object = { &subject.fighter };
        ftData_x44_t authored = { 1, 2, 3, 4, 5, 6 };
        FighterData data = { 0, &authored };
        Vec3 points[7] = { {0,100,0}, {0,104,0}, {0,109,0}, {0,108,0},
                           {0,102,0}, {0,106,0}, {0,105,0} };
        subject.fighter.gobj = &object; subject.fighter.ft_data = &data;
        subject.fighter.parts = subject.parts; subject.fighter.x34_scale.y = 1;
        subject.partCount = 7;
        for (unsigned i = 0; i < 7; ++i) subject.parts[i].joint = &points[i];
        assert(FighterCollisionTop(&subject.fighter) == 11);
        authored.unk4 = 127; authored.unk6 = -1;
        assert(FighterCollisionTop(&subject.fighter) == 11);
        subject.fighter.ft_data = 0; subject.fighter.x34_scale.y = 2;
        assert(FighterCollisionTop(&subject.fighter) == 24);
        puts("PASS: ceiling height uses authored ECB bones, relative origin and scaled fallback");
    }
    Fighter_804D6540_x0_t reserved[] = {
        {7,6,1,7}, {8,7,0,8}, {9,8,0,9}, {10,9,0,10},
        {28,12,2,22}, {29,28,0,23}, {30,29,0,24}, {31,30,0,25},
        {32,31,0,26}, {33,32,0,27}, {34,33,0,28}, {15,14,3,11}, {16,15,3,12}
    };
    Fighter_804D6540_t table = {reserved, 13};
    DiscU32 tables[] = {{0}, {(uintptr_t) &table}};
    M360Fighter fighter = {};
    Fighter_804D6540 = tables;
    fighter.jointCount = 46;
    for (unsigned joint = 0; joint < fighter.jointCount; ++joint)
        fighter.joints[joint] = (void*) (uintptr_t) (joint + 1);
    assert(SetupFighterParts(&fighter, 1));
    assert(fighter.partCount == 59);
    unsigned hurtBones[] = {5, 42, 37, 55, 49};
    for (unsigned hurt = 0; hurt < 5; ++hurt)
        assert(fighter.parts[hurtBones[hurt]].joint);
    assert(fighter.parts[55].joint == fighter.joints[42]);
    for (unsigned entry = 0; entry < 13; ++entry) {
        assert(!fighter.parts[reserved[entry].x0].joint);
        assert(ftParts_8007506C(1, reserved[entry].x0) == (1u << entry));
    }
    fighter = M360Fighter();
    fighter.jointCount = 128;
    assert(SetupFighterParts(&fighter, 0));
    assert(fighter.partCount == 128);
    assert(!SetupFighterParts(&fighter, 1));
    Fighter_804D6540 = 0;
    assert(ftParts_8007506C(1, 7) == 0);
    ColorOverlay overlay;
    memset(&overlay, 0xCC, sizeof(overlay));
    lb_80014498(&overlay);
    assert(!overlay.x8_ptr1 && !overlay.x4_pri);
    assert(!overlay.x28_colanim.ptr && !overlay.x28_colanim.i);
    assert(!overlay.x7C_color_enable && !overlay.x7C_flag2);
    lb_80014498(&overlay);
    assert(!overlay.x28_colanim.i);
    puts("PASS: original Kirby reserved bones, hurtboxes, dense skeletons and bounds");
    puts("PASS: recycled item color overlays reset their table index and flags");
    HSD_AObj animations[3] = {{1,0},{1,0},{1,0}};
    HSD_TObj textures[3] = {{&textures[1], &animations[0],0}, {0,&animations[1],0}, {0,&animations[2],0}};
    HSD_MObj materials[2] = {{&textures[0]}, {&textures[2]}};
    HSD_DObj displays[3] = {{&materials[0]}, {0}, {&materials[1]}};
    DiscU16 indices[] = {{2}, {0}};
    DiscU32 costumes[] = {{(uintptr_t) indices}, {0}};
    ftData_x8 partsData = {{2, costumes}};
    FighterData data = {&partsData};
    fighter = M360Fighter();
    fighter.fighter.ft_data = &data;
    fighter.fighter.x619_costume_id = 1;
    fighter.dobjCount = 3;
    for (unsigned display = 0; display < 3; ++display)
        fighter.dobjs[display] = &displays[display];
    assert(SetupCostumeTextures(&fighter));
    assert(fighter.fighter.tobj_list.n_costume_tobjs == 2);
    assert(fighter.fighter.tobj_list.costume_tobjs[0] == &textures[2]);
    assert(fighter.fighter.tobj_list.costume_tobjs[1] == &textures[0]);
    assert(animations[2].rate == 0 && animations[0].rate == 0 && animations[1].rate == 1);
    Fighter_GObj object = {&fighter.fighter};
    ftData_UnkCallbackPairs0[0].x4 = TextureFrameCallback;
    ftData_UnkCallbackPairs0[0].x0 = TextureResetCallback;
    ftAnim_800704F0(&object, 0, 3);
    assert(animations[2].frame == 3 && textures[2].updates == 1 && fighter.fighter.x221E_b7);
    ftAnim_80070654(&object);
    assert(animations[2].frame == 0 && textures[2].updates == 2 && !fighter.fighter.x221E_b7);
    assert(textures[0].updates == 1);
    assert(textureFrameCallbacks == 1 && textureResetCallbacks == 1);
    ftAnim_80070458(&fighter.fighter, &fighter.fighter.tobj_list, 5, 8);
    assert(textureMisses == 1 && textures[2].updates == 2);
    indices[0].v = 99;
    assert(!SetupCostumeTextures(&fighter));
    assert(!fighter.fighter.tobj_list.n_costume_tobjs);
    indices[0].v = 2;
    textures[2].aobj = 0;
    assert(!SetupCostumeTextures(&fighter));
    partsData.x8.x8 = 6;
    assert(!SetupCostumeTextures(&fighter));
    partsData.x8.x8 = 0;
    assert(SetupCostumeTextures(&fighter));
    puts("PASS: costume texture mapping/fallback, frame changes, reset and invalid descriptors");
    ftCo_800A0098(&fighter.fighter);
    const unsigned expectedCommands[9][2] = {
        {0x80,0}, {0x81,0}, {0x8E,1}, {0x91,0xB0}, {0x8E,1},
        {0x81,0}, {0x8E,10}, {0x80,0}, {0x7F,0}
    };
    assert(cpuCommandCount == 9);
    assert(memcmp(cpuCommands, expectedCommands, sizeof(cpuCommands)) == 0);
    puts("PASS: retail CPU spacing recovery command sequence and signed stick argument");
    FtPartsVisLookup lookups[3] = {{1}, {2}, {3}};
    DiscU32 visibility[2][4] = {{{(uintptr_t)&lookups[0]}, {(uintptr_t)&lookups[1]}, {0}, {0}},
                              {{{0}}, {(uintptr_t)&lookups[2]}, {0}, {0}}};
    FtPartsDesc visibilityDesc = {2, visibility};
    SetupCostumeVisibility(&fighter, &visibilityDesc, 1);
    assert(fighter.fighter.x5AC.model_num == 2);
    assert(fighter.vis[0] == &lookups[0] && fighter.vis[1] == &lookups[2]);
    for (unsigned group = 0; group < 5; ++group) {
        assert(fighter.fighter.x5AC.xC[group] == fighter.vis[group]);
        assert(fighter.fighter.x5AC.cleared[group]);
    }
    visibilityDesc.vis_table = 0;
    SetupCostumeVisibility(&fighter, &visibilityDesc, 0);
    assert(!fighter.fighter.x5AC.xC[0] && !fighter.vis[0]);
    puts("PASS: native costume visibility tables, fallback and Yoshi OnLoad descriptors");
}
