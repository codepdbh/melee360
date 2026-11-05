#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "match_xdk.h"
typedef unsigned u32;
typedef short s16;
struct Vec2 { float x,y; };
struct Vec3 { float x,y,z; };
struct HSD_JObj { Vec3 world; };
struct ftECB { Vec2 top,bottom,right,left; };
enum { ECBSource_JObj=1, CollData_X130_Locked=16, CollData_X130_Clear=32,
       CollisionFlagAir_CanGrabLedge=4, LINE_FLAG_EMPTY=128 };
struct ECBSource {
    int kind; HSD_JObj* x108_joint; HSD_JObj* x10C_joint[6];
    float x124,x128,x12C;
};
struct CollData {
    Vec3 cur_pos; unsigned x130_flags;
    struct { unsigned b0:1; } x34_flags;
    ftECB ecb,xE4_ecb,desired_ecb; ECBSource ecb_source;
    int joint_id_skip,joint_id_only;
    struct { s16 index; } ceiling;
};
struct Fighter { CollData coll_data; Vec3 x34_scale,prev_pos,cur_pos,self_vel,x8c_kb_vel; };
static M360MatchStage stage;
const M360MatchStage* M360_MatchStageData() { return &stage; }
int M360_MatchLineMatchesJoint(int, int skip, int only) {
    return skip!=0 && (only==-1 || only==0);
}
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
    fp.coll_data.joint_id_skip=fp.coll_data.joint_id_only=-1;
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
    fp.coll_data.ecb_source.x10C_joint[4]=&bones[4];
    fp.x34_scale.y=1;fp.coll_data.x130_flags=0;
    root.world={0,0,0};
    for(int i=0;i<6;++i) bones[i].world={0,static_cast<float>(i*2),0};
    stage.lineCount=1;stage.lines[0]={-20,12,20,12,M360_LINE_CEILING};
    fp.prev_pos={0,0,0};fp.cur_pos={0,3,0};fp.self_vel.y=2;fp.x8c_kb_vel.y=1;
    fp.coll_data.ceiling.index=-1;
    AirCeilings(&fp);
    Near(fp.cur_pos.y,2);Near(fp.self_vel.y,0);Near(fp.x8c_kb_vel.y,0);
    assert(fp.coll_data.ceiling.index==0);
    bones[5].world.y=6; // The next-highest bone is at y=8.
    fp.cur_pos.y=3;fp.self_vel.y=2;fp.x8c_kb_vel.y=1;fp.coll_data.ceiling.index=-1;
    AirCeilings(&fp);
    Near(fp.cur_pos.y,3);Near(fp.self_vel.y,2);Near(fp.x8c_kb_vel.y,1);
    assert(fp.coll_data.ceiling.index==-1);
    bones[5].world.y=10;fp.cur_pos.y=3;
    fp.coll_data.joint_id_skip=0;
    AirCeilings(&fp);Near(fp.cur_pos.y,3);assert(fp.coll_data.ceiling.index==-1);
    fp.coll_data.joint_id_skip=-1;fp.coll_data.joint_id_only=1;
    AirCeilings(&fp);Near(fp.cur_pos.y,3);assert(fp.coll_data.ceiling.index==-1);
    fp.coll_data.joint_id_only=-1;stage.lines[0].kind|=LINE_FLAG_EMPTY;
    AirCeilings(&fp);Near(fp.cur_pos.y,3);assert(fp.coll_data.ceiling.index==-1);
    stage.lines[0]={0,-20,0,20,M360_LINE_LEFT_WALL};
    fp.prev_pos={-5,0,0};fp.cur_pos={5,0,0};fp.self_vel.x=2;
    AirWalls(&fp);Near(fp.cur_pos.x,0);Near(fp.self_vel.x,0);
    fp.cur_pos.x=5;fp.self_vel.x=2;fp.coll_data.joint_id_skip=0;
    AirWalls(&fp);Near(fp.cur_pos.x,5);Near(fp.self_vel.x,2);
    fp.coll_data.joint_id_skip=-1;fp.coll_data.joint_id_only=1;
    AirWalls(&fp);Near(fp.cur_pos.x,5);Near(fp.self_vel.x,2);
    fp.coll_data.joint_id_only=-1;stage.lines[0].kind|=LINE_FLAG_EMPTY;
    AirWalls(&fp);Near(fp.cur_pos.x,5);Near(fp.self_vel.x,2);
    puts("PASS: original six-bone ECB top tracks animation, root translation and locked bottom without changing live collision state");
    puts("PASS: native ceiling contact uses the sampled animated height and preserves velocity when no contact occurs");
    puts("PASS: native air walls and ceilings reject empty lines and respect joint skip/only filters");
}
