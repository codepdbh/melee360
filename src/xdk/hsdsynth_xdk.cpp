#include "hsdsynth_xdk_compat.h"

extern "C" {
#include <sysdolphin/baselib/synth.h>
#include <sysdolphin/baselib/devcom.h>
}

#include <cmath>
#include <cstdio>
#include <cstring>

extern "C" M360SynthStubState g_m360SynthStub;
M360SynthStubState g_m360SynthStub;

namespace {

/* ---- AX voice pool: the only piece of "hardware" synth.c leans on for its
 * priority/voice-stealing algorithm. Real GC AX steals the active voice with
 * the lowest priority when the pool is full and the new request outranks it
 * (i.e. requested priority is strictly higher); otherwise the request fails.
 * That behaviour is real and load-bearing here, not a no-op, per the
 * milestone's "prefer something real and useful for tests" guidance. */
AXVPB s_voices[AX_MAX_VOICES];
bool s_voiceActive[AX_MAX_VOICES];
bool s_axPoolInit;

void EnsurePoolInit()
{
    if (s_axPoolInit)
        return;
    std::memset(s_voices, 0, sizeof(s_voices));
    std::memset(s_voiceActive, 0, sizeof(s_voiceActive));
    for (u32 i = 0; i < AX_MAX_VOICES; i++)
        s_voices[i].index = i;
    s_axPoolInit = true;
}

void (*s_axFrameCallback)(void);

u32 s_arCursor;
u32 s_soundMode;
BOOL s_interruptsEnabled = 1;

const unsigned char* s_dvdPayload;
size_t s_dvdPayloadSize;

/* devcom.c's own DVD/ARAM wake-up routines reassign file-scope static
 * pointers (dvdDC, aramDC in devcom.static.h) at entry and read them again
 * after the DMA they post returns. On real hardware that DMA is genuinely
 * asynchronous, so those statics are stable across the whole function. A
 * stub that invokes the completion callback *inline* from ARQPostRequest/
 * DVDReadAsyncPrio reenters those wake-up routines while the outer call is
 * still using the (now stale) static, corrupting it -- this was a real
 * crash during development. Deferring every completion to a flat queue,
 * drained only by an explicit pump the test driver calls between its own
 * top-level requests, keeps every devcom.c call synchronous-from-the-
 * caller's-perspective without ever reentering it. */
struct PendingCompletion {
    bool isArq;
    ARQRequest* arqRequest;
    void (*arqCallback)(ARQRequest*);
    DVDFileInfo* dvdFileInfo;
    void (*dvdCallback)(s32, DVDFileInfo*);
};

const int kMaxPending = 64;
PendingCompletion s_pending[kMaxPending];
int s_pendingHead;
int s_pendingTail;
int s_pendingCount;

void PushPending(const PendingCompletion& pc)
{
    if (s_pendingCount >= kMaxPending)
        return;
    s_pending[s_pendingTail] = pc;
    s_pendingTail = (s_pendingTail + 1) % kMaxPending;
    s_pendingCount++;
}

} // namespace

