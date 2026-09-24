#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_Damage.h>
#include <melee/pl/player.h>
#include <melee/it/item.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Ottotto.h>
#include <melee/ft/ft_0D4D.h>
#include <melee/cm/types.h>
#include <melee/ft/kinds/ftMario/ftmario.h>
#include <melee/ft/kinds/ftMario/ftmariospecialhi.h>
#include <melee/ft/kinds/ftMario/ftmariospeciallw.h>
#include <melee/ft/kinds/ftMario/ftmariospecialn.h>
#include <melee/ft/kinds/ftMario/ftmariospecials.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/kinds/ftLuigi/ftluigi.h>
#include <melee/ft/kinds/ftLuigi/ftluigispecialn.h>
#include <melee/ft/kinds/ftLuigi/ftluigispecials.h>
#include <melee/ft/kinds/ftLuigi/ftluigispecialhi.h>
#include <melee/ft/kinds/ftLuigi/ftluigispeciallw.h>
#include <melee/ft/kinds/ftDrMario/ftdrmario.h>
#include <melee/ft/kinds/ftPopo/ftpopo.h>
#include <melee/ft/kinds/ftPopo/ftpopospecialhi.h>
#include <melee/ft/kinds/ftPopo/ftpopospeciallw.h>
#include <melee/ft/kinds/ftPopo/ftpopospecialn.h>
#include <melee/ft/kinds/ftPopo/ftpopospecials.h>
#include <melee/ft/kinds/ftNana/ftnana.h>
#include <melee/ft/kinds/ftNana/ftnanaspecialhi.h>
#include <melee/ft/kinds/ftKirby/ftKb_Init.static.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/ft/kinds/ftKirby/ftkirbyattackdash.h>
#include <melee/ft/kinds/ftKirby/ftkirbycaptureyoshi.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialdonkey.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialgamewatch.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialhi.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialiceclimber.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialmars.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialmewtwo.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialpeach.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialpikachu.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialpurin.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialzelda.h>
#include <melee/ft/kinds/ftKirby/ftkirbyyoshiegg.h>
#include <melee/ft/kinds/ftPurin/ftpurin.h>
#include <melee/ft/kinds/ftPurin/ftpurinspecialhi.h>
#include <melee/ft/kinds/ftPurin/ftpurinspeciallw.h>
#include <melee/ft/kinds/ftPurin/ftpurinspecialn.h>
#include <melee/ft/kinds/ftPurin/ftpurinspecials.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeachattacks4.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloat.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloatattack.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloatfall.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/ft/kinds/ftPeach/ftpeachspeciallw.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialn.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecials.h>
#include <melee/ft/kinds/ftYoshi/ftYs_SpecialS.static.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/ft/kinds/ftYoshi/ftyoshiguard.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispecialhi.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispeciallw.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispecialn.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispecials.h>
#include <melee/ft/kinds/ftNess/ftness.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacklw4.h>
#include <melee/ft/kinds/ftNess/ftnessattacks4.h>
#include <melee/ft/kinds/ftNess/ftnessspecialhi.h>
#include <melee/ft/kinds/ftNess/ftnessspeciallw.h>
#include <melee/ft/kinds/ftNess/ftnessspecialn.h>
#include <melee/ft/kinds/ftNess/ftnessspecials.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatch.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack100.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack11.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattackair.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattacklw3.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattacks4.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchspecialhi.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchspecialn.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchspecials.h>
#include <melee/ft/kinds/ftFox/ftfox.h>
#include <melee/ft/kinds/ftFox/ftfoxappeals.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialhi.h>
#include <melee/ft/kinds/ftFox/ftfoxspeciallw.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialn.h>
#include <melee/ft/kinds/ftFox/ftfoxspecials.h>
#include <melee/ft/kinds/ftFalco/ftfalco.h>
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/ftlinkattackair.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialhi.h>
#include <melee/ft/kinds/ftLink/ftlinkspeciallw.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlinkspecials.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftCLink/ftclinkappeals.h>
#include <melee/ft/kinds/ftSamus/ftsamus.h>
#include <melee/ft/kinds/ftSamus/ftsamusspecialhi.h>
#include <melee/ft/kinds/ftSamus/ftsamusspeciallw0.h>
#include <melee/ft/kinds/ftSamus/ftsamusspeciallw1.h>
#include <melee/ft/kinds/ftSamus/ftsamusspecialn.h>
#include <melee/ft/kinds/ftSamus/ftsamusspecials.h>
#include <melee/ft/kinds/ftPikachu/ftpikachu.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialhi.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspeciallw.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialn.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecials.h>
#include <melee/ft/kinds/ftPichu/ftpichu.h>
#include <melee/ft/kinds/ftZelda/ftzelda.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecialhi.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspeciallw.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecialn.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecials.h>
#include <melee/ft/kinds/ftSeak/ftseak.h>
#include <melee/ft/kinds/ftSeak/ftseakspecialhi.h>
#include <melee/ft/kinds/ftSeak/ftseakspeciallw.h>
#include <melee/ft/kinds/ftSeak/ftseakspecialn.h>
#include <melee/ft/kinds/ftSeak/ftseakspecials.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwo.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecialhi.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospeciallw.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecialn.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecials.h>
#include <melee/ft/kinds/ftCaptain/ftcaptain.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecialhi.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspeciallw.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecialn.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecials.h>
#include <melee/ft/kinds/ftGanon/ftganon.h>
#include <melee/ft/kinds/ftMars/ftmars.h>
#include <melee/ft/kinds/ftMars/ftmarsspecialhi.h>
#include <melee/ft/kinds/ftMars/ftmarsspeciallw.h>
#include <melee/ft/kinds/ftMars/ftmarsspecialn.h>
#include <melee/ft/kinds/ftMars/ftmarsspecials.h>
#include <melee/ft/kinds/ftEmblem/ftemblem.h>
#include <melee/ft/kinds/ftDonkey/ftdonkey.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavyfall.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavyjump.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavylanding.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavyturn.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavywait0.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavywait1.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyheavywalk.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyms3450.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyspecialhi.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyspeciallw.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyspecialn.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyspecials.h>
#include <melee/ft/kinds/ftKoopa/ftkoopa.h>
#include <melee/ft/kinds/ftKoopa/ftkoopaspecialhi.h>
#include <melee/ft/kinds/ftKoopa/ftkoopaspeciallw.h>
#include <melee/ft/kinds/ftKoopa/ftkoopaspecialn.h>
#include <melee/ft/kinds/ftKoopa/ftkoopaspecials.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lbarchive.h>
#include <melee/pl/plstale.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/mobj.h>
#pragma warning(pop)

#include "match_xdk.h"

bool lb_8000B074(HSD_JObj* jobj);
void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);

extern void M360_AudioSfx(unsigned sfxId, unsigned volume, unsigned pan);

typedef union AnimFn {
    void (*fn)(void);
    void* ptr;
} AnimFn;

static void* AnimCallback(void (*fn)(void))
{
    AnimFn u;
    u.fn = fn;
    return u.ptr;
}

typedef struct M360MotionEntry {
    int msid;
    MotionState state;
} M360MotionEntry;

extern const M360MotionEntry M360_MotionTable[];
extern const unsigned M360_MotionTableCount;

u32 HSD_GObj_80390EB8(s32 i);
void HSD_GObjProc_8038FED4(HSD_GObjProc* proc);

enum {
    kMaxJoints = 128,
    kMaxDObjs = 256,
    kMaxAnims = 512,
    kMaxFighters = 4,
    kMaxHitboxes = 4,
    kLinkFighter = 2,
    kFighterStates = 0x80,
    kMaxCostumes = 6
};

typedef struct M360Hitbox {
    int active;
    int bone;
    float damage;
    float size;
    float angle;
    float kbGrowth;
    float kbBase;
    Vec3 offset;
    Vec3 pos;
} M360Hitbox;

typedef struct M360Fighter {
    Fighter fighter;
    HSD_GObj* gobj;
    HSD_JObj* joints[kMaxJoints];
    HSD_Joint* jointDescs[kMaxJoints];
    unsigned jointCount;
    HSD_DObj* dobjs[kMaxDObjs];
    unsigned dobjCount;
    FtPartsVisLookup* vis[5];
    FighterBone parts[kMaxJoints];
    int port;
    int modelIdx;
    M360Hitbox hitboxes[kMaxHitboxes];
    int hitTarget;
    float hitlag;
    unsigned flinchFrames;
    int traceMotion;
    int dead;
    u32 datAttrs[0x140]; /* fighter_dat_attrs_alloc_data storage */
    CmSubject cameraSubject;
} M360Fighter;


/* Per-character archives and entry points, following ftdata.c's per-kind
 * tables (OnLoad, OnDeath, motion-state table, special-move dispatch). */
typedef struct M360KindDesc {
    FighterKind kind;
    const char* name;
    const char* dat;
    const char* dataSymbol;
    const char* animDat;
    const char* costumeDat[kMaxCostumes];
    const char* costumeJoint[kMaxCostumes];
    const char* costumeMatAnim[kMaxCostumes];
    MotionState* states;
    unsigned stateCount;
    HSD_GObjEvent onLoad;
    HSD_GObjEvent onDeath;
    HSD_GObjEvent special[8];
} M360KindDesc;

typedef struct M360LoadedKind {
    ftData* data;
    unsigned char* animImage;
    unsigned animImageSize;
    FigaTree* trees[kMaxAnims];
    MotionState states[kFighterStates];
    HSD_Joint* costumeJoint[kMaxCostumes];
    HSD_MatAnimJoint* costumeMatAnim[kMaxCostumes];
    int loaded;
} M360LoadedKind;

