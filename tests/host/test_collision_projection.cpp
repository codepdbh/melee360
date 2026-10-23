#include <cassert>
#include <cmath>
#include <cstdio>
#include "match_xdk.h"
struct Vec3 { float x,y,z; };
typedef unsigned u32;
struct RefMap { int v0_idx,v1_idx; u32 lo_flags; } maps[3];
struct CollLine { RefMap* x0; u32 flags; } groundCollLine[3];
struct Vertex { Vec3 pos; } groundCollVtx[6];
enum { CollLine_Floor=1, CollLine_Ceiling=2, CollLine_RightWall=4, CollLine_LeftWall=8 };
static M360MatchStage stage;
const M360MatchStage* M360_MatchStageData() { return &stage; }
bool mpLib_80054ED8(int i) { return i>=0 && i<3 && stage.lines[i].kind; }
int mpLineGetPrev(int i) { return i>0?i-1:-1; }
int mpLineGetNext(int i) { return i<2?i+1:-1; }
void PSVECNormalize(const Vec3* in, Vec3* out) {
    const float length=std::sqrt(in->x*in->x+in->y*in->y+in->z*in->z);
    *out={in->x/length,in->y/length,in->z/length};
}
#define LINEID_CHECK(n,i) assert(mpLib_80054ED8(i))
#pragma warning(push)
#pragma warning(disable:4244)
#include "projection_reference.h"
#include "collision_projection.h"
#pragma warning(pop)
struct CollData {
    Vec3 cur_pos;
    struct { int index; u32 flags; Vec3 normal; } floor;
    unsigned env_flags;
    struct { Vec3 left,right,bottom; } ecb;
    int facing_dir,joint_id_skip,joint_id_only;
    float lstick_x;
};
struct Fighter {
    Vec3 cur_pos; CollData coll_data;
    float facing_dir;
    struct { Vec3 lstick[1]; } input;
    float gr_vel; Vec3 self_vel;
};
struct HSD_GObj { Fighter* fp; };
#define GET_FIGHTER(gobj) ((gobj)->fp)
enum { Collide_LeftEdge=1,Collide_RightEdge=2,Collide_FloorHug=4,
       Collide_LeftWallMask=8,Collide_RightWallMask=16,Collide_Edge=32 };
