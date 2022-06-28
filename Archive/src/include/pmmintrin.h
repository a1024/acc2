//pmmintrin.h - SSE3 intrinsics
#pragma once
#ifndef PMMINTRIN_H
#define PMMINTRIN_H

#include	"emmintrin.h"

//MACRO functions for setting and reading the DAZ bit in the MXCSR
#define _MM_DENORMALS_ZERO_MASK		0x0040
#define _MM_DENORMALS_ZERO_ON		0x0040
#define _MM_DENORMALS_ZERO_OFF		0x0000

#define _MM_SET_DENORMALS_ZERO_MODE(mode)	_mm_setcsr ((_mm_getcsr()&~_MM_DENORMALS_ZERO_MASK)|(mode))
#define _MM_GET_DENORMALS_ZERO_MODE()		(_mm_getcsr()&_MM_DENORMALS_ZERO_MASK)

#ifdef __cplusplus
extern "C"
{
#endif
	
#ifdef __GNUC__

extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_addsub_ps (__m128 __X, __m128 __Y){return (__m128) __builtin_ia32_addsubps ((__v4sf)__X, (__v4sf)__Y);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_ps (__m128 __X, __m128 __Y){return (__m128) __builtin_ia32_haddps ((__v4sf)__X, (__v4sf)__Y);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_ps (__m128 __X, __m128 __Y){return (__m128) __builtin_ia32_hsubps ((__v4sf)__X, (__v4sf)__Y);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movehdup_ps (__m128 __X){return (__m128) __builtin_ia32_movshdup ((__v4sf)__X);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_moveldup_ps (__m128 __X){return (__m128) __builtin_ia32_movsldup ((__v4sf)__X);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_addsub_pd (__m128d __X, __m128d __Y){return (__m128d) __builtin_ia32_addsubpd ((__v2df)__X, (__v2df)__Y);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_pd (__m128d __X, __m128d __Y){return (__m128d) __builtin_ia32_haddpd ((__v2df)__X, (__v2df)__Y);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_pd (__m128d __X, __m128d __Y){return (__m128d) __builtin_ia32_hsubpd ((__v2df)__X, (__v2df)__Y);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loaddup_pd (double const *__P){return _mm_load1_pd (__P);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movedup_pd (__m128d __X){return _mm_shuffle_pd (__X, __X, _MM_SHUFFLE2 (0,0));}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_lddqu_si128 (__m128i const *__P){return (__m128i) __builtin_ia32_lddqu ((char const *)__P);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_monitor (void const * __P, unsigned int __E, unsigned int __H){__builtin_ia32_monitor (__P, __E, __H);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_mwait (unsigned int __E, unsigned int __H){__builtin_ia32_mwait (__E, __H);}

#elif defined _MSC_VER || defined __ACC2__

//New Single precision vector instructions.
__m128 _mm_addsub_ps(__m128 a, __m128 b);
__m128 _mm_hadd_ps(__m128 a, __m128 b);
__m128 _mm_hsub_ps(__m128 a, __m128 b);
__m128 _mm_movehdup_ps(__m128 a);
__m128 _mm_moveldup_ps(__m128 a);

//New double precision vector instructions.
__m128d _mm_addsub_pd(__m128d a, __m128d b);
__m128d _mm_hadd_pd(__m128d a, __m128d b);
__m128d _mm_hsub_pd(__m128d a, __m128d b);
__m128d _mm_loaddup_pd(double const * dp);
__m128d _mm_movedup_pd(__m128d a);

//New unaligned integer vector load instruction.
__m128i _mm_lddqu_si128(__m128i const *p);

//Miscellaneous new instructions.
//For _mm_monitor p goes in eax, extensions goes in ecx, hints goes in edx.
void _mm_monitor(void const *p, unsigned extensions, unsigned hints);

//For _mm_mwait, extensions goes in ecx, hints goes in eax.
void _mm_mwait(unsigned extensions, unsigned hints);

#else
#error Unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif