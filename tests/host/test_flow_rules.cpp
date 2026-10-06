#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/xdk/melee_flow_xdk.h"
#include "../../src/xdk/melee_movie_xdk.h"
#include "../../src/xdk/menu_scene_xdk.h"
#include "../../src/xdk/match_xdk.h"

const unsigned kGameModeVs = 2, kGameModeClassic = 3, kGameModeAdventure = 4;
const unsigned kCampaignRounds = 5, kBgmOpening = 0x3E, kTitleCountdown = 20;
unsigned M360_MatchCampaignRounds(unsigned mode) { return mode == kGameModeAdventure ? 21 : kCampaignRounds; }
const unsigned kOpeningRateTable[] = { 1250, 2, 394, 1, 65536, 2 };
const unsigned __int64 kPadCancel = 1ull << 33;
bool s_menuActive, s_menuAvailable = true;
int s_playingBgm;
const char* s_matchMusic;
unsigned leaves, enters, modeRound, modeKind, menuLeaves, menuEnters;
unsigned M360_MatchNextCampaignRound() { return modeRound + 1; }
unsigned liveScenes, cacheClears, musicUpdates, stopSfxCalls;
int matchResult, menuResult;
typedef unsigned DWORD;
typedef int HANDLE;
const int GENERIC_READ = 1, FILE_SHARE_READ = 1, OPEN_EXISTING = 1, FILE_ATTRIBUTE_NORMAL = 1;
const HANDLE INVALID_HANDLE_VALUE = -1;
const char* configFixture;
MeleeFlow* s_flow;
unsigned randomSeed;
unsigned* HSD_RandSeedPtr = &randomSeed;
M360MatchAutoConfig autoConfig;
HANDLE CreateFileA(const char*, int, int, int, int, int, int) { return 1; }
bool ReadFile(HANDLE, char* text, unsigned length, DWORD* count, int) {
    *count = static_cast<DWORD>(strlen(configFixture));
    assert(*count < length);
    memcpy(text, configFixture, *count);
    return true;
}
void CloseHandle(HANDLE) {}
#define ZeroMemory(ptr, size) memset(ptr, 0, size)
void M360_MatchSetAutoConfig(const M360MatchAutoConfig* config) { autoConfig = *config; }

void M360_Trace(const char*, unsigned) {}
void M360_MatchTrace(const char*, unsigned) {}
void M360_AudioStopAllSfx() { ++stopSfxCalls; }
void M360_AudioStop(MeleeAudioStatus*) {}
void M360_MovieClose() {}
void M360_MovieGetStatus(MeleeMovieStatus* status) { memset(status, 0, sizeof(*status)); }
bool M360_MovieOpen(const char*, const unsigned*) { return true; }
void M360_TitleEnter(bool) {}
void PlayBgm(unsigned, MeleeAudioStatus*) {}
void UpdateMatchMusic(MeleeAudioStatus*) { ++musicUpdates; }
void M360_MenuSceneLeave() { ++menuLeaves; }
void M360_MenuSceneEnter(unsigned, unsigned) { ++menuEnters; }
int M360_MenuSceneFrame() { return menuResult; }
void M360_MenuSceneState(unsigned* kind, unsigned* selection) { *kind = *selection = 0; }
void M360_HsdRenderClearTextures() {
    // Match objects must release references before clearing cached textures.
    assert(liveScenes == 0);
    ++cacheClears;
}
void M360_MatchLeave() {
    assert(liveScenes == 1);
    --liveScenes;
    ++leaves;
}
void M360_MatchSetMode(unsigned kind, unsigned round) { modeKind = kind; modeRound = round; }
void M360_MatchEnter() {
    assert(liveScenes == 0);
    ++liveScenes;
    ++enters;
}
int M360_MatchFrame() { return matchResult; }

#include "flow_rules_original.h"

static void Campaign(unsigned kind) {
    MeleeFlow flow = {};
    MeleeAudioStatus audio = {};
    flow.state = kFlowMainMenu;
    s_menuActive = true;
    const unsigned oldLeaves = leaves, oldEnters = enters;
    menuResult = kind == kGameModeClassic ? M360_MENU_TO_CLASSIC : M360_MENU_TO_ADVENTURE;
    UpdateMenu(&flow, 0, &audio);
    assert(flow.state == kFlowMatch && flow.campaignRound == 0);
    assert(modeKind == kind && modeRound == 0 && liveScenes == 1);
    matchResult = M360_MATCH_CONTINUE;
    UpdateMatch(&flow, &audio);
    assert(leaves == oldLeaves && enters == oldEnters + 1);
    matchResult = M360_MATCH_NEXT_ROUND;
    for (unsigned round = 1; round < M360_MatchCampaignRounds(kind); ++round) {
        UpdateMatch(&flow, &audio);
        assert(flow.state == kFlowMatch && flow.campaignRound == round);
        assert(modeRound == round && modeKind == kind && liveScenes == 1);
        assert(leaves == oldLeaves + round && enters == oldEnters + round + 1);
    }
    UpdateMatch(&flow, &audio);
    assert(flow.state == kFlowMainMenu && flow.gameMode == kGameModeVs);
    assert(flow.campaignRound == 0 && liveScenes == 0);
    assert(leaves == oldLeaves + M360_MatchCampaignRounds(kind) && enters == oldEnters + M360_MatchCampaignRounds(kind));
}

int main() {
    Campaign(kGameModeClassic);
    Campaign(kGameModeAdventure);
    MeleeFlow flow = {};
    MeleeAudioStatus audio = {};
    flow.state = kFlowMainMenu;
    menuResult = M360_MENU_TO_MATCH;
    UpdateMenu(&flow, 0, &audio);
    assert(flow.gameMode == kGameModeVs && liveScenes == 1);
    const unsigned oldLeaves = leaves, oldEnters = enters;
    matchResult = M360_MATCH_RESTART;
    for (unsigned restart = 1; restart <= 3; ++restart) {
        UpdateMatch(&flow, &audio);
        assert(flow.state == kFlowMatch && liveScenes == 1);
        assert(leaves == oldLeaves + restart && enters == oldEnters + restart);
    }
    matchResult = M360_MATCH_TO_MENU;
    UpdateMatch(&flow, &audio);
    assert(flow.state == kFlowMainMenu && liveScenes == 0);
    assert(leaves == oldLeaves + 4);
    assert(cacheClears == enters + 3); // Two completions and one exit.
    puts("PASS: five Classic rounds, twenty-one Adventure phases, teardown before cache clearing and VS restarts");
    MeleeFlow configFlow = {};
    s_flow = &configFlow;
    configFixture = "mode 4\nround 4\np1 18\nseed 12000\n";
    randomSeed = 777;
    LoadAutoConfig();
    assert(randomSeed == 12000 && configFlow.gameMode == 4 && configFlow.campaignRound == 4);
    assert(autoConfig.enabled && autoConfig.kinds[0] == 18);
    configFixture = "mode 4\nseed -1\n";
    randomSeed = 777; LoadAutoConfig(); assert(randomSeed == 777);
    configFixture = "mode 4\n";
    LoadAutoConfig(); assert(randomSeed == 777);
    puts("PASS: diagnostic seed is explicit, rejects negative values and leaves the RNG unchanged when absent");
}
