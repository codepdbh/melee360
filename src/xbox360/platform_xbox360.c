#include "platform.h"
#include "m360_log.h"

#include <malloc.h>
#include <stdlib.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <xenon_soc/xenon_power.h>
#include <ppc/timebase.h>
#include <xenos/xe.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <console/console.h>

#ifndef M360_DISABLE_VIDEO
static struct XenosDevice m360_xenos_device;
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

unsigned long long platform_get_time(void)
{
    return mftb();
}
