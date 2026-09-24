/* Host-check stand-in for the XDK CRT math.h (declarations only). */
#ifndef M360_HOST_MATH_H
#define M360_HOST_MATH_H
#ifdef __cplusplus
extern "C" {
#endif
double sin(double); double cos(double); double tan(double);
double asin(double); double acos(double); double atan(double); double atan2(double, double);
double sqrt(double); double fabs(double); double floor(double); double ceil(double);
double pow(double, double); double exp(double); double log(double); double log10(double);
double fmod(double, double); double modf(double, double*); double ldexp(double, int); double frexp(double, int*);
double sinh(double); double cosh(double); double tanh(double);
/* The XDK maps the float variants onto the double routines. */
#define sinf(x) ((float) sin(x))
#define cosf(x) ((float) cos(x))
#define tanf(x) ((float) tan(x))
#define asinf(x) ((float) asin(x))
#define acosf(x) ((float) acos(x))
#define atanf(x) ((float) atan(x))
#define atan2f(y, x) ((float) atan2(y, x))
#define sqrtf(x) ((float) sqrt(x))
#define fabsf(x) ((float) fabs(x))
#define floorf(x) ((float) floor(x))
#define ceilf(x) ((float) ceil(x))
#define powf(x, y) ((float) pow(x, y))
#define fmodf(x, y) ((float) fmod(x, y))
#define expf(x) ((float) exp(x))
#define logf(x) ((float) log(x))
#define HUGE_VAL __builtin_huge_val()
#ifdef __cplusplus
}
#endif
#endif
