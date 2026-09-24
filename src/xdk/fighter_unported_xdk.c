#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <dolphin/mtx.h>
#include <melee/cm/camera.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/db/db.h>
#include <melee/ft/ftdata.h>
#include <melee/gr/stage.h>
#include <melee/gr/ground.h>
#include <melee/gm/gm_18A1.h>
#include <melee/lb/lblanguage.h>
#include <sysdolphin/baselib/mobj.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/if/ifmagnify.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_279C.h>
#include <melee/it/it_3F14.h>
#include <melee/it/itcoll.h>
#include <melee/lb/lb_0146.h>
#include <melee/lb/lbrefract.h>
#include <melee/mp/mplib.h>
#include <melee/pl/pl_040D.h>
#include <melee/pl/pltrick.h>
#include <melee/sfx/crowdsfx.h>
#include <melee/ef/eflib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0C60.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0899.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0C88.h>
#include <melee/ft/ft_0D27.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftdynamics.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftmetal.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftswing.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F7.h>
#include <melee/ft/kinds/ftCommon/ftCo_AirCatch.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackDash.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS4.h>
#include <melee/ft/kinds/ftCommon/ftCo_Damage.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownSpot.h>
#include <melee/ft/kinds/ftCommon/ftCo_Escape.h>
#include <melee/ft/kinds/ftCommon/ftCo_EscapeAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerFall.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerJump.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerLanding.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemScrew.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialS.h>
#include <melee/ft/kinds/ftCommon/ftCo_Squat.h>
#include <melee/ft/kinds/ftCommon/ftCo_SquatWait.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCrazyHand/ftcrazyhandtagcancel.h>
#include <melee/ft/kinds/ftFox/ftfoxappeals.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack11.h>
#include <melee/ft/kinds/ftLink/ftlinkattackair.h>
#include <melee/ft/kinds/ftMasterHand/ftmasterhandwait12.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloat.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/gm/gmvs.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <melee/it/kinds/it_2E5A.h>
#include <melee/it/kinds/itfflowerflame.h>
#include <melee/it/kinds/itparasol.h>
#include <melee/it/kinds/itpeachparasol.h>
#include <melee/it/kinds/itrabbitc.h>
#include <melee/it/kinds/itsword.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_0219.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mplib.h>
#include <melee/pl/plbonuslib.h>
#include <melee/pl/player.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/rumble.h>
#pragma warning(pop)
#pragma warning(disable : 4100)

#include "match_xdk.h"

void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);

Fighter_ItemEvent ftData_OnItemDrop[Ft_Kind_Max];
HSD_GObjEvent ftData_OnItemDropExt[Ft_Kind_Max];
HSD_GObjEvent ftData_OnItemInvisible[Ft_Kind_Max];
HSD_GObjEvent ftData_OnItemPickup[Ft_Kind_Max];
HSD_GObjEvent ftData_OnItemVisible[Ft_Kind_Max];
HSD_GObjEvent ftData_OnKnockbackEnter[Ft_Kind_Max];
HSD_GObjEvent ftData_OnKnockbackExit[Ft_Kind_Max];
HSD_GObjEvent ftData_UnkMotionStates1[Ft_Kind_Max];
HSD_GObjEvent ftData_UnkMotionStates2[Ft_Kind_Max];
HSD_GObjEvent ftKindCalcIndiviParamTable[Ft_Kind_Max];

static unsigned s_reported[128];
static unsigned s_reportedCount;

static void Report(const char* name)
{
    unsigned i;
    const unsigned key = (unsigned) (uintptr_t) name;
    for (i = 0; i < s_reportedCount; ++i)
        if (s_reported[i] == key)
            return;
    if (s_reportedCount < 128)
        s_reported[s_reportedCount++] = key;
    M360_MatchTrace(name, 1);
}

