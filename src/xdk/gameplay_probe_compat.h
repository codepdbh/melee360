#ifndef M360_GAMEPLAY_PROBE_COMPAT_H
#define M360_GAMEPLAY_PROBE_COMPAT_H
#include "hsdjobj_xdk_compat.h"
#include <float.h>
#include <math.h>
/* OSContext is already supplied by the existing XDK platform shim. */
#define _DOLPHIN_OSCONTEXT_H_
#include <sysdolphin/baselib/video.h>
#ifndef U8_MAX
#define U8_MAX 255
#endif
#ifndef M_TAU
#define M_TAU 6.283185307179586
#endif
#ifndef SDATA
#define SDATA
#endif
#define F32_MAX FLT_MAX
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif
#ifndef FP_NAN
#define FP_NAN 1
#define FP_INFINITE 2
#define FP_ZERO 3
#define FP_NORMAL 4
#define FP_SUBNORMAL 5
static __inline int M360_ClassifyDouble(double value)
{
    int kind = _fpclass(value);
    if (kind & (_FPCLASS_SNAN | _FPCLASS_QNAN)) return FP_NAN;
    if (kind & (_FPCLASS_NINF | _FPCLASS_PINF)) return FP_INFINITE;
    if (kind & (_FPCLASS_NZ | _FPCLASS_PZ)) return FP_ZERO;
    if (kind & (_FPCLASS_ND | _FPCLASS_PD)) return FP_SUBNORMAL;
    return FP_NORMAL;
}
static __inline int M360_ClassifyFloat(float value)
{
    if (value != 0 && value < FLT_MIN && value > -FLT_MIN)
        return FP_SUBNORMAL;
    return M360_ClassifyDouble((double)value);
}
#define fpclassify(value) (sizeof(value) == sizeof(float) ? M360_ClassifyFloat((float)(value)) : M360_ClassifyDouble((double)(value)))
#endif
#define M360_JOIN_INNER(a,b) a##b
#define M360_JOIN(a,b) M360_JOIN_INNER(a,b)
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(condition) typedef char M360_JOIN(m360_assert_, __LINE__)[(condition) ? 1 : -1]
#endif
typedef int BOOL;
#define RETURN_IF(cond) do { if ((cond)) { return; } } while (0)
typedef bool (*Predicate)(void);
typedef struct OSCalendarTime {
    int sec, min, hour, mday, mon, year, wday, yday, msec, usec;
} OSCalendarTime;
#endif
