//emmintrin.h - SSE2 (Wilamette) intrinsics
#pragma once
#ifndef EMMINTRIN_H
#define EMMINTRIN_H

#include		"xmmintrin.h"

//Create a selector for use with the SHUFPD instruction.
#define _MM_SHUFFLE2(fp1,fp0)	(((fp1) << 1) | (fp0))

#ifdef __cplusplus
extern "C"
{
#endif
	
#ifdef __GNUC__


//SSE2
typedef double				__v2df __attribute__((__vector_size__(16)));
typedef long long			__v2di __attribute__((__vector_size__(16)));
typedef unsigned long long	__v2du __attribute__((__vector_size__(16)));
typedef int					__v4si __attribute__((__vector_size__(16)));
typedef unsigned int		__v4su __attribute__((__vector_size__(16)));
typedef short				__v8hi __attribute__((__vector_size__(16)));
typedef unsigned short		__v8hu __attribute__((__vector_size__(16)));
typedef char				__v16qi __attribute__((__vector_size__(16)));
typedef signed char			__v16qs __attribute__((__vector_size__(16)));
typedef unsigned char		__v16qu __attribute__((__vector_size__(16)));

//The Intel API is flexible enough that we must allow aliasing with other vector types, and their scalar components.
typedef long long	__m128i __attribute__((__vector_size__(16), __may_alias__));
typedef double		__m128d __attribute__((__vector_size__(16), __may_alias__));

//Unaligned version of the same types.
typedef long long	__m128i_u __attribute__((__vector_size__ (16), __may_alias__, __aligned__ (1)));
typedef double		__m128d_u __attribute__((__vector_size__ (16), __may_alias__, __aligned__ (1)));

//Create a vector with element 0 as F and the rest zero.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_sd(double __F){return __extension__ (__m128d){__F, 0.0};}

//Create a vector with both elements equal to F.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_pd(double __F){return __extension__ (__m128d){ __F, __F };}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_pd1(double __F){return _mm_set1_pd (__F);}

//Create a vector with the lower value X and upper value W.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_pd(double __W, double __X){return __extension__ (__m128d){ __X, __W };}

//Create a vector with the lower value W and upper value X.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_pd(double __W, double __X){return __extension__ (__m128d){ __W, __X };}

//Create an undefined vector.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_undefined_pd(void)
{
  __m128d __Y=__Y;
  return __Y;
}

//Create a vector of zeros.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setzero_pd(void){return __extension__(__m128d){0.0, 0.0};}

//Sets the low DPFP value of A from the low value of B.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_move_sd(__m128d __A, __m128d __B){return __extension__ (__m128d)__builtin_shuffle((__v2df)__A, (__v2df)__B, (__v2di){2, 1});}

//Load two DPFP values from P.  The address must be 16-byte aligned.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_pd(double const *__P){return *(__m128d *)__P;}

/* Load two DPFP values from P.  The address need not be 16-byte aligned.  */
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadu_pd(double const *__P){return *(__m128d_u *)__P;}

/* Create a vector with all two elements equal to *P.  */
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load1_pd(double const *__P){return _mm_set1_pd (*__P);}

/* Create a vector with element 0 as *P and the rest zero.  */
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_sd(double const *__P){return _mm_set_sd (*__P);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_pd1(double const *__P){return _mm_load1_pd (__P);}

/* Load two DPFP values in reverse order.  The address must be aligned.  */
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadr_pd(double const *__P)
{
  __m128d __tmp = _mm_load_pd (__P);
  return __builtin_ia32_shufpd (__tmp, __tmp, _MM_SHUFFLE2 (0,1));
}

/* Store two DPFP values.  The address must be 16-byte aligned.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_pd(double *__P, __m128d __A){*(__m128d *)__P=__A;}

/* Store two DPFP values.  The address need not be 16-byte aligned.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeu_pd(double *__P, __m128d __A){*(__m128d_u *)__P=__A;}

/* Stores the lower DPFP value.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_sd(double *__P, __m128d __A){*__P = ((__v2df)__A)[0];}
extern __inline double __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsd_f64(__m128d __A){return ((__v2df)__A)[0];}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storel_pd(double *__P, __m128d __A){_mm_store_sd (__P, __A);}

/* Stores the upper DPFP value.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeh_pd(double *__P, __m128d __A){*__P = ((__v2df)__A)[1];}

/* Store the lower DPFP value across two words.
   The address must be 16-byte aligned.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store1_pd(double *__P, __m128d __A){_mm_store_pd (__P, __builtin_ia32_shufpd (__A, __A, _MM_SHUFFLE2 (0,0)));}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_pd1(double *__P, __m128d __A){_mm_store1_pd (__P, __A);}

/* Store two DPFP values in reverse order.  The address must be aligned.  */
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storer_pd(double *__P, __m128d __A){_mm_store_pd (__P, __builtin_ia32_shufpd (__A, __A, _MM_SHUFFLE2 (0,1)));}

extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvtsi128_si32(__m128i __A){return __builtin_ia32_vec_ext_v4si ((__v4si)__A, 0);}

#ifdef __x86_64__
/* Intel intrinsic.  */
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtsi128_si64(__m128i __A){return ((__v2di)__A)[0];}

/* Microsoft intrinsic.  */
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtsi128_si64x(__m128i __A){return ((__v2di)__A)[0];}
#endif

extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_pd(__m128d __A, __m128d __B){return (__m128d) ((__v2df)__A + (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_addsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_pd(__m128d __A, __m128d __B){return (__m128d) ((__v2df)__A - (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_subsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_pd(__m128d __A, __m128d __B){return (__m128d) ((__v2df)__A * (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_mulsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_div_pd(__m128d __A, __m128d __B){return (__m128d) ((__v2df)__A / (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_div_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_divsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sqrt_pd(__m128d __A){return (__m128d)__builtin_ia32_sqrtpd ((__v2df)__A);}

/* Return pair {sqrt (B[0]), A[1]}.  */
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sqrt_sd(__m128d __A, __m128d __B)
{
	__v2df __tmp = __builtin_ia32_movsd ((__v2df)__A, (__v2df)__B);
	return (__m128d)__builtin_ia32_sqrtsd ((__v2df)__tmp);
}

extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_minpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_minsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_maxpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_maxsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_and_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_andpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_andnot_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_andnpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_or_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_orpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_xor_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_xorpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpeqpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpltpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmple_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmplepd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpgtpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpge_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpgepd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpneq_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpneqpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnlt_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpnltpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnle_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpnlepd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpngt_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpngtpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnge_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpngepd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpord_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpordpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpunord_pd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpunordpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpeqsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpltsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmple_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmplesd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_sd(__m128d __A, __m128d __B){return (__m128d) __builtin_ia32_movsd ((__v2df) __A, (__v2df)__builtin_ia32_cmpltsd ((__v2df) __B, (__v2df)__A));}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpge_sd(__m128d __A, __m128d __B){return (__m128d) __builtin_ia32_movsd ((__v2df) __A, (__v2df)__builtin_ia32_cmplesd ((__v2df) __B, (__v2df)__A));}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpneq_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpneqsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnlt_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpnltsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnle_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpnlesd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpngt_sd(__m128d __A, __m128d __B){return (__m128d) __builtin_ia32_movsd ((__v2df) __A, (__v2df)__builtin_ia32_cmpnltsd ((__v2df) __B, (__v2df)__A));}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnge_sd(__m128d __A, __m128d __B){return (__m128d) __builtin_ia32_movsd ((__v2df) __A, (__v2df)__builtin_ia32_cmpnlesd ((__v2df) __B,(__v2df)__A));}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpord_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpordsd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpunord_sd(__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_cmpunordsd ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comieq_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdeq ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comilt_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdlt ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comile_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdle ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comigt_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdgt ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comige_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdge ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comineq_sd(__m128d __A, __m128d __B){return __builtin_ia32_comisdneq ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomieq_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdeq ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomilt_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdlt ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomile_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdle ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomigt_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdgt ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomige_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdge ((__v2df)__A, (__v2df)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomineq_sd(__m128d __A, __m128d __B){return __builtin_ia32_ucomisdneq ((__v2df)__A, (__v2df)__B);}

//Create a vector of Qi, where i is the element number.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_epi64x(long long __q1, long long __q0){return __extension__ (__m128i)(__v2di){ __q0, __q1 };}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_epi64(__m64 __q1,  __m64 __q0){return _mm_set_epi64x ((long long)__q1, (long long)__q0);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_epi32(int __q3, int __q2, int __q1, int __q0){return __extension__ (__m128i)(__v4si){ __q0, __q1, __q2, __q3 };}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_epi16(short __q7, short __q6, short __q5, short __q4, short __q3, short __q2, short __q1, short __q0)
{
	return __extension__(__m128i)(__v8hi){__q0, __q1, __q2, __q3, __q4, __q5, __q6, __q7 };
}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_epi8(char __q15, char __q14, char __q13, char __q12, char __q11, char __q10, char __q09, char __q08, char __q07, char __q06, char __q05, char __q04, char __q03, char __q02, char __q01, char __q00)
{
	return __extension__(__m128i)(__v16qi){__q00, __q01, __q02, __q03, __q04, __q05, __q06, __q07, __q08, __q09, __q10, __q11, __q12, __q13, __q14, __q15};
}

//Set all of the elements of the vector to A.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_epi64x (long long __A){return _mm_set_epi64x (__A, __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_epi64 (__m64 __A){return _mm_set_epi64 (__A, __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_epi32 (int __A){return _mm_set_epi32 (__A, __A, __A, __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_epi16 (short __A){return _mm_set_epi16 (__A, __A, __A, __A, __A, __A, __A, __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_epi8 (char __A){return _mm_set_epi8 (__A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A, __A);}

//Create a vector of Qi, where i is the element number.
//The parameter order is reversed from the _mm_set_epi* functions.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_epi64 (__m64 __q0, __m64 __q1){return _mm_set_epi64 (__q1, __q0);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_epi32 (int __q0, int __q1, int __q2, int __q3){return _mm_set_epi32 (__q3, __q2, __q1, __q0);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_epi16 (short __q0, short __q1, short __q2, short __q3, short __q4, short __q5, short __q6, short __q7)
{
	return _mm_set_epi16 (__q7, __q6, __q5, __q4, __q3, __q2, __q1, __q0);
}

extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_epi8 (char __q00, char __q01, char __q02, char __q03, char __q04, char __q05, char __q06, char __q07, char __q08, char __q09, char __q10, char __q11, char __q12, char __q13, char __q14, char __q15)
{
  return _mm_set_epi8 (__q15, __q14, __q13, __q12, __q11, __q10, __q09, __q08, __q07, __q06, __q05, __q04, __q03, __q02, __q01, __q00);
}

//Create a vector with element 0 as *P and the rest zero.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_si128 (__m128i const *__P){return *__P;}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadu_si128 (__m128i_u const *__P){return *__P;}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadl_epi64 (__m128i_u const *__P){return _mm_set_epi64 ((__m64)0LL, *(__m64_u *)__P);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadu_si64 (void const *__P){return _mm_loadl_epi64 ((__m128i_u *)__P);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_si128 (__m128i *__P, __m128i __B){*__P = __B;}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeu_si128 (__m128i_u *__P, __m128i __B){*__P = __B;}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storel_epi64 (__m128i_u *__P, __m128i __B){*(__m64_u *)__P = (__m64) ((__v2di)__B)[0];}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeu_si64 (void *__P, __m128i __B){_mm_storel_epi64 ((__m128i_u *)__P, __B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movepi64_pi64 (__m128i __B){return (__m64) ((__v2di)__B)[0];}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movpi64_epi64 (__m64 __A){return _mm_set_epi64 ((__m64)0LL, __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_move_epi64 (__m128i __A){return (__m128i)__builtin_ia32_movq128 ((__v2di) __A);}

//Create an undefined vector.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_undefined_si128 (void)
{
  __m128i __Y = __Y;
  return __Y;
}

//Create a vector of zeros.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setzero_si128 (void){return __extension__ (__m128i)(__v4si){ 0, 0, 0, 0 };}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtepi32_pd (__m128i __A){return (__m128d)__builtin_ia32_cvtdq2pd ((__v4si) __A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtepi32_ps (__m128i __A){return (__m128)__builtin_ia32_cvtdq2ps ((__v4si) __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpd_epi32 (__m128d __A){return (__m128i)__builtin_ia32_cvtpd2dq ((__v2df) __A);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpd_pi32 (__m128d __A){return (__m64)__builtin_ia32_cvtpd2pi ((__v2df) __A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpd_ps (__m128d __A){return (__m128)__builtin_ia32_cvtpd2ps ((__v2df) __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvttpd_epi32 (__m128d __A){return (__m128i)__builtin_ia32_cvttpd2dq ((__v2df) __A);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvttpd_pi32 (__m128d __A){return (__m64)__builtin_ia32_cvttpd2pi ((__v2df) __A);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpi32_pd (__m64 __A){return (__m128d)__builtin_ia32_cvtpi2pd ((__v2si) __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtps_epi32 (__m128 __A){return (__m128i)__builtin_ia32_cvtps2dq ((__v4sf) __A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvttps_epi32 (__m128 __A){return (__m128i)__builtin_ia32_cvttps2dq ((__v4sf) __A);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtps_pd (__m128 __A){return (__m128d)__builtin_ia32_cvtps2pd ((__v4sf) __A);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvtsd_si32 (__m128d __A){return __builtin_ia32_cvtsd2si ((__v2df) __A);}

#ifdef __x86_64__
//Intel intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtsd_si64 (__m128d __A){return __builtin_ia32_cvtsd2si64 ((__v2df) __A);}
//Microsoft intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtsd_si64x (__m128d __A){return __builtin_ia32_cvtsd2si64 ((__v2df) __A);}
#endif

extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvttsd_si32 (__m128d __A){return __builtin_ia32_cvttsd2si ((__v2df) __A);}

#ifdef __x86_64__
//Intel intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvttsd_si64 (__m128d __A){return __builtin_ia32_cvttsd2si64 ((__v2df) __A);}
//Microsoft intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvttsd_si64x (__m128d __A){return __builtin_ia32_cvttsd2si64 ((__v2df) __A);}
#endif

extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsd_ss (__m128 __A, __m128d __B){return (__m128)__builtin_ia32_cvtsd2ss ((__v4sf) __A, (__v2df) __B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi32_sd (__m128d __A, int __B){return (__m128d)__builtin_ia32_cvtsi2sd ((__v2df) __A, __B);}

#ifdef __x86_64__
//Intel intrinsic.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64_sd (__m128d __A, long long __B){return (__m128d)__builtin_ia32_cvtsi642sd ((__v2df) __A, __B);}
//Microsoft intrinsic.
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64x_sd (__m128d __A, long long __B){return (__m128d)__builtin_ia32_cvtsi642sd ((__v2df) __A, __B);}
#endif

extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtss_sd (__m128d __A, __m128 __B){return (__m128d)__builtin_ia32_cvtss2sd ((__v2df) __A, (__v4sf)__B);}

#ifdef __OPTIMIZE__
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_pd(__m128d __A, __m128d __B, const int __mask){return (__m128d)__builtin_ia32_shufpd ((__v2df)__A, (__v2df)__B, __mask);}
#else
#define _mm_shuffle_pd(A, B, N)	((__m128d)__builtin_ia32_shufpd ((__v2df)(__m128d)(A), (__v2df)(__m128d)(B), (int)(N)))
#endif

extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_pd (__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_unpckhpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_pd (__m128d __A, __m128d __B){return (__m128d)__builtin_ia32_unpcklpd ((__v2df)__A, (__v2df)__B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadh_pd (__m128d __A, double const *__B){return (__m128d)__builtin_ia32_loadhpd ((__v2df)__A, __B);}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadl_pd (__m128d __A, double const *__B){return (__m128d)__builtin_ia32_loadlpd ((__v2df)__A, __B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_movemask_pd (__m128d __A){return __builtin_ia32_movmskpd ((__v2df)__A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_packs_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_packsswb128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_packs_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_packssdw128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_packus_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_packuswb128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_epi8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpckhbw128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpckhwd128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpckhdq128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_epi64 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpckhqdq128 ((__v2di)__A, (__v2di)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_epi8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpcklbw128 ((__v16qi)__A, (__v16qi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpcklwd128 ((__v8hi)__A, (__v8hi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpckldq128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_epi64 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_punpcklqdq128 ((__v2di)__A, (__v2di)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_epi8 (__m128i __A, __m128i __B){return (__m128i) ((__v16qu)__A + (__v16qu)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hu)__A + (__v8hu)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_epi32 (__m128i __A, __m128i __B){return (__m128i) ((__v4su)__A + (__v4su)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_epi64 (__m128i __A, __m128i __B){return (__m128i) ((__v2du)__A + (__v2du)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_adds_epi8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_paddsb128 ((__v16qi)__A, (__v16qi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_adds_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_paddsw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_adds_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_paddusb128 ((__v16qi)__A, (__v16qi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_adds_epu16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_paddusw128 ((__v8hi)__A, (__v8hi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_epi8 (__m128i __A, __m128i __B){return (__m128i) ((__v16qu)__A - (__v16qu)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hu)__A - (__v8hu)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_epi32 (__m128i __A, __m128i __B){return (__m128i) ((__v4su)__A - (__v4su)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_epi64 (__m128i __A, __m128i __B){return (__m128i) ((__v2du)__A - (__v2du)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_subs_epi8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psubsb128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_subs_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psubsw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_subs_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psubusb128 ((__v16qi)__A, (__v16qi)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_subs_epu16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psubusw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_madd_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmaddwd128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mulhi_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmulhw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mullo_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hu)__A * (__v8hu)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_su32 (__m64 __A, __m64 __B){return (__m64)__builtin_ia32_pmuludq ((__v2si)__A, (__v2si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_epu32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmuludq128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_slli_epi16 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psllwi128 ((__v8hi)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_slli_epi32 (__m128i __A, int __B){return (__m128i)__builtin_ia32_pslldi128 ((__v4si)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_slli_epi64 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psllqi128 ((__v2di)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srai_epi16 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psrawi128 ((__v8hi)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srai_epi32 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psradi128 ((__v4si)__A, __B);}

#ifdef __OPTIMIZE__
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_bsrli_si128 (__m128i __A, const int __N){return (__m128i)__builtin_ia32_psrldqi128 (__A, __N * 8);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_bslli_si128 (__m128i __A, const int __N){return (__m128i)__builtin_ia32_pslldqi128 (__A, __N * 8);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srli_si128 (__m128i __A, const int __N){return (__m128i)__builtin_ia32_psrldqi128 (__A, __N * 8);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_slli_si128 (__m128i __A, const int __N){return (__m128i)__builtin_ia32_pslldqi128 (__A, __N * 8);}
#else
#define _mm_bsrli_si128(A, N)	((__m128i)__builtin_ia32_psrldqi128 ((__m128i)(A), (int)(N)*8))
#define _mm_bslli_si128(A, N)	((__m128i)__builtin_ia32_pslldqi128 ((__m128i)(A), (int)(N)*8))
#define _mm_srli_si128(A, N)	((__m128i)__builtin_ia32_psrldqi128 ((__m128i)(A), (int)(N)*8))
#define _mm_slli_si128(A, N)	((__m128i)__builtin_ia32_pslldqi128 ((__m128i)(A), (int)(N)*8))
#endif

extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srli_epi16 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psrlwi128 ((__v8hi)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srli_epi32 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psrldi128 ((__v4si)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srli_epi64 (__m128i __A, int __B){return (__m128i)__builtin_ia32_psrlqi128 ((__v2di)__A, __B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sll_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psllw128((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sll_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pslld128((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sll_epi64 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psllq128((__v2di)__A, (__v2di)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sra_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psraw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sra_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psrad128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srl_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psrlw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srl_epi32 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psrld128 ((__v4si)__A, (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_srl_epi64 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psrlq128 ((__v2di)__A, (__v2di)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_and_si128 (__m128i __A, __m128i __B){return (__m128i) ((__v2du)__A & (__v2du)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_andnot_si128 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pandn128 ((__v2di)__A, (__v2di)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_or_si128 (__m128i __A, __m128i __B){return (__m128i) ((__v2du)__A | (__v2du)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_xor_si128 (__m128i __A, __m128i __B){return (__m128i) ((__v2du)__A ^ (__v2du)__B);}	
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_epi8 (__m128i __A, __m128i __B){return (__m128i) ((__v16qi)__A == (__v16qi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hi)__A == (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_epi32 (__m128i __A, __m128i __B){return (__m128i) ((__v4si)__A == (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_epi8 (__m128i __A, __m128i __B){return (__m128i) ((__v16qs)__A < (__v16qs)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hi)__A < (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_epi32 (__m128i __A, __m128i __B){return (__m128i) ((__v4si)__A < (__v4si)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_epi8 (__m128i __A, __m128i __B){return (__m128i) ((__v16qs)__A > (__v16qs)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_epi16 (__m128i __A, __m128i __B){return (__m128i) ((__v8hi)__A > (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_epi32 (__m128i __A, __m128i __B){return (__m128i) ((__v4si)__A > (__v4si)__B);}

#ifdef __OPTIMIZE__
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_extract_epi16 (__m128i const __A, int const __N){return (unsigned short) __builtin_ia32_vec_ext_v8hi ((__v8hi)__A, __N);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_insert_epi16 (__m128i const __A, int const __D, int const __N){return (__m128i) __builtin_ia32_vec_set_v8hi ((__v8hi)__A, __D, __N);}
#else
#define _mm_extract_epi16(A, N)		((int) (unsigned short) __builtin_ia32_vec_ext_v8hi ((__v8hi)(__m128i)(A), (int)(N)))
#define _mm_insert_epi16(A, D, N)	((__m128i) __builtin_ia32_vec_set_v8hi ((__v8hi)(__m128i)(A), (int)(D), (int)(N)))
#endif

extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmaxsw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmaxub128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_epi16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pminsw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pminub128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_movemask_epi8 (__m128i __A){return __builtin_ia32_pmovmskb128 ((__v16qi)__A);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mulhi_epu16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pmulhuw128 ((__v8hi)__A, (__v8hi)__B);}

#ifdef __OPTIMIZE__
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shufflehi_epi16 (__m128i __A, const int __mask){return (__m128i)__builtin_ia32_pshufhw ((__v8hi)__A, __mask);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shufflelo_epi16 (__m128i __A, const int __mask){return (__m128i)__builtin_ia32_pshuflw ((__v8hi)__A, __mask);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_epi32 (__m128i __A, const int __mask){return (__m128i)__builtin_ia32_pshufd ((__v4si)__A, __mask);}
#else
#define _mm_shufflehi_epi16(A, N)	((__m128i)__builtin_ia32_pshufhw((__v8hi)(__m128i)(A), (int)(N)))
#define _mm_shufflelo_epi16(A, N)	((__m128i)__builtin_ia32_pshuflw((__v8hi)(__m128i)(A), (int)(N)))
#define _mm_shuffle_epi32(A, N)		((__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(A), (int)(N)))
#endif

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_maskmoveu_si128 (__m128i __A, __m128i __B, char *__C){__builtin_ia32_maskmovdqu ((__v16qi)__A, (__v16qi)__B, __C);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_avg_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pavgb128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_avg_epu16 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_pavgw128 ((__v8hi)__A, (__v8hi)__B);}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sad_epu8 (__m128i __A, __m128i __B){return (__m128i)__builtin_ia32_psadbw128 ((__v16qi)__A, (__v16qi)__B);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_si32 (int *__A, int __B){__builtin_ia32_movnti (__A, __B);}

#ifdef __x86_64__
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_si64 (long long int *__A, long long int __B){__builtin_ia32_movnti64 (__A, __B);}
#endif

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_si128 (__m128i *__A, __m128i __B){__builtin_ia32_movntdq ((__v2di *)__A, (__v2di)__B);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_pd (double *__A, __m128d __B){__builtin_ia32_movntpd (__A, (__v2df)__B);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_clflush (void const *__A){__builtin_ia32_clflush (__A);}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_lfence(void){__builtin_ia32_lfence();}
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_mfence (void){__builtin_ia32_mfence ();}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi32_si128 (int __A){return _mm_set_epi32 (0, 0, 0, __A);}

#ifdef __x86_64__
//Intel intrinsic.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64_si128 (long long __A){return _mm_set_epi64x (0, __A);}
//Microsoft intrinsic.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64x_si128 (long long __A){return _mm_set_epi64x (0, __A);}
#endif

//Casts between various SP, DP, INT vector types.
//Note that these do no conversion of values, they just change the type.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castpd_ps(__m128d __A){return (__m128) __A;}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castpd_si128(__m128d __A){return (__m128i) __A;}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castps_pd(__m128 __A){return (__m128d) __A;}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castps_si128(__m128 __A){return (__m128i) __A;}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castsi128_ps(__m128i __A){return (__m128) __A;}
extern __inline __m128d __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_castsi128_pd(__m128i __A){return (__m128d) __A;}


#elif defined _MSC_VER || defined __ACC2__
	

typedef union _CRT_INTRIN _CRT_ALIGN(16) __m128i
{
	__int8				m128i_i8[16];
	__int16				m128i_i16[8];
	__int32				m128i_i32[4];    
	__int64				m128i_i64[2];
	unsigned __int8		m128i_u8[16];
	unsigned __int16	m128i_u16[8];
	unsigned __int32	m128i_u32[4];
	unsigned __int64	m128i_u64[2];
} __m128i;

typedef struct _CRT_INTRIN _CRT_ALIGN(16) __m128d
{
	double				m128d_f64[2];
} __m128d;

//DP, arithmetic
__m128d _mm_add_sd(__m128d _A, __m128d _B);
__m128d _mm_add_pd(__m128d _A, __m128d _B);
__m128d _mm_sub_sd(__m128d _A, __m128d _B);
__m128d _mm_sub_pd(__m128d _A, __m128d _B);
__m128d _mm_mul_sd(__m128d _A, __m128d _B);
__m128d _mm_mul_pd(__m128d _A, __m128d _B);
__m128d _mm_sqrt_sd(__m128d _A, __m128d _B);
__m128d _mm_sqrt_pd(__m128d _A);
__m128d _mm_div_sd(__m128d _A, __m128d _B);
__m128d _mm_div_pd(__m128d _A, __m128d _B);
__m128d _mm_min_sd(__m128d _A, __m128d _B);
__m128d _mm_min_pd(__m128d _A, __m128d _B);
__m128d _mm_max_sd(__m128d _A, __m128d _B);
__m128d _mm_max_pd(__m128d _A, __m128d _B);

//DP, logicals
__m128d _mm_and_pd(__m128d _A, __m128d _B);
__m128d _mm_andnot_pd(__m128d _A, __m128d _B);
__m128d _mm_or_pd(__m128d _A, __m128d _B);
__m128d _mm_xor_pd(__m128d _A, __m128d _B);

//DP, comparisons
__m128d _mm_cmpeq_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpeq_pd(__m128d _A, __m128d _B);
__m128d _mm_cmplt_sd(__m128d _A, __m128d _B);
__m128d _mm_cmplt_pd(__m128d _A, __m128d _B);
__m128d _mm_cmple_sd(__m128d _A, __m128d _B);
__m128d _mm_cmple_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpgt_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpgt_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpge_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpge_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpneq_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpneq_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpnlt_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpnlt_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpnle_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpnle_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpngt_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpngt_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpnge_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpnge_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpord_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpord_sd(__m128d _A, __m128d _B);
__m128d _mm_cmpunord_pd(__m128d _A, __m128d _B);
__m128d _mm_cmpunord_sd(__m128d _A, __m128d _B);
int _mm_comieq_sd(__m128d _A, __m128d _B);
int _mm_comilt_sd(__m128d _A, __m128d _B);
int _mm_comile_sd(__m128d _A, __m128d _B);
int _mm_comigt_sd(__m128d _A, __m128d _B);
int _mm_comige_sd(__m128d _A, __m128d _B);
int _mm_comineq_sd(__m128d _A, __m128d _B);
int _mm_ucomieq_sd(__m128d _A, __m128d _B);
int _mm_ucomilt_sd(__m128d _A, __m128d _B);
int _mm_ucomile_sd(__m128d _A, __m128d _B);
int _mm_ucomigt_sd(__m128d _A, __m128d _B);
int _mm_ucomige_sd(__m128d _A, __m128d _B);
int _mm_ucomineq_sd(__m128d _A, __m128d _B);

//DP, converts
__m128d _mm_cvtepi32_pd(__m128i _A);
__m128i _mm_cvtpd_epi32(__m128d _A);
__m128i _mm_cvttpd_epi32(__m128d _A);
__m128 _mm_cvtepi32_ps(__m128i _A);
__m128i _mm_cvtps_epi32(__m128 _A);
__m128i _mm_cvttps_epi32(__m128 _A);
__m128 _mm_cvtpd_ps(__m128d _A);
__m128d _mm_cvtps_pd(__m128 _A);
__m128 _mm_cvtsd_ss(__m128 _A, __m128d _B);
__m128d _mm_cvtss_sd(__m128d _A, __m128 _B);

int _mm_cvtsd_si32(__m128d _A);
int _mm_cvttsd_si32(__m128d _A);
__m128d _mm_cvtsi32_sd(__m128d _A, int _B);

extern __m64 _mm_cvtpd_pi32(__m128d _A);
extern __m64 _mm_cvttpd_pi32(__m128d _A);
extern __m128d _mm_cvtpi32_pd(__m64 _A);

//DP, misc
__m128d _mm_unpackhi_pd(__m128d _A, __m128d _B);
__m128d _mm_unpacklo_pd(__m128d _A, __m128d _B);
int _mm_movemask_pd(__m128d _A);
__m128d _mm_shuffle_pd(__m128d _A, __m128d _B, int _I);

//DP, loads
__m128d _mm_load_pd(double const*_Dp);
__m128d _mm_load1_pd(double const*_Dp);
__m128d _mm_loadr_pd(double const*_Dp);
__m128d _mm_loadu_pd(double const*_Dp);
__m128d _mm_load_sd(double const*_Dp);
__m128d _mm_loadh_pd(__m128d _A, double const*_Dp);
__m128d _mm_loadl_pd(__m128d _A, double const*_Dp);

//DP, sets
__m128d _mm_set_sd(double _W);
__m128d _mm_set1_pd(double _A);
__m128d _mm_set_pd(double _Z, double _Y);
__m128d _mm_setr_pd(double _Y, double _Z);
__m128d _mm_setzero_pd(void);
__m128d _mm_move_sd(__m128d _A, __m128d _B);

//DP, stores
void _mm_store_sd(double *_Dp, __m128d _A);
void _mm_store1_pd(double *_Dp, __m128d _A);
void _mm_store_pd(double *_Dp, __m128d _A);
void _mm_storeu_pd(double *_Dp, __m128d _A);
void _mm_storer_pd(double *_Dp, __m128d _A);
void _mm_storeh_pd(double *_Dp, __m128d _A);
void _mm_storel_pd(double *_Dp, __m128d _A);

//Integer, arithmetic
__m128i _mm_add_epi8(__m128i _A, __m128i _B);
__m128i _mm_add_epi16(__m128i _A, __m128i _B);
__m128i _mm_add_epi32(__m128i _A, __m128i _B);
__m64 _mm_add_si64(__m64 _A, __m64 _B);
__m128i _mm_add_epi64(__m128i _A, __m128i _B);
__m128i _mm_adds_epi8(__m128i _A, __m128i _B);
__m128i _mm_adds_epi16(__m128i _A, __m128i _B);
__m128i _mm_adds_epu8(__m128i _A, __m128i _B);
__m128i _mm_adds_epu16(__m128i _A, __m128i _B);
__m128i _mm_avg_epu8(__m128i _A, __m128i _B);
__m128i _mm_avg_epu16(__m128i _A, __m128i _B);
__m128i _mm_madd_epi16(__m128i _A, __m128i _B);
__m128i _mm_max_epi16(__m128i _A, __m128i _B);
__m128i _mm_max_epu8(__m128i _A, __m128i _B);
__m128i _mm_min_epi16(__m128i _A, __m128i _B);
__m128i _mm_min_epu8(__m128i _A, __m128i _B);
__m128i _mm_mulhi_epi16(__m128i _A, __m128i _B);
__m128i _mm_mulhi_epu16(__m128i _A, __m128i _B);
__m128i _mm_mullo_epi16(__m128i _A, __m128i _B);
__m64 _mm_mul_su32(__m64 _A, __m64 _B);
__m128i _mm_mul_epu32(__m128i _A, __m128i _B);
__m128i _mm_sad_epu8(__m128i _A, __m128i _B);
__m128i _mm_sub_epi8(__m128i _A, __m128i _B);
__m128i _mm_sub_epi16(__m128i _A, __m128i _B);
__m128i _mm_sub_epi32(__m128i _A, __m128i _B);
__m64 _mm_sub_si64(__m64 _A, __m64 _B);
__m128i _mm_sub_epi64(__m128i _A, __m128i _B);
__m128i _mm_subs_epi8(__m128i _A, __m128i _B);
__m128i _mm_subs_epi16(__m128i _A, __m128i _B);
__m128i _mm_subs_epu8(__m128i _A, __m128i _B);
__m128i _mm_subs_epu16(__m128i _A, __m128i _B);

//Integer, logicals
__m128i _mm_and_si128(__m128i _A, __m128i _B);
__m128i _mm_andnot_si128(__m128i _A, __m128i _B);
__m128i _mm_or_si128(__m128i _A, __m128i _B);
__m128i _mm_xor_si128(__m128i _A, __m128i _B);

//Integer, shifts
__m128i _mm_slli_si128(__m128i _A, int _Imm);
__m128i _mm_slli_epi16(__m128i _A, int _Count);
__m128i _mm_sll_epi16(__m128i _A, __m128i _Count);
__m128i _mm_slli_epi32(__m128i _A, int _Count);
__m128i _mm_sll_epi32(__m128i _A, __m128i _Count);
__m128i _mm_slli_epi64(__m128i _A, int _Count);
__m128i _mm_sll_epi64(__m128i _A, __m128i _Count);
__m128i _mm_srai_epi16(__m128i _A, int _Count);
__m128i _mm_sra_epi16(__m128i _A, __m128i _Count);
__m128i _mm_srai_epi32(__m128i _A, int _Count);
__m128i _mm_sra_epi32(__m128i _A, __m128i _Count);
__m128i _mm_srli_si128(__m128i _A, int _Imm);
__m128i _mm_srli_epi16(__m128i _A, int _Count);
__m128i _mm_srl_epi16(__m128i _A, __m128i _Count);
__m128i _mm_srli_epi32(__m128i _A, int _Count);
__m128i _mm_srl_epi32(__m128i _A, __m128i _Count);
__m128i _mm_srli_epi64(__m128i _A, int _Count);
__m128i _mm_srl_epi64(__m128i _A, __m128i _Count);

//Integer, comparisons
__m128i _mm_cmpeq_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmpeq_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmpeq_epi32(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmpgt_epi32(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi8(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi16(__m128i _A, __m128i _B);
__m128i _mm_cmplt_epi32(__m128i _A, __m128i _B);

//Integer, converts
__m128i _mm_cvtsi32_si128(int _A);
int _mm_cvtsi128_si32(__m128i _A);

//Integer, misc
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

//Integer, loads
__m128i _mm_load_si128(__m128i const*_P);
__m128i _mm_loadu_si128(__m128i const*_P);
__m128i _mm_loadl_epi64(__m128i const*_P);

//Integer, sets
__m128i _mm_set_epi64(__m64 _Q1, __m64 _Q0);
__m128i _mm_set_epi32(int _I3, int _I2, int _I1, int _I0);
__m128i _mm_set_epi16(	short _W7, short _W6, short _W5, short _W4,
						short _W3, short _W2, short _W1, short _W0);
__m128i _mm_set_epi8(	char _B15, char _B14, char _B13, char _B12, 
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
__m128i _mm_setr_epi16(	short _W0, short _W1, short _W2, short _W3, 
						short _W4, short _W5, short _W6, short _W7);
__m128i _mm_setr_epi8(	char _B15, char _B14, char _B13, char _B12, 
						char _B11, char _B10, char _B9, char _B8, 
						char _B7, char _B6, char _B5, char _B4, 
						char _B3, char _B2, char _B1, char _B0);
__m128i _mm_setzero_si128(void);

//Integer, stores
void _mm_store_si128(__m128i *_P, __m128i _B);
void _mm_storeu_si128(__m128i *_P, __m128i _B);
void _mm_storel_epi64(__m128i *_P, __m128i _Q);
void _mm_maskmoveu_si128(__m128i _D, __m128i _N, char *_P);

//Integer, moves
__m128i _mm_move_epi64(__m128i _Q);
__m128i _mm_movpi64_epi64(__m64 _Q);
__m64 _mm_movepi64_pi64(__m128i _Q);

//Cacheability support
void _mm_stream_pd(double *_Dp, __m128d _A);
void _mm_stream_si128(__m128i *_P, __m128i _A);
void _mm_clflush(void const*_P);
void _mm_lfence(void);
void _mm_mfence(void);
void _mm_stream_si32(int *_P, int _I);
void _mm_pause(void);

//New convert to float
double _mm_cvtsd_f64(__m128d _A);

//Support for casting between various SP, DP, INT vector types.
//Note that these do no conversion of values, they just change the type.
__m128  _mm_castpd_ps(__m128d);
__m128i _mm_castpd_si128(__m128d);
__m128d _mm_castps_pd(__m128);
__m128i _mm_castps_si128(__m128);
__m128  _mm_castsi128_ps(__m128i);
__m128d _mm_castsi128_pd(__m128i);

//Support for 64-bit extension intrinsics
#if defined(_M_AMD64)
__int64 _mm_cvtsd_si64(__m128d);
__int64 _mm_cvttsd_si64(__m128d);
__m128d _mm_cvtsi64_sd(__m128d, __int64);
__m128i _mm_cvtsi64_si128(__int64);
__int64 _mm_cvtsi128_si64(__m128i);
//Alternate intrinsic name definitions
#define _mm_stream_si64 _mm_stream_si64x
#endif


#else
#error Unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif