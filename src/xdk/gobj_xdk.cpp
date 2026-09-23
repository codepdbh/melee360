#include "gobj_xdk_compat.h"

#include <cstdio>

/*
 * M17 stubs: cobj (camera object), fog and lobj (light object) are not ported
 * yet. gobj.c only calls into their APIs from render/teardown callbacks
 * (HSD_GObj_LObjCallback, HSD_GObj_JObjCallback, HSD_GObj_FogCallback,
 * HSD_GObj_803910D8) and from the HSD_GObj_80408600 object-kind teardown
 * table (HSD_LObjRemoveAll taken by address only). None of these paths are
 * exercised by GObj_Create/proc/plink/userdata/destroy, which is all
 * test_gobj.c and the main.cpp self-test cover, so these are deliberately
 * no-op stand-ins, not real ports. jobj is real since M20: HSD_JObjDispAll
 * and HSD_JObjRemoveAll come from jobj.c, and HSD_CObjGetCurrent is a
 * controllable stub owned by hsdjobj_xdk.cpp.
 */

extern "C" {

/* gobj.c declares this `extern` for aurora's per-GX-link draw-call
 * attribution; nothing else in the ported cluster defines it yet. */
unsigned int aurora_draw_tag;

#ifndef M360_NATIVE_RENDER
u8 HSD_CObjSetCurrent(HSD_CObj* cobj)
{
    (void) cobj;
    return 0;
}

void HSD_CObjEndCurrent(void)
{
}

void HSD_LObj_803668EC(HSD_LObj* lobj)
{
    (void) lobj;
}

void HSD_LObjSetupInit(HSD_CObj* cobj)
{
    (void) cobj;
}

void HSD_LObjRemoveAll(HSD_LObj* lobj)
{
    (void) lobj;
}

void HSD_FogSet(HSD_Fog* fog)
{
    (void) fog;
}

#endif

} /* extern "C" */
