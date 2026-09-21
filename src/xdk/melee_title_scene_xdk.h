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
    unsigned materialCount;
    unsigned polygonObjectCount;
    unsigned textureObjectCount;
    unsigned textureImageCount;
    unsigned firstTextureWidth;
    unsigned firstTextureHeight;
    unsigned firstTextureFormat;
};

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status);
void M360_AnimateTitleScene(void);

#endif
