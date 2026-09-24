#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <dolphin/mtx.h>
#include <melee/cm/camera.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/db/db.h>
#include <melee/ft/ftdata.h>
#include <melee/gr/stage.h>
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

UNPORTED_BOOL(ftCo_80094E54, (Fighter* fp))
UNPORTED_BOOL(ftCo_80095328, (Fighter_GObj* gobj, bool* arg))
UNPORTED_VOID(ftCo_800957F4, (Fighter_GObj* gobj, FtMotionId msid))
UNPORTED_BOOL(ftCo_800C3B10, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C4ED8, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C5500, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800C5A50, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C5D34, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800CEE70, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800D730C, (Fighter_GObj* gobj, bool arg))
UNPORTED_VOID(ftCo_Attack_800CCF58, (Fighter_GObj* gobj, enum_t arg))
UNPORTED_VOID(ftCo_Attack_800CDD14, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_HammerLanding_Enter, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_ItemScrew_Enter, (Fighter_GObj* gobj))
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
UNPORTED_VOID(ftCo_800C8540, (Fighter_GObj* gobj))
UNPORTED_VOID(ftCo_800C8348, (Fighter_GObj* gobj, int timer, int health))
UNPORTED_VOID(ftLib_SetScale, (HSD_GObj* gobj, float scale))
UNPORTED_VOID(ft_80081C88, (Fighter_GObj* gobj, float scl_y))
UNPORTED_VOID(ft_800849EC, (Fighter* a, Fighter* b))
UNPORTED_BOOL(ft_80084BFC, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_BOOL(ft_80084C38, (Fighter_GObj* gobj, int* a, int* b, int* c))
UNPORTED_VOID(ftAnim_ApplyPartAnim, (Fighter_GObj* gobj, s32 a, s32 b, float c))
UNPORTED_VOID(ftAnim_800704F0, (Fighter_GObj* gobj, int tobj_idx, float frame))
UNPORTED_BOOL(ftLib_80086FD4, (HSD_GObj* a, HSD_GObj* b))
UNPORTED_BOOL(Player_8003544C, (s32 slot, bool condition))

SILENT_VOID(efLib_DestroyAll, (HSD_GObj* gobj))
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
bool ftCo_800C5240(Fighter_GObj* gobj) { (void) gobj; return false; }
int mpLineGetNext(int line_id) { (void) line_id; return -1; }
int mpLineGetPrev(int line_id) { (void) line_id; return -1; }



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

UNPORTED_VOID(ftCo_800CDDA0, (Fighter_GObj* gobj))
UNPORTED_VOID(ftGw_AttackLw3_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftGw_AttackS4_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftPe_AttackS4_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftNs_AttackS4_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftNs_AttackHi4_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftNs_AttackLw4_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftLk_AttackAir_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftGw_AttackAirN_DecideAction, (HSD_GObj* gobj))
UNPORTED_VOID(ftGw_Attack100Start_Enter, (HSD_GObj* gobj))
UNPORTED_VOID(ftKb_SpecialN_800F1CD8, (HSD_GObj* gobj))
UNPORTED_VOID(ftKb_SpecialN_800F1F68, (HSD_GObj* gobj))
UNPORTED_VOID(pl_8003E854, (int a, int b, Item_GObj* c))
SILENT_VOID(efLib_PauseAll, (HSD_GObj* gobj))
SILENT_VOID(efLib_ResumeAll, (HSD_GObj* gobj))
UNPORTED_BOOL(ftCo_800C60C8, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftpickupitem_8009447C, (HSD_GObj* gobj, Item_GObj* item))
UNPORTED_BOOL(it_8026B594, (Item_GObj* gobj))
bool gm_8016B0FC(void) { return false; }
Item_GObj* ftpickupitem_800942A0(Fighter_GObj* gobj, u32 flags) { (void) gobj; (void) flags; return NULL; }
void efAsync_Spawn(HSD_GObj* gobj, void* queue_head, u32 spawn_kind, u32 gfx_id, HSD_JObj* jobj, ...)
{
    (void) gobj; (void) queue_head; (void) spawn_kind; (void) gfx_id; (void) jobj;
}

DbLKind DbLevel;
ItemCommonData* it_804D6D28;
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

bool ftLib_80086960(HSD_GObj* gobj)
{
    return gobj && gobj->classifier == HSD_GOBJ_CLASS_FIGHTER;
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

void ftKb_SpecialN_800F1F1C(Fighter_GObj* a0, Vec3* a1) { (void) a0; (void) a1; Report("fighter.unported.ftKb_SpecialN_800F1F1C");  }
void Camera_RequestQuake(CmQuakeKind a0, Vec* a1) { (void) a0; (void) a1; Report("fighter.unported.Camera_RequestQuake");  }
void pl_80037C60(Fighter_GObj* a0, volatile s32 a1) { (void) a0; (void) a1; Report("fighter.unported.pl_80037C60");  }
s32 it_8026B588(void) {  Report("fighter.unported.it_8026B588"); return (s32) 0; }
bool gm_8016B1C4(void) {  Report("fighter.unported.gm_8016B1C4"); return (bool) 0; }
void ft_800C80A4(Fighter* a0) { (void) a0; Report("fighter.unported.ft_800C80A4");  }
bool ftCo_800C7CA0(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C7CA0"); return (bool) 0; }
void* efSync_Spawn(s32 a0, HSD_GObj* a1, ...) { (void) a0; (void) a1; Report("fighter.unported.efSync_Spawn"); return NULL; }
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
s32 it_802E5EF4(Item_GObj* a0) { (void) a0; Report("fighter.unported.it_802E5EF4"); return (s32) 0; }
void it_8027B4A4(Fighter_GObj* a0, Item_GObj* a1) { (void) a0; (void) a1; Report("fighter.unported.it_8027B4A4");  }
s32 it_80272818(Item* a0) { (void) a0; Report("fighter.unported.it_80272818"); return (s32) 0; }
void it_8026FC00(Item* a0, HitCapsule* a1, s32 a2, Fighter* a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.it_8026FC00");  }
void it_8026FAC4(Item* a0, HitCapsule* a1, s32 a2, void* a3, bool a4) { (void) a0; (void) a1; (void) a2; (void) a3; (void) a4; Report("fighter.unported.it_8026FAC4");  }
bool it_8026BC68(Item_GObj* a0) { (void) a0; Report("fighter.unported.it_8026BC68"); return (bool) 0; }
bool it_8026B6C8(Item_GObj* a0) { (void) a0; Report("fighter.unported.it_8026B6C8"); return (bool) 0; }
bool it_8026B2D8(Item_GObj* a0) { (void) a0; Report("fighter.unported.it_8026B2D8"); return (bool) 0; }
float it_8026B1D4(Item_GObj* a0, HitCapsule* a1) { (void) a0; (void) a1; Report("fighter.unported.it_8026B1D4"); return 0.0f; }
bool itIsHeavy(Item_GObj* a0) { (void) a0; Report("fighter.unported.itIsHeavy"); return (bool) 0; }
HSD_GObj* itGetOwner(Item_GObj* a0) { (void) a0; Report("fighter.unported.itGetOwner"); return NULL; }
s32 itGetAttackId(Item_GObj* a0) { (void) a0; Report("fighter.unported.itGetAttackId"); return (s32) 0; }
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
void ftKb_SpecialN_800F5C34(Fighter* a0) { (void) a0; Report("fighter.unported.ftKb_SpecialN_800F5C34");  }
void ftKb_SpecialN_800F5BA4(Fighter* a0) { (void) a0; Report("fighter.unported.ftKb_SpecialN_800F5BA4");  }
void ftKb_SpecialN_800F1D24(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftKb_SpecialN_800F1D24");  }
bool ftFx_AppealS_CheckIfUsed(Fighter* a0) { (void) a0; Report("fighter.unported.ftFx_AppealS_CheckIfUsed"); return (bool) 0; }
void ftDk_MS_349_800E06D8(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftDk_MS_349_800E06D8");  }
void ftCo_HammerWait_IASA(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_HammerWait_IASA");  }
void ftCo_HammerFall_IASA(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_HammerFall_IASA");  }
void ftCo_DamageIce_Init(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_DamageIce_Init");  }
void ftCo_DamageIce_HitWhileFrozen(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_DamageIce_HitWhileFrozen");  }
bool ftCo_800D3158(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800D3158"); return (bool) 0; }
bool ftCo_800D2FA4(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800D2FA4"); return (bool) 0; }
void ftCo_800CF4DC(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800CF4DC");  }
bool ftCo_800C74F4(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C74F4"); return (bool) 0; }
void ftCo_800C703C(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C703C");  }
bool ftCo_800C5DDC(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C5DDC"); return (bool) 0; }
bool ftCo_800C5CD4(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C5CD4"); return (bool) 0; }
void ftCo_800C555C(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C555C");  }
void ftCo_800C554C(Fighter* a0) { (void) a0; Report("fighter.unported.ftCo_800C554C");  }
bool ftCo_800C53E4(Fighter* a0) { (void) a0; Report("fighter.unported.ftCo_800C53E4"); return (bool) 0; }
void ftCo_800C511C(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C511C");  }
bool ftCo_800C44CC(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C44CC"); return (bool) 0; }
bool ftCo_800C3538(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C3538"); return (bool) 0; }
void ftCo_800C318C(Fighter_GObj* a0, bool a1) { (void) a0; (void) a1; Report("fighter.unported.ftCo_800C318C");  }
void ftCo_800C2FD8(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C2FD8");  }
bool ftCo_800C0CB8(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C0CB8"); return (bool) 0; }
void ftCo_800C0B20(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C0B20");  }
void ftCo_800C0A98(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_800C0A98");  }
bool ftCo_800C0A28(Fighter_GObj* a0, UNK_T a1, ftCommon_BuryType a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftCo_800C0A28"); return (bool) 0; }
void ftCo_800C08A0(Fighter_GObj* a0, Fighter_GObj* a1, DynamicsDesc* a2, ftCommon_BuryType a3) { (void) a0; (void) a1; (void) a2; (void) a3; Report("fighter.unported.ftCo_800C08A0");  }
void ftCo_800C0874(Fighter_GObj* a0, UNK_T a1, ftCommon_BuryType a2) { (void) a0; (void) a1; (void) a2; Report("fighter.unported.ftCo_800C0874");  }
void ftCo_800A0DA4(Fighter* a0) { (void) a0; Report("fighter.unported.ftCo_800A0DA4");  }
void ftCo_8009750C(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_8009750C");  }
void ftCo_80095744(Fighter_GObj* a0, bool* a1) { (void) a0; (void) a1; Report("fighter.unported.ftCo_80095744");  }
void ftCo_80090984(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftCo_80090984");  }
void ftCh_Init_80156014(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftCh_Init_80156014");  }
void ftCh_GrabUnk1_8015ADD0(HSD_GObj* a0) { (void) a0; Report("fighter.unported.ftCh_GrabUnk1_8015ADD0");  }
void ftAnim_80070F28(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070F28");  }
void ftAnim_80070E74(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070E74");  }
void ftAnim_80070654(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.ftAnim_80070654");  }
void efAsync_QueueFlush(HSD_GObj* a0, void* a1) { (void) a0; (void) a1; Report("fighter.unported.efAsync_QueueFlush");  }
void Player_SetTotalCoins(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.Player_SetTotalCoins");  }
void Player_SetCoins(int a0, int a1) { (void) a0; (void) a1; Report("fighter.unported.Player_SetCoins");  }
int Player_GetTotalCoins(int a0) { (void) a0; Report("fighter.unported.Player_GetTotalCoins"); return (int) 0; }
s32 Player_GetRemainingHPByIndex(s32 a0, s32 a1) { (void) a0; (void) a1; Report("fighter.unported.Player_GetRemainingHPByIndex"); return (s32) 0; }
bool Player_GetMoreFlagsBit3(s32 a0) { (void) a0; Report("fighter.unported.Player_GetMoreFlagsBit3"); return (bool) 0; }
bool Player_GetMoreFlagsBit0(s32 a0) { (void) a0; Report("fighter.unported.Player_GetMoreFlagsBit0"); return (bool) 0; }
int Player_GetCoins(int a0) { (void) a0; Report("fighter.unported.Player_GetCoins"); return (int) 0; }
int Fighter_SuperMushroomEnd(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.Fighter_SuperMushroomEnd"); return (int) 0; }
bool Fighter_SuperMushroomApply(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.Fighter_SuperMushroomApply"); return (bool) 0; }
bool Fighter_PoisonMushroomEnd(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.Fighter_PoisonMushroomEnd"); return (bool) 0; }
bool Fighter_PoisonMushroomApply(Fighter_GObj* a0) { (void) a0; Report("fighter.unported.Fighter_PoisonMushroomApply"); return (bool) 0; }

/* Shield, dodge and special-fall dependencies. Item tosses and grabs out of
 * shield stay unavailable until items/grabs are integrated; the Yoshi and
 * Sheik branches are unreachable with the current Mario-only roster. */
UNPORTED_BOOL(ftCo_8009515C, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_8009563C, (Fighter_GObj* gobj))
UNPORTED_VOID(ftSs_SpecialLw_8012AEBC, (HSD_GObj* gobj))
UNPORTED_VOID(ftSs_SpecialLw_8012AF38, (HSD_GObj* gobj))
UNPORTED_VOID(ftYs_Init_8012B8A4, (HSD_GObj* gobj))
UNPORTED_VOID(ftYs_Init_8012BDA0, (Fighter_GObj* gobj))
UNPORTED_VOID(ftYs_Init_8012BE3C, (HSD_GObj* gobj))
UNPORTED_VOID(ftYs_Init_8012BECC, (Fighter_GObj* gobj))
UNPORTED_VOID(ftYs_Shield_8012C1D4, (Fighter_GObj* gobj))
UNPORTED_VOID(ftYs_Shield_8012C49C, (Fighter_GObj* gobj))
UNPORTED_VOID(ftYs_Shield_8012C600, (Fighter_GObj* gobj, bool arg))
UNPORTED_VOID(ftYs_Shield_8012C850, (Fighter_GObj* gobj))
UNPORTED_VOID(ftYs_Shield_8012C914, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftYs_Shield_8012CC1C, (Fighter_GObj* gobj))
SILENT_VOID(efLib_SetParamAlpha, (HSD_GObj* gobj, u8 alpha))
SILENT_VOID(pl_8003E0E8, (int slot, int sub))
SILENT_VOID(pl_8003E114, (int slot, int sub, float health))
u32 Player_GetUnk45(s32 slot) { (void) slot; return 0; }
int un_80322598(int spawn_id, float y) { (void) spawn_id; (void) y; return 0; }

/* Grab/throw dependencies outside the Mario-vs-Mario match: item grabs,
 * tether/hookshot items, cargo throws, Kirby/Yoshi/Samus/Mewtwo/Fox hooks. */
UNPORTED_BOOL(ftCo_800951D0, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_80095254, (Fighter_GObj* gobj))
UNPORTED_BOOL(ftCo_800952DC, (Fighter_GObj* gobj))
UNPORTED_VOID(ftpickupitem_80094694, (Fighter_GObj* gobj, FtMotionId msid, bool arg))
UNPORTED_VOID(ftpickupitem_800948A8, (Fighter_GObj* gobj, Item_GObj* item))
UNPORTED_VOID(ftCo_8009B56C, (Fighter_GObj* gobj))
UNPORTED_VOID(ftSs_Init_CreateThrowGrappleBeam, (HSD_GObj* gobj, s32 motion_state, float anim_speed))
UNPORTED_VOID(Fighter_UpdateModelScale, (Fighter_GObj* gobj))
UNPORTED_VOID(it_802A7840, (HSD_GObj* gobj))
UNPORTED_VOID(it_802A7AAC, (HSD_GObj* gobj))
UNPORTED_VOID(it_802BAA94, (Item_GObj* gobj))
UNPORTED_VOID(it_802BAC3C, (Fighter_GObj* gobj))
SILENT_VOID(ftMt_SpecialN_Shoot, (HSD_GObj* gobj))
SILENT_VOID(ftFx_Throw_Anim, (HSD_GObj* gobj))
SILENT_VOID(ftCamera_800762F4, (HSD_GObj* gobj))
SILENT_VOID(pl_80040614, (int slot, bool sub, float damage))
float ftYs_Init_8012BAC0(Fighter* fp) { (void) fp; Report("fighter.unported.ftYs_Init_8012BAC0"); return 1.0f; }
/* Standing rank (gm_8016C5C0) and handicap: tied players at the default
 * VS handicap of 9 (gm_1601.c). */
s32 Player_80033BB8(int slot) { (void) slot; return 0; }
int Player_GetHandicap(int slot) { (void) slot; return 9; }
/* Link hookshot, Samus grapple and boomerang items used by the tether-grab
 * branches of ftCo_0D8E/ftCo_0D95; unreachable for Mario. */
UNPORTED_BOOL(ftLk_Init_BoomerangExists, (HSD_GObj* gobj))
UNPORTED_VOID(it_802A2B10, (Item_GObj* gobj))
UNPORTED_VOID(it_802A77DC, (Item_GObj* gobj))
UNPORTED_VOID(it_802A78B8, (HSD_GObj* gobj, Vec3* pos))
UNPORTED_VOID(it_802A7AF0, (HSD_GObj* gobj))
UNPORTED_VOID(it_802A7B34, (HSD_GObj* gobj))
UNPORTED_VOID(it_802B7B84, (Item_GObj* gobj))
UNPORTED_VOID(it_802BAA58, (Item_GObj* gobj))
UNPORTED_VOID(it_802BAAE4, (Item_GObj* gobj, Vec3* vel))
UNPORTED_VOID(it_802BAC80, (Fighter_GObj* gobj))
UNPORTED_VOID(it_802BACC4, (Fighter_GObj* gobj))
Item_GObj* it_802A2BA4(Fighter_GObj* gobj, Vec3* pos, f32 facing, s32 arg)
{
    (void) gobj; (void) pos; (void) facing; (void) arg;
    Report("fighter.unported.it_802A2BA4");
    return NULL;
}
Item_GObj* it_802B7C18(Fighter_GObj* gobj, Vec3* pos, float facing)
{
    (void) gobj; (void) pos; (void) facing;
    Report("fighter.unported.it_802B7C18");
    return NULL;
}

/* Mario special-move items: fireball, Dr. Mario pill and cape. The moves
 * run their original motion states; the projectiles/cape objects wait for
 * the item system. */
SILENT_VOID(it_8026B3F8, (Article* article, s32 kind))
UNPORTED_VOID(it_8029B6F8, (Item_GObj* gobj, Vec3* pos, ItemKind kind, f32 facing))
UNPORTED_VOID(itDrMarioPill_Spawn, (Item_GObj* gobj, Vec3* pos, s32 arg, ItemKind kind, f32 facing))
UNPORTED_VOID(it_802B2674, (Item_GObj* gobj))
UNPORTED_VOID(it_802B26C0, (Item_GObj* gobj))
UNPORTED_VOID(it_802B26E0, (Item_GObj* gobj))
SILENT_VOID(lb_800119DC, (Vec3* pos, int arg, float a, float b, float c))
SILENT_VOID(ftCo_800C7158, (Fighter_GObj* gobj))
SILENT_VOID(ftCo_800C7200, (Fighter_GObj* gobj))
Item_GObj* it_802B2560(Fighter_GObj* gobj, float facing_dir, Vec3* pos, Fighter_Part part, ItemKind kind)
{
    (void) gobj; (void) facing_dir; (void) pos; (void) part; (void) kind;
    Report("fighter.unported.it_802B2560");
    return NULL;
}

/* Ledge bookkeeping without a gameplay effect in the quick match: stage
 * joint callbacks (static Battlefield) and ledge-grab statistics. */
SILENT_VOID(mpLib_8005811C, (CollData* coll, int ledge_id))
SILENT_VOID(pl_8003FFDC, (int a, int b, int c, int d, int e))
SILENT_VOID(pl_80040048, (int slot, int sub))
/* Taunt hooks for other characters and the taunt statistic. */
UNPORTED_VOID(ftCl_Init_80149318, (Fighter_GObj* gobj))
UNPORTED_VOID(ftDr_Init_80149910, (HSD_GObj* gobj))
UNPORTED_VOID(ftKb_SpecialN_800F5D04, (Fighter_GObj* gobj, bool arg))
UNPORTED_VOID(ftPe_Init_8011B93C, (HSD_GObj* gobj))
UNPORTED_VOID(ftZd_Init_801395C8, (HSD_GObj* gobj))
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
