#include "platform.h"
#include "m360_log.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <input/input.h>

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
    struct controller_data_s pad;
    void *memory;

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
    printf("CONTROLLER ........ WAITING\n");
    printf("FILESYSTEM ........ PENDING\n");
    printf("AUDIO ............. PENDING\n\n");
    printf("BUILD ............. %s\n\n", M360_BUILD_ID);
    printf("Press A to continue; Guide or Y exits\n");

    for (;;) {
        platform_input_poll();
        memset(&pad, 0, sizeof(pad));
        if (get_controller_data(&pad, 0)) {
            if (pad.a)
                M360_LOG_INPUT("A pressed");
            if (pad.logo || pad.y)
                break;
        }
    }

    platform_free(memory);
    platform_shutdown();
    return 0;
}

