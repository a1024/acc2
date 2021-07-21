#pragma once
#ifndef TMMINTRIN_H
#define TMMINTRIN_H

//TODO: declare all intrinsics in the standard headers

#define _CRT_ALIGN(x) __declspec(align(x))
typedef union __declspec(intrin_type) _CRT_ALIGN(8) __m64
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
typedef union __declspec(intrin_type) _CRT_ALIGN(16) __m128
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
typedef union __declspec(intrin_type) _CRT_ALIGN(16) __m128i
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
typedef struct __declspec(intrin_type) _CRT_ALIGN(16) __m128d
{
	double              m128d_f64[2];
} __m128d;
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))

#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif

#endif//TMMINTRIN_H