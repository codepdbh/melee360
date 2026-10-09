#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using f32=float; using s32=int;
struct Vec3 { float x,y,z; };
struct CollData {
    unsigned env_flags;
    struct { Vec3 left,right; } ecb;
    struct { Vec3 normal; } right_facing_wall,left_facing_wall;
};
struct Fighter {
    Vec3 x8c_kb_vel,cur_pos,self_vel,x68C_transNPos;
    float xF0_ground_kb_vel,facing_dir;
    CollData coll_data;
    struct { struct { struct { int x4; } downreflect; } co; } mv;
    struct { float x18A8; } dmg;
    int x60C;
};
struct HSD_GObj { Fighter* user_data; }; using Fighter_GObj=HSD_GObj;
struct CommonData { float x1B0,x1BC,x1B8; } common={2,0.6f,12};
static CommonData* p_ftCommonData=&common;
enum { Collide_RightWallHug=1,Collide_LeftWallHug=2,ftCo_MS_DownReflect=335,QuakeKind_Small=2 };
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define PAD_STACK(n) ((void)0)
static unsigned effects,quakes,changes,recoveries,physics,landings;
static bool framesRemain,landed;
static Vec3 effectPosition; static float effectAngle,collisionTimer;
static void Near(float a,float b) { assert(std::fabs(a-b)<0.0001f); }
void ftKb_SpecialN_800F1F1C(HSD_GObj*,Vec3*) {}
void ftCommon_8007D5D4(Fighter*) {}
void efAsync_Spawn(HSD_GObj*,int*,int priority,int kind,void*,Vec3* pos,float* angle) {
    assert(priority==5 && kind==0x406); ++effects; effectPosition=*pos; effectAngle=*angle;
}
void Camera_RequestQuake(int kind,Vec3* pos) { assert(kind==QuakeKind_Small); ++quakes; Near(pos->x,effectPosition.x); }
void Fighter_ChangeMotionState(HSD_GObj*,int state,unsigned flags,float start,float speed,float blend,void*) {
    assert(state==335 && flags==0x18040); Near(start,0); Near(speed,1); Near(blend,0); ++changes;
}
void ftCo_80090574(HSD_GObj*) {}
void ftCommon_8007EBAC(Fighter*,int kind,int arg) { assert(kind==7 && arg==0); }
void ftColl_8007B760(HSD_GObj*,float timer) { collisionTimer=timer; }
bool ftAnim_IsFramesRemaining(HSD_GObj*) { return framesRemain; }
void ftCo_80090780(HSD_GObj*) { ++recoveries; }
void ft_80084DB0(HSD_GObj*) { ++physics; }
bool ft_80081DD4(HSD_GObj*) { return landed; }
void ftCo_80097D88(HSD_GObj*) { ++landings; }
void fn_800C7DC4(HSD_GObj*,s32,Vec3*,Vec3*);
#pragma warning(push)
#pragma warning(disable:4100)
#include "down_reflect_original.h"
#pragma warning(pop)
static Fighter MakeFighter() {
    Fighter fp={}; fp.cur_pos={10,20,0}; fp.self_vel={3,4,5}; fp.xF0_ground_kb_vel=10;
    fp.x68C_transNPos={0,3,2}; fp.coll_data.ecb.left={-4,5,0}; fp.coll_data.ecb.right={4,5,0};
    fp.coll_data.right_facing_wall.normal={1,0,0}; fp.coll_data.left_facing_wall.normal={-1,0,0};
    return fp;
}
int main() {
    Fighter fp=MakeFighter(); HSD_GObj gobj={&fp};
    for(unsigned wall : {0u,1u,2u,3u}) {
        for(float speed : {-3.0f,-2.0001f,-2.0f,0.0f,2.0f,2.0001f,3.0f}) {
            for(int prior=0;prior<=2;++prior) {
                fp=MakeFighter(); fp.coll_data.env_flags=wall; fp.x8c_kb_vel.x=speed;
                fp.mv.co.downreflect.x4=prior;
                bool expected=(speed < -2 && (wall&1) && prior!=1) || (speed > 2 && (wall&2) && prior!=2);
                unsigned before=changes;
                assert(ftCo_800C7CA0(&gobj)==expected); assert(changes==before+(expected?1:0));
                if(expected) {
                    bool right=speed<0; Near(fp.x8c_kb_vel.x,right?6.0f:-6.0f);
                    Near(fp.x8c_kb_vel.y,0); Near(fp.self_vel.x,0); Near(fp.self_vel.y,0); Near(fp.self_vel.z,0);
                    Near(fp.facing_dir,right?1.0f:-1.0f); Near(fp.dmg.x18A8,10); Near(collisionTimer,12);
                    assert(fp.mv.co.downreflect.x4==(right?1:2));
                    Near(effectPosition.x,right?6.0f:14.0f); Near(effectPosition.y,25);
                    Near(effectAngle,right?-1.57079632679f:1.57079632679f);
                    Near(fp.cur_pos.x,(wall&1)?(right?8.0f:12.0f):10.0f);
                    Near(fp.cur_pos.y,(wall&1)?20.0f:28.0f);
                    // The original remembers the reflected side and prevents repeated bounces.
                    fp.x8c_kb_vel.x=speed; assert(!ftCo_800C7CA0(&gobj));
                }
            }
        }
    }
    assert(effects==changes && quakes==changes);
    framesRemain=true; ftCo_DownReflect_Anim(&gobj); assert(recoveries==0);
    framesRemain=false; ftCo_DownReflect_Anim(&gobj); assert(recoveries==1);
    ftCo_DownReflect_IASA(&gobj); ftCo_DownReflect_Phys(&gobj); assert(physics==1);
    landed=false; ftCo_DownReflect_Coll(&gobj); assert(landings==0);
    landed=true; ftCo_DownReflect_Coll(&gobj); assert(landings==1);
    puts("PASS: original DownReflect strict thresholds, wall sides, repeat guard, velocity/position, effects and state callbacks (contacts mocked)");
}
