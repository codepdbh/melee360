#include <xtl.h>
#include <stdio.h>
#include <string.h>

#include "controller_xdk_compat.h"
#ifdef M360_INPUT_SCRIPT
#include "match_xdk.h"
#endif

extern "C" void M360_MatchTrace(const char* stage, unsigned value);

namespace {

HSD_PadData g_padQueue[2];

s8 ScaleThumb(SHORT value)
{
    int scaled = value >= 0 ? (static_cast<int>(value) * 127) / 32767
                            : (static_cast<int>(value) * 127) / 32768;
    if (scaled > 127)
        scaled = 127;
    if (scaled < -127)
        scaled = -127;
    return static_cast<s8>(scaled);
}

u16 TranslateButtons(WORD buttons)
{
    u16 translated = 0;
    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) translated |= PAD_BUTTON_LEFT;
    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) translated |= PAD_BUTTON_RIGHT;
    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) translated |= PAD_BUTTON_DOWN;
    if (buttons & XINPUT_GAMEPAD_DPAD_UP) translated |= PAD_BUTTON_UP;
    if (buttons & XINPUT_GAMEPAD_A) translated |= PAD_BUTTON_A;
    if (buttons & XINPUT_GAMEPAD_B) translated |= PAD_BUTTON_B;
    if (buttons & XINPUT_GAMEPAD_X) translated |= PAD_BUTTON_X;
    if (buttons & XINPUT_GAMEPAD_Y) translated |= PAD_BUTTON_Y;
    if (buttons & XINPUT_GAMEPAD_START) translated |= PAD_BUTTON_START;
    /* Bumpers map to GameCube digital shoulder functions: LB shields with L,
     * while RB is Melee's Z/grab button. LT/RT remain analog L/R shields. */
    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) translated |= PAD_TRIGGER_L;
    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) translated |= PAD_TRIGGER_Z;
    return translated;
}

#ifdef M360_INPUT_SCRIPT
struct ScriptStep {
    unsigned frames;
    int snap;
    u16 buttons;
    s8 stickX, stickY, subX, subY;
};

ScriptStep g_script[512];
unsigned g_scriptCount;
unsigned g_scriptIndex;
unsigned g_scriptLeft;
unsigned g_scriptHold;
bool g_scriptLoaded;
bool g_scriptLoop;
unsigned g_scriptPasses;
bool g_routePilot;
unsigned g_routePilotTick;
unsigned g_routeJumpCooldown;
float g_routePreviousY;
bool g_routeArena;
float g_routeArenaX;
float g_routeArenaY;
float g_routeArenaLeft, g_routeArenaRight;
int g_escapeTargetLine = -1;
unsigned g_routePreviousFrame;
unsigned g_routeArenaObjective;
int g_mazeTargetRoom = -1;
bool g_mazeDetour;
float g_mazeDetourX, g_mazeDetourY;
unsigned g_mazeJumpHold;
unsigned g_mazeDetourAge;

/* Follow authored adjoining surfaces; disconnected nearby geometry must not
 * extend the span. This is navigation for the diagnostic PAD pilot only. */
static void PilotSurfaceSpan(const M360MatchStage* stage, unsigned seed,
                             unsigned kind, float* left, float* leftY,
                             float* right, float* rightY)
{
    const M360StageLine& first = stage->lines[seed];
    *left = first.x0; *leftY = first.y0; *right = first.x1; *rightY = first.y1;
    if (*left > *right) {
        *left = first.x1; *leftY = first.y1; *right = first.x0; *rightY = first.y0;
    }
    for (unsigned pass = 0; pass < stage->lineCount; ++pass) {
        bool changed = false;
        for (unsigned line = 0; line < stage->lineCount; ++line) {
            const M360StageLine& candidate = stage->lines[line];
            if (!(candidate.kind & kind) || (candidate.flags & M360_LINE_PLATFORM)) continue;
            const float lx = candidate.x0 < candidate.x1 ? candidate.x0 : candidate.x1;
            const float rx = candidate.x0 < candidate.x1 ? candidate.x1 : candidate.x0;
            const float ly = candidate.x0 < candidate.x1 ? candidate.y0 : candidate.y1;
            const float ry = candidate.x0 < candidate.x1 ? candidate.y1 : candidate.y0;
            if (fabsf(rx - *left) < 1.0f && fabsf(ry - *leftY) < 1.0f && lx < *left - 0.1f) {
                *left = lx; *leftY = ly; changed = true;
            }
            if (fabsf(lx - *right) < 1.0f && fabsf(ly - *rightY) < 1.0f && rx > *right + 0.1f) {
                *right = rx; *rightY = ry; changed = true;
            }
        }
        if (!changed) break;
    }
}

