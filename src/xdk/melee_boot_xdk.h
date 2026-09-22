#ifndef MELEE360_XDK_BOOT_H
#define MELEE360_XDK_BOOT_H

#include "melee_title_scene_xdk.h"

struct MeleeBootStatus {
    bool isoOpened;
    bool discValid;
    bool fstMounted;
    bool bannerDecoded;
    bool titleArchiveFound;
    bool titleArchiveValid;
    bool titleArchiveRelocated;
    bool titlePublicRootResolved;
    bool languageUS;
    unsigned entryCount;
    unsigned titleArchiveSize;
    unsigned relocationCount;
    unsigned publicCount;
    unsigned externalCount;
    char gameId[7];
    char title[65];
    char firstPublicSymbol[65];
    char error[96];
    MeleeTitleSceneStatus titleScene;
    unsigned bannerPixels[96 * 32];
};

bool M360_BootMelee(const char* isoPath, MeleeBootStatus* status);

#endif
