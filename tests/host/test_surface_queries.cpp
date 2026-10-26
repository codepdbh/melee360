#include <cassert>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstddef>
#include "match_xdk.h"
typedef unsigned u32;
typedef unsigned char u8;
struct Vec3 { float x,y,z; };
struct MapJoint {
    int floor_start,floor_count,ceiling_start,ceiling_count;
    int right_wall_start,right_wall_count,left_wall_start,left_wall_count;
    int dynamic_start,dynamic_count;
} groups[2];
struct MapLine { int v0_idx,v1_idx; u32 lo_flags; } maps[3];
struct CollLine { MapLine* x0; u32 flags; } groundCollLine[3];
struct CollVtx { Vec3 pos; } groundCollVtx[6];
struct CollJoint { unsigned flags; CollJoint* next; MapJoint* inner; } groundCollJoint[2];
static CollJoint* jointListStart=groundCollJoint;
enum { CollLine_Floor=1, CollLine_Ceiling=2, CollLine_LeftWall=8, CollLine_RightWall=4,
       LINE_FLAG_EMPTY=1<<7, LINE_FLAG_ENABLED=1<<16, CollJoint_TooFar=1 };
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
#include "surface_queries_reference.h"
#include "collision_projection.h"
#include "collision_surface_queries.h"
#pragma warning(pop)
static void Near(float a,float b) { assert(std::isfinite(a) && std::isfinite(b) && std::fabs(a-b)<0.00001f); }
static void Configure(int type,int variant) {
    const unsigned kinds[]={M360_LINE_CEILING,M360_LINE_LEFT_WALL,M360_LINE_RIGHT_WALL};
    stage.lineCount=3;groups[0]={};groups[1]={};
    float slope=variant==1?0.5f:variant==2?-0.5f:0;
    for(int i=0;i<3;++i) {
        float shift=static_cast<float>(i*5);
        if(type==0) stage.lines[i]={20,shift+20*slope,-20,shift-20*slope,kinds[type],101u+i};
        else if(type==1) stage.lines[i]={shift-20*slope,-20,shift+20*slope,20,kinds[type],101u+i};
        else stage.lines[i]={shift+20*slope,20,shift-20*slope,-20,kinds[type],101u+i};
        prevLine[i]=nextLine[i]=-1;
    }
    if(type==0) { groups[0].ceiling_count=2;prevLine[0]=1; }
    if(type==1) groups[0].left_wall_count=2;
    if(type==2) groups[0].right_wall_count=2;
    groups[1].dynamic_start=variant==5?0:2;groups[1].dynamic_count=variant==5?3:1;
    if(variant==3) stage.lines[1].kind=0;
    if(variant==4) stage.lines[2].kind|=LINE_FLAG_EMPTY;
    if(variant==6) stage.lines[2].kind=M360_LINE_FLOOR;
    for(int i=0;i<3;++i) {
        const auto& l=stage.lines[i];maps[i]={2*i,2*i+1,l.flags};
        groundCollLine[i]={&maps[i],l.kind?l.kind|LINE_FLAG_ENABLED:kinds[type]};
        groundCollVtx[2*i].pos={l.x0,l.y0,0};groundCollVtx[2*i+1].pos={l.x1,l.y1,0};
    }
    for(int i=0;i<2;++i) groundCollJoint[i]={0,i==0?&groundCollJoint[1]:NULL,&groups[i]};
}
typedef bool (*Query)(float,float,float,float,Vec3*,int*,u32*,Vec3*,int,int);
int main() {
    const Query native[]={mpCheckCeiling,mpCheckLeftWall,mpCheckRightWall};
    const Query original[]={Original_mpCheckCeiling,Original_mpCheckLeftWall,Original_mpCheckRightWall};
    unsigned comparisons=0;
    for(int type=0;type<3;++type) for(int v=0;v<7;++v) {
        Configure(type,v);
        for(int x=-25;x<=25;x+=5) for(int y=-25;y<=25;y+=5)
        for(int dx=-30;dx<=30;dx+=30) for(int dy=-30;dy<=30;dy+=30)
        for(int skip=-1;skip<3;++skip) for(int only=-1;only<3;++only)
        for(int mask=0;mask<16;++mask) {
            Vec3 a={91,92,93},b=a,na={81,82,83},nb=na;int ia=77,ib=77;u32 fa=88,fb=88;
            bool ha=native[type](static_cast<float>(x),static_cast<float>(y),static_cast<float>(x+dx),static_cast<float>(y+dy),mask&1?&a:NULL,mask&2?&ia:NULL,mask&4?&fa:NULL,mask&8?&na:NULL,skip,only);
            bool hb=original[type](static_cast<float>(x),static_cast<float>(y),static_cast<float>(x+dx),static_cast<float>(y+dy),mask&1?&b:NULL,mask&2?&ib:NULL,mask&4?&fb:NULL,mask&8?&nb:NULL,skip,only);
            if(ha!=hb || ia!=ib || fa!=fb) {
                printf("Mismatch type=%d variant=%d from=%d,%d delta=%d,%d skip=%d only=%d mask=%d native=%d,%d,%u original=%d,%d,%u\n",type,v,x,y,dx,dy,skip,only,mask,ha,ia,fa,hb,ib,fb);
                fflush(stdout);
                assert(false);
            }
            Near(a.x,b.x);Near(a.y,b.y);Near(a.z,b.z);Near(na.x,nb.x);Near(na.y,nb.y);Near(na.z,nb.z);
            ++comparisons;
        }
    }
    Vec3 hit={};int line=-1;
    Configure(0,0);
    assert(mpCheckCeiling(0,-10,0,20,&hit,&line,NULL,NULL,-1,-1));assert(line==0);Near(hit.y,0);
    assert(!mpCheckCeiling(0,20,0,-10,&hit,&line,NULL,NULL,-1,-1));
    Configure(1,0);
    assert(mpCheckLeftWall(-10,0,20,0,&hit,&line,NULL,NULL,-1,-1));assert(line==0);Near(hit.x,0);
    assert(!mpCheckLeftWall(20,0,-10,0,&hit,&line,NULL,NULL,-1,-1));
    Configure(2,0);
    assert(mpCheckRightWall(20,0,-10,0,&hit,&line,NULL,NULL,-1,-1));assert(line==2);Near(hit.x,10);
    assert(!mpCheckRightWall(-10,0,20,0,&hit,&line,NULL,NULL,-1,-1));
    for(int type=0;type<3;++type) {
        Configure(type,0);groups[0]={};groups[1].dynamic_start=-1;groups[1].dynamic_count=100;
        assert(!native[type](-30,-30,30,30,NULL,NULL,NULL,NULL,-1,-1));
        groups[1].dynamic_start=0;groups[1].dynamic_count=-1;
        assert(!native[type](-30,-30,30,30,NULL,NULL,NULL,NULL,-1,-1));
        groups[1].dynamic_count=100;
        native[type](-30,-30,30,30,NULL,NULL,NULL,NULL,-1,-1);
    }
    printf("PASS: %u original ceiling/wall queries agree: slopes, crossing direction, groups, nearest hit and all optional outputs\n",comparisons);
    puts("PASS: reverse-side crossings rejected and malformed native dynamic ranges guarded");
}
