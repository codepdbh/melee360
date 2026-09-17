#ifndef M360_PLATFORM_H
#define M360_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

struct m360_input_state {
    int16_t left_x, left_y, right_x, right_y;
    uint8_t left_trigger, right_trigger;
    uint32_t buttons;
};

enum {
    M360_BUTTON_A = 1u << 0, M360_BUTTON_B = 1u << 1,
    M360_BUTTON_X = 1u << 2, M360_BUTTON_Y = 1u << 3,
    M360_BUTTON_LB = 1u << 4, M360_BUTTON_RB = 1u << 5,
    M360_BUTTON_START = 1u << 6, M360_BUTTON_BACK = 1u << 7,
    M360_BUTTON_UP = 1u << 8, M360_BUTTON_DOWN = 1u << 9,
    M360_BUTTON_LEFT = 1u << 10, M360_BUTTON_RIGHT = 1u << 11,
    M360_BUTTON_GUIDE = 1u << 12
};

int platform_init(void);
void platform_shutdown(void);
void *platform_alloc(size_t size, size_t alignment);
void platform_free(void *pointer);
void platform_input_poll(void);
int platform_input_get(unsigned port, struct m360_input_state *state);
int platform_filesystem_init(void);
int platform_find_melee_iso(char *path, size_t path_size, char game_id[7], uint8_t *revision);
int platform_audio_init(void);
void platform_audio_test_tone(void);
int platform_video_draw_test_triangle(void);
#ifdef M360_EMULATOR_MODE
void platform_emulator_draw_status(void);
#endif
unsigned long long platform_get_time(void);

#endif
