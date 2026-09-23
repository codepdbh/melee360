#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/ef/eflib.h>
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

struct Fighter_804D6518_t* Fighter_804D6518;
struct Fighter_ShakeTable_t* Fighter_GrabMashShake;
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

UNPORTED_BOOL(ftCo_80091A4C, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_80091AD8, (Fighter_GObj* gobj, int mv_x20))
UNPORTED_VOID(ftCo_80091B90, (Fighter_GObj* gobj, int arg))
UNPORTED_VOID(ftCo_80091B9C, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_80094E54, (Fighter* fp))
UNPORTED_BOOL(ftCo_80095328, (Fighter_GObj* gobj, bool* arg))
UNPORTED_VOID(ftCo_800957F4, (Fighter_GObj* gobj, FtMotionId msid))
UNPORTED_BOOL(ftCo_80099264, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_80099794, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_80099A58, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800C3B10, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C4ED8, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C5500, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800C5A50, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C5D34, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800CEE70, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D5FB0, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D6824, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D68C0, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D705C, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D7100, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D730C, (Fighter_GObj* gobj, bool arg))
UNPORTED_BOOL(ftCo_800D8A38, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800DE9D8, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800DF910, (Fighter* fp))
UNPORTED_VOID(ftCo_Attack_800CCF58, (Fighter_GObj* gobj, enum_t arg))
UNPORTED_VOID(ftCo_Attack_800CDD14, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_Attack_800D6A50, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_Attack100_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackAir_CheckItemThrowInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackDash_CheckInput, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_AttackDash_SetMv0, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackHi3_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackHi4_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackHi4_CheckInputNoD0, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackLw3_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackLw4_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackS3_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackS4_8008C114, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_AttackS4_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_Catch_CheckInput, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_DownSpot_Enter, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_HammerLanding_Enter, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_ItemScrew_Enter, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_SpecialAir_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_SpecialS_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_SquatWait_CheckInput, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftFx_AppealS_CheckInput, (HSD_GObj* gobj))
UNPORTED_VOID(ftGw_Attack11_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftLk_AttackAir_800EB3BC, (HSD_GObj* gobj))
UNPORTED_VOID(ftMh_MS_389_80151018, (HSD_GObj* gobj))
UNPORTED_VOID(ftCh_GrabUnk1_8015BC88, (HSD_GObj* gobj))
UNPORTED_VOID(ftCl_Init_8014919C, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftPe_8011BA54, (HSD_GObj* gobj))
UNPORTED_BOOL(ftPe_8011BAD8, (HSD_GObj* gobj))
UNPORTED_VOID(ftPe_8011D598, (HSD_GObj* gobj))
UNPORTED_BOOL(ftpickupitem_80094790, (HSD_GObj* gobj))
UNPORTED_VOID(ftpickupitem_80094818, (HSD_GObj* gobj, bool arg))
UNPORTED_BOOL(ft_800D2D0C, (Fighter_GObj* gobj))
UNPORTED_VOID(ft_800D2E7C, (Fighter_GObj* gobj, Vec3* v))
UNPORTED_VOID(ftCo_80090780, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C8540, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C8348, (Fighter_GObj* gobj, int timer, int health))
UNPORTED_VOID(ftCo_800C8A64, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C88D4, (Fighter_GObj* gobj, int a, bool b))
UNPORTED_VOID(Fighter_UnkInitReset_80067C98, (Fighter* fp))
UNPORTED_VOID(ftLib_SetScale, (HSD_GObj* gobj, float scale))
UNPORTED_VOID(ft_80081C88, (Fighter_GObj* gobj, float scl_y))
UNPORTED_VOID(ft_800849EC, (Fighter* a, Fighter* b))
UNPORTED_BOOL(ft_80084BFC, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_BOOL(ft_80084C38, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_VOID(ftAnim_ApplyPartAnim, (Fighter_GObj* gobj, s32 a, s32 b, float c))
UNPORTED_VOID(ftAnim_800704F0, (Fighter_GObj* gobj, int tobj_idx, float frame))
UNPORTED_BOOL(ftLib_80086FD4, (HSD_GObj* a, HSD_GObj* b))
UNPORTED_VOID(ftCo_800BFD04, (Fighter_GObj* gobj))
UNPORTED_BOOL(Player_8003544C, (s32 slot, bool condition))

SILENT_VOID(efLib_DestroyAll, (HSD_GObj* gobj))
SILENT_VOID(ft_800880D8, (Fighter* fp))
SILENT_VOID(ft_800881D8, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_80088328, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_80088478, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_80088510, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_800885A8, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_80088640, (Fighter* fp, int sfx_id, u8 sfx_vol, u8 sfx_pan))
SILENT_VOID(ft_80088770, (Fighter* fp))
SILENT_VOID(ft_800887CC, (Fighter* fp))
SILENT_VOID(ft_80088828, (Fighter* fp))
SILENT_VOID(ft_80088884, (Fighter* fp))
SILENT_VOID(ft_800888E0, (Fighter* fp))
SILENT_VOID(ft_8008893C, (Fighter* fp))
SILENT_VOID(ft_800889F4, (Fighter* fp, FtSFXArr* sfx))
SILENT_VOID(ft_800892A0, (Fighter_GObj* gobj))
SILENT_VOID(ft_80089824, (Fighter_GObj* gobj))
SILENT_VOID(ft_8008A1B8, (Fighter_GObj* gobj, u32 flags))
SILENT_VOID(ft_PlaySFX, (Fighter* fp, enum_t a, u8 b, u8 c))
SILENT_VOID(ftCamera_80076064, (Fighter* fp))
SILENT_VOID(ftCo_8009E7B4, (Fighter* fp, u8 (*blend)[2]))
SILENT_VOID(ftCo_8009F834, (Fighter_GObj* gobj, int a, Fighter_Part b, int c, int d, Vec3* e, Vec3* f, float g))
SILENT_VOID(ftCo_800C0200, (Fighter* fp, int a))
SILENT_VOID(ftCo_800C0358, (Fighter* fp, Fighter* b, s32 c))
SILENT_VOID(ftCo_800C8B60, (Fighter* fp, u8 a, u8 b))
SILENT_VOID(ftCo_800DEE84, (Fighter_GObj* gobj, u32 a, f32 b, f32 c))
SILENT_VOID(ftColl_8007AEE0, (Fighter_GObj* gobj))
SILENT_VOID(ftColl_8007B0C0, (Fighter_GObj* gobj, HurtCapsuleState state))
SILENT_VOID(ftColl_8007B128, (Fighter_GObj* gobj, int bone_id, HurtCapsuleState state))
SILENT_VOID(ftColl_8007B62C, (Fighter_GObj* gobj, enum_t a))
SILENT_VOID(ftColl_8007B760, (Fighter_GObj* gobj, int a))
SILENT_VOID(ftColl_8007B7FC, (Fighter* fp, int a))
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
SILENT_VOID(Player_SwapTransformedStates, (s32 slot, s32 a, s32 b))
SILENT_VOID(Player_UpdateJoystickCountByIndex, (s32 slot, s32 index))
SILENT_VOID(un_80322314, (void))
SILENT_VOID(Item_8026A8EC, (Item_GObj* gobj))
SILENT_VOID(Item_8026AB54, (HSD_GObj* gobj, HSD_GObj* owner, Fighter_Part part))
SILENT_VOID(Item_8026ABD8, (Item_GObj* gobj, Vec3* pos, float a))
SILENT_VOID(it_8026B9A8, (Item_GObj* gobj, HSD_GObj* a, Fighter_Part b))
SILENT_VOID(it_8026BCF4, (Item_GObj* gobj))
SILENT_VOID(it_8026BD0C, (Item_GObj* gobj))
SILENT_VOID(it_80284FC4, (Item_GObj* gobj, int a, float b))
SILENT_VOID(it_80285024, (Item_GObj* gobj, int a))
SILENT_VOID(it_8028B618, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B648, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B6B0, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B718, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B780, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B7E8, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_8028B850, (Item_GObj* gobj, f32 s))
SILENT_VOID(it_80294E78, (Item_GObj* gobj, float a))
SILENT_VOID(it_80294EB0, (Item_GObj* gobj, Vec3* a, Vec3* b))
SILENT_VOID(it_802950D4, (Item_GObj* gobj, bool a))
SILENT_VOID(it_8029A89C, (Item_GObj* gobj, f32 a))
SILENT_VOID(it_8029A8F4, (Item_GObj* gobj, Vec3* a))
SILENT_VOID(it_802BDB94, (Item_GObj* gobj))
SILENT_VOID(it_802BDD40, (Item_GObj* gobj, f32 a))
SILENT_VOID(it_802BDDB4, (Item_GObj* gobj, f32 a))

HSD_GObj* Player_GetEntity(s32 slot) { (void) slot; return NULL; }
HSD_GObj* Player_GetEntityAtIndex(int slot, int index) { (void) slot; (void) index; return NULL; }
s32 Player_GetUnk98(s32 slot) { (void) slot; return 0; }
bool gm_8016B0B4(void) { return false; }
bool un_803224DC(s32 spawn_id, f32 pos_x, f32 kb_mag) { (void) spawn_id; (void) pos_x; (void) kb_mag; return false; }
enum_t it_8026B30C(Item_GObj* gobj) { (void) gobj; return 0; }
float it_8026B54C(Item_GObj* gobj) { (void) gobj; return 0.0f; }
int it_8028B08C(Item_GObj* gobj, int statenum) { (void) gobj; (void) statenum; return 0; }
HSD_GObj* it_8029A748(Item_GObj* gobj, Vec3* a, Fighter_Part b, f32 c) { (void) gobj; (void) a; (void) b; (void) c; return NULL; }
int it_802BDA40(Item_GObj* gobj, int a) { (void) gobj; (void) a; return 0; }
s32 it_802E5F8C(Item_GObj* gobj, Vec3* a, s32 b, s32 c, f32 d, f32 e) { (void) gobj; (void) a; (void) b; (void) c; (void) d; (void) e; return 0; }
Item_HoldKinds itGetHoldKind(Item_GObj* gobj) { (void) gobj; return (Item_HoldKinds) 0; }
ItemKind itGetKind(Item_GObj* gobj) { (void) gobj; return (ItemKind) 0; }
s32 itGetMotionId(Item_GObj* gobj) { (void) gobj; return 0; }
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
bool ftCo_800BFFD0(Fighter* fp, enum_t a, bool b) { (void) fp; (void) a; (void) b; return false; }
bool ftCo_800C5240(Fighter_GObj* gobj) { (void) gobj; return false; }
int mpLineGetNext(int line_id) { (void) line_id; return -1; }
int mpLineGetPrev(int line_id) { (void) line_id; return -1; }

Vec2* ftCo_80090690(Fighter* fp, Vec2* shift)
{
    (void) fp;
    shift->x = shift->y = 0.0f;
    return shift;
}

Vec2* ftCo_800DEEE8(Fighter* fp, Vec2* shift)
{
    (void) fp;
    shift->x = shift->y = 0.0f;
    return shift;
}

float ftCo_Damage_CalcAngle(Fighter* fp, float kb_applied)
{
    (void) fp;
    (void) kb_applied;
    Report("fighter.unported.ftCo_Damage_CalcAngle");
    return 0.0f;
}

FighterKind ftLib_GetKind(HSD_GObj* gobj)
{
    return GET_FIGHTER(gobj)->kind;
}

void ftLib_800866DC(HSD_GObj* gobj, Vec3* out)
{
    *out = GET_FIGHTER(gobj)->cur_pos;
}

struct ftData_80085FD4_ret* ftData_80085FD4(Fighter* fp, FtMotionId msid)
{
    return (struct ftData_80085FD4_ret*) &fp->x24[msid];
}

void Fighter_TakeDamage_8006CC7C(Fighter* fp, float damage_amount)
{
    fp->dmg.x1830_percent += damage_amount;
    if (fp->dmg.x1830_percent > 999.0f)
        fp->dmg.x1830_percent = 999.0f;
}

void ftPartSetRotZ(Fighter* fp, int part_idx, f32 rotate_z)
{
    if (fp->parts && fp->parts[part_idx].joint)
        HSD_JObjSetRotationZ(fp->parts[part_idx].joint, rotate_z);
}

void ftColl_800768A0(Fighter* fp, HitCapsule* dst)
{
    (void) fp;
    dst->x44 = 0;
    dst->x45 = 0;
}

void ftColl_8007ABD0(HitCapsule* hit, u32 damage, Fighter_GObj* gobj)
{
    (void) gobj;
    hit->unk_count = damage;
    hit->damage = (float) damage;
}

void ftColl_8007AC9C(HitCapsule* hit, int angle, Fighter_GObj* gobj)
{
    (void) gobj;
    hit->kb_angle = angle;
}

void ftColl_8007AD18(Fighter* fp, HitCapsule* hit)
{
    Vec3 offset;
    (void) fp;
    switch (hit->state) {
    case HitCapsule_Enabled:
        offset = hit->b_offset;
        lb_8000B1CC(hit->jobj, &offset, &hit->x4C);
        hit->x58 = hit->x4C;
        hit->state = HitCapsule_Unk2;
        break;
    case HitCapsule_Unk2:
        hit->state = HitCapsule_Unk3;
    case HitCapsule_Unk3:
        hit->x58 = hit->x4C;
        offset = hit->b_offset;
        lb_8000B1CC(hit->jobj, &offset, &hit->x4C);
        break;
    default:
        break;
    }
}

void ftColl_8007AFC8(Fighter_GObj* gobj, int hit_idx)
{
    GET_FIGHTER(gobj)->x914[hit_idx].state = HitCapsule_Disabled;
}

void ftColl_8007AFF8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    unsigned i;
    for (i = 0; i < ARRAY_SIZE(fp->x914); ++i)
        fp->x914[i].state = HitCapsule_Disabled;
    fp->x2219_b3 = false;
}