extern "C" {

void M360_SynthStubReset(void)
{
    EnsurePoolInit();
    std::memset(s_voices, 0, sizeof(s_voices));
    std::memset(s_voiceActive, 0, sizeof(s_voiceActive));
    for (u32 i = 0; i < AX_MAX_VOICES; i++)
        s_voices[i].index = i;
    std::memset(&g_m360SynthStub, 0, sizeof(g_m360SynthStub));
    s_arCursor = 0;
    s_soundMode = 0;
    s_interruptsEnabled = 1;
    s_dvdPayload = NULL;
    s_dvdPayloadSize = 0;
    s_pendingHead = 0;
    s_pendingTail = 0;
    s_pendingCount = 0;
}

void M360_SynthStubSetDvdPayload(const void* data, size_t size)
{
    s_dvdPayload = static_cast<const unsigned char*>(data);
    s_dvdPayloadSize = size;
}

void M360_SynthStubPump(void)
{
    while (s_pendingCount > 0) {
        PendingCompletion pc = s_pending[s_pendingHead];
        s_pendingHead = (s_pendingHead + 1) % kMaxPending;
        s_pendingCount--;
        if (pc.isArq) {
            if (pc.arqCallback != NULL)
                pc.arqCallback(pc.arqRequest);
        } else {
            if (pc.dvdCallback != NULL)
                pc.dvdCallback(0, pc.dvdFileInfo);
        }
    }
}

AXVPB* M360_SynthDebugGetVoice(int index)
{
    EnsurePoolInit();
    if (index < 0 || index >= AX_MAX_VOICES)
        return NULL;
    return &s_voices[index];
}

int M360_SynthDebugVoiceActive(int index)
{
    EnsurePoolInit();
    if (index < 0 || index >= AX_MAX_VOICES)
        return 0;
    return s_voiceActive[index] ? 1 : 0;
}

/* ---- AX (mixing DSP) --------------------------------------------------- */

void AXInit(void)
{
    EnsurePoolInit();
}

void AXRegisterCallback(void (*callback)(void))
{
    s_axFrameCallback = callback;
}

AXVPB* AXAcquireVoice(u32 priority, void (*callback)(void*), u32 userContext)
{
    EnsurePoolInit();
    g_m360SynthStub.acquireCalls++;

    int freeIdx = -1;
    for (int i = 0; i < AX_MAX_VOICES; i++) {
        if (!s_voiceActive[i]) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx < 0) {
        int lowestIdx = -1;
        u32 lowestPriority = 0xFFFFFFFFu;
        for (int i = 0; i < AX_MAX_VOICES; i++) {
            if (s_voiceActive[i] &&
                (u32) s_voices[i].priority < lowestPriority) {
                lowestPriority = (u32) s_voices[i].priority;
                lowestIdx = i;
            }
        }
        if (lowestIdx < 0 || lowestPriority >= priority)
            return NULL;

        g_m360SynthStub.stolenCalls++;
        if (s_voices[lowestIdx].callback != NULL) {
            g_m360SynthStub.dropCallbackCalls++;
            s_voices[lowestIdx].callback(&s_voices[lowestIdx]);
        }
        freeIdx = lowestIdx;
    }

    AXVPB* v = &s_voices[freeIdx];
    std::memset(v, 0, sizeof(*v));
    v->index = (u32) freeIdx;
    v->priority = (int) priority;
    v->callback = callback;
    v->userContext = userContext;
    s_voiceActive[freeIdx] = true;
    return v;
}

void AXFreeVoice(AXVPB* p)
{
    EnsurePoolInit();
    g_m360SynthStub.freeCalls++;
    if (p == NULL)
        return;
    int idx = (int) p->index;
    if (idx >= 0 && idx < AX_MAX_VOICES)
        s_voiceActive[idx] = false;
}

void AXSetVoicePriority(AXVPB* p, u32 priority)
{
    if (p != NULL)
        p->priority = (int) priority;
}

void AXSetVoiceState(AXVPB* p, u16 state)
{
    if (p != NULL)
        p->pb.state = state;
}

void AXSetVoiceMix(AXVPB* p, AXPBMIX* mix)
{
    if (p != NULL && mix != NULL)
        p->pb.mix = *mix;
}

void AXSetVoiceItdOn(AXVPB* p)
{
    if (p != NULL)
        p->pb.itd.flag = 1;
}

void AXSetVoiceItdTarget(AXVPB* p, u16 lShift, u16 rShift)
{
    if (p != NULL) {
        p->pb.itd.targetShiftL = lShift;
        p->pb.itd.targetShiftR = rShift;
    }
}

void AXSetVoiceVe(AXVPB* p, AXPBVE* ve)
{
    if (p != NULL && ve != NULL)
        p->pb.ve = *ve;
}

void AXSetVoiceVeDelta(AXVPB* p, s16 delta)
{
    if (p != NULL)
        p->pb.ve.currentDelta = delta;
}

void AXSetVoiceAddr(AXVPB* p, AXPBADDR* addr)
{
    if (p != NULL && addr != NULL)
        p->pb.addr = *addr;
}

void AXSetVoiceLoop(AXVPB* p, u16 loop)
{
    if (p != NULL)
        p->pb.addr.loopFlag = loop;
}

void AXSetVoiceLoopAddr(AXVPB* p, u32 addr)
{
    if (p != NULL) {
        p->pb.addr.loopAddressHi = (u16) (addr >> 16);
        p->pb.addr.loopAddressLo = (u16) addr;
    }
}

void AXSetVoiceEndAddr(AXVPB* p, u32 addr)
{
    if (p != NULL) {
        p->pb.addr.endAddressHi = (u16) (addr >> 16);
        p->pb.addr.endAddressLo = (u16) addr;
    }
}

void AXSetVoiceCurrentAddr(AXVPB* p, u32 addr)
{
    if (p != NULL) {
        p->pb.addr.currentAddressHi = (u16) (addr >> 16);
        p->pb.addr.currentAddressLo = (u16) addr;
    }
}

void AXSetVoiceAdpcm(AXVPB* p, AXPBADPCM* adpcm)
{
    if (p != NULL && adpcm != NULL)
        p->pb.adpcm = *adpcm;
}

void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src_)
{
    if (p != NULL && src_ != NULL)
        p->pb.src = *src_;
}

