#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include <algorithm>
using u8 = uint8_t; using u32 = uint32_t; using s32 = int;
using f32 = float; using f64 = double;
struct Vec3 { float x, y, z; };
struct Quaternion { float x, y, z, w; };
struct HSD_JObj { Quaternion rotate; Vec3 translate; float mtx[3][4]; };
struct IKState {
    HSD_JObj *jobj0, *jobj1, *jobj2;
    Vec3 pos0, pos1, pos2, pos3, pos4;
    float len0, len1, angle_max, angle_limit;
};
struct ftData_x58_t { u8 x0, x1; float x4; u8 x8, x9; float xC; u8 x10, x11; float x18; };
struct FighterData { ftData_x58_t* x58; };
struct HSD_GObj;
struct Fighter {
    HSD_GObj* gobj; bool x2219_b5, x221F_b3; int ground_or_air;
    Vec3 x34_scale; FighterData* ft_data;
    struct { HSD_JObj* joint; } parts[7];
    unsigned short x221C_u16_y; float facing_dir;
    struct { struct { int index; Vec3 normal; } floor; } coll_data;
};
struct HSD_GObj { Fighter* user_data; HSD_JObj* hsd_obj; };
using Fighter_GObj = HSD_GObj;
struct CommonData { float x804; } common = { 20 };
static CommonData* p_ftCommonData = &common;
enum { GA_Ground = 0, GA_Air = 1, CollLine_Floor = 1 };
static int db_804D4AF8 = 1;
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define DP(type, value) ((type*)(value))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define PAD_STACK(n) ((void)0)
#define M_PI_2_F 1.57079632679f
double __frsqrte(double x) { return 1.0 / std::sqrt(x); }
static unsigned setupCalls, tiltCalls, footCalls, rotationCalls;
static float floorHeight, mockSlope;
static bool floorHit = true;
static int nextLine = -1, prevLine = -1;
static Vec3 endpoints[2] = { {0,0,0}, {10,0,0} };
static Vec3 normals[2] = { {0,1,0}, {0,1,0} };
static float rotations[2];
void HSD_JObjSetupMatrix(HSD_JObj*) { ++setupCalls; }
void HSD_JObjGetTranslation2(HSD_JObj* joint, Vec3* out) { *out = joint->translate; }
void lb_8000B1CC(HSD_JObj* joint, Vec3*, Vec3* out) { *out = joint->translate; }
Vec3* lbVector_Diff(Vec3* a, Vec3* b, Vec3* out) {
    *out = { a->x-b->x, a->y-b->y, a->z-b->z }; return out;
}
float Length(Vec3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
void lbVector_Normalize(Vec3* a) { float n=Length(*a); if(n) { a->x/=n; a->y/=n; a->z/=n; } }
void lbVector_Add(Vec3* a, Vec3* b) { a->x+=b->x; a->y+=b->y; a->z+=b->z; }
float lbVector_Angle(Vec3* a, Vec3* b) {
    float denominator=Length(*a)*Length(*b);
    return denominator ? std::acos(std::clamp((a->x*b->x+a->y*b->y+a->z*b->z)/denominator,-1.0f,1.0f)) : 0;
}
void fn_8002113C(HSD_JObj*, Vec3* axis, float angle) {
    assert(std::isfinite(angle)); assert(std::fabs(axis->z-1)<0.0001f);
    rotations[rotationCalls++ % 2] = angle;
}
void lbBgFlash_80020E38(HSD_JObj* joint, Vec3*, float, float, float) {
    ++footCalls; joint->rotate = { 9, 8, 7, 6 }; // Original caller must restore the entire quaternion.
}
int mpLib_8004DD90_Floor(int, Vec3* pos, float* height, u32*, Vec3* normal) {
    if (!floorHit) return -1;
    *height = floorHeight + mockSlope * pos->x - pos->y;
    *normal = { -mockSlope, 1, 0 }; return 0;
}
void mpFloorGetLeft(int, Vec3* out) { *out = endpoints[0]; }
void mpFloorGetRight(int, Vec3* out) { *out = endpoints[1]; }
void mpLineGetV0Pos(int, Vec3* out) { *out = endpoints[0]; }
void mpLineGetV1Pos(int, Vec3* out) { *out = endpoints[1]; }
int mpLineGetNext(int) { return nextLine; }
int mpLineGetPrev(int) { return prevLine; }
int mpLineGetKind(int) { return CollLine_Floor; }
void mpLineGetNormal(int line, Vec3* out) { *out = normals[line]; }
void ftPartSetRotX(Fighter* fp, int index, float angle) {
    ++tiltCalls; fp->parts[index].joint->rotate.x=angle;
}
#pragma warning(push)
#pragma warning(disable:4101 4244 4456)
#include "ground_pose_original.h"
#pragma warning(pop)
static void Near(float a,float b) { assert(std::fabs(a-b)<0.0001f); }
int main()
{
    ftData_x58_t feet = { 1, 4, 5, 2, 5, 4, 3, 6, 1 };
    FighterData data = { &feet }; Fighter fp = {};
    HSD_JObj joints[7] = {}; HSD_GObj gobj = { &fp, &joints[0] };
    fp.gobj=&gobj; fp.ft_data=&data; fp.x34_scale={1,1,1}; fp.facing_dir=1;
    fp.coll_data.floor.normal={-0.5f,1,0};
    for(unsigned i=0;i<7;++i) {
        fp.parts[i].joint=&joints[i]; joints[i].mtx[2][2]=1;
        joints[i].rotate={0.1f,0.2f,0.3f,0.4f};
    }
    // Authored command flags, root tilt clamping, both facings, air/dormant guards.
    ft_8008A1B8(&gobj,4); assert(fp.x221C_u16_y==4);
    Fighter_8006C5F4(&gobj); Near(joints[0].rotate.x,-0.017453292f*20);
    fp.facing_dir=-1; Fighter_8006C5F4(&gobj); Near(joints[0].rotate.x,0.017453292f*20);
    unsigned before=tiltCalls; fp.x221F_b3=true; Fighter_8006C5F4(&gobj); assert(tiltCalls==before);
    fp.x221F_b3=false; fp.ground_or_air=GA_Air; Fighter_8006C5F4(&gobj); assert(tiltCalls==before);
    fp.ground_or_air=GA_Ground; fp.x2219_b5=true; Fighter_8006C5F4(&gobj); assert(tiltCalls==before);
    fp.x2219_b5=false; ft_8008A1B8(&gobj,3); Near(joints[0].rotate.x,0);
    // Short floor segments select authored adjacent normals.
    endpoints[1]={2,0,0}; normals[0]={-0.1f,1,0}; normals[1]={-0.2f,1,0};
    nextLine=0; prevLine=1; fp.facing_dir=1; ft_8008A1B8(&gobj,4);
    Fighter_8006C5F4(&gobj); Near(joints[0].rotate.x,0.5f*(std::atan2(-0.1f,1.0f)+std::atan2(-0.2f,1.0f)));
    nextLine=prevLine=-1; endpoints[1]={10,0,0};
    // Original floor helper clamps slope correction to 0.45 and handles endpoint fallback.
    IKState ik={}; ik.pos4={2,0,0}; Vec3 normal;
    mockSlope=2; assert(fn_8008998C(&fp,&ik,&normal)); Near(ik.pos4.y,0.9f);
    ik.pos4={-2,0,0}; assert(fn_8008998C(&fp,&ik,&normal)); Near(ik.pos4.y,-0.9f);
    mockSlope=0; floorHeight=0; ik.pos4={2,0,0}; assert(!fn_8008998C(&fp,&ik,&normal));
    floorHit=false; endpoints[1]={10,1,0}; ik.pos4={3,0,0}; assert(fn_8008998C(&fp,&ik,&normal)); Near(ik.pos4.y,1);
    floorHit=true; floorHeight=0.4f;
    // Both leg chains use authored joints; the solver executes and local animation rotations survive.
    for(unsigned leg=0;leg<2;++leg) {
        unsigned start=leg?4:1;
        joints[start].translate={leg?-2.0f:2.0f,10,0};
        joints[start+1].translate={leg?-2.0f:2.0f,5,0};
        joints[start+2].translate={leg?-2.0f:2.0f,0,0};
    }
    ft_8008A1B8(&gobj,3); rotationCalls=footCalls=0;
    Fighter_8006C5F4(&gobj); assert(rotationCalls==4 && footCalls==2);
    for(unsigned i=1;i<7;++i) {
        Near(joints[i].rotate.x,0.1f); Near(joints[i].rotate.y,0.2f);
        Near(joints[i].rotate.z,0.3f); Near(joints[i].rotate.w,0.4f);
    }
    // Encounter size changes must retain both authored leg chains and finite solver angles.
    for(float scale : {0.5f, 2.0f}) {
        fp.x34_scale={scale,scale,1}; rotationCalls=footCalls=0;
        Fighter_8006C5F4(&gobj); assert(rotationCalls==4 && footCalls==2);
        for(unsigned i=1;i<7;++i) {
            Near(joints[i].rotate.x,0.1f); Near(joints[i].rotate.y,0.2f);
            Near(joints[i].rotate.z,0.3f); Near(joints[i].rotate.w,0.4f);
        }
    }
    fp.x34_scale={1,1,1};
    db_804D4AF8=0; before=footCalls; Fighter_8006C5F4(&gobj); assert(footCalls==before);
    db_804D4AF8=1;
    // Exercise the actual two-bone solver with a reachable target and finite 2D results.
    ik={}; ik.jobj0=&joints[1]; ik.jobj1=&joints[2]; ik.jobj2=&joints[3];
    ik.pos0={0,10,0}; ik.pos1={0,5,0}; ik.pos2=ik.pos3={0,0,0}; ik.pos4={2,1,0};
    ik.len0=ik.len1=5; rotationCalls=0; lbBgFlash_80021410(&ik);
    assert(rotationCalls==2 && std::isfinite(rotations[0]) && std::isfinite(rotations[1]));
    Near(ik.pos4.x,2); Near(ik.pos4.y,1); Near(ik.pos4.z,0);
    puts("PASS: original ground-pose commands, slope/adjacency limits, callback guards, foot correction, both leg chains and solver math (matrix application mocked)");
}