static const M360KindDesc s_kinds[] = {
    { Ft_Kind_Mario, "MARIO", "PlMr.dat", "ftDataMario", "PlMrAJ.dat",
      { "PlMrNr.dat", "PlMrYe.dat", "PlMrBk.dat", "PlMrBu.dat", "PlMrGr.dat", NULL },
      { "PlyMario5K_Share_joint", "PlyMario5KYe_Share_joint", "PlyMario5KBk_Share_joint", "PlyMario5KBu_Share_joint", "PlyMario5KGr_Share_joint", NULL },
      { "PlyMario5K_Share_matanim_joint", "PlyMario5KYe_Share_matanim_joint", "PlyMario5KBk_Share_matanim_joint", "PlyMario5KBu_Share_matanim_joint", "PlyMario5KGr_Share_matanim_joint", NULL },
      ftMr_Init_MotionStateTable, sizeof(ftMr_Init_MotionStateTable) / sizeof(MotionState), ftMr_Init_OnLoad, ftMr_Init_OnDeath,
      { ftMr_SpecialN_Enter, ftMr_SpecialS_Enter, ftMr_SpecialHi_Enter, ftMr_SpecialLw_Enter, ftMr_SpecialAirN_Enter, ftMr_SpecialAirS_Enter, ftMr_SpecialAirHi_Enter, ftMr_SpecialAirLw_Enter } },
    { Ft_Kind_Luigi, "LUIGI", "PlLg.dat", "ftDataLuigi", "PlLgAJ.dat",
      { "PlLgNr.dat", "PlLgWh.dat", "PlLgAq.dat", "PlLgPi.dat", NULL, NULL },
      { "PlyLuigi5K_Share_joint", "PlyLuigi5KWh_Share_joint", "PlyLuigi5KAq_Share_joint", "PlyLuigi5KPi_Share_joint", NULL, NULL },
      { "PlyLuigi5K_Share_matanim_joint", "PlyLuigi5KWh_Share_matanim_joint", "PlyLuigi5KAq_Share_matanim_joint", "PlyLuigi5KPi_Share_matanim_joint", NULL, NULL },
      ftLg_Init_MotionStateTable, sizeof(ftLg_Init_MotionStateTable) / sizeof(MotionState), ftLg_Init_OnLoad, ftLg_Init_OnDeath,
      { ftLg_SpecialN_Enter, ftLg_SpecialS_Enter, ftLg_SpecialHi_Enter, ftLg_SpecialLw_Enter, ftLg_SpecialAirN_Enter, ftLg_SpecialAirS_Enter, ftLg_SpecialAirHi_Enter, ftLg_SpecialAirLw_Enter } },
    { Ft_Kind_DrMario, "DR. MARIO", "PlDr.dat", "ftDataDrmario", "PlDrAJ.dat",
      { "PlDrNr.dat", "PlDrRe.dat", "PlDrBu.dat", "PlDrGr.dat", "PlDrBk.dat", NULL },
      { "PlyDrmario5K_Share_joint", "PlyDrmario5KRe_Share_joint", "PlyDrmario5KBu_Share_joint", "PlyDrmario5KGr_Share_joint", "PlyDrmario5KBk_Share_joint", NULL },
      { "PlyDrmario5K_Share_matanim_joint", "PlyDrmario5KRe_Share_matanim_joint", "PlyDrmario5KBu_Share_matanim_joint", "PlyDrmario5KGr_Share_matanim_joint", "PlyDrmario5KBk_Share_matanim_joint", NULL },
      ftDr_Init_MotionStateTable, sizeof(ftDr_Init_MotionStateTable) / sizeof(MotionState), ftDr_Init_OnLoad, ftDr_Init_OnDeath,
      { ftMr_SpecialN_Enter, ftMr_SpecialS_Enter, ftMr_SpecialHi_Enter, ftMr_SpecialLw_Enter, ftMr_SpecialAirN_Enter, ftMr_SpecialAirS_Enter, ftMr_SpecialAirHi_Enter, ftMr_SpecialAirLw_Enter } },
    { Ft_Kind_Peach, "PEACH", "PlPe.dat", "ftDataPeach", "PlPeAJ.dat",
      { "PlPeNr.dat", "PlPeYe.dat", "PlPeWh.dat", "PlPeBu.dat", "PlPeGr.dat", NULL },
      { "PlyPeach5K_Share_joint", "PlyPeach5KYe_Share_joint", "PlyPeach5KWh_Share_joint", "PlyPeach5KBu_Share_joint", "PlyPeach5KGr_Share_joint", NULL },
      { "PlyPeach5K_Share_matanim_joint", "PlyPeach5KYe_Share_matanim_joint", "PlyPeach5KWh_Share_matanim_joint", "PlyPeach5KBu_Share_matanim_joint", "PlyPeach5KGr_Share_matanim_joint", NULL },
      ftPe_Init_MotionStateTable, sizeof(ftPe_Init_MotionStateTable) / sizeof(MotionState), ftPe_Init_OnLoad, ftPe_Init_OnDeath,
      { ftPe_SpecialN_Enter, ftPe_SpecialS_Enter, ftPe_SpecialHi_Enter, ftPe_SpecialLw_Enter, ftPe_SpecialAirN_Enter, ftPe_SpecialAirS_Enter, ftPe_SpecialAirHi_Enter, ftPe_SpecialAirLw_Enter } },
    { Ft_Kind_Yoshi, "YOSHI", "PlYs.dat", "ftDataYoshi", "PlYsAJ.dat",
      { "PlYsNr.dat", "PlYsRe.dat", "PlYsBu.dat", "PlYsYe.dat", "PlYsPi.dat", "PlYsAq.dat" },
      { "PlyYoshi5K_Share_joint", "PlyYoshi5KRe_Share_joint", "PlyYoshi5KBu_Share_joint", "PlyYoshi5KYe_Share_joint", "PlyYoshi5KPi_Share_joint", "PlyYoshi5KAq_Share_joint" },
      { "PlyYoshi5K_Share_matanim_joint", "PlyYoshi5KRe_Share_matanim_joint", "PlyYoshi5KBu_Share_matanim_joint", "PlyYoshi5KYe_Share_matanim_joint", "PlyYoshi5KPi_Share_matanim_joint", "PlyYoshi5KAq_Share_matanim_joint" },
      ftYs_Init_MotionStateTable, sizeof(ftYs_Init_MotionStateTable) / sizeof(MotionState), ftYs_Init_OnLoad, ftYs_Init_OnDeath,
      { ftYs_SpecialN_Enter, ftYs_SpecialS_Enter, ftYs_SpecialHi_Enter, ftYs_SpecialLw_Enter, ftYs_SpecialAirN_Enter, ftYs_SpecialAirS_Enter, ftYs_SpecialAirHi_Enter, ftYs_SpecialAirLw_Enter } },
    { Ft_Kind_Koopa, "BOWSER", "PlKp.dat", "ftDataKoopa", "PlKpAJ.dat",
      { "PlKpNr.dat", "PlKpRe.dat", "PlKpBu.dat", "PlKpBk.dat", NULL, NULL },
      { "PlyKoopa5K_Share_joint", "PlyKoopa5KRe_Share_joint", "PlyKoopa5KBu_Share_joint", "PlyKoopa5KBk_Share_joint", NULL, NULL },
      { "PlyKoopa5K_Share_matanim_joint", "PlyKoopa5KRe_Share_matanim_joint", "PlyKoopa5KBu_Share_matanim_joint", "PlyKoopa5KBk_Share_matanim_joint", NULL, NULL },
      ftKp_Init_MotionStateTable, sizeof(ftKp_Init_MotionStateTable) / sizeof(MotionState), ftKp_Init_OnLoad, ftKp_Init_OnDeath,
      { ftKp_SpecialN_Enter, ftKp_SpecialS_Enter, ftKp_SpecialHi_Enter, ftKp_SpecialLw_Enter, ftKp_SpecialAirN_Enter, ftKp_SpecialAirS_Enter, ftKp_SpecialAirHi_Enter, ftKp_SpecialAirLw_Enter } },
    { Ft_Kind_Donkey, "DONKEY KONG", "PlDk.dat", "ftDataDonkey", "PlDkAJ.dat",
      { "PlDkNr.dat", "PlDkBk.dat", "PlDkRe.dat", "PlDkBu.dat", "PlDkGr.dat", NULL },
      { "PlyDonkey5K_Share_joint", "PlyDonkey5KBk_Share_joint", "PlyDonkey5KRe_Share_joint", "PlyDonkey5KBu_Share_joint", "PlyDonkey5KGr_Share_joint", NULL },
      { "PlyDonkey5K_Share_matanim_joint", "PlyDonkey5KBk_Share_matanim_joint", "PlyDonkey5KRe_Share_matanim_joint", "PlyDonkey5KBu_Share_matanim_joint", "PlyDonkey5KGr_Share_matanim_joint", NULL },
      ftDk_Init_MotionStateTable, sizeof(ftDk_Init_MotionStateTable) / sizeof(MotionState), ftDk_Init_OnLoad, ftDk_Init_OnDeath,
      { ftDk_SpecialN_Enter, ftDk_SpecialS_Enter, ftDk_SpecialHi_Enter, ftDk_SpecialLw_Enter, ftDk_SpecialAirN_Enter, ftDk_SpecialAirS_Enter, ftDk_SpecialAirHi_Enter, NULL } },
    { Ft_Kind_Captain, "CAPTAIN FALCON", "PlCa.dat", "ftDataCaptain", "PlCaAJ.dat",
      { "PlCaNr.dat", "PlCaGy.dat", "PlCaRe.dat", "PlCaWh.dat", "PlCaGr.dat", "PlCaBu.dat" },
      { "PlyCaptain5K_Share_joint", "PlyCaptain5KGy_Share_joint", "PlyCaptain5KRe_Share_joint", "PlyCaptain5KWh_Share_joint", "PlyCaptain5KGr_Share_joint", "PlyCaptain5KBu_Share_joint" },
      { NULL, NULL, NULL, NULL, NULL, NULL },
      ftCa_Init_MotionStateTable, sizeof(ftCa_Init_MotionStateTable) / sizeof(MotionState), ftCa_Init_OnLoad, ftCa_Init_OnDeath,
      { ftCa_SpecialN_Enter, ftCa_SpecialS_Enter, ftCa_SpecialHi_Enter, ftCa_SpecialLw_Enter, ftCa_SpecialAirN_Enter, ftCa_SpecialAirS_Enter, ftCa_SpecialAirHi_Enter, ftCa_SpecialAirLw_Enter } },
    { Ft_Kind_Ganon, "GANONDORF", "PlGn.dat", "ftDataGanon", "PlGnAJ.dat",
      { "PlGnNr.dat", "PlGnRe.dat", "PlGnBu.dat", "PlGnGr.dat", "PlGnLa.dat", NULL },
      { "PlyGanon5K_Share_joint", "PlyGanon5KRe_Share_joint", "PlyGanon5KBu_Share_joint", "PlyGanon5KGr_Share_joint", "PlyGanon5KLa_Share_joint", NULL },
      { NULL, NULL, NULL, NULL, NULL, NULL },
      ftGn_Init_MotionStateTable, sizeof(ftGn_Init_MotionStateTable) / sizeof(MotionState), ftGn_Init_OnLoad, ftGn_Init_OnDeath,
      { ftCa_SpecialN_Enter, ftCa_SpecialS_Enter, ftCa_SpecialHi_Enter, ftCa_SpecialLw_Enter, ftCa_SpecialAirN_Enter, ftCa_SpecialAirS_Enter, ftCa_SpecialAirHi_Enter, ftCa_SpecialAirLw_Enter } },
    { Ft_Kind_Fox, "FOX", "PlFx.dat", "ftDataFox", "PlFxAJ.dat",
      { "PlFxNr.dat", "PlFxOr.dat", "PlFxLa.dat", "PlFxGr.dat", NULL, NULL },
      { "PlyFox5K_Share_joint", "PlyFox5KOr_Share_joint", "PlyFox5KLa_Share_joint", "PlyFox5KGr_Share_joint", NULL, NULL },
      { "PlyFox5K_Share_matanim_joint", "PlyFox5KOr_Share_matanim_joint", "PlyFox5KLa_Share_matanim_joint", "PlyFox5KGr_Share_matanim_joint", NULL, NULL },
      ftFx_Init_MotionStateTable, sizeof(ftFx_Init_MotionStateTable) / sizeof(MotionState), ftFx_Init_OnLoad, ftFx_Init_OnDeath,
      { ftFx_SpecialN_Enter, ftFx_SpecialSStart_Enter, ftFx_SpecialHi_Enter, ftFx_SpecialLw_Enter, ftFx_SpecialAirN_Enter, ftFx_SpecialAirSStart_Enter, ftFx_SpecialAirHiStart_Enter, ftFx_SpecialAirLw_Enter } },
    { Ft_Kind_Falco, "FALCO", "PlFc.dat", "ftDataFalco", "PlFcAJ.dat",
      { "PlFcNr.dat", "PlFcRe.dat", "PlFcBu.dat", "PlFcGr.dat", NULL, NULL },
      { "PlyFalco5K_Share_joint", "PlyFalco5KRe_Share_joint", "PlyFalco5KBu_Share_joint", "PlyFalco5KGr_Share_joint", NULL, NULL },
      { "PlyFalco5K_Share_matanim_joint", "PlyFalco5KRe_Share_matanim_joint", "PlyFalco5KBu_Share_matanim_joint", "PlyFalco5KGr_Share_matanim_joint", NULL, NULL },
      ftFc_Init_MotionStateTable, sizeof(ftFc_Init_MotionStateTable) / sizeof(MotionState), ftFc_Init_OnLoad, ftFc_Init_OnDeath,
      { ftFx_SpecialN_Enter, ftFx_SpecialSStart_Enter, ftFx_SpecialHi_Enter, ftFx_SpecialLw_Enter, ftFx_SpecialAirN_Enter, ftFx_SpecialAirSStart_Enter, ftFx_SpecialAirHiStart_Enter, ftFx_SpecialAirLw_Enter } },
    { Ft_Kind_Link, "LINK", "PlLk.dat", "ftDataLink", "PlLkAJ.dat",
      { "PlLkNr.dat", "PlLkRe.dat", "PlLkBu.dat", "PlLkBk.dat", "PlLkWh.dat", NULL },
      { "PlyLink5K_Share_joint", "PlyLink5KRe_Share_joint", "PlyLink5KBu_Share_joint", "PlyLink5KBk_Share_joint", "PlyLink5KWh_Share_joint", NULL },
      { "PlyLink5K_Share_matanim_joint", "PlyLink5KRe_Share_matanim_joint", "PlyLink5KBu_Share_matanim_joint", "PlyLink5KBk_Share_matanim_joint", "PlyLink5KWh_Share_matanim_joint", NULL },
      ftLk_Init_MotionStateTable, sizeof(ftLk_Init_MotionStateTable) / sizeof(MotionState), ftLk_Init_OnLoad, ftLk_Init_OnDeath,
      { ftLk_SpecialN_Enter, ftLk_SpecialS_Enter, ftLk_SpecialHi_Enter, ftLk_SpecialLw_Enter, ftLk_SpecialAirN_Enter, ftLk_SpecialAirS_Enter, ftLk_SpecialAirHi_Enter, ftLk_SpecialAirLw_Enter } },
    { Ft_Kind_CLink, "YOUNG LINK", "PlCl.dat", "ftDataClink", "PlClAJ.dat",
      { "PlClNr.dat", "PlClRe.dat", "PlClBu.dat", "PlClWh.dat", "PlClBk.dat", NULL },
      { "PlyClink5K_Share_joint", "PlyClink5KRe_Share_joint", "PlyClink5KBu_Share_joint", "PlyClink5KWh_Share_joint", "PlyClink5KBk_Share_joint", NULL },
      { "PlyClink5K_Share_matanim_joint", "PlyClink5KRe_Share_matanim_joint", "PlyClink5KBu_Share_matanim_joint", "PlyClink5KWh_Share_matanim_joint", "PlyClink5KBk_Share_matanim_joint", NULL },
      ftCl_Init_MotionStateTable, sizeof(ftCl_Init_MotionStateTable) / sizeof(MotionState), ftCl_Init_OnLoad, ftCl_Init_OnDeath,
      { ftLk_SpecialN_Enter, ftLk_SpecialS_Enter, ftLk_SpecialHi_Enter, ftLk_SpecialLw_Enter, ftLk_SpecialAirN_Enter, ftLk_SpecialAirS_Enter, ftLk_SpecialAirHi_Enter, ftLk_SpecialAirLw_Enter } },
    { Ft_Kind_Zelda, "ZELDA", "PlZd.dat", "ftDataZelda", "PlZdAJ.dat",
      { "PlZdNr.dat", "PlZdRe.dat", "PlZdBu.dat", "PlZdGr.dat", "PlZdWh.dat", NULL },
      { "PlyZelda5K_Share_joint", "PlyZelda5KRe_Share_joint", "PlyZelda5KBu_Share_joint", "PlyZelda5KGr_Share_joint", "PlyZelda5KWh_Share_joint", NULL },
      { "PlyZelda5K_Share_matanim_joint", "PlyZelda5KRe_Share_matanim_joint", "PlyZelda5KBu_Share_matanim_joint", "PlyZelda5KGr_Share_matanim_joint", "PlyZelda5KWh_Share_matanim_joint", NULL },
      ftZd_Init_MotionStateTable, sizeof(ftZd_Init_MotionStateTable) / sizeof(MotionState), ftZd_Init_OnLoad, ftZd_Init_OnDeath,
      { ftZd_SpecialN_Enter, ftZd_SpecialS_Enter, ftZd_SpecialHi_Enter, ftZd_SpecialLw_Enter, ftZd_SpecialAirN_Enter, ftZd_SpecialAirS_Enter, ftZd_SpecialAirHi_Enter, ftZd_SpecialAirLw_Enter } },
    { Ft_Kind_Seak, "SHEIK", "PlSk.dat", "ftDataSeak", "PlSkAJ.dat",
      { "PlSkNr.dat", "PlSkRe.dat", "PlSkBu.dat", "PlSkGr.dat", "PlSkWh.dat", NULL },
      { "PlySeak5K_Share_joint", "PlySeak5KRe_Share_joint", "PlySeak5KBu_Share_joint", "PlySeak5KGr_Share_joint", "PlySeak5KWh_Share_joint", NULL },
      { "PlySeak5K_Share_matanim_joint", "PlySeak5KRe_Share_matanim_joint", "PlySeak5KBu_Share_matanim_joint", "PlySeak5KGr_Share_matanim_joint", "PlySeak5KWh_Share_matanim_joint", NULL },
      ftSk_Init_MotionStateTable, sizeof(ftSk_Init_MotionStateTable) / sizeof(MotionState), ftSk_Init_OnLoad, ftSk_Init_OnDeath,
      { ftSk_SpecialN_Enter, ftSk_SpecialS_Enter, ftSk_SpecialHi_Enter, ftSk_SpecialLw_Enter, ftSk_SpecialAirN_Enter, ftSk_SpecialAirS_Enter, ftSk_SpecialAirHi_Enter, ftSk_SpecialAirLw_Enter } },
    { Ft_Kind_Samus, "SAMUS", "PlSs.dat", "ftDataSamus", "PlSsAJ.dat",
      { "PlSsNr.dat", "PlSsPi.dat", "PlSsBk.dat", "PlSsGr.dat", "PlSsLa.dat", NULL },
      { "PlySamus5K_Share_joint", "PlySamus5KPi_Share_joint", "PlySamus5KBk_Share_joint", "PlySamus5KGr_Share_joint", "PlySamus5KLa_Share_joint", NULL },
      { NULL, NULL, NULL, NULL, NULL, NULL },
      ftSs_Init_MotionStateTable, sizeof(ftSs_Init_MotionStateTable) / sizeof(MotionState), ftSs_Init_OnLoad, ftSs_Init_OnDeath,
      { ftSs_SpecialN_Enter, ftSs_SpecialS_Enter, ftSs_SpecialHi_Enter, ftSs_SpecialLw_Enter, ftSs_SpecialAirN_Enter, ftSs_SpecialAirS_Enter, ftSs_SpecialAirHi_Enter, ftSs_SpecialAirLw_Enter } },
    { Ft_Kind_Pikachu, "PIKACHU", "PlPk.dat", "ftDataPikachu", "PlPkAJ.dat",
      { "PlPkNr.dat", "PlPkRe.dat", "PlPkBu.dat", "PlPkGr.dat", NULL, NULL },
      { "PlyPikachu5K_Share_joint", "PlyPikachu5KRe_Share_joint", "PlyPikachu5KBu_Share_joint", "PlyPikachu5KGr_Share_joint", NULL, NULL },
      { "PlyPikachu5K_Share_matanim_joint", "PlyPikachu5KRe_Share_matanim_joint", "PlyPikachu5KBu_Share_matanim_joint", "PlyPikachu5KGr_Share_matanim_joint", NULL, NULL },
      ftPk_Init_MotionStateTable, sizeof(ftPk_Init_MotionStateTable) / sizeof(MotionState), ftPk_Init_OnLoad, ftPk_Init_OnDeath,
      { ftPk_SpecialN_Enter, ftPk_SpecialS_Enter, ftPk_SpecialHi_Enter, ftPk_SpecialLw_Enter, ftPk_SpecialAirN_Enter, ftPk_SpecialAirS_Enter, ftPk_SpecialAirHi_Enter, ftPk_SpecialAirLw_Enter } },
    { Ft_Kind_Pichu, "PICHU", "PlPc.dat", "ftDataPichu", "PlPcAJ.dat",
      { "PlPcNr.dat", "PlPcRe.dat", "PlPcBu.dat", "PlPcGr.dat", NULL, NULL },
      { "PlyPichu5K_Share_joint", "PlyPichu5KRe_Share_joint", "PlyPichu5KBu_Share_joint", "PlyPichu5KGr_Share_joint", NULL, NULL },
      { "PlyPichu5K_Share_matanim_joint", "PlyPichu5KRe_Share_matanim_joint", "PlyPichu5KBu_Share_matanim_joint", "PlyPichu5KGr_Share_matanim_joint", NULL, NULL },
      ftPc_Init_MotionStateTable, sizeof(ftPc_Init_MotionStateTable) / sizeof(MotionState), ftPc_Init_OnLoad, ftPc_Init_OnDeath,
      { ftPk_SpecialN_Enter, ftPk_SpecialS_Enter, ftPk_SpecialHi_Enter, ftPk_SpecialLw_Enter, ftPk_SpecialAirN_Enter, ftPk_SpecialAirS_Enter, ftPk_SpecialAirHi_Enter, ftPk_SpecialAirLw_Enter } },
    { Ft_Kind_Purin, "JIGGLYPUFF", "PlPr.dat", "ftDataPurin", "PlPrAJ.dat",
      { "PlPrNr.dat", NULL, NULL, NULL, NULL, NULL },
      { "PlyPurin5K_Share_joint", NULL, NULL, NULL, NULL, NULL },
      { "PlyPurin5K_Share_matanim_joint", NULL, NULL, NULL, NULL, NULL },
      ftPr_Init_MotionStateTable, sizeof(ftPr_Init_MotionStateTable) / sizeof(MotionState), ftPr_Init_OnLoad, ftPr_Init_OnDeath,
      { ftPr_SpecialN_Enter, ftPr_SpecialS_Enter, ftPr_SpecialHi_Enter, ftPr_SpecialLw_Enter, ftPr_SpecialAirN_Enter, ftPr_SpecialAirS_Enter, ftPr_SpecialAirHi_Enter, ftPr_SpecialAirLw_Enter } },
    { Ft_Kind_Ness, "NESS", "PlNs.dat", "ftDataNess", "PlNsAJ.dat",
      { "PlNsNr.dat", "PlNsYe.dat", "PlNsBu.dat", "PlNsGr.dat", NULL, NULL },
      { "PlyNess5K_Share_joint", "PlyNess5KYe_Share_joint", "PlyNess5KBu_Share_joint", "PlyNess5KGr_Share_joint", NULL, NULL },
      { "PlyNess5K_Share_matanim_joint", "PlyNess5KYe_Share_matanim_joint", "PlyNess5KBu_Share_matanim_joint", "PlyNess5KGr_Share_matanim_joint", NULL, NULL },
      ftNs_Init_MotionStateTable, sizeof(ftNs_Init_MotionStateTable) / sizeof(MotionState), ftNs_Init_OnLoad, ftNs_Init_OnDeath,
      { ftNs_SpecialNStart_Enter, ftNs_SpecialS_Enter, ftNs_SpecialHiStart_Enter, ftNs_SpecialLwStart_Enter, ftNs_SpecialAirNStart_Enter, ftNs_SpecialAirS_Enter, ftNs_SpecialAirHiStart_Enter, ftNs_SpecialAirLwStart_Enter } },
    { Ft_Kind_Mars, "MARTH", "PlMs.dat", "ftDataMars", "PlMsAJ.dat",
      { "PlMsNr.dat", "PlMsRe.dat", "PlMsGr.dat", "PlMsBk.dat", "PlMsWh.dat", NULL },
      { "PlyMars5K_Share_joint", "PlyMars5KRe_Share_joint", "PlyMars5KGr_Share_joint", "PlyMars5KBk_Share_joint", "PlyMars5KWh_Share_joint", NULL },
      { "PlyMars5K_Share_matanim_joint", "PlyMars5KRe_Share_matanim_joint", "PlyMars5KGr_Share_matanim_joint", "PlyMars5KBk_Share_matanim_joint", "PlyMars5KWh_Share_matanim_joint", NULL },
      ftMs_Init_MotionStateTable, sizeof(ftMs_Init_MotionStateTable) / sizeof(MotionState), ftMs_Init_OnLoad, ftMs_Init_OnDeath,
      { ftMs_SpecialN_Enter, ftMs_SpecialS_Enter, ftMs_SpecialHi_Enter, ftMs_SpecialLw_Enter, ftMs_SpecialAirN_Enter, ftMs_SpecialAirS_Enter, ftMs_SpecialAirHi_Enter, ftMs_SpecialAirLw_Enter } },
    { Ft_Kind_Emblem, "ROY", "PlFe.dat", "ftDataEmblem", "PlFeAJ.dat",
      { "PlFeNr.dat", "PlFeRe.dat", "PlFeBu.dat", "PlFeGr.dat", "PlFeYe.dat", NULL },
      { "PlyEmblem5K_Share_joint", "PlyEmblem5KRe_Share_joint", "PlyEmblem5KBu_Share_joint", "PlyEmblem5KGr_Share_joint", "PlyEmblem5KYe_Share_joint", NULL },
      { "PlyEmblem5K_Share_matanim_joint", "PlyEmblem5KRe_Share_matanim_joint", "PlyEmblem5KBu_Share_matanim_joint", "PlyEmblem5KGr_Share_matanim_joint", "PlyEmblem5KYe_Share_matanim_joint", NULL },
      ftFe_Init_MotionStateTable, sizeof(ftFe_Init_MotionStateTable) / sizeof(MotionState), ftFe_Init_OnLoad, ftFe_Init_OnDeath,
      { ftMs_SpecialN_Enter, ftMs_SpecialS_Enter, ftMs_SpecialHi_Enter, ftMs_SpecialLw_Enter, ftMs_SpecialAirN_Enter, ftMs_SpecialAirS_Enter, ftMs_SpecialAirHi_Enter, ftMs_SpecialAirLw_Enter } },
    { Ft_Kind_Mewtwo, "MEWTWO", "PlMt.dat", "ftDataMewtwo", "PlMtAJ.dat",
      { "PlMtNr.dat", "PlMtRe.dat", "PlMtBu.dat", "PlMtGr.dat", NULL, NULL },
      { "PlyMewtwo5K_Share_joint", "PlyMewtwo5KRe_Share_joint", "PlyMewtwo5KBu_Share_joint", "PlyMewtwo5KGr_Share_joint", NULL, NULL },
      { "PlyMewtwo5K_Share_matanim_joint", "PlyMewtwo5KRe_Share_matanim_joint", "PlyMewtwo5KBu_Share_matanim_joint", "PlyMewtwo5KGr_Share_matanim_joint", NULL, NULL },
      ftMt_Init_MotionStateTable, sizeof(ftMt_Init_MotionStateTable) / sizeof(MotionState), ftMt_Init_OnLoad, ftMt_Init_OnDeath,
      { ftMt_SpecialN_Enter, ftMt_SpecialS_Enter, ftMt_SpecialHiStart_Enter, ftMt_SpecialLw_Enter, ftMt_SpecialAirN_Enter, ftMt_SpecialAirS_Enter, ftMt_SpecialAirHiStart_Enter, ftMt_SpecialAirLw_Enter } },
    { Ft_Kind_GameWatch, "MR. GAME & WATCH", "PlGw.dat", "ftDataGamewatch", "PlGwAJ.dat",
      { "PlGwNr.dat", NULL, NULL, NULL, NULL, NULL },
      { "PlyGamewatch5K_Share_joint", NULL, NULL, NULL, NULL, NULL },
      { NULL, NULL, NULL, NULL, NULL, NULL },
      ftGw_Init_MotionStateTable, sizeof(ftGw_Init_MotionStateTable) / sizeof(MotionState), ftGw_Init_OnLoad, ftGw_Init_OnDeath,
      { ftGw_SpecialN_Enter, ftGw_SpecialS_Enter, ftGw_SpecialHi_Enter, ftGw_SpecialLw_Enter, ftGw_SpecialAirN_Enter, ftGw_SpecialAirS_Enter, ftGw_SpecialAirHi_Enter, ftGw_SpecialAirLw_Enter } },
    { Ft_Kind_Kirby, "KIRBY", "PlKb.dat", "ftDataKirby", "PlKbAJ.dat",
      { "PlKbNr.dat", "PlKbYe.dat", "PlKbBu.dat", "PlKbRe.dat", "PlKbGr.dat", "PlKbWh.dat" },
      { "PlyKirby5K_Share_joint", "PlyKirby5KYe_Share_joint", "PlyKirby5KBu_Share_joint", "PlyKirby5KRe_Share_joint", "PlyKirby5KGr_Share_joint", "PlyKirby5KWh_Share_joint" },
      { "PlyKirby5K_Share_matanim_joint", "PlyKirby5KYe_Share_matanim_joint", "PlyKirby5KBu_Share_matanim_joint", "PlyKirby5KRe_Share_matanim_joint", "PlyKirby5KGr_Share_matanim_joint", "PlyKirby5KWh_Share_matanim_joint" },
      ftKb_Init_MotionStateTable, sizeof(ftKb_Init_MotionStateTable) / sizeof(MotionState), ftKb_Init_OnLoad, ftKb_Init_OnDeath,
      { ftKb_SpecialN_Enter, ftKb_SpecialS_Enter, ftKb_SpecialHi_Enter, ftKb_SpecialLw_Enter, ftKb_SpecialAirN_Enter, ftKb_SpecialAirS_Enter, ftKb_SpecialAirHi_Enter, ftKb_SpecialAirLw_Enter } },
    { Ft_Kind_Popo, "POPO", "PlPp.dat", "ftDataPopo", "PlPpAJ.dat",
      { "PlPpNr.dat", "PlPpGr.dat", "PlPpOr.dat", "PlPpRe.dat", NULL, NULL },
      { "PlyPopo5K_Share_joint", "PlyPopo5KGr_Share_joint", "PlyPopo5KOr_Share_joint", "PlyPopo5KRe_Share_joint", NULL, NULL },
      { "PlyPopo5K_Share_matanim_joint", "PlyPopo5KGr_Share_matanim_joint", "PlyPopo5KOr_Share_matanim_joint", "PlyPopo5KRe_Share_matanim_joint", NULL, NULL },
      ftPp_Init_MotionStateTable, sizeof(ftPp_Init_MotionStateTable) / sizeof(MotionState), ftPp_Init_OnLoad, ftPp_Init_OnDeath,
      { ftPp_SpecialN_Enter, ftPp_SpecialS_Enter, ftPp_SpecialHi_Enter, ftPp_SpecialLw_Enter, ftPp_SpecialAirN_Enter, ftPp_SpecialAirS_Enter, ftPp_SpecialAirHi_Enter, ftPp_SpecialAirLw_Enter } },
};

