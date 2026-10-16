#include <string.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/gmscene.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lblanguage.h>
#include <melee/lb/types.h>
#include <melee/mn/mncharsel.h>
#include <melee/mn/mnmainrule.h>
#include <melee/mn/mnstagesel.h>
#include <melee/mn/mnnamenew.h>
#include <melee/mn/types.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/mobj.h>
#include "character_select_xdk.h"
#include "match_xdk.h"
void M360_MenuTextStart(void);
void M360_MenuTextStop(void);
void M360_MenuTextPoolReleased(void);
void M360_MenuSetGameMode(unsigned);
void M360_MenuResetExit(void);
int M360_MenuTakeExit(void);
void M360_AudioSfx(unsigned,unsigned,unsigned);
void HSD_GObj_RunProcs(void);
static CSSData s_css;
static u8 s_koCounts[4];
static SSSData s_sss;
static unsigned s_mode;
static int s_resumeCss;
static unsigned s_lastPlayer[4];
static const char* s_notice;
static unsigned s_noticeFrames;
static int s_active;
static PreloadedGameModeState s_preload;

HSD_Archive* lbArchive_LoadArchive(const char* name) {
    HSD_Archive* archive=NULL;
    lbArchive_80016F80(&archive,name);
    return archive;
}
/* The native archive cache owns immutable backing storage until shutdown;
 * original scene releases do not invalidate pointers retained by the cache. */
void lbArchive_80016EFC(HSD_Archive* archive) { (void)archive; }
bool lbLang_IsSavedLanguageJP(void) { return false; }
PreloadedGameModeState* lbDvd_GetPreloadCacheScene(void) { return &s_preload; }
/* Native loads are synchronous at match start; CSS still writes the original
 * requested character/costume cache entries each frame. */
void lbDvd_80018254(void) {}
void lbCardNew_AllocWorkArea(void) {}
void lbCardGame_LoadArchive(int slot) { (void)slot; }
void lb_800145F4(void) {}
void gm_ChangeGameModeAfterCurrentScene(int mode) { M360_MenuSetGameMode((unsigned)mode); }
void HSD_MObjSetAlpha(HSD_MObj* mobj, float alpha) { mobj->mat->alpha=alpha; }
void lbAudioAx_80026F2C(u32 mask) { (void)mask; }
void lbAudioAx_8002702C(u32 group, u64 mask) { (void)group; (void)mask; }
void lbAudioAx_80027168(void) {}
void lbAudioAx_80027648(void) {}
u64 lbAudioAx_80026E84(CharacterKind kind) { return 1ull << kind; }
int M360_MatchSupportsStage(unsigned stkind);
bool gm_80164430(u16 stkind) { return M360_MatchSupportsStage(stkind)!=0; }
/* Name editing is pending; resume on the next frame after CSS has set its
 * auxiliary-screen state, so the unavailable screen cannot trap input. */
void mnNameNew_EnterFromMnCharSel(HSD_Archive* archive, s32 slot) {
    (void)archive; (void)slot; M360_MatchTrace("menu.leaf.unported:css_names",0); s_resumeCss=1;
    s_notice="NAME ENTRY NOT PORTED";s_noticeFrames=240;
}

