#ifndef M360_GAMEPLAY_PROBE_COMPAT_H
#define M360_GAMEPLAY_PROBE_COMPAT_H
#include "hsdjobj_xdk_compat.h"
#define M360_JOIN_INNER(a,b) a##b
#define M360_JOIN(a,b) M360_JOIN_INNER(a,b)
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(condition) typedef char M360_JOIN(m360_assert_, __LINE__)[(condition) ? 1 : -1]
#endif
typedef int BOOL;
#endif
