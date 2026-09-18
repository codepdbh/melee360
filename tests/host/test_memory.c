#include "memory_xdk_compat.h"
#include "memory.h"

#include <stdio.h>
#include <string.h>

extern void M360_HSD_HeapInit(void);

static int fail_count = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "[M360][MEMORY][FAIL] %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        fail_count++; \
    } \
} while (0)

static int is_aligned(void* ptr, unsigned align)
{
    return (((unsigned long) ptr) & (align - 1u)) == 0u;
}

static void test_basic_alloc(void)
{
    void* a = HSD_MemAlloc(128);
    CHECK(a != NULL);
    CHECK(is_aligned(a, 16));

    memset(a, 0xAB, 128);
    unsigned char* bytes = (unsigned char*) a;
    int ok = 1;
    for (int i = 0; i < 128; ++i) {
        if (bytes[i] != 0xAB)
            ok = 0;
    }
    CHECK(ok);

    HSD_Free(a);
}

static void test_zero_and_negative(void)
{
    CHECK(HSD_MemAlloc(0) == NULL);
    CHECK(HSD_MemAlloc(-4) == NULL);
    HSD_Free(NULL);
}

static void test_many_sizes(void)
{
    void* blocks[64];
    for (int i = 0; i < 64; ++i) {
        blocks[i] = HSD_MemAlloc((ssize_t) (16 + i * 37));
        CHECK(blocks[i] != NULL);
        memset(blocks[i], (unsigned char) i, (size_t) (16 + i * 37));
    }
    for (int i = 0; i < 64; i += 2)
        HSD_Free(blocks[i]);
    for (int i = 0; i < 64; i += 2) {
        blocks[i] = HSD_MemAlloc((ssize_t) (8 + i));
        CHECK(blocks[i] != NULL);
    }
    for (int i = 0; i < 64; ++i)
        HSD_Free(blocks[i]);
}

static void test_burn_in(void)
{
    enum { kRounds = 4000 };
    void* live[128];
    for (int i = 0; i < 128; ++i)
        live[i] = NULL;

    for (int round = 0; round < kRounds; ++round) {
        int slot = round % 128;
        if (live[slot])
            HSD_Free(live[slot]);
        ssize_t size = (ssize_t) (1 + (round * 131) % 4096);
        live[slot] = HSD_MemAlloc(size);
        CHECK(live[slot] != NULL);
        if (live[slot])
            memset(live[slot], (unsigned char) round, (size_t) size);
    }

    for (int i = 0; i < 128; ++i) {
        if (live[i])
            HSD_Free(live[i]);
    }
}

int main(void)
{
    M360_HSD_HeapInit();

    test_basic_alloc();
    test_zero_and_negative();
    test_many_sizes();
    test_burn_in();

    if (fail_count) {
        fprintf(stderr, "[M360][MEMORY] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][MEMORY] HSD_MemAlloc/HSD_Free host validation passed\n");
    return 0;
}
