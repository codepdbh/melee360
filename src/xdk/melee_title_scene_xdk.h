#ifndef MELEE360_XDK_TITLE_SCENE_H
#define MELEE360_XDK_TITLE_SCENE_H

struct MeleeTitleSceneStatus {
    bool symbolsResolved;
    bool modelsLoaded;
    bool animationsBound;
    unsigned expectedSymbols;
    unsigned resolvedSymbols;
    unsigned modelCount;
    unsigned jointCount;
    unsigned displayObjectCount;
};

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status);

#endif
