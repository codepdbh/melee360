#include <xtl.h>
#include <xaudio2.h>
#include <stdio.h>

#include "melee_audio_xdk.h"
extern "C" {
#include "../common/gcm.h"
#include "../common/hps.h"
}

namespace {

const unsigned kBufferCount = 4;
const unsigned kBufferFrames = 4096;

IXAudio2* s_engine = 0;
IXAudio2MasteringVoice* s_master = 0;
IXAudio2SourceVoice* s_source = 0;
unsigned char* s_track = 0;
m360_hps_stream s_stream;
short s_pcm[kBufferCount][kBufferFrames * M360_HPS_MAX_CHANNELS];
unsigned s_nextBuffer = 0;

bool LoadTrack(const char* isoPath, const char* track,
               MeleeAudioStatus* status)
{
    FILE* image = fopen(isoPath, "rb");
    if (!image)
        return false;
    m360_gcm gcm;
    bool loaded = false;
    if (m360_gcm_mount(&gcm, image) == 0) {
        m360_gcm_file file;
        if (m360_gcm_find(&gcm, track, &file)) {
            status->fileFound = true;
            status->fileSize = file.size;
            s_track = static_cast<unsigned char*>(malloc(file.size));
            loaded = s_track &&
                     m360_gcm_read(&gcm, &file, 0, s_track, file.size) ==
                         file.size;
        }
        m360_gcm_unmount(&gcm);
    }
    fclose(image);
    return loaded;
}

bool SubmitNext(MeleeAudioStatus* status)
{
    short* pcm = s_pcm[s_nextBuffer];
    const size_t frames =
        m360_hps_stream_read(&s_stream, pcm, kBufferFrames);
    if (!frames)
        return false;
    XAUDIO2_BUFFER buffer;
    ZeroMemory(&buffer, sizeof(buffer));
    buffer.AudioBytes = static_cast<UINT32>(
        frames * s_stream.header.channels * sizeof(short));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(pcm);
    if (FAILED(s_source->SubmitSourceBuffer(&buffer)))
        return false;
    s_nextBuffer = (s_nextBuffer + 1) % kBufferCount;
    ++status->buffersSubmitted;
    return true;
}

void Snapshot(MeleeAudioStatus* status)
{
    status->adpcmFrames = s_stream.frames_decoded;
    status->blocksEntered = s_stream.blocks_entered;
    status->loops = s_stream.loops;
    status->historyMismatches = s_stream.history_mismatches;
}

} // namespace

bool M360_AudioStart(const char* isoPath, const char* track,
                     MeleeAudioStatus* status)
{
    ZeroMemory(status, sizeof(*status));
    status->createResult = status->masterResult = status->sourceResult =
        status->startResult = E_FAIL;
    if (!LoadTrack(isoPath, track, status) ||
        !m360_hps_stream_open(&s_stream, s_track, status->fileSize))
        return false;
    status->headerValid = true;
    status->sampleRate = s_stream.header.sample_rate;
    status->channels = s_stream.header.channels;

    status->createResult = XAudio2Create(&s_engine, 0,
                                         XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(status->createResult))
        return false;
    status->masterResult = s_engine->CreateMasteringVoice(&s_master);
    if (FAILED(status->masterResult))
        return false;

    WAVEFORMATEX format;
    ZeroMemory(&format, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = static_cast<WORD>(status->channels);
    format.nSamplesPerSec = status->sampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(status->channels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    status->sourceResult = s_engine->CreateSourceVoice(&s_source, &format);
    if (FAILED(status->sourceResult))
        return false;

    for (unsigned i = 0; i < kBufferCount; ++i)
        SubmitNext(status);
    status->startResult = s_source->Start(0);
    status->playing = SUCCEEDED(status->startResult);
    Snapshot(status);
    return status->playing;
}

void M360_AudioUpdate(MeleeAudioStatus* status)
{
    if (!status->playing)
        return;
    XAUDIO2_VOICE_STATE state;
    s_source->GetState(&state);
    status->samplesPlayed = static_cast<unsigned>(state.SamplesPlayed);
    for (unsigned queued = state.BuffersQueued; queued < kBufferCount;
         ++queued) {
        if (!SubmitNext(status))
            break;
    }
    Snapshot(status);
}
