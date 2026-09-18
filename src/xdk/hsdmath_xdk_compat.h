#ifndef MELEE360_XDK_HSDMATH_COMPAT_H
#define MELEE360_XDK_HSDMATH_COMPAT_H

#include "hsd_class_xdk_compat.h"

/* dolphin/types.h types s32/u32 as long, which collides with the int-based
 * typedefs shared by every other compat header; the real dolphin/mtx.h only
 * needs the scalar typedefs, so short-circuit types.h and keep the rest of
 * the real, untouched Vec/Quaternion/Mtx declarations. */
#define _DOLPHIN_TYPES_H_

#include <pc/disc.h>
#include <sysdolphin/baselib/forward.h>

/* placeholder.h drags in m2c_macros.h; the handful of macros the ported
 * files use from it are supplied here. Stack padding exists only to match
 * MWCC codegen and is a no-op. */
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

#ifndef SQ
#define SQ(x) ((x) * (x))
#endif
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#endif