enum { kKindCount = sizeof(s_kinds) / sizeof(s_kinds[0]) };

static M360LoadedKind s_loaded[kKindCount];
ftData* gFtDataList[Ft_Kind_Max];
static unsigned s_selectKind[kMaxFighters];
static unsigned s_selectCostume[kMaxFighters] = { 0, 3, 1, 2 };
static M360Fighter s_fighters[kMaxFighters];
static StaleMoveTable s_staleTables[6];
static unsigned s_hitCount;
static unsigned s_cpuLevel = 3;
static MotionState s_commonStates[ftCo_MS_Count];
static Vec3 s_playerPos[6];
static Vec3 s_rebirthOffset[6];
static float s_playerFacing[6];
static int s_playerCpu[6];
static unsigned s_unported[64];
static const char* s_unportedName[64];
static unsigned s_unportedCount;

static void BuildMotionTables(void);
static void OnDeath(HSD_GObj* gobj);
static MotionState s_unknownState;

static void Unported(const char* name)
{
    unsigned i;
    for (i = 0; i < s_unportedCount; ++i)
        if (s_unportedName[i] == name)
            return;
    if (s_unportedCount < 64) {
        s_unportedName[s_unportedCount] = name;
        s_unported[s_unportedCount++] = 1;
        M360_MatchTrace(name, 0);
    }
}

static M360Fighter* Owner(HSD_GObj* gobj)
{
    return (M360Fighter*) gobj->user_data;
}

static void* LoadArchiveSymbol(const char* file, const char* symbol)
{
    unsigned size = 0;
    unsigned char* image = M360_ReadDiscFile(file, &size);
    void* archive = image ? M360_ArchiveOpen(image, size) : NULL;
    M360_MatchTrace(file, size);
    return archive ? M360_ArchiveFind(archive, symbol) : NULL;
}

HSD_Archive* M360_ArchiveLoadSymbolsV(const char* filename, void* symbols, va_list args)
{
    unsigned size = 0;
    unsigned char* image = M360_ReadDiscFile(filename, &size);
    void* archive = image ? M360_ArchiveOpen(image, size) : NULL;
    void** out = symbols;
    M360_MatchTrace(filename, size);
    while (out) {
        const char* name = va_arg(args, const char*);
        *out = archive ? M360_ArchiveFind(archive, name) : NULL;
        out = va_arg(args, void**);
    }
    return archive;
}

StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 slot)
{
    return &s_staleTables[slot < 6 && slot >= 0 ? slot : 0];
}