void AXSetVoiceSrcRatio(AXVPB* p, float ratio)
{
    if (p == NULL)
        return;
    u32 fixed = (u32) (65536.0f * ratio);
    p->pb.src.ratioHi = (u16) (fixed >> 16);
    p->pb.src.ratioLo = (u16) fixed;
}

void AXSetVoiceAdpcmLoop(AXVPB* p, AXPBADPCMLOOP* adpcmloop)
{
    if (p != NULL && adpcmloop != NULL)
        p->pb.adpcmLoop = *adpcmloop;
}

/* ---- AI (audio interface: stream volume/rate, no mixing state to check
 * host-side) -------------------------------------------------------------- */

void AISetDSPSampleRate(u32 rate)
{
    (void) rate;
}

void AISetStreamVolLeft(u8 vol)
{
    (void) vol;
}

void AISetStreamVolRight(u8 vol)
{
    (void) vol;
}

/* ---- AR (auxiliary RAM / ARAM DMA): a real bump allocator over a fake
 * address space, and a synchronous "DMA" that completes its callback inline
 * since there is no interrupt controller on the host or under Xenia to defer
 * to. -------------------------------------------------------------------- */

u32 ARAlloc(u32 length)
{
    u32 addr = s_arCursor;
    s_arCursor += (length + 31u) & ~31u;
    return addr;
}

void ARQPostRequest(ARQRequest* request, uintptr_t owner, u32 type,
                    u32 priority, uintptr_t source, uintptr_t dest,
                    u32 length, void (*callback)(ARQRequest*))
{
    if (request != NULL) {
        request->owner = owner;
        request->type = type;
        request->priority = priority;
        request->source = source;
        request->dest = dest;
        request->length = length;
        request->callback = callback;
    }
    PendingCompletion pc;
    pc.isArq = true;
    pc.arqRequest = request;
    pc.arqCallback = callback;
    pc.dvdFileInfo = NULL;
    pc.dvdCallback = NULL;
    PushPending(pc);
}

/* ---- DVD (async file read): synchronous completion, same rationale as
 * ARQPostRequest above. -------------------------------------------------- */

s32 DVDConvertPathToEntrynum(const char* pathPtr)
{
    u32 hash = 5381u;
    for (const char* c = pathPtr; c != NULL && *c != '\0'; c++)
        hash = hash * 33u + (unsigned char) *c;
    return (s32) (hash & 0x7FFFFFFFu);
}

BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* fileInfo)
{
    if (fileInfo != NULL) {
        std::memset(fileInfo, 0, sizeof(*fileInfo));
        fileInfo->startAddr = (u32) entrynum;
    }
    return 1;
}

BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length,
                      s32 offset, void (*callback)(s32, DVDFileInfo*),
                      s32 prio)
{
    (void) offset;
    (void) prio;
    if (addr != NULL && length > 0) {
        if (s_dvdPayload != NULL) {
            size_t n = ((size_t) length < s_dvdPayloadSize)
                          ? (size_t) length
                          : s_dvdPayloadSize;
            std::memcpy(addr, s_dvdPayload, n);
            if ((size_t) length > n)
                std::memset((unsigned char*) addr + n, 0,
                           (size_t) length - n);
            s_dvdPayload = NULL;
            s_dvdPayloadSize = 0;
        } else {
            std::memset(addr, 0, (size_t) length);
        }
    }
    PendingCompletion pc;
    pc.isArq = false;
    pc.arqRequest = NULL;
    pc.arqCallback = NULL;
    pc.dvdFileInfo = fileInfo;
    pc.dvdCallback = callback;
    PushPending(pc);
    return 1;
}

