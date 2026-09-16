#ifndef M360_PLATFORM_H
#define M360_PLATFORM_H

#include <stddef.h>

int platform_init(void);
void platform_shutdown(void);
void *platform_alloc(size_t size, size_t alignment);
void platform_free(void *pointer);
void platform_input_poll(void);
unsigned long long platform_get_time(void);

#endif