static int PilotCeilingDetour(const M360MatchStage* stage, float x, float y,
                              float goalX, float goalY, float* detourX, float* detourY)
{
    int selected = -1;
    float nearest = 90.0f;
    for (unsigned i = 0; i < stage->lineCount; ++i) {
        const M360StageLine& line = stage->lines[i];
        if (!(line.kind & M360_LINE_CEILING)) continue;
        const float lo = line.x0 < line.x1 ? line.x0 : line.x1;
        const float hi = line.x0 < line.x1 ? line.x1 : line.x0;
        if (x < lo || x > hi || hi - lo < 0.1f) continue;
        const float height = line.y0 + (line.y1 - line.y0) * ((x - line.x0) / (line.x1 - line.x0));
        // A roof above the target's head does not obstruct reaching it.
        // Detouring around that roof can trap the pilot in an upper corridor.
        if (height > goalY + 20.0f) continue;
        if (height > y + 2.0f && height - y < nearest) {
            nearest = height - y; selected = static_cast<int>(i);
        }
    }
    if (selected < 0) return 0;
    float left, leftY, right, rightY;
    PilotSurfaceSpan(stage, static_cast<unsigned>(selected), M360_LINE_CEILING,
                     &left, &leftY, &right, &rightY);
    // A sloping passage's high end may be farther away horizontally but
    // leads upward; the low end can trap the pilot under another ceiling.
    const float leftCost = fabsf(x - left) + fabsf(goalX - left) + fabsf(goalY - leftY) * 3.0f;
    const float rightCost = fabsf(x - right) + fabsf(goalX - right) + fabsf(goalY - rightY) * 3.0f;
    *detourX = leftCost < rightCost ? left - 25.0f : right + 25.0f;
    *detourY = (leftCost < rightCost ? leftY : rightY) + 25.0f;
    // Stay outside the overhang until near the destination's height. Turning
    // back immediately after clearing a low roof wastes jumps below a ledge.
    // Maze entrances have a 35-unit half extent; retain a five-unit margin.
    if (*detourY < goalY - 30.0f) *detourY = goalY - 30.0f;
    return 1;
}

/* Diagnostic input only: follow the collision surface with normal controller
 * commands. No position, damage, stocks or encounter state is modified. */
static int PilotMazeSteer(float delta, bool walkOffEdge)
{
    if (fabsf(delta) < 2.0f) return 0;
    if (walkOffEdge) return delta < 0.0f ? -127 : 127;
    int steer = static_cast<int>(delta * 4.0f);
    if (steer > 127) steer = 127;
    if (steer < -127) steer = -127;
    // Do not stop eight pixels short of an edge inside the stick dead zone.
    if (steer > 0 && steer < 64) steer = 64;
    if (steer < 0 && steer > -64) steer = -64;
    return steer;
}

static bool PilotMazeCanJump(bool detour, float delta, int floor)
{
    // Grounded movement follows the ramp first; airborne recovery must
    // remain available when a small wall prevents approaching the edge.
    return !detour || fabsf(delta) < 20.0f || floor < 0;
}

