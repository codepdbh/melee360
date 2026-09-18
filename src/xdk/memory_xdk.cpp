#include "memory_xdk_compat.h"

#include <cstdio>
#include <cstdlib>

namespace {

struct M360_MemBlock {
    u32 size;
    int used;
    M360_MemBlock* next;
    M360_MemBlock* prev;
};

const u32 kArenaSize = 24u * 1024u * 1024u;
const u32 kAlign = 16u;

unsigned char s_arena[kArenaSize];
unsigned char* s_arenaStart = 0;
unsigned char* s_arenaEnd = 0;
M360_MemBlock* s_first = 0;

u32 RoundUp(u32 value, u32 align)
{
    return (value + (align - 1)) & ~(align - 1);
}

const u32 kHeaderSize = (sizeof(M360_MemBlock) + (kAlign - 1)) & ~(kAlign - 1);

} // namespace

extern "C" void M360_HSD_HeapInit(void)
{
    if (s_arenaStart)
        return;

    const u32 misalign = static_cast<u32>(
        reinterpret_cast<size_t>(s_arena) & (kAlign - 1));
    const u32 pad = misalign ? (kAlign - misalign) : 0;

    s_arenaStart = s_arena + pad;
    s_arenaEnd = s_arena + kArenaSize;
    s_first = reinterpret_cast<M360_MemBlock*>(s_arenaStart);
    s_first->size = static_cast<u32>(s_arenaEnd - s_arenaStart) - kHeaderSize;
    s_first->used = 0;
    s_first->next = 0;
    s_first->prev = 0;
}

extern "C" OSHeapHandle HSD_GetHeap(void)
{
    return 0;
}

extern "C" void* OSAllocFromHeap(OSHeapHandle heap, u32 size)
{
    (void) heap;
    if (!s_arenaStart || size == 0)
        return 0;

    const u32 need = RoundUp(size, kAlign);
    for (M360_MemBlock* block = s_first; block; block = block->next) {
        if (block->used || block->size < need)
            continue;

        const u32 remaining = block->size - need;
        if (remaining > kHeaderSize + kAlign) {
            unsigned char* splitAddr = reinterpret_cast<unsigned char*>(block) +
                kHeaderSize + need;
            M360_MemBlock* split = reinterpret_cast<M360_MemBlock*>(splitAddr);
            split->size = remaining - kHeaderSize;
            split->used = 0;
            split->next = block->next;
            split->prev = block;
            if (block->next)
                block->next->prev = split;
            block->next = split;
            block->size = need;
        }

        block->used = 1;
        return reinterpret_cast<unsigned char*>(block) + kHeaderSize;
    }

    return 0;
}

extern "C" void OSFreeToHeap(OSHeapHandle heap, void* ptr)
{
    (void) heap;
    if (!ptr || !s_arenaStart)
        return;

    unsigned char* raw = reinterpret_cast<unsigned char*>(ptr) - kHeaderSize;
    if (raw < s_arenaStart || raw >= s_arenaEnd)
        return;

    M360_MemBlock* block = reinterpret_cast<M360_MemBlock*>(raw);
    block->used = 0;

    if (block->next && !block->next->used) {
        M360_MemBlock* next = block->next;
        block->size += kHeaderSize + next->size;
        block->next = next->next;
        if (next->next)
            next->next->prev = block;
    }

    if (block->prev && !block->prev->used) {
        M360_MemBlock* prev = block->prev;
        prev->size += kHeaderSize + block->size;
        prev->next = block->next;
        if (block->next)
            block->next->prev = prev;
    }
}

extern "C" long OSCheckHeap(int heap)
{
    (void) heap;
    if (!s_arenaStart)
        return 0;

    long free_bytes = 0;
    for (M360_MemBlock* block = s_first; block; block = block->next) {
        if (!block->used)
            free_bytes += static_cast<long>(block->size);
    }
    return free_bytes;
}

extern "C" void M360_HSD_MemoryAssertFail(const char* file, u32 line, const char* expr)
{
    std::fprintf(stderr, "[M360][MEMORY] assertion failed: %s (%s:%u)\n",
                 expr, file, static_cast<unsigned>(line));
    std::abort();
}
