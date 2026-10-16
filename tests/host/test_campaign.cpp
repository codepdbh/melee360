#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "melee_classic_matchups.h"
#include "melee_adventure_matchups.h"
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef int s32;
typedef float f32;
#define ASSERT_SIZE(type,size)
#define HSD_ASSERTREPORT(line,expr,...) assert(expr)
#define NUM_OPPONENTS 3
enum { ChKind_None=33,CKind_GKoops=29,Gm_PKind_Human=0,Gm_PKind_Cpu=1,Gm_PKind_NA=3 };
struct gmClassicMatchup;
struct gm_803DDEC8Struct { u8 x0,x1,x2,pad03;u16 x4,x6;u8 x8,pad09[3];gmClassicMatchup* xC; };
struct PlayerInitData {
    int ckind,slot_type,stocks,team,color,cpu_kind,cpu_level,slot,xD_b1,xC_b1,xB,vs_metal;
    float attack_ratio,defense_ratio,model_scale;
};
struct Unk1PData_x24 { s8 ckind;u8 color,cpu_level,cpu_kind;float attack_ratio,defense_ratio; };
struct Unk1PData { struct { u8 cpu_level; } x0;u8 (*x54)(u8,u8,u8); };
struct MainData { u8 c_kind; } mainData;
MainData* gmMainLib_8015CDC8() { return &mainData; }
unsigned seed;
int HSD_Randi(int count) { assert(count>0);seed=seed*1664525u+1013904223u;return (int)((seed>>8)%(unsigned)count); }
bool gm_IsCKindUnlocked(unsigned kind) { return kind<26; }
int Stage_8022519C(unsigned kind) { assert(kind<286);return g_m360StageGroundKinds[kind]; }
bool gm_80164430(unsigned stage) {
    static const int available[]={36,37,28,10,12,29,7,30,5,2,4,8,13,14,16,20,18};
    for(unsigned i=0;i<sizeof(available)/sizeof(available[0]);++i)
        if(Stage_8022519C(stage)==available[i]) return true;
    return false;
}
void gm_SetupPlayerDefaults(PlayerInitData* p) { memset(p,0,sizeof(*p));p->attack_ratio=p->defense_ratio=p->model_scale=1; }
void gm_SetupAllPlayerDefaults(PlayerInitData* p) { for(unsigned i=0;i<6;++i) gm_SetupPlayerDefaults(&p[i]); }
u8 gm_GetNumCostumesForCKind(u8) { return 6; }
s32 fn_8017DE54(u8,u8*);
s32 fn_8017D9C0(const u8*,const u8*);
extern u8 lbl_803D79F0[];
#pragma warning(push)
#pragma warning(disable: 4100 4244)
#include "campaign_original.h"
#pragma warning(pop)
#include "campaign_native.h"

