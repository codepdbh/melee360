#include "gobj_xdk_compat.h"

#include <cstdio>

/*
 * M17 stubs: cobj (camera object), fog, jobj (joint/skeleton object) and
 * lobj (light object) are not ported yet. gobj.c only calls into their APIs
 * from render/teardown callbacks (HSD_GObj_LObjCallback, HSD_GObj_JObjCallback,
 * HSD_GObj_FogCallback, HSD_GObj_803910D8) and from the HSD_GObj_80408600
 * object-kind teardown table (HSD_LObjRemoveAll/HSD_JObjRemoveAll taken by
 * address only). None of these paths are exercised by GObj_Create/proc/plink/
 * userdata/destroy, which is all test_gobj.c and the main.cpp self-test
 * cover, so these are deliberately no-op stand-ins, not real ports.
 */

extern "C" {

/* gobj.c declares this `extern` for aurora's per-GX-link draw-call
 * attribution; nothing else in the ported cluster defines it yet. */
unsigned int aurora_draw_tag;

HSD_CObj* HSD_CObjGetCurrent(void)
{
    return NULL;
}

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

void HSD_JObjDispAll(HSD_JObj* jobj, void* unused, u32 flags, int arg3)
{
    (void) jobj;
    (void) unused;
    (void) flags;
    (void) arg3;
}

void HSD_JObjRemoveAll(HSD_JObj* jobj)
{
    (void) jobj;
}

void HSD_FogSet(HSD_Fog* fog)
{
    (void) fog;
}

} /* extern "C" */
