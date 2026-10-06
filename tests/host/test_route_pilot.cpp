#include <cassert>
#include <cmath>
#include <cstdio>
#include "match_xdk.h"
#include "route_pilot_original.h"

int main()
{
    M360MatchStage stage = {};
    stage.lineCount = 6;
    stage.lines[0] = { 0, 10, 20, 10, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    stage.lines[1] = { 40, 20, 20, 10, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    stage.lines[2] = { -20, 5, 0, 10, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    stage.lines[3] = { 40, 20, 80, 20, M360_LINE_CEILING, M360_LINE_PLATFORM, 0, 0, 0, 0, 0, 0 };
    stage.lines[4] = { -40, 5, -20, 5, 0, 0, 0, 0, 0, 0, 0, 0 };
    stage.lines[5] = { 40, 40, 90, 40, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    float left, leftY, right, rightY, x, y;
    PilotSurfaceSpan(&stage, 0, M360_LINE_CEILING, &left, &leftY, &right, &rightY);
    assert(left == -20 && leftY == 5 && right == 40 && rightY == 20);
    assert(PilotCeilingDetour(&stage, 10, 0, 100, 100, &x, &y));
    assert(x == 65 && y == 45);
    assert(PilotCeilingDetour(&stage, 10, 0, -100, 100, &x, &y));
    assert(x == -45 && y == 30);
    assert(!PilotCeilingDetour(&stage, 100, 0, 100, 100, &x, &y));
    assert(!PilotCeilingDetour(&stage, 10, -100, 100, 100, &x, &y));
    assert(!PilotCeilingDetour(&stage, 10, 15, 100, 100, &x, &y));
    stage.lines[0].kind = stage.lines[1].kind = stage.lines[2].kind = 0;
    assert(!PilotCeilingDetour(&stage, 10, 0, 100, 100, &x, &y));
    stage.lineCount = 1;
    stage.lines[0] = { 555, -297, 828, -190, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    assert(PilotCeilingDetour(&stage, 572, -310, 711, -95, &x, &y));
    assert(x == 853 && y == -165);
    stage.lines[0] = { 778, -48, 828, -60, M360_LINE_CEILING, 0, 0, 0, 0, 0, 0, 0 };
    assert(!PilotCeilingDetour(&stage, 811, -130, 711, -95, &x, &y));
    puts("PASS: diagnostic ceiling navigation follows connected ramps, ignores disabled and disconnected surfaces, and chooses a real edge");
}
