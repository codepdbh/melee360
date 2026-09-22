#include <xtl.h>
#include <stdio.h>

#include "melee_movie_xdk.h"
#include "sprite_vs.h"
#include "movie_ps.h"
extern "C" {
#include "../common/gcm.h"
#include "../common/jpeg_decode.h"
#include "../common/mth.h"
}

namespace {

const unsigned kSlots = 10;
const unsigned kLookahead = 8;
const unsigned kWorkers = 2;
const unsigned kMaxWidth = 640;
const unsigned kMaxHeight = 480;

enum SlotState { kFree, kDecoding, kReady, kUploading };

struct Slot {
    SlotState state;
    unsigned frame;
    unsigned char* planes[3];
};

struct Vertex {
    float x, y, z, w;
    float red, green, blue, alpha;
    float u, v;
};

IDirect3DVertexShader9* s_vertexShader = 0;
IDirect3DPixelShader9* s_pixelShader = 0;
IDirect3DVertexDeclaration9* s_declaration = 0;
IDirect3DTexture9* s_textures[2][3];
unsigned s_textureSet = 0;
bool s_hasFrame = false;

char s_isoPath[128];
FILE* s_indexImage = 0;
m360_gcm s_gcm;
bool s_mounted = false;
char s_loadedPath[64];
m360_mth_header s_header;
unsigned s_fileBase = 0;
unsigned* s_offsets = 0;
unsigned* s_sizes = 0;
const unsigned* s_rateTable = 0;

CRITICAL_SECTION s_lock;
HANDLE s_threads[kWorkers];
volatile bool s_quit = false;
Slot s_slots[kSlots];
unsigned s_next = 0;
unsigned s_target = 0;
unsigned s_uploaded = 0xFFFFFFFFu;
unsigned s_tick = 0;
LARGE_INTEGER s_frequency;
unsigned __int64 s_decodeUsTotal = 0;
MeleeMovieStatus s_status;

unsigned ElapsedUs(const LARGE_INTEGER& start)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<unsigned>((now.QuadPart - start.QuadPart) * 1000000 /
                                 s_frequency.QuadPart);
}

DWORD WINAPI DecodeWorker(void*)
{
    FILE* image = fopen(s_isoPath, "rb");
    unsigned char* compressed = static_cast<unsigned char*>(malloc(256 * 1024));
    for (;;) {
        int slot = -1;
        unsigned frame = 0;
        EnterCriticalSection(&s_lock);
        if (s_quit) {
            LeaveCriticalSection(&s_lock);
            break;
        }
        const unsigned target = s_target;
        if (s_next < target) {
            s_status.framesSkipped += target - s_next;
            s_next = target;
        }
        if (s_next < s_header.frame_count && s_next < target + kLookahead) {
            for (unsigned i = 0; i < kSlots; ++i) {
                if (s_slots[i].state == kFree ||
                    (s_slots[i].state == kReady && s_slots[i].frame < target)) {
                    slot = static_cast<int>(i);
                    break;
                }
            }
            if (slot >= 0) {
                frame = s_next++;
                s_slots[slot].state = kDecoding;
                s_slots[slot].frame = frame;
            }
        }
        LeaveCriticalSection(&s_lock);
        if (slot < 0) {
            Sleep(1);
            continue;
        }

        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
        const unsigned size = s_sizes[frame];
        bool ok = image && compressed && size > M360_MTH_FRAME_PREFIX &&
                  size <= 256 * 1024 &&
                  fseek(image, static_cast<long>(s_fileBase + s_offsets[frame]),
                        SEEK_SET) == 0 &&
                  fread(compressed, 1, size, image) == size;
        if (ok) {
            unsigned pitches[3] = { kMaxWidth, kMaxWidth / 2, kMaxWidth / 2 };
            m360_jpeg_info info;
            ok = m360_jpeg_decode(compressed + M360_MTH_FRAME_PREFIX,
                                  size - M360_MTH_FRAME_PREFIX, M360_JPEG_THP,
                                  s_slots[slot].planes, pitches, &info) &&
                 info.width == s_header.width && info.height == s_header.height;
        }
        const unsigned us = ElapsedUs(start);
        EnterCriticalSection(&s_lock);
        s_slots[slot].state = ok ? kReady : kFree;
        if (ok) {
            ++s_status.framesDecoded;
            s_decodeUsTotal += us;
            s_status.decodeUsLast = us;
            if (us > s_status.decodeUsMax)
                s_status.decodeUsMax = us;
            s_status.decodeUsAverage = static_cast<unsigned>(
                s_decodeUsTotal / s_status.framesDecoded);
        } else {
            ++s_status.decodeErrors;
        }
        LeaveCriticalSection(&s_lock);
    }
    free(compressed);
    if (image)
        fclose(image);
    return 0;
}

