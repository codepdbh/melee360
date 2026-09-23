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
    unsigned meshVertexCount;
};

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status);
void M360_TitleEnter(bool openingMode);
bool M360_TitleUpdate(unsigned sceneTick);
bool M360_TitleAnyVisible(void);
void M360_TitleRender(void);
unsigned M360_TitleClearColor(void);
bool M360_DecodeFirstTitleTexture(unsigned** pixels, unsigned* width,
                                  unsigned* height);
void M360_FreeDecodedTitleTexture(unsigned* pixels);

#endif
