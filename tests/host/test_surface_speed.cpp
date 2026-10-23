#include <cassert>
#include <cmath>
#include <cstdio>
#include "match_xdk.h"

struct Vec3 { float x, y, z; };
typedef unsigned u32;
struct Fighter_GObj {};
struct MapJoint {
    int floor_start, floor_count, ceiling_start, ceiling_count;
    int right_wall_start, right_wall_count, left_wall_start, left_wall_count;
    int dynamic_start, dynamic_count;
};
static MapJoint groups[3];
struct TestColl { int joint_count; MapJoint* joints; } collision = {3, groups};
static TestColl* s_coll = &collision;
void mpLineGetNormal(int, Vec3* normal) { *normal = {0, 1, 0}; }
struct CollData {
    struct Surface { int index; } floor, ceiling, left_facing_wall, right_facing_wall;
    struct { Vec3 top; } ecb;
};
static M360MatchStage testStage;
const M360MatchStage* M360_MatchStageData() { return &testStage; }
#pragma warning(push)
#pragma warning(disable: 4244)
#include "surface_speed_original.h"
#include "surface_remap_reference.h"
#pragma warning(pop)

static void Near(float a, float b) { assert(std::fabs(a - b) < 0.0001f); }

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
    assert(mpCheckCeiling(0,-10,0,10,&hit,&line,NULL,NULL,-1,0) && line==0);
    assert(!mpCheckCeiling(0,-10,0,10,&hit,&line,NULL,NULL,0,0));
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_LEFT_WALL;
    assert(mpCheckLeftWall(0,10,0,-10,&hit,&line,NULL,NULL,-1,1) && line==1);
    testStage.lines[0].kind = testStage.lines[1].kind = M360_LINE_RIGHT_WALL;
    assert(mpCheckRightWall(0,10,0,-10,&hit,&line,NULL,NULL,0,-1) && line==1);
    // Shared ranges retain a line if any non-excluded group owns it.
    groups[2].left_wall_start=0; groups[2].left_wall_count=1;
    assert(M360_MatchLineMatchesJoint(0,0,-1));
    assert(!M360_MatchLineMatchesJoint(0,-1,1));
    puts("PASS: original joint skip/only semantics across all query kinds and dynamic/shared ranges");
}
