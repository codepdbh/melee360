/* Native scene boundary for original Classic selection and 1P setup tables.
 * Bonus courses, boss scenes, introductions and the enemy generator are pending. */
#include <string.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/gm_1B03.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gmregclear.h>
#include <melee/gm/gmregcommon.h>
#include <melee/gr/stage.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/debug.h>
#include "campaign_xdk.h"
#include "campaign_original.h"

static const u8 s_combatOrdinals[M360_CLASSIC_COMBAT_ROUNDS] = { 0, 1, 3, 4, 6, 7, 9 };
static unsigned s_setupMode;
static u8 AllyLevel(s32 round, s32 difficulty, u8 index) {
    return s_setupMode==3 ? gm_8017ECA0((u8)round,(u8)difficulty,index) : gm_8017E704((u8)round,(u8)difficulty,index);
}
static u8 AllyColor(s32 round, s32 difficulty, u8 index) {
    return s_setupMode==3 ? gm_8017ECD4((u8)round,(u8)difficulty,index) : gm_8017E738((u8)round,(u8)difficulty,index);
}
static u8 AllyKind(s32 round, s32 difficulty, u8 index) {
    return s_setupMode==3 ? gm_8017ED08((u8)round,(u8)difficulty,index) : gm_8017E76C((u8)round,(u8)difficulty,index);
}
static f32 AllyAttack(s32 round, s32 difficulty) {
    return s_setupMode==3 ? gm_8017EC00((u8)round,(u8)difficulty) : gm_8017E664((u8)round,(u8)difficulty);
}
static f32 AllyDefense(s32 round, s32 difficulty) {
    return s_setupMode==3 ? gm_8017EC50((u8)round,(u8)difficulty) : gm_8017E6B4((u8)round,(u8)difficulty);
}

int M360_ClassicBegin(unsigned ckind)
{
    unsigned i;
    gmMainLib_8015CDC8()->c_kind=(u8)ckind;
    for(i=0;i<12;++i) gmClassic_803DDEC8.x00[i].xC=NULL;
    gmClassic_InitMatchupOrder(gmClassic_803DDEC8.x2B0,(gmClassicOrderIndex*)&gm_804908A0[0x60]);
    gmClassic_InitMatchupOrder(gmClassic_803DDEC8.x26C,(gmClassicOrderIndex*)&gm_804908A0[0x54]);
    gmClassic_InitMatchupOrder(gmClassic_803DDEC8.x1B8,(gmClassicOrderIndex*)&gm_804908A0[0x34]);
    gmClassic_InitMatchupOrder(gmClassic_803DDEC8.x0CC,(gmClassicOrderIndex*)&gm_804908A0[0x0C]);
    return gmClassic_801B2D54(gmClassic_803DDEC8.x00)!=NULL;
}

int M360_ClassicEncounter(unsigned round, M360CampaignEncounter* out)
{
    gm_803DDEC8Struct* row;
    unsigned i;
    if(round>=M360_CLASSIC_COMBAT_ROUNDS) return 0;
    row=&gmClassic_803DDEC8.x00[s_combatOrdinals[round]];
    if(!row->xC) return 0;
    out->ordinal=row->x0;out->flags=row->x1;out->seconds=row->x4;
    out->opponents=row->x6;out->stkind=row->x1==4 ? 0xAF : row->xC->x00;
    for(i=0;i<3;++i) out->ckinds[i]=row->xC->x02_u8[i];
    return 1;
}

int M360_CampaignPlayers(unsigned mode, const M360CampaignEncounter* e,
    unsigned difficulty, unsigned ckind, unsigned color, unsigned stocks,
    PlayerInitData* players, int* itemFreq)
{
    Unk1PData_x24 allies[3];
    u8 enemies[3],colors[3];
    unsigned i, slot, count=0;
    f32 attack,defense;
    if((mode!=3 && mode!=4) || difficulty>4 || e->ordinal>=(mode==3 ? 11u : 22u)) return 0;
    s_setupMode=mode;
    for(i=0;i<3;++i) { enemies[i]=(u8)e->ckinds[i];if(enemies[i]!=ChKind_None) ++count; }
    memset(allies,0,sizeof(allies));
    for(i=0;i<3;++i) allies[i].ckind=ChKind_None;
    /* Only Classic's intro calls gm_8017DB88. Adventure initializes an
     * empty ally list; giant flags alone must not create extra allies. */
    if(mode==3) gm_8017DB88(allies,(u8)e->flags,(s32)difficulty,(s32)e->ordinal,enemies,(u8)ckind,
        AllyLevel,AllyKind,AllyColor,AllyAttack,AllyDefense);
    for(i=0;i<3;++i) if((u8)allies[i].ckind==ChKind_None) break;
    if(count+i+1>4) return 0;
    gm_SetupAllPlayerDefaults(players);
    for(i=0;i<6;++i) players[i].slot_type=Gm_PKind_NA;
    gm_SetupHumanPlayer(players,(s8)ckind,(u8)color,(u8)stocks,0);
    slot=(unsigned)fn_8017DD7C(players,allies,0)+1;
    *itemFreq=(s8)(mode==3 ? gm_8017EB3C((u8)e->ordinal,(u8)difficulty) : gm_8017E500((u8)e->ordinal,(u8)difficulty));
    attack=mode==3 ? gm_8017ED3C((u8)e->ordinal,(u8)difficulty) : gm_8017E528((u8)e->ordinal,(u8)difficulty);
    defense=mode==3 ? gm_8017ED8C((u8)e->ordinal,(u8)difficulty) : gm_8017E578((u8)e->ordinal,(u8)difficulty);
    for(i=0;i<3;++i) {
        unsigned index=(e->flags&8) ? 0 : i;
        colors[i]=mode==3 ? gm_8017EB98((u8)e->ordinal,(u8)difficulty,(u8)index) : gm_8017E5FC((u8)e->ordinal,(u8)difficulty,(u8)index);
    }
    gmRegSetupEnemyColorTable((s8)ckind,(u8)color,(s8*)enemies,colors);
    for(i=0;i<3;++i) if(enemies[i]!=ChKind_None) {
        unsigned index=(e->flags&8) ? 0 : i;
        PlayerInitData* p=&players[slot++];
        gm_SetupPlayerDefaults(p);
        p->slot_type=Gm_PKind_Cpu;p->ckind=(s8)enemies[i];p->stocks=1;p->team=4;
        p->cpu_level=mode==3 ? gm_8017EB64((u8)e->ordinal,(u8)difficulty,(u8)index) : gm_8017E5C8((u8)e->ordinal,(u8)difficulty,(u8)index);
        p->cpu_kind=mode==3 ? gm_8017EBCC((u8)e->ordinal,(u8)difficulty,(u8)index) : gm_8017E630((u8)e->ordinal,(u8)difficulty,(u8)index);
        p->color=colors[i];
        p->attack_ratio=attack;p->defense_ratio=defense;
        p->model_scale=(e->flags&2) ? 2.0f : 1.0f;
        if(e->flags&4) { p->vs_metal=1;p->cpu_kind=27; }
    }
    return (int)slot;
}
