#ifndef MELEE360_XDK_HSDSYNTH_COMPAT_H
#define MELEE360_XDK_HSDSYNTH_COMPAT_H

/* Same short-circuit strategy as hsd_class_xdk_compat.h / hsdmath_xdk_compat.h:
 * block the real Dolphin headers synth.c/devcom.c reach for (they are
 * hardware-bound: AX mixing DSP, AI streaming, AR/ARAM DMA, DVD async read,
 * OS interrupt/cache primitives) and supply just the declarations these two
 * translation units use, keeping struct layouts identical to the real SDK
 * headers (upstream/melee-pc/src/sdk_include/dolphin/{ax,ai,ar,dvd,os}.h). */
#define RUNTIME_PLATFORM_H
#define _DOLPHIN_OS_H_
#define _DOLPHIN_OSALLOC_H_
#define _DOLPHIN_OSCACHE_H_
#define _DOLPHIN_AI_H_
#define _DOLPHIN_AR_H_
#define _DOLPHIN_AX_H_
#define _DOLPHIN_DVD_H_
#define _DOLPHIN_TYPES_H_
#define _initialize_h_

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* synth.static.h/devcom.static.h align a couple of static buffers with GCC's
 * __attribute__((aligned(N))); MSVC's C front end (used for both the host
 * test and the real XDK PPC build) doesn't understand it. The alignment
 * itself is a DMA/perf detail these compat stubs don't depend on. */
#if defined(_MSC_VER) && !defined(__attribute__)
#define __attribute__(x)
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef float f32;
typedef double f64;
typedef signed int ssize_t;
typedef int enum_t;
typedef int BOOL;

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#define inline __inline
#define _Alignof __alignof
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*) 0)
#endif
#endif

#define ATTRIBUTE_NORETURN __declspec(noreturn)
#define ASSERT_SIZE(expr, size)
#define ASSERT_OFFSET(type, member, offset)

typedef int OSHeapHandle;

typedef struct OSContext {
    unsigned char reserved[16];
} OSContext;

#define OSRoundUp32B(x) (((u32) (x) + 32 - 1) & ~(u32) (32 - 1))
#define OSRoundDown32B(x) (((u32) (x)) & ~(u32) (32 - 1))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef __cplusplus
extern "C" {
#endif

void OSReport(char* fmt, ...);
ATTRIBUTE_NORETURN void OSPanic(char* file, int line, char* msg, ...);
void OSSaveContext(OSContext* context);

OSHeapHandle HSD_GetHeap(void);
void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);
long OSCheckHeap(int heap);

/* dolphin/os.h subset synth.c/devcom.c need. */
BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);
u32 OSGetSoundMode(void);
void OSSetSoundMode(u32 mode);

/* dolphin/os/OSCache.h subset devcom.c needs (ARAM/DVD DMA cache sync;
 * no-ops on Xbox 360's coherent architecture and on x86 host). */
void DCInvalidateRange(void* addr, u32 nBytes);
void DCStoreRange(void* addr, u32 nBytes);

#ifdef __cplusplus
}
#endif

#include <pc/disc.h>
#include <sysdolphin/baselib/forward.h>

#define PLACEHOLDER_H
#ifndef UNK_T
#define UNK_T void*
#endif
#ifndef UNUSED
#define UNUSED
#endif
#ifndef PAD_STACK
#define PAD_STACK(bytes) ((void) 0)
#endif
#ifndef sqrtf__Ff
#define sqrtf__Ff(x) sqrtf(x)
#endif
#ifndef sqrtf_accurate
#define sqrtf_accurate(x) sqrtf(x)
#endif

typedef void (*Event)(void);

/* ---- dolphin/ai.h subset ------------------------------------------------ */
#ifdef __cplusplus
extern "C" {
#endif
void AISetDSPSampleRate(u32 rate);
void AISetStreamVolLeft(u8 vol);
void AISetStreamVolRight(u8 vol);
#ifdef __cplusplus
}
#endif

/* ---- dolphin/ar.h subset (struct layout preserved) ---------------------- */
typedef struct ARQRequest {
    struct ARQRequest* next;
    uintptr_t owner;
    u32 type;
    u32 priority;
    uintptr_t source;
    uintptr_t dest;
    u32 length;
    void (*callback)(struct ARQRequest*);
} ARQRequest;

#ifdef __cplusplus
extern "C" {
#endif
u32 ARAlloc(u32 length);
void ARQPostRequest(ARQRequest* request, uintptr_t owner, u32 type,
                    u32 priority, uintptr_t source, uintptr_t dest,
                    u32 length, void (*callback)(ARQRequest*));
#ifdef __cplusplus
}
#endif

/* ---- dolphin/dvd.h subset ------------------------------------------------ */
typedef struct DVDFileInfo {
    u8 cb[44];
    u32 startAddr;
    u32 length;
    void (*callback)(s32, struct DVDFileInfo*);
} DVDFileInfo;

#ifdef __cplusplus
extern "C" {
#endif
s32 DVDConvertPathToEntrynum(const char* pathPtr);
BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* fileInfo);
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length,
                      s32 offset, void (*callback)(s32, DVDFileInfo*),
                      s32 prio);
#ifdef __cplusplus
}
#endif

/* ---- dolphin/ax.h subset (struct layout matches the real SDK header so
 * field access in synth.c, e.g. voice->pb.addr.currentAddressHi/Lo,
 * voice->pb.itd.*, voice->pb.state, voice->index, voice->sync, lines up). */
