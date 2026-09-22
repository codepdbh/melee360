#include "melee_flow_xdk.h"
#include "melee_movie_xdk.h"
#include "melee_title_scene_xdk.h"

extern "C" int HSD_Randi(int max_val);

namespace {

const unsigned kOpeningRateTable[] = { 1250, 2, 394, 1, 65536, 2 };
const unsigned kBgmMenu01 = 0x34;
const unsigned kBgmMenu3 = 0x36;
const unsigned kBgmOpening = 0x3E;
const unsigned kTitleLogoTick = 0x140A;
const unsigned kOpeningMenuTick = 0x157C;
const unsigned kTitleTimeout = 600;
const unsigned kTitleCountdown = 20;
const unsigned __int64 kPadConfirm = 1ull << 32;
const unsigned __int64 kPadCancel = 1ull << 33;
const unsigned __int64 kPadUp = 1ull << 36;
const unsigned __int64 kPadDown = 1ull << 37;
const unsigned kMenuSelectionCounts[29] = {
    5, 5, 5, 4, 6, 5, 3, 0, 0, 3, 0, 0, 10,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3
};

bool MenuOptionAvailable(unsigned kind, unsigned selection)
{
    // mn_80229938: these slots are hidden in the original scene. Sound Test
    // additionally depends on progress; no save-game unlock bridge exists yet.
    if ((kind == 1 || kind == 3 || kind == 5) && selection == 2)
        return false;
    if (kind == 4 && selection == 3)
        return false;
    if (kind == 6 && selection == 2)
        return false; // All-Star unlock state has not been imported yet.
    return true;
}

bool s_allStagesUnlocked = false;
bool s_allCharactersUnlocked = false;

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
        M360_Trace("audio.stopped", 1);
        M360_TitleEnter(false);
        flow->titleCountdown = kTitleCountdown;
        flow->titleFrames = 0;
        flow->movieVisible = false;
        flow->titleVisible = true;
    } else {
        flow->menuSelection = 0;
        flow->menuKind = 0;
        PlayBgm(flow->rulesBgm, audio);
        flow->movieVisible = false;
        flow->titleVisible = false;
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

void UpdateMenu(MeleeFlow* flow, unsigned __int64 buttons,
                MeleeAudioStatus* audio)
{
    ++flow->sceneTick;
    if (buttons & kPadUp) {
        const unsigned count = kMenuSelectionCounts[flow->menuKind];
        do {
            flow->menuSelection = (flow->menuSelection + count - 1) % count;
        } while (!MenuOptionAvailable(flow->menuKind, flow->menuSelection));
        M360_Trace("input.menu.selection", flow->menuSelection);
    } else if (buttons & kPadDown) {
        do {
            flow->menuSelection = (flow->menuSelection + 1) %
                                  kMenuSelectionCounts[flow->menuKind];
        } while (!MenuOptionAvailable(flow->menuKind, flow->menuSelection));
        M360_Trace("input.menu.selection", flow->menuSelection);
    }
    if (buttons & kPadConfirm) {
        M360_Trace("input.menu.confirm.selection", flow->menuSelection);
        unsigned nextKind = flow->menuKind;
        if (flow->menuKind == 0)
            nextKind = flow->menuSelection + 1;
        else if (flow->menuKind == 1 && flow->menuSelection == 0)
            nextKind = 6; // Regular Match
        else if (flow->menuKind == 1 && flow->menuSelection == 3)
            nextKind = 9; // Stadium
        else if (flow->menuKind == 2 && flow->menuSelection == 2)
            nextKind = 12; // Special Melee
        else if (flow->menuKind == 5 && flow->menuSelection == 3)
            nextKind = 28; // Records
        if (nextKind != flow->menuKind) {
            flow->menuKind = nextKind;
            flow->menuSelection = 0;
            M360_Trace("input.menu.kind", flow->menuKind);
        }
    }
    if (buttons & kPadCancel) {
        M360_Trace("input.menu.back", flow->sceneTick);
        if (flow->menuKind) {
            const unsigned previousKind = flow->menuKind;
            flow->menuKind = previousKind == 6 || previousKind == 9 ? 1 :
                             previousKind == 12 ? 2 :
                             previousKind == 28 ? 5 : 0;
            flow->menuSelection = previousKind == 9 || previousKind == 28 ? 3 :
                                  previousKind == 12 ? 2 :
                                  previousKind == 6 ? 0 : previousKind - 1;
            M360_Trace("input.menu.kind", flow->menuKind);
        } else {
            EnterState(flow, kFlowTitle, audio);
        }
    }
}

} // namespace

void M360_FlowStart(MeleeFlow* flow, MeleeAudioStatus* audio)
{
    ZeroMemory(flow, sizeof(*flow));
    QueryPerformanceFrequency(&flow->frequency);
    flow->rulesBgm = kBgmMenu01;
    flow->state = kFlowMainMenu;
    EnterState(flow, kFlowOpening, audio);
}

void M360_FlowUpdate(MeleeFlow* flow, unsigned __int64 buttonsTriggered,
                     MeleeAudioStatus* audio)
{
    switch (flow->state) {
    case kFlowOpening: UpdateOpening(flow, buttonsTriggered, audio); break;
    case kFlowTitle: UpdateTitle(flow, buttonsTriggered, audio); break;
    default: UpdateMenu(flow, buttonsTriggered, audio); break;
    }
}

const char* M360_FlowStateName(MeleeFlowState state)
{
    switch (state) {
    case kFlowOpening: return "OPENING MOVIE";
    case kFlowTitle: return "TITLE";
    default: return "MAIN MENU";
    }
}
