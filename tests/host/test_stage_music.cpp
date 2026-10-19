#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../src/xdk/stage_music_xdk.h"
typedef unsigned char u8;
typedef unsigned u32;
typedef int s32;
typedef int enum_t;
typedef unsigned StKind;
enum { CKind_Mars=9, CKind_CLink=21, HSD_PAD_L=0x40, HSD_PAD_R=0x20, Gm_PKind_Human=0 };
struct StageParam { StKind stkind;int x4,x8;u32 xC,x10;short x14,x16; };
struct UnkArchiveStruct { StageParam* stage_params;int stage_param_count; };
struct StartMeleeData { struct { int xA; } rules;struct { int slot_type; } players[6]; };
struct Pad { unsigned button; } HSD_PadCopyStatus[6];
unsigned roll, randomCalls;
int HSD_Randi(int count) { assert(count>0);++randomCalls;return (int)(roll%(unsigned)count); }
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define HSD_ASSERT(line,expr) assert(expr)
#pragma warning(push)
#pragma warning(disable: 4018 4800)
#include "stage_music_native.h"
#pragma warning(pop)
#include "stage_music_rows.h"

static int Theme(int bgm,unsigned character) {
    return bgm==-2 ? (character<33 ? unk_arr_803BC4A0[character][roll%2] : 0x62) : bgm;
}

int main()
{
    StageParam row={7,10,11,20,21,0,35};
    UnkArchiveStruct param={&row,1};
    int alternate=-1;
    // Every probability boundary, unlock combination and forced L/R condition.
    for(int rule=0;rule<=8;++rule) for(unsigned unlocks=0;unlocks<32;++unlocks)
        for(roll=0;roll<100;++roll) for(unsigned force=0;force<2;++force) {
            row.x14=(short)rule;
            bool eligible=rule==0 || rule==1 || rule==2 ||
                (rule==3 && (unlocks&M360_MUSIC_MARTH)) ||
                (rule==4 && (unlocks&M360_MUSIC_YOUNG_LINK)) ||
                (rule==5 && (unlocks&M360_MUSIC_TROPHY_11A)) ||
                (rule==6 && (unlocks&M360_MUSIC_ALL_CHARACTERS)) ||
                (rule==7 && (unlocks&M360_MUSIC_ALL_STAGES));
            bool choose=eligible && (force || (rule==1 ? roll%12==0 : rule>=2 && roll<35));
            randomCalls=0;
            assert(M360_StageMusicChoose(&param,7,4|(force?0x40:0),0,unlocks,&alternate)==(choose?11:10));
            assert(alternate==(int)choose);
            assert(randomCalls==(eligible && rule!=0 ? 1u : 0u));
        }
    row.x14=0;row.x8=-1;row.x10=(u32)-1;
    assert(M360_StageMusicChoose(&param,7,2,0,31,&alternate)==10 && alternate==0);
    assert(M360_StageMusicChoose(&param,7,0x22,0,31,&alternate)==20 && alternate==0);
    row.x8=11;row.x10=21;
    assert(M360_StageMusicChoose(&param,7,0x12,0,31,&alternate)==21 && alternate==0);
    assert(M360_StageMusicChoose(&param,7,0x22,0,31,&alternate)==21 && alternate==1);
    puts("PASS: original music probabilities, unlock gates, forced L/R, absent alternatives and VS flags");

    for(unsigned i=0;i<ARRAY_SIZE(originalMusicRows);++i) {
        param.stage_params=&originalMusicRows[i];
        const StageParam& r=originalMusicRows[i];
        for(unsigned character=0;character<34;++character) for(roll=0;roll<2;++roll) {
            if(r.x4!=-1) {
                assert(M360_StageMusicChoose(&param,r.stkind,1,character,31,&alternate)==Theme(r.x4,character));
                assert(alternate==0);
                int expected=r.x8==-1 ? r.x4 : r.x8;
                assert(M360_StageMusicChoose(&param,r.stkind,2,character,31,&alternate)==Theme(expected,character));
                assert(alternate==(r.x8!=-1));
            }
            if(r.xC!=(u32)-1) {
                assert(M360_StageMusicChoose(&param,r.stkind,0x11,character,31,&alternate)==Theme((int)r.xC,character));
                int expected=r.x10==(u32)-1 ? (int)r.xC : (int)r.x10;
                assert(M360_StageMusicChoose(&param,r.stkind,0x22,character,31,&alternate)==Theme(expected,character));
            }
        }
    }
    printf("PASS: %u authored stage rows and original character-theme table, primary/alternate fields\n",(unsigned)ARRAY_SIZE(originalMusicRows));
    assert(M360_StageMusicChoose(NULL,7,1,0,31,&alternate)==-1);
    assert(M360_StageMusicChoose(&param,9999,1,0,31,&alternate)==-1);
    assert(M360_StageMusicChoose(&param,param.stage_params->stkind,0,0,31,&alternate)==-1);

    StartMeleeData start;
    memset(&start,0,sizeof(start));
    for(unsigned i=0;i<6;++i) start.players[i].slot_type=3;
    assert(M360_StageMusicInput(NULL)==0 && M360_StageMusicInput(&start)==0);
    start.players[0].slot_type=0;HSD_PadCopyStatus[0].button=HSD_PAD_L;
    assert(M360_StageMusicInput(&start)==1);
    start.players[3].slot_type=0;
    assert(M360_StageMusicInput(&start)==0);
    HSD_PadCopyStatus[3].button=HSD_PAD_R;
    assert(M360_StageMusicInput(&start)==1);
    HSD_PadCopyStatus[0].button=0;
    assert(M360_StageMusicInput(&start)==0);
    start.rules.xA=1;
    assert(M360_StageMusicInput(&start)==1);
    puts("PASS: original L/R input requires every human slot and preserves the rules override");
    return 0;
}
