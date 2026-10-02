#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/xdk/match_xdk.h"

enum { kMaxFighters = 4, kStartingStocks = 3, kCampaignRounds = 5,
       kGameModeClassic = 3, kGameModeAdventure = 4,
       kPhaseSelect = 0, kPhaseFight = 1, kRespawnFrames = 60,
       kMaxCostumes = 6, kKindCount = 3, kStageCount = 9, kTimeOptionCount = 2,
       HSD_GOBJ_CLASS_STAGE = 1 };
struct CostumeDesc { const char* costumeDat[6]; const char* costumeJoint[6]; };
const CostumeDesc s_kinds[] = {
    { { "a", "b", "c", "d", "e", NULL }, { "a", "b", "c", "d", "e", NULL } },
    { { "a", "b", "c", "d", NULL, NULL }, { "a", "b", "c", "d", NULL, NULL } },
    { { "a", "b", "c", "d", "e", "f" }, { "a", "b", "c", "d", "e", "f" } }
};
struct HSD_GObj {};
struct Fighter { float x, y; unsigned reads, deaths, rebirths; } fighters[4];
unsigned s_gameMode, s_campaignRound, s_campaignStocks, s_stocks;
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
unsigned s_modelCount, s_collBindCount, s_timeOption, g_m360Crumb;
const unsigned kTimeOptions[] = { 0, 1 };
void* s_models[8];
void* s_lobj;
M360MatchAutoConfig s_auto;
M360MatchStage s_stage;
unsigned mockButtons, stockNotices[4], gameEnds;
unsigned M360_MatchPadTriggered() { return mockButtons; }
unsigned M360_MatchPadHeld() { return mockButtons; }
unsigned M360_MatchPadTriggeredPort(unsigned port) { return port == 0 ? mockButtons : 0; }
float M360_MatchPadStickXPort(unsigned) { return mockStickX; }
float M360_MatchPadSubStickYPort(unsigned) { return mockSubY; }
int M360_MatchControllerConnected(unsigned) { return 0; }
void M360_MatchTrace(const char*, unsigned) {}
void M360_HudFrame(unsigned) {}
void M360_HudPause(int, int) {}
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
void M360_AudioStopAllSfx() {}
void M360_HsdRenderClearTextures() {}
void FreeAllGObjs() { ++cleanupCalls; }
int BuildStage(unsigned stage) { s_stageIndex = stage; return 1; }
void* M360_FighterSpawn(int slot, float, float, float, int) {
    return slot == failSlot ? NULL : &fighters[slot];
}
void it_8026D018() {}
HSD_GObj* GObj_Create(int, int, int) { return NULL; }
void DebugRender(HSD_GObj*, int) {}
void GObj_SetupGXLink(HSD_GObj*, void (*)(HSD_GObj*, int), int, int) {}
void M360_HudStart(unsigned, unsigned, unsigned) { ++hudStarts; }
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
    s_matchOver = s_draw = s_paused = s_suddenDeath = 0;
    s_timeOption = 0; s_stocks = 4;
    M360_MatchSetMode(2, 0);
}

int main() {
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
    assert(M360_MatchFrame() == M360_MATCH_CONTINUE); // Draw cannot advance campaign.

    Reset(2);
    M360_MatchSetMode(kGameModeClassic, 0);
    s_stocksRemaining[0] = 2;
    fighters[1].x = 200;
    M360_MatchFrame();
    assert(s_matchOver && !s_draw && s_winner == 0 && gameEnds == 1);
    mockButtons = 0x100;
    assert(M360_MatchFrame() == M360_MATCH_NEXT_ROUND);

    Reset(2);
    s_timeOption = 1;
    s_stocksRemaining[0] = 0; // Timed play has unlimited lives.
    fighters[0].x = 200;
    M360_MatchFrame();
    assert(!s_matchOver && s_score[0] == -1 && s_respawn[0] == kRespawnFrames);
    puts("PASS: campaign lives, eliminated slots, simultaneous falls, results and timed respawn");
}