static M360LoadedKind* LoadKind(unsigned index)
{
    const M360KindDesc* desc;
    M360LoadedKind* k;
    unsigned i;
    if (index >= kKindCount)
        return NULL;
    desc = &s_kinds[index];
    k = &s_loaded[index];
    if (k->loaded)
        return k->loaded > 0 ? k : NULL;
    k->loaded = -1;
    k->data = LoadArchiveSymbol(desc->dat, desc->dataSymbol);
    k->animImage = M360_ReadDiscFile(desc->animDat, &k->animImageSize);
    M360_MatchTrace("fighter.aj.bytes", k->animImageSize);
    if (!k->data || !k->animImage)
        return NULL;
    M360_MatchTrace("fighter.attr.gravity_x1000",
               (unsigned) (k->data->x0->gravity * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.walk_max_x1000",
               (unsigned) (k->data->x0->walk_max_vel * 1000.0f + 0.5f));
    M360_MatchTrace("fighter.attr.max_jumps", (unsigned) k->data->x0->max_jumps);
    M360_MatchTrace("fighter.attr.weight_x1000", (unsigned) (k->data->x0->weight * 1000.0f));
    for (i = 0; i < kFighterStates; ++i)
        k->states[i] = s_unknownState;
    for (i = 0; i < desc->stateCount && i < kFighterStates; ++i)
        k->states[i] = desc->states[i];
    ftData_SpecialN[desc->kind] = desc->special[0];
    ftData_SpecialS[desc->kind] = desc->special[1];
    ftData_SpecialHi[desc->kind] = desc->special[2];
    ftData_SpecialLw[desc->kind] = desc->special[3];
    ftData_SpecialAirN[desc->kind] = desc->special[4];
    ftData_SpecialAirS[desc->kind] = desc->special[5];
    ftData_SpecialAirHi[desc->kind] = desc->special[6];
    ftData_SpecialAirLw[desc->kind] = desc->special[7];
    gFtDataList[desc->kind] = k->data;
    k->loaded = 1;
    return k;
}

/* Costume model (PlXxNr/Ye/...); falls back to the default costume. */
static int LoadCostume(M360LoadedKind* k, const M360KindDesc* desc, unsigned costume)
{
    if (costume >= kMaxCostumes || !desc->costumeDat[costume])
        costume = 0;
    if (!k->costumeJoint[costume] && desc->costumeDat[costume]) {
        unsigned size = 0;
        unsigned char* image = M360_ReadDiscFile(desc->costumeDat[costume], &size);
        void* archive = image ? M360_ArchiveOpen(image, size) : NULL;
        M360_MatchTrace(desc->costumeDat[costume], size);
        if (archive) {
            k->costumeJoint[costume] = M360_ArchiveFind(archive, desc->costumeJoint[costume]);
            if (desc->costumeMatAnim[costume])
                k->costumeMatAnim[costume] = M360_ArchiveFind(archive, desc->costumeMatAnim[costume]);
        }
    }
    if (!k->costumeJoint[costume] && costume)
        return LoadCostume(k, desc, 0);
    return k->costumeJoint[costume] ? (int) costume : -1;
}

unsigned M360_FighterKindCount(void)
{
    return kKindCount;
}

const char* M360_FighterKindName(unsigned index)
{
    return index < kKindCount ? s_kinds[index].name : "";
}

void M360_FighterSelect(int slot, unsigned kindIndex, unsigned costume)
{
    if (slot < 0 || slot >= kMaxFighters)
        return;
    s_selectKind[slot] = kindIndex < kKindCount ? kindIndex : 0;
    s_selectCostume[slot] = costume;
}

static const M360KindDesc* KindDesc(FighterKind kind, M360LoadedKind** loaded)
{
    unsigned i;
    for (i = 0; i < kKindCount; ++i)
        if (s_kinds[i].kind == kind) {
            if (loaded)
                *loaded = &s_loaded[i];
            return &s_kinds[i];
        }
    if (loaded)
        *loaded = &s_loaded[0];
    return &s_kinds[0];
}

int M360_FighterLoad(void)
{
    Fighter_LoadCommonData();
    if (!p_ftCommonData)
        return 0;
    /* gmvs.c match setup: load ItCo and initialize the item allocators before
     * fighters register their articles. */
    Item_80266FA8();
    Item_80266FCC();
    M360_MatchTrace("item.common.loaded", 1);
    BuildMotionTables();
    M360_MatchTrace("fighter.kb.xF4_x1000", (unsigned) (p_ftCommonData->xF4 * 1000.0f));
    M360_MatchTrace("fighter.kb.xF8_x1000", (unsigned) (p_ftCommonData->xF8 * 1000.0f));
    M360_MatchTrace("fighter.kb.x110_x1000", (unsigned) (p_ftCommonData->x110 * 1000.0f));
    M360_MatchTrace("fighter.kb.x114_x1000", (unsigned) (p_ftCommonData->x114 * 1000.0f));
    M360_MatchTrace("fighter.kb.x118_x1000", (unsigned) (p_ftCommonData->x118 * 1000.0f));
    M360_MatchTrace("fighter.kb.x11C_x1000", (unsigned) (p_ftCommonData->x11C * 1000.0f));
    M360_MatchTrace("fighter.kb.x120_x1000", (unsigned) (p_ftCommonData->x120 * 1000.0f));
    M360_MatchTrace("fighter.kb.x154_x1000", (unsigned) (p_ftCommonData->x154 * 1000.0f));
    return LoadKind(0) != NULL && LoadCostume(&s_loaded[0], &s_kinds[0], 0) >= 0;
}

/* gmvs.c match setup for effects: the effect manager GObjs, common effect
 * banks (EfCoData, index 0x1F), then each fighter's bank at spawn. */
void M360_FighterEffectsInit(void)
{
    efLib_Init();
    efAsync_LoadSync(0);
    efAsync_LoadSync(0x1F);
    M360_MatchTrace("effect.init", 1);
}

void M360_FighterResetMatch(void)
{
    static int s_itemsReady;
    if (s_itemsReady)
        Item_80266FCC();
    s_itemsReady = 1;
    s_hitCount = 0;
    memset(s_fighters, 0, sizeof(s_fighters));
}

static FigaTree* LoadTree(M360LoadedKind* k, int anim)
{
    struct Fighter_WaitAnimData* entry;
    unsigned char* copy;
    void* archive;
    if (!k || !k->data || anim < 0 || anim >= kMaxAnims)
        return NULL;
    if (k->trees[anim])
        return k->trees[anim];
    entry = &k->data->xC[anim];
    if (!entry->x8 || (unsigned) entry->x4 + (unsigned) entry->x8 > k->animImageSize)
        return NULL;
    /* Animation archives stay resident for the session; keep them out of
     * the fixed HSD heap so several rosters can load. */
    copy = malloc(entry->x8);
    if (!copy)
        return NULL;
    memcpy(copy, k->animImage + entry->x4, entry->x8);
    archive = M360_ArchiveOpen(copy, entry->x8);
    if (!archive)
        return NULL;
    k->trees[anim] = M360_ArchiveFind(archive, M360_ArchiveFirstSymbol(archive));
    return k->trees[anim];
}

static void CollectJoints(M360Fighter* f, HSD_JObj* jobj, HSD_Joint* desc)
{
    while (jobj && desc && f->jointCount < kMaxJoints) {
        HSD_DObj* dobj;
        f->joints[f->jointCount] = jobj;
        f->jointDescs[f->jointCount] = desc;
        ++f->jointCount;
        if (union_type_dobj(jobj))
            for (dobj = jobj->u.dobj; dobj && f->dobjCount < kMaxDObjs; dobj = dobj->next)
                f->dobjs[f->dobjCount++] = dobj;
        if (!(jobj->flags & JOBJ_INSTANCE))
            CollectJoints(f, jobj->child, desc->child);
        jobj = jobj->next;
        desc = desc->next;
    }
}

static void SetVisGroup(M360Fighter* f, int idx, int showIndex)
{
    FtPartsVisLookup* lookup = f->vis[idx];
    int j, k;
    if (!lookup)
        return;
    for (j = 0; j < lookup[0].x0; ++j) {
        TempS* group = &lookup[0].x4[j];
        for (k = 0; k < group->x0; ++k) {
            const unsigned d = group->x4[k];
            if (d >= f->dobjCount)
                continue;
            if (j == showIndex)
                HSD_DObjClearFlags(f->dobjs[d], DOBJ_HIDDEN);
            else
                HSD_DObjSetFlags(f->dobjs[d], DOBJ_HIDDEN);
        }
    }
}

static void FighterRender(HSD_GObj* gobj, int pass)
{
    M360Fighter* f = Owner(gobj);
    if (f->dead)
        return;
    SetVisGroup(f, 1, -1);
    SetVisGroup(f, 4, -1);
    SetVisGroup(f, 0, f->modelIdx);
    HSD_JObjDispAll(gobj->hsd_obj, NULL, HSD_GObj_80390EB8(pass), 0);
    /* ftdrawcommon.c: accessories such as the rebirth platform. */
    if (f->fighter.x20A0_accessory)
        HSD_JObjDispAll(f->fighter.x20A0_accessory, NULL, HSD_GObj_80390EB8(pass), 0);
}

static void ResetJoints(M360Fighter* f)
{
    unsigned i;
    const unsigned special = f->fighter.ft_data->x8->x10;
    HSD_JObjRemoveAnimAllByFlags(f->joints[0], 1);
    for (i = 1; i < f->jointCount; ++i) {
        HSD_JObj* j = f->joints[i];
        HSD_Joint* d = f->jointDescs[i];
        j->rotate.x = d->rotation.x;
        j->rotate.y = d->rotation.y;
        j->rotate.z = d->rotation.z;
        if (i == special) {
            const float s = 1.0f / f->fighter.co_attrs.model_scaling;
            j->scale.x = j->scale.y = j->scale.z = s;
        } else {
            j->scale.x = d->scale.x;
            j->scale.y = d->scale.y;
            j->scale.z = d->scale.z;
        }
        j->translate.x = d->position.x;
        j->translate.y = d->position.y;
        j->translate.z = d->position.z;
        HSD_JObjClearFlags(j, JOBJ_USE_QUATERNION);
        if (!(j->flags & JOBJ_MTX_INDEP_SRT))
            HSD_JObjSetMtxDirty(j);
    }
}

static void ApplyTree(M360Fighter* f, FigaTree* tree)
{
    s8* nodes = tree->nodes;
    FigaTrack* tracks = tree->tracks;
    unsigned i;
    for (i = 0; i < f->jointCount && *nodes != -1; ++i) {
        lbAnim_8001E6D8(f->joints[i], tree, tracks, *nodes);
        tracks += *nodes;
        ++nodes;
    }
}

static float ModelScale(Fighter* fp)
{
    return fp->x34_scale.y * fp->co_attrs.model_scaling;
}

static void AnimAdvance(M360Fighter* f)
{
    Fighter* fp = &f->fighter;
    HSD_JObj* transN = f->jointCount > 1 ? f->joints[1] : NULL;
    unsigned i;
    if (fp->anim_id == -1)
        return;
    HSD_AObjInitEndCallBack();
    for (i = 0; i < f->jointCount; ++i) {
        HSD_JObj* j = f->joints[i];
        if (fp->x594_b0 && j == transN) {
            Vec3 zero = { 0.0f, 0.0f, 0.0f };
            const float scale = ModelScale(fp);
            HSD_JObjAnim(j);
            fp->x698 = fp->x68C_transNPos;
            HSD_JObjGetTranslation(j, &fp->x68C_transNPos);
            fp->x68C_transNPos.x *= scale;
            fp->x68C_transNPos.y *= scale;
            fp->x68C_transNPos.z *= scale;
            fp->x6B0 = fp->x6A4_transNOffset;
            fp->x6A4_transNOffset.x = fp->x68C_transNPos.x - fp->x698.x;
            fp->x6A4_transNOffset.y = fp->x68C_transNPos.y - fp->x698.y;
            fp->x6A4_transNOffset.z = fp->x68C_transNPos.z - fp->x698.z;
            HSD_JObjSetTranslate(j, &zero);
        } else {
            HSD_JObjAnim(j);
        }
    }
    HSD_AObjInvokeCallBacks();
    for (i = 0; i < f->jointCount; ++i)
        if (f->joints[i]->aobj) {
            fp->cur_anim_frame = f->joints[i]->aobj->curr_frame;
            break;
        }
}

void ftAnim_8006E9B4(Fighter_GObj* gobj)
{
    AnimAdvance(Owner(gobj));
}

void ftAnim_8006EBA4(Fighter_GObj* gobj)
{
    AnimAdvance(Owner(gobj));
    ftAction_80073240(gobj);
}

void ftAnim_80070758(HSD_JObj* jobj)
{
    if (jobj)
        HSD_JObjRemoveAnimAllByFlags(jobj, 1);
}

bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    unsigned i;
    for (i = 0; i < f->jointCount; ++i) {
        HSD_AObj* aobj = f->joints[i]->aobj;
        if (aobj && !(aobj->flags & AOBJ_NO_ANIM))
            return true;
    }
    return false;
}

void ftAnim_8006F0FC(Fighter_GObj* gobj, float anim_rate)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_ForeachAnim(GET_JOBJ(gobj), JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetRate), AOBJ_ARG_AF, anim_rate);
    fp->frame_speed_mul = anim_rate;
}

void ftAnim_SetAnimRate(Fighter_GObj* gobj, float anim_rate)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->x2223_b0) {
        fp->x8A0_unk = anim_rate;
        return;
    }
    ftAnim_8006F0FC(gobj, anim_rate);
}

float ftAnim_8006F484(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    unsigned i;
    for (i = 0; i < f->jointCount; ++i)
        if (f->joints[i]->aobj)
            return f->joints[i]->aobj->end_frame;
    return 0.0f;
}

void ftAnim_8006EDD0(Fighter* fp, int arg1, float arg8, float arg9)
{
    (void) fp; (void) arg1; (void) arg8; (void) arg9;
    Unported("fighter.unported.anim_blend");
}

void ftAnim_8006FE9C(Fighter* fp, Fighter_Part start, float a, float b)
{
    (void) fp; (void) start; (void) a; (void) b;
}

void ftAnim_8006FF74(Fighter* fp, Fighter_Part start)
{
    (void) fp; (void) start;
}

bool ftAnim_8006F368(Fighter* fp, Fighter_Part part)
{
    return fp->parts[part].joint && lb_8000B074(fp->parts[part].joint);
}

/* ftAnim_8006DF0C: keep a grabbed fighter's special joint on its hip. */
void ftAnim_8006DF0C(Fighter* fp)
{
    if (fp->x2221_b2) {
        HSD_JObj* root = fp->parts[0].joint;
        HSD_JObj* special = fp->parts[DP(struct ftData_x8, fp->ft_data->x8)->x10].joint;
        Mtx mtx;
        Vec3 pos0, vec;
        DISC_VEC3_GET(pos0, p_ftCommonData->x808);
        lb_8000B1CC(fp->parts[ftParts_GetBoneIndex(fp, FtPart_HipN)].joint, &pos0, &vec);
        HSD_MtxInverse(HSD_JObjGetMtxPtr(root), mtx);
        PSMTXMultVec(mtx, &vec, &vec);
        HSD_JObjSetTranslate(special, &vec);
    }
}

/* Costume/part animation hooks from the Mario motion table (metal and
 * vitamin swaps); the native part-visibility path does not model them. */
void ftAnim_80070C48(Fighter_GObj* gobj, s32 arg)
{
    (void) gobj; (void) arg;
}

void ftAnim_80070CC4(Fighter_GObj* gobj, int arg)
{
    (void) gobj; (void) arg;
}

void ftAnim_80070FB4(Fighter_GObj* gobj, s32 a, s32 b)
{
    (void) gobj; (void) a; (void) b;
}

/* ftparts.c getters; the original ftPartGetRotZ reads the Y rotation. */
f32 ftPartGetRotX(Fighter* fp, int part_idx)
{
    HSD_JObj* jobj = fp->parts ? fp->parts[(u8) part_idx].joint : NULL;
    if (!jobj)
        return 0.0f;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        jobj = fp->parts[(u8) part_idx].x4_jobj2;
        if (!jobj || (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION))
            return 0.0f;
    }
    return HSD_JObjGetRotationX(jobj);
}

f32 ftPartGetRotZ(Fighter* fp, int part_idx)
{
    HSD_JObj* jobj = fp->parts ? fp->parts[(u8) part_idx].joint : NULL;
    if (!jobj)
        return 0.0f;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        jobj = fp->parts[(u8) part_idx].x4_jobj2;
        if (!jobj || (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION))
            return 0.0f;
    }
    return HSD_JObjGetRotationY(jobj);
}

int ftParts_80074A74(Fighter_GObj* gobj, int model_idx)
{
    return GET_FIGHTER(gobj)->x5F4_arr[model_idx].prev;
}

/* The shield-tilt pose blends a second skeleton (x8AC_animSkeleton) that the
 * native animation path does not build yet; the Guard animation itself still
 * plays through the normal motion tree. */
void ftAnim_8006F4C8(Fighter* fp, bool do_blending, FigaTree* tree)
{
    (void) fp; (void) do_blending; (void) tree;
}

void ftAnim_8006FA58(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    (void) fp; (void) part; (void) joint;
}

void ftAnim_8006FB88(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    (void) fp; (void) part; (void) joint;
}

void ftAnim_80070010(Fighter* fp, Fighter_Part start, float t, float t_inv, HSD_Joint* joint)
{
    (void) fp; (void) start; (void) t; (void) t_inv; (void) joint;
}

void ftAnim_80070108(Fighter* fp, Fighter_Part start, float t, float t_inv, HSD_Joint* joint)
{
    (void) fp; (void) start; (void) t; (void) t_inv; (void) joint;
}

void ftAnim_80070710(HSD_JObj* jobj, float frame)
{
    (void) jobj; (void) frame;
}

FigaTree* ftData_80085E50(Fighter* fp, enum_t msid)
{
    (void) fp; (void) msid;
    return NULL;
}



void ftData_80085CD8(Fighter* fp, Fighter* src, enum_t msid)
{
    M360LoadedKind* k;
    (void) src;
    KindDesc(fp->kind, &k);
    fp->x590 = LoadTree(k, msid);
    fp->x597_bits = fp->kind;
}

