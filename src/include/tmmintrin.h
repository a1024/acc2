//tmmintrin.h - SSSE3/SSE3S intrinsics
#pragma once
#ifndef TMMINTRIN_H
#define TMMINTRIN_H

#include		"pmmintrin.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __GNUC__


extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_epi16(__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phaddw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_epi32 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phaddd128((__v4si)__X, (__v4si)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadds_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phaddsw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phaddw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadd_pi32 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phaddd((__v2si)__X, (__v2si)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hadds_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phaddsw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phsubw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_epi32 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phsubd128((__v4si)__X, (__v4si)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsubs_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_phsubsw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phsubw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsub_pi32 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phsubd((__v2si)__X, (__v2si)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_hsubs_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_phsubsw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_maddubs_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_pmaddubsw128((__v16qi)__X, (__v16qi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_maddubs_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_pmaddubsw((__v8qi)__X, (__v8qi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mulhrs_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_pmulhrsw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mulhrs_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_pmulhrsw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_epi8 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_pshufb128 ((__v16qi)__X, (__v16qi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_pi8 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_pshufb((__v8qi)__X, (__v8qi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_epi8 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_psignb128((__v16qi)__X, (__v16qi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_epi16 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_psignw128((__v8hi)__X, (__v8hi)__Y);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_epi32 (__m128i __X, __m128i __Y){return (__m128i) __builtin_ia32_psignd128((__v4si)__X, (__v4si)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_pi8 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_psignb((__v8qi)__X, (__v8qi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_pi16 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_psignw((__v4hi)__X, (__v4hi)__Y);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sign_pi32 (__m64 __X, __m64 __Y){return (__m64) __builtin_ia32_psignd((__v2si)__X, (__v2si)__Y);}
#ifdef __OPTIMIZE__
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_alignr_epi8(__m128i __X, __m128i __Y, const int __N){return (__m128i)__builtin_ia32_palignr128 ((__v2di)__X, (__v2di)__Y, __N * 8);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_alignr_pi8(__m64 __X, __m64 __Y, const int __N){return (__m64) __builtin_ia32_palignr ((__v1di)__X, (__v1di)__Y, __N * 8);}
#else
#define _mm_alignr_epi8(X, Y, N) ((__m128i) __builtin_ia32_palignr128 ((__v2di)(__m128i)(X), (__v2di)(__m128i)(Y), (int)(N) * 8))
#define _mm_alignr_pi8(X, Y, N)	((__m64) __builtin_ia32_palignr ((__v1di)(__m64)(X), (__v1di)(__m64)(Y), (int)(N) * 8))
#endif
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_epi8 (__m128i __X){return (__m128i) __builtin_ia32_pabsb128 ((__v16qi)__X);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_epi16 (__m128i __X){return (__m128i) __builtin_ia32_pabsw128 ((__v8hi)__X);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_epi32 (__m128i __X){return (__m128i) __builtin_ia32_pabsd128 ((__v4si)__X);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_pi8 (__m64 __X){return (__m64) __builtin_ia32_pabsb ((__v8qi)__X);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_pi16 (__m64 __X){return (__m64) __builtin_ia32_pabsw ((__v4hi)__X);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_abs_pi32 (__m64 __X){return (__m64) __builtin_ia32_pabsd ((__v2si)__X);}


#elif defined _MSC_VER || defined __ACC2__


// Add horizonally packed [saturated] words, double words, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_hadd_epi16 (__m128i a, __m128i b);
__m128i _mm_hadd_epi32 (__m128i a, __m128i b);
__m128i _mm_hadds_epi16 (__m128i a, __m128i b);

__m64 _mm_hadd_pi16 (__m64 a, __m64 b);
__m64 _mm_hadd_pi32 (__m64 a, __m64 b);
__m64 _mm_hadds_pi16 (__m64 a, __m64 b);

// Subtract horizonally packed [saturated] words, double words, {X,}MM2/m{128,64} (b) from {X,}MM1 (a).
__m128i _mm_hsub_epi16 (__m128i a, __m128i b);
__m128i _mm_hsub_epi32 (__m128i a, __m128i b);
__m128i _mm_hsubs_epi16 (__m128i a, __m128i b);

__m64 _mm_hsub_pi16 (__m64 a, __m64 b);
__m64 _mm_hsub_pi32 (__m64 a, __m64 b);
__m64 _mm_hsubs_pi16 (__m64 a, __m64 b);

// Multiply and add packed words, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_maddubs_epi16 (__m128i a, __m128i b);

__m64 _mm_maddubs_pi16 (__m64 a, __m64 b);

// Packed multiply high integers with round and scaling, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_mulhrs_epi16 (__m128i a, __m128i b);

__m64 _mm_mulhrs_pi16 (__m64 a, __m64 b);

// Packed shuffle bytes {X,}MM2/m{128,64} (b) by {X,}MM1 (a).
__m128i _mm_shuffle_epi8 (__m128i a, __m128i b);

__m64 _mm_shuffle_pi8 (__m64 a, __m64 b);

// Packed byte, word, double word sign, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_sign_epi8 (__m128i a, __m128i b);
__m128i _mm_sign_epi16 (__m128i a, __m128i b);
__m128i _mm_sign_epi32 (__m128i a, __m128i b);

__m64 _mm_sign_pi8 (__m64 a, __m64 b);
__m64 _mm_sign_pi16 (__m64 a, __m64 b);
__m64 _mm_sign_pi32 (__m64 a, __m64 b);

// Packed align and shift right by n*8 bits, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_alignr_epi8 (__m128i a, __m128i b, int n);

__m64 _mm_alignr_pi8 (__m64 a, __m64 b, int n);

// Packed byte, word, double word absolute value, {X,}MM2/m{128,64} (b) to {X,}MM1 (a).
__m128i _mm_abs_epi8 (__m128i a);
__m128i _mm_abs_epi16 (__m128i a);
__m128i _mm_abs_epi32 (__m128i a);

__m64 _mm_abs_pi8 (__m64 a);
__m64 _mm_abs_pi16 (__m64 a);
__m64 _mm_abs_pi32 (__m64 a);


#else
#error Unknown platform
#endif

#if 0
//TODO: declare all intrinsics in the standard headers

typedef union _CRT_INTRIN _CRT_ALIGN(8) __m64
{
	unsigned __int64    m64_u64;
	float               m64_f32[2];
	__int8              m64_i8[8];
	__int16             m64_i16[4];
	__int32             m64_i32[2];
	__int64             m64_i64;
	unsigned __int8     m64_u8[8];
	unsigned __int16    m64_u16[4];
	unsigned __int32    m64_u32[2];
} __m64;
typedef union _CRT_INTRIN _CRT_ALIGN(16) __m128
{
	float               m128_f32[4];
	unsigned __int64    m128_u64[2];
	__int8              m128_i8[16];
	__int16             m128_i16[8];
	__int32             m128_i32[4];
	__int64             m128_i64[2];
	unsigned __int8     m128_u8[16];
	unsigned __int16    m128_u16[8];
	unsigned __int32    m128_u32[4];
} __m128;
typedef union _CRT_INTRIN _CRT_ALIGN(16) __m128i
{
	__int8              m128i_i8[16];
	__int16             m128i_i16[8];
	__int32             m128i_i32[4];
	__int64             m128i_i64[2];
	unsigned __int8     m128i_u8[16];
	unsigned __int16    m128i_u16[8];
	unsigned __int32    m128i_u32[4];
	unsigned __int64    m128i_u64[2];
} __m128i;
typedef struct _CRT_INTRIN _CRT_ALIGN(16) __m128d
{
	double              m128d_f64[2];
} __m128d;

void __cdecl __debugbreak();

unsigned char _BitScanForward(unsigned long *index, unsigned long mask);
unsigned char _BitScanForward64(unsigned long *index,  unsigned __int64 mask);
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);
unsigned char _BitScanReverse64(unsigned long *index, unsigned __int64 mask);

unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)
	
__m128i _mm_set_epi64(__m64 _Q1, __m64 _Q0);
__m128i _mm_set_epi32(int _I3, int _I2, int _I1, int _I0);
__m128i _mm_set_epi16(short _W7, short _W6, short _W5, short _W4,
                      short _W3, short _W2, short _W1, short _W0);
__m128i _mm_set_epi8(char _B15, char _B14, char _B13, char _B12,
                     char _B11, char _B10, char _B9, char _B8,
                     char _B7, char _B6, char _B5, char _B4,
                     char _B3, char _B2, char _B1, char _B0);
__m128i _mm_set1_epi64(__m64 _Q);
__m128i _mm_set1_epi32(int _I);
__m128i _mm_set1_epi16(short _W);
__m128i _mm_set1_epi8(char _B);
__m128i _mm_setl_epi64(__m128i _Q);
__m128i _mm_setr_epi64(__m64 _Q0, __m64 _Q1);
__m128i _mm_setr_epi32(int _I0, int _I1, int _I2, int _I3);
__m128i _mm_setr_epi16(short _W0, short _W1, short _W2, short _W3,
                       short _W4, short _W5, short _W6, short _W7);
__m128i _mm_setr_epi8(char _B15, char _B14, char _B13, char _B12,
						char _B11, char _B10, char _B9, char _B8,
						char _B7, char _B6, char _B5, char _B4,
						char _B3, char _B2, char _B1, char _B0);
__m128i _mm_setzero_si128(void);
	
__m128i _mm_load_si128(__m128i const *_P);
__m128i _mm_loadu_si128(__m128i const *_P);
__m128i _mm_loadl_epi64(__m128i const *_P);

__m128i _mm_and_si128(__m128i _A, __m128i _B);
__m128i _mm_andnot_si128(__m128i _A, __m128i _B);
__m128i _mm_or_si128(__m128i _A, __m128i _B);
__m128i _mm_xor_si128(__m128i _A, __m128i _B);

__m128i _mm_cmpeq_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmpeq_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmpeq_epi32(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi32(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi32(__m128i _A, __m128i _B);

__m128i _mm_packs_epi16(__m128i _A, __m128i _B);
__m128i _mm_packs_epi32(__m128i _A, __m128i _B);
__m128i _mm_packus_epi16(__m128i _A, __m128i _B);
int _mm_extract_epi16(__m128i _A, int _Imm);
__m128i _mm_insert_epi16(__m128i _A, int _B, int _Imm);
int _mm_movemask_epi8(__m128i _A);
__m128i _mm_shuffle_epi32(__m128i _A, int _Imm);
__m128i _mm_shufflehi_epi16(__m128i _A, int _Imm);
__m128i _mm_shufflelo_epi16(__m128i _A, int _Imm);
__m128i _mm_unpackhi_epi8(__m128i _A, __m128i _B);
__m128i _mm_unpackhi_epi16(__m128i _A, __m128i _B);
__m128i _mm_unpackhi_epi32(__m128i _A, __m128i _B);
__m128i _mm_unpackhi_epi64(__m128i _A, __m128i _B);
__m128i _mm_unpacklo_epi8(__m128i _A, __m128i _B);
__m128i _mm_unpacklo_epi16(__m128i _A, __m128i _B);
__m128i _mm_unpacklo_epi32(__m128i _A, __m128i _B);
__m128i _mm_unpacklo_epi64(__m128i _A, __m128i _B);
#endif

#ifdef __cplusplus
}
#endif

#endif//TMMINTRIN_H