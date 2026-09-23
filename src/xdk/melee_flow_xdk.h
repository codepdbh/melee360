#ifndef MELEE360_XDK_FLOW_H
#define MELEE360_XDK_FLOW_H

#include <xtl.h>

#include "melee_audio_xdk.h"

enum MeleeFlowState {
    kFlowOpening,
    kFlowTitle,
    kFlowMainMenu,
    kFlowMatch
};

struct MeleeFlow {
    MeleeFlowState state;
    unsigned sceneTick;
    unsigned titleCountdown;
    unsigned titleFrames;
    unsigned rulesBgm;
    unsigned transitions;
    unsigned openingPlays;
    unsigned menuSelection;
    unsigned menuKind;
    bool movieVisible;
    bool titleVisible;
    bool movieReady;
    LARGE_INTEGER sceneStart;
    LARGE_INTEGER frequency;
};

void M360_Trace(const char* stage, unsigned value);
void M360_FlowStart(MeleeFlow* flow, MeleeAudioStatus* audio);
void M360_FlowUpdate(MeleeFlow* flow, unsigned __int64 buttonsTriggered,
                     MeleeAudioStatus* audio);
const char* M360_FlowStateName(MeleeFlowState state);
void M360_FlowSetMenuAvailable(bool available);
bool M360_FlowMenuActive(void);

#endif
