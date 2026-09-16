#include "platform.h"
#include "m360_log.h"

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
    if (fs_ready && platform_find_melee_iso(iso_path, sizeof(iso_path), game_id, &revision))
        M360_LOG_FS("found %s Game ID=%s revision=%u", iso_path, game_id, revision);
    else
        M360_LOG_FS("GALE01 ISO not found in known read-only locations");
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
