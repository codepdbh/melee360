#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "melee_classic_matchups.h"
#include "melee_adventure_matchups.h"
#include "stage_identity_reference.h"
#include "../../src/xdk/match_xdk.h"

enum { kMaxFighters = 4, kStartingStocks = 3, kCampaignRounds = 5,
       kGameModeClassic = 3, kGameModeAdventure = 4,
       kPhaseSelect = 0, kPhaseFight = 1, kRespawnFrames = 60,
       kMaxCostumes = 6, kKindCount = 11, kSelectableKinds = 6, kStageCount = 21, kTimeOptionCount = 2,
       HSD_GOBJ_CLASS_STAGE = 1 };
struct CostumeDesc { const char* costumeDat[6]; const char* costumeJoint[6]; int kind; };
const CostumeDesc s_kinds[] = {
    { { "a", "b", "c", "d", "e", NULL }, { "a", "b", "c", "d", "e", NULL }, 0 },
    { { "a", "b", "c", "d", NULL, NULL }, { "a", "b", "c", "d", NULL, NULL }, 17 },
    { { "a", "b", "c", "d", "e", "f" }, { "a", "b", "c", "d", "e", "f" }, 14 },
    { { "a" }, { "a" }, 3 }, { { "a" }, { "a" }, 4 }, { { "a" }, { "a" }, 6 },
    { { "a" }, { "a" }, 12 }, { { "a" }, { "a" }, 23 }, { { "a" }, { "a" }, 15 },
    { { "a" }, { "a" }, 29 }, { { "a" }, { "a" }, 30 }
};
struct StageDesc { int grkind; };
const StageDesc s_stages[] = { {36}, {37}, {28}, {10}, {12}, {29}, {7}, {30}, {5}, {2}, {4}, {8}, {13}, {14}, {16}, {20}, {18}, {31}, {33}, {32}, {34} };
struct StageParam { unsigned stkind; int x4; unsigned xC; };
typedef unsigned StKind;
struct GroundParam { StageParam* stage_params; int stage_param_count; };
StageParam mockStageParams[64];
GroundParam mockGround;
GroundParam* s_param = &mockGround;
void LoadItemTable() {}
unsigned mockRand;
int HSD_Randi(int count) { assert(count > 0); return (int) (mockRand % (unsigned) count); }
struct HSD_GObj {};
struct Fighter { float x, y; unsigned reads, deaths, rebirths; } fighters[4];
unsigned s_gameMode, s_campaignRound, s_campaignStocks, s_stocks;
unsigned s_classicMatchups[kCampaignRounds], s_classicStKind;
int s_campaignRetryPending, s_campaignTimedOut;
unsigned s_adventureLives[4];
unsigned s_campaignClearFrames;
unsigned s_adventureElapsedFrames;
int s_adventureLuigi;
int s_adventureCheckpoint;
float s_routeBlast[4] = {-100, 100, -100, 100};
float s_routeFightBlast[4] = {-100, 100, -100, 100};
float s_routeCamera[4] = {-170, 170, -60, 120};
float s_routeFightCamera[4] = {-60, 60, -60, 60};
struct Vec3 { float x, y, z; };
bool routePoints;
bool racePoints;
int PointPosition(int index, Vec3* out) {
    if (racePoints && ((index >= 5 && index <= 7) || index == 0x99)) {
        out->x = index == 0x99 ? 80.0f : (index - 5) * 20.0f + 10.0f;
        out->y = index == 0x99 ? 0.0f : (index - 4) * 20.0f; out->z = 0;
        return 1;
    }
    if (!routePoints || (index != 0xBD && index != 0x99)) return 0;
    out->x = index == 0xBD ? 0.0f : 80.0f; out->y = out->z = 0;
    return 1;
}
float fabsf(float value) { return value < 0 ? -value : value; }
float encounterScale[4];
void M360_FighterSetEncounter(void* object, float scale, int) {
    encounterScale[(Fighter*) object - fighters] = scale;
}

