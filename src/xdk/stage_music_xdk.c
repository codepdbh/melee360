#include <melee/gr/types.h>
#include <melee/gm/types.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/random.h>
#include "stage_music_xdk.h"

/* The original selector writes only param and the alternate-track bit.
 * Keep this context private so music cannot reset collision/stage state. */
static struct {
    UnkArchiveStruct* param;
    struct { unsigned b0; } unk8C;
} s_musicContext;
static unsigned s_musicCharacter, s_musicUnlocks;

#undef DP
#define DP(T, slot) ((T*) (uintptr_t) (slot))
#define stage_info s_musicContext
#define gm_IsCKindUnlocked(kind) ((kind)==CKind_Mars ? (s_musicUnlocks&M360_MUSIC_MARTH)!=0 : (s_musicUnlocks&M360_MUSIC_YOUNG_LINK)!=0)
#define Toy_803048C0(id) ((s_musicUnlocks&M360_MUSIC_TROPHY_11A)!=0)
#define gm_80164ABC() ((s_musicUnlocks&M360_MUSIC_ALL_CHARACTERS)!=0)
#define gm_80164600() ((s_musicUnlocks&M360_MUSIC_ALL_STAGES)!=0)
#define Player_GetPlayerCharacter(slot) ((int)s_musicCharacter)
#include "stage_music_original.h"

int M360_StageMusicChoose(void* param, unsigned stkind, unsigned flags,
                         unsigned character, unsigned unlocks, int* alternate)
{
    StageParam* rows;
    int i, bgm = -1;
    if (alternate) *alternate = 0;
    s_musicContext.param = (UnkArchiveStruct*)param;
    if (!param || !s_musicContext.param->stage_params ||
        s_musicContext.param->stage_param_count <= 0) return -1;
    rows = DP(StageParam, s_musicContext.param->stage_params);
    for (i=0; i<s_musicContext.param->stage_param_count; ++i)
        if ((unsigned)rows[i].stkind==stkind) break;
    /* Retail asserts on missing rows. Unavailable native stages report an
     * absent track instead of entering that fatal path. */
    if (i==s_musicContext.param->stage_param_count || !(flags&7)) return -1;
    s_musicCharacter=character;
    s_musicUnlocks=unlocks;
    s_musicContext.unk8C.b0=0;
    Ground_801C24F8((StKind)stkind,flags,&bgm);
    if (alternate) *alternate=(int)s_musicContext.unk8C.b0;
    return bgm;
}

unsigned M360_StageMusicInput(void* start)
{
    return start ? fn_8016E5C0((StartMeleeData*)start) : 0;
}
