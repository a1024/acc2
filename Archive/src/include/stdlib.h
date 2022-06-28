#pragma once
#ifndef STDLIB_H
#define STDLIB_H

#include		"crtdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void*	__cdecl malloc(size_t bytesize);
void*	__cdecl calloc(size_t count, size_t esize);
void*	__cdecl realloc(void *p, size_t bytesize);
void	__cdecl free(void *p);

#if defined _MSC_VER || defined __ACC2__
void*	__cdecl _aligned_malloc(size_t bytesize, unsigned alignbytes);
void*	__cdecl _aligned_realloc(void *p, unsigned bytesize, unsigned alignbytes);
void	__cdecl _aligned_free(void *p);
#else
#include		"string.h"
struct			BufferHeader
{
	void *ptr;
	size_t bytesize;
	byte data[];
};
static void*	_aligned_malloc(size_t bytesize, unsigned alignbytes)
{
	void *ptr=malloc(sizeof(BufferHeader)+bytesize+alignbytes);
	if(!ptr)
		return 0;
	size_t value=(size_t)((byte*)ptr+sizeof(BufferHeader)+alignbytes);
	value-=value%alignbytes;
	BufferHeader *header=(BufferHeader*)value-1;
	header->ptr=ptr;
	header->bytesize=bytesize;
	return (void*)header->data;
}
static void		_aligned_free(void *p)
{
	BufferHeader *header=(BufferHeader*)p-1;
	free(header->ptr);
}
static void*	_aligned_realloc(void *p, size_t bytesize, unsigned alignbytes)//waste
{
	BufferHeader *h1=(BufferHeader*)p-1;
	if(bytesize==h1->bytesize)
		return p;
	void *p2=_aligned_malloc(bytesize, alignbytes);
	if(!p2)
		return 0;
	if(bytesize<h1->bytesize)
		memcpy(p2, p, bytesize);
	else
		memcpy(p2, p, h1->bytesize);
	_aligned_free(p);
	return p2;
}
#endif//_MSC_VER

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