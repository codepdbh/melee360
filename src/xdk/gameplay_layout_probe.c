/* These checks deliberately bypass the legacy ASSERT_SIZE compatibility macro.
 * Passing source compilation must not hide incompatible disc record layouts.
 */
#include <melee/ft/types.h>
#include <melee/mp/types.h>

STATIC_ASSERT(sizeof(void*) == 4);
STATIC_ASSERT(sizeof(DiscU32) == 4);
STATIC_ASSERT(sizeof(DiscVec2) == 8);
STATIC_ASSERT(sizeof(struct MapLine) == 0x10);
STATIC_ASSERT(sizeof(struct MapJoint) == 0x28);
STATIC_ASSERT(sizeof(struct MapCollData) == 0x30);
STATIC_ASSERT(sizeof(struct FighterBone) == 0x10);
STATIC_ASSERT(offsetof(Fighter, motion_id) == 0x10);
STATIC_ASSERT(offsetof(Fighter, facing_dir) == 0x2C);
STATIC_ASSERT(offsetof(Fighter, self_vel) == 0x80);
STATIC_ASSERT(offsetof(Fighter, gr_vel) == 0xEC);
STATIC_ASSERT(offsetof(Fighter, co_attrs) == 0x110);
STATIC_ASSERT(offsetof(Fighter, x598) == 0x598);
#ifndef M360_REPORT_LAYOUT
STATIC_ASSERT(offsetof(Fighter, input) == 0x620);
#else
/* Diagnostic-only object: use dumpbin /rawdata to inspect big-endian words. */
unsigned M360_LayoutWords[] = {
    sizeof(Fighter), offsetof(Fighter, x2C4),
    offsetof(Fighter, x590), offsetof(Fighter, x598),
    offsetof(Fighter, tobj_list), offsetof(Fighter, input)
};
#endif

int M360_GameplayLayoutProbe(void)
{
    static Fighter fighter;
    /* The integer and bitfield views must agree on the target, not just fit.
     * x596.x7 occupies bits 8..6 of the big-endian animation flags word.
     */
    fighter.x594_s32 = 0x000001C0;
    if (fighter.x596_bits.x7 != 7 || fighter.x596_bits.x0 != 0)
        return 0;
    fighter.x594_s32 = 0;
    fighter.x596_bits.x7 = 5;
    if (fighter.x594_s32 != 0x00000140)
        return 0;
    fighter.x594_s32 = 0;
    fighter.x594_b1_loop = 1;
    return fighter.x594_s32 == 0x40000000;
}