unsigned gateHidden;
void HideMapJoint(int map, int joint) { assert(map == 3 && joint == 0x53); ++gateHidden; }
unsigned M360_MatchStageCount() { return kStageCount - 4; }
struct MapJoint { int floor_start, floor_count, ceiling_start, ceiling_count, right_wall_start, right_wall_count, left_wall_start, left_wall_count, dynamic_start, dynamic_count; };
MapJoint mockGroups[14];
struct MapLine { unsigned hi_flags; } mockLines[64];
struct MapCollData { int joint_count; MapJoint* joints; MapLine* lines; } mockColl = {14, mockGroups, mockLines};
MapCollData* s_coll = &mockColl;
int mockFloorLine = -1;
int M360_FighterFloorLine(void*) { return mockFloorLine; }
void M360_HudAddFighter(unsigned) {}
void M360_HudRefreshFighterTags() {}
unsigned s_stocksRemaining[4], s_stocksLost[4], s_respawn[4], s_fighterCount;
void* s_fighters[4];
int s_score[4], s_human[4], s_matchOver, s_draw;
unsigned s_winner, s_frame, s_autoPlayed, s_autoResultFrames;
unsigned s_slotCount, s_selKind[4], s_selCostume[4], s_selReady[4];
int s_selHuman[4], s_itemFreq, s_selProbeP2;
unsigned s_loadFailedSlot, s_builtStage, s_stageIndex, s_cpuLevel;
float s_selPrevStick[4], s_selPrevSub, mockStickX, mockSubY;
int failSlot = -1;
unsigned cleanupCalls, hudStarts;
int s_active, s_phase, s_paused, s_holdTraced, s_debugHitboxes, s_suddenDeath;
unsigned s_disconnectedControllers;
int s_rematchPending;
unsigned s_modelCount, s_collBindCount, s_timeOption, g_m360Crumb;
const unsigned kTimeOptions[] = { 0, 1 };
void* s_models[8];
struct HSD_JObj { unsigned flags; };
enum { JOBJ_HIDDEN = 0x10 };
unsigned s_mazeGoal, s_mazeVisited, s_mazeFinishFrames;
int s_mazeRoom = -1;
Vec3 s_mazePoints[6], s_mazeSpawns[6];
HSD_JObj mockSymbols[6];
HSD_JObj* s_mazeSymbols[6];
float s_mazeRadiusX = 35, s_mazeRadiusY = 35;
unsigned mazeHidden, mazeVisibility;
void HSD_JObjSetFlagsAll(HSD_JObj* joint, unsigned flags) { ++mazeHidden; joint->flags |= flags; }
void HSD_JObjClearFlagsAll(HSD_JObj* joint, unsigned flags) { joint->flags &= ~flags; }
void MazeMapJointVisible(int, int, int) { ++mazeVisibility; }
void M360_FighterBuildIslands() {}
int InitMaze() {
    s_mazeGoal = 5; s_mazeVisited = s_mazeFinishFrames = 0; s_mazeRoom = -1;
    for (unsigned i = 0; i < 6; ++i) {
        s_mazePoints[i].x = s_mazeSpawns[i].x = i * 300.0f;
        s_mazePoints[i].y = 0; s_mazeSpawns[i].y = 10;
        s_mazeSymbols[i] = &mockSymbols[i];
    }
    return 1;
}
static void MazeFrame(float, float);
void* s_lobj;
M360MatchAutoConfig s_auto;
M360MatchStage s_stage;
unsigned mockButtons, stockNotices[4], gameEnds;
unsigned connectedControllers = 1, secondaryButtons[4];
int lastPausePort;
unsigned M360_MatchPadTriggered() { return mockButtons; }
unsigned M360_MatchPadHeld() { return mockButtons; }
unsigned M360_MatchPadTriggeredPort(unsigned port) { return port == 0 ? mockButtons : secondaryButtons[port]; }
float M360_MatchPadStickXPort(unsigned) { return mockStickX; }
float M360_MatchPadSubStickYPort(unsigned) { return mockSubY; }
int M360_MatchControllerConnected(unsigned port) { return (connectedControllers & (1u << port)) != 0; }
void M360_MatchTrace(const char*, unsigned) {}
void M360_HudFrame(unsigned) {}
void M360_HudPause(int, int port) { lastPausePort = port; }
void M360_HudStockLost(unsigned slot) { ++stockNotices[slot]; }
void M360_HudGameEnd(int) { ++gameEnds; }
int M360_HudGameEndDone() { return 1; }
int M360_HudFightStarted() { return 1; }
unsigned M360_HudFightFrames() { return s_frame; }
int M360_InputScriptHolding() { return 0; }
void M360_MatchGetStatus(M360MatchStatus* status) { memset(status, 0, sizeof(*status)); }
void HSD_JObjAnimAll(void*) {}
void HSD_LObjAnimAll(void*) {}
void HSD_GObj_RunProcs() {}
void UpdateMapColl() {}
void CamUpdate(int) {}
void TraceHeap(const char*, const char*) {}
void M360_FighterFollowFloors() {}
void M360_FighterSetPort(void*, int) {}
void M360_FighterTraceCpu(void*) {}
unsigned M360_FighterKindCount() { return kKindCount; }
void M360_FighterSelect(int, unsigned, unsigned) {}
void M360_FighterSetCpuLevel(unsigned) {}
void M360_FighterEffectsInit() {}
void M360_FighterResetMatch() {}
int M360_MatchLoad() { return 1; }
void ApplyAutoConfig() {}
void M360_AudioStopAllSfx() {}
void M360_HsdRenderClearTextures() {}
void FreeAllGObjs() { ++cleanupCalls; }
int BuildStage(unsigned stage) {
    s_stageIndex = s_builtStage = stage;
    s_stage.lineCount = 57;
    memset(mockGroups, 0, sizeof(mockGroups));
    mockGroups[0].floor_count = 44;
    for (unsigned i = 1; i < 14; ++i) {
        mockGroups[i].floor_start = 43 + i;
        mockGroups[i].floor_count = 1;
    }
    for (unsigned i = 0; i < 64; ++i) mockLines[i].hi_flags = i % 2 ? 1 : 4;
    mockGround.stage_params = mockStageParams;
    mockGround.stage_param_count = 1;
    mockStageParams[0].stkind = 0;
    mockStageParams[0].x4 = 11; mockStageParams[0].xC = 22;
    for (unsigned i = 0; i < sizeof(g_m360ClassicNormal) / sizeof(g_m360ClassicNormal[0]); ++i) {
        if (g_m360ClassicNormal[i].grkind == s_stages[stage].grkind) {
            StageParam& row = mockStageParams[mockGround.stage_param_count++];
            row.stkind = g_m360ClassicNormal[i].stkind;
            row.x4 = 1000 + row.stkind; row.xC = 2000 + row.stkind;
        }
    }
    for (unsigned i = 0; i < M360_ADVENTURE_ROUNDS; ++i) {
        if (g_m360Adventure[i].grkind == s_stages[stage].grkind) {
            StageParam& row = mockStageParams[mockGround.stage_param_count++];
            row.stkind = g_m360Adventure[i].stkind;
            row.x4 = 1000 + row.stkind; row.xC = 2000 + row.stkind;
        }
    }
    return 1;
}
void* M360_FighterSpawn(int slot, float, float, float, int) {
    return slot == failSlot ? NULL : &fighters[slot];
}
void it_8026D018() {}
HSD_GObj* GObj_Create(int, int, int) { return NULL; }
void DebugRender(HSD_GObj*, int) {}
void GObj_SetupGXLink(HSD_GObj*, void (*)(HSD_GObj*, int), int, int) {}
unsigned hudTimeLimit;
int hudStockMatch;
void M360_HudStart(unsigned, unsigned, unsigned seconds, int stockMatch) {
    ++hudStarts; hudTimeLimit = seconds; hudStockMatch = stockMatch;
}
void* M360_FighterActive(int) { return NULL; }
void* M360_FighterFollower(int) { return NULL; }
void M360_FighterSleep(void*) {}
void M360_FighterRespawn(void* f, float x, float y) {
    ((Fighter*) f)->x = x; ((Fighter*) f)->y = y;
}
void M360_FighterRebirth(void* f) { ++((Fighter*) f)->rebirths; }
void M360_FighterSetDead(void* f) { ++((Fighter*) f)->deaths; }
void M360_FighterSetDamage(void*, float) {}
int M360_FighterLastAttacker(void*) { return -1; }
void M360_FighterGetState(void* f, float* x, float* y, float* facing,
                          unsigned* motion, unsigned* damage) {
    Fighter* fighter = (Fighter*) f;
    ++fighter->reads;
    *x = fighter->x; *y = fighter->y; *facing = 1; *motion = *damage = 0;
}

