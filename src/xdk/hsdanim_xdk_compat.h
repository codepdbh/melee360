#ifndef MELEE360_XDK_HSDANIM_COMPAT_H
#define MELEE360_XDK_HSDANIM_COMPAT_H

#include "hsdmath_xdk_compat.h"

#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>
#define _DOLPHIN_GX_H_

#include <math.h>
#include <stdint.h>
#include <string.h>

typedef void (*Event)(void);

typedef struct M360AnimStubState {
    int mobjSetCurrent;
    int mobjRemoveAnimByFlags;
    int mobjAddAnim;
    int mobjReqAnimByFlags;
    int mobjAnim;
    int mobjLoadDesc;
    int mobjRemove;
    int pobjRemoveAnimAllByFlags;
    int pobjAddAnimAll;
    int pobjReqAnimAllByFlags;
    int pobjAnimAll;
    int pobjLoadDesc;
    int pobjRemoveAll;
    int pobjResolveRefsAll;
    u32 lastFlags;
    f32 lastFrame;
    void* lastPtr;
    void* lastPtr2;
    HSD_MObj* mobjLoadDescResult;
    HSD_PObj* pobjLoadDescResult;
} M360AnimStubState;

#ifdef __cplusplus
extern "C" {
#endif
extern M360AnimStubState g_m360AnimStub;
#ifdef __cplusplus
}
#endif

#endif
