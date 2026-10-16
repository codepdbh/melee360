#include "../../src/xdk/melee_audio_xdk.cpp"
#include <assert.h>
#include "ssm_files_original.h"
#include "stage_music_files.h"

static void TestStageMusic()
{
    static short pcm[4096 * M360_HPS_MAX_CHANNELS];
    for (unsigned i = 0; i < sizeof(stageMusicFiles) / sizeof(stageMusicFiles[0]); ++i) {
        MeleeAudioStatus music = {};
        assert(LoadTrack(stageMusicFiles[i], &music));
        m360_hps_stream stream;
        assert(m360_hps_stream_open(&stream, s_track, music.fileSize));
        m360_hps_block block;
        unsigned offset = M360_HPS_HEADER_SIZE;
        unsigned long long frames = 0;
        unsigned blocks = 0;
        for (;;) {
            assert(m360_hps_parse_block(s_track, music.fileSize, offset,
                                       stream.header.channels, &block));
            frames += m360_hps_block_samples(block.end_nibble);
            assert(++blocks < music.fileSize / M360_HPS_BLOCK_HEADER_SIZE);
            if (block.next <= offset || block.next == M360_HPS_NO_NEXT) break;
            offset = block.next;
        }
        // Stage backgrounds must carry their authored loop, not restart at
        // the beginning as a fallback for a non-looping jingle.
        assert(block.next != M360_HPS_NO_NEXT);
        assert(block.next >= M360_HPS_HEADER_SIZE && block.next <= block.offset);
        frames += 8192; // Cross the authored loop point and decode its next block.
        bool nonzero = false;
        while (frames) {
            const size_t count = frames > 4096 ? 4096 : (size_t)frames;
            assert(m360_hps_stream_read(&stream, pcm, count) == count);
            for (size_t p = 0; p < count * stream.header.channels; ++p)
                if (pcm[p]) nonzero = true;
            frames -= count;
        }
        assert(nonzero && stream.loops > 0 && stream.history_mismatches == 0);
        printf("PASS: stage BGM %s, %u Hz, %u blocks, authored loop decoded\n",
               stageMusicFiles[i], stream.header.sample_rate, blocks);
        free(s_track);
        s_track = NULL;
    }
}

int main(int argc, char** argv)
{
    MeleeAudioStatus status;
    assert(argc == 2);
    assert(M360_AudioInit(argv[1], &status));
    TestStageMusic();
    assert(LoadSfxSem());
    assert(sizeof(originalSsmFiles)/sizeof(originalSsmFiles[0])==56);
    for(unsigned i=0;i<55;++i) assert(strcmp(originalSsmFiles[i],kSfxBankFiles[i])==0);
    unsigned valid = 0, rejected = 0, ownBank = 0, foreignBank = 0, missingBank = 0;
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
            if (ResolveSfxSampleId(slot * 10000 + local, &sample)) {
                ++valid;
                SfxBank* bank;unsigned bankId;
                if(!ResolveSfxBank(sample,slot,&bank,&bankId)) ++missingBank;
                else if(bank->slot==slot) ++ownBank;
                else {
                    if(foreignBank<12) printf("FOREIGN: sound=%u sample=%u expected=%u actual=%u\n",slot*10000+local,sample,slot,bank->slot);
                    ++foreignBank;
                }
            }
    }
    unsigned sample;
    assert(!ResolveSfxSampleId(550000, &sample));
    assert(!ResolveSfxSampleId(0xFFFFFFFFu, &sample));
    assert(valid > 4000);
    printf("AUDIT: own=%u foreign=%u missing=%u\n",ownBank,foreignBank,missingBank);
    assert(ownBank==valid && foreignBank==0 && missingBank==0);
    SfxBank* wrongBank;unsigned wrongId;
    // Dr. Mario's out-of-range Japanese sample number is valid in Falco's US
    // bank. It must be rejected instead of reproducing the regional mismatch.
    assert(!ResolveSfxBank(s_sfxBankBase[9]+s_sfxBankCount[9],9,&wrongBank,&wrongId));
    assert(!ResolveSfxBank(0,55,&wrongBank,&wrongId));

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
    M360_AudioStopAllSfx();
    int trackA = M360_AudioPlaySfx(ids[0], 127, 64, 54);
    int trackA2 = M360_AudioPlaySfx(ids[1], 127, 64, 54);
    int trackB = M360_AudioPlaySfx(ids[2], 127, 64, 55);
    assert(trackA > 0 && trackA2 > 0 && trackB > 0);
    assert(trackA != trackA2 && trackA2 != trackB);
    assert(M360_AudioSetSfxPitch(trackB, 1200));
    assert(lastVoice->pitch == 2.0f);
    assert(M360_AudioSetSfxPitch(trackB, -2400));
    assert(lastVoice->pitch == 0.5f);
    assert(!M360_AudioSetSfxPitch(-1, 100));
    M360_AudioStopSfxTrack(54);
    assert(!M360_AudioSfxPlaying(trackA));
    assert(!M360_AudioSfxPlaying(trackA2));
    assert(M360_AudioSfxPlaying(trackB));
    M360_AudioStopSfx(trackB);
    assert(!M360_AudioSfxPlaying(trackB));
    assert(!M360_AudioSetSfxPitch(trackB, 0));

    int stale = M360_AudioPlaySfx(ids[0], 127, 64, 1);
    int current = -1;
    for (unsigned i = 0; i < kSfxVoiceCount; ++i)
        current = M360_AudioPlaySfx(ids[i % 4], 127, 64, 2);
    assert(!M360_AudioSfxPlaying(stale));
    M360_AudioStopSfx(stale);
    assert(M360_AudioSfxPlaying(current));
    assert(lastVoice->pitch == 1.0f);
    M360_AudioStopSfx(-1);
    M360_AudioStopSfx(0);
    M360_AudioStopSfxTrack(256);
    assert(M360_AudioSfxPlaying(current));
    // Natural completion is reported without stopping unrelated voices.
    lastVoice->pending = false;
    assert(!M360_AudioSfxPlaying(current));
    IXAudio2SourceVoice* finishedVoice = lastVoice;
    int reused = M360_AudioPlaySfx(ids[3], 127, 64, 3);
    assert(reused > 0 && lastVoice == finishedVoice);
    assert(M360_AudioSfxPlaying(reused));
    M360_AudioStopSfx(current);
    assert(M360_AudioSfxPlaying(reused));
    assert(M360_AudioPlaySfx(ids[0], 127, 64, 256) == -1);
    M360_AudioStopAllSfx();
    for (unsigned i = 0; i < kSfxVoiceCount; ++i) {
        assert(!s_sfxVoices[i]);
        assert(!s_sfxHandles[i]);
    }
    printf("PASS: %u SEM streams resolve; %u bank boundaries reject spillover; "
           "32 real submissions preserve PCM/pan/volume and live buffers\n", valid, rejected);
    puts("PASS: track/voice stop, stale tokens, natural completion and scene cleanup");
    return 0;
}