void DCInvalidateRange(void* addr, u32 nBytes)
{
    (void) addr;
    (void) nBytes;
}

void DCStoreRange(void* addr, u32 nBytes)
{
    (void) addr;
    (void) nBytes;
}

/* ---- OS (interrupts, sound mode) ---------------------------------------
 * OSDisableInterrupts/OSRestoreInterrupts are real on the Xbox 360 build
 * already (melee_pad_xdk.cpp supplies a real PPC MSR-based implementation
 * for the controller poll); this milestone reuses that one there and only
 * supplies its own host-only stand-in for the test binary, which never
 * links melee_pad_xdk.obj. */
#if !defined(_XBOX)
BOOL OSDisableInterrupts(void)
{
    BOOL prev = s_interruptsEnabled;
    s_interruptsEnabled = 0;
    return prev;
}

BOOL OSRestoreInterrupts(BOOL level)
{
    BOOL prev = s_interruptsEnabled;
    s_interruptsEnabled = level;
    return prev;
}
#endif

#if defined(_XBOX)
/* synth.c calls getenv() once (MELEE_SFX_STATS diagnostic gate, cached);
 * the Xbox 360 CRT subset linked here has no environment block. */
char* getenv(const char* name)
{
    (void) name;
    return NULL;
}
#endif

u32 OSGetSoundMode(void)
{
    return s_soundMode;
}

void OSSetSoundMode(u32 mode)
{
    s_soundMode = mode;
}

} /* extern "C" */

/* Layout-identical (not textually shared) transcription of the private
 * `struct HSD_SynthSFXNode` from synth.static.h. That header cannot be
 * #included a second time here: it defines the non-static global
 * `HSD_Synth_804D6018`, and both this TU and synth.c's are linked into the
 * same binary, so a second definition would be a duplicate-symbol error.
 * synth.c's compiled code only ever touches this struct through the pointer
 * arguments of its own exported functions (HSD_SynthSFXUpdateMix,
 * HSD_SynthSFXUpdateVolume, HSD_SynthSFXUpdatePitch, and the node list
 * HSD_Synth_8038A000 walks), so byte-identical layout compiled under the
 * same ABI (this same translation unit set, same target) is all that's
 * required for real inter-TU calls on real instances of it -- there is no
 * separate synth.c-side registry these test nodes need to be entered into. */