int M360_CharacterSelectEnter(unsigned mode) {
    unsigned i;
    s_mode=mode;
    s_resumeCss=0;
    s_noticeFrames=0;
    memset(s_lastPlayer,0xFF,sizeof(s_lastPlayer));
    memset(&s_css,0,sizeof(s_css));
    memset(s_koCounts,0,sizeof(s_koCounts));
    s_css.ko_counts=s_koCounts;
    s_css.unk_0x0=1;
    s_css.match_type=mode==3?REG_CLASSIC:mode==4?REG_ADVENTURE:VS_MELEE;
    gm_SetupAllPlayerDefaults(s_css.vs.start.players);
    gm_SetupRulesDefaults(&s_css.vs.start.rules);
    for(i=0;i<6;++i) {
        s_css.vs.start.players[i].slot_type=Gm_PKind_NA;
        s_css.vs.start.players[i].ckind=CKind_Playable_Count;
        s_css.vs.start.players[i].nametag=0x78;
        s_css.vs.start.players[i].handicap=9;
        s_css.vs.start.players[i].cpu_level=9;
        s_css.vs.start.players[i].stocks=3;
    }
    for(i=0;i<4;++i)
        if(HSD_PadCopyStatus[i].err==0) s_css.vs.start.players[i].slot_type=Gm_PKind_Human;
    s_css.vs.start.players[0].slot_type=Gm_PKind_Human;
    if(mode==3 || mode==4) s_css.vs.start.players[0].cpu_level=2;
    M360_MenuTextStart();
    M360_MenuSetGameMode(mode);
    M360_MenuResetExit();
    mnCharSel_Scene_OnEnter(&s_css);
    s_active=1;
    M360_MatchTrace("match.select.original",s_css.match_type);
    return 1;
}
void M360_CharacterSelectResume(void) {
    s_resumeCss=0;s_noticeFrames=0;
    memset(s_lastPlayer,0xFF,sizeof(s_lastPlayer));
    M360_MenuTextStart();M360_MenuSetGameMode(s_mode);M360_MenuResetExit();
    mnCharSel_Scene_OnEnter(&s_css);s_active=1;
    M360_MatchTrace("match.select.resume",s_css.match_type);
}
void M360_CharacterSelectAddKOs(const unsigned* counts) {
    unsigned i;
    for(i=0;i<4;++i) {
        unsigned total=s_koCounts[i]+counts[i];
        s_koCounts[i]=(u8)(total>255?255:total);
    }
}
int M360_CharacterSelectActive(void) { return s_active; }
const char* M360_CharacterSelectNotice(void) { return s_noticeFrames?s_notice:NULL; }
void* M360_CharacterSelectData(void) { return &s_css; }
int M360_CharacterSelectFrame(void) {
    int exiting;
    unsigned i;
    if(s_noticeFrames) --s_noticeFrames;
    if(s_active==2) mnStageSel_Scene_OnFrame();
    else mnCharSel_Scene_OnFrame();
    if(s_resumeCss) { s_resumeCss=0; mnCharSel_802640A0(); }
    exiting=M360_MenuTakeExit();
    if(!exiting) {
        HSD_GObj_RunProcs();
        exiting=M360_MenuTakeExit();
    }
    if(s_active==1) {
        static const char* tags[4]={"match.css.player0","match.css.player1","match.css.player2","match.css.player3"};
        for(i=0;i<4;++i) {
            PlayerInitData* p=&s_css.vs.start.players[i];
            unsigned value=p->ckind | (p->color<<8) | (p->slot_type<<16);
            if(value!=s_lastPlayer[i]) { M360_MatchTrace(tags[i],value);s_lastPlayer[i]=value; }
        }
    }
    if(!exiting) return -1;
    if(s_active==2) {
        mnStageSel_Scene_OnExit(NULL);
        if(s_sss.start_game) {
            s_css.vs=s_sss.vs;
            M360_MenuTextStop();
            s_active=0;
            M360_MatchTrace("match.select.stage",s_css.vs.start.rules.stkind);
            return 1;
        }
        if(gm_GetCurrentGameMode()==GM_MENU) {
            M360_MenuTextStop();s_active=0;return 0;
        }
        M360_MenuTextStop();
        mn_8022F138(1,8);
        M360_MenuTextStart();
        mnCharSel_Scene_OnEnter(&s_css);
        s_active=1;
        M360_MatchTrace("match.select.stage.cancel",1);
        return -1;
    }
    mnCharSel_Scene_OnExit(NULL);
    M360_MenuTextPoolReleased();
    s_active=0;
    if(s_css.pending_scene_change==1 && s_mode!=3 && s_mode!=4) {
        if(gmMainLib_GetGameRules()->mode>1) {
            M360_MatchTrace("match.rules.unported",gmMainLib_GetGameRules()->mode);
            mn_8022F138(1,8);
            M360_MenuTextStart();
            mnCharSel_Scene_OnEnter(&s_css);
            s_active=1;
            s_notice="COIN / BONUS MATCHES NOT PORTED";s_noticeFrames=240;
            return -1;
        }
        memset(&s_sss,0,sizeof(s_sss));
        s_sss.vs=s_css.vs;
        gm_80167FC4(&s_sss);
        mn_8022F138(1,8);
        M360_MenuTextStart();
        mnStageSel_Scene_OnEnter(&s_sss);
        s_active=2;
        M360_MatchTrace("match.select.stage.enter",0);
        return -1;
    }
    return s_css.pending_scene_change==1;
}
void M360_CharacterSelectLeave(void) {
    if(s_active) {
        if(s_active==2) { mnStageSel_Scene_OnExit(NULL);M360_MenuTextStop(); }
        else {
            mnCharSel_Scene_OnExit(NULL);
            M360_MenuTextPoolReleased();
        }
        s_active=0;
    }
}