void ftAnim_8006EBE8(HSD_GObj* gobj, float anim_start, float anim_rate,
                     float anim_blend_frames)
{
    M360Fighter* f = Owner(gobj);
    Fighter* fp = &f->fighter;
    HSD_JObj* root = f->joints[0];
    (void) anim_blend_frames;
    ResetJoints(f);
    if (fp->x590)
        ApplyTree(f, fp->x590);
    HSD_JObjReqAnimAllByFlags(root, 1, anim_start);
    if (fp->x594_b1_loop)
        HSD_ForeachAnim(root, JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetFlags), AOBJ_ARG_AU, AOBJ_LOOP);
    HSD_ForeachAnim(root, JOBJ_TYPE, 0xFB7F, AnimCallback((void (*)(void)) HSD_AObjSetRate), AOBJ_ARG_AF, anim_rate);
    fp->x8A4_animBlendFrames = 0.0f;
    fp->x8A8_anim_frame = 0.0f;
}

static void UnknownMotion_Anim(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    M360_MatchTrace("fighter.unported.motion", (unsigned) fp->motion_id);
    ftCommon_8007D92C(gobj);
}

static void BuildMotionTables(void)
{
    MotionState unknown;
    unsigned i;
    memset(&unknown, 0, sizeof(unknown));
    unknown.anim_id = -1;
    unknown.anim_cb = UnknownMotion_Anim;
    s_unknownState = unknown;
    for (i = 0; i < ftCo_MS_Count; ++i)
        s_commonStates[i] = unknown;
    for (i = 0; i < M360_MotionTableCount; ++i)
        s_commonStates[M360_MotionTable[i].msid] = M360_MotionTable[i].state;
}

static int FloorAt(float x, float yTop, float yBottom, int allowPlatform, float* y,
                   int* line)
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned i;
    int best = -1;
    float bestY = -3.4e38f;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, ly;
        if (!(l->kind & M360_LINE_FLOOR))
            continue;
        if (!allowPlatform && (l->flags & M360_LINE_PLATFORM))
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (x < lo || x > hi || hi - lo < 1e-4f)
            continue;
        t = (x - l->x0) / (l->x1 - l->x0);
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (ly <= yTop && ly >= yBottom && ly > bestY) {
            bestY = ly;
            best = (int) i;
        }
    }
    if (best < 0)
        return 0;
    *y = bestY;
    *line = best;
    return 1;
}

int M360_MatchGroundBelow(float x, float y, float depth, float* groundY, unsigned* line)
{
    int l;
    if (!FloorAt(x, y, y - depth, 1, groundY, &l))
        return 0;
    *line = (unsigned) l;
    return 1;
}

static void SetFloorColl(CollData* cd, int line)
{
    const M360StageLine* l = &M360_MatchStageData()->lines[line];
    float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
    const float len = sqrtf(nx * nx + ny * ny);
    if (len > 0.0f) {
        nx /= len;
        ny /= len;
    }
    cd->floor.index = line;
    cd->floor.flags = l->flags;
    cd->floor.normal.x = nx;
    cd->floor.normal.y = ny;
    cd->floor.normal.z = 0.0f;
}

static void SetFloor(Fighter* fp, int line)
{
    SetFloorColl(&fp->coll_data, line);
}

u32 mpLineGetFlags(int line_id)
{
    const M360MatchStage* st = M360_MatchStageData();
    if (line_id < 0 || (unsigned) line_id >= st->lineCount)
        return 0;
    return st->lines[line_id].flags;
}

bool mpColl_IsOnPlatform(CollData* coll)
{
    return (mpLineGetFlags(coll->floor.index) & LINE_FLAG_PLATFORM) != 0;
}

void mpUpdateFloorSkip(CollData* coll)
{
    coll->floor_skip = coll->floor.index;
}

void mpClearFloorSkip(CollData* coll)
{
    coll->floor_skip = -1;
}

static int GroundStep(HSD_GObj* gobj, int stopAtEdge)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const M360MatchStage* st = M360_MatchStageData();
    const float step = 4.0f;
    float y;
    int line;
    fp->coll_data.env_flags &= ~(Collide_LeftEdge | Collide_RightEdge);
    if (FloorAt(fp->cur_pos.x, fp->cur_pos.y + step, fp->cur_pos.y - step, 1, &y, &line)) {
        fp->cur_pos.y = y;
        SetFloor(fp, line);
        fp->coll_data.env_flags |= Collide_FloorHug;
        return 1;
    }
    if (stopAtEdge && fp->coll_data.floor.index >= 0 &&
        (unsigned) fp->coll_data.floor.index < st->lineCount) {
        const M360StageLine* l = &st->lines[fp->coll_data.floor.index];
        const float lo = l->x0 < l->x1 ? l->x0 : l->x1;
        const float hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (fp->cur_pos.x < lo) {
            fp->cur_pos.x = lo;
            fp->coll_data.env_flags |= Collide_LeftEdge;
        } else {
            fp->cur_pos.x = hi;
            fp->coll_data.env_flags |= Collide_RightEdge;
        }
        fp->cur_pos.y = l->y0 + (l->y1 - l->y0) * ((fp->cur_pos.x - l->x0) / (l->x1 - l->x0));
        fp->gr_vel = 0.0f;
        fp->self_vel.x = 0.0f;
        return 1;
    }
    return 0;
}

GroundOrAir ft_80082708(Fighter_GObj* gobj)
{
    return (GroundOrAir) GroundStep(gobj, 0);
}

/* mpColl_8004A678_Floor: a fighter drifting off a floor end without a hard
 * tilt (|stick x| < 0.75) toward it stops on the edge and flags Collide_Edge,
 * which the callers turn into Ottotto (teeter). */
static int GroundStepEdge(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const int line = fp->coll_data.floor.index;
    const float stick = fp->input.lstick[0].x;
    Vec3 left, right;
    fp->coll_data.env_flags &= ~Collide_Edge;
    if (GroundStep(gobj, 0))
        return 1;
    if (!mpLib_80054ED8(line))
        return 0;
    mpFloorGetLeft(line, &left);
    mpFloorGetRight(line, &right);
    if (fp->cur_pos.x <= left.x && fp->facing_dir < 0.0f && stick > -0.75f) {
        fp->cur_pos.x = left.x;
        fp->cur_pos.y = left.y;
    } else if (fp->cur_pos.x >= right.x && fp->facing_dir > 0.0f && stick < 0.75f) {
        fp->cur_pos.x = right.x;
        fp->cur_pos.y = right.y;
    } else {
        return 0;
    }
    fp->coll_data.env_flags |= Collide_Edge;
    fp->gr_vel = 0.0f;
    fp->self_vel.x = 0.0f;
    return 0;
}

void ft_800843FC(Fighter_GObj* gobj)
{
    if (GroundStepEdge(gobj) || ftCo_8009A3C8(gobj))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_800848DC(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 0))
        cb(gobj);
}

bool ft_80084C74(Fighter_GObj* gobj, int* arg1, int* arg2, int* arg3)
{
    (void) gobj; (void) arg1; (void) arg2; (void) arg3;
    return false;
}

bool ft_80084CE4(Fighter* attacker, Fighter* victim)
{
    (void) attacker; (void) victim;
    return false;
}

bool ft_800827A0(Fighter_GObj* gobj)
{
    return GroundStep(gobj, 1);
}

void ft_80084280(Fighter_GObj* gobj)
{
    if (GroundStepEdge(gobj) || ftCo_8009A3C8(gobj))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_800844EC(Fighter_GObj* gobj)
{
    if (GroundStep(gobj, 0))
        return;
    ftCo_Fall_Enter(gobj);
}

void ft_80083F88(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

void ft_80084104(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 1))
        ftCo_Fall_Enter(gobj);
}

/* Finds the highest floor line crossed while moving from `from` to `to`,
 * honoring the platform-drop skip line and an optional per-line filter. */
static int LandBetweenColl(CollData* cd, HSD_GObj* owner, const Vec3* from, Vec3* to,
                           bool (*land)(Fighter_GObj*, int))
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned i;
    int best = -1;
    float bestY = -3.4e38f;
    if (cd->floor_skip >= 0 && (unsigned) cd->floor_skip < st->lineCount) {
        const M360StageLine* l = &st->lines[cd->floor_skip];
        const float lo = l->x0 < l->x1 ? l->x0 : l->x1;
        const float hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (to->x < lo || to->x > hi || to->y < (l->y0 < l->y1 ? l->y0 : l->y1) - 12.0f)
            cd->floor_skip = -1;
    }
    if (to->y > from->y)
        return 0;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, ly;
        if (!(l->kind & M360_LINE_FLOOR))
            continue;
        if ((int) i == cd->floor_skip)
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (to->x < lo || to->x > hi || hi - lo < 1e-4f)
            continue;
        t = (to->x - l->x0) / (l->x1 - l->x0);
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (from->y >= ly - 0.01f && to->y <= ly && ly > bestY) {
            if (land && (!owner || !land(owner, (int) i)))
                continue;
            bestY = ly;
            best = (int) i;
        }
    }
    if (best < 0)
        return 0;
    to->y = bestY;
    SetFloorColl(cd, best);
    return 1;
}

static int LandBetween(Fighter* fp, const Vec3* from, Vec3* to,
                       bool (*land)(Fighter_GObj*, int))
{
    return LandBetweenColl(&fp->coll_data, fp->gobj, from, to, land);
}

static int AirStep(HSD_GObj* gobj, bool (*land)(Fighter_GObj*, int))
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;
    if (!LandBetween(fp, &fp->prev_pos, &fp->cur_pos, land))
        return 0;
    fp->coll_data.cur_pos = fp->cur_pos;
    return 1;
}

static void AirWalls(Fighter* fp)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float mid = fp->cur_pos.y + 8.0f;
    unsigned i;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, lx;
        if (!(l->kind & (M360_LINE_LEFT_WALL | M360_LINE_RIGHT_WALL)))
            continue;
        lo = l->y0 < l->y1 ? l->y0 : l->y1;
        hi = l->y0 < l->y1 ? l->y1 : l->y0;
        if (mid < lo || mid > hi || hi - lo < 1e-4f)
            continue;
        t = (mid - l->y0) / (l->y1 - l->y0);
        lx = l->x0 + (l->x1 - l->x0) * t;
        if ((l->kind & M360_LINE_LEFT_WALL) && fp->prev_pos.x <= lx && fp->cur_pos.x > lx) {
            fp->cur_pos.x = lx;
            if (fp->self_vel.x > 0.0f) fp->self_vel.x = 0.0f;
        } else if ((l->kind & M360_LINE_RIGHT_WALL) && fp->prev_pos.x >= lx && fp->cur_pos.x < lx) {
            fp->cur_pos.x = lx;
            if (fp->self_vel.x < 0.0f) fp->self_vel.x = 0.0f;
        }
    }
}

static void AirCeilings(Fighter* fp)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float height = fp->ft_data && fp->ft_data->x3C
        ? fp->ft_data->x3C->xC.x * fp->x34_scale.y : 12.0f;
    unsigned i;
    if (fp->cur_pos.y <= fp->prev_pos.y)
        return;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float lo, hi, t, lx, ly;
        if (!(l->kind & M360_LINE_CEILING))
            continue;
        lo = l->x0 < l->x1 ? l->x0 : l->x1;
        hi = l->x0 < l->x1 ? l->x1 : l->x0;
        if (hi - lo < 1e-4f)
            continue;
        t = (fp->cur_pos.x - l->x0) / (l->x1 - l->x0);
        if (t < 0.0f || t > 1.0f)
            continue;
        lx = l->x0 + (l->x1 - l->x0) * t;
        ly = l->y0 + (l->y1 - l->y0) * t;
        if (fp->prev_pos.y + height <= ly && fp->cur_pos.y + height >= ly) {
            fp->cur_pos.y = ly - height;
            if (fp->self_vel.y > 0.0f) fp->self_vel.y = 0.0f;
            if (fp->x8c_kb_vel.y > 0.0f) fp->x8c_kb_vel.y = 0.0f;
            fp->coll_data.ceiling.index = (s16) i;
            (void) lx;
            return;
        }
    }
}

/* Native counterpart of mpColl_80044164/800443C4: looks for a ledge-flagged
 * floor end inside the fighter's ledge-snap box (ftData x44) while falling. */
static int FindLedge(Fighter* fp, int left, int* ledge)
{
    const M360MatchStage* st = M360_MatchStageData();
    const ftData_x44_t* snap = DP(ftData_x44_t, fp->ft_data->x44);
    const float scale = fp->x34_scale.y;
    const float snapX = snap->ledge_snap_x * scale;
    const float snapY = snap->ledge_snap_y * scale;
    const float half = 0.5f * snap->ledge_snap_height * scale;
    const Vec3* prev = &fp->prev_pos;
    const Vec3* cur = &fp->cur_pos;
    const float bottom = (cur->y < prev->y ? cur->y : prev->y) + snapY - half;
    const float top = (cur->y < prev->y ? prev->y : cur->y) + snapY + half;
    float lo, hi;
    unsigned i;
    if (left) {
        lo = prev->x < cur->x ? prev->x : cur->x;
        hi = snapX + (prev->x < cur->x ? cur->x : prev->x);
    } else {
        lo = (prev->x < cur->x ? prev->x : cur->x) - snapX;
        hi = prev->x < cur->x ? cur->x : prev->x;
    }
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        float ex, ey;
        if (!(l->kind & M360_LINE_FLOOR) || !(l->flags & LINE_FLAG_LEDGE) || (int) i == fp->coll_data.floor_skip)
            continue;
        if (left == (l->x0 < l->x1)) {
            ex = l->x0; ey = l->y0;
        } else {
            ex = l->x1; ey = l->y1;
        }
        if (ex < lo || ex > hi || ey < bottom || ey > top)
            continue;
        if ((left && cur->x >= ex) || (!left && cur->x <= ex) || cur->y >= ey)
            continue;
        *ledge = (int) i;
        return 1;
    }
    return 0;
}

/* Raises Collide_*LedgeGrab like mpColl_80047E14 for the given facing
 * (0 checks both sides). */
static int DetectLedge(Fighter* fp, int dir, int* ledge)
{
    fp->coll_data.env_flags &= ~Collide_LedgeGrabMask;
    if (fp->x2064_ledgeCooldown || fp->x2224_b2 || fp->cur_pos.y >= fp->prev_pos.y ||
        !fp->ft_data || !fp->ft_data->x44)
        return 0;
    if (dir >= 0 && FindLedge(fp, 1, ledge)) {
        fp->coll_data.env_flags |= Collide_LeftLedgeGrab;
        fp->coll_data.ledge_id_left = *ledge;
        return 1;
    }
    if (dir <= 0 && FindLedge(fp, 0, ledge)) {
        fp->coll_data.env_flags |= Collide_RightLedgeGrab;
        fp->coll_data.ledge_id_right = *ledge;
        return 1;
    }
    return 0;
}

/* ---- Item-system entry points over the native stage and runtime ---- */

void (HSD_JObjSetMtxDirty)(HSD_JObj* jobj)
{
    HSD_JObjSetMtxDirtyInline(jobj);
}

f32 Stage_GetBlastZoneLeftOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->blastLeft + st->camX;
}

f32 Stage_GetBlastZoneRightOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->blastRight + st->camX;
}

f32 Stage_GetBlastZoneTopOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->blastTop + st->camY;
}

f32 Stage_GetBlastZoneBottomOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->blastBottom + st->camY;
}

float Ground_801C0498(void)
{
    return M360_MatchStageScale();
}

void Camera_GetTransformInterest(Vec* out)
{
    float interest[3], eye[3];
    M360_MatchCameraVectors(interest, eye);
    out->x = interest[0];
    out->y = interest[1];
    out->z = interest[2];
}

void Camera_GetTransformPosition(Vec* out)
{
    float interest[3], eye[3];
    M360_MatchCameraVectors(interest, eye);
    out->x = eye[0];
    out->y = eye[1];
    out->z = eye[2];
}

