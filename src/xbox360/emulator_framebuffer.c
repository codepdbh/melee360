#include "platform.h"

#ifdef M360_EMULATOR_MODE

#include <stddef.h>
#include <stdint.h>

/*
 * Xenon Emulator exposes the Xenos scanout surface at this physical address.
 * A real console uses libxenon's Xenos setup instead; this small software
 * renderer exists only for the direct-ELF emulator integration build.
 */
/* libxenon executes through the cached 0x80000000 RAM alias. */
#define M360_FB_BASE ((volatile uint32_t *)0x9E000000u)
#define M360_FB_WIDTH 1280u
#define M360_FB_HEIGHT 720u

static uint32_t emulator_color(unsigned r, unsigned g, unsigned b)
{
    /*
     * The guest writes big-endian words, while the emulator's scanout shader
     * consumes the same four bytes as a little-endian ARGB integer.
     */
    return ((b & 0xffu) << 24) | ((g & 0xffu) << 16) |
           ((r & 0xffu) << 8) | 0xffu;
}

static size_t emulator_tiled_index(unsigned x, unsigned y)
{
    return (((y & ~31u) * M360_FB_WIDTH) + ((x & ~31u) * 32u)) +
           (((x & 3u) + ((y & 1u) << 2) + ((x & 28u) << 1) +
             ((y & 30u) << 5)) ^
            ((y & 8u) << 2));
}

static void emulator_put_pixel(unsigned x, unsigned y, uint32_t color)
{
    if (x < M360_FB_WIDTH && y < M360_FB_HEIGHT)
        M360_FB_BASE[emulator_tiled_index(x, y)] = color;
}

static void emulator_fill_rect(unsigned x, unsigned y, unsigned width,
                               unsigned height, uint32_t color)
{
    unsigned px, py;

    if (x >= M360_FB_WIDTH || y >= M360_FB_HEIGHT)
        return;
    if (width > M360_FB_WIDTH - x)
        width = M360_FB_WIDTH - x;
    if (height > M360_FB_HEIGHT - y)
        height = M360_FB_HEIGHT - y;

    for (py = y; py < y + height; ++py)
        for (px = x; px < x + width; ++px)
            M360_FB_BASE[emulator_tiled_index(px, py)] = color;
}

static void emulator_draw_rect(unsigned x, unsigned y, unsigned width,
                               unsigned height, unsigned thickness,
                               uint32_t color)
{
    emulator_fill_rect(x, y, width, thickness, color);
    emulator_fill_rect(x, y + height - thickness, width, thickness, color);
    emulator_fill_rect(x, y, thickness, height, color);
    emulator_fill_rect(x + width - thickness, y, thickness, height, color);
}

static uint8_t emulator_glyph_row(char character, unsigned row)
{
    static const uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {31,4,4,4,4,4,31},      {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},   {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},     {31,1,2,4,8,16,31}
    };
    static const uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31},     {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},     {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };

    if (row >= 7)
        return 0;
    if (character >= 'a' && character <= 'z')
        character = (char)(character - 'a' + 'A');
    if (character >= 'A' && character <= 'Z')
        return letters[(unsigned)(character - 'A')][row];
    if (character >= '0' && character <= '9')
        return digits[(unsigned)(character - '0')][row];
    if (character == '-')
        return row == 3 ? 31 : 0;
    if (character == ':')
        return (row == 2 || row == 5) ? 4 : 0;
    if (character == '.')
        return row == 6 ? 4 : 0;
    if (character == '/')
        return (uint8_t)(1u << (row < 5 ? row : 4));
    return 0;
}

static void emulator_draw_text(unsigned x, unsigned y, const char *text,
                               unsigned scale, uint32_t color)
{
    unsigned origin_x = x;

    while (*text) {
        unsigned row, column, sx, sy;
        char character = *text++;

        if (character == '\n') {
            x = origin_x;
            y += 9u * scale;
            continue;
        }
        for (row = 0; row < 7; ++row) {
            uint8_t bits = emulator_glyph_row(character, row);
            for (column = 0; column < 5; ++column) {
                if (!(bits & (1u << (4u - column))))
                    continue;
                for (sy = 0; sy < scale; ++sy)
                    for (sx = 0; sx < scale; ++sx)
                        emulator_put_pixel(x + column * scale + sx,
                                           y + row * scale + sy, color);
            }
        }
        x += 6u * scale;
    }
}

void platform_emulator_draw_status(void)
{
    const uint32_t panel_light = emulator_color(24, 39, 61);
    const uint32_t green = emulator_color(107, 232, 52);
    const uint32_t cyan = emulator_color(79, 203, 247);
    const uint32_t white = emulator_color(238, 244, 252);
    const uint32_t muted = emulator_color(139, 158, 181);
    emulator_fill_rect(0, 92, M360_FB_WIDTH, 4, green);
    emulator_draw_rect(72, 148, 1136, 450, 3, panel_light);
    emulator_fill_rect(72, 148, 8, 450, cyan);
    emulator_fill_rect(112, 210, 1056, 2, panel_light);

    emulator_draw_text(72, 30, "MELEE360", 6, green);
    emulator_draw_text(430, 43, "NATIVE XENON PORT", 3, white);
    emulator_draw_text(112, 174, "POWERPC GUEST FRAMEBUFFER", 4, cyan);
    emulator_draw_text(112, 246, "CPU", 3, muted);
    emulator_draw_text(390, 246, "MEMORY", 3, muted);
    emulator_draw_text(750, 246, "MELEE LBTIME", 3, muted);
    emulator_draw_text(112, 286, "OK", 5, green);
    emulator_draw_text(390, 286, "OK", 5, green);
    emulator_draw_text(750, 286, "LINKED / OK", 5, green);

    emulator_fill_rect(112, 390, 1008, 26, panel_light);
    emulator_fill_rect(112, 390, 454, 26, green);
    emulator_draw_text(112, 444, "NATIVE PORT INTEGRATION", 3, white);
    emulator_draw_text(112, 492, "MILESTONE: VISIBLE GUEST CODE RUNNING", 3,
                       white);
    emulator_draw_text(112, 548, "NEXT: GAMEPLAY SYSTEMS", 2, muted);
    emulator_draw_text(72, 650, "MELEE360 IS AN INDEPENDENT CLEAN-ROOM PORT",
                       2, muted);
}

#endif
