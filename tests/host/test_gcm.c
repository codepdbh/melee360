#include "gcm.h"
#include "dvd_compat.h"

#include <dolphin/dvd.h>

#include <stdio.h>
#include <string.h>

static s32 async_result = -999;

static void read_complete(s32 result, DVDFileInfo *file_info)
{
    async_result = result;
    if (!file_info)
        async_result = -998;
}

int main(int argc, char **argv)
{
    struct m360_gcm gcm;
    struct m360_gcm_file file;
    FILE *image;
    unsigned char banner[4];

    if (argc != 2) {
        fprintf(stderr, "usage: %s melee.iso\n", argv[0]);
        return 2;
    }
    image = fopen(argv[1], "rb");
    if (!image || m360_gcm_mount(&gcm, image) != 0) {
        fprintf(stderr, "could not mount GCM image\n");
        return 1;
    }
    if (!m360_gcm_find(&gcm, "opening.bnr", &file) || file.size < 4 ||
        m360_gcm_read(&gcm, &file, 0, banner, sizeof(banner)) != sizeof(banner) ||
        memcmp(banner, "BNR1", 4) != 0) {
        fprintf(stderr, "opening.bnr validation failed\n");
        m360_gcm_unmount(&gcm);
        fclose(image);
        return 1;
    }
    printf("GCM OK: %u entries, opening.bnr offset=%u size=%u magic=BNR1\n",
           gcm.entry_count, file.offset, file.size);
    m360_gcm_unmount(&gcm);
    fclose(image);

    {
        DVDFileInfo info;
        s32 entry;
        unsigned char async_banner[4];
        char title[65];

        if (m360_dvd_mount_image(argv[1]) != 0 ||
            (entry = DVDConvertPathToEntrynum("/opening.bnr")) < 0 ||
            !DVDFastOpen(entry, &info) || info.length != file.size ||
            DVDReadPrio(&info, banner, sizeof(banner), 0, 2) !=
                (s32)sizeof(banner) ||
            memcmp(banner, "BNR1", 4) != 0 ||
            DVDGetTransferredSize(&info) != (s32)sizeof(banner) ||
            !DVDReadAsyncPrio(&info, async_banner, sizeof(async_banner), 0,
                              read_complete, 2) ||
            async_result != (s32)sizeof(async_banner) ||
            memcmp(async_banner, "BNR1", 4) != 0 ||
            DVDReadPrio(&info, title, 64, 0x1860, 2) != 64 ||
            (title[64] = '\0', strcmp(title, "SUPER SMASH BROS. Melee") != 0) ||
            !DVDClose(&info) || !DVDOpen("opening.bnr", &info) ||
            info.length != file.size || !DVDClose(&info)) {
            fprintf(stderr, "Dolphin DVD compatibility validation failed\n");
            m360_dvd_unmount_image();
            return 1;
        }
        printf("DVD API OK: entry=%d length=%u read=BNR1 title=%s\n", entry,
               info.length, title);
        m360_dvd_unmount_image();
    }
    return 0;
}