/* lbAudioAx_800237A8: one-shot SFX; the id translation is the identity. */
int lbAudioAx_800237A8(enum_t sfx_id, int sfx_vol, int sfx_pan)
{
    if (sfx_id != 0x83D60 && sfx_id != 0x83D61)
        M360_AudioSfx((unsigned) sfx_id, (unsigned) sfx_vol, (unsigned) sfx_pan);
    return 0;
}

int lbAudioAx_800233EC(int sfx_id)
{
    return sfx_id;
}

int lbAudioAx_800236B8(int handle)
{
    (void) handle;
    return 0;
}

/* lbArchive_80017040: load an archive and resolve (out, name) pairs. Like the
 * original's lbDvd preload cache, a file loaded before is reused (it has
 * already been relocated) and reported as preloaded. */
typedef struct M360CachedArchive {
    const char* name;
    HSD_Archive* archive;
} M360CachedArchive;

static M360CachedArchive s_archiveCache[64];
static unsigned s_archiveCacheCount;

bool lbArchive_80017040(HSD_Archive** dst, const char* filename, void* symbols, ...)
{
    HSD_Archive* archive = NULL;
    bool preloaded = false;
    unsigned i;
    va_list args;
    for (i = 0; i < s_archiveCacheCount; ++i)
        if (strcmp(s_archiveCache[i].name, filename) == 0) {
            archive = s_archiveCache[i].archive;
            preloaded = true;
            break;
        }
    va_start(args, symbols);
    if (preloaded) {
        void** out = symbols;
        while (out) {
            const char* name = va_arg(args, const char*);
            *out = archive ? M360_ArchiveFind(archive, name) : NULL;
            out = va_arg(args, void**);
        }
    } else {
        archive = M360_ArchiveLoadSymbolsV(filename, symbols, args);
        if (archive && s_archiveCacheCount < 64) {
            s_archiveCache[s_archiveCacheCount].name = filename;
            s_archiveCache[s_archiveCacheCount].archive = archive;
            ++s_archiveCacheCount;
        }
    }
    va_end(args);
    if (dst)
        *dst = archive;
    return preloaded;
}

void mpCollSetFacingDir(CollData* coll, int dir)
{
    coll->facing_dir = dir;
}

/* Battlefield-style stages have no moving collision joints. */
static bool NoSurfaceSpeed(Vec3* speed)
{
    speed->x = speed->y = speed->z = 0.0f;
    return false;
}

bool mpCollGetSpeedFloor(CollData* coll, Vec3* speed) { (void) coll; return NoSurfaceSpeed(speed); }
bool mpCollGetSpeedCeiling(CollData* coll, Vec3* speed) { (void) coll; return NoSurfaceSpeed(speed); }
bool mpCollGetSpeedLeftWall(CollData* coll, Vec3* speed) { (void) coll; return NoSurfaceSpeed(speed); }
bool mpCollGetSpeedRightWall(CollData* coll, Vec3* speed) { (void) coll; return NoSurfaceSpeed(speed); }

void mpColl_SetECBSource_Fixed(CollData* cd, HSD_GObj* gobj, float up, float down, float front,
                               float back)
{
    cd->x0_gobj = gobj;
    cd->ecb_source.kind = ECBSource_Fixed;
    cd->ecb_source.up = up;
    cd->ecb_source.down = down;
    cd->ecb_source.front = front;
    cd->ecb_source.back = back;
    cd->ecb_source.angle = 0.0f;
}

void mpColl_800436E4(CollData* coll, float angle)
{
    coll->ecb_source.angle = angle;
}

void mpColl_80043558(CollData* coll, int line_id)
{
    (void) coll; (void) line_id;
}

/* Ground collision variants used by items (inline2/3/4 in mpcoll.c): snap to
 * a floor within reach of the current position, otherwise fall. */
static bool CollGround(CollData* coll)
{
    float y;
    int line;
    coll->env_flags &= ~Collide_FloorMask;
    if (!FloorAt(coll->cur_pos.x, coll->cur_pos.y + 4.0f, coll->cur_pos.y - 4.0f, 1, &y, &line))
        return false;
    coll->cur_pos.y = y;
    SetFloorColl(coll, line);
    coll->env_flags |= Collide_FloorHug;
    return true;
}

bool mpColl_80048844(CollData* coll) { return CollGround(coll); }
bool mpColl_8004B108(CollData* coll) { return CollGround(coll); }
bool mpColl_8004B2DC(CollData* coll) { return CollGround(coll); }
bool mpColl_8004C750(CollData* coll) { return CollGround(coll); }

/* mpColl_8004D024: is there floor directly under this point? */
bool mpColl_8004D024(Vec3* pos)
{
    float y;
    int line;
    return FloorAt(pos->x, pos->y + 1.0f, pos->y - 3.0f, 1, &y, &line) != 0;
}

Vec3* mpLineGetNormal(int line_id, Vec3* normal_out)
{
    const M360StageLine* l;
    float nx, ny, len;
    normal_out->x = normal_out->z = 0.0f;
    normal_out->y = 1.0f;
    if (!mpLib_80054ED8(line_id))
        return normal_out;
    l = &M360_MatchStageData()->lines[line_id];
    nx = -(l->y1 - l->y0);
    ny = l->x1 - l->x0;
    len = sqrtf(nx * nx + ny * ny);
    if (len > 0.0f) {
        normal_out->x = nx / len;
        normal_out->y = ny / len;
    }
    return normal_out;
}

bool mpCheckMultiple(float x0, float y0, float x1, float y1, Vec3* pos_out, int* line_id_out,
                     u32* flags_out, Vec3* normal_out, u32 checks, int joint_id_skip,
                     int joint_id_only)
{
    (void) checks;
    return mpCheckAllRemap(pos_out, line_id_out, flags_out, normal_out, joint_id_skip,
                           joint_id_only, x0, y0, x1, y1);
}

/* ---- Stage islands and line-kind raycasts for the original CPU AI ---- */

enum { kMaxIslands = 64 };
static mp_UnkStruct0 s_islands[kMaxIslands];
static int s_islandOfLine[128];
struct mpIsland_80458E88_t mpIsland_80458E88;

static int LineEndsJoin(const M360StageLine* a, const M360StageLine* b)
{
    const float dx = a->x1 - b->x0, dy = a->y1 - b->y0;
    return dx * dx + dy * dy < 4.0f;
}

/* mpIsland_8005A728 over the native lines: chains of connected floor lines,
 * left to right, with their end points. Ceiling/wall lists stay empty. */
void M360_FighterBuildIslands(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    mp_UnkStruct0* prev = NULL;
    unsigned used = 0, i, j;
    memset(&mpIsland_80458E88, 0, sizeof(mpIsland_80458E88));
    memset(s_islands, 0, sizeof(s_islands));
    for (i = 0; i < 128; ++i)
        s_islandOfLine[i] = -1;
    for (i = 0; i < st->lineCount && used < kMaxIslands; ++i) {
        mp_UnkStruct0* isl;
        unsigned first = i, last = i;
        int grew;
        if (!(st->lines[i].kind & M360_LINE_FLOOR) || s_islandOfLine[i] >= 0)
            continue;
        isl = &s_islands[used];
        s_islandOfLine[i] = (int) used;
        do {
            grew = 0;
            for (j = 0; j < st->lineCount; ++j) {
                if (!(st->lines[j].kind & M360_LINE_FLOOR) || s_islandOfLine[j] >= 0)
                    continue;
                if (LineEndsJoin(&st->lines[last], &st->lines[j])) {
                    last = j;
                } else if (LineEndsJoin(&st->lines[j], &st->lines[first])) {
                    first = j;
                } else {
                    continue;
                }
                s_islandOfLine[j] = (int) used;
                grew = 1;
            }
        } while (grew);
        isl->x24 = (s16) first;
        isl->x26 = (s16) last;
        isl->x8.x = st->lines[first].x0;
        isl->x8.y = st->lines[first].y0;
        isl->x14.x = st->lines[last].x1;
        isl->x14.y = st->lines[last].y1;
        isl->x28 = 0;
        if (prev)
            prev->next = isl;
        else
            mpIsland_80458E88.next = isl;
        prev = isl;
        ++used;
    }
    mpIsland_80458E88.x8 = prev;
    M360_MatchTrace("stage.islands", used);
}

mp_UnkStruct0* mpIsland_8005AB54(int line_idx)
{
    if (line_idx < 0 || line_idx >= 128 || s_islandOfLine[line_idx] < 0)
        return NULL;
    return &s_islands[s_islandOfLine[line_idx]];
}

bool mpIsland_8005AC8C(mp_UnkStruct0* island)
{
    (void) island;
    return false;
}

void mpIsland_8005ACE8(mp_UnkStruct0* island, Vec3* left, Vec3* right)
{
    if (left)
        *left = island->x8;
    if (right)
        *right = island->x14;
}

/* Segment test against lines of one kind (mpCheckFloor & co). */
static bool RaycastKind(unsigned kindMask, float ax, float ay, float bx, float by, Vec3* pos_out,
                        int* line_id_out, u32* flags_out, Vec3* normal_out, int line_skip,
                        bool (*filter)(Fighter_GObj*, int), Fighter_GObj* gobj)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float dx = bx - ax, dy = by - ay;
    float bestT = 2.0f;
    int best = -1;
    unsigned i;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        const float ex = l->x1 - l->x0, ey = l->y1 - l->y0;
        const float den = dx * ey - dy * ex;
        float t, u;
        if (!(l->kind & kindMask) || (int) i == line_skip)
            continue;
        if (den > -1e-6f && den < 1e-6f)
            continue;
        t = ((l->x0 - ax) * ey - (l->y0 - ay) * ex) / den;
        u = ((l->x0 - ax) * dy - (l->y0 - ay) * dx) / den;
        if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f || t >= bestT)
            continue;
        if (filter && gobj && !filter(gobj, (int) i))
            continue;
        bestT = t;
        best = (int) i;
    }
    if (best < 0)
        return false;
    if (pos_out) {
        pos_out->x = ax + dx * bestT;
        pos_out->y = ay + dy * bestT;
        pos_out->z = 0.0f;
    }
    if (line_id_out)
        *line_id_out = best;
    if (flags_out)
        *flags_out = st->lines[best].flags;
    if (normal_out)
        mpLineGetNormal(best, normal_out);
    return true;
}

bool mpCheckFloor(float ax, float ay, float bx, float by, float y_offset, Vec3* vec_out,
                  int* line_id_out, u32* flags_out, Vec3* normal_out, int line_id_skip,
                  int joint_id_skip, int joint_id_only, bool (*filter)(Fighter_GObj*, int),
                  Fighter_GObj* gobj)
{
    bool hit;
    (void) joint_id_skip; (void) joint_id_only;
    hit = RaycastKind(M360_LINE_FLOOR, ax, ay + y_offset, bx, by + y_offset, vec_out, line_id_out,
                      flags_out, normal_out, line_id_skip, filter, gobj);
    if (hit && vec_out)
        vec_out->y -= y_offset;
    return hit;
}

bool mpCheckCeiling(float ax, float ay, float bx, float by, Vec3* vec_out, int* line_id_out,
                    u32* flags_out, Vec3* normal_out, int joint_id_skip, int joint_id_only)
{
    (void) joint_id_skip; (void) joint_id_only;
    return RaycastKind(M360_LINE_CEILING, ax, ay, bx, by, vec_out, line_id_out, flags_out,
                       normal_out, -1, NULL, NULL);
}

bool mpCheckLeftWall(float ax, float ay, float bx, float by, Vec3* vec_out, int* line_id_out,
                     u32* flags_out, Vec3* normal_out, int joint_id_skip, int joint_id_only)
{
    (void) joint_id_skip; (void) joint_id_only;
    return RaycastKind(M360_LINE_LEFT_WALL, ax, ay, bx, by, vec_out, line_id_out, flags_out,
                       normal_out, -1, NULL, NULL);
}

bool mpCheckRightWall(float ax, float ay, float bx, float by, Vec3* vec_out, int* line_id_out,
                      u32* flags_out, Vec3* normal_out, int joint_id_skip, int joint_id_only)
{
    (void) joint_id_skip; (void) joint_id_only;
    return RaycastKind(M360_LINE_RIGHT_WALL, ax, ay, bx, by, vec_out, line_id_out, flags_out,
                       normal_out, -1, NULL, NULL);
}

bool mpCheckAll(Vec3* pos_out, int* line_id_out, u32* flags_out, Vec3* normal_out,
                int joint_id_skip, int joint_id_only, float x0, float y0, float x1, float y1)
{
    return mpCheckAllRemap(pos_out, line_id_out, flags_out, normal_out, joint_id_skip,
                           joint_id_only, x0, y0, x1, y1);
}

mp_UnkStruct0* mpIsland_8005AC14(Vec3* pos, float dist)
{
    int line;
    if (mpCheckFloor(pos->x, pos->y, pos->x, pos->y + dist, 0.0f, NULL, &line, NULL, NULL, -1, -1,
                     -1, NULL, NULL))
        return mpIsland_8005AB54(line);
    return NULL;
}

/* lbArchive_InitializeDAT: parse an archive image already in memory (effect
 * banks) with the XEX's archive.c. */
void lbArchive_InitializeDAT(HSD_Archive* archive, void* data, size_t length)
{
    memset(archive, 0, sizeof(*archive));
    HSD_ArchiveParse(archive, (u8*) data, length);
}

HSD_TObj* HSD_MObjGetTObj(HSD_MObj* mobj)
{
    return mobj ? mobj->tobj : NULL;
}

/* mpColl_80044164/800443C4: CollData-level left/right ledge queries. */
bool mpColl_80044164(CollData* cd, int* p_ledge_id)
{
    int ledge;
    if (!FindLedge((Fighter*) ((char*) cd - offsetof(Fighter, coll_data)), 1, &ledge))
        return false;
    if (p_ledge_id)
        *p_ledge_id = ledge;
    return true;
}

bool mpColl_800443C4(CollData* cd, int* p_ledge_id)
{
    int ledge;
    if (!FindLedge((Fighter*) ((char*) cd - offsetof(Fighter, coll_data)), 0, &ledge))
        return false;
    if (p_ledge_id)
        *p_ledge_id = ledge;
    return true;
}

static int TryCliff(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int ledge;
    if (!DetectLedge(fp, fp->facing_dir < 0.0f ? -1 : 1, &ledge))
        return 0;
    if (ftCliffCommon_80081298(gobj)) {
        M360_MatchTrace("fighter.cliff.catch", (unsigned) ledge);
        return 1;
    }
    return 0;
}

void ft_800831CC(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_800835B0(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, arg1))
        cb(gobj);
    else
        TryCliff(gobj);
}

bool ft_80081DD4(Fighter_GObj* gobj)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    return AirStep(gobj, NULL) != 0;
}

bool ft_80081F2C(Fighter_GObj* gobj)
{
    return ft_80081DD4(gobj);
}

bool ft_80082084(Fighter_GObj* gobj)
{
    return ft_80081DD4(gobj);
}

void ft_8008370C(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (ft_80081DD4(gobj))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_80083318(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_80083464(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_80082D40(Fighter_GObj* gobj, f32 arg1)
{
    (void) arg1;
    if (ft_80081DD4(gobj)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp))
            ft_8008A2BC(gobj);
        else
            ftCo_Landing_Enter_Basic(gobj);
    }
}

void ft_80082C74(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, NULL))
        cb(gobj);
}

void ft_80082F28(Fighter_GObj* gobj)
{
    AirWalls(GET_FIGHTER(gobj));
    AirCeilings(GET_FIGHTER(gobj));
    if (AirStep(gobj, NULL)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp)) {
            ft_8008A2BC(gobj);
            return;
        }
        ftCo_Landing_Enter_Basic(gobj);
    } else {
        TryCliff(gobj);
    }
}

