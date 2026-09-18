#include "hsdmath_xdk_compat.h"

#include <math.h>

/* The XDK C front end rejects the mixed declarations/statements in spline.c,
 * so the untouched original is compiled as C++ with C linkage instead. */
extern "C" {
#include <sysdolphin/baselib/spline.c>
}