typedef struct _AXPBMIX {
    u16 vL, vDeltaL, vR, vDeltaR;
    u16 vAuxAL, vDeltaAuxAL, vAuxAR, vDeltaAuxAR;
    u16 vAuxBL, vDeltaAuxBL, vAuxBR, vDeltaAuxBR;
    u16 vAuxBS, vDeltaAuxBS, vS, vDeltaS;
    u16 vAuxAS, vDeltaAuxAS;
} AXPBMIX;

typedef struct _AXPBITD {
    u16 flag;
    u16 bufferHi;
    u16 bufferLo;
    u16 shiftL;
    u16 shiftR;
    u16 targetShiftL;
    u16 targetShiftR;
} AXPBITD;

typedef struct _AXPBUPDATE {
    u16 updNum[5];
    u16 dataHi;
    u16 dataLo;
} AXPBUPDATE;

typedef struct _AXPBDPOP {
    s16 aL, aAuxAL, aAuxBL, aR, aAuxAR, aAuxBR, aS, aAuxAS, aAuxBS;
} AXPBDPOP;

typedef struct _AXPBVE {
    u16 currentVolume;
    s16 currentDelta;
} AXPBVE;

typedef struct _AXPBFIR {
    u16 numCoefs, coefsHi, coefsLo;
} AXPBFIR;

typedef struct _AXPBADDR {
    u16 loopFlag;
    u16 format;
    u16 loopAddressHi;
    u16 loopAddressLo;
    u16 endAddressHi;
    u16 endAddressLo;
    u16 currentAddressHi;
    u16 currentAddressLo;
} AXPBADDR;

typedef struct _AXPBADPCM {
    u16 a[8][2];
    u16 gain;
    u16 pred_scale;
    u16 yn1;
    u16 yn2;
} AXPBADPCM;

typedef struct _AXPBSRC {
    u16 ratioHi;
    u16 ratioLo;
    u16 currentAddressFrac;
    u16 last_samples[4];
} AXPBSRC;

typedef struct _AXPBADPCMLOOP {
    u16 loop_pred_scale, loop_yn1, loop_yn2;
} AXPBADPCMLOOP;

typedef struct _AXPB {
    u16 nextHi, nextLo, currHi, currLo;
    u16 srcSelect, coefSelect, mixerCtrl, state;
    u16 type;
    AXPBMIX mix;
    AXPBITD itd;
    AXPBUPDATE update;
    AXPBDPOP dpop;
    AXPBVE ve;
    AXPBFIR fir;
    AXPBADDR addr;
    AXPBADPCM adpcm;
    AXPBSRC src;
    AXPBADPCMLOOP adpcmLoop;
    u16 pad[3];
} AXPB;

typedef struct _AXVPB {
    struct _AXVPB* next;
    struct _AXVPB* prev;
    void* next1;
    int priority;
    void (*callback)(void*);
    u32 userContext;
    u32 index;
    u32 sync;
    u32 depop;
    u32 updateMS;
    u32 updateCounter;
    u32 updateTotal;
    u16* updateWrite;
    u16 updateData[128];
    void* itdBuffer;
    AXPB pb;
} AXVPB;

#define AX_MAX_VOICES 64

#ifdef __cplusplus
extern "C" {
#endif
void AXInit(void);
void AXFreeVoice(AXVPB* p);
AXVPB* AXAcquireVoice(u32 priority, void (*callback)(void*), u32 userContext);
void AXSetVoicePriority(AXVPB* p, u32 priority);
void AXRegisterCallback(void (*callback)(void));
void AXSetVoiceState(AXVPB* p, u16 state);
void AXSetVoiceMix(AXVPB* p, AXPBMIX* mix);
void AXSetVoiceItdOn(AXVPB* p);
void AXSetVoiceItdTarget(AXVPB* p, u16 lShift, u16 rShift);
void AXSetVoiceVe(AXVPB* p, AXPBVE* ve);
void AXSetVoiceVeDelta(AXVPB* p, s16 delta);
void AXSetVoiceAddr(AXVPB* p, AXPBADDR* addr);
void AXSetVoiceLoop(AXVPB* p, u16 loop);
void AXSetVoiceLoopAddr(AXVPB* p, u32 addr);
void AXSetVoiceEndAddr(AXVPB* p, u32 addr);
void AXSetVoiceCurrentAddr(AXVPB* p, u32 addr);
void AXSetVoiceAdpcm(AXVPB* p, AXPBADPCM* adpcm);
void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src_);
void AXSetVoiceSrcRatio(AXVPB* p, float ratio);
void AXSetVoiceAdpcmLoop(AXVPB* p, AXPBADPCMLOOP* adpcmloop);
#ifdef __cplusplus
}
#endif

/* ---- test/diagnostic hooks into the AX voice-pool stub ------------------ */
typedef struct M360SynthStubState {
    int acquireCalls;
    int freeCalls;
    int stolenCalls;
    int dropCallbackCalls;
} M360SynthStubState;

#ifdef __cplusplus
extern "C" {
#endif
extern M360SynthStubState g_m360SynthStub;
void M360_SynthStubReset(void);
void M360_SynthStubSetDvdPayload(const void* data, size_t size);
void M360_SynthStubPump(void);
#ifdef __cplusplus
}
#endif

#endif
