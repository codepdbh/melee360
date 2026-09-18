#include "hsd_class_xdk_compat.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_XBOX)
#include <xtl.h>
#endif

extern "C" void OSReport(char* fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';

#if defined(_XBOX)
    OutputDebugStringA(buffer);
#else
    std::fputs(buffer, stderr);
#endif
}

extern "C" void OSSaveContext(OSContext* context)
{
    if (context != NULL) {
        std::memset(context, 0, sizeof(OSContext));
    }
}

extern "C" void OSPanic(char* file, int line, char* msg, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';

#if defined(_XBOX)
    char full[600];
    _snprintf(full, sizeof(full), "[M360][PANIC] %s (%s:%d)\n", buffer, file,
              line);
    full[sizeof(full) - 1] = '\0';
    OutputDebugStringA(full);
#else
    std::fprintf(stderr, "[M360][PANIC] %s (%s:%d)\n", buffer, file, line);
#endif
    std::abort();
}
