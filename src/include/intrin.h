#pragma once
#ifndef INTRIN_H
#define INTRIN_H

#include		"crtdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __64bit__
#define		__MACHINEX64(X) X
#elif defined __32bit__
#define		__MACHINEX64(X)
#endif

void __cpuid(int[4], int);
unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)

void __cdecl __debugbreak();

unsigned char _BitScanForward(unsigned long *index, unsigned long mask);
unsigned char _BitScanForward64(unsigned long *index,  unsigned __int64 mask);
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);
unsigned char _BitScanReverse64(unsigned long *index, unsigned __int64 mask);

void __stosb(unsigned char *, unsigned char, size_t);
void __stosd(unsigned long *, unsigned long, size_t);
__MACHINEX64(void __stosq(unsigned __int64 *, unsigned __int64, size_t));
void __stosw(unsigned short *, unsigned short, size_t);

#undef __X64


#ifdef __cplusplus
}
#endif

#endif//INTRIN_H