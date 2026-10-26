#include <cassert>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include "match_xdk.h"

struct Vec3 { float x, y, z; };
typedef unsigned u32;
typedef unsigned char u8;
struct Fighter_GObj {};
struct MapJoint {
    int floor_start, floor_count, ceiling_start, ceiling_count;
    int right_wall_start, right_wall_count, left_wall_start, left_wall_count;
    int dynamic_start, dynamic_count;
};
static MapJoint groups[3];
struct TestColl { int joint_count; MapJoint* joints; } collision = {3, groups};
static TestColl* s_coll = &collision;
unsigned M360_MatchMapJointCount() { return static_cast<unsigned>(s_coll->joint_count); }
const MapJoint* M360_MatchMapJoint(unsigned i) { return &groups[i]; }
int mpLineGetPrev(int) { return -1; }
int mpLineGetNext(int) { return -1; }
void PSVECNormalize(const Vec3* in,Vec3* out) {
    float length=std::sqrt(in->x*in->x+in->y*in->y+in->z*in->z);
    *out={in->x/length,in->y/length,in->z/length};
}
void mpLineGetNormal(int, Vec3* normal) { *normal = {0, 1, 0}; }
struct CollData {
    struct Surface { int index; } floor, ceiling, left_facing_wall, right_facing_wall;
    struct { Vec3 top; } ecb;
};
static M360MatchStage testStage;
#define s_stage testStage
const M360MatchStage* M360_MatchStageData() { return &testStage; }
#pragma warning(push)
#pragma warning(disable: 4244)
#include "surface_speed_original.h"
#include "surface_remap_reference.h"
#define SQ(x) ((x)*(x))
enum { LINE_FLAG_EMPTY=1u<<7 };
#pragma warning(push)
#pragma warning(disable:4101 4701)
#include "collision_projection.h"
#include "collision_floor_query.h"
#include "collision_surface_queries.h"
#pragma warning(pop)
#pragma warning(pop)

static void Near(float a, float b) { assert(std::fabs(a - b) < 0.0001f); }

// Run the unmodified upstream function against the same small map view.
struct RefLine { int v0_idx,v1_idx; } refLines[2];
struct CollLine { RefLine* x0; unsigned flags; } groundCollLine[2];
struct RefVertex { Vec3 pos; } groundCollVtx[4];
struct CollJoint { unsigned flags; CollJoint* next; MapJoint* inner; } groundCollJoint[3];
static CollJoint* jointListStart=groundCollJoint;
enum { CollJoint_TooFar=1, CollLine_Floor=1, LINE_FLAG_ENABLED=1u<<16 };
bool mpCheckedBounding() { return false; }
void mpBoundingCheck2(float,float,float,float) {}
void mpUncheckBounding() {}
#define PAD_STACK(n)
#pragma warning(push)
#pragma warning(disable:4244) // Original pointer differences are 32-bit on target.
#include "floor_below_reference.h"
#pragma warning(pop)

