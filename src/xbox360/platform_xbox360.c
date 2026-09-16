#include "platform.h"
#include "m360_log.h"

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <diskio/ata.h>
#include <libfat/fat.h>
#include <xenon_sound/sound.h>
#include <xenon_soc/xenon_power.h>
#include <ppc/timebase.h>
#include <xenos/xe.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <console/console.h>

#ifndef M360_DISABLE_VIDEO
static struct XenosDevice m360_xenos_device;
extern int m360_xenos_draw_triangle(struct XenosDevice *device);
#endif

int platform_init(void)
{
#ifndef M360_DISABLE_VIDEO
    struct XenosSurface *framebuffer;

    xenos_init(VIDEO_MODE_AUTO);
    Xe_Init(&m360_xenos_device);
    framebuffer = Xe_GetFramebufferSurface(&m360_xenos_device);
    Xe_SetRenderTarget(&m360_xenos_device, framebuffer);
    edram_init(&m360_xenos_device);
    Xe_InvalidateState(&m360_xenos_device);
    Xe_SetClearColor(&m360_xenos_device, 0x101828ff);
    Xe_Resolve(&m360_xenos_device);
    Xe_Sync(&m360_xenos_device);
    m360_xenos_draw_triangle(&m360_xenos_device);
    console_init();
#endif
    xenon_make_it_faster(XENON_SPEED_FULL);
    usb_init();
    usb_do_poll();
    return 0;
}

void platform_shutdown(void)
{
    exit(0);
}

void *platform_alloc(size_t size, size_t alignment)
{
    return memalign(alignment, size);
}

void platform_free(void *pointer)
{
    free(pointer);
}

void platform_input_poll(void)
{
    usb_do_poll();
}

int platform_input_get(unsigned port, struct m360_input_state *state)
{
    struct controller_data_s pad;
    if (!state || port >= 4 || !get_controller_data(&pad, (int)port))
        return 0;

    memset(state, 0, sizeof(*state));
    state->left_x = pad.s1_x; state->left_y = pad.s1_y;
    state->right_x = pad.s2_x; state->right_y = pad.s2_y;
    state->left_trigger = pad.lt; state->right_trigger = pad.rt;
    state->buttons = (pad.a ? M360_BUTTON_A : 0) | (pad.b ? M360_BUTTON_B : 0) |
        (pad.x ? M360_BUTTON_X : 0) | (pad.y ? M360_BUTTON_Y : 0) |
        (pad.lb ? M360_BUTTON_LB : 0) | (pad.rb ? M360_BUTTON_RB : 0) |
        (pad.start ? M360_BUTTON_START : 0) | (pad.back ? M360_BUTTON_BACK : 0) |
        (pad.up ? M360_BUTTON_UP : 0) | (pad.down ? M360_BUTTON_DOWN : 0) |
        (pad.left ? M360_BUTTON_LEFT : 0) | (pad.right ? M360_BUTTON_RIGHT : 0) |
        (pad.logo ? M360_BUTTON_GUIDE : 0);
    return 1;
}

extern int bdev_enum(int handle, const char **name);

int platform_filesystem_init(void)
{
#ifndef M360_SAFE_MODE
    xenon_ata_init();
    xenon_atapi_init();
#endif
    return fatInitDefault() ? 0 : -1;
}

int platform_find_melee_iso(char *path, size_t path_size, char game_id[7], uint8_t *revision)
{
    static const char *relative_paths[] = {
        "Melee360/melee.iso", "Melee360/MeleeUSAv1.02.iso",
        "melee.iso", "MeleeUSAv1.02.iso"
    };
    const char *device;
    int handle = -1;
    size_t i;

    while ((handle = bdev_enum(handle, &device)) >= 0) {
        for (i = 0; i < sizeof(relative_paths) / sizeof(relative_paths[0]); ++i) {
            unsigned char header[8];
            FILE *file;
            snprintf(path, path_size, "%s:/%s", device, relative_paths[i]);
            file = fopen(path, "rb");
            if (!file)
                continue;
            if (fread(header, 1, sizeof(header), file) == sizeof(header)) {
                fclose(file);
                memcpy(game_id, header, 6);
                game_id[6] = '\0';
                *revision = header[7];
                if (memcmp(game_id, "GALE01", 6) == 0)
                    return 1;
            } else {
                fclose(file);
            }
        }
    }
    path[0] = '\0';
    game_id[0] = '\0';
    *revision = 0;
    return 0;
}

int platform_audio_init(void)
{
#ifdef M360_DISABLE_AUDIO
    return -1;
#else
    xenon_sound_init();
    return 0;
#endif
}

void platform_audio_test_tone(void)
{
#ifndef M360_DISABLE_AUDIO
    xenon_tone(440, 200, XENON_TONE_AMPLITUDE_25);
#endif
}

int platform_video_draw_test_triangle(void)
{
#ifdef M360_DISABLE_VIDEO
    return -1;
#else
    return m360_xenos_draw_triangle(&m360_xenos_device);
#endif
}

unsigned long long platform_get_time(void)
{
    return mftb();
}
