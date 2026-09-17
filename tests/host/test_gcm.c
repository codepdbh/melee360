#include "gcm.h"
#include "dvd_compat.h"

#include <dolphin/dvd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static s32 async_result = -999;

static void read_complete(s32 result, DVDFileInfo *file_info)
{
    async_result = result;
    if (!file_info)
        async_result = -998;
}

static unsigned read_be32(const unsigned char *data)
{
    return ((unsigned)data[0] << 24) | ((unsigned)data[1] << 16) |
           ((unsigned)data[2] << 8) | data[3];
}

static int validate_title_dat(void)
{
    DVDFileInfo info;
    unsigned char *data;
    unsigned file_size, data_size, reloc_count, public_count, extern_count;
    size_t public_offset, symbol_offset;
    const char *first_symbol;

    if (!DVDOpen("GmTtAll.dat", &info) || info.length < 0x20)
        return 0;
    data = malloc(info.length);
    if (!data)
        return 0;
    if (DVDReadPrio(&info, data, (s32)info.length, 0, 2) !=
        (s32)info.length) {
        free(data);
        return 0;
    }

    file_size = read_be32(data + 0x00);
    data_size = read_be32(data + 0x04);
    reloc_count = read_be32(data + 0x08);
    public_count = read_be32(data + 0x0c);
    extern_count = read_be32(data + 0x10);
    public_offset = 0x20u + data_size + (size_t)reloc_count * 4u;
    symbol_offset = public_offset + (size_t)public_count * 8u +
                    (size_t)extern_count * 8u;
    if (file_size != info.length || public_count == 0 ||
        public_offset + 8u > info.length || symbol_offset >= info.length) {
        free(data);
        return 0;
    }
    first_symbol = (const char *)data + symbol_offset +
                   read_be32(data + public_offset + 4u);
    if ((const unsigned char *)first_symbol >= data + info.length ||
        !memchr(first_symbol, '\0', (size_t)(data + info.length -
                                             (const unsigned char *)first_symbol))) {
        free(data);
        return 0;
    }
    printf("DAT OK: GmTtAll.dat size=%u data=%u reloc=%u public=%u extern=%u first=%s\n",
           file_size, data_size, reloc_count, public_count, extern_count,
           first_symbol);
    free(data);
    return DVDClose(&info);
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
        if (!validate_title_dat()) {
            fprintf(stderr, "GmTtAll.dat validation failed\n");
            m360_dvd_unmount_image();
            return 1;
        }
        m360_dvd_unmount_image();
    }
    return 0;
}
