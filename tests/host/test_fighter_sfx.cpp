#include <assert.h>
#include <stdio.h>
// Sound selection uses only these fields. ABI/layout remains covered by the
// PowerPC gameplay layout probe; this harness executes the original selectors.
typedef int s32;
typedef signed char s8;
typedef int enum_t;
enum { Ft_Kind_Popo = 10, Ft_Kind_Nana = 11 };
struct Fighter {
    int kind, x619_costume_id, x2228_b3, x2220_b5, x2220_b6, is_metal;
};
int lbAudioAx_800233EC(int id) { return id; }
bool ftCommon_80080144(Fighter*);
#pragma warning(push)
#pragma warning(disable : 4101) // original codegen stack padding
#include "fighter_sfx_original.h"
#pragma warning(pop)

int main(void)
{
    Fighter f = {};
    assert(ft_80087D0C(&f, 377) == 377);
    f.x2220_b5 = 1;
    assert(ft_80087D0C(&f, 377) == 378);
    assert(ft_80087D0C(&f, 332) == 333);
    f.is_metal = 1;
    assert(ft_80087D0C(&f, 377) == 381);
    assert(ft_80087D0C(&f, 332) == 333); // no metal variant for this group
    f.x2220_b5 = 0;
    f.x2220_b6 = 1;
    assert(ft_80087D0C(&f, 377) == 382);
    f.is_metal = 0;
    f.x2220_b6 = 0;
    assert(ft_80087D0C(&f, 180000) == 180000);
    f.kind = Ft_Kind_Popo;
    assert(ft_80087D0C(&f, 130165) == 130063);
    f.x619_costume_id = 2;
    assert(ft_80087D0C(&f, 130063) == 130165);
    assert(ft_80087D0C(&f, 130165) == 130165);
    f.kind = Ft_Kind_Nana;
    assert(ft_80087D0C(&f, 130063) == 130165);
    f.x619_costume_id = 1;
    assert(ft_80087D0C(&f, 130165) == 130063);
    assert(lbAudioAx_80023130(-1) == 55);
    assert(lbAudioAx_80023130(0x83D60) == 55);
    puts("PASS: original fighter size/metal variants and Ice Climbers costume voices");
    return 0;
}
