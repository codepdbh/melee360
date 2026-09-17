#ifndef MELEE360_XDK_CONTROLLER_COMPAT_H
#define MELEE360_XDK_CONTROLLER_COMPAT_H

/* Suppress the GameCube headers after supplying the ABI used by controller.c. */
#define _controller_h_
#define SYSDOLPHIN_BASELIB_RUMBLE_H
#define _DOLPHIN_PAD_H_
#define _DOLPHIN_OS_H_

#include "melee_pad_xdk.h"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#define inline __inline
#endif

#define PAD_CHAN0_BIT 0x80000000u
#define PAD_CHAN1_BIT 0x40000000u
#define PAD_CHAN2_BIT 0x20000000u
#define PAD_CHAN3_BIT 0x10000000u

#define PAD_BUTTON_LEFT (1u << 0)
#define PAD_BUTTON_RIGHT (1u << 1)
#define PAD_BUTTON_DOWN (1u << 2)
#define PAD_BUTTON_UP (1u << 3)
#define PAD_TRIGGER_Z (1u << 4)
#define PAD_TRIGGER_R (1u << 5)
#define PAD_TRIGGER_L (1u << 6)
#define PAD_BUTTON_A (1u << 8)
#define PAD_BUTTON_B (1u << 9)
#define PAD_BUTTON_X (1u << 10)
#define PAD_BUTTON_Y (1u << 11)
#define PAD_BUTTON_START (1u << 12)

typedef struct PADStatus {
    u16 button;
    s8 stickX;
    s8 stickY;
    s8 substickX;
    s8 substickY;
    u8 triggerLeft;
    u8 triggerRight;
    u8 analogA;
    u8 analogB;
    s8 err;
} PADStatus;

typedef struct HSD_PadData {
    PADStatus stat[4];
} HSD_PadData;

typedef struct HSD_PadRumbleListData HSD_PadRumbleListData;

struct RumbleInfo {
    u16 max_list;
    u8 unk2;
    HSD_PadRumbleListData* listdatap;
};

typedef enum HSD_FlushType {
    HSD_PAD_FLUSH_QUEUE_MERGE,
    HSD_PAD_FLUSH_QUEUE_THROWAWAY,
    HSD_PAD_FLUSH_QUEUE_LEAVE1,
    HSD_PAD_FLUSH_QUEUE_TERMINATE,
} HSD_FlushType;

typedef struct PadLibData {
    u8 qnum;
    u8 qread;
    u8 qwrite;
    u8 qcount;
    u8 qtype;
    HSD_PadData* queue;
    s32 repeat_start;
    s32 repeat_interval;
    s8 adc_type;
    s8 adc_th;
    f32 adc_angle;
    u8 clamp_stickType;
    u8 clamp_stickShift;
    s8 clamp_stickMax;
    s8 clamp_stickMin;
    u8 clamp_analogLRShift;
    u8 clamp_analogLRMax;
    u8 clamp_analogLRMin;
    u8 clamp_analogABShift;
    u8 clamp_analogABMax;
    u8 clamp_analogABMin;
    s8 scale_stick;
    u8 scale_analogLR;
    u8 scale_analogAB;
    u8 cross_dir;
    u8 reset_switch_status;
    u8 reset_switch;
    struct RumbleInfo rumble_info;
} PadLibData;

#ifdef __cplusplus
extern "C" {
#endif

u32 OSDisableInterrupts(void);
void OSRestoreInterrupts(u32 level);
int OSGetResetSwitchState(void);
int PADRead(PADStatus* status);
int PADReset(unsigned long mask);
int PADRecalibrate(u32 mask);
int PADInit(void);
void HSD_PadRumbleInterpret(void);
void HSD_PadRumbleRemoveAll(void);
void HSD_PadRumbleOffN(u8 channel);
void HSD_PadRumbleInit(u16 count, void* data);
void HSD_PadInit(u8 qnum, HSD_PadData* queue, u16 count,
                 HSD_PadRumbleListData* data);

#ifdef __cplusplus
}
#endif

#endif
