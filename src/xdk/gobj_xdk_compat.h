#ifndef MELEE360_XDK_GOBJ_COMPAT_H
#define MELEE360_XDK_GOBJ_COMPAT_H

/* Reuse the exact typedefs/macros already established for the class/objalloc
 * cluster instead of redefining u8/u32/HSD_ASSERT/etc a third way. */
#include "hsd_class_xdk_compat.h"

/* list.h's on-disc HSD_DiscSList needs DISC_STRUCT/DISC_PTR/DISC_ASSERT_SIZE,
 * normally force-included transitively from Runtime/platform.h -> dolphin/
 * types.h -> pc/disc.h in the real PC build (see src/pc/compat.h). Our
 * RUNTIME_PLATFORM_H short-circuit skips that chain, so pull the real,
 * self-contained, untouched pc/disc.h in directly; without TARGET_PC defined
 * it collapses to the plain/identity (GameCube) branch, which is what an XEX
 * build wants anyway. */
#include <pc/disc.h>

/* Opaque forward declarations (HSD_CObj, HSD_Fog, HSD_JObj, HSD_LObj, ...)
 * for every subsystem referenced only by pointer; real, untouched header. */
#include <sysdolphin/baselib/forward.h>

/* Keep list.c/gobjobject.c/gobjuserdata.c/gobjproc.c/gobjplink.c/
 * gobjgxlink.c/gobjinit.c/gobj.c untouched while short-circuiting headers
 * for subsystems not ported yet. The preprocessor still needs to physically
 * locate every #include'd file, so build_xex.ps1/test_gobj.sh must keep
 * passing /I upstream\melee-pc\src and /I upstream\melee-pc\src\sdk_include
 * even though these guards make the file contents empty. */
#define PLACEHOLDER_H
#ifndef UNK_T
#define UNK_T void*
#endif

/* cobj.h / fog.h / jobj.h / lobj.h are NOT ported yet (separate, much larger
 * future subsystems). gobj.c only ever references their types as opaque
 * pointers (already forward-declared in forward.h) and calls a handful of
 * their functions from render/teardown callbacks that our host test never
 * exercises. Short-circuit the headers and declare just those functions
 * here, stubbed as no-ops in gobj_xdk.cpp. */
#define _cobj_h_
#define SYSDOLPHIN_BASELIB_FOG_H
#define _jobj_h_
#define SYSDOLPHIN_BASELIB_LOBJ_H

#ifdef __cplusplus
extern "C" {
#endif

/* --- stubbed cobj.h API actually referenced by gobj.c --- */
HSD_CObj* HSD_CObjGetCurrent(void);
u8 HSD_CObjSetCurrent(HSD_CObj* cobj);
void HSD_CObjEndCurrent(void);

/* --- stubbed lobj.h API actually referenced by gobj.c --- */
void HSD_LObj_803668EC(HSD_LObj* lobj);
void HSD_LObjSetupInit(HSD_CObj* cobj);
void HSD_LObjRemoveAll(HSD_LObj* lobj);

/* --- stubbed jobj.h API actually referenced by gobj.c --- */
void HSD_JObjDispAll(HSD_JObj* jobj, void* unused, u32 flags, int arg3);
void HSD_JObjRemoveAll(HSD_JObj* jobj);

/* --- stubbed fog.h API actually referenced by gobj.c --- */
void HSD_FogSet(HSD_Fog* fog);

#ifdef __cplusplus
}
#endif

#endif
