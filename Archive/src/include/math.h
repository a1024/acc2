#pragma once
#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C"
{
#endif

int       __cdecl abs(int _X);
long      __cdecl labs(long _X);
long long __cdecl llabs(long long _X);

double  __cdecl acos(double _X);
double __cdecl acosh(double _X);
double  __cdecl asin(double _X);
double __cdecl asinh(double _X);
double  __cdecl atan(double _X);
double __cdecl atanh(double _X);
double  __cdecl atan2(double _Y, double _X);

double __cdecl cbrt(double _X);
double __cdecl copysign(double _X, double _Y);
double  __cdecl cos(double _X);
double  __cdecl cosh(double _X);
double __cdecl erf(double _X);
double __cdecl erfc(double _X);
double  __cdecl exp(double _X);
double __cdecl exp2(double _X);
double __cdecl expm1(double _X);
double  __cdecl fabs(double _X);
double __cdecl fdim(double _X, double _Y);
double __cdecl fma(double _X, double _Y, double _Z);
double __cdecl fmax(double _X, double _Y);
double __cdecl fmin(double _X, double _Y);
double  __cdecl fmod(double _X, double _Y);
int __cdecl ilogb(double _X);
double __cdecl lgamma(double _X);
long long __cdecl llrint(double _X);
long long __cdecl llround(double _X);
double  __cdecl log(double _X);
double  __cdecl log10(double _X);
double __cdecl log1p(double _X);
double __cdecl log2(double _X);
double __cdecl logb(double _X);
long __cdecl lrint(double _X);
long __cdecl lround(double _X);
double __cdecl nan(const char *);
double __cdecl nearbyint(double _X);
double __cdecl nextafter(double _X, double _Y);
double __cdecl nexttoward(double _X, long double _Y);
double  __cdecl pow(double _X, double _Y);
double __cdecl remainder(double _X, double _Y);
double __cdecl remquo(double _X, double _Y, int *_Z);
double __cdecl rint(double _X);
double __cdecl round(double _X);
double __cdecl scalbln(double _X, long _Y);
double __cdecl scalbn(double _X, int _Y);
double  __cdecl sin(double _X);
double  __cdecl sinh(double _X);
double  __cdecl sqrt(double _X);
double  __cdecl tan(double _X);
double  __cdecl tanh(double _X);
double __cdecl tgamma(double _X);
double __cdecl trunc(double _X);

double  __cdecl atof(const char *_String);
//double  __cdecl _atof_l(const char *_String, _locale_t _Locale);

double  __cdecl _cabs(struct _complex _Complex_value);
double  __cdecl ceil(double _X);

double __cdecl _chgsign (double _X);
double __cdecl _copysign (double _Number, double _Sign);

double  __cdecl floor(double _X);
double  __cdecl frexp(double _X, int * _Y);
double  __cdecl _hypot(double _X, double _Y);
double  __cdecl _j0(double _X );
double  __cdecl _j1(double _X );
double  __cdecl _jn(int _X, double _Y);
double  __cdecl ldexp(double _X, int _Y);

double  __cdecl modf(double _X, double * _Y);
double  __cdecl _y0(double _X);
double  __cdecl _y1(double _X);
double  __cdecl _yn(int _X, double _Y);

inline double hypot(double _X, double _Y)
{
    return _hypot(_X, _Y);
}


float __cdecl acoshf(float _X);
float __cdecl asinhf(float _X);
float __cdecl atanhf(float _X);
float __cdecl cbrtf(float _X);
float  __cdecl _chgsignf(float _X);
float __cdecl copysignf(float _X, float _Y);
float  __cdecl _copysignf(float _Number, float _Sign);
float __cdecl erff(float _X);
float __cdecl erfcf(float _X);
float __cdecl expm1f(float _X);
float __cdecl exp2f(float _X);
float __cdecl fdimf(float _X, float _Y);
float __cdecl fmaf(float _X, float _Y, float _Z);
float __cdecl fmaxf(float _X, float _Y);
float __cdecl fminf(float _X, float _Y);
float  __cdecl _hypotf(float _X, float _Y);
int __cdecl ilogbf(float _X);
float __cdecl lgammaf(float _X);
long long __cdecl llrintf(float _X);
long long __cdecl llroundf(float _X);
float __cdecl log1pf(float _X);
float __cdecl log2f(float _X);
float __cdecl logbf(float _X);
long __cdecl lrintf(float _X);
long __cdecl lroundf(float _X);
float __cdecl nanf(const char *);
float __cdecl nearbyintf(float _X);
float __cdecl nextafterf(float _X, float _Y);
float __cdecl nexttowardf(float _X, long double _Y);
float __cdecl remainderf(float _X, float _Y);
float __cdecl remquof(float _X, float _Y, int *_Z);
float __cdecl rintf(float _X);
float __cdecl roundf(float _X);
float __cdecl scalblnf(float _X, long _Y);
float __cdecl scalbnf(float _X, int _Y);
float __cdecl tgammaf(float _X);
float __cdecl truncf(float _X);

#ifdef __cplusplus
}
#endif

#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#define M_1_PI     0.318309886183790671538
#define M_2_PI     0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.707106781186547524401

extern double _HUGE, _QNAN;

#endif//MATH_H