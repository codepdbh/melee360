#include <xtl.h>
#include <stdio.h>

#include "controller_xdk_compat.h"

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
            if (sscanf(line, "snap %d %u", &snap, &frames) == 2) {
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