void ApplyRoutePilot(PADStatus* pad)
{
    M360MatchStatus match;
    M360_MatchGetStatus(&match);
    if (match.frame < g_routePreviousFrame) {
        g_escapeTargetLine = -1; g_routeArena = false;
        g_routeJumpCooldown = g_mazeJumpHold = g_mazeDetourAge = 0; g_mazeTargetRoom = -1; g_mazeDetour = false;
    }
    g_routePreviousFrame = match.frame;
    pad->err = 0;
    const unsigned tick = ++g_routePilotTick;
    if (match.matchOver) {
        if (tick % 60 == 0) pad->button |= PAD_BUTTON_A;
        return;
    }
    if (!match.loaded || match.selecting || match.motion[0] == 12 || match.motion[0] == 13)
        return;
    if (match.campaignObjective != g_routeArenaObjective) {
        g_routeArena = false;
        g_mazeDetour = false;
        g_mazeJumpHold = 0;
        g_mazeDetourAge = 0;
        g_routeArenaObjective = match.campaignObjective;
    }
    const float x = match.posX[0], y = match.posY[0];
    const float dy = y - g_routePreviousY;
    g_routePreviousY = y;
    if (g_routeJumpCooldown) --g_routeJumpCooldown;
    float targetX = x + 50.0f;
    float targetY = y;
    float escapeHeight = y;
    if (match.campaignObjective == 5) {
        const unsigned visited = M360_MatchMazeVisited();
        bool walkOffEdge = false;
        const M360MatchStage* stage = M360_MatchStageData();
        void* fighter = M360_FighterActive(0);
        if (g_mazeTargetRoom >= 0 && (visited & (1u << g_mazeTargetRoom))) g_mazeTargetRoom = -1;
        if (g_mazeTargetRoom < 0) {
            float best = 1.0e30f;
            for (unsigned room = 0; room < 6; ++room) {
                float px, py;
                if ((visited & (1u << room)) || !M360_MatchMazePoint(room, &px, &py)) continue;
                const float score = fabsf(px - x) + fabsf(py - y) * 0.6f;
                if (score < best) { best = score; g_mazeTargetRoom = static_cast<int>(room); }
            }
        }
        if (g_mazeTargetRoom >= 0) M360_MatchMazePoint(g_mazeTargetRoom, &targetX, &targetY);
        if (g_mazeDetour && y >= g_mazeDetourY) { g_mazeDetour = false; g_mazeDetourAge = 0; }
        if (g_mazeDetour && ++g_mazeDetourAge > 120 && fabsf(x - g_mazeDetourX) < 15.0f) {
            // A separate lower ceiling may obstruct the chosen corner.
            float alternateX, alternateY;
            if (PilotCeilingDetour(stage, x, y, targetX, targetY, &alternateX, &alternateY) &&
                fabsf(alternateX - g_mazeDetourX) > 5.0f) {
                g_mazeDetourX = alternateX; g_mazeDetourY = alternateY;
                g_mazeDetourAge = 0;
            }
        }
        if (!g_mazeDetour && targetY > y + 25.0f) {
            g_mazeDetour = PilotCeilingDetour(stage, x, y, targetX, targetY, &g_mazeDetourX, &g_mazeDetourY) != 0;
            g_mazeDetourAge = 0;
        }
        if (g_mazeDetour) {
            targetX = g_mazeDetourX;
            if (targetY < g_mazeDetourY + 30.0f) targetY = g_mazeDetourY + 30.0f;
        }
        const int floor = fighter ? M360_FighterFloorLine(fighter) : -1;
        if (floor >= 0 && static_cast<unsigned>(floor) < stage->lineCount && targetY < y - 70.0f) {
            const M360StageLine& ground = stage->lines[floor];
            if (!(ground.flags & M360_LINE_PLATFORM)) {
                float left = ground.x0, leftY = ground.y0, right = ground.x1, rightY = ground.y1;
                if (left > right) { left = ground.x1; leftY = ground.y1; right = ground.x0; rightY = ground.y0; }
                // Follow connected solid floor segments to a real edge.
                for (unsigned pass = 0; pass < stage->lineCount; ++pass) {
                    bool changed = false;
                    for (unsigned line = 0; line < stage->lineCount; ++line) {
                        const M360StageLine& candidate = stage->lines[line];
                        if (!(candidate.kind & M360_LINE_FLOOR) || (candidate.flags & M360_LINE_PLATFORM)) continue;
                        const float lx = candidate.x0 < candidate.x1 ? candidate.x0 : candidate.x1;
                        const float rx = candidate.x0 < candidate.x1 ? candidate.x1 : candidate.x0;
                        const float ly = candidate.x0 < candidate.x1 ? candidate.y0 : candidate.y1;
                        const float ry = candidate.x0 < candidate.x1 ? candidate.y1 : candidate.y0;
                        if (fabsf(rx - left) < 1.0f && fabsf(ry - leftY) < 1.0f && lx < left - 0.1f) {
                            left = lx; leftY = ly; changed = true;
                        }
                        if (fabsf(lx - right) < 1.0f && fabsf(ly - rightY) < 1.0f && rx > right + 0.1f) {
                            right = rx; rightY = ry; changed = true;
                        }
                    }
                    if (!changed) break;
                }
                const float leftCost = fabsf(x - left) + fabsf(targetX - left);
                const float rightCost = fabsf(x - right) + fabsf(targetX - right);
                targetX = leftCost < rightCost ? left - 15.0f : right + 15.0f;
                walkOffEdge = true;
            }
        }
        const int steer = PilotMazeSteer(targetX - x, walkOffEdge);
        pad->stickX = static_cast<s8>(steer);
        // Save aerial jumps until descent instead of consuming all of them
        // eight frames apart while still rising under a ceiling.
        // Walk along the authored ramp under an overhang before jumping
        // around its edge; early jumps just hit the roof and consume them.
        if (targetY > y + 25.0f && !g_routeJumpCooldown && (floor >= 0 || dy < -0.1f) &&
            PilotMazeCanJump(g_mazeDetour, targetX - x, floor)) {
            g_mazeJumpHold = 6;
            g_routeJumpCooldown = 18;
        }
        if (g_mazeJumpHold) { pad->button |= PAD_BUTTON_X; --g_mazeJumpHold; }
        // Down crouches on solid floors; use it only to drop through platforms.
        if (targetY < y - 35.0f && floor >= 0 &&
            static_cast<unsigned>(floor) < stage->lineCount &&
            (stage->lines[floor].flags & M360_LINE_PLATFORM) && tick % 30 < 4)
            pad->stickY = -127;
        if (match.frame % 300 == 0) {
            M360_MatchTrace("pilot.maze.target_room", static_cast<unsigned>(g_mazeTargetRoom));
            M360_MatchTrace("pilot.maze.target_x", static_cast<unsigned>(targetX));
            M360_MatchTrace("pilot.maze.floor", static_cast<unsigned>(floor));
            M360_MatchTrace("pilot.maze.ceiling_detour", g_mazeDetour ? 1 : 0);
        }
        return;
    }
    if (match.campaignObjective != 4) g_escapeTargetLine = -1;
    if (match.campaignObjective == 4) {
        const M360MatchStage* stage = M360_MatchStageData();
        float best = 1.0e30f;
        int selected = -1;
        targetX = x;
        void* fighter = M360_FighterActive(0);
        const int landedLine = fighter ? M360_FighterFloorLine(fighter) : -1;
        if (landedLine >= 0) g_escapeTargetLine = -1;
        if (g_escapeTargetLine >= 0 &&
            (static_cast<unsigned>(g_escapeTargetLine) >= stage->lineCount ||
             (fighter && M360_FighterFloorLine(fighter) == g_escapeTargetLine)))
            g_escapeTargetLine = -1;
        if (g_escapeTargetLine >= 0 && y < stage->lines[g_escapeTargetLine].y0 - 200.0f)
            g_escapeTargetLine = -1;
        for (unsigned i = 0; landedLine >= 0 && g_escapeTargetLine < 0 && i < stage->lineCount; ++i) {
            const M360StageLine& line = stage->lines[i];
            if (!(line.kind & M360_LINE_FLOOR)) continue;
            const float height = (line.y0 + line.y1) * 0.5f;
            const float center = (line.x0 + line.x1) * 0.5f;
            const float distance = center > x ? center - x : x - center;
            if (height < y + 8.0f || height > y + 100.0f || distance > 100.0f) continue;
            const float score = (height - y) * 2.0f + distance;
            if (score < best) {
                best = score; targetX = center; escapeHeight = height;
                selected = static_cast<int>(i);
            }
        }
        if (g_escapeTargetLine < 0) g_escapeTargetLine = selected;
        if (g_escapeTargetLine >= 0) {
            const M360StageLine& line = stage->lines[g_escapeTargetLine];
            escapeHeight = (line.y0 + line.y1) * 0.5f;
            targetX = (line.x0 + line.x1) * 0.5f;
        }
        if (match.frame % 300 == 0) {
            M360_MatchTrace("pilot.escape.target", static_cast<unsigned>(g_escapeTargetLine));
            M360_MatchTrace("pilot.escape.height", static_cast<unsigned>(escapeHeight));
            M360_MatchTrace("pilot.escape.floor", static_cast<unsigned>(fighter ? M360_FighterFloorLine(fighter) : -1));
        }
    }
    if (match.campaignObjective != 2 && match.campaignObjective != 0 && match.campaignObjective != 6) g_routeArena = false;
    if (match.campaignObjective == 2 || match.campaignObjective == 0 || match.campaignObjective == 6) {
        if (!g_routeArena) {
            g_routeArenaX = (match.posX[1] + match.posX[2] + match.posX[3]) / 3.0f;
            /* The route enemies enter thirty units above their checkpoint. */
            g_routeArenaY = (match.posY[1] + match.posY[2] + match.posY[3]) / 3.0f - 30.0f;
            if (match.campaignObjective == 0 || match.campaignObjective == 6) {
                const M360MatchStage* stage = M360_MatchStageData();
                float widest = 0.0f;
                g_routeArenaLeft = g_routeArenaX - 35.0f;
                g_routeArenaRight = g_routeArenaX + 35.0f;
                for (unsigned i = 0; i < stage->lineCount; ++i) {
                    const M360StageLine& line = stage->lines[i];
                    if (!(line.kind & M360_LINE_FLOOR) || (line.flags & M360_LINE_PLATFORM)) continue;
                    const float width = line.x1 > line.x0 ? line.x1 - line.x0 : line.x0 - line.x1;
                    if (width <= widest || width < 20.0f) continue;
                    widest = width;
                    g_routeArenaX = (line.x0 + line.x1) * 0.5f;
                    g_routeArenaY = (line.y0 + line.y1) * 0.5f;
                    g_routeArenaLeft = (line.x0 < line.x1 ? line.x0 : line.x1) + 8.0f;
                    g_routeArenaRight = (line.x0 > line.x1 ? line.x0 : line.x1) - 8.0f;
                }
            }
            g_routeArena = true;
        }
        float best = 1.0e30f;
        for (unsigned i = 1; i < match.fighters && i < 4; ++i) {
            if (!match.stocksRemaining[i]) continue;
            const float distance = match.posX[i] - x;
            if (distance * distance < best) {
                best = distance * distance;
                targetX = match.posX[i];
                targetY = match.posY[i];
            }
        }
        /* Fight on the checkpoint platform rather than chasing an enemy
         * already falling into a pit. The native CPU can return to us. */
        if (match.campaignObjective == 2) {
            if (targetX < g_routeArenaX - 35.0f) targetX = g_routeArenaX - 35.0f;
            if (targetX > g_routeArenaX + 35.0f) targetX = g_routeArenaX + 35.0f;
        } else {
            if (targetX < g_routeArenaLeft) targetX = g_routeArenaLeft;
            if (targetX > g_routeArenaRight) targetX = g_routeArenaRight;
        }
        if (tick % 40 < 3) pad->button |= PAD_BUTTON_A;
        if (tick % 90 < 3) pad->substickX = targetX < x ? -127 : 127;
        if (y < g_routeArenaY - 25.0f) {
            targetX = g_routeArenaX;
            pad->button &= ~PAD_BUTTON_A;
            pad->substickX = 0;
        }
    }
    const int direction = targetX < x ? -1 : 1;
    if (targetX - x > 10.0f || targetX - x < -10.0f)
        pad->stickX = static_cast<s8>(direction * 127);
    if ((match.campaignObjective == 2 || match.campaignObjective == 0 || match.campaignObjective == 6) && y < g_routeArenaY - 25.0f) {
        if (tick % 8 < 7) pad->button |= PAD_BUTTON_X;
        return;
    }
    if (match.campaignObjective == 0 || match.campaignObjective == 6) {
        /* Ordinary combat stays on the main solid floor. Use native full
         * hops/multijumps to reach an opponent on a higher platform. */
        if (targetY > y + 16.0f && targetX - x < 60.0f && targetX - x > -60.0f && tick % 8 < 7)
            pad->button |= PAD_BUTTON_X;
        return;
    }
    if (match.campaignObjective == 4) {
        float steer = (targetX - x) * 16.0f;
        if (steer > 127.0f) steer = 127.0f;
        if (steer < -127.0f) steer = -127.0f;
        pad->stickX = static_cast<s8>(steer);
        /* Hold through jump squat for a full hop. Multijump characters can
         * request their next jump when the original animation permits it. */
        if (escapeHeight > y && tick % 8 < 7) pad->button |= PAD_BUTTON_X;
        return;
    }
    float nearY, aheadY;
    unsigned nearLine, aheadLine;
    const bool groundNear = M360_MatchGroundBelow(x, y + 4.0f, 12.0f, &nearY, &nearLine) != 0;
    const bool ahead = M360_MatchGroundBelow(x + direction * 22.0f, y + 100.0f,
                                           150.0f, &aheadY, &aheadLine) != 0;
    const bool jump = match.campaignObjective == 4 ?
        (escapeHeight > y && (groundNear || dy <= 0.0f)) :
        groundNear ? (!ahead || aheadY > y + 4.0f)
                           : (dy <= 0.0f && (!ahead || y < aheadY + 12.0f));
    if (jump && !g_routeJumpCooldown) {
        pad->button |= PAD_BUTTON_X;
        g_routeJumpCooldown = 20;
    }
}

