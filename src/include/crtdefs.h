#pragma once
#ifndef CRTDEFS_H
#define CRTDEFS_H

typedef unsigned char byte;

#ifdef __GNUC__


#define _CRT_ALIGN(x)	__attribute__((aligned(x)))
#define	_CRT_INTRIN
#define __int8	char
#define __int16	short
#define __int32	int
#define __int64	long long

#ifdef __x86_64__
	#define	__64bit__
	typedef unsigned long long	size_t;
	typedef long long			ptrdiff_t;
#elif defined __i386__
	#define	__32bit__
	typedef unsigned	size_t;
	typedef int			ptrdiff_t;
#else
	#error Unknown processor
#endif


#elif defined _MSC_VER || defined __ACC2__


#if _MSC_VER>=1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#define _CRT_ALIGN(x)	__declspec(align(x))
#define	_CRT_INTRIN		__declspec(intrin_type)

#ifdef _M_AMD64
	#define	__64bit__
	typedef unsigned long long	size_t;
	typedef long long			ptrdiff_t;
#elif defined _M_IX86
	#define	__32bit__
	typedef unsigned	size_t;
	typedef int			ptrdiff_t;
#else
	#error Unknown processor
#endif
typedef char*		va_list;


#endif//compiler

#endif//CRTDEFS_H