void ft_80083090(Fighter_GObj* gobj, bool (*arg1)(Fighter_GObj*, int), HSD_GObjEvent cb)
{
    ft_800831CC(gobj, arg1, cb);
}

void ft_800845B4(Fighter_GObj* gobj)
{
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

void ft_800847D0(Fighter_GObj* gobj, ftCollisionBox* box)
{
    (void) box;
    if (!GroundStep(gobj, 0))
        ftCo_Fall_Enter(gobj);
}

/* Air collision used by grab/throw states (mpColl_800477E0 in ft_80082578):
 * reports floor contact through env_flags like the original. */
static int AirCollide(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->coll_data.env_flags &= ~Collide_FloorMask;
    AirWalls(fp);
    AirCeilings(fp);
    if (!AirStep(gobj, NULL))
        return 0;
    fp->coll_data.env_flags |= Collide_FloorHug;
    return 1;
}

GroundOrAir ft_80081D0C(Fighter_GObj* gobj)
{
    return AirCollide(gobj) ? GA_Air : GA_Ground;
}

bool ft_800824A0(Fighter_GObj* gobj, ftCollisionBox* ecb)
{
    (void) ecb;
    return AirCollide(gobj) != 0;
}

bool ft_80082888(Fighter_GObj* gobj, ftCollisionBox* ecb)
{
    (void) ecb;
    return GroundStep(gobj, 0) != 0;
}

void ft_80083910(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (AirCollide(gobj))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_80083B68(Fighter_GObj* gobj)
{
    AirCollide(gobj);
}

void ft_80083844(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    Fighter* fp = GET_FIGHTER(gobj);
    bool landed;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;
    landed = mpColl_80048654(&fp->coll_data);
    fp->cur_pos = fp->coll_data.cur_pos;
    if (landed)
        cb(gobj);
}

void ft_80083DCC(Fighter_GObj* gobj)
{
    AirCollide(gobj);
}

/* ft_CheckGroundAndLedge/ft_8008239C: air collision that also flags a
 * grabbable ledge for the caller's own ftCliffCommon_80081298 check. */
bool ft_CheckGroundAndLedge(Fighter_GObj* gobj, int dir)
{
    int ledge;
    if (AirCollide(gobj))
        return true;
    DetectLedge(GET_FIGHTER(gobj), dir, &ledge);
    return false;
}

bool ft_8008239C(Fighter_GObj* gobj, int dir, ftCollisionBox* height_attributes)
{
    (void) height_attributes;
    return ft_CheckGroundAndLedge(gobj, dir);
}

bool ft_80082978(HSD_GObj* gobj, ftCollisionBox* arg1)
{
    (void) arg1;
    return GroundStep(gobj, 1) != 0;
}

bool ft_80082578(Fighter_GObj* gobj)
{
    return AirCollide(gobj) != 0;
}

bool ft_80082638(Fighter_GObj* gobj, ftCollisionBox* box)
{
    (void) box;
    return AirCollide(gobj) != 0;
}

void ft_80083E64(Fighter_GObj* gobj, ftCollisionBox* ecb, HSD_GObjEvent cb)
{
    (void) ecb;
    if (AirCollide(gobj))
        cb(gobj);
}

void ft_80084CB0(Fighter* fp, ftCollisionBox* box)
{
    box->top = fp->coll_data.ecb.top.y;
    box->bottom = fp->coll_data.ecb.bottom.y;
    box->right.x = fp->coll_data.ecb.right.x;
    box->right.y = fp->coll_data.ecb.right.y;
    box->left.x = fp->coll_data.ecb.left.x;
    box->left.y = fp->coll_data.ecb.left.y;
}

bool ft_800821DC(Fighter_GObj* gobj)
{
    return AirCollide(gobj) != 0;
}

void mpLib_80053ECC_Floor(int line_id, Vec* vec)
{
    const M360StageLine* l;
    if (!mpLib_80054ED8(line_id))
        return;
    l = &M360_MatchStageData()->lines[line_id];
    vec->x = l->x0 < l->x1 ? l->x0 : l->x1;
    vec->y = l->x0 < l->x1 ? l->y0 : l->y1;
    vec->z = 0.0f;
}

void mpFloorGetLeft(int line_id, Vec3* vec)
{
    mpLib_80053ECC_Floor(line_id, vec);
}

void mpFloorGetRight(int line_id, Vec3* vec)
{
    mpLib_80053DA4_Floor(line_id, vec);
}

void mpLib_80053DA4_Floor(int line_id, Vec3* vec)
{
    const M360StageLine* l;
    if (!mpLib_80054ED8(line_id))
        return;
    l = &M360_MatchStageData()->lines[line_id];
    vec->x = l->x0 < l->x1 ? l->x1 : l->x0;
    vec->y = l->x0 < l->x1 ? l->y1 : l->y0;
    vec->z = 0.0f;
}

void ft_80083A48(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (AirCollide(gobj))
        cb(gobj);
    else
        TryCliff(gobj);
}

void ft_80083C00(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (AirCollide(gobj))
        cb(gobj);
}

void ft_80083CE4(Fighter_GObj* gobj, bool (*cb1)(Fighter_GObj*, int), HSD_GObjEvent cb2)
{
    Fighter* fp = GET_FIGHTER(gobj);
    AirWalls(fp);
    AirCeilings(fp);
    if (AirStep(gobj, cb1))
        cb2(gobj);
}

void ft_8008403C(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 0))
        cb(gobj);
}

void ft_800841B8(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    if (!GroundStep(gobj, 1))
        cb(gobj);
}

void ftCo_AirCatchHit_Coll(Fighter_GObj* gobj)
{
    if (AirCollide(gobj)) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp->self_vel.y > ftCo_800D0EC8(fp))
            ft_8008A2BC(gobj);
        else
            ftCo_Landing_Enter_Basic(gobj);
    }
}

/* CollData-only entry points used when a grab releases or throws its victim
 * and by items such as Ness's yo-yo that own a CollData of their own. */

void mpColl_80043670(CollData* coll)
{
    coll->x130_flags |= CollData_X130_Clear;
}

void mpColl_80043680(CollData* coll, Vec3* pos)
{
    coll->cur_pos = *pos;
    coll->prev_pos = coll->cur_pos;
    coll->last_pos = coll->prev_pos;
    coll->x130_flags |= CollData_X130_Clear;
}

static bool CollAir(CollData* coll)
{
    coll->env_flags &= ~Collide_FloorMask;
    if (!LandBetweenColl(coll, NULL, &coll->last_pos, &coll->cur_pos, NULL))
        return false;
    coll->env_flags |= Collide_FloorHug;
    return true;
}

bool mpColl_800471F8(CollData* coll)
{
    return CollAir(coll);
}

bool mpColl_800477E0(CollData* coll)
{
    return CollAir(coll);
}

bool mpColl_8004730C(CollData* coll, ftCollisionBox* box)
{
    (void) box;
    return CollAir(coll);
}

/* mpColl_80041EE4: reset a CollData to its unattached state. */
void mpColl_80041EE4(CollData* cd)
{
    cd->x0_gobj = NULL;
    cd->env_flags = 0;
    cd->x130_flags = 0;
    cd->prev_pos = cd->cur_pos;
    cd->last_pos = cd->cur_pos;
    cd->floor_skip = -1;
    cd->ledge_id_right = -1;
    cd->ledge_id_left = -1;
    cd->floor.index = -1;
    cd->floor.flags = 0;
    cd->floor.normal.x = 0.0f;
    cd->floor.normal.y = 1.0f;
    cd->floor.normal.z = 0.0f;
    cd->ceiling.index = -1;
    cd->right_facing_wall.index = -1;
    cd->left_facing_wall.index = -1;
}

/* Floor-material friction (mpLib_803BF248); the native stage data has no
 * material table, matching ft_GetGroundFrictionMultiplier's 1.0. */
float mpLib_800569EC(u32 flags)
{
    (void) flags;
    return 1.0f;
}

bool mpColl_80048654(CollData* coll)
{
    float y;
    int line;
    coll->env_flags &= ~Collide_FloorMask;
    if (!FloorAt(coll->cur_pos.x, coll->cur_pos.y + 4.0f, coll->cur_pos.y - 4.0f, 1, &y, &line))
        return CollAir(coll);
    coll->cur_pos.y = y;
    SetFloorColl(coll, line);
    coll->env_flags |= Collide_FloorHug;
    return true;
}

bool mpLib_80054ED8(int line_id)
{
    return line_id >= 0 && (unsigned) line_id < M360_MatchStageData()->lineCount;
}

int mpLib_8005199C_Floor(Vec3* vec, int joint_id_skip, int joint_id_only)
{
    float y;
    int line;
    (void) joint_id_skip; (void) joint_id_only;
    return FloorAt(vec->x, vec->y, -30000.0f, 1, &y, &line) ? line : -1;
}

static int LineEndsMeet(const M360StageLine* a, const M360StageLine* b)
{
    const float d0x = a->x1 - b->x0, d0y = a->y1 - b->y0;
    const float d1x = a->x0 - b->x1, d1y = a->y0 - b->y1;
    return d0x * d0x + d0y * d0y < 4.0f || d1x * d1x + d1y * d1y < 4.0f;
}

bool mpLinesConnected(int start_id, int target_id)
{
    const M360MatchStage* st = M360_MatchStageData();
    unsigned char seen[128];
    int queue[128];
    int head = 0, tail = 0;
    unsigned i;
    if (!mpLib_80054ED8(start_id) || !mpLib_80054ED8(target_id))
        return false;
    if (start_id == target_id)
        return true;
    memset(seen, 0, sizeof(seen));
    seen[start_id] = 1;
    queue[tail++] = start_id;
    while (head < tail) {
        const M360StageLine* cur = &st->lines[queue[head++]];
        for (i = 0; i < st->lineCount; ++i) {
            if (seen[i] || st->lines[i].kind != cur->kind || !LineEndsMeet(cur, &st->lines[i]))
                continue;
            if ((int) i == target_id)
                return true;
            seen[i] = 1;
            queue[tail++] = (int) i;
        }
    }
    return false;
}

/* mpCheckAllRemap: nearest stage line crossed by the segment (x0,y0)-(x1,y1). */
bool mpCheckAllRemap(Vec3* pos_out, int* line_id_out, u32* flags_out, Vec3* normal_out,
                     int joint_id_skip, int joint_id_only, float x0, float y0, float x1, float y1)
{
    const M360MatchStage* st = M360_MatchStageData();
    const float dx = x1 - x0, dy = y1 - y0;
    float bestT = 2.0f;
    int best = -1;
    unsigned i;
    (void) joint_id_skip; (void) joint_id_only;
    for (i = 0; i < st->lineCount; ++i) {
        const M360StageLine* l = &st->lines[i];
        const float ex = l->x1 - l->x0, ey = l->y1 - l->y0;
        const float den = dx * ey - dy * ex;
        float t, u;
        if (den > -1e-6f && den < 1e-6f)
            continue;
        t = ((l->x0 - x0) * ey - (l->y0 - y0) * ex) / den;
        u = ((l->x0 - x0) * dy - (l->y0 - y0) * dx) / den;
        if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f || t >= bestT)
            continue;
        bestT = t;
        best = (int) i;
    }
    if (best < 0)
        return false;
    if (pos_out) {
        pos_out->x = x0 + dx * bestT;
        pos_out->y = y0 + dy * bestT;
        pos_out->z = 0.0f;
    }
    if (line_id_out)
        *line_id_out = best;
    if (flags_out)
        *flags_out = st->lines[best].flags;
    if (normal_out) {
        const M360StageLine* l = &st->lines[best];
        float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
        const float len = sqrtf(nx * nx + ny * ny);
        normal_out->x = len > 0.0f ? nx / len : 0.0f;
        normal_out->y = len > 0.0f ? ny / len : 1.0f;
        normal_out->z = 0.0f;
    }
    return true;
}

int mpLib_8004DD90_Floor(int line_id, Vec3* vec, float* y_out, u32* flags_out, Vec3* normal_out)
{
    const M360StageLine* l;
    float x, t;
    if (!mpLib_80054ED8(line_id))
        return -1;
    l = &M360_MatchStageData()->lines[line_id];
    x = vec->x;
    if (x < (l->x0 < l->x1 ? l->x0 : l->x1))
        x = l->x0 < l->x1 ? l->x0 : l->x1;
    if (x > (l->x0 < l->x1 ? l->x1 : l->x0))
        x = l->x0 < l->x1 ? l->x1 : l->x0;
    t = l->x1 != l->x0 ? (x - l->x0) / (l->x1 - l->x0) : 0.0f;
    if (y_out)
        *y_out = l->y0 + (l->y1 - l->y0) * t - vec->y;
    if (flags_out)
        *flags_out = l->flags;
    if (normal_out) {
        float nx = -(l->y1 - l->y0), ny = l->x1 - l->x0;
        const float len = sqrtf(nx * nx + ny * ny);
        normal_out->x = len > 0.0f ? nx / len : 0.0f;
        normal_out->y = len > 0.0f ? ny / len : 1.0f;
        normal_out->z = 0.0f;
    }
    return line_id;
}

bool ft_80084A18(Fighter_GObj* gobj)
{
    const int line = GET_FIGHTER(gobj)->coll_data.floor.index;
    return line >= 0 && (unsigned) line < M360_MatchStageData()->lineCount;
}

float ft_GetGroundFrictionMultiplier(Fighter* fp)
{
    (void) fp;
    return 1.0f;
}

void ftCamera_UpdateCameraBox(HSD_GObj* gobj)
{
    (void) gobj;
}

void ftParts_80074A4C(Fighter_GObj* gobj, int model_idx, int val)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x5F4_arr[model_idx].prev = (s8) val;
    fp->x221D_b2 = true;
}

void ftParts_80074A8C(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    f->modelIdx = f->fighter.x5F4_arr[0].prev;
    f->fighter.x5F4_arr[0].idx = f->fighter.x5F4_arr[0].prev;
    f->fighter.x221D_b2 = false;
}

void ftParts_80074ACC(Fighter_GObj* gobj)
{
    M360Fighter* f = Owner(gobj);
    f->fighter.x5F4_arr[0].idx = -1;
    f->modelIdx = -1;
    f->fighter.x221D_b2 = false;
}

void ftParts_80074B0C(Fighter_GObj* gobj, int model_idx, int val)
{
    M360Fighter* f = Owner(gobj);
    if (model_idx == 0) {
        f->modelIdx = val;
        f->fighter.x5F4_arr[0].idx = (s8) val;
    }
}

Fighter_Part ftParts_GetBoneIndex(Fighter* fp, Fighter_Part part)
{
    FighterPartsTable* table = (FighterPartsTable*) (uintptr_t) ftPartsTable[fp->kind].v;
    return (Fighter_Part) (s8) table->part_to_joint[part];
}

void ft_PlaySFX(Fighter* fp, enum_t sfx_id, u8 sfx_vol, u8 sfx_pan)
{
    (void) fp;
    M360_AudioSfx((unsigned) sfx_id, sfx_vol, sfx_pan);
}

/* lbAudioAx_80023870: fighter voice/SFX tracks. Key-off requests (0x83D61)
 * and the list terminator (0x83D60) have no native voice to stop; other ids
 * go to the SSM player. The track number serves as the handle. */
int lbAudioAx_80023870(int id, int vol, int pan, int track)
{
    if (id == 0x83D60 || id == 0x83D61)
        return -1;
    M360_AudioSfx((unsigned) id, (unsigned) vol, (unsigned) pan);
    return track;
}

