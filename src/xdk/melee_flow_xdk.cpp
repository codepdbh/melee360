#include "melee_flow_xdk.h"
#include "melee_archive_xdk.h"
#include "melee_movie_xdk.h"
#include "melee_title_scene_xdk.h"
#include "menu_scene_xdk.h"
#include "match_xdk.h"

extern "C" int HSD_Randi(int max_val);

namespace {

const unsigned kOpeningRateTable[] = { 1250, 2, 394, 1, 65536, 2 };
const unsigned kBgmMenu01 = 0x34;
const unsigned kBgmMenu3 = 0x36;
const unsigned kBgmOpening = 0x3E;
const unsigned kGameModeVs = 2;
const unsigned kGameModeClassic = 3;
const unsigned kGameModeAdventure = 4;
const unsigned kCampaignRounds = 5;
const unsigned kTitleLogoTick = 0x140A;
const unsigned kOpeningMenuTick = 0x157C;
const unsigned kTitleTimeout = 600;
const unsigned kTitleCountdown = 20;
const unsigned __int64 kPadConfirm = 1ull << 32;
const unsigned __int64 kPadCancel = 1ull << 33;
bool s_allStagesUnlocked = false;
bool s_allCharactersUnlocked = false;
MeleeFlow* s_flow;
MeleeAudioStatus* s_audio;
int s_playingBgm = -1;
bool s_menuAvailable;
bool s_menuActive;

const char* BgmFile(unsigned bgm)
{
    switch (bgm) {
    case kBgmMenu3: return "audio/menu3.hps";
    case kBgmOpening: return "audio/opening.hps";
    default: return "audio/menu01.hps";
    }
}

void PlayBgm(unsigned bgm, MeleeAudioStatus* audio)
{
    s_playingBgm = static_cast<int>(bgm);
    const bool playing = M360_AudioPlay(BgmFile(bgm), audio);
    M360_Trace("audio.track.bgm", bgm);
    M360_Trace("audio.track.playing", playing);
    M360_Trace("audio.track.sample_rate", audio->sampleRate);
}

void ChooseMenuBgm(MeleeFlow* flow)
{
    if (s_allStagesUnlocked && s_allCharactersUnlocked)
        flow->rulesBgm = HSD_Randi(4) != 0 ? kBgmMenu01 : kBgmMenu3;
    else
        flow->rulesBgm = kBgmMenu01;
    M360_Trace("menu.bgm.choice", flow->rulesBgm);
}

void EnterState(MeleeFlow* flow, MeleeFlowState state, MeleeAudioStatus* audio)
{
    if (flow->state == kFlowOpening && state != kFlowOpening) {
        M360_MovieClose();
        MeleeMovieStatus movie;
        M360_MovieGetStatus(&movie);
        M360_Trace("movie.close.frames_decoded", movie.framesDecoded);
        M360_Trace("movie.close.frames_presented", movie.framesPresented);
        M360_Trace("movie.close.decode_us_avg", movie.decodeUsAverage);
        M360_Trace("movie.close.decode_us_max", movie.decodeUsMax);
    }
    if (flow->state == kFlowMainMenu && state != kFlowMainMenu && s_menuActive) {
        M360_MenuSceneLeave();
        s_menuActive = false;
    }
    if (flow->state == kFlowMatch && state != kFlowMatch)
        M360_MatchLeave();
    flow->state = state;
    flow->sceneTick = 0;
    ++flow->transitions;
    QueryPerformanceCounter(&flow->sceneStart);
    M360_Trace("flow.state", static_cast<unsigned>(state));
    if (state == kFlowOpening) {
        ++flow->openingPlays;
        M360_TitleEnter(true);
        flow->movieReady = M360_MovieOpen("MvOpen.mth", kOpeningRateTable);
        MeleeMovieStatus movie;
        M360_MovieGetStatus(&movie);
        M360_Trace("movie.open", flow->movieReady);
        M360_Trace("movie.frames", movie.frameCount);
        M360_Trace("movie.width", movie.width);
        M360_Trace("movie.height", movie.height);
        M360_Trace("movie.fps", movie.frameRate);
        M360_Trace("movie.index_ms", movie.indexMs);
        PlayBgm(kBgmOpening, audio);
        QueryPerformanceCounter(&flow->sceneStart);
        flow->movieVisible = flow->movieReady;
        flow->titleVisible = true;
    } else if (state == kFlowTitle) {
        M360_AudioStop(audio);
        s_playingBgm = -1;
        M360_Trace("audio.stopped", 1);
        M360_TitleEnter(false);
        flow->titleCountdown = kTitleCountdown;
        flow->titleFrames = 0;
        flow->movieVisible = false;
        flow->titleVisible = true;
    } else if (state == kFlowMatch) {
        M360_AudioStop(audio);
        s_playingBgm = -1;
        flow->movieVisible = false;
        flow->titleVisible = false;
        M360_MatchSetMode(flow->gameMode, flow->campaignRound);
        M360_MatchEnter();
        const bool playing = M360_AudioPlay("audio/vl_battle.hps", audio);
        M360_Trace("audio.match.background", playing);
        M360_Trace("audio.match.file_found", audio->fileFound);
    } else {
        flow->menuSelection = 0;
        flow->menuKind = 0;
        flow->movieVisible = false;
        flow->titleVisible = false;
        s_menuActive = s_menuAvailable;
        if (s_menuActive)
            M360_MenuSceneEnter(0, 0);
        else
            PlayBgm(flow->rulesBgm, audio);
    }
}

void UpdateOpening(MeleeFlow* flow, unsigned __int64 buttons,
                   MeleeAudioStatus* audio)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    flow->sceneTick = static_cast<unsigned>(
        (now.QuadPart - flow->sceneStart.QuadPart) * 60 / flow->frequency.QuadPart);
    const bool backgroundStarted = M360_TitleUpdate(flow->sceneTick);
    if (flow->movieReady && !backgroundStarted)
        M360_MovieSetTick(flow->sceneTick);
    flow->movieVisible = flow->movieReady && !backgroundStarted;
    if (flow->sceneTick > kOpeningMenuTick) {
        if (buttons & kPadConfirm) {
            M360_AudioStop(audio);
            s_playingBgm = -1;
            M360_Trace("input.opening.start_to_menu", flow->sceneTick);
            EnterState(flow, kFlowMainMenu, audio);
            return;
        }
    } else if (buttons & kPadConfirm) {
        M360_Trace("input.opening.skip_to_title", flow->sceneTick);
        EnterState(flow, kFlowTitle, audio);
        return;
    }
    if (flow->sceneTick >= kTitleLogoTick + 400 + 600) {
        M360_Trace("opening.end_tick", flow->sceneTick);
        EnterState(flow, kFlowTitle, audio);
    }
}