bool BuildIndex(const char* path)
{
    if (!_stricmp(s_loadedPath, path))
        return true;
    m360_gcm_file file;
    unsigned char head[M360_MTH_HEADER_SIZE];
    if (!s_mounted || !m360_gcm_find(&s_gcm, path, &file) ||
        m360_gcm_read(&s_gcm, &file, 0, head, sizeof(head)) != sizeof(head) ||
        !m360_mth_parse_header(head, sizeof(head), &s_header) ||
        s_header.width > kMaxWidth || s_header.height > kMaxHeight)
        return false;
    free(s_offsets);
    free(s_sizes);
    s_offsets = static_cast<unsigned*>(malloc(s_header.frame_count * sizeof(unsigned)));
    s_sizes = static_cast<unsigned*>(malloc(s_header.frame_count * sizeof(unsigned)));
    if (!s_offsets || !s_sizes)
        return false;
    unsigned offset = s_header.first_frame;
    unsigned size = s_header.first_frame_size;
    for (unsigned i = 0; i < s_header.frame_count; ++i) {
        unsigned char next[4];
        if (offset + size > file.size ||
            m360_gcm_read(&s_gcm, &file, offset, next, 4) != 4)
            return false;
        s_offsets[i] = offset;
        s_sizes[i] = size;
        offset += size;
        size = m360_mth_next_frame_size(next);
    }
    s_fileBase = file.offset;
    _snprintf(s_loadedPath, sizeof(s_loadedPath) - 1, "%s", path);
    return true;
}

void StopWorkers()
{
    if (!s_threads[0])
        return;
    s_quit = true;
    WaitForMultipleObjects(kWorkers, s_threads, TRUE, INFINITE);
    for (unsigned i = 0; i < kWorkers; ++i) {
        CloseHandle(s_threads[i]);
        s_threads[i] = 0;
    }
    s_quit = false;
}

void Upload(const Slot& slot)
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);
    s_textureSet ^= 1;
    for (unsigned plane = 0; plane < 3; ++plane) {
        IDirect3DTexture9* texture = s_textures[s_textureSet][plane];
        const unsigned width = plane ? s_header.width / 2 : s_header.width;
        const unsigned height = plane ? s_header.height / 2 : s_header.height;
        const unsigned pitch = plane ? kMaxWidth / 2 : kMaxWidth;
        texture->BlockUntilNotBusy();
        D3DLOCKED_RECT locked;
        if (FAILED(texture->LockRect(0, &locked, 0, 0)))
            continue;
        for (unsigned y = 0; y < height; ++y)
            memcpy(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch,
                   slot.planes[plane] + y * pitch, width);
        texture->UnlockRect(0);
    }
    s_hasFrame = true;
    s_status.uploadUsLast = ElapsedUs(start);
}

} // namespace

bool M360_MovieInit(IDirect3DDevice9* device, const char* isoPath)
{
    QueryPerformanceFrequency(&s_frequency);
    InitializeCriticalSection(&s_lock);
    _snprintf(s_isoPath, sizeof(s_isoPath) - 1, "%s", isoPath);
    if (FAILED(device->CreateVertexShader(
            reinterpret_cast<const DWORD*>(g_melee360SpriteVS), &s_vertexShader)) ||
        FAILED(device->CreatePixelShader(
            reinterpret_cast<const DWORD*>(g_melee360MoviePS), &s_pixelShader)))
        return false;
    static const D3DVERTEXELEMENT9 elements[] = {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    if (FAILED(device->CreateVertexDeclaration(elements, &s_declaration)))
        return false;
    for (unsigned set = 0; set < 2; ++set)
        for (unsigned plane = 0; plane < 3; ++plane) {
            const unsigned width = plane ? kMaxWidth / 2 : kMaxWidth;
            const unsigned height = plane ? kMaxHeight / 2 : kMaxHeight;
            if (FAILED(device->CreateTexture(width, height, 1, 0, D3DFMT_LIN_L8,
                    D3DPOOL_MANAGED, &s_textures[set][plane], 0)))
                return false;
        }
    for (unsigned i = 0; i < kSlots; ++i) {
        for (unsigned plane = 0; plane < 3; ++plane) {
            const unsigned bytes = plane ? kMaxWidth * kMaxHeight / 4
                                         : kMaxWidth * kMaxHeight;
            s_slots[i].planes[plane] = static_cast<unsigned char*>(malloc(bytes));
            if (!s_slots[i].planes[plane])
                return false;
        }
    }
    s_indexImage = fopen(isoPath, "rb");
    if (s_indexImage)
        setvbuf(s_indexImage, 0, _IONBF, 0);
    s_mounted = s_indexImage && m360_gcm_mount(&s_gcm, s_indexImage) == 0;
    return s_mounted;
}

bool M360_MovieOpen(const char* path, const unsigned* rateTable)
{
    M360_MovieClose();
    ZeroMemory(&s_status, sizeof(s_status));
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);
    if (!BuildIndex(path))
        return false;
    s_status.indexMs = ElapsedUs(start) / 1000;
    s_rateTable = rateTable;
    s_status.width = s_header.width;
    s_status.height = s_header.height;
    s_status.frameCount = s_header.frame_count;
    s_status.frameRate = s_header.frame_rate;
    for (unsigned i = 0; i < kSlots; ++i)
        s_slots[i].state = kFree;
    s_next = s_target = s_tick = 0;
    s_uploaded = 0xFFFFFFFFu;
    s_hasFrame = false;
    s_decodeUsTotal = 0;
    static const DWORD processors[kWorkers] = { 2, 4 };
    for (unsigned i = 0; i < kWorkers; ++i) {
        s_threads[i] = CreateThread(0, 64 * 1024, DecodeWorker, 0,
                                    CREATE_SUSPENDED, 0);
        if (!s_threads[i])
            return false;
        XSetThreadProcessor(s_threads[i], processors[i]);
        ResumeThread(s_threads[i]);
    }
    for (unsigned wait = 0; wait < 500; ++wait) {
        EnterCriticalSection(&s_lock);
        bool ready = s_status.decodeErrors != 0;
        for (unsigned i = 0; i < kSlots; ++i)
            ready |= s_slots[i].state == kReady && s_slots[i].frame == 0;
        LeaveCriticalSection(&s_lock);
        if (ready)
            break;
        Sleep(1);
    }
    s_status.opened = true;
    M360_MovieSetTick(0);
    return true;
}

