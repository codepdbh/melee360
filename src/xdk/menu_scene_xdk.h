#ifndef MELEE360_XDK_MENU_SCENE_H
#define MELEE360_XDK_MENU_SCENE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    M360_MENU_CONTINUE = 0,
    M360_MENU_TO_TITLE = 1,
    M360_MENU_RESTART = 2
};

void M360_MenuSceneEnter(unsigned kind, unsigned selection);
int M360_MenuSceneFrame(void);
void M360_MenuSceneRender(void);
void M360_MenuSceneLeave(void);
void M360_MenuSceneState(unsigned* kind, unsigned* selection);

void M360_MenuTrace(const char* stage, unsigned value);
void* M360_MenuSymbol(const char* name);
void M360_MenuPlayBgm(int bgm);
int M360_MenuBgmChoice(void);

#ifdef __cplusplus
}
#endif

#endif
