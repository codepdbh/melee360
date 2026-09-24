#ifndef M360_HOST_STDLIB_H
#define M360_HOST_STDLIB_H
#include <stddef.h>
void* malloc(size_t); void free(void*); void* calloc(size_t, size_t); void* realloc(void*, size_t);
int abs(int); long labs(long); int atoi(const char*); double atof(const char*);
int rand(void); void srand(unsigned); void abort(void); void exit(int);
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
#endif
