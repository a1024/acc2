//xmmintrin.h - SSE intrinsics
#pragma once
#ifndef XMMINTRIN_H
#define XMMINTRIN_H

#include		"mmintrin.h"


#define _MM_SHUFFLE(fp3,fp2,fp1,fp0)	((fp3)<<6|(fp2)<<4|(fp1)<<2|(fp0))
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3)	\
{\
__m128 tmp3, tmp2, tmp1, tmp0;\
tmp0=_mm_shuffle_ps((row0), (row1), 0x44);\
tmp2=_mm_shuffle_ps((row0), (row1), 0xEE);\
tmp1=_mm_shuffle_ps((row2), (row3), 0x44);\
tmp3=_mm_shuffle_ps((row2), (row3), 0xEE);\
(row0) = _mm_shuffle_ps(tmp0, tmp1, 0x88);\
(row1) = _mm_shuffle_ps(tmp0, tmp1, 0xDD);\
(row2) = _mm_shuffle_ps(tmp2, tmp3, 0x88);\
(row3) = _mm_shuffle_ps(tmp2, tmp3, 0xDD);\
}

#define _MM_HINT_T0     1
#define _MM_HINT_T1     2
#define _MM_HINT_T2     3
#define _MM_HINT_NTA    0

//(this declspec not supported with 0.A or 0.B)
#define _MM_ALIGN16 _CRT_ALIGN(16)

//MACRO functions for setting and reading the MXCSR
#define _MM_EXCEPT_MASK       0x003f
#define _MM_EXCEPT_INVALID    0x0001
#define _MM_EXCEPT_DENORM     0x0002
#define _MM_EXCEPT_DIV_ZERO   0x0004
#define _MM_EXCEPT_OVERFLOW   0x0008
#define _MM_EXCEPT_UNDERFLOW  0x0010
#define _MM_EXCEPT_INEXACT    0x0020

#define _MM_MASK_MASK         0x1f80
#define _MM_MASK_INVALID      0x0080
#define _MM_MASK_DENORM       0x0100
#define _MM_MASK_DIV_ZERO     0x0200
#define _MM_MASK_OVERFLOW     0x0400
#define _MM_MASK_UNDERFLOW    0x0800
#define _MM_MASK_INEXACT      0x1000

#define _MM_ROUND_MASK        0x6000
#define _MM_ROUND_NEAREST     0x0000
#define _MM_ROUND_DOWN        0x2000
#define _MM_ROUND_UP          0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_FLUSH_ZERO_MASK   0x8000
#define _MM_FLUSH_ZERO_ON     0x8000
#define _MM_FLUSH_ZERO_OFF    0x0000


#ifdef __GNUC__


//The Intel API is flexible enough that we must allow aliasing with other vector types, and their scalar components.
typedef float	__m128		__attribute__((__vector_size__(16), __may_alias__));

//Unaligned version of the same type.
typedef float	__m128_u	__attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));

//Internal data types for implementing the intrinsics.
typedef float	__v4sf		__attribute__((__vector_size__(16)));

//Create an undefined vector.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_undefined_ps(void)
{
	__m128 __Y = __Y;
	return __Y;
}

//Create a vector of zeros.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setzero_ps(void)
{
	return __extension__ (__m128){ 0.0f, 0.0f, 0.0f, 0.0f };
}

