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
FILE* s_image = 0;
m360_gcm s_gcm;
bool s_mounted = false;
unsigned char* s_track = 0;
m360_hps_stream s_stream;
short s_pcm[kBufferCount][kBufferFrames * M360_HPS_MAX_CHANNELS];
unsigned s_nextBuffer = 0;

bool LoadTrack(const char* track, MeleeAudioStatus* status)
{
    m360_gcm_file file;
    if (!s_mounted || !m360_gcm_find(&s_gcm, track, &file))
        return false;
    status->fileFound = true;
    status->fileSize = file.size;
    s_track = static_cast<unsigned char*>(malloc(file.size));
    return s_track &&
           m360_gcm_read(&s_gcm, &file, 0, s_track, file.size) == file.size;
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

bool M360_AudioInit(const char* isoPath, MeleeAudioStatus* status)
{
    ZeroMemory(status, sizeof(*status));
    status->createResult = status->masterResult = status->sourceResult =
        status->startResult = E_FAIL;
    s_image = fopen(isoPath, "rb");
    s_mounted = s_image && m360_gcm_mount(&s_gcm, s_image) == 0;
    status->createResult = XAudio2Create(&s_engine, 0,
                                         XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(status->createResult))
        return false;
    status->masterResult = s_engine->CreateMasteringVoice(&s_master);
    return s_mounted && SUCCEEDED(status->masterResult);
}

void M360_AudioStop(MeleeAudioStatus* status)
{
    if (s_source) {
        s_source->Stop(0);
        s_source->FlushSourceBuffers();
        s_source->DestroyVoice();
        s_source = 0;
    }
    free(s_track);
    s_track = 0;
    s_nextBuffer = 0;
    status->playing = false;
}

bool M360_AudioPlay(const char* track, MeleeAudioStatus* status)
{
    M360_AudioStop(status);
    status->fileFound = status->headerValid = status->finished = false;
    status->fileSize = status->sampleRate = status->channels = 0;
    status->buffersSubmitted = status->samplesPlayed = 0;
    status->sourceResult = status->startResult = E_FAIL;
    _snprintf(status->track, sizeof(status->track) - 1, "%s", track);
    ++status->trackSwitches;
    if (!s_master || !LoadTrack(track, status) ||
        !m360_hps_stream_open(&s_stream, s_track, status->fileSize))
        return false;
    status->headerValid = true;
    status->sampleRate = s_stream.header.sample_rate;
    status->channels = s_stream.header.channels;

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
    if (!status->playing || !s_source)
        return;
    XAUDIO2_VOICE_STATE state;
    s_source->GetState(&state);
    status->samplesPlayed = static_cast<unsigned>(state.SamplesPlayed);
    unsigned queued = state.BuffersQueued;
    for (; queued < kBufferCount; ++queued) {
        if (!SubmitNext(status))
            break;
    }
    if (!queued)
        status->finished = true;
    Snapshot(status);
}
