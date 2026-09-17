#include <xtl.h>

#include "controller_xdk_compat.h"

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
    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) translated |= PAD_TRIGGER_L;
    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) translated |= PAD_TRIGGER_R;
    return translated;
}

} // namespace

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
