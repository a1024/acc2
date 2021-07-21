#include			"include/stdlib.h"
#include			"include/assert.h"
#include			"include/stdio.h"

//	#define		DEBUG_MEMORY

#ifdef DEBUG_MEMORY
#include			"include/string.h"
#define				NPOINTERS	2048
unsigned			pointers[NPOINTERS]={};
int					npointers=0;
static int			bin_search(unsigned val)
{
	int L=0, R=npointers-1;
	while(L<=R)
	{
		int m=(L+R)>>1;
		if(pointers[m]<val)
			L=m+1;
		else if(pointers[m]>val)
			R=m-1;
		else
			return m;
	}
	L+=L<npointers&&pointers[L]<val;
	return L;
}
static void			pointer_add(void *p)
{
	unsigned val=(unsigned)p;
	int off=bin_search(val);
	if(npointers>=NPOINTERS)
	{
		printf("Out of memory\n");
		return;
	}
	if(off<npointers)
		memmove(pointers+off+1, pointers+off, (npointers-off)*sizeof(unsigned));
	pointers[off]=val;
	++npointers;
}
static int			pointer_remove(void *p)
{
	unsigned val=(unsigned)p;
	int off=bin_search(val);
	if(off>=npointers||pointers[off]!=val)//not found
		return 0;
	if(off<npointers)
		memmove(pointers+off, pointers+off+1, (npointers-(off+1))*sizeof(unsigned));
	return 1;
}
static int			pointer_relocate(void *p, void *p2)
{
	if(p!=p2)
	{
		int ret=pointer_remove(p);
		if(!ret)
			return 0;
		pointer_add(p2);
	}
	return 1;
	//unsigned val=(unsigned)p;
	//int off=bin_search(val);
	//if(off>=npointers||pointers[off]!=val)
	//	return 0;
	//if(p!=p2)
	//{
	//	if(off<npointers)//remove old pointer
	//		memmove(pointers+off, pointers+off+1, (npointers-(off+1))*sizeof(unsigned));
	//	pointer_add(p2);
	//}
	//return 1;
}
static int			pointer_check(void *p)
{
	unsigned val=(unsigned)p;
	int off=bin_search(val);
	return off<npointers&&pointers[off]==val;
}
#endif

static double		init_infinity()
{
	long long val=0x7FF0000000000000;
	return (double&)val;
}
static double		inin_qnan()
{
	long long val=0xFFF0000000000000;
	return (double&)val;
}
double				_HUGE=init_infinity(), _QNAN=inin_qnan();
int					crash(const char *file, const char *function, int line, const char *expr, const char *msg, ...)
{
	printf("\n\nRUNTINE ERROR\n");
	printf("\t%s(%d):\n", file, line);
	printf("\t%s\n", function);
	if(msg)
	{
		vprintf(msg, (char*)(&msg+1));
		printf("\n");
	//	printf("%s\n", msg);
	}
	else
		printf("%s \t== false\n", expr);

	int t=0;
	scanf("%d", &t);
	abort();
	return 0;
}

#if 0
inline unsigned		alignedbuffersize(unsigned bytesize, unsigned alignbytes)
{
	return sizeof(unsigned)+bytesize+alignbytes;
}
inline void*		alignpointer(void *p, unsigned alignbytes)
{
	unsigned i=(unsigned)p;
	i+=sizeof(void*);
	return (void*)((i/alignbytes+(i%alignbytes!=0))*alignbytes);
}
extern "C" void*	__cdecl _aligned_malloc(unsigned bytesize, unsigned alignbytes)
{
	void *p=(void*)malloc(alignedbuffersize(bytesize, alignbytes));
	assert2(p, "_aligned_malloc");
	void **p2=(void**)alignpointer(p, alignbytes);
	p2[-1]=p;
#ifdef DEBUG_MEMORY
	pointer_add(p);
	printf("malloc: %p (%d)\n", p, bytesize);
	//free(malloc(1));//
#endif
	return p2;
}
extern "C" void*	__cdecl _aligned_realloc(void *p, unsigned bytesize, unsigned alignbytes)
{
	if(p==(void*)0xCDCDCDCD)//
		return nullptr;//
	void *p2=((void**)p)[-1];
#ifdef DEBUG_MEMORY
	if(!pointer_check(p2))
		printf("Invalid pointer %p\n", p2);
#endif
	void *p3=realloc(p2, alignedbuffersize(bytesize, alignbytes));//X new pointer is misaligned differently, realloc copies data with new random wrong alignment
	assert2(p3, "_aligned_realloc");
	void **p4=(void**)alignpointer(p3, alignbytes);
	p4[-1]=p3;
#ifdef DEBUG_MEMORY
	pointer_relocate(p2, p3);
	printf("realloc: %p -> %p (%d)\n", p2, p3, bytesize);
	//free(malloc(1));//
#endif
	return p4;
}
extern "C" void		__cdecl _aligned_free(void *p)
{
	void *p2=((void**)p)[-1];
#ifdef DEBUG_MEMORY
	printf("free: %p\n", p2);
	if(!pointer_remove(p2))
		printf("Invalid pointer %p\n", p2);
	//free(malloc(1));//
#endif
	free(p2);
}
#endif