// Extracted from the actual native match source by the runner.
#include "match_rules_original.h"

static void Reset(unsigned count) {
    memset(fighters, 0, sizeof(fighters));
    memset(s_stocksRemaining, 0, sizeof(s_stocksRemaining));
    memset(s_stocksLost, 0, sizeof(s_stocksLost));
    memset(s_respawn, 0, sizeof(s_respawn));
    memset(s_score, 0, sizeof(s_score));
    memset(s_human, 0, sizeof(s_human));
    memset(stockNotices, 0, sizeof(stockNotices));
    memset(s_selKind, 0, sizeof(s_selKind));
    memset(s_selCostume, 0, sizeof(s_selCostume));
    memset(s_selReady, 0, sizeof(s_selReady));
    memset(s_selHuman, 0, sizeof(s_selHuman));
    memset(s_selPrevStick, 0, sizeof(s_selPrevStick));
    memset(&s_auto, 0, sizeof(s_auto));
    memset(&s_stage, 0, sizeof(s_stage));
    s_stage.blastLeft = s_stage.blastBottom = -100;
    s_stage.blastRight = s_stage.blastTop = 100;
    s_fighterCount = count;
    for (unsigned i = 0; i < 4; ++i) {
        s_fighters[i] = i < count ? &fighters[i] : NULL;
        s_stocksRemaining[i] = i < count ? 1 : 0;
    }
    s_active = 1; s_phase = kPhaseFight;
    s_slotCount = count;
    s_loadFailedSlot = cleanupCalls = hudStarts = 0;
    failSlot = -1;
    mockStickX = mockSubY = s_selPrevSub = 0;
    s_frame = mockButtons = gameEnds = 0;
    mockRand = 0;
    s_campaignClearFrames = s_adventureElapsedFrames = 0;
    s_matchOver = s_draw = s_paused = s_suddenDeath = 0;
    s_disconnectedControllers = s_rematchPending = 0;
    s_campaignRetryPending = s_campaignTimedOut = 0;
    connectedControllers = 1;
    memset(secondaryButtons, 0, sizeof(secondaryButtons));
    s_timeOption = 0; s_stocks = 4;
    M360_MatchSetMode(2, 0);
    BuildStage(0);
}

