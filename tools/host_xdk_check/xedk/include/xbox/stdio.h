#ifndef M360_HOST_STDIO_H
#define M360_HOST_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct _iobuf FILE;
int printf(const char*, ...); int sprintf(char*, const char*, ...); int _snprintf(char*, size_t, const char*, ...);
int vsprintf(char*, const char*, va_list); int _vsnprintf(char*, size_t, const char*, va_list);
int snprintf(char*, size_t, const char*, ...); int vsnprintf(char*, size_t, const char*, va_list);
int sscanf(const char*, const char*, ...); int fprintf(FILE*, const char*, ...); int puts(const char*);
int vfprintf(FILE*, const char*, va_list); int fflush(FILE*); int putchar(int);
#endif
