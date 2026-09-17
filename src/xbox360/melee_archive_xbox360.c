/*
 * Compile the upstream HAL archive implementation with original-console
 * pointer semantics.  The rest of the port keeps TARGET_PC for Aurora's
 * fixed-width SDK typedefs, but archive.c's only TARGET_PC-only branch is a
 * 64-bit desktop address guard that cannot apply to Xenon's 32-bit ABI.
 */
#ifdef TARGET_PC
#undef TARGET_PC
#endif

/* Aurora only declares BOOL in its desktop branch; the original SDK uses an
 * integer BOOL on PowerPC. */
typedef int BOOL;

#include "../../upstream/melee-pc/src/sysdolphin/baselib/archive.c"
