#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
typedef unsigned u32;
struct Vec2 { float x,y; };
struct Vec3 { float x,y,z; };
struct HSD_JObj { Vec3 world; };
struct ftECB { Vec2 top,bottom,right,left; };
enum { ECBSource_JObj=1, CollData_X130_Locked=16, CollData_X130_Clear=32,
       CollisionFlagAir_CanGrabLedge=4 };
struct ECBSource {
    int kind; HSD_JObj* x108_joint; HSD_JObj* x10C_joint[6];
    float x124,x128,x12C;
};
struct CollData {
    Vec3 cur_pos; unsigned x130_flags;
    struct { unsigned b0:1; } x34_flags;
    ftECB ecb,xE4_ecb,desired_ecb; ECBSource ecb_source;
};
struct Fighter { CollData coll_data; Vec3 x34_scale; };
void lb_8000B1CC(HSD_JObj* joint, void*, Vec3* pos) { *pos=joint->world; }
#define ABS(x) ((x)<0?-(x):(x))
#pragma warning(push)
#pragma warning(disable:4101)
#include "fighter_ecb_original.h"
#include "fighter_ecb_native.h"
#pragma warning(pop)
static void Near(float a,float b) { assert(std::fabs(a-b)<0.0001f); }
int main() {
    HSD_JObj root={{100,40,0}};
    HSD_JObj bones[]={{{99,41,0}},{{101,44,0}},{{98,43,0}},
                      {{102,45,0}},{{100,47,0}},{{100,50,0}}};
    Fighter fp={}; fp.x34_scale.y=1;
    fp.coll_data.ecb_source.kind=ECBSource_JObj;
    fp.coll_data.ecb_source.x108_joint=&root;
    fp.coll_data.ecb_source.x128=fp.coll_data.ecb_source.x12C=10;
    for(int i=0;i<6;++i) fp.coll_data.ecb_source.x10C_joint[i]=&bones[i];
    CollData before=fp.coll_data;
    Near(FighterCollisionTop(&fp),10);
    assert(std::memcmp(&before,&fp.coll_data,sizeof(before))==0);
    bones[5].world.y=54;
    Near(FighterCollisionTop(&fp),14);
    for(auto& b : bones) { b.world.x+=300;b.world.y-=27; }
    root.world.x+=300;root.world.y-=27;
    Near(FighterCollisionTop(&fp),14);
    fp.coll_data.x130_flags=CollData_X130_Locked;
    fp.coll_data.desired_ecb.bottom.y=20;
    Near(FighterCollisionTop(&fp),21); // Original ECB constraint keeps top above locked bottom.
    fp.coll_data.ecb_source.x10C_joint[4]=nullptr;
    fp.x34_scale.y=2;
    Near(FighterCollisionTop(&fp),24);
    puts("PASS: original six-bone ECB top tracks animation, root translation and locked bottom without changing live collision state");
}