//Perform the respective operation on the lower SPFP (single-precision
//floating-point) values of A and B; the upper three SPFP values are
//passed through from A.  */
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_addss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_subss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_mulss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_div_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_divss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sqrt_ss (__m128 __A){return (__m128) __builtin_ia32_sqrtss ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_rcp_ss (__m128 __A){return (__m128) __builtin_ia32_rcpss ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_rsqrt_ss (__m128 __A){return (__m128) __builtin_ia32_rsqrtss ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_minss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_maxss ((__v4sf)__A, (__v4sf)__B);}

//Perform the respective operation on the four SPFP values in A and B.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_add_ps (__m128 __A, __m128 __B){return (__m128) ((__v4sf)__A + (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sub_ps (__m128 __A, __m128 __B){return (__m128) ((__v4sf)__A - (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mul_ps (__m128 __A, __m128 __B){return (__m128) ((__v4sf)__A * (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_div_ps (__m128 __A, __m128 __B){return (__m128) ((__v4sf)__A / (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sqrt_ps (__m128 __A){return (__m128) __builtin_ia32_sqrtps ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_rcp_ps (__m128 __A){return (__m128) __builtin_ia32_rcpps ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_rsqrt_ps (__m128 __A){return (__m128) __builtin_ia32_rsqrtps ((__v4sf)__A);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_minps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_maxps ((__v4sf)__A, (__v4sf)__B);}

//Perform logical bit-wise operations on 128-bit values.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_and_ps (__m128 __A, __m128 __B){return __builtin_ia32_andps (__A, __B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_andnot_ps (__m128 __A, __m128 __B){return __builtin_ia32_andnps (__A, __B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_or_ps (__m128 __A, __m128 __B){return __builtin_ia32_orps (__A, __B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_xor_ps (__m128 __A, __m128 __B){return __builtin_ia32_xorps (__A, __B);}

//Perform a comparison on the lower SPFP values of A and B.
//If the comparison is true, place a mask of all ones in the result, otherwise a mask of zeros.
//The upper three SPFP values are passed through from A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpeqss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpltss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmple_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpless ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movss ((__v4sf) __A, (__v4sf)__builtin_ia32_cmpltss ((__v4sf) __B, (__v4sf)__A));}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpge_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movss ((__v4sf) __A, (__v4sf)__builtin_ia32_cmpless ((__v4sf) __B, (__v4sf)__A));}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpneq_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpneqss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnlt_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpnltss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnle_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpnless ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpngt_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movss ((__v4sf) __A, (__v4sf)__builtin_ia32_cmpnltss ((__v4sf) __B, (__v4sf)__A));}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnge_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movss ((__v4sf) __A, (__v4sf)__builtin_ia32_cmpnless ((__v4sf) __B, (__v4sf)__A));}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpord_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpordss ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpunord_ss (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpunordss ((__v4sf)__A, (__v4sf)__B);}

//Perform a comparison on the four SPFP values of A and B.
//For each element, if the comparison is true, place a mask of all ones in the result, otherwise a mask of zeros.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpeq_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpeqps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmplt_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpltps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmple_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpleps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpgt_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpgtps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpge_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpgeps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpneq_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpneqps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnlt_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpnltps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnle_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpnleps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpngt_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpngtps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpnge_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpngeps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpord_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpordps ((__v4sf)__A, (__v4sf)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cmpunord_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_cmpunordps ((__v4sf)__A, (__v4sf)__B);}

//Compare the lower SPFP values of A and B and return 1 if true and 0 if false.
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comieq_ss (__m128 __A, __m128 __B){return __builtin_ia32_comieq ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comilt_ss (__m128 __A, __m128 __B){return __builtin_ia32_comilt ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comile_ss (__m128 __A, __m128 __B){return __builtin_ia32_comile ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comigt_ss (__m128 __A, __m128 __B){return __builtin_ia32_comigt ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comige_ss (__m128 __A, __m128 __B){return __builtin_ia32_comige ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_comineq_ss (__m128 __A, __m128 __B){return __builtin_ia32_comineq ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomieq_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomieq ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomilt_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomilt ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomile_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomile ((__v4sf)__A, (__v4sf)__B);}	
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomigt_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomigt ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomige_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomige ((__v4sf)__A, (__v4sf)__B);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_ucomineq_ss (__m128 __A, __m128 __B){return __builtin_ia32_ucomineq ((__v4sf)__A, (__v4sf)__B);}

//Convert the lower SPFP value to a 32-bit integer according to the current rounding mode.
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvtss_si32 (__m128 __A){return __builtin_ia32_cvtss2si ((__v4sf) __A);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvt_ss2si (__m128 __A){return _mm_cvtss_si32 (__A);}

#ifdef __x86_64__
//Convert the lower SPFP value to a 32-bit integer according to the current rounding mode.  */
//Intel intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtss_si64 (__m128 __A){return __builtin_ia32_cvtss2si64 ((__v4sf) __A);}
//Microsoft intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvtss_si64x (__m128 __A){return __builtin_ia32_cvtss2si64 ((__v4sf) __A);}
#endif

//Convert the two lower SPFP values to 32-bit integers according to the current rounding mode.
//Return the integers in packed form.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtps_pi32 (__m128 __A){return (__m64) __builtin_ia32_cvtps2pi ((__v4sf) __A);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvt_ps2pi (__m128 __A){return _mm_cvtps_pi32 (__A);}

//Truncate the lower SPFP value to a 32-bit integer.
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvttss_si32 (__m128 __A){return __builtin_ia32_cvttss2si ((__v4sf) __A);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_cvtt_ss2si (__m128 __A){return _mm_cvttss_si32 (__A);}

#ifdef __x86_64__
//Truncate the lower SPFP value to a 32-bit integer.
//Intel intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvttss_si64 (__m128 __A){return __builtin_ia32_cvttss2si64 ((__v4sf) __A);}
//Microsoft intrinsic.
extern __inline long long __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_cvttss_si64x (__m128 __A){return __builtin_ia32_cvttss2si64 ((__v4sf) __A);}
#endif

//Truncate the two lower SPFP values to 32-bit integers.
//Return the integers in packed form.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvttps_pi32 (__m128 __A){return (__m64) __builtin_ia32_cvttps2pi ((__v4sf) __A);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtt_ps2pi (__m128 __A){return _mm_cvttps_pi32 (__A);}

//Convert B to a SPFP value and insert it as element zero in A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi32_ss (__m128 __A, int __B){return (__m128) __builtin_ia32_cvtsi2ss ((__v4sf) __A, __B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvt_si2ss (__m128 __A, int __B){return _mm_cvtsi32_ss (__A, __B);}

#ifdef __x86_64__
//Convert B to a SPFP value and insert it as element zero in A.
//Intel intrinsic.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64_ss (__m128 __A, long long __B){return (__m128) __builtin_ia32_cvtsi642ss ((__v4sf) __A, __B);}
//Microsoft intrinsic.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtsi64x_ss (__m128 __A, long long __B){return (__m128) __builtin_ia32_cvtsi642ss ((__v4sf) __A, __B);}
#endif

//Convert the two 32-bit values in B to SPFP form and insert them as the two lower elements in A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpi32_ps (__m128 __A, __m64 __B){return (__m128) __builtin_ia32_cvtpi2ps ((__v4sf) __A, (__v2si)__B);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvt_pi2ps (__m128 __A, __m64 __B){return _mm_cvtpi32_ps (__A, __B);}

//Convert the four signed 16-bit values in A to SPFP form.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpi16_ps(__m64 __A)
{
	__v4hi __sign;
	__v2si __hisi, __losi;
	__v4sf __zero, __ra, __rb;

	//This comparison against zero gives us a mask that can be used to
	//fill in the missing sign bits in the unpack operations below, so
	//that we get signed values after unpacking.
	__sign = __builtin_ia32_pcmpgtw ((__v4hi)0LL, (__v4hi)__A);

	//Convert the four words to doublewords.
	__losi = (__v2si) __builtin_ia32_punpcklwd ((__v4hi)__A, __sign);
	__hisi = (__v2si) __builtin_ia32_punpckhwd ((__v4hi)__A, __sign);

	//Convert the doublewords to floating point two at a time.
	__zero = (__v4sf) _mm_setzero_ps ();
	__ra = __builtin_ia32_cvtpi2ps (__zero, __losi);
	__rb = __builtin_ia32_cvtpi2ps (__ra, __hisi);

	return (__m128) __builtin_ia32_movlhps (__ra, __rb);
}

//Convert the four unsigned 16-bit values in A to SPFP form.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtpu16_ps(__m64 __A)
{
	__v2si __hisi, __losi;
	__v4sf __zero, __ra, __rb;

	//Convert the four words to doublewords.
	__losi = (__v2si) __builtin_ia32_punpcklwd ((__v4hi)__A, (__v4hi)0LL);
	__hisi = (__v2si) __builtin_ia32_punpckhwd ((__v4hi)__A, (__v4hi)0LL);

	//Convert the doublewords to floating point two at a time.
	__zero = (__v4sf) _mm_setzero_ps ();
	__ra = __builtin_ia32_cvtpi2ps (__zero, __losi);
	__rb = __builtin_ia32_cvtpi2ps (__ra, __hisi);

	return (__m128) __builtin_ia32_movlhps (__ra, __rb);
}

//Convert the low four signed 8-bit values in A to SPFP form.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtpi8_ps (__m64 __A)
{
	__v8qi __sign;

	//This comparison against zero gives us a mask that can be used to
	//fill in the missing sign bits in the unpack operations below, so
	//that we get signed values after unpacking.
	__sign = __builtin_ia32_pcmpgtb ((__v8qi)0LL, (__v8qi)__A);

	//Convert the four low bytes to words.
	__A = (__m64) __builtin_ia32_punpcklbw ((__v8qi)__A, __sign);

	return _mm_cvtpi16_ps(__A);
}

//Convert the low four unsigned 8-bit values in A to SPFP form.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtpu8_ps(__m64 __A)
{
	__A = (__m64) __builtin_ia32_punpcklbw ((__v8qi)__A, (__v8qi)0LL);
	return _mm_cvtpu16_ps(__A);
}

//Convert the four signed 32-bit values in A and B to SPFP form.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtpi32x2_ps(__m64 __A, __m64 __B)
{
	__v4sf __zero = (__v4sf) _mm_setzero_ps ();
	__v4sf __sfa = __builtin_ia32_cvtpi2ps (__zero, (__v2si)__A);
	__v4sf __sfb = __builtin_ia32_cvtpi2ps (__sfa, (__v2si)__B);
	return (__m128) __builtin_ia32_movlhps (__sfa, __sfb);
}

//Convert the four SPFP values in A to four signed 16-bit integers.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtps_pi16(__m128 __A)
{
	__v4sf __hisf = (__v4sf)__A;
	__v4sf __losf = __builtin_ia32_movhlps (__hisf, __hisf);
	__v2si __hisi = __builtin_ia32_cvtps2pi (__hisf);
	__v2si __losi = __builtin_ia32_cvtps2pi (__losf);
	return (__m64) __builtin_ia32_packssdw (__hisi, __losi);
}

//Convert the four SPFP values in A to four signed 8-bit integers.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtps_pi8(__m128 __A)
{
  __v4hi __tmp = (__v4hi) _mm_cvtps_pi16 (__A);
  return (__m64) __builtin_ia32_packsswb (__tmp, (__v4hi)0LL);
}

//Selects four specific SPFP values from A and B based on MASK.
#ifdef __OPTIMIZE__
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_ps (__m128 __A, __m128 __B, int const __mask){return (__m128) __builtin_ia32_shufps ((__v4sf)__A, (__v4sf)__B, __mask);}
#else
#define _mm_shuffle_ps(A, B, MASK)	((__m128) __builtin_ia32_shufps ((__v4sf)(__m128)(A), (__v4sf)(__m128)(B), (int)(MASK)))
#endif

//Selects and interleaves the upper two SPFP values from A and B.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpackhi_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_unpckhps ((__v4sf)__A, (__v4sf)__B);}

//Selects and interleaves the lower two SPFP values from A and B.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_unpacklo_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_unpcklps ((__v4sf)__A, (__v4sf)__B);}

//Sets the upper two SPFP values with 64-bits of data loaded from P;
//the lower two values are passed through from A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadh_pi (__m128 __A, __m64 const *__P){return (__m128) __builtin_ia32_loadhps ((__v4sf)__A, (const __v2sf *)__P);}

//Stores the upper two SPFP values of A into P.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeh_pi (__m64 *__P, __m128 __A){__builtin_ia32_storehps ((__v2sf *)__P, (__v4sf)__A);}

//Moves the upper two values of B into the lower two values of A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movehl_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movhlps ((__v4sf)__A, (__v4sf)__B);}

//Moves the lower two values of B into the upper two values of A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_movelh_ps (__m128 __A, __m128 __B){return (__m128) __builtin_ia32_movlhps ((__v4sf)__A, (__v4sf)__B);}

//Sets the lower two SPFP values with 64-bits of data loaded from P; the upper two values are passed through from A.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadl_pi (__m128 __A, __m64 const *__P){return (__m128) __builtin_ia32_loadlps ((__v4sf)__A, (const __v2sf *)__P);}

//Stores the lower two SPFP values of A into P.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storel_pi (__m64 *__P, __m128 __A){__builtin_ia32_storelps ((__v2sf *)__P, (__v4sf)__A);}

//Creates a 4-bit mask from the most significant bits of the SPFP values.
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_movemask_ps (__m128 __A){return __builtin_ia32_movmskps ((__v4sf)__A);}

//Return the contents of the control register.
extern __inline unsigned int __attribute__((__gnu_inline__, __always_inline__, __artificial__))_mm_getcsr (void){return __builtin_ia32_stmxcsr ();}

//Read exception bits from the control register.
extern __inline unsigned int __attribute__((__gnu_inline__, __always_inline__, __artificial__))_MM_GET_EXCEPTION_STATE (void){return _mm_getcsr() & _MM_EXCEPT_MASK;}
extern __inline unsigned int __attribute__((__gnu_inline__, __always_inline__, __artificial__))_MM_GET_EXCEPTION_MASK (void){return _mm_getcsr() & _MM_MASK_MASK;}
extern __inline unsigned int __attribute__((__gnu_inline__, __always_inline__, __artificial__))_MM_GET_ROUNDING_MODE (void){return _mm_getcsr() & _MM_ROUND_MASK;}
extern __inline unsigned int __attribute__((__gnu_inline__, __always_inline__, __artificial__))_MM_GET_FLUSH_ZERO_MODE (void){return _mm_getcsr() & _MM_FLUSH_ZERO_MASK;}

//Set the control register to I.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_setcsr (unsigned int __I){__builtin_ia32_ldmxcsr (__I);}

//Set exception bits in the control register.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_MM_SET_EXCEPTION_STATE(unsigned int __mask){_mm_setcsr((_mm_getcsr() & ~_MM_EXCEPT_MASK) | __mask);}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_MM_SET_EXCEPTION_MASK (unsigned int __mask){_mm_setcsr((_mm_getcsr() & ~_MM_MASK_MASK) | __mask);}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_MM_SET_ROUNDING_MODE (unsigned int __mode){_mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | __mode);}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_MM_SET_FLUSH_ZERO_MODE (unsigned int __mode){_mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO_MASK) | __mode);}

//Create a vector with element 0 as F and the rest zero.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_ss (float __F){return __extension__ (__m128)(__v4sf){ __F, 0.0f, 0.0f, 0.0f };}

//Create a vector with all four elements equal to F.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set1_ps (float __F){return __extension__ (__m128)(__v4sf){ __F, __F, __F, __F };}

extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_ps1 (float __F){return _mm_set1_ps (__F);}

//Create a vector with element 0 as *P and the rest zero.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_ss (float const *__P){return _mm_set_ss (*__P);}

//Create a vector with all four elements equal to *P.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load1_ps (float const *__P){return _mm_set1_ps (*__P);}
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_ps1 (float const *__P){return _mm_load1_ps (__P);}

//Load four SPFP values from P.  The address must be 16-byte aligned.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_load_ps (float const *__P){return *(__m128 *)__P;}

//Load four SPFP values from P.  The address need not be 16-byte aligned.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadu_ps (float const *__P){return *(__m128_u *)__P;}

//Load four SPFP values in reverse order.  The address must be aligned.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_loadr_ps (float const *__P)
{
	__v4sf __tmp = *(__v4sf *)__P;
	return (__m128) __builtin_ia32_shufps (__tmp, __tmp, _MM_SHUFFLE (0,1,2,3));
}

//Create the vector [Z Y X W].
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_set_ps (const float __Z, const float __Y, const float __X, const float __W){return __extension__ (__m128)(__v4sf){ __W, __X, __Y, __Z };}

//Create the vector [W X Y Z].
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_setr_ps (float __Z, float __Y, float __X, float __W){return __extension__ (__m128)(__v4sf){ __Z, __Y, __X, __W };}

//Stores the lower SPFP value.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_ss (float *__P, __m128 __A){*__P = ((__v4sf)__A)[0];}

extern __inline float __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_cvtss_f32 (__m128 __A){return ((__v4sf)__A)[0];}

//Store four SPFP values.  The address must be 16-byte aligned.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_ps (float *__P, __m128 __A){*(__m128 *)__P = __A;}

//Store four SPFP values.  The address need not be 16-byte aligned.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storeu_ps (float *__P, __m128 __A){*(__m128_u *)__P = __A;}

//Store the lower SPFP value across four words.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store1_ps (float *__P, __m128 __A)
{
	__v4sf __va = (__v4sf)__A;
	__v4sf __tmp = __builtin_ia32_shufps (__va, __va, _MM_SHUFFLE (0,0,0,0));
	_mm_storeu_ps (__P, __tmp);
}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_store_ps1 (float *__P, __m128 __A){_mm_store1_ps (__P, __A);}

//Store four SPFP values in reverse order.  The address must be aligned.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_storer_ps (float *__P, __m128 __A)
{
	__v4sf __va = (__v4sf)__A;
	__v4sf __tmp = __builtin_ia32_shufps (__va, __va, _MM_SHUFFLE (0,1,2,3));
	_mm_store_ps (__P, __tmp);
}

//Sets the low SPFP value of A from the low value of B.
extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_move_ss (__m128 __A, __m128 __B)
{
	return (__m128) __builtin_shuffle ((__v4sf)__A, (__v4sf)__B, __extension__(__attribute__((__vector_size__ (16))) int){4,1,2,3});
}

//Extracts one of the four words of A.  The selector N must be immediate.
#ifdef __OPTIMIZE__
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_extract_pi16 (__m64 const __A, int const __N){return __builtin_ia32_vec_ext_v4hi ((__v4hi)__A, __N);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pextrw (__m64 const __A, int const __N){return _mm_extract_pi16 (__A, __N);}
#else
#define _mm_extract_pi16(A, N)	((int)__builtin_ia32_vec_ext_v4hi ((__v4hi)(__m64)(A), (int)(N)))
#define _m_pextrw(A, N)			_mm_extract_pi16(A, N)
#endif

//Inserts word D into one of four words of A.
//The selector N must be immediate.
#ifdef __OPTIMIZE__
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_insert_pi16 (__m64 const __A, int const __D, int const __N){return (__m64) __builtin_ia32_vec_set_v4hi ((__v4hi)__A, __D, __N);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pinsrw (__m64 const __A, int const __D, int const __N){return _mm_insert_pi16 (__A, __D, __N);}
#else
#define _mm_insert_pi16(A, D, N)	((__m64) __builtin_ia32_vec_set_v4hi ((__v4hi)(__m64)(A), (int)(D), (int)(N)))
#define _m_pinsrw(A, D, N)			_mm_insert_pi16(A, D, N)
#endif

//Compute the element-wise maximum of signed 16-bit values.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_pi16 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pmaxsw ((__v4hi)__A, (__v4hi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pmaxsw (__m64 __A, __m64 __B){return _mm_max_pi16 (__A, __B);}

//Compute the element-wise maximum of unsigned 8-bit values.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_max_pu8 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pmaxub ((__v8qi)__A, (__v8qi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pmaxub (__m64 __A, __m64 __B){return _mm_max_pu8 (__A, __B);}

//Compute the element-wise minimum of signed 16-bit values.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_pi16 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pminsw ((__v4hi)__A, (__v4hi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pminsw (__m64 __A, __m64 __B){return _mm_min_pi16 (__A, __B);}

//Compute the element-wise minimum of unsigned 8-bit values.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_min_pu8 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pminub ((__v8qi)__A, (__v8qi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pminub (__m64 __A, __m64 __B){return _mm_min_pu8 (__A, __B);}

//Create an 8-bit mask of the signs of 8-bit values.
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_movemask_pi8 (__m64 __A){return __builtin_ia32_pmovmskb ((__v8qi)__A);}
extern __inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_m_pmovmskb (__m64 __A){return _mm_movemask_pi8 (__A);}

//Multiply four unsigned 16-bit values in A by four unsigned 16-bit values in B and produce the high 16 bits of the 32-bit results.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_mulhi_pu16 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pmulhuw ((__v4hi)__A, (__v4hi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pmulhuw (__m64 __A, __m64 __B){return _mm_mulhi_pu16 (__A, __B);}

//Return a combination of the four 16-bit values in A.  The selector must be an immediate.
#ifdef __OPTIMIZE__
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_shuffle_pi16 (__m64 __A, int const __N){return (__m64) __builtin_ia32_pshufw ((__v4hi)__A, __N);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pshufw (__m64 __A, int const __N){return _mm_shuffle_pi16 (__A, __N);}
#else
#define _mm_shuffle_pi16(A, N)	((__m64) __builtin_ia32_pshufw ((__v4hi)(__m64)(A), (int)(N)))
#define _m_pshufw(A, N)			_mm_shuffle_pi16 (A, N)
#endif

//Conditionally store byte elements of A into P.
//The high bit of each byte in the selector N determines whether the corresponding byte from A is stored.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_maskmove_si64 (__m64 __A, __m64 __N, char *__P)
{
#ifdef __MMX_WITH_SSE__
	//Emulate MMX maskmovq with SSE2 maskmovdqu and handle unmapped bits 64:127 at address __P.
	typedef long long __v2di __attribute__ ((__vector_size__ (16)));
	typedef char __v16qi __attribute__ ((__vector_size__ (16)));
	//Zero-extend __A and __N to 128 bits.
	__v2di __A128 = __extension__ (__v2di) { ((__v1di) __A)[0], 0 };
	__v2di __N128 = __extension__ (__v2di) { ((__v1di) __N)[0], 0 };

	//Check the alignment of __P.
	__SIZE_TYPE__ offset = ((__SIZE_TYPE__) __P) & 0xf;
	if (offset)
	{
		//If the misalignment of __P > 8, subtract __P by 8 bytes.
		//Otherwise, subtract __P by the misalignment.
		if (offset > 8)
			offset = 8;
		__P = (char *) (((__SIZE_TYPE__) __P) - offset);

		//Shift __A128 and __N128 to the left by the adjustment.
		switch (offset)
		{
		case 1:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 8);
			break;
		case 2:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 2 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 2 * 8);
			break;
		case 3:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 3 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 3 * 8);
			break;
		case 4:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 4 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 4 * 8);
			break;
		case 5:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 5 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 5 * 8);
			break;
		case 6:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 6 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 6 * 8);
			break;
		case 7:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 7 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 7 * 8);
			break;
		case 8:
			__A128 = __builtin_ia32_pslldqi128 (__A128, 8 * 8);
			__N128 = __builtin_ia32_pslldqi128 (__N128, 8 * 8);
			break;
		default:
			break;
		}
	}
	__builtin_ia32_maskmovdqu((__v16qi)__A128, (__v16qi)__N128, __P);
#else
	__builtin_ia32_maskmovq((__v8qi)__A, (__v8qi)__N, __P);
#endif
}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_m_maskmovq (__m64 __A, __m64 __N, char *__P){_mm_maskmove_si64 (__A, __N, __P);}

//Compute the rounded averages of the unsigned 8-bit values in A and B.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_avg_pu8 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pavgb ((__v8qi)__A, (__v8qi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pavgb (__m64 __A, __m64 __B){return _mm_avg_pu8 (__A, __B);}

//Compute the rounded averages of the unsigned 16-bit values in A and B.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_avg_pu16 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_pavgw ((__v4hi)__A, (__v4hi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_pavgw (__m64 __A, __m64 __B){return _mm_avg_pu16 (__A, __B);}

//Compute the sum of the absolute differences of the unsigned 8-bit values in A and B.
//Return the value in the lower 16-bit word; the upper words are cleared.
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_mm_sad_pu8 (__m64 __A, __m64 __B){return (__m64) __builtin_ia32_psadbw ((__v8qi)__A, (__v8qi)__B);}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))	_m_psadbw (__m64 __A, __m64 __B){return _mm_sad_pu8 (__A, __B);}

//Stores the data in A to the address P without polluting the caches.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_pi (__m64 *__P, __m64 __A){__builtin_ia32_movntq ((unsigned long long *)__P, (unsigned long long)__A);}

//Likewise.  The address must be 16-byte aligned.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_stream_ps (float *__P, __m128 __A){__builtin_ia32_movntps (__P, (__v4sf)__A);}

//Guarantees that every preceding store is globally visible before any subsequent store.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))		_mm_sfence (void){__builtin_ia32_sfence();}


#elif defined _MSC_VER || defined __ACC2__



#define _MM_SET_EXCEPTION_STATE(mask)	_mm_setcsr((_mm_getcsr()&~_MM_EXCEPT_MASK)|(mask))
#define _MM_GET_EXCEPTION_STATE()		(_mm_getcsr()&_MM_EXCEPT_MASK)

#define _MM_SET_EXCEPTION_MASK(mask)	_mm_setcsr((_mm_getcsr()&~_MM_MASK_MASK)|(mask))
#define _MM_GET_EXCEPTION_MASK()		(_mm_getcsr()&_MM_MASK_MASK)

#define _MM_SET_ROUNDING_MODE(mode)		_mm_setcsr((_mm_getcsr()&~_MM_ROUND_MASK)|(mode))
#define _MM_GET_ROUNDING_MODE()			(_mm_getcsr()&_MM_ROUND_MASK)

#define _MM_SET_FLUSH_ZERO_MODE(mode)	_mm_setcsr((_mm_getcsr()&~_MM_FLUSH_ZERO_MASK)|(mode))
#define _MM_GET_FLUSH_ZERO_MODE(mode)	(_mm_getcsr()&_MM_FLUSH_ZERO_MASK)

typedef union _CRT_INTRIN _CRT_ALIGN(16) __m128
{
	float				m128_f32[4];
	unsigned __int64	m128_u64[2];
	__int8				m128_i8[16];
	__int16				m128_i16[8];
	__int32				m128_i32[4];
	__int64				m128_i64[2];
	unsigned __int8		m128_u8[16];
	unsigned __int16	m128_u16[8];
	unsigned __int32	m128_u32[4];
} __m128;

//FP, arithmetic
__m128 _mm_add_ss(__m128 _A, __m128 _B);
__m128 _mm_add_ps(__m128 _A, __m128 _B);
__m128 _mm_sub_ss(__m128 _A, __m128 _B);
__m128 _mm_sub_ps(__m128 _A, __m128 _B);
__m128 _mm_mul_ss(__m128 _A, __m128 _B);
__m128 _mm_mul_ps(__m128 _A, __m128 _B);
__m128 _mm_div_ss(__m128 _A, __m128 _B);
__m128 _mm_div_ps(__m128 _A, __m128 _B);
__m128 _mm_sqrt_ss(__m128 _A);
__m128 _mm_sqrt_ps(__m128 _A);
__m128 _mm_rcp_ss(__m128 _A);
__m128 _mm_rcp_ps(__m128 _A);
__m128 _mm_rsqrt_ss(__m128 _A);
__m128 _mm_rsqrt_ps(__m128 _A);
__m128 _mm_min_ss(__m128 _A, __m128 _B);
__m128 _mm_min_ps(__m128 _A, __m128 _B);
__m128 _mm_max_ss(__m128 _A, __m128 _B);
__m128 _mm_max_ps(__m128 _A, __m128 _B);

//FP, logical
__m128 _mm_and_ps(__m128 _A, __m128 _B);
__m128 _mm_andnot_ps(__m128 _A, __m128 _B);
__m128 _mm_or_ps(__m128 _A, __m128 _B);
__m128 _mm_xor_ps(__m128 _A, __m128 _B);

//FP, comparison
__m128 _mm_cmpeq_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpeq_ps(__m128 _A, __m128 _B);
__m128 _mm_cmplt_ss(__m128 _A, __m128 _B);
__m128 _mm_cmplt_ps(__m128 _A, __m128 _B);
__m128 _mm_cmple_ss(__m128 _A, __m128 _B);
__m128 _mm_cmple_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpgt_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpgt_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpge_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpge_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpneq_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpneq_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpnlt_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpnlt_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpnle_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpnle_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpngt_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpngt_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpnge_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpnge_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpord_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpord_ps(__m128 _A, __m128 _B);
__m128 _mm_cmpunord_ss(__m128 _A, __m128 _B);
__m128 _mm_cmpunord_ps(__m128 _A, __m128 _B);
int _mm_comieq_ss(__m128 _A, __m128 _B);
int _mm_comilt_ss(__m128 _A, __m128 _B);
int _mm_comile_ss(__m128 _A, __m128 _B);
int _mm_comigt_ss(__m128 _A, __m128 _B);
int _mm_comige_ss(__m128 _A, __m128 _B);
int _mm_comineq_ss(__m128 _A, __m128 _B);
int _mm_ucomieq_ss(__m128 _A, __m128 _B);
int _mm_ucomilt_ss(__m128 _A, __m128 _B);
int _mm_ucomile_ss(__m128 _A, __m128 _B);
int _mm_ucomigt_ss(__m128 _A, __m128 _B);
int _mm_ucomige_ss(__m128 _A, __m128 _B);
int _mm_ucomineq_ss(__m128 _A, __m128 _B);

//FP, conversions
int _mm_cvt_ss2si(__m128 _A);
__m64 _mm_cvt_ps2pi(__m128 _A);
int _mm_cvtt_ss2si(__m128 _A);
__m64 _mm_cvtt_ps2pi(__m128 _A);
__m128 _mm_cvt_si2ss(__m128, int);
__m128 _mm_cvt_pi2ps(__m128, __m64);
float _mm_cvtss_f32(__m128 _A);

//Support for 64-bit extension intrinsics
#if defined(_M_AMD64)
__int64 _mm_cvtss_si64(__m128 _A);
__int64 _mm_cvttss_si64(__m128 _A);
__m128  _mm_cvtsi64_ss(__m128 _A, __int64 _B);
#endif

//FP, misc
__m128 _mm_shuffle_ps(__m128 _A, __m128 _B, unsigned int _Imm8);
__m128 _mm_unpackhi_ps(__m128 _A, __m128 _B);
__m128 _mm_unpacklo_ps(__m128 _A, __m128 _B);
__m128 _mm_loadh_pi(__m128, __m64 const*);
__m128 _mm_movehl_ps(__m128, __m128);
__m128 _mm_movelh_ps(__m128, __m128);
void _mm_storeh_pi(__m64 *, __m128);
__m128 _mm_loadl_pi(__m128, __m64 const*);
void _mm_storel_pi(__m64 *, __m128);
int _mm_movemask_ps(__m128 _A);


//Integer extensions
int _m_pextrw(__m64, int);
__m64 _m_pinsrw(__m64, int, int);
__m64 _m_pmaxsw(__m64, __m64);
__m64 _m_pmaxub(__m64, __m64);
__m64 _m_pminsw(__m64, __m64);
__m64 _m_pminub(__m64, __m64);
int _m_pmovmskb(__m64);
__m64 _m_pmulhuw(__m64, __m64);
__m64 _m_pshufw(__m64, int);
void _m_maskmovq(__m64, __m64, char *);
__m64 _m_pavgb(__m64, __m64);
__m64 _m_pavgw(__m64, __m64);
__m64 _m_psadbw(__m64, __m64);

//memory & initialization
__m128 _mm_set_ss(float _A);
__m128 _mm_set_ps1(float _A);
__m128 _mm_set_ps(float _A, float _B, float _C, float _D);
__m128 _mm_setr_ps(float _A, float _B, float _C, float _D);
__m128 _mm_setzero_ps(void);
__m128 _mm_load_ss(float const*_A);
__m128 _mm_load_ps1(float const*_A);
__m128 _mm_load_ps(float const*_A);
__m128 _mm_loadr_ps(float const*_A);
__m128 _mm_loadu_ps(float const*_A);
void _mm_store_ss(float *_V, __m128 _A);
void _mm_store_ps1(float *_V, __m128 _A);
void _mm_store_ps(float *_V, __m128 _A);
void _mm_storer_ps(float *_V, __m128 _A);
void _mm_storeu_ps(float *_V, __m128 _A);
void _mm_prefetch(char const*_A, int _Sel);
void _mm_stream_pi(__m64 *, __m64);
void _mm_stream_ps(float *, __m128);
__m128 _mm_move_ss(__m128 _A, __m128 _B);

void _mm_sfence(void);
unsigned int _mm_getcsr(void);
void _mm_setcsr(unsigned int);

#ifdef __ICL
void* __cdecl _mm_malloc(size_t _Siz, size_t _Al);
void __cdecl _mm_free(void *_P);
#endif

//Alternate intrinsic names definition
#define _mm_cvtss_si32    _mm_cvt_ss2si
#define _mm_cvtps_pi32    _mm_cvt_ps2pi
#define _mm_cvttss_si32   _mm_cvtt_ss2si
#define _mm_cvttps_pi32   _mm_cvtt_ps2pi
#define _mm_cvtsi32_ss    _mm_cvt_si2ss
#define _mm_cvtpi32_ps    _mm_cvt_pi2ps
#define _mm_extract_pi16  _m_pextrw
#define _mm_insert_pi16   _m_pinsrw
#define _mm_max_pi16      _m_pmaxsw
#define _mm_max_pu8       _m_pmaxub
#define _mm_min_pi16      _m_pminsw
#define _mm_min_pu8       _m_pminub
#define _mm_movemask_pi8  _m_pmovmskb
#define _mm_mulhi_pu16    _m_pmulhuw
#define _mm_shuffle_pi16  _m_pshufw
#define _mm_maskmove_si64 _m_maskmovq
#define _mm_avg_pu8       _m_pavgb
#define _mm_avg_pu16      _m_pavgw
#define _mm_sad_pu8       _m_psadbw
#define _mm_set1_ps       _mm_set_ps1
#define _mm_load1_ps      _mm_load_ps1
#define _mm_store1_ps     _mm_store_ps1

//UTILITY INTRINSICS FUNCTION DEFINITIONS START HERE

//NAME		: _mm_cvtpi16_ps
//DESCRIPTION : Convert 4 16-bit signed integer values to 4 single-precision float values
//IN		: __m64 a
//OUT		: none
//RETURN	: __m128 : (float)a
__inline __m128 _mm_cvtpi16_ps(__m64 a)
{
	__m128 tmp;
	__m64  ext_val = _mm_cmpgt_pi16(_mm_setzero_si64(), a);

	tmp=_mm_cvtpi32_ps(_mm_setzero_ps(), _mm_unpackhi_pi16(a, ext_val));
	return _mm_cvtpi32_ps(_mm_movelh_ps(tmp, tmp), _mm_unpacklo_pi16(a, ext_val));
}

//NAME		: _mm_cvtpu16_ps
//DESCRIPTION : Convert 4 16-bit unsigned integer values to 4 single-precision float values
//IN		: __m64 a
//OUT		: none
//RETURN	: __m128 : (float)a
__inline __m128 _mm_cvtpu16_ps(__m64 a)
{
	__m128 tmp;
	__m64  ext_val = _mm_setzero_si64();

	tmp = _mm_cvtpi32_ps(_mm_setzero_ps(), _mm_unpackhi_pi16(a, ext_val));
	return(_mm_cvtpi32_ps(_mm_movelh_ps(tmp, tmp), _mm_unpacklo_pi16(a, ext_val)));
}

//NAME		: _mm_cvtps_pi16
//DESCRIPTION : Convert 4 single-precision float values to 4 16-bit integer values
//IN		: __m128 a
//OUT		: none
//RETURN	: __m64 : (short)a
__inline __m64 _mm_cvtps_pi16(__m128 a)
{
	return _mm_packs_pi32(_mm_cvtps_pi32(a), _mm_cvtps_pi32(_mm_movehl_ps(a, a)));
}

//NAME		: _mm_cvtpi8_ps
//DESCRIPTION : Convert 4 8-bit integer values to 4 single-precision float values
//IN		: __m64 a
//OUT		: none
//RETURN	: __m128 : (float)a
__inline __m128 _mm_cvtpi8_ps(__m64 a)
{
	__m64  ext_val = _mm_cmpgt_pi8(_mm_setzero_si64(), a);

	return _mm_cvtpi16_ps(_mm_unpacklo_pi8(a, ext_val));
}

//NAME : _mm_cvtpu8_ps
//DESCRIPTION : Convert 4 8-bit unsigned integer values to 4 single-precision float values
//IN : __m64 a
//OUT : none
//RETURN : __m128 : (float)a
__inline __m128 _mm_cvtpu8_ps(__m64 a)
{
	return _mm_cvtpu16_ps(_mm_unpacklo_pi8(a, _mm_setzero_si64()));
}

//NAME		: _mm_cvtps_pi8
//DESCRIPTION : Convert 4 single-precision float values to 4 8-bit integer values
//IN		: __m128 a
//OUT		: none
//RETURN	: __m64 : (char)a
__inline __m64 _mm_cvtps_pi8(__m128 a)
{
	return _mm_packs_pi16(_mm_cvtps_pi16(a), _mm_setzero_si64());
}

//NAME		: _mm_cvtpi32x2_ps
//DESCRIPTION : Convert 4 32-bit integer values to 4 single-precision float values
//IN		:	__m64 a : operand 1
//				__m64 b : operand 2
//OUT		: none
//RETURN	: __m128 : (float)a,(float)b
__inline __m128 _mm_cvtpi32x2_ps(__m64 a, __m64 b)
{
	return _mm_movelh_ps(_mm_cvt_pi2ps(_mm_setzero_ps(), a), _mm_cvt_pi2ps(_mm_setzero_ps(), b)); 
}


#else
#error Unknown platform
#endif//compiler

#endif//XMMINTRIN_H