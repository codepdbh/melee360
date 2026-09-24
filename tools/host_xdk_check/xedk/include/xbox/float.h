#ifndef M360_HOST_FLOAT_H
#define M360_HOST_FLOAT_H
#include_next <float.h>
#define _FPCLASS_SNAN 0x0001
#define _FPCLASS_QNAN 0x0002
#define _FPCLASS_NINF 0x0004
#define _FPCLASS_NN 0x0008
#define _FPCLASS_ND 0x0010
#define _FPCLASS_NZ 0x0020
#define _FPCLASS_PZ 0x0040
#define _FPCLASS_PD 0x0080
#define _FPCLASS_PN 0x0100
#define _FPCLASS_PINF 0x0200
int __cdecl _fpclass(double);
#endif