void UpdateTitle(MeleeFlow* flow, unsigned __int64 buttons,
                 MeleeAudioStatus* audio)
{
    M360_TitleUpdate(0);
    ++flow->sceneTick;
    if (flow->titleCountdown) {
        --flow->titleCountdown;
        return;
    }
    ++flow->titleFrames;
    if (flow->titleFrames > kTitleTimeout) {
        M360_Trace("title.idle_timeout", flow->titleFrames);
        EnterState(flow, kFlowOpening, audio);
    } else if (buttons & kPadConfirm) {
        M360_Trace("input.title.confirm", flow->titleFrames);
        ChooseMenuBgm(flow);
        EnterState(flow, kFlowMainMenu, audio);
    }
}

void UpdateMenu(MeleeFlow* flow, unsigned __int64 buttons, MeleeAudioStatus* audio)
{
    ++flow->sceneTick;
    if (!s_menuActive) {
        if (buttons & kPadCancel)
            EnterState(flow, kFlowTitle, audio);
        return;
    }
    const unsigned previousKind = flow->menuKind;
    const unsigned previousSelection = flow->menuSelection;
    const int result = M360_MenuSceneFrame();
    M360_MenuSceneState(&flow->menuKind, &flow->menuSelection);
    if (flow->menuKind != previousKind)
        M360_Trace("menu.kind", flow->menuKind);
    if (flow->menuSelection != previousSelection)
        M360_Trace("menu.selection", flow->menuSelection);
    if (result == M360_MENU_TO_TITLE) {
        M360_Trace("menu.exit.title", flow->sceneTick);
        EnterState(flow, kFlowTitle, audio);
    } else if (result == M360_MENU_TO_MATCH ||
               result == M360_MENU_TO_CLASSIC ||
               result == M360_MENU_TO_ADVENTURE) {
        flow->gameMode = result == M360_MENU_TO_CLASSIC ? kGameModeClassic :
                         result == M360_MENU_TO_ADVENTURE ? kGameModeAdventure :
                         kGameModeVs;
        flow->campaignRound = 0;
        M360_Trace("mode.selected", flow->gameMode);
        EnterState(flow, kFlowMatch, audio);
    } else if (result == M360_MENU_RESTART) {
        M360_Trace("menu.exit.restart", flow->sceneTick);
        M360_MenuSceneEnter(0, 0);
    }
}

