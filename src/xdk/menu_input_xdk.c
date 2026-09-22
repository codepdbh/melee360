/* Target-side tests of the original gm controller mapping, not a menu mock. */
#include <string.h>
#include <melee/gm/gm_1A36.h>
#include <dolphin/pad.h>
#include <sysdolphin/baselib/controller.h>

unsigned M360_MenuInputSelfTest(void)
{
    HSD_PadStatus saved[PAD_MAX_CONTROLLERS];
    unsigned passed = 0;
    u64 input;
    int frame;
    memcpy(saved, HSD_PadCopyStatus, sizeof(saved));
    memset(HSD_PadCopyStatus, 0, sizeof(saved));
    gm_801A3E88();
    HSD_PadCopyStatus[0].button = PAD_BUTTON_START;
    HSD_PadCopyStatus[0].trigger = PAD_BUTTON_START;
    gm_EvaluateAllControllerInputs();
    input = gm_GetButtonsTriggered(PAD_MAX_CONTROLLERS);
    if ((input & (PAD_BUTTON_START | PAD_CONFIRM)) ==
        (PAD_BUTTON_START | PAD_CONFIRM)) passed |= 1;

    HSD_PadCopyStatus[0].trigger = 0;
    gm_EvaluateAllControllerInputs();
    if (gm_GetButtonsTriggered(PAD_MAX_CONTROLLERS) == 0 &&
        (gm_GetButtonsPressed(0) & PAD_CONFIRM)) passed |= 2;

    memset(HSD_PadCopyStatus, 0, sizeof(saved));
    HSD_PadCopyStatus[3].button = PAD_BUTTON_B;
    HSD_PadCopyStatus[3].trigger = PAD_BUTTON_B;
    gm_EvaluateAllControllerInputs();
    if ((gm_GetButtonsTriggered(PAD_MAX_CONTROLLERS) & PAD_CANCEL) &&
        gm_GetButtonsTriggered(0) == 0) passed |= 4;

    memset(HSD_PadCopyStatus, 0, sizeof(saved));
    HSD_PadCopyStatus[1].button = PAD_BUTTON_DOWN;
    HSD_PadCopyStatus[1].trigger = PAD_BUTTON_DOWN;
    gm_EvaluateAllControllerInputs();
    if (gm_GetButtonsTriggered(PAD_MAX_CONTROLLERS) & PAD_ANY_DOWN)
        passed |= 8;
    HSD_PadCopyStatus[1].trigger = 0;
    for (frame = 0; frame < 21; ++frame)
        gm_EvaluateAllControllerInputs();
    if (gm_801A36C0(PAD_MAX_CONTROLLERS) & PAD_ANY_DOWN)
        passed |= 16;
    memcpy(HSD_PadCopyStatus, saved, sizeof(saved));
    gm_801A3E88();
    return passed;
}
