#ifndef MELEE360_XDK_MOVIE_H
#define MELEE360_XDK_MOVIE_H

#include <xtl.h>

struct MeleeMovieStatus {
    bool opened;
    unsigned width;
    unsigned height;
    unsigned frameCount;
    unsigned frameRate;
    unsigned indexMs;
    unsigned currentFrame;
    unsigned framesDecoded;
    unsigned framesPresented;
    unsigned framesSkipped;
    unsigned framesLate;
    unsigned decodeErrors;
    unsigned decodeUsLast;
    unsigned decodeUsMax;
    unsigned decodeUsAverage;
    unsigned uploadUsLast;
};

bool M360_MovieInit(IDirect3DDevice9* device, const char* isoPath);
bool M360_MovieOpen(const char* path, const unsigned* rateTable);
void M360_MovieSetTick(unsigned tick);
void M360_MovieDraw(IDirect3DDevice9* device);
bool M360_MovieEnded(void);
void M360_MovieClose(void);
void M360_MovieGetStatus(MeleeMovieStatus* status);

#endif
