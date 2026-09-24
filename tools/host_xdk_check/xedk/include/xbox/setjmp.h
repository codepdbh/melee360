#ifndef M360_HOST_SETJMP_H
#define M360_HOST_SETJMP_H
typedef int jmp_buf[16];
int setjmp(jmp_buf); void longjmp(jmp_buf, int);
#endif
