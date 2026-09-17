#include "lbmath_xdk_compat.h"

/* XDK math.h provides C++ inline expf/powf bodies. Rename those declarations
 * while loading the remaining CRT math API so Melee's originals stay intact. */
#define expf M360_XdkExpf
#define powf M360_XdkPowf
#include <math.h>
#undef expf
#undef powf

/* Avoid colliding with implementations exported by d3d9.lib. */
#define expf MeleeExpf
#define powf MeleePowf
extern "C" {
#include "../../upstream/melee-pc/src/melee/lb/lb_00CE.c"
}
#undef expf
#undef powf
