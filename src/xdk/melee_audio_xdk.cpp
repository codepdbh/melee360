#include <xtl.h>
#include <xaudio2.h>
#include <stdio.h>
#include <math.h>

#include "melee_audio_xdk.h"
extern "C" {
#include "../common/gcm.h"
#include "../common/hps.h"
#include "../common/dsp_adpcm.h"
}

namespace {

const unsigned kBufferCount = 4;
const unsigned kBufferFrames = 4096;
const unsigned kSfxVoiceCount = 8;
const unsigned kSfxMaxFrames = 65536;
const unsigned kSfxBankCacheCount = 2;

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
IXAudio2SourceVoice* s_sfxVoices[kSfxVoiceCount] = {};
unsigned s_sfxRates[kSfxVoiceCount] = {};
short s_sfxPcm[kSfxVoiceCount][kSfxMaxFrames * 2];
short s_sfxLeftScratch[kSfxMaxFrames];
short s_sfxRightScratch[kSfxMaxFrames];
unsigned s_sfxNextVoice = 0;
unsigned s_sfxSubmitted = 0;
unsigned s_sfxMisses = 0;
unsigned s_sfxBankFallbacks = 0;
unsigned s_sfxBankBase[55] = {};
unsigned s_sfxBankCount[55] = {};
bool s_sfxBankIndexReady = false;
unsigned char* s_sfxSem = 0;
unsigned s_sfxSemSize = 0;
unsigned s_sfxSemBankTable = 0;
unsigned s_sfxSemStreamTable = 0;
unsigned s_sfxSemStreamCount = 0;
bool s_sfxSemReady = false;

struct SfxBank {
    unsigned slot;
    unsigned size;
    unsigned char* bytes;
    unsigned age;
};

SfxBank s_sfxBanks[kSfxBankCacheCount] = {
    { 0xFFFFFFFFu, 0, 0, 0 }, { 0xFFFFFFFFu, 0, 0, 0 }
};
unsigned s_sfxBankAge = 0;

const char* const kSfxBankFiles[] = {
    "main.ssm", "pokemon.ssm", "nr_title.ssm", "nr_select.ssm",
    "nr_1p.ssm", "nr_vs.ssm", "captain.ssm", "clink.ssm",
    "dk.ssm", "drmario.ssm", "falco.ssm", "fox.ssm",
    "gkoopa.ssm", "ice.ssm", "kirby.ssm", "koopa.ssm",
    "link.ssm", "luigi.ssm", "mario.ssm", "mars.ssm",
    "mewtwo.ssm", "ness.ssm", "peach.ssm", "pichu.ssm",
    "pikachu.ssm", "purin.ssm", "samus.ssm", "zs.ssm",
    "yoshi.ssm", "gw.ssm", "ganon.ssm", "emblem.ssm",
    "mhands.ssm", "kirbytm.ssm", "castle.ssm", "corneria.ssm",
    "greatbay.ssm", "kongo.ssm", "mutecity.ssm", "onett.ssm",
    "zebes.ssm", "garden.ssm", "klaid.ssm", "greens.ssm",
    "venom.ssm", "bigblue.ssm", "fourside.ssm", "pupupu.ssm",
    "pstadium.ssm", "1padv.ssm", "ending.ssm", "nr_name.ssm",
    "1pend.ssm", "last.ssm", "end.ssm"
};

unsigned ReadBe16(const unsigned char* p)
{
    return ((unsigned) p[0] << 8) | p[1];
}

unsigned ReadBe32(const unsigned char* p)
{
    return ((unsigned) p[0] << 24) | ((unsigned) p[1] << 16) |
           ((unsigned) p[2] << 8) | p[3];
}

SfxBank* LoadSfxBank(unsigned slot)
{
    unsigned i;
    unsigned victim = 0;
    m360_gcm_file file;
    char path[48];
    SfxBank* bank;
    if (slot >= sizeof(kSfxBankFiles) / sizeof(kSfxBankFiles[0]) || !s_mounted)
        return 0;
    for (i = 0; i < kSfxBankCacheCount; ++i) {
        if (s_sfxBanks[i].slot == slot && s_sfxBanks[i].bytes) {
            s_sfxBanks[i].age = ++s_sfxBankAge;
            return &s_sfxBanks[i];
        }
        if (!s_sfxBanks[i].bytes || s_sfxBanks[i].age < s_sfxBanks[victim].age)
            victim = i;
    }
    _snprintf(path, sizeof(path) - 1, "audio/us/%s", kSfxBankFiles[slot]);
    if (!m360_gcm_find(&s_gcm, path, &file) || !file.size)
        return 0;
    bank = &s_sfxBanks[victim];
    free(bank->bytes);
    bank->bytes = static_cast<unsigned char*>(malloc(file.size));
    bank->slot = 0xFFFFFFFFu;
    bank->size = 0;
    if (!bank->bytes || m360_gcm_read(&s_gcm, &file, 0, bank->bytes, file.size) != file.size) {
        free(bank->bytes);
        bank->bytes = 0;
        return 0;
    }
    bank->slot = slot;
    bank->size = file.size;
    bank->age = ++s_sfxBankAge;
    return bank;
}

bool DecodeSfxVoice(const unsigned char* sampleData, unsigned sampleSize,
                    const unsigned char* voice, unsigned entryRate,
                    unsigned* sampleRate,
                    short* out, unsigned* outFrames)
{
    /* AX addresses for ADPCM are nibble positions. Each frame occupies 16
     * nibbles: a 2-nibble predictor header followed by 14 samples. SSM voices
     * often begin at nibble 2, so decoding from start>>1 as a frame header
     * mistakes the first audio byte for predictor metadata and produces noise. */
    unsigned current = ReadBe32(voice + 12);
    unsigned end = ReadBe32(voice + 8);
    short coefs[16];
    short hist1 = (short) ReadBe16(voice + 16 + 36);
    short hist2 = (short) ReadBe16(voice + 16 + 38);
    unsigned i;
    if (ReadBe16(voice + 2) != 0)
        return false;
    for (i = 0; i < 16; ++i)
        coefs[i] = (short) ReadBe16(voice + 16 + i * 2u);
    *sampleRate = entryRate;
    if (*sampleRate < 8000 || *sampleRate > 48000)
        return false;
    *outFrames = m360_dsp_decode_range(sampleData, sampleSize, current, end,
        (unsigned char) ReadBe16(voice + 16 + 34), coefs, &hist1, &hist2,
        out, kSfxMaxFrames);
    return *outFrames != 0;
}

bool DecodeSfx(const SfxBank* bank, unsigned sfxId, unsigned voiceIndex,
               unsigned* sampleRate, unsigned* outFrames,
               short* outLeft, short* outRight)
{
    const unsigned char* image = bank->bytes;
    unsigned headerSize, sampleBytes, count, base, sampleStart, sampleSize;
    unsigned char* entry;
    unsigned i;
    if (!image || bank->size < 0x20)
        return false;
    headerSize = ReadBe32(image);
    sampleBytes = ReadBe32(image + 4);
    count = ReadBe32(image + 8);
    base = ReadBe32(image + 12);
    if (sampleBytes > bank->size)
        return false;
    sampleStart = bank->size - sampleBytes;
    if (headerSize < 0x10 || count == 0 || count > 0x10000 ||
        sampleStart < 0x10 || headerSize > sampleStart - 0x10 ||
        sfxId < base || sfxId - base >= count)
        return false;
    sampleSize = sampleBytes;
    entry = const_cast<unsigned char*>(image + 0x10);
    for (i = 0; i < sfxId - base; ++i) {
        const unsigned voices = ReadBe32(entry);
        if (!voices || voices > 2)
            return false;
        entry += 8u + voices * 0x40u;
        if ((unsigned) (entry - image) > sampleStart)
            return false;
    }
    {
        const unsigned voices = ReadBe32(entry);
        const unsigned entryRate = ReadBe32(entry + 4);
        unsigned leftFrames = 0, rightFrames = 0;
        unsigned leftRate = 0, rightRate = 0;
        const unsigned char* voiceData;
        if (!voices || voices > 2 || (unsigned) (entry - image) + 8u + voices * 0x40u > sampleStart)
            return false;
        voiceData = entry + 8;
        if (!DecodeSfxVoice(image + sampleStart, sampleSize, voiceData, entryRate,
                            &leftRate, outLeft, &leftFrames))
            return false;
        if (voices > 1 && DecodeSfxVoice(image + sampleStart, sampleSize,
                                         voiceData + 0x40, entryRate, &rightRate,
                                         s_sfxRightScratch, &rightFrames)) {
            const unsigned n = leftFrames < rightFrames ? leftFrames : rightFrames;
            for (i = 0; i < n; ++i)
                outRight[i] = s_sfxRightScratch[i];
            rightFrames = n;
        } else {
            for (i = 0; i < leftFrames; ++i)
                outRight[i] = outLeft[i];
            rightFrames = leftFrames;
        }
        *sampleRate = leftRate;
        *outFrames = leftFrames < rightFrames ? leftFrames : rightFrames;
        (void) voiceIndex;
        return *outFrames != 0;
    }
}

/* sfxId is the SSM sample id from the SEM stream; preferSlot is the SEM bank
 * (sound id / 10000), which is also the ssm_files index (lbaudio_ax.c). SSM id
 * ranges overlap between files that are never loaded together (menu, stage
 * and 1P banks), so the bank's own file must win over a range scan. */
bool ResolveSfxBank(unsigned sfxId, unsigned preferSlot, SfxBank** outBank, unsigned* outBankId)
{
    unsigned slot;
    if (!s_sfxBankIndexReady) {
        for (slot = 0; slot < 55; ++slot) {
            m360_gcm_file file;
            char path[48];
            unsigned char header[16];
            _snprintf(path, sizeof(path) - 1, "audio/us/%s", kSfxBankFiles[slot]);
            if (m360_gcm_find(&s_gcm, path, &file) && file.size >= sizeof(header) &&
                m360_gcm_read(&s_gcm, &file, 0, header, sizeof(header)) == sizeof(header)) {
                s_sfxBankBase[slot] = ReadBe32(header + 12);
                s_sfxBankCount[slot] = ReadBe32(header + 8);
            }
        }
        s_sfxBankIndexReady = true;
    }
    if (preferSlot < 55 && sfxId >= s_sfxBankBase[preferSlot] &&
        sfxId - s_sfxBankBase[preferSlot] < s_sfxBankCount[preferSlot]) {
        *outBank = LoadSfxBank(preferSlot);
        *outBankId = sfxId;
        if (*outBank)
            return true;
    }
    ++s_sfxBankFallbacks;
    for (slot = 0; slot < 55; ++slot) {
        if (sfxId >= s_sfxBankBase[slot] &&
            sfxId - s_sfxBankBase[slot] < s_sfxBankCount[slot]) {
            *outBank = LoadSfxBank(slot);
            *outBankId = sfxId;
            return *outBank != 0;
        }
    }
    return false;
}

bool LoadSfxSem(void)
{
    m360_gcm_file file = {};
    const unsigned char* image;
    unsigned cursor, count;
    const char* paths[] = { "audio/smash2.sem", "audio/us/smash2.sem" };
    unsigned p;
    if (s_sfxSemReady)
        return s_sfxSem != 0;
    s_sfxSemReady = true;
    if (!s_mounted)
        return false;
    for (p = 0; p < sizeof(paths) / sizeof(paths[0]); ++p) {
        if (m360_gcm_find(&s_gcm, paths[p], &file))
            break;
    }
    if (p == sizeof(paths) / sizeof(paths[0]) || file.size < 16)
        return false;
    s_sfxSem = static_cast<unsigned char*>(malloc(file.size));
    if (!s_sfxSem || m360_gcm_read(&s_gcm, &file, 0, s_sfxSem, file.size) != file.size) {
        free(s_sfxSem);
        s_sfxSem = 0;
        return false;
    }
    s_sfxSemSize = file.size;
    image = s_sfxSem;
    cursor = 0;
    count = ReadBe32(image + cursor);
    cursor += 4;
    if (count > 0x10000 || count > (s_sfxSemSize - cursor) / 4)
        return false;
    cursor += count * 4;
    if (cursor + 4 > s_sfxSemSize)
        return false;
    count = ReadBe32(image + cursor);
    cursor += 4;
    if (count > 0x10000 || count > (s_sfxSemSize - cursor) / 4)
        return false;
    cursor += count * 4;
    if (cursor + 4 > s_sfxSemSize)
        return false;
    count = ReadBe32(image + cursor);
    cursor += 4;
    if (count < 55 || count > 0x1000 || count > (s_sfxSemSize - cursor) / 4)
        return false;
    s_sfxSemBankTable = cursor;
    cursor += count * 4;
    if (cursor + 4 > s_sfxSemSize)
        return false;
    s_sfxSemStreamCount = ReadBe32(image + cursor);
    cursor += 4;
    if (s_sfxSemStreamCount > 0x10000 ||
        s_sfxSemStreamCount > (s_sfxSemSize - cursor) / 4)
        return false;
    s_sfxSemStreamTable = cursor;
    return true;
}

bool ResolveSfxSampleId(unsigned sfxId, unsigned* outSampleId)
{
    unsigned bank, bankLocal, sampleIndex, streamOffset, command, i;
    if (!LoadSfxSem())
        return false;
    bank = sfxId / 10000u;
    bankLocal = sfxId % 10000u;
    if (bank >= 55)
        return false;
    sampleIndex = ReadBe32(s_sfxSem + s_sfxSemBankTable + bank * 4u);
    if (sampleIndex > s_sfxSemStreamCount ||
        bankLocal >= s_sfxSemStreamCount - sampleIndex)
        return false;
    sampleIndex += bankLocal;
    streamOffset = ReadBe32(s_sfxSem + s_sfxSemStreamTable + sampleIndex * 4u);
    if (streamOffset > s_sfxSemSize || s_sfxSemSize - streamOffset < 4)
        return false;
    /* SEM command streams can begin with a delay or priority adjustment. The
     * retail AX driver walks these commands until opcode 1 selects the SSM ID. */
    for (i = 0; i < 256 && streamOffset <= s_sfxSemSize - 4; ++i) {
        command = ReadBe32(s_sfxSem + streamOffset);
        if ((command >> 24) == 1) {
            *outSampleId = command & 0x00FFFFFFu;
            return true;
        }
        if ((command >> 24) == 14 || (command >> 24) == 15)
            return false;
        streamOffset += 4;
    }
    return false;
}

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

extern "C" void M360_AudioSfx(unsigned sfxId, unsigned volume, unsigned pan)
{
    static const float kPi = 3.14159265358979323846f;
    const unsigned voiceIndex = s_sfxNextVoice++ % kSfxVoiceCount;
    const unsigned boundedVolume = volume > 127 ? 127 : volume;
    const unsigned boundedPan = pan > 127 ? 127 : pan;
    const float pan01 = (float) boundedPan / 127.0f;
    const float leftGain = (float) cos(pan01 * kPi * 0.5f);
    const float rightGain = (float) sin(pan01 * kPi * 0.5f);
    const float gain = (float) boundedVolume / 127.0f;
    SfxBank* bank = 0;
    unsigned bankId = 0, sampleId = 0;
    unsigned sampleRate = 0, frames = 0, i;
    XAUDIO2_BUFFER buffer;
    if (!s_engine || !s_master || !boundedVolume)
        return;
    if (!ResolveSfxSampleId(sfxId, &sampleId) ||
        !ResolveSfxBank(sampleId, sfxId / 10000u, &bank, &bankId) ||
        !DecodeSfx(bank, bankId, voiceIndex, &sampleRate, &frames,
                            s_sfxLeftScratch,
                            s_sfxRightScratch)) {
        ++s_sfxMisses;
        return;
    }
    if (s_sfxVoices[voiceIndex]) {
        XAUDIO2_VOICE_STATE state;
        s_sfxVoices[voiceIndex]->GetState(&state);
        if (s_sfxRates[voiceIndex] != sampleRate || state.BuffersQueued) {
            /* Stop/Flush can leave the current buffer in use. DestroyVoice
             * waits until XAudio2 no longer reads this slot's PCM memory. */
            s_sfxVoices[voiceIndex]->DestroyVoice();
            s_sfxVoices[voiceIndex] = 0;
            s_sfxRates[voiceIndex] = 0;
        }
    }
    if (s_sfxVoices[voiceIndex]) {
        s_sfxVoices[voiceIndex]->Stop(0);
        s_sfxVoices[voiceIndex]->FlushSourceBuffers();
    } else {
        WAVEFORMATEX format;
        ZeroMemory(&format, sizeof(format));
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = sampleRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = 4;
        format.nAvgBytesPerSec = sampleRate * format.nBlockAlign;
        if (FAILED(s_engine->CreateSourceVoice(&s_sfxVoices[voiceIndex], &format))) {
            ++s_sfxMisses;
            return;
        }
        s_sfxRates[voiceIndex] = sampleRate;
    }
    for (i = 0; i < frames; ++i) {
        s_sfxPcm[voiceIndex][i * 2] = (short) (s_sfxLeftScratch[i] * gain * leftGain);
        s_sfxPcm[voiceIndex][i * 2 + 1] = (short) (s_sfxRightScratch[i] * gain * rightGain);
    }
    ZeroMemory(&buffer, sizeof(buffer));
    buffer.AudioBytes = frames * 2u * sizeof(short);
    buffer.pAudioData = reinterpret_cast<const BYTE*>(s_sfxPcm[voiceIndex]);
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (FAILED(s_sfxVoices[voiceIndex]->SubmitSourceBuffer(&buffer))) {
        ++s_sfxMisses;
        return;
    }
    if (FAILED(s_sfxVoices[voiceIndex]->Start(0))) {
        ++s_sfxMisses;
        return;
    }
    ++s_sfxSubmitted;
}

unsigned M360_AudioSfxSubmitted(void)
{
    return s_sfxSubmitted;
}

unsigned M360_AudioSfxMisses(void)
{
    return s_sfxMisses;
}
