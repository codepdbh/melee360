#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
struct Vec3 { float x,y,z; };
struct ftECB { Vec3 top,bottom,left,right; };
typedef unsigned char u8;
typedef short s16;
typedef float f32;
struct HSD_JObj { unsigned id; };
struct Fighter_GObj;
typedef Fighter_GObj HSD_GObj;
enum { ECBSource_JObj=0, ECBSource_Fixed=1 };
struct ECBSource {
    int kind;
    HSD_JObj* x108_joint;
    HSD_JObj* x10C_joint[6];
    float x124,x128,x12C,up,down,front,back,angle;
};
struct Surface { int index; unsigned flags; Vec3 normal; };
struct CollData {
    void* x0_gobj; Vec3 cur_pos,prev_pos,last_pos,x28_vec;
    struct { unsigned b0:1,b1234:4,b5:1,b6:1,b7:1; } x34_flags;
    struct { unsigned b0:1,other:7; } x35_flags;
    int x38,floor_skip,ledge_id_right,ledge_id_left,joint_id_skip,joint_id_only;
    float facing_dir,lstick_x,x50,ledge_snap_x,ledge_snap_y,ledge_snap_height;
    unsigned env_flags,prev_env_flags,x130_flags,x13C,contact;
    ftECB x64_ecb,desired_ecb,ecb,prev_ecb,xE4_ecb;
    ECBSource ecb_source;
    Surface floor,ceiling,right_facing_wall,left_facing_wall;
};
static int mpColl_804D64AC=47;
void memzero(void* ptr,size_t count) { std::memset(ptr,0,count); }
enum GroundOrAir { GA_Ground=0, GA_Air=1 };
struct ftData_x44_t { s16 unk0,unk2,unk4,unk6,unk8,unkA;float unkC,ledge_snap_x,ledge_snap_y,ledge_snap_height; };
struct FighterBone { HSD_JObj* joint; };
struct FighterData { ftData_x44_t* x44; };
struct Fighter {
    Vec3 cur_pos; CollData coll_data; int ecb_lock;
    FighterBone* parts; FighterData* ft_data; Vec3 x34_scale;
    struct { float weight; } co_attrs;
};
struct Fighter_GObj { Fighter* user_data; };
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define DP(type, value) ((type*)(value))
static bool groundContact;
static Vec3 groundQueryPos, groundQueryLast;
bool mpColl_8004B108(CollData* coll) {
    groundQueryPos=coll->cur_pos; groundQueryLast=coll->last_pos;
    if(groundContact) coll->cur_pos.y=17;
    return groundContact;
}
#pragma warning(push)
#pragma warning(disable:4101)
#include "collision_lifecycle_original.h"
#pragma warning(pop)
static bool Equal(Vec3 a,Vec3 b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
template<class T> static bool Zero(const T& value) {
    const unsigned char* p=reinterpret_cast<const unsigned char*>(&value);
    for(size_t i=0;i<sizeof(value);++i) if(p[i]) return false;
    return true;
}
int main() {
    CollData cd; std::memset(&cd,0xA5,sizeof(cd)); cd.cur_pos={1,2,3};
    cd.x34_flags.b5=1; cd.x35_flags.other=31; cd.prev_env_flags=123;
    mpColl_80041EE4(&cd);
    assert(!cd.x0_gobj && cd.x34_flags.b0 && !cd.x34_flags.b6 && !cd.x34_flags.b7);
    assert(!cd.x34_flags.b1234 && cd.x34_flags.b5 && cd.x35_flags.b0 && cd.x35_flags.other==31);
    assert(!cd.env_flags && !cd.x130_flags && cd.prev_env_flags==123);
    assert(Equal(cd.prev_pos,cd.cur_pos) && Equal(cd.last_pos,cd.cur_pos) && Equal(cd.x28_vec,cd.cur_pos));
    assert(cd.floor_skip==-1 && cd.ledge_id_right==-1 && cd.ledge_id_left==-1);
    assert(cd.joint_id_skip==-1 && cd.joint_id_only==-1 && cd.x38==47);
    assert(cd.x50==0 && cd.ledge_snap_x==0 && cd.ledge_snap_y==0 && cd.ledge_snap_height==0);
    for(const Surface* s : {&cd.floor,&cd.ceiling,&cd.right_facing_wall,&cd.left_facing_wall}) {
        assert(s->index==-1 && s->flags==0 && s->normal.x==0 && s->normal.z==0);
    }
    assert(cd.floor.normal.y==1 && cd.ceiling.normal.y==-1);
    assert(cd.right_facing_wall.normal.y==1 && cd.left_facing_wall.normal.y==-1);
    assert(Zero(cd.ecb) && Zero(cd.prev_ecb) && Zero(cd.xE4_ecb) && Zero(cd.ecb_source));
    assert(Zero(cd.desired_ecb) && Zero(cd.x64_ecb));
    // Copy only authored fields; source owner/filter/initialization state stays local.
    CollData src={}; src.cur_pos={4,5,6}; src.prev_pos={1,2,3}; src.last_pos={7,8,9};
    src.x34_flags.b0=1; src.x34_flags.b1234=9; src.x34_flags.b5=1; src.x34_flags.b6=1;
    src.facing_dir=-1; src.x38=99; src.floor_skip=3; src.ledge_id_left=4; src.ledge_id_right=5;
    src.joint_id_skip=6; src.lstick_x=0.25f; src.env_flags=11; src.prev_env_flags=12;
    src.x130_flags=13; src.x13C=14; src.contact=15;
    src.ecb.top={10,20,30}; src.x64_ecb=src.desired_ecb=src.prev_ecb=src.xE4_ecb=src.ecb;
    src.floor={3,7,{0,1,0}}; src.ceiling={4,8,{0,-1,0}};
    src.left_facing_wall={5,9,{-1,0,0}}; src.right_facing_wall={6,10,{1,0,0}};
    for(int mode : {0,1,2}) {
        CollData dst={}; dst.x34_flags.b7=1; dst.joint_id_only=123; dst.x28_vec={8,8,8};
        dst.ecb_source.x124=987; dst.x35_flags.other=17;
        mpCopyCollData(&src,&dst,mode);
        assert(Equal(dst.cur_pos,src.cur_pos) && Equal(dst.prev_pos,src.prev_pos) && Equal(dst.last_pos,src.last_pos));
        assert(dst.x34_flags.b0==1 && dst.x34_flags.b1234==9 && dst.x34_flags.b5 && dst.x34_flags.b6);
        assert(dst.x34_flags.b7 && dst.joint_id_only==123 && Equal(dst.x28_vec,{8,8,8}));
        assert(dst.ecb_source.x124==987 && dst.x35_flags.other==17);
        assert(dst.facing_dir==-1 && dst.x38==99 && dst.floor_skip==3);
        assert(dst.ledge_id_left==4 && dst.ledge_id_right==5 && dst.joint_id_skip==6 && dst.lstick_x==0.25f);
        assert(dst.env_flags==11 && dst.prev_env_flags==12 && dst.x130_flags==13 && dst.x13C==14 && dst.contact==15);
        assert(std::memcmp(&src.ecb,&dst.ecb,sizeof(ftECB))==0);
        assert(std::memcmp(&src.desired_ecb,&dst.desired_ecb,sizeof(ftECB))==0);
        assert(std::memcmp(&src.x64_ecb,&dst.x64_ecb,sizeof(ftECB))==0);
        assert(std::memcmp(&src.prev_ecb,&dst.prev_ecb,sizeof(ftECB))==0);
        assert(std::memcmp(&src.xE4_ecb,&dst.xE4_ecb,sizeof(ftECB))==0);
        assert(dst.floor.index==3 && dst.ceiling.flags==8 && dst.left_facing_wall.index==5);
        assert(Equal(dst.right_facing_wall.normal,{1,0,0}));
    }
    Fighter fp={}; fp.cur_pos={20,18,3}; fp.coll_data.cur_pos={1,2,3};
    Fighter_GObj gobj={&fp}; groundContact=true;
    assert(ft_80082708(&gobj)==GA_Air);
    assert(Equal(groundQueryPos,{20,18,3}) && Equal(groundQueryLast,{1,2,3}));
    assert(Equal(fp.cur_pos,{20,17,3}) && Equal(fp.coll_data.cur_pos,fp.cur_pos));
    fp.cur_pos={30,16,3}; groundContact=false;
    assert(ft_80082708(&gobj)==GA_Ground);
    assert(Equal(groundQueryLast,{20,17,3}) && Equal(fp.cur_pos,{30,16,3}));
    puts("PASS: original collision lifecycle and grounded fighter position synchronization");
    HSD_JObj joints[7]={}; FighterBone bones[7];
    for(unsigned i=0;i<7;++i) { joints[i].id=i; bones[i].joint=&joints[i]; }
    ftData_x44_t authored={1,2,3,4,5,6,2,4,6,8};
    FighterData data={&authored};fp.parts=bones;fp.ft_data=&data;fp.co_attrs.weight=99;
    for(float scale : {0.5f,1.0f,2.0f,3.0f}) {
        fp.x34_scale={scale,scale,scale};fp.ecb_lock=17;
        std::memset(&fp.coll_data,0xA5,sizeof(fp.coll_data));
        ft_80081B38(&gobj);
        const CollData& setup=fp.coll_data;
        assert(fp.ecb_lock==0 && setup.x0_gobj==&gobj);
        assert(setup.x34_flags.b1234==1 && setup.ecb_source.kind==ECBSource_JObj);
        assert(setup.ecb_source.x108_joint==&joints[0]);
        for(unsigned i=0;i<6;++i) assert(setup.ecb_source.x10C_joint[i]==&joints[i+1]);
        assert(setup.ecb_source.x124==2*scale && setup.ecb_source.x128==10*scale && setup.ecb_source.x12C==10*scale);
        assert(setup.ledge_snap_x==4*scale && setup.ledge_snap_y==6*scale && setup.ledge_snap_height==8*scale && setup.x50==99);
        assert(setup.facing_dir==-1 && setup.floor.index==-1 && setup.floor_skip==-1 && setup.ceiling.index==-1);
        assert(setup.left_facing_wall.index==-1 && setup.right_facing_wall.index==-1);
        assert(Equal(setup.cur_pos,fp.cur_pos) && Equal(setup.prev_pos,fp.cur_pos) && Equal(setup.last_pos,fp.cur_pos));
        assert(setup.ecb.top.y==8 && setup.ecb.bottom.y==0 && setup.ecb.left.x==-4 && setup.ecb.right.x==4);
        for(const ftECB* buffer : {&setup.prev_ecb,&setup.xE4_ecb,&setup.x64_ecb,&setup.desired_ecb})
            assert(std::memcmp(buffer,&setup.ecb,sizeof(ftECB))==0);
        ft_80081C88(&gobj,scale*2);
        assert(setup.ecb_source.x124==4*scale && setup.ledge_snap_height==16*scale);
        // Retail uses the explicit size for snap/offset, but the fighter's
        // current scale for the two minimum-extent parameters.
        assert(setup.ecb_source.x128==10*scale && setup.ecb_source.x12C==10*scale);
    }
    puts("PASS: original fighter ECB owner, six bones, all buffers, normals, weight, ledge dimensions and size updates");
    CollData fixed={};mpColl_80041EE4(&fixed);
    mpColl_SetECBSource_Fixed(&fixed,&gobj,20,3,7,5);
    assert(fixed.x0_gobj==&gobj && fixed.ecb_source.kind==ECBSource_Fixed && fixed.facing_dir==-1);
    assert(fixed.ecb_source.up==20 && fixed.ecb_source.down==3 && fixed.ecb_source.front==7 && fixed.ecb_source.back==5);
    assert(fixed.ecb.top.y==8 && fixed.ecb.left.x==-4 && fixed.prev_ecb.right.x==4 && fixed.desired_ecb.right.y==4);
    fixed.x34_flags.b0=0;fixed.ecb.top.y=123;fixed.desired_ecb.right.x=42;
    mpColl_SetECBSource_Fixed(&fixed,&gobj,30,4,9,6);
    assert(fixed.ecb.top.y==123 && fixed.desired_ecb.right.x==42);
    puts("PASS: original fixed item ECB initializes buffers once and preserves live geometry on reconfiguration");
}