void M360_MovieSetTick(unsigned tick)
{
    if (!s_status.opened)
        return;
    s_tick = tick;
    unsigned frame = m360_mth_frame_for_tick(s_rateTable, tick);
    if (frame >= s_header.frame_count)
        frame = s_header.frame_count - 1;
    s_status.currentFrame = frame;
    int best = -1;
    EnterCriticalSection(&s_lock);
    s_target = frame;
    for (unsigned i = 0; i < kSlots; ++i) {
        const Slot& slot = s_slots[i];
        if (slot.state == kReady && slot.frame <= frame &&
            (s_uploaded == 0xFFFFFFFFu || slot.frame > s_uploaded) &&
            (best < 0 || slot.frame > s_slots[best].frame))
            best = static_cast<int>(i);
    }
    if (best >= 0)
        s_slots[best].state = kUploading;
    LeaveCriticalSection(&s_lock);
    if (best >= 0) {
        Upload(s_slots[best]);
        EnterCriticalSection(&s_lock);
        s_slots[best].state = kReady;
        s_uploaded = s_slots[best].frame;
        ++s_status.framesPresented;
        LeaveCriticalSection(&s_lock);
    }
    if (s_uploaded != frame)
        ++s_status.framesLate;
}

void M360_MovieDraw(IDirect3DDevice9* device)
{
    if (!s_status.opened || !s_hasFrame)
        return;
    const float x0 = 160.0f, x1 = 1120.0f, y0 = 0.0f, y1 = 720.0f;
    const Vertex quad[4] = {
        { x0, y0, 0, 1, 1, 1, 1, 1, 0, 0 },
        { x1, y0, 0, 1, 1, 1, 1, 1, 1, 0 },
        { x1, y1, 0, 1, 1, 1, 1, 1, 1, 1 },
        { x0, y1, 0, 1, 1, 1, 1, 1, 0, 1 },
    };
    device->SetVertexShader(s_vertexShader);
    device->SetPixelShader(s_pixelShader);
    device->SetVertexDeclaration(s_declaration);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    for (unsigned plane = 0; plane < 3; ++plane) {
        device->SetTexture(plane, s_textures[s_textureSet][plane]);
        device->SetSamplerState(plane, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(plane, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(plane, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(plane, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    }
    device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, quad, sizeof(Vertex));
    for (unsigned plane = 0; plane < 3; ++plane)
        device->SetTexture(plane, 0);
}

bool M360_MovieEnded(void)
{
    return !s_status.opened ||
           m360_mth_frame_for_tick(s_rateTable, s_tick) >= s_header.frame_count;
}

void M360_MovieClose(void)
{
    StopWorkers();
    s_status.opened = false;
    s_hasFrame = false;
}

void M360_MovieGetStatus(MeleeMovieStatus* status)
{
    EnterCriticalSection(&s_lock);
    *status = s_status;
    LeaveCriticalSection(&s_lock);
}
