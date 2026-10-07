#include <cassert>
#include <cmath>
#include <cstdio>
#include "match_xdk.h"

struct Vec3 { float x, y, z; };
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
}
