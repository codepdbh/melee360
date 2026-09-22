#ifndef MELEE360_XDK_AUDIO_H
#define MELEE360_XDK_AUDIO_H

struct MeleeAudioStatus {
    bool fileFound;
    bool headerValid;
    bool playing;
    unsigned fileSize;
    unsigned sampleRate;
    unsigned channels;
    long createResult;
    long masterResult;
    long sourceResult;
    long startResult;
    unsigned buffersSubmitted;
    unsigned adpcmFrames;
    unsigned blocksEntered;
    unsigned loops;
    unsigned historyMismatches;
    unsigned samplesPlayed;
};

bool M360_AudioStart(const char* isoPath, const char* track,
                     MeleeAudioStatus* status);
void M360_AudioUpdate(MeleeAudioStatus* status);

#endif
