#ifndef MELEE360_XDK_BOOT_H
#define MELEE360_XDK_BOOT_H

struct MeleeBootStatus {
    bool isoOpened;
    bool discValid;
    bool fstMounted;
    bool bannerDecoded;
    bool titleArchiveFound;
    bool titleArchiveValid;
    unsigned entryCount;
    unsigned titleArchiveSize;
    unsigned relocationCount;
    unsigned publicCount;
    unsigned externalCount;
    char gameId[7];
    char title[65];
    char firstPublicSymbol[65];
    char error[96];
    unsigned bannerPixels[96 * 32];
};

bool M360_BootMelee(const char* isoPath, MeleeBootStatus* status);

#endif
