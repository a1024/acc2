#pragma once
#ifndef STDIO_H
#define STDIO_H
//#define _NO_CRT_STDIO_INLINE
//#include<stdio.h>
#include		"crtdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef char* va_list;
#define va_start(ap, paramN)	(ap=(char*)(&(paramN)+1))
#define va_arg(ap, type)		(ap+=(sizeof(type)+sizeof(int)-1)&~(sizeof(int)-1))
#define	va_end(ap)

int		__cdecl printf(const char *format, ...);
int		__cdecl vprintf(const char *format, va_list args);
int		__cdecl scanf(const char *format, ...);
int		__cdecl scanf_s(const char *format, ...);

int		__cdecl sprintf_s(char *dstBuf, size_t sizeInBytes, const char *format, ...);
int		__cdecl vsprintf_s(char *dstBuf, size_t sizeInBytes, const char *format, va_list _ArgList);

typedef void FILE;
FILE*	__cdecl fopen(const char *filename, const char *mode);
int		__cdecl fopen_s(FILE **file, const char *filename, const char *mode);
int		__cdecl fclose(FILE *file);
size_t	__cdecl fread(void *dst, size_t elementsize, size_t count, FILE *file);
size_t	__cdecl fwrite(const void *src, size_t elementsize, size_t count, FILE *file);

#ifdef __cplusplus
}
#endif

#endif//STDIO_H