namespace {

const int kUserVolNum = 2;

struct M360SynthNode {
    int x0;
    int sfx_id;
    u8 pad8;
    u8 flags;
    u8 voice_count;
    u8 xB;
    AXVPB* voice[2];
    float x14;
    float x18[2];
    M360SynthNode* x20;
    u16 x24;
    u8 volume_update_pending;
    u8 x27;
    float unk28;
    struct {
        float volume;
        int x4;
        union {
            u8 x8;
            float x8_float;
        };
    } user_vol[2];
    float x44;
    float x48;
    float x4C;
};

bool Near(float a, float b, float eps)
{
    float d = a - b;
    return d <= eps && d >= -eps;
}

int s_dropCalls;
AXVPB* s_lastDropped;

void TestDropCallback(void* voice)
{
    s_dropCalls++;
    s_lastDropped = static_cast<AXVPB*>(voice);
}

/* -- Test A: voice alloc/free and priority-based voice stealing ---------- */
bool TestVoiceStealing()
{
    M360_SynthStubReset();
    s_dropCalls = 0;
    s_lastDropped = NULL;

    /* Fill the entire 64-voice pool at priority 10. */
    AXVPB* filled[AX_MAX_VOICES];
    for (int i = 0; i < AX_MAX_VOICES; i++) {
        filled[i] = AXAcquireVoice(10, TestDropCallback, 0);
        if (filled[i] == NULL)
            return false;
    }
    if (g_m360SynthStub.acquireCalls != AX_MAX_VOICES)
        return false;

    /* Pool full, no voice has priority lower than 10: acquiring at priority
     * 10 (equal, not lower) must fail without stealing anything. */
    AXVPB* failed = AXAcquireVoice(10, TestDropCallback, 0);
    bool noStealOnEqual = (failed == NULL) && (s_dropCalls == 0) &&
                          (g_m360SynthStub.stolenCalls == 0);

    /* Requesting a strictly higher priority must steal the lowest-priority
     * active voice (all are 10 here, so any one of them) and invoke its
     * drop callback exactly once. */
    AXVPB* stolen = AXAcquireVoice(20, TestDropCallback, 0);
    bool stealOk = (stolen != NULL) && (s_dropCalls == 1) &&
                  (s_lastDropped != NULL) &&
                  (g_m360SynthStub.stolenCalls == 1) && (stolen->priority == 20);

    /* Explicit free makes a slot available without stealing. */
    AXFreeVoice(stolen);
    int freedIndex = (int) stolen->index;
    bool freeOk = !M360_SynthDebugVoiceActive(freedIndex);

    int stolenBeforeReacquire = g_m360SynthStub.stolenCalls;
    AXVPB* reused = AXAcquireVoice(5, TestDropCallback, 0);
    bool reuseOk = (reused != NULL) && ((int) reused->index == freedIndex) &&
                  (g_m360SynthStub.stolenCalls == stolenBeforeReacquire);

    return noStealOnEqual && stealOk && freeOk && reuseOk;
}

/* -- Test B: pitch/frequency ratio, via the real HSD_SynthSFXUpdatePitch
 * (synth.c). ratio = x14 * x18[0] * x18[1], with x14 = 0.00003125 *
 * sampleRate, and AXSetVoiceSrcRatio (real synth.c call, stubbed above to
 * write the fixed-point 16.16 AXPBSRC ratioHi/Lo exactly like the real
 * AXVPB.c would) receiving it -- hand-verified below. --------------------- */
bool TestPitchRatio()
{
    M360_SynthStubReset();

    M360SynthNode node;
    std::memset(&node, 0, sizeof(node));
    AXVPB* v = AXAcquireVoice(1, NULL, 0);
    if (v == NULL)
        return false;
    node.voice[0] = v;
    node.voice_count = 1;
    node.flags = 0;
    node.x18[0] = 1.0f;
    node.x18[1] = 1.0f;

    node.x14 = 0.00003125f * 32000.0f; /* = 1.0 */
    HSD_SynthSFXUpdatePitch(reinterpret_cast<HSD_SynthSFXNode*>(&node));
    /* ratio = 1.0 -> 65536 * 1.0 = 0x10000 -> ratioHi = 1, ratioLo = 0. */
    bool r1 = (v->pb.src.ratioHi == 1) && (v->pb.src.ratioLo == 0);

    node.x14 = 0.00003125f * 16000.0f; /* = 0.5 */
    HSD_SynthSFXUpdatePitch(reinterpret_cast<HSD_SynthSFXNode*>(&node));
    /* ratio = 0.5 -> 65536 * 0.5 = 0x8000 -> ratioHi = 0, ratioLo = 32768. */
    bool r2 = (v->pb.src.ratioHi == 0) && (v->pb.src.ratioLo == 32768);

    node.flags = 4; /* "muted pitch": ratio forced to 0 */
    HSD_SynthSFXUpdatePitch(reinterpret_cast<HSD_SynthSFXNode*>(&node));
    bool r3 = (v->pb.src.ratioHi == 0) && (v->pb.src.ratioLo == 0);

    return r1 && r2 && r3;
}

/* -- Test C: volume/pan curve, via the real HSD_SynthSFXUpdateMix
 * (synth.c). With sound mode != 0, l/r follow a constant-power (sqrt) pan
 * curve around the 0..255 pan byte:
 *   l = 32767 * sqrt((255 - pan) / 255), r = 32767 * sqrt(pan / 255)
 * hand-verified at hard-left, center and hard-right. ---------------------- */
bool TestPanCurve()
{
    M360_SynthStubReset();
    HSD_SynthSetSoundMode(1);

    M360SynthNode node;
    std::memset(&node, 0, sizeof(node));
    AXVPB* v = AXAcquireVoice(1, NULL, 0);
    if (v == NULL)
        return false;
    node.voice[0] = v;
    node.voice_count = 1;
    node.x44 = 1.0f; /* mix_main: passes l/r through unscaled */
    node.x48 = 0.0f;
    node.x4C = 0.0f;

    auto checkPan = [&](u8 pan, float expectL, float expectR) -> bool {
        node.user_vol[1].x8 = pan;
        HSD_SynthSFXUpdateMix(reinterpret_cast<HSD_SynthSFXNode*>(&node), 1);
        return Near((float) v->pb.mix.vL, expectL, 2.0f) &&
              Near((float) v->pb.mix.vR, expectR, 2.0f);
    };

    bool center = checkPan(0x80, 32767.0f * sqrtf(127.0f / 255.0f),
                          32767.0f * sqrtf(128.0f / 255.0f));
    bool hardLeft = checkPan(0x00, 32767.0f * sqrtf(255.0f / 255.0f), 0.0f);
    bool hardRight = checkPan(0xFF, 0.0f, 32767.0f * sqrtf(255.0f / 255.0f));

    /* Mono/surround (sound mode 0): fixed equal-power split regardless of
     * pan, per synth.c's `else` branch (sqrt(0.5) * 32767). */
    HSD_SynthSetSoundMode(0);
    bool monoFixed = checkPan(0x00, 23169.768f, 23169.768f) &&
                     checkPan(0xFF, 23169.768f, 23169.768f);

    return center && hardLeft && hardRight && monoFixed;
}

/* -- Test D: envelope/volume-ramp stepping via the real HSD_Synth_8038A000
 * (synth.c), driven directly through HSD_SynthSFXUpdateVolume's node list
 * (both exported, real synth.c). vol = 32767 * x8_float * unk28 * masterVol
 * * channelVol; x24 ramps toward it by at most 20 * 160 = 3200 per call
 * (the delta clamp), hand-verified below. Channel volume (normally primed
 * by HSD_SynthInit, which is skipped here since it touches real AX/AR init
 * calls out of scope for this test) is primed through the real exported
 * HSD_SynthSFXUpdateAllVolume + one HSD_Synth_8038A000 step first. --------- */
bool TestVolumeEnvelope()
{
    M360_SynthStubReset();

    HSD_SynthSFXUpdateAllVolume(255, 1, 0); /* channel 0 target = 1.0, 1 frame */
    HSD_Synth_8038A000();                  /* channel fade section converges it */

    M360SynthNode node;
    std::memset(&node, 0, sizeof(node));
    AXVPB* v = AXAcquireVoice(1, NULL, 0);
    if (v == NULL)
        return false;
    node.x0 = 1; /* must be > 0 for HSD_Synth_8038A000 to keep processing it */
    node.xB = 0; /* channel index */
    node.voice_count = 1;
    node.voice[0] = v;
    node.flags = 0;

    /* Silent starting point: link, step once, expect immediate settle
     * (target 0 == starting x24 0) and self-unlink (verified indirectly:
     * a second HSD_SynthSFXUpdateVolume call below must re-link it, which
     * only makes sense if the first pass actually cleared
     * volume_update_pending). */
    HSD_SynthSFXUpdateVolume(reinterpret_cast<HSD_SynthSFXNode*>(&node));
    bool wasPendingBefore = node.volume_update_pending != 0;
    HSD_Synth_8038A000();
    bool settledSilent = node.volume_update_pending == 0;

    /* Now drive it to full volume: unk28 * x8_float * masterVol *
     * channelVol = 1 * 1 * 1 * 1 -> target 32767. */
    node.unk28 = 1.0f;
    node.user_vol[0].x8_float = 1.0f;
    HSD_SynthSFXUpdateVolume(reinterpret_cast<HSD_SynthSFXNode*>(&node));
    bool relinked = node.volume_update_pending != 0;

    /* delta clamps to +-20 raw units, applied as node->x24 += delta * 160,
     * i.e. up to 3200 per call; ceil(32767 / 3200) = 11 calls to fully
     * converge (the last call may land exactly or snap via the
     * delta==0-and-target-reached branch). Give a generous margin. */
    bool reached = false;
    int steps = 0;
    for (; steps < 32; steps++) {
        HSD_Synth_8038A000();
        if (node.x24 >= 32767 - 1) {
            reached = true;
            break;
        }
    }
    bool convergedFast = reached && steps <= 15;
    bool voiceSnapped = v->pb.ve.currentVolume >= 32767 - 1;
    bool stillActive = M360_SynthDebugVoiceActive((int) v->index) != 0;

    return wasPendingBefore && settledSilent && relinked && convergedFast &&
          voiceSnapped && stillActive;
}

/* -- Test E: devcom queue enqueue/ordering/cancel and free-list reuse
 * ("wraparound" of the fixed 16-entry devcom pool), via the real
 * HSD_DevComRequest / HSD_DevComCancelEx (devcom.c). ---------------------- */
bool TestDevComQueue()
{
    M360_SynthStubReset();

    /* dcReq = counter + pri, counter += 4 per request; pri is folded into
     * the low bits so devComStatus[dcReq & 3] finds the right bucket. Type
     * 0x21 ((type & 0x38) == 0x20) is one of the types that keeps the
     * caller-supplied pri instead of forcing it to 3. */
    int r0 = HSD_DevComRequest(0, 0, 0, 32, 0x21, 0, NULL, NULL);
    M360_SynthStubPump();
    int r1 = HSD_DevComRequest(0, 0, 0, 32, 0x21, 1, NULL, NULL);
    M360_SynthStubPump();
    int r2 = HSD_DevComRequest(0, 0, 0, 32, 0x21, 2, NULL, NULL);
    M360_SynthStubPump();
    bool ordering = ((r1 - r0) == 5) && ((r2 - r1) == 5) &&
                    ((r0 & 3) == 0) && ((r1 & 3) == 1) && ((r2 & 3) == 2);

    /* Each request's DMA "completion" is only a flat, non-reentrant pump
     * away (see M360_SynthStubPump above), which is the correct behaviour
     * for a synchronous DMA stand-in and still exercises real enqueue/
     * dequeue bookkeeping (dcReq allocation, bucket selection, the
     * recursive wake-up chain real devcom.c runs once its DMA completes). */

    /* Free-list ("wraparound") reuse: run far more requests than the
     * INIT_N_DEVCOMS(16) pool so every slot must be recycled through
     * HSD_DevCom_804D77F0 multiple times; if that free list were broken
     * every request would grow the heap and OSCheckHeap() would trend
     * downward without bound. */
    long before = OSCheckHeap(0);
    for (int i = 0; i < 200; i++) {
        HSD_DevComRequest(0, 0, 0, 32, 3, i % 4, NULL, NULL);
        M360_SynthStubPump();
    }
    long after = OSCheckHeap(0);
    bool noLeak = (before - after) < (long) (32 * 1024);

    /* Cancel a genuinely still-queued (non-head) request. Every completion
     * here only runs from the flat pump loop, never reentrantly, so a
     * request never sits idle in the queue mid-pump -- except for the brief
     * window while an earlier request in the SAME bucket is inside its own
     * completion callback (real devcom.c unlinks a request only *after*
     * invoking its callback; see HSD_DevComARAMCallback). Enqueue-and-
     * immediately-cancel a second request from within the first's callback
     * lands exactly on HSD_DevComCancelEx's "still linked but not the
     * bucket head" branch. */
    static bool cancelSeen;
    static int cancelDcReq;
    static int secondReq;
    cancelSeen = false;
    cancelDcReq = 0;
    secondReq = -1;
    struct Local {
        static void secondCb(int dcReq, uintptr_t, void*, bool cancelflag)
        {
            if (cancelflag) {
                cancelSeen = true;
                cancelDcReq = dcReq;
            }
        }
        static void firstCb(int, uintptr_t, void*, bool)
        {
            secondReq = HSD_DevComRequest(0, 0, 0, 32, 3, 0, &secondCb, NULL);
            HSD_DevComCancelEx(secondReq, 0, NULL, NULL);
        }
    };
    HSD_DevComRequest(0, 0, 0, 32, 3, 0, &Local::firstCb, NULL);
    M360_SynthStubPump();
    bool cancelOk = cancelSeen && secondReq >= 0 && cancelDcReq == secondReq;

    return ordering && noLeak && cancelOk;
}

bool SelfTestHsdSynth()
{
    bool a = TestVoiceStealing();
    bool b = TestPitchRatio();
    bool c = TestPanCurve();
    bool d = TestVolumeEnvelope();
    bool e = TestDevComQueue();
    return a && b && c && d && e;
}

} // namespace

extern "C" int M360_HsdSynthSelfTest(void)
{
    return SelfTestHsdSynth() ? 1 : 0;
}