void UpdateMatch(MeleeFlow* flow, MeleeAudioStatus* audio)
{
    ++flow->sceneTick;
    const int result = M360_MatchFrame();
    if (result == M360_MATCH_TO_MENU) {
        flow->gameMode = kGameModeVs;
        flow->campaignRound = 0;
        EnterState(flow, kFlowMainMenu, audio);
    } else if (result == M360_MATCH_NEXT_ROUND) {
        ++flow->campaignRound;
        if (flow->campaignRound >= kCampaignRounds) {
            M360_MatchTrace("mode.complete", flow->gameMode);
            flow->gameMode = kGameModeVs;
            flow->campaignRound = 0;
            EnterState(flow, kFlowMainMenu, audio);
        } else {
            M360_MatchLeave();
            EnterState(flow, kFlowMatch, audio);
        }
    }
}

} // namespace

void M360_FlowStart(MeleeFlow* flow, MeleeAudioStatus* audio)
{
    ZeroMemory(flow, sizeof(*flow));
    s_flow = flow;
    s_audio = audio;
    QueryPerformanceFrequency(&flow->frequency);
    flow->rulesBgm = kBgmMenu01;
    flow->gameMode = kGameModeVs;
    flow->campaignRound = 0;
    flow->state = kFlowMainMenu;
#ifdef M360_BOOT_TO_MATCH
    EnterState(flow, kFlowMatch, audio);
#else
    EnterState(flow, kFlowOpening, audio);
#endif
}

void M360_FlowUpdate(MeleeFlow* flow, unsigned __int64 buttonsTriggered,
                     MeleeAudioStatus* audio)
{
    switch (flow->state) {
    case kFlowOpening: UpdateOpening(flow, buttonsTriggered, audio); break;
    case kFlowTitle: UpdateTitle(flow, buttonsTriggered, audio); break;
    case kFlowMatch: UpdateMatch(flow, audio); break;
    default: UpdateMenu(flow, buttonsTriggered, audio); break;
    }
}

const char* M360_FlowStateName(MeleeFlowState state)
{
    switch (state) {
    case kFlowOpening: return "OPENING MOVIE";
    case kFlowTitle: return "TITLE";
    case kFlowMatch: return "MATCH";
    default: return "MAIN MENU";
    }
}

extern "C" void M360_MatchTrace(const char* stage, unsigned value)
{
    M360_Trace(stage, value);
}

extern "C" void M360_MenuTrace(const char* stage, unsigned value)
{
    M360_Trace(stage, value);
}

extern "C" void* M360_MenuSymbol(const char* name)
{
    return M360_GetMenuHsdPublic(name);
}

extern "C" int M360_MenuBgmChoice(void)
{
    return s_flow ? static_cast<int>(s_flow->rulesBgm) : static_cast<int>(kBgmMenu01);
}

extern "C" void M360_MenuPlayBgm(int bgm)
{
    if (s_audio && bgm != s_playingBgm)
        PlayBgm(static_cast<unsigned>(bgm), s_audio);
}

void M360_FlowSetMenuAvailable(bool available)
{
    s_menuAvailable = available;
}

bool M360_FlowMenuActive(void)
{
    return s_menuActive;
}
