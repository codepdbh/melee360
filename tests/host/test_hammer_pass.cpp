#include <cassert>
#include <cstdio>
typedef float f32;
typedef int FtMotionId;
typedef unsigned MotionFlags;
struct Fighter { int marker; };
struct HSD_GObj { Fighter* user_data; };
typedef HSD_GObj Fighter_GObj;
#define GET_FIGHTER(gobj) ((gobj)->user_data)
enum { ftCo_MS_HammerFall=311, Ft_MF_None=0 };
static bool platformInput;
static unsigned drops, restores;
bool ftCo_80099F1C(Fighter_GObj*) { return platformInput; }
void ftCo_8009A184(Fighter_GObj* gobj, FtMotionId state, MotionFlags flags, float start) {
    assert(gobj->user_data->marker==42);
    assert(state==ftCo_MS_HammerFall && flags==Ft_MF_None && start==0);
    ++drops;
}
void ftCo_800C4E94(Fighter* fp) { assert(fp->marker==42);++restores; }
void ftCo_800C6110(Fighter_GObj*);
#include "hammer_pass_original.h"
int main() {
    Fighter fighter={42};Fighter_GObj object={&fighter};
    platformInput=false;
    assert(!ftCo_800C60C8(&object) && drops==0 && restores==0);
    platformInput=true;
    assert(ftCo_800C60C8(&object) && drops==1 && restores==1);
    ftCo_800C6110(&object);
    assert(drops==2 && restores==2);
    puts("PASS: original hammer platform drop preserves HammerFall and restores hammer state (mocked platform input)");
}
