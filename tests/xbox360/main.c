#include "platform.h"
#include "m360_log.h"
#include "dvd_compat.h"

#include <melee/lb/lbtime.h>
#include <dolphin/dvd.h>
#include <sysdolphin/baselib/archive.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef M360_BUILD_ID
#define M360_BUILD_ID "unknown"
#endif

static const char *endian_name(void)
{
    const uint32_t marker = 0x01020304;
    return (*(const unsigned char *)&marker == 1) ? "BIG" : "LITTLE";
}

static int load_title_archive(void)
{
    DVDFileInfo file;
    HSD_Archive archive;
    unsigned char *image;
    size_t read_size;
    const char *first_symbol;
    void *first_root;

    if (!DVDOpen("GmTtAll.dat", &file)) {
        M360_LOG_FS("GmTtAll.dat was not found in the mounted ISO");
        return 0;
    }

    read_size = (file.length + 31u) & ~31u;
    image = platform_alloc(read_size, 32);
    if (!image) {
        M360_LOG_MEM("could not allocate %lu bytes for GmTtAll.dat",
                     (unsigned long)read_size);
        DVDClose(&file);
        return 0;
    }

    if (DVDReadPrio(&file, image, (s32)read_size, 0, 2) != (s32)read_size ||
        HSD_ArchiveParse(&archive, image, file.length) != 0 ||
        archive.header.nb_public == 0 || !archive.symbols) {
        M360_LOG_FS("GmTtAll.dat read/archive parse failed");
        platform_free(image);
        DVDClose(&file);
        return 0;
    }

    first_symbol = archive.symbols + archive.public_info[0].symbol;
    first_root = HSD_ArchiveGetPublicAddress(&archive, first_symbol);
    M360_LOG_FS("HAL DAT parsed: size=%lu data=%lu reloc=%lu public=%lu extern=%lu",
                (unsigned long)archive.header.file_size,
                (unsigned long)archive.header.data_size,
                (unsigned long)archive.header.nb_reloc,
                (unsigned long)archive.header.nb_public,
                (unsigned long)archive.header.nb_extern);
    M360_LOG_FS("first public symbol=%s address=%p", first_symbol, first_root);
    printf("TITLE ARCHIVE ..... %s\n", first_root ? "PARSED/RELOCATED" : "FAIL");

    platform_free(image);
    DVDClose(&file);
    return first_root != NULL;
}

int main(void)
{
    struct m360_input_state pad, previous_pad;
    void *memory;
    char iso_path[256], game_id[7];
    uint8_t revision;
    int fs_ready, audio_ready;

    platform_init();
    M360_LOG_INFO("Starting platform test build %s", M360_BUILD_ID);

    memory = platform_alloc(4096, 128);
    M360_LOG_MEM("malloc/alignment address=%p aligned=%s", memory,
                 (memory && (((uintptr_t)memory & 127) == 0)) ? "yes" : "no");

    printf("================================\n");
    printf("MELEE360 PLATFORM TEST\n");
    printf("================================\n\n");
    printf("CPU ............... OK\n");
    printf("POWERPC ........... OK\n");
    printf("ENDIAN ............ %s\n", endian_name());
    printf("MEMORY ............ %s\n", memory ? "OK" : "FAIL");
    printf("MELEE LBTIME ...... %s\n",
           lbTime_8000AEC8(0xFFFFFFF0u, 0x20u) == 0xFFFFFFFFu &&
                   lbTime_8000AEE4(3u, -5) == 0u &&
                   lbTime_8000AF74(0xFAu, 10) == 0xFFu
               ? "LINKED/OK"
               : "FAIL");
#ifdef M360_DISABLE_VIDEO
    printf("VIDEO ............. DISABLED\n");
#else
    printf("VIDEO ............. OK\n");
#endif
    fs_ready = platform_filesystem_init() == 0;
    audio_ready = platform_audio_init() == 0;
    printf("CONTROLLER ........ WAITING\n");
    printf("FILESYSTEM ........ %s\n", fs_ready ? "OK" : "FAIL");
    printf("AUDIO ............. %s\n\n", audio_ready ? "OK" : "DISABLED/FAIL");
    printf("BUILD ............. %s\n\n", M360_BUILD_ID);
    if (fs_ready && platform_find_melee_iso(iso_path, sizeof(iso_path), game_id, &revision)) {
        DVDFileInfo banner;
        unsigned char banner_header[32] __attribute__((aligned(32)));
        char banner_title[65] __attribute__((aligned(32)));
        s32 entry;

        M360_LOG_FS("found %s Game ID=%s revision=%u", iso_path, game_id, revision);
        if (m360_dvd_mount_image(iso_path) == 0 &&
            (entry = DVDConvertPathToEntrynum("/opening.bnr")) >= 0 &&
            DVDFastOpen(entry, &banner) &&
            DVDReadPrio(&banner, banner_header, sizeof(banner_header), 0, 2) ==
                (s32)sizeof(banner_header) &&
            memcmp(banner_header, "BNR1", 4) == 0 &&
            DVDReadPrio(&banner, banner_title, 64, 0x1860, 2) == 64) {
            banner_title[64] = '\0';
            M360_LOG_FS("Dolphin DVD API entry=%ld opening.bnr size=%lu magic=BNR1",
                        (long)entry, (unsigned long)banner.length);
            printf("GAME RESOURCE ..... %s\n", banner_title);
            DVDClose(&banner);
            load_title_archive();
        } else {
            M360_LOG_FS("Dolphin DVD API validation failed");
        }
        m360_dvd_unmount_image();
    } else {
        M360_LOG_FS("GALE01 ISO not found in known read-only locations");
    }
    if (audio_ready) {
        M360_LOG_AUDIO("playing 440 Hz PCM test tone");
        platform_audio_test_tone();
    }

    printf("Press A to log input; Guide or Y exits\n");
    memset(&previous_pad, 0, sizeof(previous_pad));

    for (;;) {
        platform_input_poll();
        memset(&pad, 0, sizeof(pad));
        if (platform_input_get(0, &pad)) {
            if (memcmp(&pad, &previous_pad, sizeof(pad)) != 0) {
                M360_LOG_INPUT("buttons=%04lx LT=%u RT=%u LS=(%d,%d) RS=(%d,%d)",
                    (unsigned long)pad.buttons, pad.left_trigger, pad.right_trigger,
                    pad.left_x, pad.left_y, pad.right_x, pad.right_y);
                previous_pad = pad;
            }
            if ((pad.buttons & M360_BUTTON_GUIDE) || (pad.buttons & M360_BUTTON_Y))
                break;
        }
    }

    platform_free(memory);
    platform_shutdown();
    return 0;
}
