#ifndef MELEE360_XDK_MEMORY_COMPAT_H
#define MELEE360_XDK_MEMORY_COMPAT_H

/* Keep memory.c untouched while replacing its Dolphin OS/runtime headers. */
#define RUNTIME_PLATFORM_H
#define SYSDOLPHIN_BASELIB_DEBUG_H
#define _initialize_h_
#define _DOLPHIN_OSALLOC_H_

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef float f32;
typedef double f64;
typedef signed int ssize_t;

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*) 0)
#endif
#endif

typedef int OSHeapHandle;

#ifdef __cplusplus
extern "C" {
#endif

OSHeapHandle HSD_GetHeap(void);
void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);
void M360_HSD_MemoryAssertFail(const char* file, u32 line, const char* expr);

#ifdef __cplusplus
}
#endif

#define HSD_ASSERT(line, cond) \
    ((cond) ? (void) 0 : M360_HSD_MemoryAssertFail(__FILE__, (u32) (line), #cond))

#endif
