#pragma once
#ifndef STDLIB_H
#define STDLIB_H

typedef unsigned char byte;

#ifdef __cplusplus
extern "C"
{
#endif

void*	__cdecl malloc(unsigned bytesize);
void*	__cdecl calloc(unsigned bytesize);
void*	__cdecl realloc(void *p, unsigned bytesize);
void	__cdecl free(void *p);

void*	__cdecl _aligned_malloc(unsigned bytesize, unsigned alignbytes);
void*	__cdecl _aligned_realloc(void *p, unsigned bytesize, unsigned alignbytes);
void	__cdecl _aligned_free(void *p);

int			__cdecl atoi(const char *str);//useless: no advance
double		__cdecl atof(const char *str);
long		__cdecl atol(const char *str);
long long	__cdecl atoll(const char *str);

void	__cdecl exit(int code);
void	__cdecl abort();

#ifdef __cplusplus
}
#endif

#endif//STDLIB_H