int main() {
    const unsigned stageIdCount = sizeof(referenceStageIds) / sizeof(referenceStageIds[0]);
    assert(stageIdCount == sizeof(g_m360StageGroundKinds) / sizeof(g_m360StageGroundKinds[0]));
    for (unsigned i = 0; i < stageIdCount; ++i)
        assert(Stage_8022519C(i) == referenceStageIds[i].grkind);
    assert(Stage_8022519C(~0u) == Gr_Kind_Unk00);
    assert(Stage_8022519C(stageIdCount) == Gr_Kind_Unk00);
    Reset(4);
    for (unsigned i = 0; i < kStageCount; ++i) {
        BuildStage(i);
        assert((int) Stage_8022519C(Stage_80225194()) == s_stages[i].grkind);
        // Preserve an authored variant when present in the stage parameter row.
        for (unsigned id = stageIdCount; id-- > 0;) {
            if ((int) referenceStageIds[id].grkind == s_stages[i].grkind) {
                mockStageParams[0].stkind = id;
                assert(Stage_80225194() == id);
                break;
            }
        }
        mockStageParams[0].stkind = ~0u;
        assert((int) Stage_8022519C(Stage_80225194()) == s_stages[i].grkind);
    }
    s_gameMode = kGameModeClassic;
    for (const auto& encounter : g_m360ClassicNormal) {
        s_classicStKind = encounter.stkind;
        assert(Stage_80225194() == encounter.stkind);
        assert((int) Stage_8022519C(Stage_80225194()) == encounter.grkind);
    }
    s_gameMode = kGameModeAdventure;
    for (const auto& encounter : g_m360Adventure) {
        s_classicStKind = encounter.stkind;
        assert(Stage_80225194() == encounter.stkind);
        assert((int) Stage_8022519C(Stage_80225194()) == encounter.grkind);
    }
    printf("PASS: all %u original stage identities, 21 native grounds, archive rows and campaign variants\n", stageIdCount);
    for (s_stageIndex = 0; s_stageIndex < kStageCount; ++s_stageIndex)
        assert(M360_MatchGroundKind() == s_stages[s_stageIndex].grkind);
    s_stageIndex = 0;
    Reset(4);
    s_gameMode = kGameModeAdventure; s_campaignRound = 0; s_cpuLevel = 3;
    assert(M360_MatchCombatRatio(0, 0) == 1.0f);
    assert(M360_MatchCombatRatio(0, 1) == 1.0f);
    assert(M360_MatchCombatRatio(1, 0) == 0.5f);
    assert(M360_MatchCombatRatio(1, 1) == 4.0f);
    assert(M360_MatchCpuLevel(0) == 3);
    assert(M360_MatchCpuLevel(1) == 2);
    assert(M360_MatchCpuLevel(2) == 2);
    assert(M360_MatchCpuLevel(3) == 2);
    assert(M360_MatchCpuKind(0) == 4);
    assert(M360_MatchCpuKind(1) == 23);
    mockRand = 3; assert(M360_MatchCpuKind(1) == 24); mockRand = 0;
    s_slotCount = 1;
    assert(M360_MatchCpuLevel(2) == 2);
    assert(M360_MatchCpuLevel(3) == 2);
    s_slotCount = 4;
    s_cpuLevel = 9;
    assert(M360_MatchCombatRatio(1, 0) == 0.8f);
    assert(M360_MatchCombatRatio(1, 1) == 1.7f);
    s_campaignRound = 9; s_cpuLevel = 5;
    assert(M360_MatchCombatRatio(1, 0) == 0.8f);
    assert(M360_MatchCombatRatio(1, 1) == 3.0f);
    assert(M360_MatchCpuLevel(1) == 4);
    assert(M360_MatchCpuLevel(3) == 4);
    s_campaignRound = 1;
    assert(M360_MatchCpuLevel(1) == 5);
    assert(M360_MatchCpuLevel(2) == 4);
    assert(M360_MatchCpuKind(1) == 16);
    assert(M360_MatchCpuKind(2) == 4);
    s_campaignRound = 18;
    assert(M360_MatchCpuLevel(1) == 5);
    assert(M360_MatchCpuLevel(2) == 3);
    assert(M360_MatchCpuKind(1) == 27);
    assert(M360_MatchCpuKind(2) == 27);
    s_campaignRound = 7;
    assert(M360_MatchCombatRatio(1, 1) == 1.0f);
    s_gameMode = 0;
    assert(M360_MatchCombatRatio(1, 0) == 1.0f);
    assert(M360_MatchCpuLevel(1) == 5);
    s_gameMode = kGameModeAdventure; s_cpuLevel = 3;
    mockButtons = 0x40;
    SelectInput(0, 0); assert(s_cpuLevel == 5);
    SelectInput(0, 0); assert(s_cpuLevel == 7);
    SelectInput(0, 0); assert(s_cpuLevel == 9);
    SelectInput(0, 0); assert(s_cpuLevel == 1);
    SelectInput(0, 0); assert(s_cpuLevel == 3);
    s_gameMode = 2;
    SelectInput(0, 0); assert(s_cpuLevel == 4);
    Reset(4);
    assert(M360_FighterCostumeCount(0) == 5);
    assert(M360_FighterCostumeCount(1) == 4);
    assert(M360_FighterCostumeCount(2) == 6);
    assert(M360_FighterCostumeCount(99) == 1);
    for (unsigned i = 0; i < 4; ++i) { s_selKind[i] = 1; s_selCostume[i] = 5; }
    ResolveCostumes();
    for (unsigned i = 0; i < 4; ++i) {
        assert(s_selCostume[i] < 4);
        for (unsigned j = 0; j < i; ++j) assert(s_selCostume[i] != s_selCostume[j]);
    }
    Reset(2);
    s_selCostume[0] = 4;
    mockButtons = 2; // Mario's fifth color becomes valid when switching to Luigi.
    SelectInput(0, 0);
    assert(s_selKind[0] == 1 && s_selCostume[0] == 0);
    mockButtons = 0x800;
    SelectInput(0, 0);
    assert(s_selCostume[0] == 3);
    mockButtons = 0x400;
    SelectInput(0, 0);
    assert(s_selCostume[0] == 0);
    s_selReady[1] = s_selReady[2] = 1;
    mockButtons = 0x20; // Expanding the match clears old CPU confirmations.
    SelectInput(0, 0);
    assert(s_slotCount == 3 && !s_selReady[1] && !s_selReady[2]);
    Reset(2);
    s_phase = kPhaseSelect;
    s_selHuman[1] = 1;
    s_selReady[0] = 1;
    SelectFrame(); // Unplugged P2 becomes a CPU that P1 can select.
    assert(!s_selHuman[1] && !s_selReady[1] && P1Slot() == 1);
    Reset(4);
    s_selHuman[2] = s_selHuman[3] = 1;
    mockButtons = 0x20;
    SelectInput(0, 0);
    assert(s_slotCount == 2 && !s_selHuman[2] && !s_selHuman[3]);
    Reset(2);
    s_selReady[0] = s_selReady[1] = 1;
    failSlot = 1;
    StartFight();
    assert(s_active && s_phase == kPhaseSelect && s_loadFailedSlot == 2);
    assert(cleanupCalls == 1 && !hudStarts && !s_fighterCount);
    assert(!s_fighters[0] && !s_fighters[1] && !s_selReady[0] && !s_selReady[1]);
    failSlot = -1;
    StartFight();
    assert(s_phase == kPhaseFight && s_fighterCount == 2 && hudStarts == 1);
    assert(!s_loadFailedSlot);
    Reset(2);
    s_auto.enabled = 1; failSlot = 0;
    StartFight();
    assert(!s_active); // An unattended load failure exits instead of waiting forever.
    puts("PASS: costume limits/wrapping, distinct colors, player count and partial load recovery");
    Reset(2);
    M360_MatchSetMode(kGameModeClassic, 0);
    assert(StartingStocks(0) == 3 && StartingStocks(1) == 1);
    s_stocksRemaining[0] = StartingStocks(0);
    assert(LoseStock(0));
    M360_MatchSetMode(kGameModeClassic, 1);
    assert(StartingStocks(0) == 2); // Next round keeps the remaining lives.
    M360_MatchSetMode(kGameModeClassic, 0);
    assert(StartingStocks(0) == 3); // A new campaign resets them.
    M360_MatchSetMode(kGameModeAdventure, 0);
    assert(StartingStocks(0) == 3);
    M360_MatchSetMode(2, 0);
    s_stocks = 7;
    assert(StartingStocks(0) == 7 && StartingStocks(1) == 7);

    Reset(4);
    s_matchOver = s_draw = 1;
    s_human[0] = s_human[2] = 1; // Includes a player who joined during combat.
    s_stocks = 8; s_timeOption = 1; s_itemFreq = 2; s_cpuLevel = 7; s_stageIndex = 5;
    for (unsigned i = 0; i < 4; ++i) { s_selKind[i] = 1; s_selCostume[i] = i; }
    mockButtons = 0x100;
    assert(M360_MatchFrame() == M360_MATCH_RESTART && s_rematchPending);
    M360_MatchLeave();
    M360_MatchEnter();
    assert(s_phase == kPhaseFight && s_fighterCount == 4 && !s_rematchPending);
    assert(s_human[0] && !s_human[1] && s_human[2] && !s_human[3]);
    assert(s_stocks == 8 && s_timeOption == 1 && s_itemFreq == 2 && s_cpuLevel == 7 && s_stageIndex == 5);
    assert(!s_matchOver && !s_draw && !s_paused && s_frame == 0);
    for (unsigned i = 0; i < 4; ++i)
        assert(s_selKind[i] == 1 && s_selCostume[i] == i && s_selReady[i]);
    puts("PASS: VS rematch after draw retains roster, colors, rules and human slots");

    Reset(2);
    s_human[0] = s_human[1] = 1;
    connectedControllers = 3;
    secondaryButtons[1] = 0x1000; // P2 can pause.
    M360_MatchFrame();
    assert(s_paused && s_frame == 0 && lastPausePort == 1);
    secondaryButtons[1] = 0;
    M360_MatchFrame();
    assert(s_paused && s_frame == 0);
    secondaryButtons[1] = 0x1000;
    M360_MatchFrame();
    assert(!s_paused && s_frame == 1);
    secondaryButtons[1] = 0;
    connectedControllers = 1; // P2 disconnects: no simulation/time advance.
    M360_MatchFrame();
    assert(s_paused && s_disconnectedControllers == 2 && s_frame == 1);
    mockButtons = 0x1000; // P1 cannot resume before P2 reconnects.
    M360_MatchFrame();
    assert(s_paused && s_frame == 1);
    connectedControllers = 3;
    mockButtons = 0;
    M360_MatchFrame();
    assert(s_paused && !s_disconnectedControllers && s_frame == 1);
    mockButtons = 0x1000;
    M360_MatchFrame();
    assert(!s_paused && s_frame == 2);
    connectedControllers = 2; // P1 disconnects; P2 can still leave via B.
    secondaryButtons[1] = 0x200;
    mockButtons = 0;
    assert(M360_MatchFrame() == M360_MATCH_TO_MENU);
    assert(s_paused && s_disconnectedControllers == 1 && s_frame == 2);
    puts("PASS: secondary pause, disconnect freeze, explicit reconnect/resume and exit from another pad");

    Reset(4);
    fighters[0].x = 200;
    M360_MatchFrame();
    assert(!s_matchOver && s_stocksRemaining[0] == 0);
    const unsigned reads = fighters[0].reads;
    for (unsigned i = 0; i < 5; ++i) M360_MatchFrame();
    assert(s_stocksLost[0] == 1 && stockNotices[0] == 1);
    assert(fighters[0].deaths == 1 && fighters[0].reads == reads);
    assert(!LoseStock(0)); // No underflow from a repeated stop/fall.

    Reset(2);
    M360_MatchSetMode(kGameModeClassic, 0);
    fighters[0].x = fighters[1].x = 200;
    M360_MatchFrame();
    assert(s_matchOver && s_draw && gameEnds == 1);
    mockButtons = 0x100;
    assert(M360_MatchFrame() == M360_MATCH_RESTART && s_campaignRetryPending); // Draw retries the same phase.

    Reset(2);
    M360_MatchSetMode(kGameModeClassic, 0);
    s_stocksRemaining[0] = 2;
    fighters[1].x = 200;
    M360_MatchFrame();
    assert(s_matchOver && !s_draw && s_winner == 0 && gameEnds == 1);
    mockButtons = 0x100;
    assert(M360_MatchFrame() == M360_MATCH_NEXT_ROUND);

    // Exercise the actual entry/spawn/frame/leave functions through every round.
    for (unsigned mode = kGameModeClassic; mode <= kGameModeClassic; ++mode) {
        Reset(2);
        unsigned lives = 3;
        for (unsigned round = 0; round < kCampaignRounds; ++round) {
            M360_MatchSetMode(mode, round);
            M360_MatchEnter();
            if (round == 0) {
                assert(s_phase == kPhaseSelect);
                mockButtons = 0x100;
                M360_MatchFrame();
            }
            assert(s_phase == kPhaseFight && s_fighterCount == 2);
            assert(s_stocksRemaining[0] == lives && s_stocksRemaining[1] == 1);
            assert(!s_matchOver && !s_draw && !s_paused && s_frame == 0);
            if (mode == kGameModeClassic) {
                const unsigned chosen = s_classicMatchups[round];
                assert(chosen < 38 && s_classicStKind == g_m360ClassicNormal[chosen].stkind);
                assert(s_stages[s_stageIndex].grkind == g_m360ClassicNormal[chosen].grkind);
                assert(s_kinds[s_selKind[1]].kind == g_m360ClassicNormal[chosen].fighterKind);
                assert(s_selKind[1] != s_selKind[0]);
                assert(hudTimeLimit == 300 && hudStockMatch);
                assert(M360_MatchBgmId() == 1000 + (int) s_classicStKind);
                if (round < 4)
                    for (unsigned previous = 0; previous < round; ++previous)
                        assert(g_m360ClassicNormal[s_classicMatchups[previous]].grkind != g_m360ClassicNormal[chosen].grkind);
            }
            if (round == 0 || round == 2) {
                fighters[0].x = 200;
                mockButtons = 0;
                M360_MatchFrame();
                assert(!s_matchOver && s_stocksRemaining[0] == --lives);
                fighters[0].x = 0;
                for (unsigned frame = 0; frame < kRespawnFrames; ++frame)
                    M360_MatchFrame();
            }
            fighters[1].x = 200;
            mockButtons = 0;
            M360_MatchFrame();
            assert(s_matchOver && !s_draw && s_winner == 0);
            mockButtons = 0x100;
            assert(M360_MatchFrame() == M360_MATCH_NEXT_ROUND);
            M360_MatchLeave();
            assert(!s_active && s_fighterCount == 0 && !s_fighters[0]);
            fighters[1].x = 0;
        }
        assert(lives == 1 && s_campaignStocks == 1);
        M360_MatchSetMode(mode, 0);
        assert(s_campaignStocks == 3);
    }
    puts("PASS: Classic entry, respawns, retained lives and fresh campaign reset");

    assert(M360_MatchCampaignRounds(kGameModeAdventure) == 21);
    Reset(2);
    M360_MatchSetMode(kGameModeAdventure, 0);
    M360_MatchEnter();
    mockButtons = 0x100;
    M360_MatchFrame();
    assert(s_phase == kPhaseFight && s_fighterCount == 1 && s_stageIndex == 17);
    assert(hudTimeLimit == 420 && M360_MatchIsTeams());
    assert(M360_MatchTeam(0) == 0 && M360_MatchTeam(3) == 1);
    routePoints = true; mockButtons = 0;
    M360_MatchFrame();
    assert(s_adventureCheckpoint == 1 && s_fighterCount == 4 && !s_matchOver);
    assert(s_stage.blastLeft == -100 && s_stage.blastRight == 100);
    assert(s_stage.blastBottom == -100 && s_stage.blastTop == 100 && s_stage.camY == 30);
    assert(s_stocksRemaining[1] == 4 && s_stocksRemaining[2] == 3 && s_stocksRemaining[3] == 3);
    for (unsigned wave = 0; wave < 4; ++wave) {
        for (unsigned i = 1; i < 4; ++i)
            if (s_stocksRemaining[i]) fighters[i].x = 200;
        M360_MatchFrame();
        if (wave < 3) {
            assert(s_adventureCheckpoint == 1 && !s_matchOver);
            for (unsigned frame = 0; frame < kRespawnFrames; ++frame) M360_MatchFrame();
            for (unsigned i = 1; i < 4; ++i)
                assert(!fighters[i].rebirths);
        }
    }
    assert(s_adventureCheckpoint == 2 && !s_matchOver);
    assert(gateHidden == 1);
    assert(s_stage.blastBottom == -100 && s_stage.blastTop == 100);
    assert(s_stage.camX == 0 && s_stage.camY == 0 && s_stage.camTop == 120);
    fighters[0].x = 80;
    s_frame = (CampaignSeconds() - 12u) * 60u - 1;
    M360_MatchFrame();
    assert(s_matchOver && s_winner == 0 && !s_draw);
    assert(s_adventureLuigi);
    routePoints = false;
    M360_MatchLeave();
    M360_MatchSetMode(kGameModeAdventure, 1); M360_MatchEnter();
    assert(s_kinds[s_selKind[1]].kind == M360_ADVENTURE_LUIGI_KIND);
    M360_MatchLeave();
    const unsigned waveRounds[] = {9, 13, 17};
    for (unsigned round = 0; round < 3; ++round) {
        Reset(4);
        M360_MatchSetMode(kGameModeAdventure, waveRounds[round]);
        M360_MatchEnter();
        mockButtons = 0x100; M360_MatchFrame(); mockButtons = 0;
        const unsigned lives = s_stocksRemaining[1];
        assert(lives > 1);
        fighters[1].x = 200; M360_MatchFrame();
        assert(s_stocksRemaining[1] == lives - 1 && !s_matchOver);
        for (unsigned frame = 0; frame < kRespawnFrames; ++frame) M360_MatchFrame();
        assert(!fighters[1].rebirths && !s_respawn[1]);
        assert(fighters[1].x == s_stage.spawnX[1] && fighters[1].y == s_stage.spawnY[1]);
        M360_MatchLeave();
    }
    puts("PASS: Kirby, Pokemon and wireframe wave replacements use arena spawns without player platforms");
    M360_MatchSetMode(kGameModeAdventure, 9);
    s_frame = 1800; assert(M360_MatchNextCampaignRound() == 10);
    s_frame = 1859; assert(M360_MatchNextCampaignRound() == 10);
    s_frame = 1860; assert(M360_MatchNextCampaignRound() == 11);
    s_matchOver = 1; s_campaignClearFrames = 1800; s_frame = 1920;
    assert(M360_MatchNextCampaignRound() == 10);
    s_matchOver = 0;
    M360_MatchSetMode(kGameModeAdventure, 8);
    assert(M360_MatchNextCampaignRound() == 9);
    puts("PASS: original Team Kirby time rule retains or skips Giant Kirby at whole-second boundary");
    M360_MatchSetMode(kGameModeAdventure, 19);
    s_cpuLevel = 3; s_frame = 300;
    assert(M360_MatchNextCampaignRound() == 21);
    s_cpuLevel = 5; s_adventureElapsedFrames = 64499;
    assert(M360_MatchNextCampaignRound() == 20);
    s_adventureElapsedFrames = 64500;
    assert(M360_MatchNextCampaignRound() == 21);
    s_adventureElapsedFrames = 64499;
    s_matchOver = 1; s_campaignClearFrames = 300; s_frame = 450;
    assert(M360_MatchNextCampaignRound() == 20);
    s_active = 1;
    M360_MatchLeave();
    assert(s_adventureElapsedFrames == 64799);
    M360_MatchLeave();
    assert(s_adventureElapsedFrames == 64799);
    s_active = 1; s_campaignClearFrames = ~0u;
    M360_MatchLeave();
    assert(s_adventureElapsedFrames == 64800);
    assert(M360_MatchNextCampaignRound() == 21);
    s_adventureElapsedFrames = 64799;
    s_campaignRetryPending = 1;
    M360_MatchSetMode(kGameModeAdventure, 0);
    assert(s_adventureElapsedFrames == 64799);
    s_campaignRetryPending = 0;
    M360_MatchSetMode(kGameModeAdventure, 0);
    assert(s_adventureElapsedFrames == 0);
    puts("PASS: Giga eligibility uses original difficulty, strict eighteen-minute limit and captured time");
    Reset(2);
    M360_MatchSetMode(kGameModeAdventure, 7);
    M360_MatchEnter();
    mockButtons = 0x100; M360_MatchFrame(); mockButtons = 0;
    assert(s_fighterCount == 1 && s_stageIndex == 18 && hudTimeLimit == 40);
    assert(CampaignSeconds() == 40 && !s_matchOver);
    mockFloorLine = 10; M360_MatchFrame(); assert(!s_matchOver);
    mockFloorLine = 44; M360_MatchFrame(); assert(s_matchOver && s_winner == 0);
    mockFloorLine = -1; M360_MatchLeave();
    assert(!CollisionGroupHasFloor(1, -1) && !CollisionGroupHasFloor(2, 44));
    Reset(2);
    M360_MatchSetMode(kGameModeAdventure, 7);
    M360_MatchEnter();
    mockButtons = 0x100; M360_MatchFrame(); mockButtons = 0;
    s_frame = 2399; M360_MatchFrame();
    assert(s_matchOver && s_campaignTimedOut && s_campaignStocks == 2);
    mockButtons = 0x100;
    assert(M360_MatchFrame() == M360_MATCH_RESTART && s_campaignRound == 7);
    M360_MatchLeave();
    puts("PASS: Brinstar escape waits for top-platform contact and uses the original forty-second timer");
    Reset(2);
    M360_MatchSetMode(kGameModeAdventure, 4); M360_MatchEnter();
    mockButtons = 0;
    assert(s_fighterCount == 1 && s_stageIndex == 19 && hudTimeLimit == 420);
    s_routeBlast[0] = s_routeBlast[2] = -2000;
    s_routeBlast[1] = s_routeBlast[3] = 2000;
    s_stage.lineCount = 132;
    const unsigned mazeWalls[] = { 51, 79, 101, 102, 115, 116, 131 };
    for (unsigned i = 0; i < 7; ++i) s_stage.lines[mazeWalls[i]].kind = 1;
    MazeRoomBounds(-1);
    for (unsigned i = 0; i < 7; ++i) assert(s_stage.lines[mazeWalls[i]].kind == 0);
    fighters[0].x = 35; M360_MatchFrame(); assert(s_mazeRoom == -1);
    fighters[0].x = 34; M360_MatchFrame(); assert(s_mazeRoom == 0);
    fighters[0].x = 0; M360_MatchFrame();
    assert(s_mazeRoom == 0 && s_fighterCount == 2 && s_stocksRemaining[1] == 1);
    for (unsigned i = 0; i < 6; ++i) assert(mockSymbols[i].flags & JOBJ_HIDDEN);
    for (unsigned group = 0; group < 14; ++group)
        assert((s_stage.lines[mockGroups[group].floor_start].kind != 0) == (group == 8));
    fighters[1].x = 200; M360_MatchFrame();
    assert(s_mazeVisited == 1 && s_mazeRoom == -1 && !s_matchOver);
    assert(mockSymbols[0].flags & JOBJ_HIDDEN);
    for (unsigned i = 1; i < 6; ++i) assert(!(mockSymbols[i].flags & JOBJ_HIDDEN));
    for (unsigned group = 0; group < 14; ++group)
        assert(s_stage.lines[mockGroups[group].floor_start].kind ==
               (group > 8 || mockGroups[group].floor_start == 51 ? 0 : mockLines[mockGroups[group].floor_start].hi_flags));
    fighters[0].x = 0; M360_MatchFrame(); assert(s_mazeRoom == -1);
    fighters[0].x = 300; M360_MatchFrame();
    assert(s_mazeRoom == 1 && s_stocksRemaining[1] == 1 && !fighters[1].rebirths);
    assert(fighters[1].x == 300 && fighters[1].y == 10);
    fighters[1].x = 600; M360_MatchFrame();
    assert(s_mazeVisited == 3 && s_mazeRoom == -1 && !s_matchOver);
    for (unsigned group = 8; group < 14; ++group)
        assert((s_stage.lines[mockGroups[group].floor_start].kind != 0) == (group == 9));
    float mazeX, mazeY;
    assert(M360_MatchMazePoint(5, &mazeX, &mazeY) && mazeX == 1500 && mazeY == 0);
    assert(!M360_MatchMazePoint(6, &mazeX, &mazeY));
    assert(M360_MatchMazeVisited() == 3);
    fighters[0].x = 1500; M360_MatchFrame();
    assert(s_mazeFinishFrames == 1 && !s_matchOver);
    for (unsigned frame = 0; frame < 59; ++frame) M360_MatchFrame();
    assert(s_matchOver && s_winner == 0 && !s_draw);
    mockButtons = 0x100; assert(M360_MatchFrame() == M360_MATCH_NEXT_ROUND);
    M360_MatchLeave();
    s_routeBlast[0] = s_routeBlast[2] = -100;
    s_routeBlast[1] = s_routeBlast[3] = 100;
    puts("PASS: maze starts solo, confines Link rooms, restores traversal, reuses enemies and clears at the Triforce");

    Reset(2); racePoints = true;
    M360_MatchSetMode(kGameModeAdventure, 14); M360_MatchEnter();
    assert(s_fighterCount == 1 && s_stageIndex == 20 && hudTimeLimit == 240);
    mockButtons = 0;
    fighters[0].x = 10; M360_MatchFrame(); assert(s_adventureCheckpoint == 0 && !s_matchOver);
    fighters[0].x = 11; M360_MatchFrame();
    assert(s_adventureCheckpoint == 1 && s_stage.rebirthX[0] == 10 && s_stage.rebirthY[0] == 20);
    fighters[0].x = 31; M360_MatchFrame(); assert(s_adventureCheckpoint == 2);
    fighters[0].x = 51; M360_MatchFrame();
    assert(s_adventureCheckpoint == 3 && s_stage.rebirthX[0] == 50 && s_stage.rebirthY[0] == 60);
    fighters[0].x = 65; M360_MatchFrame(); assert(!s_matchOver);
    fighters[0].x = 66; M360_MatchFrame(); assert(s_matchOver && s_winner == 0);
    mockButtons = 0x100; assert(M360_MatchFrame() == M360_MATCH_NEXT_ROUND);
    M360_MatchLeave(); racePoints = false;
    puts("PASS: experimental race starts solo, uses the original timer, advances authored checkpoints and finishes within original half extents");

    const unsigned courseRounds[] = { 0, 4, 7, 14 };
    for (unsigned i = 0; i < 4; ++i) {
        Reset(2); M360_MatchSetMode(kGameModeAdventure, courseRounds[i]); M360_MatchEnter();
        if (!courseRounds[i]) { mockButtons = 0x100; M360_MatchFrame(); M360_MatchFrame(); }
        mockButtons = 0; s_stocksRemaining[0] = s_campaignStocks = 1;
        fighters[0].x = 200; M360_MatchFrame();
        assert(s_matchOver && s_winner == 1 && !s_draw && s_campaignStocks == 0);
        mockButtons = 0x100; assert(M360_MatchFrame() == M360_MATCH_RESTART);
        assert(s_campaignStocks == 3 && s_campaignRetryPending);
        M360_MatchLeave();
    }
    puts("PASS: final-life course falls lose the attempt, then Continue restores lives without declaring a draw");

    Reset(3);
    M360_MatchSetMode(kGameModeAdventure, 2);
    M360_MatchEnter();
    assert(s_fighterCount == 3 && hudTimeLimit == 240);
    assert(encounterScale[1] == 0.5f && encounterScale[2] == 0.5f);
    fighters[1].x = 200;
    M360_MatchFrame();
    assert(!s_matchOver && !s_stocksRemaining[1] && s_stocksRemaining[2]);
    fighters[2].x = 200;
    M360_MatchFrame();
    assert(s_matchOver && s_winner == 0);
    puts("PASS: Adventure traversal/checkpoint/goal, enemy teams, original stage and time limit");

    Reset(2);
    M360_MatchSetMode(kGameModeClassic, 0);
    M360_MatchEnter();
    mockButtons = 0x100;
    M360_MatchFrame();
    const unsigned firstEncounter = s_classicMatchups[0];
    for (unsigned lives = 2; ; --lives) {
        s_frame = 300 * 60 - 1;
        mockButtons = 0;
        M360_MatchFrame();
        assert(s_matchOver && s_campaignTimedOut && !s_draw && s_winner == 1);
        assert(s_campaignStocks == lives && s_stocksRemaining[0] == lives);
        mockButtons = 0x100;
        if (!lives) {
            assert(M360_MatchFrame() == M360_MATCH_RESTART && s_campaignRetryPending);
            assert(s_campaignStocks == 3);
            M360_MatchLeave();
            M360_MatchSetMode(kGameModeClassic, 0);
            M360_MatchEnter();
            assert(s_classicMatchups[0] == firstEncounter && s_stocksRemaining[0] == 3);
            M360_MatchLeave();
            break;
        }
        assert(M360_MatchFrame() == M360_MATCH_RESTART && s_campaignRetryPending);
        M360_MatchLeave();
        M360_MatchSetMode(kGameModeClassic, 0);
        assert(s_campaignStocks == lives); // Retry round 0 must not refill lives.
        M360_MatchEnter();
        assert(s_phase == kPhaseFight && !s_campaignTimedOut && s_frame == 0);
        assert(s_classicMatchups[0] == firstEncounter && s_stocksRemaining[0] == lives);
    }
    M360_MatchSetMode(kGameModeClassic, 0);
    assert(s_campaignStocks == 3 && s_classicMatchups[0] == ~0u);
    s_classicStKind = 999;
    assert(!MatchStageParam() && M360_MatchBgmId() == -1);
    M360_MatchSetMode(2, 0);
    assert(M360_MatchBgmId() == 22); // VS uses the base row again.
    puts("PASS: original Classic encounter pairs/variant BGM, stock timer, timeout life loss and same-encounter retry");

    Reset(2);
    s_timeOption = 1;
    s_stocksRemaining[0] = 0; // Timed play has unlimited lives.
    fighters[0].x = 200;
    M360_MatchFrame();
    assert(!s_matchOver && s_score[0] == -1 && s_respawn[0] == kRespawnFrames);
    puts("PASS: campaign lives, eliminated slots, simultaneous falls, results and timed respawn");
}