u16 ParseButtons(const char* text)
{
    u16 buttons = 0;
    for (; *text; ++text) {
        switch (*text) {
        case 'A': buttons |= PAD_BUTTON_A; break;
        case 'B': buttons |= PAD_BUTTON_B; break;
        case 'X': buttons |= PAD_BUTTON_X; break;
        case 'Y': buttons |= PAD_BUTTON_Y; break;
        case 'Z': buttons |= PAD_TRIGGER_Z; break;
        case 'L': buttons |= PAD_TRIGGER_L; break;
        case 'R': buttons |= PAD_TRIGGER_R; break;
        case 'S': buttons |= PAD_BUTTON_START; break;
        case 'u': buttons |= PAD_BUTTON_UP; break;
        case 'd': buttons |= PAD_BUTTON_DOWN; break;
        case 'l': buttons |= PAD_BUTTON_LEFT; break;
        case 'r': buttons |= PAD_BUTTON_RIGHT; break;
        default: break;
        }
    }
    return buttons;
}

void LoadScript()
{
    g_scriptLoaded = true;
    HANDLE file = CreateFileA("game:\\input-script.txt", GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return;
    static char text[32768];
    DWORD read = 0;
    ReadFile(file, text, sizeof(text) - 1, &read, 0);
    CloseHandle(file);
    text[read] = 0;
    char* line = text;
    while (*line && g_scriptCount < 512) {
        char* end = line;
        while (*end && *end != '\n')
            ++end;
        const char next = *end;
        *end = 0;
        if (*line != '#') {
            ScriptStep& step = g_script[g_scriptCount];
            ZeroMemory(&step, sizeof(step));
            int snap = 0;
            unsigned frames = 0;
            int sx = 0, sy = 0, cx = 0, cy = 0;
            char buttons[32] = "-";
            if (!strcmp(line, "routepilot")) {
                g_routePilot = true;
            } else if (!strncmp(line, "loop", 4)) {
                g_scriptLoop = true;
            } else if (sscanf(line, "snap %d %u", &snap, &frames) == 2) {
                step.snap = snap;
                step.frames = frames;
                ++g_scriptCount;
            } else if (sscanf(line, "%u %d %d %d %d %31s", &frames, &sx, &sy, &cx, &cy, buttons) >= 5) {
                step.frames = frames;
                step.stickX = static_cast<s8>(sx);
                step.stickY = static_cast<s8>(sy);
                step.subX = static_cast<s8>(cx);
                step.subY = static_cast<s8>(cy);
                step.buttons = ParseButtons(buttons);
                ++g_scriptCount;
            }
        }
        line = next ? end + 1 : end;
    }
    M360_MatchTrace("script.steps", g_scriptCount);
}

void ApplyScript(PADStatus* status)
{
    if (!g_scriptLoaded)
        LoadScript();
    if (g_routePilot) {
        ApplyRoutePilot(status);
        return;
    }
    while (g_scriptIndex < g_scriptCount && !g_scriptLeft && !g_scriptHold) {
        const ScriptStep& step = g_script[g_scriptIndex];
        if (step.snap) {
            g_scriptHold = step.frames;
            M360_MatchTrace("script.snap", static_cast<unsigned>(step.snap));
            ++g_scriptIndex;
            break;
        }
        g_scriptLeft = step.frames;
        M360_MatchTrace("script.step", g_scriptIndex);
        if (!step.frames)
            ++g_scriptIndex;
    }
    if (g_scriptLoop && g_scriptCount && g_scriptIndex >= g_scriptCount && !g_scriptLeft && !g_scriptHold) {
        /* "loop": replay the steps for long soak runs. */
        M360_MatchTrace("script.pass", ++g_scriptPasses);
        g_scriptIndex = 0;
        ApplyScript(status);
        return;
    }
    if (g_scriptIndex >= g_scriptCount && !g_scriptLeft && !g_scriptHold) {
        if (g_scriptCount && g_scriptIndex++ == g_scriptCount)
            M360_MatchTrace("script.done", g_scriptCount);
        return;
    }
    if (g_scriptHold) {
        --g_scriptHold;
        if (g_scriptIndex == 0)
            return;
    }
    const unsigned current = g_scriptLeft ? g_scriptIndex : g_scriptIndex - 1;
    const ScriptStep& step = g_script[current];
    if (!step.snap) {
        status->err = 0;
        status->button |= step.buttons;
        if (step.stickX) status->stickX = step.stickX;
        if (step.stickY) status->stickY = step.stickY;
        if (step.subX) status->substickX = step.subX;
        if (step.subY) status->substickY = step.subY;
        if (step.buttons & PAD_TRIGGER_L) status->triggerLeft = 255;
        if (step.buttons & PAD_TRIGGER_R) status->triggerRight = 255;
    }
    if (g_scriptLeft && !g_scriptHold && --g_scriptLeft == 0)
        ++g_scriptIndex;
}
#endif

} // namespace

