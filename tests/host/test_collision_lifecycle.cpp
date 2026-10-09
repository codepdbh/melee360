#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
struct Vec3 { float x,y,z; };
struct ftECB { Vec3 top,bottom,left,right; };
struct ECBSource { unsigned data[12]; };
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
#include "collision_lifecycle_original.h"
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
        dst.ecb_source.data[0]=987; dst.x35_flags.other=17;
        mpCopyCollData(&src,&dst,mode);
        assert(Equal(dst.cur_pos,src.cur_pos) && Equal(dst.prev_pos,src.prev_pos) && Equal(dst.last_pos,src.last_pos));
        assert(dst.x34_flags.b0==1 && dst.x34_flags.b1234==9 && dst.x34_flags.b5 && dst.x34_flags.b6);
        assert(dst.x34_flags.b7 && dst.joint_id_only==123 && Equal(dst.x28_vec,{8,8,8}));
        assert(dst.ecb_source.data[0]==987 && dst.x35_flags.other==17);
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
    puts("PASS: original collision initialization, normals, filters, zeroed ECBs and selective copy in all modes");
}
