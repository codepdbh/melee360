#include "../../src/xdk/melee_audio_xdk.cpp"
#include <assert.h>

int main(int argc, char** argv)
{
    MeleeAudioStatus status;
    assert(argc == 2);
    assert(M360_AudioInit(argv[1], &status));
    assert(LoadSfxSem());
    unsigned valid = 0, rejected = 0;
    for (unsigned slot = 0; slot < 55; ++slot) {
        unsigned begin = ReadBe32(s_sfxSem + s_sfxSemBankTable + slot * 4);
        unsigned end = slot + 1 < 55
            ? ReadBe32(s_sfxSem + s_sfxSemBankTable + (slot + 1) * 4)
            : s_sfxSemStreamCount;
        unsigned sample;
        assert(begin <= end && end <= s_sfxSemStreamCount);
        /* First ID outside each bank must never spill into the next one. */
        assert(!ResolveSfxSampleId(slot * 10000 + end - begin, &sample));
        assert(!ResolveSfxSampleId(slot * 10000 + 9999, &sample));
        ++rejected;
        for (unsigned local = 0; local < end - begin; ++local)
            if (ResolveSfxSampleId(slot * 10000 + local, &sample))
                ++valid;
    }
    unsigned sample;
    assert(!ResolveSfxSampleId(550000, &sample));
    assert(!ResolveSfxSampleId(0xFFFFFFFFu, &sample));
    assert(valid > 4000);

    const unsigned ids[] = {282, 100105, 110100, 110109};
    static short left[65536], right[65536];
    for (unsigned round = 0; round < 32; ++round) {
        unsigned sampleId, bankId, rate, frames;
        SfxBank* bank;
        const unsigned id = ids[round % 4];
        assert(ResolveSfxSampleId(id, &sampleId));
        assert(ResolveSfxBank(sampleId, id / 10000, &bank, &bankId));
        assert(bank->slot == id / 10000);
        assert(DecodeSfx(bank, bankId, 0, &rate, &frames, left, right));
        const unsigned pan = round % 3 == 0 ? 0 : (round % 3 == 1 ? 64 : 127);
        const unsigned volume = round % 2 ? 80 : 127;
        M360_AudioSfx(id, volume, pan);
        assert(M360_AudioSfxSubmitted() == round + 1);
        assert(lastVoice->captured.size() == frames * 2);
        const float gain = (float) volume / 127.0f;
        const float angle = (float) pan / 127.0f * 3.14159265358979323846f * 0.5f;
        for (unsigned i = 0; i < frames; ++i) {
            assert(lastVoice->captured[i * 2] == (short) (left[i] * gain * (float) cos(angle)));
            assert(lastVoice->captured[i * 2 + 1] == (short) (right[i] * gain * (float) sin(angle)));
        }
    }
    assert(M360_AudioSfxMisses() == 0);
    assert(destroyedPending == 24);
    printf("PASS: %u SEM streams resolve; %u bank boundaries reject spillover; "
           "32 real submissions preserve PCM/pan/volume and live buffers\n", valid, rejected);
    return 0;
}