int main()
{
    for(unsigned ckind=0;ckind<26;++ckind) for(unsigned s=0;s<128;++s) {
        seed=s;
        assert(M360_ClassicBegin(ckind));
        for(unsigned round=0;round<M360_CLASSIC_COMBAT_ROUNDS;++round) {
            M360CampaignEncounter e;
            assert(M360_ClassicEncounter(round,&e));
            assert(e.ordinal==s_combatOrdinals[round] && e.seconds==300);
            assert(gm_80164430(e.stkind));
            for(unsigned i=0;i<3;++i) assert(e.ckinds[i]==33 || e.ckinds[i]!=ckind);
            for(unsigned difficulty=0;difficulty<5;++difficulty) {
                PlayerInitData players[6];int freq=-1;
                const int count=M360_CampaignPlayers(3,&e,difficulty,ckind,0,3,players,&freq);
                assert(count>=2 && count<=4);
                assert(freq==(s8)gm_8017EB3C((u8)e.ordinal,(u8)difficulty));
                assert(players[0].ckind==(int)ckind && players[0].stocks==3 && players[0].slot_type==0);
                unsigned allies=0,enemies=0;
                for(int i=1;i<count;++i) {
                    assert(players[i].slot_type==1 && players[i].stocks==1);
                    if(players[i].team==players[0].team) {
                        const unsigned ally=allies;
                        ++allies;
                        assert(players[i].ckind!=(int)ckind);
                        assert(players[i].cpu_level==gm_8017ECA0((u8)e.ordinal,(u8)difficulty,(u8)ally));
                        assert(players[i].cpu_kind==gm_8017ED08((u8)e.ordinal,(u8)difficulty,(u8)ally));
                        assert(players[i].color==gm_8017ECD4((u8)e.ordinal,(u8)difficulty,(u8)ally));
                        assert(players[i].attack_ratio==gm_8017EC00((u8)e.ordinal,(u8)difficulty));
                        assert(players[i].defense_ratio==gm_8017EC50((u8)e.ordinal,(u8)difficulty));
                    } else {
                        const unsigned enemy=enemies;
                        const unsigned index=(e.flags&8) ? 0 : enemy;
                        ++enemies;
                        assert(players[i].cpu_level==gm_8017EB64((u8)e.ordinal,(u8)difficulty,(u8)index));
                        assert(players[i].cpu_kind==((e.flags&4) ? 27 : gm_8017EBCC((u8)e.ordinal,(u8)difficulty,(u8)index)));
                        assert(players[i].color==gm_8017EB98((u8)e.ordinal,(u8)difficulty,(u8)enemy)%6);
                        assert(players[i].model_scale==((e.flags&2) ? 2.0f : 1.0f));
                        assert(players[i].vs_metal==((e.flags&4)!=0));
                        assert(players[i].attack_ratio==gm_8017ED3C((u8)e.ordinal,(u8)difficulty));
                        assert(players[i].defense_ratio==gm_8017ED8C((u8)e.ordinal,(u8)difficulty));
                    }
                }
                assert(allies==(round==1 ? 1u : round==3 ? 2u : 0u));
                assert(enemies==(round==1 ? 2u : round==5 ? 3u : 1u));
                if(round==6) assert(e.stkind==175);
            }
        }
    }
    puts("PASS: original Classic shuffle/selection for 26 characters x 128 seeds; seven combat rounds x five difficulties");
    puts("PASS: original allies, teams, levels, colors, ratios, giant and metal setup; no panic loops");
    for(unsigned ckind=0;ckind<26;++ckind) for(unsigned difficulty=0;difficulty<5;++difficulty)
        for(unsigned round=0;round<M360_ADVENTURE_ROUNDS;++round) {
            const M360AdventureEncounter* source=&g_m360Adventure[round];
            M360CampaignEncounter e={source->ordinal,source->flags,source->seconds,source->opponents,source->stkind,
                {source->ckinds[0],source->ckinds[1],source->ckinds[2]}};
            PlayerInitData players[6];int freq=-1;
            const int count=M360_CampaignPlayers(4,&e,difficulty,ckind,0,3,players,&freq);
            assert(count>=1 && count<=4);
            assert(freq==(s8)gm_8017E500((u8)e.ordinal,(u8)difficulty));
            for(int i=1;i<count;++i) {
                const unsigned index=(e.flags&8) ? 0 : (unsigned)i-1;
                assert(players[i].team==4 && players[i].team!=players[0].team);
                assert(players[i].cpu_level==gm_8017E5C8((u8)e.ordinal,(u8)difficulty,(u8)index));
                assert(players[i].cpu_kind==((e.flags&4) ? 27 : gm_8017E630((u8)e.ordinal,(u8)difficulty,(u8)index)));
                assert(players[i].attack_ratio==gm_8017E528((u8)e.ordinal,(u8)difficulty));
                assert(players[i].defense_ratio==gm_8017E578((u8)e.ordinal,(u8)difficulty));
                assert(players[i].model_scale==((e.flags&2) ? 2.0f : 1.0f));
            }
        }
    puts("PASS: all twenty Adventure phases x 26 characters x five difficulties use original enemy/item tables and no extra allies");
    return 0;
}