static void ProcHit(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const float kb = fp->dmg.kb_applied;
    const float damage = fp->dmg.x1838_percentTemp;
    Fighter_ProcessHit_8006D1EC(gobj);
    if (kb) {
        ++s_hitCount;
        M360_MatchTrace("fighter.hit.player", fp->player_id);
        M360_MatchTrace("fighter.hit.damage", (unsigned) damage);
        M360_MatchTrace("fighter.hit.knockback_x100", (unsigned) (kb * 100.0f));
        M360_MatchTrace("fighter.hit.reaction_motion", (unsigned) fp->motion_id);
        M360_MatchTrace("fighter.hit.hitlag_frames", (unsigned) fp->dmg.x195c_hitlag_frames);
        M360_MatchTrace("fighter.hitstun.frames", (unsigned) fp->mv.co.damage.x0);
        M360_MatchTrace("fighter.hit.target_percent", (unsigned) fp->dmg.x1830_percent);
        M360_MatchTrace("fighter.hit.kb_vel_x100_x", (unsigned) (int) (fp->x8c_kb_vel.x * 100.0f));
        M360_MatchTrace("fighter.hit.kb_vel_x100_y", (unsigned) (int) (fp->x8c_kb_vel.y * 100.0f));
    }
}


static void ProcFinish(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    M360Fighter* f = Owner(gobj);
    if (f->port == 0 && f->traceMotion != (int) fp->motion_id) {
        f->traceMotion = (int) fp->motion_id;
        M360_MatchTrace("fighter.p1.motion", (unsigned) fp->motion_id);
    }
    HSD_JObjSetRotationY(gobj->hsd_obj, (float) M_PI_2 * fp->facing_dir);
    HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
}

static void OnDeath(HSD_GObj* gobj)
{
    const M360KindDesc* desc = KindDesc(GET_FIGHTER(gobj)->kind, NULL);
    if (desc->onDeath)
        desc->onDeath(gobj);
}

static void RemoveUserData(void* data)
{
    (void) data;
}

void* M360_FighterSpawn(int slot, float x, float y, float facing, int port)
{
    M360Fighter* f;
    Fighter* fp;
    HSD_GObj* gobj;
    HSD_JObj* root;
    Vec3 scale;
    const M360KindDesc* desc;
    M360LoadedKind* kind;
    int costume;
    int i;
    if (slot < 0 || slot >= kMaxFighters)
        return NULL;
    kind = LoadKind(s_selectKind[slot]);
    if (!kind) {
        M360_MatchTrace("fighter.kind.fallback", s_selectKind[slot]);
        s_selectKind[slot] = 0;
        kind = LoadKind(0);
    }
    desc = &s_kinds[s_selectKind[slot]];
    costume = kind ? LoadCostume(kind, desc, s_selectCostume[slot]) : -1;
    if (costume < 0)
        return NULL;
    f = &s_fighters[slot];
    memset(f, 0, sizeof(*f));
    fp = &f->fighter;
    gobj = GObj_Create(HSD_GOBJ_CLASS_FIGHTER, 8, 0);
    root = HSD_JObjLoadJoint(kind->costumeJoint[costume]);
    HSD_JObjAddAnimAll(root, NULL, kind->costumeMatAnim[costume], NULL);
    HSD_JObjReqAnimAll(root, 0.0f);
    HSD_JObjAnimAll(root);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, root);
    GObj_InitUserData(gobj, 4, RemoveUserData, f);
    GObj_SetupGXLink(gobj, FighterRender, kLinkFighter, 0);
    f->gobj = gobj;
    f->port = port;
    CollectJoints(f, root, kind->costumeJoint[costume]);
    for (i = 0; i < (int) f->jointCount; ++i) {
        f->parts[i].joint = f->joints[i];
        f->parts[i].x4_jobj2 = f->joints[i];
        f->parts[i].flags_b1 = true;
    }
    fp->parts = f->parts;
    for (i = 0; i < 4; ++i) {
        DiscU32(*table)[4] = (DiscU32(*)[4]) kind->data->x8->x0.vis_table;
        f->vis[i] = (FtPartsVisLookup*) (uintptr_t) table[0][i].v;
    }
    SetVisGroup(f, 0, -1);
    SetVisGroup(f, 1, -1);
    fp->ft_data = kind->data;
    fp->co_attrs = *kind->data->x0;
    fp->x24 = kind->data->xC;
    fp->x28 = (u8(*)[2]) kind->data->x10;
    fp->kind = desc->kind;
    fp->x597_bits = desc->kind;
    fp->x619_costume_id = (u8) costume;
    fp->player_id = (u8) slot;
    fp->x618_player_id = (u8) (port < 0 ? 0 : port);
    fp->x61A_controller_index = (u8) slot;
    fp->team = (u8) slot;
    fp->cpu.kind = 0;
    s_playerCpu[slot] = port < 0 ? Gm_PKind_Cpu : Gm_PKind_Human;
    s_playerPos[slot].x = x;
    s_playerPos[slot].y = y;
    s_playerPos[slot].z = 0.0f;
    s_playerFacing[slot] = facing;
    f->traceMotion = -1;
    fp->x34_scale.x = fp->x34_scale.y = fp->x34_scale.z = 1.0f;
    fp->x18 = ftCo_MS_Count;
    fp->x1C_actionStateList = s_commonStates;
    fp->x20_actionStateList = kind->states;
    fp->anim_id = -1;
    fp->gobj = gobj;
    fp->dat_attrs_backup = f->datAttrs;
    fp->x890_cameraBox = &f->cameraSubject;
    if ((s8) ftData_UnkBytePerCharacter[desc->kind] >= 0)
        efAsync_LoadSync(ftData_UnkBytePerCharacter[desc->kind]);
    if (desc->onLoad)
        desc->onLoad(gobj);
    ftCo_800A101C(fp, 4, (int) s_cpuLevel, 0);
    fp->x21FC_flag.byte = 1;
    fp->smash_attrs.x2135 = -1;
    fp->coll_data.floor.index = -1;
    fp->coll_data.floor_skip = -1;
    for (i = 0; i < (int) ARRAY_SIZE(fp->x8B0); ++i) {
        fp->x8B0[i].x10 = -1;
        fp->x8B0[i].x11 = -1;
    }
    scale.x = scale.y = scale.z = ModelScale(fp);
    HSD_JObjSetScale(root, &scale);
    f->modelIdx = 0;
    ftParts_80074A4C(gobj, 0, 0);
    ftColl_8007B320(gobj);
    HSD_GObj_SetupProc(gobj, Fighter_8006A1BC, 0);
    HSD_GObj_SetupProc(gobj, Fighter_8006A360, 1);
    HSD_GObj_SetupProc(gobj, Fighter_8006ABA0, 2);
    HSD_GObj_SetupProc(gobj, Fighter_Spaghetti_8006AD10, 3);
    HSD_GObj_SetupProc(gobj, Fighter_procUpdate, 4);
    HSD_GObj_SetupProc(gobj, Fighter_procMap, 6);
    HSD_GObj_SetupProc(gobj, Fighter_8006C80C, 9);
    HSD_GObj_SetupProc(gobj, Fighter_8006CB94, 0xD);
    HSD_GObj_SetupProc(gobj, ProcHit, 0xE);
    HSD_GObj_SetupProc(gobj, ProcFinish, 0x11);
    M360_FighterRespawn(gobj, x, y);
    ProcFinish(gobj);
    M360_MatchTrace("fighter.spawn.joints", f->jointCount);
    M360_MatchTrace("fighter.spawn.dobjs", f->dobjCount);
    return gobj;
}

void M360_FighterSetDead(void* handle)
{
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    Owner(gobj)->dead = 1;
    fp->x221F_b3 = true;
    ftColl_8007AFF8(gobj);
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    M360_MatchTrace("fighter.dead.player", fp->player_id);
}

void M360_FighterRespawn(void* handle, float x, float y)
{
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    Owner(gobj)->dead = 0;
    fp->x221F_b3 = false;
    s_playerPos[fp->player_id].x = x;
    s_playerPos[fp->player_id].y = y;
    Fighter_UnkInitReset_80067C98(fp);
    Fighter_ResetInputData_80068854(gobj);
    OnDeath(gobj);
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    fp->gr_vel = 0.0f;
    fp->x1968_jumpsUsed = 1;
    fp->ground_or_air = GA_Air;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.last_pos = fp->cur_pos;
    fp->coll_data.floor.index = -1;
    fp->coll_data.floor_skip = -1;
    ftCo_Fall_Enter(gobj);
}

/* Reduced Fighter_UnkProcessDeath_80068354: the per-life reset the rebirth
 * path needs, without CPU, item, stale-move or transformation state. */
void Fighter_UnkProcessDeath_80068354(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_UnkInitReset_80067C98(fp);
    HSD_JObjSetTranslate(GET_JOBJ(gobj), &fp->cur_pos);
    ftCommon_8007D5D4(fp);
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0f;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0f;
    fp->gr_vel = 0.0f;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.last_pos = fp->cur_pos;
    fp->coll_data.floor.index = -1;
    fp->coll_data.floor_skip = -1;
    ftColl_8007AFF8(gobj);
    ftColl_8007B0C0(gobj, HurtCapsule_Enabled);
    OnDeath(gobj);
    /* Player cpu_kind 4 is the VS CPU (gm_1601.c). */
    ftCo_800A101C(fp, 4, (int) s_cpuLevel, 0);
}

/* fn_8016719C: rebirth above the stage's first rebirth point, offset 16
 * units per player, entering from the camera top on the original platform. */
void M360_FighterRebirth(void* handle)
{
    static const float kOffset[6] = { 0.0f, 1.0f, -1.0f, 2.0f, 0.0f, 0.0f };
    HSD_GObj* gobj = handle;
    Fighter* fp = GET_FIGHTER(gobj);
    const M360MatchStage* st = M360_MatchStageData();
    const int slot = fp->player_id < 6 ? fp->player_id : 0;
    M360Fighter* f = Owner(gobj);
    f->dead = 0;
    fp->x221F_b3 = false;
    if (fp->x20A0_accessory) {
        HSD_JObjRemoveAll(fp->x20A0_accessory);
        fp->x20A0_accessory = NULL;
    }
    s_rebirthOffset[slot].x = 16.0f * kOffset[slot];
    s_rebirthOffset[slot].y = s_rebirthOffset[slot].z = 0.0f;
    s_playerPos[slot].x = st->rebirthX[0] + s_rebirthOffset[slot].x;
    s_playerPos[slot].y = st->camTop + st->camY;
    s_playerPos[slot].z = 0.0f;
    s_playerFacing[slot] = s_playerPos[slot].x >= 0.0f ? -1.0f : 1.0f;
    fp->facing_dir = s_playerFacing[slot];
    Fighter_ResetInputData_80068854(gobj);
    fp->x1968_jumpsUsed = 1;
    ftCo_800D4FF4(gobj);
    M360_MatchTrace("fighter.rebirth.player", fp->player_id);
}

s32 Player_80032F30(s32 slot)
{
    (void) slot;
    return 0;
}

void Stage_80224E38(Vec3* out, s32 index)
{
    const M360MatchStage* st = M360_MatchStageData();
    const int i = index >= 0 && index < 4 ? index : 0;
    out->x = st->rebirthX[i];
    out->y = st->rebirthY[i];
    out->z = 0.0f;
}

void Player_GetSomePos(s32 slot, Vec3* out)
{
    *out = s_rebirthOffset[slot < 6 && slot >= 0 ? slot : 0];
}

void Player_GetSpawnPlatformPos(s32 slot, Vec3* out)
{
    Stage_80224E38(out, 0);
    out->x += s_rebirthOffset[slot < 6 && slot >= 0 ? slot : 0].x;
}


void ftCamera_80076018(UnkFloat6_Camera* in, UnkFloat6_Camera* out, float mul)
{
    out->x0.x = in->x0.x * mul;
    out->x0.y = in->x0.y * mul;
    out->x0.z = in->x0.z * mul;
    out->xC.x = in->xC.x * mul;
    out->xC.y = in->xC.y * mul;
    out->xC.z = in->xC.z * mul;
}

s32 Player_GetDamage(s32 slot)
{
    (void) slot;
    return 0;
}

void Player_LoadPlayerCoords(s32 slot, Vec3* out_vec)
{
    *out_vec = s_playerPos[slot < 6 && slot >= 0 ? slot : 0];
}

f32 Player_GetFacingDirection(s32 slot)
{
    return s_playerFacing[slot < 6 && slot >= 0 ? slot : 0];
}

Gm_PKind Player_8003248C(s32 slot, bool arg1)
{
    (void) arg1;
    return (Gm_PKind) s_playerCpu[slot < 6 && slot >= 0 ? slot : 0];
}

f32 Player_GetAttackRatio(int slot)
{
    (void) slot;
    return 1.0f;
}

f32 Player_GetDefenseRatio(int slot)
{
    (void) slot;
    return 1.0f;
}

void M360_FighterSetCpuLevel(unsigned level)
{
    s_cpuLevel = level;
}

unsigned M360_MatchPadTriggered(void)
{
    return HSD_PadGameStatus[0].trigger;
}

unsigned M360_MatchPadTriggeredPort(unsigned port)
{
    return port < 4 ? HSD_PadGameStatus[port].trigger : 0;
}

void M360_FighterSetPort(void* handle, int port)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    Owner((HSD_GObj*) handle)->port = port;
    fp->x618_player_id = (u8) (port < 0 ? 0 : port);
    s_playerCpu[fp->player_id] = port < 0 ? Gm_PKind_Cpu : Gm_PKind_Human;
}

unsigned M360_MatchPadHeld(void)
{
    return HSD_PadGameStatus[0].button;
}

float M360_MatchPadX(void)
{
    return HSD_PadGameStatus[0].nml_stickX;
}

float M360_MatchPadStickXPort(unsigned port)
{
    return port < 4 ? HSD_PadGameStatus[port].nml_stickX : 0.0f;
}

float M360_MatchPadSubStickYPort(unsigned port)
{
    return port < 4 ? HSD_PadGameStatus[port].nml_subStickY : 0.0f;
}

/* Slot of the last fighter that hit this one (ftColl_8007861C), -1 if none. */
int M360_FighterLastAttacker(void* handle)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    const s32 ply = fp->dmg.x18c4_source_ply;
    return ply >= 0 && ply < 6 && ply != (s32) fp->player_id ? (int) ply : -1;
}

float M360_MatchPadY(void)
{
    return HSD_PadGameStatus[0].nml_stickY;
}

int M360_MatchControllerConnected(unsigned port)
{
    return port < 4 && HSD_PadGameStatus[port].err >= 0;
}

unsigned M360_FighterKindIndex(void* handle)
{
    const FighterKind kind = GET_FIGHTER((HSD_GObj*) handle)->kind;
    unsigned i;
    for (i = 0; i < kKindCount; ++i)
        if (s_kinds[i].kind == kind)
            return i;
    return 0;
}

void M360_FighterGetState(void* handle, float* x, float* y, float* facing,
                          unsigned* motion, unsigned* damage)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    *x = fp->cur_pos.x;
    *y = fp->cur_pos.y;
    *facing = fp->facing_dir;
    *motion = (unsigned) fp->motion_id;
    *damage = (unsigned) (fp->dmg.x1830_percent + 0.5f);
}

void M360_FighterCameraBox(void* handle, float* x, float* y, float* left,
                           float* right, float* up, float* down)
{
    Fighter* fp = GET_FIGHTER((HSD_GObj*) handle);
    UnkFloat6_Camera* box = fp->ft_data->x3C;
    const float s = fp->x34_scale.y;
    const float zoom = M360_MatchFixedZoom();
    *x = fp->cur_pos.x;
    *y = fp->cur_pos.y + box->x0.x * s;
    if (fp->facing_dir == 1.0f) {
        *left = box->x0.z * s;
        *right = box->x0.y * s * zoom;
    } else {
        *left = -box->x0.y * s * zoom;
        *right = -box->x0.z * s;
    }
    *up = box->xC.x * s;
    *down = box->xC.y * s;
}

unsigned M360_FighterHitCount(void)
{
    return s_hitCount;
}

