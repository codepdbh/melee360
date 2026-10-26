#include <cassert>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "match_xdk.h"
typedef unsigned u32;
typedef unsigned char u8;
typedef ptrdiff_t ssize_t;
struct Vec3 { float x,y,z; };
struct Fighter_GObj {};
typedef Fighter_GObj HSD_GObj;
struct MapJoint {
    int floor_start,floor_count,ceiling_start,ceiling_count;
    int right_wall_start,right_wall_count,left_wall_start,left_wall_count;
    int dynamic_start,dynamic_count;
} groups[2];
struct RefMap { int v0_idx,v1_idx; u32 lo_flags; } maps[3];
struct CollLine { RefMap* x0; u32 flags; } groundCollLine[3];
struct Vertex { Vec3 pos; } groundCollVtx[6];
struct CollJoint { unsigned flags; CollJoint* next; MapJoint* inner; } groundCollJoint[2];
static CollJoint* jointListStart=groundCollJoint;
enum { CollLine_Floor=1,LINE_FLAG_EMPTY=1<<7,LINE_FLAG_ENABLED=1<<16,CollJoint_TooFar=1 };
static M360MatchStage stage;
static int prevLine[3],nextLine[3];
const M360MatchStage* M360_MatchStageData() { return &stage; }
unsigned M360_MatchMapJointCount() { return 2; }
const MapJoint* M360_MatchMapJoint(unsigned i) { return &groups[i]; }
bool mpLib_80054ED8(int i) { return i>=0 && i<3 && stage.lines[i].kind; }
int mpLineGetPrev(int i) { return prevLine[i]; }
int mpLineGetNext(int i) { return nextLine[i]; }
CollLine* mpLineGetCollLine(int i) { return &groundCollLine[i]; }
bool mpCheckedBounding() { return false; }
void mpBoundingCheck2(float,float,float,float) {}
void mpUncheckBounding() {}
void PSVECNormalize(const Vec3* in,Vec3* out) {
    const float length=std::sqrt(in->x*in->x+in->y*in->y+in->z*in->z);
    *out={in->x/length,in->y/length,in->z/length};
}
#define LINEID_CHECK(n,i) assert(mpLib_80054ED8(i))
#define PAD_STACK(n)
#define SQ(x) ((x)*(x))
#define ABS(x) ((x)<0?-(x):(x))
#define F32_MAX FLT_MAX
#pragma warning(push)
#pragma warning(disable:4101 4244 4701)
#include "floor_query_reference.h"
#include "collision_projection.h"
#include "collision_floor_query.h"
#pragma warning(pop)
struct CollData {
    Vec3 cur_pos;
    struct { Vec3 bottom; } ecb;
    struct { int index;u32 flags;Vec3 normal; } floor;
    int floor_skip,joint_id_skip,joint_id_only;
};
void mpFloorGetLeft(int i,Vec3* out) { auto& l=stage.lines[i];*out={l.x0,l.y0,0}; }
void mpFloorGetRight(int i,Vec3* out) { auto& l=stage.lines[i];*out={l.x1,l.y1,0}; }
#pragma warning(push)
#pragma warning(disable:4101)
#include "floor_landing_native.h"
#pragma warning(pop)
static void Near(float a,float b) { assert(std::isfinite(a) && std::isfinite(b) && std::fabs(a-b)<0.00001f); }
static std::vector<int> callbackLines;
static int rejectedLine=-1;
bool Filter(Fighter_GObj*,int i) { callbackLines.push_back(i);return i!=rejectedLine; }
static void Configure(int variant) {
    stage.lineCount=3;
    float slope=variant==1?0.5f:variant==2?-0.5f:0;
    stage.lines[0]={-20,-20*slope,0,0,M360_LINE_FLOOR,101};
    stage.lines[1]={0,0,20,20*slope,M360_LINE_FLOOR,102};
    stage.lines[2]={-15,8,15,8,M360_LINE_FLOOR,103};
    if(variant==3) stage.lines[1].kind=0;
    if(variant==4) stage.lines[2].kind|=LINE_FLAG_EMPTY;
    groups[0]={};groups[1]={};
    groups[0].floor_count=2;groups[1].dynamic_start=variant==5?0:2;
    groups[1].dynamic_count=variant==5?3:1;
    prevLine[0]=-1;nextLine[0]=1;prevLine[1]=0;nextLine[1]=-1;prevLine[2]=nextLine[2]=-1;
    for(int i=0;i<3;++i) {
        const auto& l=stage.lines[i];maps[i]={2*i,2*i+1,l.flags};
        groundCollLine[i]={&maps[i],l.kind?l.kind|LINE_FLAG_ENABLED:CollLine_Floor};
        groundCollVtx[2*i].pos={l.x0,l.y0,0};groundCollVtx[2*i+1].pos={l.x1,l.y1,0};
    }
    for(int i=0;i<2;++i) { groundCollJoint[i]={0,i==0?&groundCollJoint[1]:NULL,&groups[i]}; }
}
int main() {
    unsigned comparisons=0;Fighter_GObj owner;
    for(int v=0;v<6;++v) {
        Configure(v);
        for(int x=-25;x<=25;x+=5) for(int y=-5;y<=15;y+=5)
        for(int dx=-18;dx<=18;dx+=18) for(int dy=-15;dy<=15;dy+=15)
        for(int skip=-1;skip<3;++skip) for(int only=-1;only<3;++only)
        for(int offset=-2;offset<=2;offset+=2) for(int filter=0;filter<2;++filter)
        for(int mask=0;mask<8;++mask) {
            Vec3 a={91,92,93},b=a,na={81,82,83},nb=na;int ia=77,ib=77;u32 fa=88,fb=88;
            rejectedLine=filter?0:-1;callbackLines.clear();
            auto cb=filter?Filter:NULL;
            bool ha=mpCheckFloor(static_cast<float>(x),static_cast<float>(y),static_cast<float>(x+dx),static_cast<float>(y+dy),static_cast<float>(offset),mask&1?&a:NULL,&ia,mask&2?&fa:NULL,mask&4?&na:NULL,skip,-1,only,cb,&owner);
            auto calls=callbackLines;callbackLines.clear();
            bool hb=Original_mpCheckFloor(static_cast<float>(x),static_cast<float>(y),static_cast<float>(x+dx),static_cast<float>(y+dy),static_cast<float>(offset),mask&1?&b:NULL,&ib,mask&2?&fb:NULL,mask&4?&nb:NULL,skip,-1,only,cb,&owner);
            assert(ha==hb && ia==ib && fa==fb && calls==callbackLines);
            Near(a.x,b.x);Near(a.y,b.y);Near(a.z,b.z);Near(na.x,nb.x);Near(na.y,nb.y);Near(na.z,nb.z);
            ++comparisons;
        }
    }
    Configure(0);Vec3 hit={};int index=-1;
    assert(mpCheckFloor(5,20,5,-20,0,&hit,&index,NULL,NULL,-1,0,-1,NULL,NULL));assert(index==2);
    assert(mpCheckFloor(5,20,5,-20,0,&hit,&index,NULL,NULL,-1,1,-1,NULL,NULL));assert(index==1);
    assert(!mpCheckFloor(5,20,5,-20,0,&hit,&index,NULL,NULL,-1,1,1,NULL,NULL));
    assert(mpCheckFloor(5,5,5,-5,2,&hit,&index,NULL,NULL,-1,-1,0,NULL,NULL));Near(hit.y,2);
    assert(!mpCheckFloor(5,-5,5,5,0,&hit,&index,NULL,NULL,-1,-1,0,NULL,NULL));
    // Airborne contact must use the whole path, not the floor at final X.
    groups[0].floor_count=1;groups[1].dynamic_count=0;prevLine[0]=nextLine[0]=-1;
    stage.lines[0]={0,10,10,0,M360_LINE_FLOOR,10};
    CollData cd={};cd.floor_skip=cd.joint_id_skip=cd.joint_id_only=-1;
    Vec3 from={0,5,0},to={10,-1,0};
    assert(!LandBetweenColl(&cd,NULL,&from,&to,NULL));Near(to.y,-1);
    stage.lines[0]={0,0,10,10,M360_LINE_FLOOR,10};from={0,8,0};to={10,2,0};
    assert(LandBetweenColl(&cd,NULL,&from,&to,NULL));Near(to.y,10.0001f);
    // Invalid native ranges are rejected/clipped instead of reading beyond storage.
    groups[0].floor_start=-5;groups[0].floor_count=99;
    assert(!mpCheckFloor(5,20,5,-20,0,NULL,NULL,NULL,NULL,-1,-1,-1,NULL,NULL));
    groups[0].floor_start=0;groups[0].floor_count=99;
    assert(mpCheckFloor(5,20,5,-20,0,NULL,NULL,NULL,NULL,-1,-1,-1,NULL,NULL));
    printf("PASS: %u original floor queries agree: direction, slopes, offsets, nearest contact, ranges, callbacks and outputs\n",comparisons);
    puts("PASS: diagonal landing regressions reject below-floor motion and retain genuine swept contacts");
}