int main()
{
    testStage.lineCount = 4;
    for (unsigned i = 0; i < 4; ++i) {
        testStage.lines[i] = { 5, 3, 15, 3, M360_LINE_FLOOR, 0, 0, 0, 0, 0, 10, 0 };
    }
    Vec3 pos = { 4, 0, 0 }, speed = { 99, 98, 97 };
    assert(mpGetSpeed(0, &pos, &speed));
    Near(speed.x, 5); Near(speed.y, 3); Near(speed.z, 0);
    // Static surfaces still return success, just as the original does.
    testStage.lines[0].x0 = 0; testStage.lines[0].y0 = 0;
    testStage.lines[0].x1 = 10; testStage.lines[0].y1 = 0;
    assert(mpGetSpeed(0, &pos, &speed));
    Near(speed.x, 0); Near(speed.y, 0);
    speed = { 99, 98, 97 };
    assert(!mpGetSpeed(-1, &pos, &speed));
    assert(!mpGetSpeed(4, &pos, &speed));
    testStage.lines[0].kind = 0;
    assert(!mpGetSpeed(0, &pos, &speed));
    Near(speed.x, 99); Near(speed.y, 98); Near(speed.z, 97);
    CollData coll = {};
    coll.ecb.top = pos;
    coll.floor.index = coll.ceiling.index = coll.left_facing_wall.index = coll.right_facing_wall.index = 1;
    assert(mpCollGetSpeedFloor(&coll, &speed)); Near(speed.x, 5); Near(speed.y, 3);
    assert(mpCollGetSpeedCeiling(&coll, &speed)); Near(speed.x, 5); Near(speed.y, 3);
    assert(mpCollGetSpeedLeftWall(&coll, &speed)); Near(speed.x, 5); Near(speed.y, 3);
    assert(mpCollGetSpeedRightWall(&coll, &speed)); Near(speed.x, 5); Near(speed.y, 3);
    // Compare with the actual original helper, including clamping,
    // slopes, reversed/vertical lines and its degenerate-line branch.
    for (int i = -50; i <= 50; ++i) {
        for (int j = 0; j < 4; ++j) {
            float ax = j == 0 ? 0.0f : 10.0f, ay = 2.0f;
            float bx = j == 1 ? ax : -10.0f, by = j == 2 ? ay : 15.0f;
            if (j == 3) { bx = ax; by = ay; }
            float nx, ny, rx, ry;
            mpRemap2d(&nx, &ny, ax, ay, bx, by, ax + 3, ay - 2, bx - 5, by + 4, static_cast<float>(i), 8);
            OriginalRemap(&rx, &ry, ax, ay, bx, by, ax + 3, ay - 2, bx - 5, by + 4, static_cast<float>(i), 8);
            Near(nx, rx); Near(ny, ry);
        }
    }
    puts("PASS: original moving-surface remap, all four collision queries, disabled lines and static surfaces");
    // A disabled nearer surface must not mask the active surface behind it.
    testStage.lineCount = 2;
    testStage.lines[0] = { -10, 5, 10, 5, 0 };
    testStage.lines[1] = { -10, 0, 10, 0, M360_LINE_FLOOR };
    Vec3 hit = {}, normal = {};
    int line = -1;
    u32 flags = 0;
    assert(mpCheckAllRemap(&hit, &line, &flags, &normal, -1, -1, 0, 10, 0, -10));
    assert(line == 1); Near(hit.y, 0); Near(normal.y, 1);
    testStage.lines[0].kind = M360_LINE_FLOOR;
    assert(mpCheckAll(&hit, &line, NULL, NULL, -1, -1, 0, 10, 0, -10));
    assert(line == 0); Near(hit.y, 5);
    testStage.lines[0].kind = testStage.lines[1].kind = 0;
    hit = { 17, 18, 19 }; line = 99; flags = 123; normal = { 7, 8, 9 };
    assert(!mpCheckAll(&hit, &line, &flags, &normal, -1, -1, 0, 10, 0, -10));
    assert(line == 99 && flags == 123); Near(hit.y, 18); Near(normal.y, 8);
    assert(!mpCheckAllRemap(NULL, NULL, NULL, NULL, -1, -1, 0, 10, 0, -10));
    puts("PASS: all-surface queries ignore inactive geometry, preserve nearest active hit and allow reactivation");
    groups[0].floor_start=0; groups[0].floor_count=1;
    groups[1].dynamic_start=1; groups[1].dynamic_count=1;
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_FLOOR;
    assert(mpCheckAll(&hit,&line,NULL,NULL,0,-1,0,10,0,-10) && line==1);
    assert(mpCheckAll(&hit,&line,NULL,NULL,-1,1,0,10,0,-10) && line==1);
    assert(!mpCheckAll(&hit,&line,NULL,NULL,1,1,0,10,0,-10));
    assert(!mpCheckAll(&hit,&line,NULL,NULL,-1,99,0,10,0,-10));
    assert(mpCheckFloor(0,10,0,-10,0,&hit,&line,NULL,NULL,-1,0,-1,NULL,NULL) && line==1);
    assert(mpCheckFloor(0,10,0,-10,0,&hit,&line,NULL,NULL,-1,-1,0,NULL,NULL) && line==0);
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_CEILING;
    testStage.lines[0]={10,5,-10,5,M360_LINE_CEILING};
    groups[0].ceiling_start=0; groups[0].ceiling_count=1;
    assert(mpCheckCeiling(0,-10,0,10,&hit,&line,NULL,NULL,-1,0) && line==0);
    assert(!mpCheckCeiling(0,-10,0,10,&hit,&line,NULL,NULL,0,0));
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_LEFT_WALL;
    testStage.lines[1]={0,-10,0,10,M360_LINE_LEFT_WALL};
    assert(mpCheckLeftWall(-10,0,10,0,&hit,&line,NULL,NULL,-1,1) && line==1);
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_RIGHT_WALL;
    testStage.lines[1]={0,10,0,-10,M360_LINE_RIGHT_WALL};
    assert(mpCheckRightWall(10,0,-10,0,&hit,&line,NULL,NULL,0,-1) && line==1);
    // Shared ranges retain a line if any non-excluded group owns it.
    groups[2].left_wall_start=0; groups[2].left_wall_count=1;
    assert(M360_MatchLineMatchesJoint(0,0,-1));
    assert(!M360_MatchLineMatchesJoint(0,-1,1));
    puts("PASS: original joint skip/only semantics across all query kinds and dynamic/shared ranges");
    groups[2] = {};
    groups[0].floor_start=0; groups[0].floor_count=1;
    groups[1].floor_start=1; groups[1].floor_count=1;
    groups[1].dynamic_count=0;
    testStage.lines[0] = {-10,0,10,0,M360_LINE_FLOOR};
    testStage.lines[1] = {-10,5,10,5,M360_LINE_FLOOR};
    Vec3 probe={0,10,73};
    assert(mpLib_8005199C_Floor(&probe,-1,-1)==0); // First, not highest.
    assert(mpLib_8005199C_Floor(&probe,0,-1)==1);
    assert(mpLib_8005199C_Floor(&probe,-1,1)==1);
    assert(mpLib_8005199C_Floor(&probe,1,1)==-1);
    assert(probe.x==0 && probe.y==10 && probe.z==73);
    testStage.lines[0].kind=0;
    assert(mpLib_8005199C_Floor(&probe,-1,-1)==1);
    groups[1].floor_count=0;groups[1].dynamic_start=1;groups[1].dynamic_count=1;
    assert(mpLib_8005199C_Floor(&probe,-1,-1)==1);
    testStage.lines[1].y0=0; testStage.lines[1].y1=10;
    probe.y=4;assert(mpLib_8005199C_Floor(&probe,-1,-1)==-1);
    probe.y=5;assert(mpLib_8005199C_Floor(&probe,-1,-1)==1);
    probe.x=11;assert(mpLib_8005199C_Floor(&probe,-1,-1)==-1);
    probe.x=0;testStage.lines[1].kind=M360_LINE_CEILING;
    assert(mpLib_8005199C_Floor(&probe,-1,-1)==-1);
    puts("PASS: floor-below query uses authored order, filters, dynamic floors, slopes and immutable input");
    unsigned comparisons=0;
    for(int variant=0;variant<8;++variant) {
        testStage.lines[0]={-10,0,10,static_cast<float>(variant),M360_LINE_FLOOR};
        testStage.lines[1]={-10,7,10,3,M360_LINE_FLOOR};
        if(variant&1) testStage.lines[0].kind=0;
        for(int i=0;i<3;++i) {
            groundCollJoint[i].flags=0;groundCollJoint[i].inner=&groups[i];
            groundCollJoint[i].next=i<2?&groundCollJoint[i+1]:NULL;
        }
        for(int i=0;i<2;++i) {
            const auto& l=testStage.lines[i];refLines[i]={i*2,i*2+1};
            groundCollLine[i]={&refLines[i],l.kind?CollLine_Floor|LINE_FLAG_ENABLED:0u};
            groundCollVtx[i*2].pos={l.x0,l.y0,0};groundCollVtx[i*2+1].pos={l.x1,l.y1,0};
        }
        for(int x=-11;x<=11;++x) for(int y=-1;y<=11;++y)
            for(int skip=-1;skip<3;++skip) for(int only=-1;only<4;++only) {
                Vec3 p={static_cast<float>(x),static_cast<float>(y),0};
                assert(mpLib_8005199C_Floor(&p,skip,only)==OriginalFloorBelow(&p,skip,only));
                ++comparisons;
            }
    }
    printf("PASS: %u floor queries match the original function on the same map view\n",comparisons);
}