#define UNPORTED_BOOL(name, params) \
    bool name params { Report("fighter.unported." #name); return false; }
#define UNPORTED_VOID(name, params) \
    void name params { Report("fighter.unported." #name); }
#define SILENT_VOID(name, params) \
    void name params { }

UNPORTED_VOID(ftMh_MS_389_80151018, (HSD_GObj* gobj))
UNPORTED_VOID(ftCh_GrabUnk1_8015BC88, (HSD_GObj* gobj))
UNPORTED_VOID(ftCo_800C8540, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C8348, (Fighter_GObj* gobj, int timer, int health))
UNPORTED_BOOL(ft_80084BFC, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_BOOL(ft_80084C38, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_VOID(ftAnim_ApplyPartAnim, (Fighter_GObj* gobj, s32 a, s32 b, float c))
UNPORTED_VOID(ftAnim_800704F0, (Fighter_GObj* gobj, int tobj_idx, float frame))
UNPORTED_BOOL(Player_8003544C, (s32 slot, bool condition))

SILENT_VOID(ft_800880D8, (Fighter* fp))
SILENT_VOID(ft_8008A1B8, (Fighter_GObj* gobj, u32 flags))
SILENT_VOID(ftCamera_80076064, (Fighter* fp))
SILENT_VOID(ftCo_8009E7B4, (Fighter* fp, u8 (*blend)[2]))
SILENT_VOID(ftCo_8009F834, (Fighter_GObj* gobj, int a, Fighter_Part b, int c, int d, Vec3* e, Vec3* f, float g))
SILENT_VOID(HSD_PadRumbleRemoveId, (u8 a, int b))
SILENT_VOID(lb_80014574, (u8 a, int b, int c, int d))
SILENT_VOID(lb_800145C0, (u8 slot))
SILENT_VOID(lbBgFlash_80021C48, (u32 a, u32 b))
SILENT_VOID(pl_8003EA08, (int slot, int a))
SILENT_VOID(pl_8003FC44, (int slot, int a))
SILENT_VOID(pl_800402D0, (int slot, int a, int b))
SILENT_VOID(pl_80040460, (int slot, int a))
SILENT_VOID(Player_SetHPByIndex, (s32 slot, s32 a, s32 b))
SILENT_VOID(Player_SetUnk98, (s32 slot, s32 a))
SILENT_VOID(Player_UpdateJoystickCountByIndex, (s32 slot, s32 index))
SILENT_VOID(un_80322314, (void))

s32 Player_GetUnk98(s32 slot) { (void) slot; return 0; }
bool gm_8016B0B4(void) { return false; }
bool un_803224DC(s32 spawn_id, f32 pos_x, f32 kb_mag) { (void) spawn_id; (void) pos_x; (void) kb_mag; return false; }
HSD_GObj* lbAudioAx_800263E8(float a, HSD_GObj* b, int c, int d, int e, int f, int g, int h, int i, int j, int k)
{
    (void) a; (void) b; (void) c; (void) d; (void) e; (void) f; (void) g; (void) h; (void) i; (void) j; (void) k;
    return NULL;
}
int lbAudioAx_800264E4(HSD_GObj* data) { (void) data; return 0; }
s32 ft_80087D0C(Fighter* fp, s32 a) { (void) fp; return a; }
enum_t ftCo_8009E318(Fighter_GObj* gobj, Fighter_Part part, float f) { (void) gobj; (void) part; (void) f; return 0; }
bool ftCo_8009E714(Fighter_GObj* gobj, Fighter_Part part, int a, float x, float y, float b, float c, float d)
{
    (void) gobj; (void) part; (void) a; (void) x; (void) y; (void) b; (void) c; (void) d;
    return false;
}
int mpLineGetNext(int line_id) { (void) line_id; return -1; }
int mpLineGetPrev(int line_id) { (void) line_id; return -1; }





struct ftData_80085FD4_ret* ftData_80085FD4(Fighter* fp, FtMotionId msid)
{
    return (struct ftData_80085FD4_ret*) &fp->x24[msid];
}

void ftPartSetRotZ(Fighter* fp, int part_idx, f32 rotate_z)
{
    if (fp->parts && fp->parts[part_idx].joint)
        HSD_JObjSetRotationZ(fp->parts[part_idx].joint, rotate_z);
}

HSD_GObjEvent ftData_SpecialN[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialHi[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialLw[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialAirHi[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialS[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialAirN[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialAirS[Ft_Kind_Max];
HSD_GObjEvent ftData_SpecialAirLw[Ft_Kind_Max];

UNPORTED_VOID(pl_8003E854, (int a, int b, Item_GObj* c))
UNPORTED_BOOL(ftCo_800C60C8, (Fighter_GObj* gobj))
bool gm_8016B0FC(void) { return false; }

DbLKind DbLevel;
HSD_GObjEvent ftData_UnkMotionStates3[Ft_Kind_Max];
HSD_GObjEvent ftData_UnkMotionStates4[Ft_Kind_Max];
HSD_GObjEvent ftData_OnAbsorb[Ft_Kind_Max];

void ftPartSetRotX(Fighter* fp, int part_idx, f32 rotate_x)
{
    if (fp->parts && fp->parts[part_idx].joint)
        HSD_JObjSetRotationX(fp->parts[part_idx].joint, rotate_x);
}

void ftPartSetRotY(Fighter* fp, int part_idx, f32 rotate_y)
{
    if (fp->parts && fp->parts[part_idx].joint)
        HSD_JObjSetRotationY(fp->parts[part_idx].joint, rotate_y);
}

void ftParts_JObjSetRotation(HSD_JObj* jobj, Quaternion* rotation)
{
    if (jobj)
        HSD_JObjSetRotation(jobj, rotation);
}


f32 gm_8016B248(void)
{
    return 1.0f;
}

float sqrtf_accurate(float x)
{
    return sqrtf(x);
}

void PSMTXTranspose(Mtx src, Mtx xPose)
{
    Mtx tmp;
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            tmp[i][j] = src[j][i];
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j)
            xPose[i][j] = tmp[i][j];
        xPose[i][3] = 0.0f;
    }
}

float Stage_GetCamBoundsBottomOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->camBottom + st->camY;
}

float Stage_CalcUnkCamY(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return 0.5f * ((st->blastBottom + st->camY) + (st->camBottom + st->camY));
}

float Stage_CalcUnkCamYBounds(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return 0.25f * (st->blastBottom + st->camY) + 0.75f * (st->camBottom + st->camY);
}

void Stage_UnkSetVec3TCam_Offset(Vec3* out)
{
    out->x = out->y = out->z = 0.0f;
}

bool ftAnim_80070FD0(Fighter* fp)
{
    (void) fp;
    return false;
}

float Camera_80031144(void)
{
    return 0.0f;
}

void GXProject(f32 x, f32 y, f32 z, Mtx mtx, f32* pm, f32* vp, f32* sx, f32* sy, f32* sz)
{
    (void) x; (void) y; (void) z; (void) mtx; (void) pm; (void) vp;
    *sx = *sy = *sz = 0.0f;
}

void Camera_RequestQuake(CmQuakeKind a0, Vec* a1) { (void) a0; (void) a1; Report("fighter.unported.Camera_RequestQuake");  }
void pl_80037C60(Fighter_GObj* a0, volatile s32 a1) { (void) a0; (void) a1; Report("fighter.unported.pl_80037C60");  }
bool gm_8016B1C4(void) {  Report("fighter.unported.gm_8016B1C4"); return (bool) 0; }
void ft_800C80A4(Fighter* a0) { (void) a0; Report("fighter.unported.ft_800C80A4");  }
bool ftCo_800C7CA0(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C7CA0"); return (bool) 0; }
void un_8032233C(u32 a0, u32 a1) { (void) a0; (void) a1; Report("fighter.unported.un_8032233C");  }
float un_803222EC(float a0, float a1) { (void) a0; (void) a1; Report("fighter.unported.un_803222EC"); return 0.0f; }
bool un_80322258(float a0) { (void) a0; Report("fighter.unported.un_80322258"); return (bool) 0; }
void pl_80041B08(int a0, UNK_T a1, u16 a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_80041B08");  }
void pl_80041280(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_80041280");  }
void pl_800411C4(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_800411C4");  }
void pl_80040B8C(int a0, int a1, int a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_80040B8C");  }
void pl_80040330(int a0, int a1, float a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_80040330");  }
void pl_80040270(int a0, int a1, float a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_80040270");  }
void pl_800401F0(int a0, int a1, float a2, float a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.pl_800401F0");  }
void pl_8003FE1C(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_8003FE1C");  }
void pl_8003EC9C(int a0, int a1, float a2, float a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.pl_8003EC9C");  }
void pl_8003EB30(float a0, int a1, int a2, int a3, int a4, int a5) { (void) a0; (void) a1; (void) a2; (void) a3; (void) a4; (void) a5; Report("fighter.unported.pl_8003EB30");  }
void pl_8003E17C(int a0, int a1, Item_GObj* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_8003E17C");  }
void pl_8003E150(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_8003E150");  }
void pl_8003E058(int a0, int a1, int a2, int a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.pl_8003E058");  }
bool pl_8003D60C(int a0) { (void) a0; Report("fighter.unported.pl_8003D60C"); return (bool) 0; }
void pl_80038144(HSD_GObj* a0, HSD_GObj* a1, s32 a2, ft_800898B4_t* a3, u16 a4, s32 a5, s32 a6) { (void) a0; (void) a1; (void) a2; (void) a3; (void) a4; (void) a5; (void) a6; Report("fighter.unported.pl_80038144");  }
void pl_80037ECC(HSD_GObj* a0) { (void) a0; Report("fighter.unported.pl_80037ECC");  }
u16 plAttack_80037B08(void) {  Report("fighter.unported.plAttack_80037B08"); return (u16) 0; }
bool mpGetSpeed(int a0, Vec3* a1, Vec3* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.mpGetSpeed"); return (bool) 0; }
bool lb_80014638(struct lb_80014638_arg0_t* a0, struct lb_80014638_arg1_t* a1) { (void) a0; (void) a1; Report("fighter.unported.lb_80014638"); return (bool) 0; }
bool lb_800144C8(ColorOverlay* a0, struct Fighter_804D653C_t* a1, int a2, int a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.lb_800144C8"); return (bool) 0; }
void lb_80014498(ColorOverlay* a0) { (void) a0; Report("fighter.unported.lb_80014498");  }
bool lb_80014258(Fighter_GObj* a0, void* a1, FtCmd2 a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.lb_80014258"); return (bool) 0; }
void lbRefract_80022BB8(void) {  Report("fighter.unported.lbRefract_80022BB8");  }
void lbRefract_800225D4(void) {  Report("fighter.unported.lbRefract_800225D4");  }
void lbRefSetUnuse(void) {  Report("fighter.unported.lbRefSetUnuse");  }
int lbAudioAx_80024184(int a0, int a1, int a2, int a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.lbAudioAx_80024184"); return (int) 0; }
bool ifMagnify_802FC998(s32 a0) { (void) a0; Report("fighter.unported.ifMagnify_802FC998"); return (bool) 0; }
s32 ifMagnify_802FB6E8(s32 a0) { (void) a0; Report("fighter.unported.ifMagnify_802FB6E8"); return (s32) 0; }
bool gm_IsCurrently1PMode_inline(void) {  Report("fighter.unported.gm_IsCurrently1PMode_inline"); return (bool) 0; }
bool gm_GetDbPauseFlag(int a0) { (void) a0; Report("fighter.unported.gm_GetDbPauseFlag"); return (bool) 0; }
u32 gm_801A4BB8(void) {  Report("fighter.unported.gm_801A4BB8"); return (u32) 0; }
bool gm_8016B168(void) {  Report("fighter.unported.gm_8016B168"); return (bool) 0; }
bool gm_8016B0D4(void) {  Report("fighter.unported.gm_8016B0D4"); return (bool) 0; }
bool gm_8016B014(void) {  Report("fighter.unported.gm_8016B014"); return (bool) 0; }
void gm_80167470(s32 a0, s32 a1) { (void) a0; (void) a1; Report("fighter.unported.gm_80167470");  }
void gm_80167320(int a0, bool a1) { (void) a0; (void) a1; Report("fighter.unported.gm_80167320");  }
void ft_800C85B8(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ft_800C85B8");  }
void ft_800C8170(Fighter* a0) { (void) a0; Report("fighter.unported.ft_800C8170");  }
void ft_80089B08(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ft_80089B08");  }
void ft_80088080(Fighter* a0) { (void) a0; Report("fighter.unported.ft_80088080");  }
void ftSb_Init_8014FBA4(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftSb_Init_8014FBA4");  }
void ftPartsPObjSetDefaultClass(void) {  Report("fighter.unported.ftPartsPObjSetDefaultClass");  }
void ftPartsPObjClearDefaultClass(void) {  Report("fighter.unported.ftPartsPObjClearDefaultClass");  }
void ftMh_MS_343_80151484(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftMh_MS_343_80151484");  }
void ftMh_MS_341_8014FE58(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftMh_MS_341_8014FE58");  }
bool ftCo_800D3158(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800D3158"); return (bool) 0; }
bool ftCo_800C74F4(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C74F4"); return (bool) 0; }
void ftCo_800C703C(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C703C");  }
void ftCo_800C2FD8(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C2FD8");  }
void ftCh_Init_80156014(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftCh_Init_80156014");  }
void ftCh_GrabUnk1_8015ADD0(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftCh_GrabUnk1_8015ADD0");  }
void ftAnim_80070F28(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070F28");  }
void ftAnim_80070E74(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070E74");  }
void ftAnim_80070654(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070654");  }
void Player_SetTotalCoins(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.Player_SetTotalCoins");  }
void Player_SetCoins(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.Player_SetCoins");  }
int Player_GetTotalCoins(int a0) { (void) a0; Report("fighter.unported.Player_GetTotalCoins"); return (int) 0; }
s32 Player_GetRemainingHPByIndex(s32 a0, s32 a1) { (void) a0; (void) a1; Report("fighter.unported.Player_GetRemainingHPByIndex"); return (s32) 0; }
bool Player_GetMoreFlagsBit3(s32 a0) { (void) a0; Report("fighter.unported.Player_GetMoreFlagsBit3"); return (bool) 0; }
bool Player_GetMoreFlagsBit0(s32 a0) { (void) a0; Report("fighter.unported.Player_GetMoreFlagsBit0"); return (bool) 0; }
int Player_GetCoins(int a0) { (void) a0; Report("fighter.unported.Player_GetCoins"); return (int) 0; }

/* Shield, dodge and special-fall dependencies. Item tosses and grabs out of
 * shield stay unavailable until items/grabs are integrated; the Yoshi and
 * Sheik branches are unreachable with the current Mario-only roster. */
SILENT_VOID(pl_8003E0E8, (int slot, int sub))
SILENT_VOID(pl_8003E114, (int slot, int sub, float health))
u32 Player_GetUnk45(s32 slot) { (void) slot; return 0; }
int un_80322598(int spawn_id, float y) { (void) spawn_id; (void) y; return 0; }

/* Grab/throw dependencies outside the Mario-vs-Mario match: item grabs,
 * tether/hookshot items, cargo throws, Kirby/Yoshi/Samus/Mewtwo/Fox hooks. */
UNPORTED_VOID(Fighter_UpdateModelScale, (Fighter_GObj* gobj))
SILENT_VOID(ftCamera_800762F4, (HSD_GObj* gobj))
SILENT_VOID(pl_80040614, (int slot, bool sub, float damage))
/* Standing rank (gm_8016C5C0) and handicap: tied players at the default
 * VS handicap of 9 (gm_1601.c). */
s32 Player_80033BB8(int slot) { (void) slot; return 0; }
int Player_GetHandicap(int slot) { (void) slot; return 9; }
/* Link hookshot, Samus grapple and boomerang items used by the tether-grab
 * branches of ftCo_0D8E/ftCo_0D95; unreachable for Mario. */

/* Mario special-move items: fireball, Dr. Mario pill and cape. The moves
 * run their original motion states; the projectiles/cape objects wait for
 * the item system. */
SILENT_VOID(lb_800119DC, (Vec3* pos, int arg, float a, float b, float c))
SILENT_VOID(ftCo_800C7158, (Fighter_GObj* gobj))
SILENT_VOID(ftCo_800C7200, (Fighter_GObj* gobj))

/* Ledge bookkeeping without a gameplay effect in the quick match: stage
 * joint callbacks (static Battlefield) and ledge-grab statistics. */
SILENT_VOID(mpLib_8005811C, (CollData* coll, int ledge_id))
SILENT_VOID(pl_8003FFDC, (int a, int b, int c, int d, int e))
SILENT_VOID(pl_80040048, (int slot, int sub))
/* Taunt hooks for other characters and the taunt statistic. */
SILENT_VOID(pl_80040120, (int slot, int sub))
/* Rebirth/death bookkeeping: coin modes, dead-state entry and camera
 * helpers not used by the quick match. */
UNPORTED_VOID(ftCo_800D331C, (Fighter_GObj* gobj))
SILENT_VOID(ftCamera_80076320, (HSD_GObj* gobj))
SILENT_VOID(pl_80040374, (int slot, int sub))
s32 Player_GetUnk9C(s32 slot) { (void) slot; return 0; }
SILENT_VOID(Player_SetUnk9C, (s32 slot, s32 value))
float Stage_GetCamBoundsTopOffset(void)
{
    const M360MatchStage* st = M360_MatchStageData();
    return st->camTop + st->camY;
}
/* Luigi fireball and Dr. Mario pill items. */


/* Item hooks of the added characters (Bowser's flame, item throws). */
bool mpLib_80056C54(int line_id, Vec3* pos, int* line_id_out, Vec3* vec_out, u32* flags_out,
                    Vec3* normal_out, float a, float b)
{
    (void) line_id; (void) pos; (void) line_id_out; (void) vec_out; (void) flags_out;
    (void) normal_out; (void) a; (void) b;
    Report("fighter.unported.mpLib_80056C54");
    return false;
}

/* Projectile/held items and stage hooks of Fox, Falco, Pikachu, Pichu,
 * Samus, Link, Young Link, Mewtwo, Zelda and Sheik (generated from the
 * upstream prototypes by tools/host_xdk_check/gen_stubs.py). Spawners return
 * NULL, which the original callers treat as "no item". */
static plActionStats s_actionStats[6];
plActionStats* Player_GetActionStats(int slot) { return &s_actionStats[slot >= 0 && slot < 6 ? slot : 0]; }
void ftParts_800753D4(Fighter* a0, struct Fighter_804D6540_x0_t* a1, HSD_Joint* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftParts_800753D4"); }
void grCorneria_801E2AF4(void) {  Report("fighter.unported.grCorneria_801E2AF4"); }
bool grCorneria_801E2B80(void) {  Report("fighter.unported.grCorneria_801E2B80"); return (bool) 0; }
bool grCorneria_801E2C34(void) {  Report("fighter.unported.grCorneria_801E2C34"); return (bool) 0; }
bool grCorneria_801E2CE8(void) {  Report("fighter.unported.grCorneria_801E2CE8"); return (bool) 0; }
bool grCorneria_801E2D14(void) {  Report("fighter.unported.grCorneria_801E2D14"); return (bool) 0; }
unsigned int pl_800386D8(plActionStats* a0, ssize_t a1) { (void) a0; (void) a1; Report("fighter.unported.pl_800386D8"); return (unsigned int) 0; }

/* Peach, Yoshi, Ness and Mr. Game & Watch items plus Kirby copy hooks
 * (generated by tools/host_xdk_check/gen_stubs.py). */
void HSD_MObjReqAnim(HSD_MObj* mobj, f32 startframe) { (void) mobj; (void) startframe; Report("fighter.unported.HSD_MObjReqAnim"); }
void ftMaterial_800BFB4C(Fighter_GObj* gobj, GXColor* diffuse) { (void) gobj; (void) diffuse; Report("fighter.unported.ftMaterial_800BFB4C"); }
bool ft_80087988(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ft_80087988"); return (bool) 0; }

/* Jigglypuff costume hats and ground/game hooks; the roster only offers her
 * default costume, which has no hat. */
void ftCo_8009DC54(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009DC54"); }
void ftParts_8007487C(FtPartsDesc* a0, FtPartsVis* a1, u32 costume_id, DObjList* a3, DObjList* a4) { (void) a0; (void) a1; (void) costume_id; (void) a3; (void) a4; Report("fighter.unported.ftParts_8007487C"); }
void ftParts_80074CA0(FtPartsVis* a0, int a1, DObjList* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftParts_80074CA0"); }
void ftParts_80074D7C(FtPartsVis* a0, int a1, DObjList* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftParts_80074D7C"); }
void ftParts_80075650(Fighter_GObj* a0, HSD_JObj* a1, struct DObjList* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftParts_80075650"); }
bool gm_8016B1D8(void) {  Report("fighter.unported.gm_8016B1D8"); return (bool) 0; }
bool grStadium_801D4FF8(int pl_slot) { (void) pl_slot; Report("fighter.unported.grStadium_801D4FF8"); return (bool) 0; }
/* ftdata.c costume archive table; the native loader keeps costumes itself, so
 * it stays empty and only Jigglypuff's hat path (never taken with her default
 * costume) reads it. */
struct UnkCostumeList CostumeListsForeachCharacter[Ft_Kind_Max];

/* Item-system dependencies outside the quick match: Kirby copies, debug
 * displays, game-mode and stage hooks, camera subjects, effects and HSD
 * material/TEV state used by custom item rendering (generated by
 * tools/host_xdk_check/gen_stubs.py). */
void Camera_800290D4(CmSubject* a0) { (void) a0; Report("fighter.unported.Camera_800290D4"); }
HSD_GObj* Camera_80030A50(void) {  Report("fighter.unported.Camera_80030A50"); return (HSD_GObj*) 0; }
bool Camera_80030CD8(CmSubject* a0, S32Vec2* a1) { (void) a0; (void) a1; Report("fighter.unported.Camera_80030CD8"); return (bool) 0; }
bool Camera_80030CFC(CmSubject* a0, float a1) { (void) a0; (void) a1; Report("fighter.unported.Camera_80030CFC"); return (bool) 0; }
enum_t Camera_80031060(void) {  Report("fighter.unported.Camera_80031060"); return (enum_t) 0; }
bool Camera_80031154(Vec3* a0) { (void) a0; Report("fighter.unported.Camera_80031154"); return (bool) 0; }
void Camera_StopQuake(CmQuakeKind a0) { (void) a0; Report("fighter.unported.Camera_StopQuake"); }
u32 Ground_801C1D84(void) {  Report("fighter.unported.Ground_801C1D84"); return (u32) 0; }
void Ground_801C4338(void) {  Report("fighter.unported.Ground_801C4338"); }
bool Ground_801C4DA0(Vec3* a0, f32* a1) { (void) a0; (void) a1; Report("fighter.unported.Ground_801C4DA0"); return (bool) 0; }
s32 Ground_801C5840(void) {  Report("fighter.unported.Ground_801C5840"); return (s32) 0; }
void HSD_ClearVtxDesc(void) {  Report("fighter.unported.HSD_ClearVtxDesc"); }
void HSD_SetMaterialColor(GXColor ambient, GXColor diffuse, GXColor specular, f32 alpha) { (void) ambient; (void) diffuse; (void) specular; (void) alpha; Report("fighter.unported.HSD_SetMaterialColor"); }
void HSD_SetMaterialShininess(f32 shininess) { (void) shininess; Report("fighter.unported.HSD_SetMaterialShininess"); }
void HSD_SetupRenderModeWithCustomPE(u32 rendermode, HSD_PEDesc* pe) { (void) rendermode; (void) pe; Report("fighter.unported.HSD_SetupRenderModeWithCustomPE"); }
void HSD_SetupTevStage(HSD_TevDesc* a0) { (void) a0; Report("fighter.unported.HSD_SetupTevStage"); }
int HSD_StateAssignTev(void) {  Report("fighter.unported.HSD_StateAssignTev"); return (int) 0; }
void HSD_StateInitTev(void) {  Report("fighter.unported.HSD_StateInitTev"); }
void HSD_StateInvalidate(int mask) { (void) mask; Report("fighter.unported.HSD_StateInvalidate"); }
void HSD_TExpSetReg(HSD_TExp* texp) { (void) texp; Report("fighter.unported.HSD_TExpSetReg"); }
void HSD_TObjSetup(HSD_TObj* tobj) { (void) tobj; Report("fighter.unported.HSD_TObjSetup"); }
void HSD_TObjSetupTextureCoordGen(HSD_TObj* tobj) { (void) tobj; Report("fighter.unported.HSD_TObjSetupTextureCoordGen"); }
s8 Player_800325C8(CharacterKind a0, bool b) { (void) a0; (void) b; Report("fighter.unported.Player_800325C8"); return (s8) 0; }
bool Player_GetMoreFlagsBit4(s32 slot) { (void) slot; Report("fighter.unported.Player_GetMoreFlagsBit4"); return (bool) 0; }
void db_80225D64(Item_GObj* item, Fighter_GObj* owner) { (void) item; (void) owner; Report("fighter.unported.db_80225D64"); }
void db_80225DD8(Item_GObj* item, Fighter_GObj* owner) { (void) item; (void) owner; Report("fighter.unported.db_80225DD8"); }
s32 db_AreItemSpawnsEnabled(void) {  Report("fighter.unported.db_AreItemSpawnsEnabled"); return (s32) 0; }
s32 db_GetCurrentlySelectedPokemon(void) {  Report("fighter.unported.db_GetCurrentlySelectedPokemon"); return (s32) 0; }
u32 db_ShowCoinPickupRange(void) {  Report("fighter.unported.db_ShowCoinPickupRange"); return (u32) 0; }
u32 db_ShowEnemyStompRange(void) {  Report("fighter.unported.db_ShowEnemyStompRange"); return (u32) 0; }
u32 db_ShowItemPickupRange(void) {  Report("fighter.unported.db_ShowItemPickupRange"); return (u32) 0; }
bool ftCo_800BF228(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ftCo_800BF228"); return (bool) 0; }
void ftCo_800C7800(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ftCo_800C7800"); }
void ftCo_800C78B0(HSD_GObj* gobj, HSD_GObj* owner) { (void) gobj; (void) owner; Report("fighter.unported.ftCo_800C78B0"); }
void ftCo_800C7B0C(Fighter_GObj* gobj, Vec3* arg1, Vec3* arg2, lbColl_80008D30_arg1* arg3, f32 arg4) { (void) gobj; (void) arg1; (void) arg2; (void) arg3; (void) arg4; Report("fighter.unported.ftCo_800C7B0C"); }
void ftCo_800C7C60(Fighter_GObj* gobj, int damage_amount) { (void) gobj; (void) damage_amount; Report("fighter.unported.ftCo_800C7C60"); }
void ftData_80085560(int idx, int increment) { (void) idx; (void) increment; Report("fighter.unported.ftData_80085560"); }
void ftData_8008572C(FighterKind a0) { (void) a0; Report("fighter.unported.ftData_8008572C"); }
void ftData_800857E0(FighterKind a0) { (void) a0; Report("fighter.unported.ftData_800857E0"); }
void ftData_80085820(FighterKind a0, int costume_id) { (void) a0; (void) costume_id; Report("fighter.unported.ftData_80085820"); }
void ftData_80085A14(FighterKind a0) { (void) a0; Report("fighter.unported.ftData_80085A14"); }
bool gm_80164ABC(void) {  Report("fighter.unported.gm_80164ABC"); return (bool) 0; }
bool gm_80165084(void) {  Report("fighter.unported.gm_80165084"); return (bool) 0; }
s32 gm_8016C6C0(Item_GObj* a0) { (void) a0; Report("fighter.unported.gm_8016C6C0"); return (s32) 0; }
bool gm_80172BC4(void) {  Report("fighter.unported.gm_80172BC4"); return (bool) 0; }
bool gm_80172C04(void) {  Report("fighter.unported.gm_80172C04"); return (bool) 0; }
int gm_8017E068(void) {  Report("fighter.unported.gm_8017E068"); return (int) 0; }
s8 gm_8017E280(u16 a0, u32 a1) { (void) a0; (void) a1; Report("fighter.unported.gm_8017E280"); return (s8) 0; }
bool gm_8018841C(void) {  Report("fighter.unported.gm_8018841C"); return (bool) 0; }
void gm_801BEB68(int a0) { (void) a0; Report("fighter.unported.gm_801BEB68"); }
void* gm_801BEC54(void) {  Report("fighter.unported.gm_801BEC54"); return (void*) 0; }
bool gm_IsCKindUnlocked(u8 ckind) { (void) ckind; Report("fighter.unported.gm_IsCKindUnlocked"); return (bool) 0; }
bool gm_IsCurrently1PMode(void) {  Report("fighter.unported.gm_IsCurrently1PMode"); return (bool) 0; }
void grCorneria_801DDCF0(Vec3* a0) { (void) a0; Report("fighter.unported.grCorneria_801DDCF0"); }
void grFigureGet_80219C34(HSD_GObj* a0) { (void) a0; Report("fighter.unported.grFigureGet_80219C34"); }
bool grFigureGet_80219C50(HSD_GObj* a0) { (void) a0; Report("fighter.unported.grFigureGet_80219C50"); return (bool) 0; }
void grIceMt_801FA6D8(void) {  Report("fighter.unported.grIceMt_801FA6D8"); }
void grInishie2_801FD448(HSD_GObj* a0) { (void) a0; Report("fighter.unported.grInishie2_801FD448"); }
void grInishie2_801FD4CC(HSD_GObj* a0) { (void) a0; Report("fighter.unported.grInishie2_801FD4CC"); }
void grKongo_801D8058(Ground_GObj* a0) { (void) a0; Report("fighter.unported.grKongo_801D8058"); }
int grLib_801C9E40(void) {  Report("fighter.unported.grLib_801C9E40"); return (int) 0; }
void grLib_801C9E50(s16 a0) { (void) a0; Report("fighter.unported.grLib_801C9E50"); }
bool grLib_801C9E60(Vec3* a0) { (void) a0; Report("fighter.unported.grLib_801C9E60"); return (bool) 0; }
void grZakoGenerator_801CAC14(HSD_GObj* gobj) { (void) gobj; Report("fighter.unported.grZakoGenerator_801CAC14"); }
void grZakoGenerator_801CACB8(Item_GObj* gobj) { (void) gobj; Report("fighter.unported.grZakoGenerator_801CACB8"); }
void lbAudioAx_80024DC4(int a0) { (void) a0; Report("fighter.unported.lbAudioAx_80024DC4"); }
bool lbAudioAx_80026510(HSD_GObj* a0) { (void) a0; Report("fighter.unported.lbAudioAx_80026510"); return (bool) 0; }
bool lbAudioAx_800265C4(HSD_GObj* a0, int sfx) { (void) a0; (void) sfx; Report("fighter.unported.lbAudioAx_800265C4"); return (bool) 0; }
bool lbColl_80009F54(HitCapsule* hit, u32 arg1, float arg8) { (void) hit; (void) arg1; (void) arg8; Report("fighter.unported.lbColl_80009F54"); return (bool) 0; }
bool lbColl_8000A10C(struct lbColl_8000A10C_arg0_t* a0, u32 a1, f32 a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.lbColl_8000A10C"); return (bool) 0; }
bool lbColl_8000A244(HurtCapsule* hurt, u32 arg1, Mtx arg2, float arg3) { (void) hurt; (void) arg1; (void) arg2; (void) arg3; Report("fighter.unported.lbColl_8000A244"); return (bool) 0; }
bool lbColl_8000A584(HurtCapsule* hurt, u32 arg1, u32 arg2, MtxPtr arg3, float arg8) { (void) hurt; (void) arg1; (void) arg2; (void) arg3; (void) arg8; Report("fighter.unported.lbColl_8000A584"); return (bool) 0; }
bool lbGx_8001E2F8(Vec4* a0, Vec3* a1, U8Vec4* a2, u32 a3, float a4) { (void) a0; (void) a1; (void) a2; (void) a3; (void) a4; Report("fighter.unported.lbGx_8001E2F8"); return (bool) 0; }
void lb_8000FD18(DynamicsDesc* a0) { (void) a0; Report("fighter.unported.lb_8000FD18"); }
void lb_8000FD48(HSD_JObj* a0, DynamicsDesc* a1, size_t a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.lb_8000FD48"); }
void lb_8001044C(DynamicsDesc* a0, UNK_T a1, int a2, float pos_y, bool a4, Fighter_Part part, int a6, bool a7) { (void) a0; (void) a1; (void) a2; (void) pos_y; (void) a4; (void) part; (void) a6; (void) a7; Report("fighter.unported.lb_8001044C"); }
void lb_80011710(DynamicsDesc* a0, DynamicsDesc* a1) { (void) a0; (void) a1; Report("fighter.unported.lb_80011710"); }
bool lb_800149E0(struct lb_80014638_arg1_t* a0, u32 a1) { (void) a0; (void) a1; Report("fighter.unported.lb_800149E0"); return (bool) 0; }
int* mpLib_80056A1C(int a0, int* a1) { (void) a0; (void) a1; Report("fighter.unported.mpLib_80056A1C"); return (int*) 0; }
void pl_80037DF4(HSD_GObj* a0, union Struct2070* a1) { (void) a0; (void) a1; Report("fighter.unported.pl_80037DF4"); }
void pl_800384DC(HSD_GObj* a0, int a1, void* a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.pl_800384DC"); }
void pl_8003E4A4(int slot, bool a1, void* a2, int a3) { (void) slot; (void) a1; (void) a2; (void) a3; Report("fighter.unported.pl_8003E4A4"); }
void pl_8003E70C(Item_GObj* igobj) { (void) igobj; Report("fighter.unported.pl_8003E70C"); }
void pl_8004049C(int a0, ItemKind a1) { (void) a0; (void) a1; Report("fighter.unported.pl_8004049C"); }
s32 tyDisplay_8031C354(s32 a0, s32* a1, s32 a2, s32 a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.tyDisplay_8031C354"); return (s32) 0; }
HSD_JObj* tyDisplay_8031C5E4(s32 a0) { (void) a0; Report("fighter.unported.tyDisplay_8031C5E4"); return (HSD_JObj*) 0; }

/* Camera subjects for items (Camera_80029044); the native camera frames
 * fighters only, so these are inert storage. */
static CmSubject s_itemCameraSubjects[32];
static unsigned s_itemCameraNext;
CmSubject* Camera_80029044(int kind)
{
    CmSubject* subject = &s_itemCameraSubjects[s_itemCameraNext++ % 32];
    (void) kind;
    memset(subject, 0, sizeof(*subject));
    return subject;
}

/* Data owned by baselib/gm objects the XEX does not link. */
HSD_MObjInfo hsdMObj;
HSD_TObj* tobj_toon;
HSD_TObj* tobj_shadows;
UnkAllstarData gm_80473A18;
/* ftdata.c: effect bank index per fighter kind (-1: none). */
u8 ftData_UnkBytePerCharacter[Ft_Kind_Max] = {
    1,  3,  4,  8, 5, 12, 6, 17, 10, 15, 14, 14, 7,  2,  9,  11, 13,
    18, 16, 17, 6, 1, 3,  7, (u8) -1, 19, 49, (u8) -1, (u8) -1, (u8) -1, (u8) -1, 12, (u8) -1,
};
bool lbLang_IsSettingUS(void) { return true; }
bool lbLang_IsSettingJP(void) { return false; }
StKind Stage_80225194(void) { return (StKind) 0; }
GrKind Stage_8022519C(StKind stkind) { (void) stkind; return (GrKind) 0; }

/* Dynamics, part-animation, stat and ground hooks of the item states. */
bool Ground_801C4DD0(void) {  Report("fighter.unported.Ground_801C4DD0"); return (bool) 0; }
DynamicsDesc* Ground_801C5700(int i) { (void) i; Report("fighter.unported.Ground_801C5700"); return (DynamicsDesc*) 0; }
void ftAnim_80070734(HSD_JObj* a0, float frame) { (void) a0; (void) frame; Report("fighter.unported.ftAnim_80070734"); }
void ftCo_8009E140(Fighter* fp, bool a1) { (void) fp; (void) a1; Report("fighter.unported.ftCo_8009E140"); }
void ftCo_8009EAF8(HSD_GObj* gobj) { (void) gobj; Report("fighter.unported.ftCo_8009EAF8"); }
void ft_800880AC(Fighter* a0) { (void) a0; Report("fighter.unported.ft_800880AC"); }
void ft_80088110(Fighter* a0) { (void) a0; Report("fighter.unported.ft_80088110"); }
void pl_8003E978(int slot, bool fp_x221F_b4, Item_GObj* item_gobj) { (void) slot; (void) fp_x221F_b4; (void) item_gobj; Report("fighter.unported.pl_8003E978"); }
void pl_8003EC30(int slot, int a1, int a2, float a3) { (void) slot; (void) a1; (void) a2; (void) a3; Report("fighter.unported.pl_8003EC30"); }
void pl_8003FDF4(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_8003FDF4"); }
void pl_800403C0(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.pl_800403C0"); }
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_Max];
/* gr/stage.c state; only the Bury states read it here. */
StageInfo stage_info;

/* Kirby copy-ability hat parts, dynamic bones and player hooks (generated). */
void Player_80031DC8(void (*func_arg)(s32, s32)) { (void) func_arg; Report("fighter.unported.Player_80031DC8"); }
u8 Player_GetFlagsAEBit1(s32 slot) { (void) slot; Report("fighter.unported.Player_GetFlagsAEBit1"); return (u8) 0; }
s32 Player_GetFlagsBit1(s32 slot) { (void) slot; Report("fighter.unported.Player_GetFlagsBit1"); return (s32) 0; }
s32 Player_GetUnk4D(s32 slot) { (void) slot; Report("fighter.unported.Player_GetUnk4D"); return (s32) 0; }
void ftAnim_80070200(Fighter* a0, ftData_x8_x8* a1, CostumeTObjList* a2, DObjList* a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.ftAnim_80070200"); }
void ftAnim_80070458(Fighter* a0, CostumeTObjList* a1, u32 tobj_idx, float frame) { (void) a0; (void) a1; (void) tobj_idx; (void) frame; Report("fighter.unported.ftAnim_80070458"); }
void ftAnim_800705E0(CostumeTObjList* tobj_list) { (void) tobj_list; Report("fighter.unported.ftAnim_800705E0"); }
void ftAnim_GetNextJointInTree(HSD_Joint** a0, s32* pdepth) { (void) a0; (void) pdepth; Report("fighter.unported.ftAnim_GetNextJointInTree"); }
void ftAnim_GetNextMatAnimJointInTree(HSD_MatAnimJoint** a0, int* a1) { (void) a0; (void) a1; Report("fighter.unported.ftAnim_GetNextMatAnimJointInTree"); }
void ftCo_8009D074(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D074"); }
void ftCo_8009D18C(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D18C"); }
void ftCo_8009D2A4(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D2A4"); }
void ftCo_8009D3BC(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D3BC"); }
void ftCo_8009D4D4(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D4D4"); }
void ftCo_8009D5EC(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D5EC"); }
void ftCo_8009D704(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D704"); }
void ftCo_8009D81C(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D81C"); }
void ftCo_8009D920(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009D920"); }
void ftCo_8009DA38(ftKb_Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009DA38"); }
void ftCo_8009DB50(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_8009DB50"); }
void ftCo_800C737C(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ftCo_800C737C"); }
void ftCo_800C7414(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ftCo_800C7414"); }
void ftCo_UnloadDynamicBones(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_UnloadDynamicBones"); }
u32 ftParts_8007506C(FighterKind ftkind, int part) { (void) ftkind; (void) part; Report("fighter.unported.ftParts_8007506C"); return (u32) 0; }
void ftParts_800755E8(Fighter* a0, struct Fighter_804D6540_x0_t* a1) { (void) a0; (void) a1; Report("fighter.unported.ftParts_800755E8"); }
void lbDvd_800178E8(int a0, const char* name, int a2, int a3, int a4, int a5, int a6, u8 a7, int a8) { (void) a0; (void) name; (void) a2; (void) a3; (void) a4; (void) a5; (void) a6; (void) a7; (void) a8; Report("fighter.unported.lbDvd_800178E8"); }
HSD_MObjInfo ftMObj;

/* CPU AI hooks for stage hazards, teams, camera and debug drawing. */
bool Camera_8003118C(Vec3* a0, float a1) { (void) a0; (void) a1; Report("fighter.unported.Camera_8003118C"); return (bool) 0; }
void Ground_801C4368(float* slope, float* intercept) { (void) slope; (void) intercept; Report("fighter.unported.Ground_801C4368"); }
s32 Ground_801C5794(void) {  Report("fighter.unported.Ground_801C5794"); return (s32) 0; }
void ftCo_800A0098(Fighter* fp) { (void) fp; Report("fighter.unported.ftCo_800A0098"); }
bool ft_80087A18(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ft_80087A18"); return (bool) 0; }
int ft_80087A80(Fighter_GObj* gobj) { (void) gobj; Report("fighter.unported.ft_80087A80"); return (int) 0; }
bool gm_8016B14C(void) {  Report("fighter.unported.gm_8016B14C"); return (bool) 0; }
int gm_8016C75C(HSD_GObj* a0) { (void) a0; Report("fighter.unported.gm_8016C75C"); return (int) 0; }
bool grBigBlue_801EF844(enum_t a0) { (void) a0; Report("fighter.unported.grBigBlue_801EF844"); return (bool) 0; }
bool grCastle_801CDF54(Vec3* a0) { (void) a0; Report("fighter.unported.grCastle_801CDF54"); return (bool) 0; }
bool grCorneria_801E2D90(enum_t a0) { (void) a0; Report("fighter.unported.grCorneria_801E2D90"); return (bool) 0; }
bool grCorneria_801E2E50(int a0) { (void) a0; Report("fighter.unported.grCorneria_801E2E50"); return (bool) 0; }
bool grGreatBay_801F66A4(void) {  Report("fighter.unported.grGreatBay_801F66A4"); return (bool) 0; }
bool grInishie1_801FCAAC(int a0) { (void) a0; Report("fighter.unported.grInishie1_801FCAAC"); return (bool) 0; }
bool grRCruise_80201988(s32 a0) { (void) a0; Report("fighter.unported.grRCruise_80201988"); return (bool) 0; }
s32 grVenom_80206D10(s32 a0) { (void) a0; Report("fighter.unported.grVenom_80206D10"); return (s32) 0; }
void lbColl_800096B4(MtxPtr a0, Vec3 a1, Vec3 a2, GXColor* a3, GXColor* a4, float a5) { (void) a0; (void) a1; (void) a2; (void) a3; (void) a4; (void) a5; Report("fighter.unported.lbColl_800096B4"); }
void lbColl_80009DD4(Vec3* v0, Vec3* v1, GXColor* clr) { (void) v0; (void) v1; (void) clr; Report("fighter.unported.lbColl_80009DD4"); }

/* Effect-system hooks: particle drawing (GX immediate mode, not yet on the
 * D3D renderer), stage and state helpers (generated). */
void HSD_StateSetColorUpdate(int a0) { (void) a0; Report("fighter.unported.HSD_StateSetColorUpdate"); }
void grLib_801C99C0(s32 a0, s32 a1, HSD_JObj* a2, s32 a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.grLib_801C99C0"); }
void lb_80011C18(HSD_JObj* jobj, u32 flags) { (void) jobj; (void) flags; Report("fighter.unported.lb_80011C18"); }
