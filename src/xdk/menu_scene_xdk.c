#include <stdarg.h>
#include <string.h>

#include <melee/gm/gm_1601.h>
#include <melee/gm/gmevent.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gmscene.h>
#include <melee/gm/types.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lblanguage.h>
#include <melee/lb/lbmthp.h>
#include <melee/mn/mncount.h>
#include <melee/mn/mndatadel.h>
#include <melee/mn/mndeflicker.h>
#include <melee/mn/mndiagram.h>
#include <melee/mn/mnevent.h>
#include <melee/mn/mngallery.h>
#include <melee/mn/mnhyaku.h>
#include <melee/mn/mninfo.h>
#include <melee/mn/mninfobonus.h>
#include <melee/mn/mnlanguage.h>
#include <melee/mn/mnmain.h>
#include <melee/mn/mnmainrule.h>
#include <melee/mn/mnname.h>
#include <melee/mn/mnsnap.h>
#include <melee/mn/mnsound.h>
#include <melee/mn/mnsoundtest.h>
#include <melee/mn/mnvibration.h>
#include <melee/mn/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/sislib.h>

#include "menu_scene_xdk.h"

extern void M360_AudioSfx(unsigned sfxId, unsigned volume, unsigned pan);

extern MenuKindData mn_803EB6B0[0x22];
void HSD_GObj_RunProcs(void);
void HSD_GObj_80390FC0(void);

HSD_Archive* mn_804D6BB8;
u8 mn_804D6BB4;
u8 mn_804D6BB5;

static struct MenuExitData s_exitData;
static int s_exitRequested;
static GameRules s_gameRules;
static struct GamePrefs s_gamePrefs;
static HSD_Text* s_texts[16];
static int s_archiveToken;
static int s_textTraced;

static void ResumeMenu(const char* leaf)
{
    void (*think)(HSD_GObj*) = mn_803EB6B0[mn_804A04F0.cur_menu].think;
    M360_MenuTrace(leaf, mn_804A04F0.cur_menu);
    if (think) {
        HSD_GObjProc* proc = HSD_GObj_SetupProc(GObj_Create(0, 1, 0x80), think, 0);
        proc->flags_3 = (u8) HSD_GObj_804D783C;
    }
}

HSD_Archive* M360_ArchiveLoadSymbolsV(const char* filename, void* symbols, va_list ap);

HSD_Archive* lbArchive_LoadSymbols(const char* filename, void* symbols, ...)
{
    va_list ap;
    void** slot = symbols;
    unsigned missing = 0;
    va_start(ap, symbols);
    if (strcmp(filename, "MnMaAll") != 0) {
        HSD_Archive* archive = M360_ArchiveLoadSymbolsV(filename, symbols, ap);
        va_end(ap);
        return archive;
    }
    while (slot) {
        const char* name = va_arg(ap, const char*);
        *slot = M360_MenuSymbol(name);
        if (!*slot)
            ++missing;
        slot = va_arg(ap, void**);
    }
    va_end(ap);
    M360_MenuTrace("menu.archive.missing_symbols", missing);
    return (HSD_Archive*) &s_archiveToken;
}

void* gm_GetCurrentSceneExitData(void)
{
    return &s_exitData;
}

void gm_801A4B60(void)
{
    s_exitRequested = 1;
}

u8 gm_GetCurrentGameMode(void)
{
    return GM_MENU;
}

int gmMainLib_8015ECB0(void) { return M360_MenuBgmChoice(); }
s32 gmMainLib_8015EDD4(void) { return 0; }
s32 gmMainLib_8015EE90(void) { return 0; }
GameRules* gmMainLib_GetGameRules(void) { return &s_gameRules; }
struct GamePrefs* gmMainLib_GetGamePrefs(void) { return &s_gamePrefs; }
void gm_801603B0(void) {}
void gm_801677E8(s8 arg0) { (void) arg0; }
void gm_801BA8FC(void) {}
u8 gm_801BEB80(void) { return 0; }
bool gm_IsCKindUnlocked(u8 ckind) { return ckind < 0x19; }
u8 gm_SelKindToCKind(u8 selkind) { return selkind; }
void gm_80190EA4(void) { M360_MenuTrace("menu.leaf.unported:tournament", 0); }
s32 mnCharSel_802640A0(void) { M360_MenuTrace("menu.leaf.unported:charsel", 0); return 0; }

int lbAudioAx_80023694(void) { return 0; }
int lbAudioAx_800236DC(void) { return 0; }
void lbAudioAx_8002392C(void) {}
int lbAudioAx_80023F28(int bgm) { M360_MenuPlayBgm(bgm); return 0; }
void lbAudioAx_80024030(int sfx)
{
    M360_MenuTrace("menu.sfx", (unsigned) sfx);
    M360_AudioSfx((unsigned) sfx, 100, 64);
}
void lbCardGame_UpdatePowerTime(void) {}
bool lbLang_IsSavedLanguageUS(void) { return true; }
void lbMthp_8001F800(void) {}
int lb_8001B760(int result) { return result; }

void HSD_SisLib_803A62A0(s32 index, char* file, char* symbol)
{
    (void) index;
    (void) file;
    (void) symbol;
}

int HSD_SisLib_803A611C(int a, HSD_GObj* gobj, u16 b, u8 c, u8 d, u8 e, u8 f, u32 g)
{
    (void) a; (void) gobj; (void) b; (void) c; (void) d; (void) e; (void) f; (void) g;
    return 0;
}

