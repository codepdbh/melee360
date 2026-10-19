#ifndef M360_STAGE_MUSIC_XDK_H
#define M360_STAGE_MUSIC_XDK_H

enum {
    M360_MUSIC_MARTH = 1,
    M360_MUSIC_YOUNG_LINK = 2,
    M360_MUSIC_TROPHY_11A = 4,
    M360_MUSIC_ALL_CHARACTERS = 8,
    M360_MUSIC_ALL_STAGES = 16
};

/* Original Ground_801C24F8 flags: 4 random, 0x40 L/R, 0x20 VS. */
int M360_StageMusicChoose(void* param, unsigned stkind, unsigned flags,
                         unsigned character, unsigned unlocks, int* alternate);
unsigned M360_StageMusicInput(void* start);

#endif
