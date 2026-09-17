#ifndef MELEE360_XDK_PAD_H
#define MELEE360_XDK_PAD_H

typedef unsigned int M360U32;
typedef signed int M360S32;
typedef signed char M360S8;
typedef unsigned char M360U8;

#define HSD_PAD_DPADLEFT (1u << 0)
#define HSD_PAD_DPADRIGHT (1u << 1)
#define HSD_PAD_DPADDOWN (1u << 2)
#define HSD_PAD_DPADUP (1u << 3)
#define HSD_PAD_Z (1u << 4)
#define HSD_PAD_R (1u << 5)
#define HSD_PAD_L (1u << 6)
#define HSD_PAD_A (1u << 8)
#define HSD_PAD_B (1u << 9)
#define HSD_PAD_X (1u << 10)
#define HSD_PAD_Y (1u << 11)
#define HSD_PAD_START (1u << 12)

typedef struct HSD_PadStatus {
    M360U32 button;
    M360U32 last_button;
    M360U32 trigger;
    M360U32 repeat;
    M360U32 release;
    M360S32 repeat_count;
    M360S8 stickX;
    M360S8 stickY;
    M360S8 subStickX;
    M360S8 subStickY;
    M360U8 analogL;
    M360U8 analogR;
    M360U8 analogA;
    M360U8 analogB;
    float nml_stickX;
    float nml_stickY;
    float nml_subStickX;
    float nml_subStickY;
    float nml_analogL;
    float nml_analogR;
    float nml_analogA;
    float nml_analogB;
    M360U8 cross_dir;
    M360S8 err;
} HSD_PadStatus;

#ifdef __cplusplus
extern "C" {
#endif

extern HSD_PadStatus HSD_PadGameStatus[4];
void HSD_PadRenewStatus(void);
void M360_HSDPadInit(void);

#ifdef __cplusplus
}
#endif

#endif