HSD_Text* HSD_SisLib_803A5ACC(int a, s32 b, f32 x, f32 y, f32 z, f32 w, f32 h)
{
    int i;
    (void) a; (void) b; (void) x; (void) y; (void) z; (void) w; (void) h;
    for (i = 0; i < 16; ++i) {
        if (!s_texts[i]) {
            s_texts[i] = HSD_MemAlloc(sizeof(HSD_Text));
            if (s_texts[i])
                memset(s_texts[i], 0, sizeof(HSD_Text));
            return s_texts[i];
        }
    }
    return NULL;
}

void HSD_SisLib_803A5CC4(HSD_Text* text)
{
    int i;
    for (i = 0; i < 16; ++i) {
        if (text && s_texts[i] == text) {
            HSD_Free(text);
            s_texts[i] = NULL;
        }
    }
}

void HSD_SisLib_803A5E70(void)
{
    int i;
    for (i = 0; i < 16; ++i) {
        if (s_texts[i]) {
            HSD_Free(s_texts[i]);
            s_texts[i] = NULL;
        }
    }
}

void HSD_SisLib_803A6368(HSD_Text* text, s32 index)
{
    (void) text;
    if (!s_textTraced) {
        s_textTraced = 1;
        M360_MenuTrace("menu.text.unported", (unsigned) index);
    }
}

void mnCount_Create(void) { ResumeMenu("menu.leaf.unported:misc_records"); }
void mnDataDel_80250170(void) { ResumeMenu("menu.leaf.unported:erase_data"); }
void mnDeflicker_8024A6C4(int arg) { (void) arg; ResumeMenu("menu.leaf.unported:screen_display"); }
void mnDiagram_Init(u8 a, u8 b) { (void) a; (void) b; ResumeMenu("menu.leaf.unported:vs_records"); }
void mnEvent_8024E838(int a, int b) { (void) a; (void) b; ResumeMenu("menu.leaf.unported:event_match"); }
void mnGallery_80259868(void) { ResumeMenu("menu.leaf.unported:movies"); }
void mnHyaku_8024CD64(u8 arg) { (void) arg; ResumeMenu("menu.leaf.unported:multiman"); }
void mnInfoBonus_80252F8C(void) { ResumeMenu("menu.leaf.unported:bonus_records"); }
s32 mnInfo_80252758(void) { ResumeMenu("menu.leaf.unported:special_data"); return 0; }
void mnLanguage_8024C5C0(HSD_GObj* gobj) { (void) gobj; ResumeMenu("menu.leaf.unported:language"); }
s32 mnName_8023AC40(void) { ResumeMenu("menu.leaf.unported:name_entry"); return 0; }
void mnSnap_80257F24(void) { ResumeMenu("menu.leaf.unported:snapshots"); }
HSD_GObjProc* mnSoundTest_8024BEE0(s32 arg) { (void) arg; ResumeMenu("menu.leaf.unported:sound_test"); return NULL; }
void mnSound_8024A09C(int arg) { (void) arg; ResumeMenu("menu.leaf.unported:sound"); }
void mnVibration_Init(int arg) { (void) arg; ResumeMenu("menu.leaf.unported:rumble"); }
void mn_80231714(void) { ResumeMenu("menu.leaf.unported:rules"); }

void M360_MenuSceneLeave(void)
{
    int link;
    for (link = 0; link < 64; ++link) {
        HSD_GObj* gobj = HSD_GObjPLinkHead[link];
        while (gobj) {
            HSD_GObj* next = gobj->next;
            HSD_GObjFree(gobj);
            gobj = next;
        }
    }
    HSD_SisLib_803A5E70();
}

void M360_MenuSceneEnter(unsigned kind, unsigned selection)
{
    struct MenuEnterData data;
    M360_MenuSceneLeave();
    memset(&s_exitData, 0, sizeof(s_exitData));
    s_exitRequested = 0;
    data.menu_kind = (u8) kind;
    data.hovered_selection = (u8) selection;
    data.load_assets = 1;
    data.x3 = 0;
    mnMain_Scene_OnEnter(&data);
}

int M360_MenuSceneFrame(void)
{
    mnMain_Scene_OnFrame();
    if (!s_exitRequested)
        HSD_GObj_RunProcs();
    if (!s_exitRequested)
        return M360_MENU_CONTINUE;
    s_exitRequested = 0;
    switch (s_exitData.pending_mode) {
    case GM_TITLE:
        return M360_MENU_TO_TITLE;
    case GM_MENU:
        return M360_MENU_RESTART;
    case GM_VS:
        M360_MenuTrace("menu.exit.quick_match", (unsigned) s_exitData.pending_mode);
        return M360_MENU_TO_MATCH;
    case GM_CLASSIC:
        M360_MenuTrace("menu.exit.classic", (unsigned) s_exitData.pending_mode);
        return M360_MENU_TO_CLASSIC;
    case GM_ADVENTURE:
        M360_MenuTrace("menu.exit.adventure", (unsigned) s_exitData.pending_mode);
        return M360_MENU_TO_ADVENTURE;
    default:
        M360_MenuTrace("menu.leaf.unported:mode", (unsigned) s_exitData.pending_mode);
        return M360_MENU_CONTINUE;
    }
}

void M360_MenuSceneRender(void)
{
    HSD_GObj_80390FC0();
}

void M360_MenuSceneState(unsigned* kind, unsigned* selection)
{
    *kind = mn_804A04F0.cur_menu;
    *selection = mn_804A04F0.hovered_selection;
}