extern "C" int M360_InputScriptHolding(void)
{
#ifdef M360_INPUT_SCRIPT
    return g_scriptHold != 0;
#else
    return 0;
#endif
}

extern "C" u32 OSDisableInterrupts(void)
{
    return 1;
}

extern "C" void OSRestoreInterrupts(u32)
{
}

extern "C" int OSGetResetSwitchState(void)
{
    return 0;
}

extern "C" int PADRead(PADStatus* status)
{
    u32 connected = 0;
    for (DWORD channel = 0; channel < 4; ++channel) {
        ZeroMemory(&status[channel], sizeof(status[channel]));
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));
        if (XInputGetState(channel, &state) != ERROR_SUCCESS) {
            status[channel].err = -1;
            continue;
        }

        connected |= PAD_CHAN0_BIT >> channel;
        status[channel].err = 0;
        status[channel].button = TranslateButtons(state.Gamepad.wButtons);
        status[channel].stickX = ScaleThumb(state.Gamepad.sThumbLX);
        status[channel].stickY = ScaleThumb(state.Gamepad.sThumbLY);
        status[channel].substickX = ScaleThumb(state.Gamepad.sThumbRX);
        status[channel].substickY = ScaleThumb(state.Gamepad.sThumbRY);
        status[channel].triggerLeft = state.Gamepad.bLeftTrigger;
        status[channel].triggerRight = state.Gamepad.bRightTrigger;
        status[channel].analogA =
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 255 : 0;
        status[channel].analogB =
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? 255 : 0;
        if (state.Gamepad.bLeftTrigger > 30)
            status[channel].button |= PAD_TRIGGER_L;
        if (state.Gamepad.bRightTrigger > 30)
            status[channel].button |= PAD_TRIGGER_R;
    }
#ifdef M360_INPUT_SCRIPT
    ApplyScript(&status[0]);
    if (status[0].err == 0)
        connected |= PAD_CHAN0_BIT;
#endif
    return static_cast<int>(connected);
}

extern "C" int PADReset(unsigned long)
{
    return 1;
}

extern "C" int PADRecalibrate(u32)
{
    return 1;
}

extern "C" int PADInit(void)
{
    return 1;
}

extern "C" void HSD_PadRumbleInterpret(void)
{
}

extern "C" void HSD_PadRumbleRemoveAll(void)
{
}

extern "C" void HSD_PadRumbleOffN(u8 channel)
{
    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(vibration));
    XInputSetState(channel, &vibration);
}

extern "C" void HSD_PadRumbleInit(u16, void*)
{
}

extern "C" void M360_HSDPadInit(void)
{
    ZeroMemory(g_padQueue, sizeof(g_padQueue));
    HSD_PadInit(2, g_padQueue, 0, 0);
}