int mpLineGetKind(int i) { return static_cast<int>(stage.lines[i].kind); }
void mpFloorGetLeft(int i,Vec3* out) { const auto& l=stage.lines[i];*out={l.x0,l.y0,0}; }
void mpFloorGetRight(int i,Vec3* out) { const auto& l=stage.lines[i];*out={l.x1,l.y1,0}; }
static bool leftBlocked,rightBlocked;
bool mpCheckLeftWall(float,float,float,float,Vec3*,int*,u32*,Vec3*,int,int) { return leftBlocked; }
bool mpCheckRightWall(float,float,float,float,Vec3*,int*,u32*,Vec3*,int,int) { return rightBlocked; }
#pragma warning(push)
#pragma warning(disable:4701) // Original conditional output assignments.
#include "edge_original.h"
#pragma warning(pop)
static unsigned fallbackCalls;
int FloorAt(float,float,float,int,float*,int*) { ++fallbackCalls;return 0; }
void SetFloor(Fighter* fp,int index) { fp->coll_data.floor.index=index; }
#include "ground_step_native.h"
#include "ground_edge_native.h"
typedef int (*Projection)(int,Vec3*,float*,u32*,Vec3*);
static void Near(float a,float b) { assert(std::isfinite(a) && std::isfinite(b) && std::fabs(a-b)<0.00001f); }
int main() {
    Projection actual[]={mpLib_8004DD90_Floor,mpLib_8004E090_Ceiling,mpLib_8004E398_LeftWall,mpLib_8004E684_RightWall};
    Projection original[]={Original_mpLib_8004DD90_Floor,Original_mpLib_8004E090_Ceiling,Original_mpLib_8004E398_LeftWall,Original_mpLib_8004E684_RightWall};
    const unsigned kinds[]={M360_LINE_FLOOR,M360_LINE_CEILING,M360_LINE_LEFT_WALL,M360_LINE_RIGHT_WALL};
    unsigned count=0;stage.lineCount=3;
    for(int kind=0;kind<4;++kind) for(int slope=0;slope<3;++slope) {
        for(int i=0;i<3;++i) {
            float a=static_cast<float>(i*10),b=a+10;
            M360StageLine l={};l.kind=kinds[kind];l.flags=static_cast<unsigned>(17+i);
            if(kind==0) { l.x0=a;l.x1=b;l.y0=a*slope*0.1f;l.y1=b*slope*0.1f; }
            if(kind==1) { l.x0=30-a;l.x1=30-b;l.y0=a*slope*0.1f;l.y1=b*slope*0.1f; }
            if(kind==2) { l.y0=a;l.y1=b;l.x0=a*slope*0.1f;l.x1=b*slope*0.1f; }
            if(kind==3) { l.y0=30-a;l.y1=30-b;l.x0=a*slope*0.1f;l.x1=b*slope*0.1f; }
            stage.lines[i]=l;maps[i]={2*i,2*i+1,l.flags};groundCollLine[i]={&maps[i],l.kind};
            groundCollVtx[2*i].pos={l.x0,l.y0,0};groundCollVtx[2*i+1].pos={l.x1,l.y1,0};
        }
        for(int line=0;line<3;++line) for(int p=-3;p<=303;++p) for(int mask=0;mask<8;++mask) {
            float coordinate=p*0.1f;
            Vec3 point={coordinate,coordinate,29},copy=point;
            float delta=123,refDelta=123;u32 flags=999,refFlags=999;
            Vec3 normal={9,8,7},refNormal=normal;
            int a=actual[kind](line,&point,mask&1?&delta:NULL,mask&2?&flags:NULL,mask&4?&normal:NULL);
            int b=original[kind](line,&copy,mask&1?&refDelta:NULL,mask&2?&refFlags:NULL,mask&4?&refNormal:NULL);
            assert(a==b && flags==refFlags);Near(delta,refDelta);
            Near(normal.x,refNormal.x);Near(normal.y,refNormal.y);Near(normal.z,refNormal.z);
            assert(point.x==copy.x && point.y==copy.y && point.z==copy.z);
            ++count;
        }
        // Beyond the final endpoint must fail, with outputs untouched.
        Vec3 point={100,100,0},normal={7,8,9};float delta=321;u32 flags=777;
        assert(actual[kind](1,&point,&delta,&flags,&normal)==-1);
        assert(delta==321 && flags==777 && normal.y==8);
        assert(actual[kind](-1,&point,NULL,NULL,NULL)==-1);
        stage.lines[0].kind=0;
        assert(actual[kind](0,&point,NULL,NULL,NULL)==-1);
    }
    printf("PASS: %u projections match original floor/ceiling/wall code, including adjacency, slopes, edge tolerance and null outputs\n",count);
    stage.lines[0]={0,0,10,20,M360_LINE_FLOOR};
    stage.lines[1]={10,20,20,40,M360_LINE_FLOOR};
    stage.lines[2]={20,40,30,60,M360_LINE_CEILING};
    Fighter fp={};HSD_GObj gobj={&fp};
    fp.coll_data.floor.index=0;fp.cur_pos={9,10,0};
    assert(GroundStep(&gobj,0));Near(fp.cur_pos.y,18.0001f);
    fp.cur_pos={14,18,0};assert(GroundStep(&gobj,0));
    assert(fp.coll_data.floor.index==1);Near(fp.cur_pos.y,28.0001f);
    fp.cur_pos={21,40,0};assert(!GroundStep(&gobj,0));Near(fp.cur_pos.x,21);
    assert(fallbackCalls==0);
    assert(GroundStep(&gobj,1));Near(fp.cur_pos.x,20);Near(fp.cur_pos.y,40);
    stage.lines[1].kind=0;fp.cur_pos={15,30,0};
    assert(!GroundStep(&gobj,1));assert(fallbackCalls==0);
    fp.coll_data.floor.index=-1;assert(!GroundStep(&gobj,0));assert(fallbackCalls==1);
    puts("PASS: grounded traversal follows steep/adjacent floors and rejects disabled/open ends without unrelated snaps");
    stage.lines[0]={0,0,10,0,M360_LINE_FLOOR};stage.lines[1].kind=stage.lines[2].kind=0;
    fp={};fp.coll_data.floor.index=0;fp.cur_pos={-1,0,0};
    fp.coll_data.ecb.left={-4,4,0};fp.coll_data.ecb.right={4,4,0};
    fp.coll_data.ecb.bottom={1,2,0};
    assert(GroundStep(&gobj,1));Near(fp.cur_pos.x,-1);Near(fp.cur_pos.y,-2);
    assert(fp.coll_data.env_flags&Collide_RightEdge);
    assert(!(fp.coll_data.env_flags&Collide_LeftEdge));
    fp.cur_pos={11,0,0};assert(GroundStep(&gobj,1));Near(fp.cur_pos.x,9);
    assert(fp.coll_data.env_flags&Collide_LeftEdge);
    rightBlocked=true;fp.cur_pos={11,0,0};assert(!GroundStep(&gobj,1));Near(fp.cur_pos.x,11);
    rightBlocked=false;leftBlocked=true;fp.cur_pos={-1,0,0};assert(!GroundStep(&gobj,1));
    leftBlocked=false;
    fp.coll_data.ecb.bottom={0,0,0};fp.cur_pos={-1,0,0};fp.facing_dir=-1;
    fp.input.lstick[0].x=-0.749f;assert(!GroundStepEdge(&gobj));
    assert(fp.coll_data.env_flags&Collide_Edge);Near(fp.cur_pos.x,0);
    fp.cur_pos={-1,0,0};fp.input.lstick[0].x=-0.75f;assert(!GroundStepEdge(&gobj));
    assert(!(fp.coll_data.env_flags&Collide_Edge));Near(fp.cur_pos.x,-1);
    fp.cur_pos={11,0,0};fp.facing_dir=1;fp.input.lstick[0].x=0.749f;
    assert(!GroundStepEdge(&gobj));assert(fp.coll_data.env_flags&Collide_Edge);
    fp.cur_pos={11,0,0};fp.input.lstick[0].x=0.75f;assert(!GroundStepEdge(&gobj));
    assert(!(fp.coll_data.env_flags&Collide_Edge));
    fp.input.lstick[0].x=0;fp.coll_data.env_flags=Collide_RightWallMask;
    assert(!GroundStepEdge(&gobj));assert(!(fp.coll_data.env_flags&Collide_Edge));
    fp.coll_data.env_flags=0;rightBlocked=true;assert(!GroundStepEdge(&gobj));
    assert(!(fp.coll_data.env_flags&Collide_Edge));
    puts("PASS: original edge direction, ECB offsets, wall blocking and strict teeter stick thresholds");
}
