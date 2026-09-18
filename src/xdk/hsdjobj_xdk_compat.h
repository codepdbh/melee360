#ifndef MELEE360_XDK_HSDJOBJ_COMPAT_H
#define MELEE360_XDK_HSDJOBJ_COMPAT_H

#include "hsdanim_xdk_compat.h"

typedef struct M360JObjStubState {
    int disp;
    int dispSub;
    int makePositionMtx;
    int cobjGetCurrent;
    u32 lastFlags;
    u32 lastRenderMode;
    void* lastJObj;
    void* lastVMtx;
    float lastVMtxCopy[12];
    void* currentCObj;
} M360JObjStubState;

#ifdef __cplusplus
extern "C" {
#endif
extern M360JObjStubState g_m360JObjStub;
int M360_HsdJObjSelfTest(void);
int M360_HsdJObjStubDispCount(void);
#ifdef __cplusplus
}
#endif

#endif
