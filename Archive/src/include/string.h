#pragma once
#ifndef STRING_H
#define STRING_H

#include		"crtdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void*		__cdecl memset(void *dst, int val, size_t size);
void*		__cdecl memcpy(void *dst, const void *src, size_t size);
void*		__cdecl memmove(void *dst, const void *src, size_t size);
int			__cdecl memcmp(const void *b1, const void *b2, size_t size);

int			__cdecl strcmp(const char *s1, const char *s2);
size_t		__cdecl strlen(const char *s);
size_t		__cdecl wcslen(const wchar_t *s);

#ifdef __cplusplus
}
#endif

static void	memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes)
{
	unsigned copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
}

#endif//STRING_H