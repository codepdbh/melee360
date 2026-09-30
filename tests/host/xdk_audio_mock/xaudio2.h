#pragma once
#include <vector>
#include <assert.h>
#define XAUDIO2_DEFAULT_PROCESSOR 0
#define XAUDIO2_END_OF_STREAM 0x0040
struct XAUDIO2_BUFFER { unsigned Flags, AudioBytes; const BYTE* pAudioData; };
struct XAUDIO2_VOICE_STATE { unsigned BuffersQueued; unsigned long long SamplesPlayed; };
class IXAudio2SourceVoice;
static IXAudio2SourceVoice* lastVoice;
static unsigned destroyedPending;
class IXAudio2SourceVoice {
public:
    const BYTE* active;
    std::vector<short> captured;
    bool pending;
    IXAudio2SourceVoice() : active(0), pending(false) {}
    HRESULT Stop(unsigned) { return S_OK; }
    HRESULT FlushSourceBuffers() { return S_OK; }
    HRESULT Start(unsigned) { return S_OK; }
    void DestroyVoice() {
        if (pending) {
            assert(memcmp(active, &captured[0], captured.size()*2) == 0);
            ++destroyedPending;
        }
        delete this;
    }
    void GetState(XAUDIO2_VOICE_STATE* state) {
        state->BuffersQueued = pending ? 1 : 0;
        state->SamplesPlayed = 0;
    }
    HRESULT SubmitSourceBuffer(const XAUDIO2_BUFFER* buffer) {
        assert(!pending);
        active = buffer->pAudioData;
        const short* pcm = reinterpret_cast<const short*>(active);
        captured.assign(pcm, pcm + buffer->AudioBytes/2);
        pending = true;
        lastVoice = this;
        return S_OK;
    }
};
class IXAudio2MasteringVoice {};
class IXAudio2 {
public:
    HRESULT CreateMasteringVoice(IXAudio2MasteringVoice** voice) {
        *voice = new IXAudio2MasteringVoice(); return S_OK;
    }
    HRESULT CreateSourceVoice(IXAudio2SourceVoice** voice, const WAVEFORMATEX*) {
        *voice = new IXAudio2SourceVoice(); return S_OK;
    }
};
static HRESULT XAudio2Create(IXAudio2** engine, unsigned, unsigned) {
    *engine = new IXAudio2(); return S_OK;
}
