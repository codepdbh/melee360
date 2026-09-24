/* PowerPC intrinsics the XDK compiler provides without a header; declared
 * here so the host check can treat implicit declarations as errors. */
#ifndef M360_HOST_INTRINSICS_H
#define M360_HOST_INTRINSICS_H
double __frsqrte(double);
double __fres(double);
double __fsel(double, double, double);
double __fabs(double);
float __fabsf(float);
#endif
