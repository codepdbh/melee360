#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdint>
typedef unsigned u32;
typedef unsigned char u8;
enum { GA_Ground, GA_Air, Ft_Kind_Popo = 10, Ft_Kind_Nana = 11 };
struct CollData { struct { int index; u32 flags; } floor; };
struct Fighter { int ground_or_air, kind; CollData coll_data; };
struct Fighter_GObj { Fighter* user_data; };
#define GET_FIGHTER(gobj) ((gobj)->user_data)
#define PAD_STACK(bytes) ((void)0)
#define M360_SURFACE_MATERIAL_HOST
static int testGround;
static u32 testFlags;
int M360_MatchGroundKind() { return testGround; }
u32 mpLineGetFlags(int) { return testFlags; }
#pragma warning(push)
#pragma warning(disable: 4701 4703)
#include "surface_materials_original.c"
#pragma warning(pop)

int main()
{
    Fighter fighter = {};
    Fighter_GObj gobj = { &fighter };
    fighter.coll_data.floor.index = 0;
    bool nonDefaultFriction = false;
    unsigned replacedSounds = 0;
    for (int ground = 0; ground < 71; ++ground) {
        testGround = ground;
        assert(mpLib_803BF248[ground].id == ground);
        for (unsigned material = 0; material < 20; ++material) {
            const mpLib_803BF248_t_x4* original = (*mpLib_803BF248[ground].x4)[material];
            testFlags = 0x300 | material;
            fighter.coll_data.floor.flags = testFlags;
            assert(mpLib_800569EC(testFlags) == original->x0);
            assert(mpColl_8004CA6C(&fighter.coll_data) == original->x0);
            assert(ft_GetGroundFrictionMultiplier(&fighter) == original->x0);
            nonDefaultFriction |= original->x0 != 1.0f;
            int count = -2, gfx = -2, aux = -2;
            assert(mpLib_80056A1C(testFlags, &count) == original->x4 && count == original->x14[0]);
            assert(mpLib_80056A54(testFlags, &aux) == original->x14[1] && aux == original->x14[2]);
            assert(mpLib_80056A8C(testFlags, &count) == original->x20 && count == original->x30[0]);
            assert(mpLib_80056AC4(testFlags, &aux) == original->x30[1] && aux == original->x30[2]);
            assert(mpLib_80056AFC(testFlags, &count) == original->x3C && count == original->x4C[0]);
            assert(mpLib_80056B34(testFlags, &aux) == original->x4C[1] && aux == original->x4C[2]);
            for (int mode = 0; mode < 3; ++mode) {
                int sounds[4] = { 999, 999, 999, 999 };
                const int* ids = mode == 0 ? original->x4 : mode == 1 ? original->x20 : original->x3C;
                const int* attributes = mode == 0 ? original->x14 : mode == 1 ? original->x30 : original->x4C;
                bool ok = mode == 0 ? ft_80084BFC(&gobj, sounds, &count, &gfx) : mode == 1 ?
                    ft_80084C38(&gobj, sounds, &count, &gfx) : ft_80084C74(&gobj, sounds, &count, &gfx);
                assert(ok && sounds[0] == ids[0]);
                assert(count == (ids[0] == -1 ? 1 : attributes[0]));
                assert(gfx == attributes[1]);
                if (mode == 2 && ids[0] != -1) {
                    for (int i = 1; i < 4; ++i) assert(sounds[i] == ids[i]);
                }
                if (ids[0] != -1) ++replacedSounds;
            }
            fighter.kind = Ft_Kind_Popo;
            assert(ft_GetGroundFrictionMultiplier(&fighter) == 1.0f);
            fighter.kind = Ft_Kind_Nana;
            assert(ft_GetGroundFrictionMultiplier(&fighter) == 1.0f);
            fighter.kind = 0;
        }
    }
    assert(nonDefaultFriction && replacedSounds > 0);
    fighter.coll_data.floor.index = -1;
    assert(mpColl_8004CA6C(&fighter.coll_data) == 1.0f);
    int sounds[4] = { 999, 998, 997, 996 }, count = 999, gfx = 999;
    assert(!ft_80084C74(&gobj, sounds, &count, &gfx));
    fighter.coll_data.floor.index = 0;
    fighter.ground_or_air = GA_Air;
    assert(!ft_80084BFC(&gobj, sounds, &count, &gfx));
    assert(!ft_80084C38(&gobj, sounds, &count, &gfx));
    assert(count == 999 && gfx == 999 && sounds[0] == 999);
    puts("PASS: 71 original ground tables, 20 materials each, friction, footsteps, landing sounds/effects and Ice Climbers exception");
}
