#include		"acc.h"
#include		<stdio.h>
#include		<stdlib.h>
#include		<stdarg.h>
#include		<string.h>
#include		<sys/stat.h>
#include		<math.h>
#include		<errno.h>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include		<Windows.h>//QueryPerformance...
#else
#include		<time.h>//clock_gettime
#define			sprintf_s	snprintf
#define			vsprintf_s	vsnprintf
#ifndef _HUGE
#define			_HUGE		HUGE_VAL
#endif
#endif
static const char file[]=__FILE__;

char			g_buf[G_BUF_SIZE]={0};

void			memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes)
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
void			memswap_slow(void *p1, void *p2, size_t size)
{
	unsigned char *s1=(unsigned char*)p1, *s2=(unsigned char*)p2, *end=s1+size;
	for(;s1<end;++s1, ++s2)
	{
		const unsigned char t=*s1;
		*s1=*s2;
		*s2=t;
	}
}
void 			memswap(void *p1, void *p2, size_t size, void *temp)
{
	memcpy(temp, p1, size);
	memcpy(p1, p2, size);
	memcpy(p2, temp, size);
}
void			memreverse(void *p, size_t count, size_t esize)
{
	size_t totalsize=count*esize;
	unsigned char *s1=(unsigned char*)p, *s2=s1+totalsize-esize;
	void *temp=malloc(esize);
	while(s1<s2)
	{
		memswap(s1, s2, esize, temp);
		s1+=esize, s2-=esize;
	}
	free(temp);
}
void 			memrotate(void *p, size_t byteoffset, size_t bytesize, void *temp)
{
	unsigned char *buf=(unsigned char*)p;

	if(byteoffset<bytesize-byteoffset)
	{
		memcpy(temp, buf, byteoffset);
		memmove(buf, buf+byteoffset, bytesize-byteoffset);
		memcpy(buf+bytesize-byteoffset, temp, byteoffset);
	}
	else
	{
		memcpy(temp, buf+byteoffset, bytesize-byteoffset);
		memmove(buf+bytesize-byteoffset, buf, byteoffset);
		memcpy(buf, temp, bytesize-byteoffset);
	}
}
int 			binary_search(const void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*), const void *val, size_t *idx)
{
	const unsigned char *buf=(const unsigned char*)base;
	ptrdiff_t L=0, R=(ptrdiff_t)count-1, mid;
	int ret;

	while(L<=R)
	{
		mid=(L+R)>>1;
		ret=threeway(buf+mid*esize, val);
		if(ret<0)
			L=mid+1;
		else if(ret>0)
			R=mid-1;
		else
		{
			if(idx)
				*idx=mid;
			return 1;
		}
	}
	if(idx)
		*idx=L+(L<(ptrdiff_t)count&&threeway(buf+L*esize, val)<0);
	return 0;
}
void 			isort(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*))
{
	unsigned char *buf=(unsigned char*)base;
	size_t k;
	void *temp;

	if(count<2)
		return;

	temp=malloc((count>>1)*esize);
	for(k=1;k<count;++k)
	{
		size_t idx=0;
		binary_search(buf, k, esize, threeway, buf+k*esize, &idx);
		if(idx<k)
			memrotate(buf+idx*esize, (k-idx)*esize, (k+1-idx)*esize, temp);
	}
	free(temp);
}
int				acme_getopt(int argc, char **argv, int *start, const char **keywords, int kw_count)
{
	int k, len;
	const char *arg, *cand;

	if(*start>=argc)
		return OPT_ENDOFARGS;
	
	arg=argv[*start];
	len=strlen(arg);
	if(len<=0)
		return OPT_INVALIDARG;
	//len>=1
	if(arg[0]!='-')
		return OPT_NOMATCH;
	++arg, --len;
	if(len<=0)
		return OPT_INVALIDARG;
	//len>=1
	if(arg[0]!='-')//short form (single dash followed by one character)
	{
		if(len!=1)
			return OPT_INVALIDARG;
		//len==1
		for(k=0;k<kw_count;++k)
		{
			cand=keywords[k];
			if(cand[0]==arg[0])
				return k;
		}
	}
	else//long form (double dash followed by a word)
	{
		++arg, --len;
		if(len<=0)
			return OPT_INVALIDARG;
		//len>=1
		for(k=0;k<kw_count;++k)
		{
			cand=keywords[k];
			if(!strcmp(arg, cand+1))
				return k;
		}
	}
	return OPT_NOMATCH;
}

int				floor_log2(unsigned n)
{
	int logn=0;
	int sh=(((short*)&n)[1]!=0)<<4;	logn^=sh, n>>=sh;	//21.54
		sh=(((char*)&n)[1]!=0)<<3;	logn^=sh, n>>=sh;
		sh=((n&0x000000F0)!=0)<<2;	logn^=sh, n>>=sh;
		sh=((n&0x0000000C)!=0)<<1;	logn^=sh, n>>=sh;
		sh=((n&0x00000002)!=0);		logn^=sh;
	return logn;
}
int				floor_log10(double x)
{
	static const double pmask[]=//positive powers
	{
		1, 10,		//10^2^0
		1, 100,		//10^2^1
		1, 1e4,		//10^2^2
		1, 1e8,		//10^2^3
		1, 1e16,	//10^2^4
		1, 1e32,	//10^2^5
		1, 1e64,	//10^2^6
		1, 1e128,	//10^2^7
		1, 1e256	//10^2^8
	};
	static const double nmask[]=//negative powers
	{
		1, 0.1,		//1/10^2^0
		1, 0.01,	//1/10^2^1
		1, 1e-4,	//1/10^2^2
		1, 1e-8,	//1/10^2^3
		1, 1e-16,	//1/10^2^4
		1, 1e-32,	//1/10^2^5
		1, 1e-64,	//1/10^2^6
		1, 1e-128,	//1/10^2^7
		1, 1e-256	//1/10^2^8
	};
	int logn, sh;
	if(x<=0)
		return 0x80000000;
	if(x>=1)
	{
		logn=0;
		sh=(x>=pmask[17])<<8;	logn+=sh, x*=nmask[16+(sh!=0)];
		sh=(x>=pmask[15])<<7;	logn+=sh, x*=nmask[14+(sh!=0)];
		sh=(x>=pmask[13])<<6;	logn+=sh, x*=nmask[12+(sh!=0)];
		sh=(x>=pmask[11])<<5;	logn+=sh, x*=nmask[10+(sh!=0)];
		sh=(x>=pmask[9])<<4;	logn+=sh, x*=nmask[8+(sh!=0)];
		sh=(x>=pmask[7])<<3;	logn+=sh, x*=nmask[6+(sh!=0)];
		sh=(x>=pmask[5])<<2;	logn+=sh, x*=nmask[4+(sh!=0)];
		sh=(x>=pmask[3])<<1;	logn+=sh, x*=nmask[2+(sh!=0)];
		sh= x>=pmask[1];		logn+=sh;
		return logn;
	}
	logn=-1;
	sh=(x<nmask[17])<<8;	logn-=sh;	x*=pmask[16+(sh!=0)];
	sh=(x<nmask[15])<<7;	logn-=sh;	x*=pmask[14+(sh!=0)];
	sh=(x<nmask[13])<<6;	logn-=sh;	x*=pmask[12+(sh!=0)];
	sh=(x<nmask[11])<<5;	logn-=sh;	x*=pmask[10+(sh!=0)];
	sh=(x<nmask[9])<<4;		logn-=sh;	x*=pmask[8+(sh!=0)];
	sh=(x<nmask[7])<<3;		logn-=sh;	x*=pmask[6+(sh!=0)];
	sh=(x<nmask[5])<<2;		logn-=sh;	x*=pmask[4+(sh!=0)];
	sh=(x<nmask[3])<<1;		logn-=sh;	x*=pmask[2+(sh!=0)];
	sh= x<nmask[1];			logn-=sh;
	return logn;
}
double			power(double x, int y)
{
	double mask[]={1, 0}, product=1;
	if(y<0)
		mask[1]=1/x, y=-y;
	else
		mask[1]=x;
	for(;;)
	{
		product*=mask[y&1], y>>=1;	//67.7
		if(!y)
			return product;
		mask[1]*=mask[1];
	}
	return product;
}
double			_10pow(int n)
{
	static double *mask=0;
	int k;
//	const double _ln10=log(10.);
	if(!mask)
	{
		mask=(double*)malloc(616*sizeof(double));
		for(k=-308;k<308;++k)		//23.0
			mask[k+308]=power(10., k);
		//	mask[k+308]=exp(k*_ln10);//inaccurate
	}
	if(n<-308)
		return 0;
	if(n>307)
		return _HUGE;
	return mask[n+308];
}
int				minimum(int a, int b)
{
	return a<b?a:b;
}
int				maximum(int a, int b)
{
	return a>b?a:b;
}
int				acme_isdigit(char c, char base)
{
	switch(base)
	{
	case 2:		return BETWEEN('0', c, '1');
	case 8:		return BETWEEN('0', c, '7');
	case 10:	return BETWEEN('0', c, '9');
	case 16:	return BETWEEN('0', c, '9')||BETWEEN('A', c&0xDF, 'F');
	}
	return 0;
}

double			time_ms()
{
#ifdef _MSC_VER
	static double inv_f=0;
	LARGE_INTEGER li;
	//if(!inv_f)
	//{
		QueryPerformanceFrequency(&li);
		inv_f=1/(double)li.QuadPart;
	//}
	QueryPerformanceCounter(&li);
	return 1000.*(double)li.QuadPart*inv_f;
#else
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec*1000+t.tv_nsec*1e-6;
#endif
}

//error handling
char			first_error_msg[G_BUF_SIZE]={0}, latest_error_msg[G_BUF_SIZE]={0};
int				log_error(const char *file, int line, const char *format, ...)
{
	int firsttime=first_error_msg[0]=='\0';

	int size=strlen(file), start=size-1;
	for(;start>=0&&file[start]!='/'&&file[start]!='\\';--start);
	start+=start==-1||file[start]=='/'||file[start]=='\\';

	int printed=sprintf_s(latest_error_msg, G_BUF_SIZE, "%s(%d): ", file+start, line);
	va_list args;
	va_start(args, format);
	printed+=vsprintf_s(latest_error_msg+printed, G_BUF_SIZE-printed, format, args);
	va_end(args);

	if(firsttime)
		memcpy(first_error_msg, latest_error_msg, printed+1);
	fprintf(stderr, "%s\n", latest_error_msg);
	//messagebox(MBOX_OK, "Error", latest_error_msg);
	return firsttime;
}
int				valid(const void *p)
{
	size_t val=(size_t)p;

	if(sizeof(size_t)==4)
	{
		switch(val)
		{
		case 0:
		case 0xCCCCCCCC:
		case 0xFEEEFEEE:
		case 0xEEFEEEFE:
		case 0xCDCDCDCD:
		case 0xFDFDFDFD:
		case 0xBAADF00D:
		case 0xBAAD0000:
			return 0;
		}
	}
	else
	{
		if(val==0xCCCCCCCCCCCCCCCC)
			return 0;
		if(val==0xFEEEFEEEFEEEFEEE)
			return 0;
		if(val==0xEEFEEEFEEEFEEEFE)
			return 0;
		if(val==0xCDCDCDCDCDCDCDCD)
			return 0;
		if(val==0xBAADF00DBAADF00D)
			return 0;
		if(val==0xADF00DBAADF00DBA)
			return 0;
	}
	return 1;
}
void			pause()
{
	int k;

	printf("Enter 0 to continue: ");
	scanf("%d", &k);
}
int				pause_abort(const char *file, int lineno, const char *extraInfo)
{
	printf("INTERNAL ERROR %s(%d)\nABORTING\n", file, lineno);
	if(extraInfo)
		printf("%s\n\n", extraInfo);
	pause();
	abort();
	return 0;
}

#if !defined __linux__
#if defined _MSC_VER && _MSC_VER<1800
#define	S_IFMT		00170000//octal
#define	S_IFREG		 0100000
#endif
#ifndef S_ISREG
#define	S_ISREG(m)	(((m)&S_IFMT)==S_IFREG)
#endif
#endif
int				file_is_readable(const char *filename)//0: not readable, 1: regular file, 2: folder
{
	struct stat info={0};

	int error=stat(filename, &info);
	if(!error)
		return 1+!S_ISREG(info.st_mode);
	return 0;
}
ArrayHandle		load_text(const char *filename, int pad)
{
	struct stat info={0};
	FILE *f;
	ArrayHandle str;

	int error=stat(filename, &info);
	if(error)
	{
		LOG_ERROR("Cannot open %s\n%s", filename, strerror(errno));
		return 0;
	}
	f=fopen(filename, "r");
	//f=fopen(filename, "rb");
	//f=fopen(filename, "r, ccs=UTF-8");//gets converted to UTF-16 on Windows

	str=array_construct(0, 1, info.st_size, 1, pad+1, 0);
	str->count=fread(str->data, 1, info.st_size, f);
	fclose(f);
	memset(str->data+str->count, 0, str->cap-str->count);
	return str;
}
int				save_text(const char *filename, const char *text, size_t len)
{
	FILE *f=fopen(filename, "w");
	if(!f)
		return 0;
	fwrite(text, 1, len, f);
	fclose(f);
	return 1;
}

//C array
#if 1
static void		array_realloc(ArrayHandle *arr, size_t count, size_t pad)//CANNOT be nullptr, array must be initialized with array_alloc()
{
	ArrayHandle p2;
	size_t size, newcap;

	ASSERT_P(*arr);
	size=(count+pad)*arr[0]->esize, newcap=arr[0]->esize;
	for(;newcap<size;newcap<<=1);
	if(newcap>arr[0]->cap)
	{
		p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+newcap);
		ASSERT_P(p2);
		*arr=p2;
		if(arr[0]->cap<newcap)
			memset(arr[0]->data+arr[0]->cap, 0, newcap-arr[0]->cap);
		arr[0]->cap=newcap;
	}
	arr[0]->count=count;
}

//Array API
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*))
{
	ArrayHandle arr;
	size_t srcsize, dstsize, cap;
	
	srcsize=count*esize;
	dstsize=rep*srcsize;
	for(cap=esize+pad*esize;cap<dstsize;cap<<=1);
	arr=(ArrayHandle)malloc(sizeof(ArrayHeader)+cap);
	ASSERT_P(arr);
	arr->count=count;
	arr->esize=esize;
	arr->cap=cap;
	arr->destructor=destructor;
	if(src)
	{
		ASSERT_P(src);
		memfill(arr->data, src, dstsize, srcsize);
	}
	else
		memset(arr->data, 0, dstsize);
		
	if(cap-dstsize>0)//zero pad
		memset(arr->data+dstsize, 0, cap-dstsize);
	return arr;
}
ArrayHandle		array_copy(ArrayHandle *arr)
{
	ArrayHandle a2;
	size_t bytesize;

	if(!*arr)
		return 0;
	bytesize=sizeof(ArrayHeader)+arr[0]->cap;
	a2=(ArrayHandle)malloc(bytesize);
	ASSERT_P(a2);
	memcpy(a2, *arr, bytesize);
	return a2;
}
void			array_clear(ArrayHandle *arr)//can be nullptr
{
	if(*arr)
	{
		if(arr[0]->destructor)
		{
			for(size_t k=0;k<arr[0]->count;++k)
				arr[0]->destructor(array_at(arr, k));
		}
		arr[0]->count=0;
	}
}
void			array_free(ArrayHandle *arr)//can be nullptr
{
	if(*arr&&arr[0]->destructor)
	{
		for(size_t k=0;k<arr[0]->count;++k)
			arr[0]->destructor(array_at(arr, k));
	}
	free(*arr);
	*arr=0;
}
void			array_fit(ArrayHandle *arr, size_t pad)//can be nullptr
{
	ArrayHandle p2;
	if(!*arr)
		return;
	arr[0]->cap=(arr[0]->count+pad)*arr[0]->esize;
	p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+arr[0]->cap);
	ASSERT_P(p2);
	*arr=p2;
}

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad)//cannot be nullptr
{
	size_t start, srcsize, dstsize, movesize;
	
	ASSERT_P(*arr);
	start=idx*arr[0]->esize;
	srcsize=count*arr[0]->esize;
	dstsize=rep*srcsize;
	movesize=arr[0]->count*arr[0]->esize-start;
	array_realloc(arr, arr[0]->count+rep*count, pad);
	memmove(arr[0]->data+start+dstsize, arr[0]->data+start, movesize);
	if(data)
		memfill(arr[0]->data+start, data, dstsize, srcsize);
	else
		memset(arr[0]->data+start, 0, dstsize);
	return arr[0]->data+start;
}
void*			array_erase(ArrayHandle *arr, size_t idx, size_t count)
{
	size_t k;

	ASSERT_P(*arr);
	if(arr[0]->count<idx+count)
	{
		LOG_ERROR("array_erase() out of bounds: idx=%lld count=%lld size=%lld", (long long)idx, (long long)count, (long long)arr[0]->count);
		if(arr[0]->count<idx)
			return 0;
		count=arr[0]->count-idx;//erase till end of array if just idx+count is OOB
	}
	if(arr[0]->destructor)
	{
		for(k=0;k<count;++k)
			arr[0]->destructor(array_at(arr, idx+k));
	}
	memmove(arr[0]->data+idx*arr[0]->esize, arr[0]->data+(idx+count)*arr[0]->esize, (arr[0]->count-(idx+count))*arr[0]->esize);
	arr[0]->count-=count;
	return arr[0]->data+idx*arr[0]->esize;
}
void*			array_replace(ArrayHandle *arr, size_t idx, size_t rem_count, const void *data, size_t ins_count, size_t rep, size_t pad)
{
	size_t k, c0, c1, start, srcsize, dstsize;

	ASSERT_P(*arr);
	if(arr[0]->count<idx+rem_count)
	{
		LOG_ERROR("array_replace() out of bounds: idx=%lld rem_count=%lld size=%lld ins_count=%lld", (long long)idx, (long long)rem_count, (long long)arr[0]->count, (long long)ins_count);
		if(arr[0]->count<idx)
			return 0;
		rem_count=arr[0]->count-idx;//erase till end of array if just idx+count is OOB
	}
	if(arr[0]->destructor)
	{
		for(k=0;k<rem_count;++k)//destroy removed objects
			arr[0]->destructor(array_at(arr, idx+k));
	}
	start=idx*arr[0]->esize;
	srcsize=ins_count*arr[0]->esize;
	dstsize=rep*srcsize;
	c0=arr[0]->count;//copy original count
	c1=arr[0]->count+rep*ins_count-rem_count;//calculate new count

	if(ins_count!=rem_count||(c1+pad)*arr[0]->esize>arr[0]->cap)//resize array
		array_realloc(arr, c1, pad);

	if(ins_count!=rem_count)//shift objects
		memmove(arr[0]->data+(idx+ins_count)*arr[0]->esize, arr[0]->data+(idx+rem_count)*arr[0]->esize, (c0-(idx+rem_count))*arr[0]->esize);

	if(dstsize)//initialize inserted range
	{
		if(data)
			memfill(arr[0]->data+start, data, dstsize, srcsize);
		else
			memset(arr[0]->data+start, 0, dstsize);
	}
	return arr[0]->data+start;//return start of inserted range
}

size_t			array_size(ArrayHandle const *arr)//can be nullptr
{
	if(!arr[0])
		return 0;
	return arr[0]->count;
}
void*			array_at(ArrayHandle *arr, size_t idx)
{
	if(!arr[0])
		return 0;
	if(idx>=arr[0]->count)
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
//const void*		array_at_const(ArrayConstHandle *arr, int idx)
//{
//	if(!arr[0])
//		return 0;
//	return arr[0]->data+idx*arr[0]->esize;
//}
void*			array_back(ArrayHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
//const void*		array_back_const(ArrayConstHandle const *arr)
//{
//	if(!*arr||!arr[0]->count)
//		return 0;
//	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
//}
#endif

//double-linked array list
#if 1
void			dlist_init(DListHandle list, size_t objsize, size_t objpernode, void (*destructor)(void*))
{
	list->i=list->f=0;
	list->objsize=objsize;
	list->objpernode=objpernode;
	list->nnodes=list->nobj=0;//empty
	list->destructor=destructor;
}
#define			DLIST_COPY_NODE(DST, PREV, NEXT, SRC, PAYLOADSIZE)\
	DST=(DNodeHandle)malloc(sizeof(DNodeHeader)+(PAYLOADSIZE)),\
	DST->prev=PREV,\
	DST->next=NEXT,\
	memcpy(DST->data, SRC->data, PAYLOADSIZE)
void			dlist_copy(DListHandle dst, DListHandle src)
{
	DNodeHandle it;
	size_t payloadsize;

	dlist_init(dst, src->objsize, src->objpernode, src->destructor);
	it=dst->i;
	if(it)
	{
		payloadsize=src->objpernode*src->objsize;

		DLIST_COPY_NODE(dst->f, 0, 0, it, payloadsize);
		//dst->f=(DNodeHandle)malloc(sizeof(DNodeHeader)+payloadsize);
		//memcpy(dst->f->data, it->data, payloadsize);

		dst->i=dst->f;

		it=it->next;

		for(;it;it=it->next)
		{
			DLIST_COPY_NODE(dst->f->next, dst->f, 0, it, payloadsize);
			dst->f=dst->f->next;
		}
	}
	dst->nnodes=src->nnodes;
	dst->nobj=src->nobj;
}
void			dlist_clear(DListHandle list)
{
	DNodeHandle it;

	it=list->i;
	if(it)
	{
		while(it->next)
		{
			if(list->destructor)
			{
				for(size_t k=0;k<list->objpernode;++k)
					list->destructor(it->data+k*list->objsize);
				list->nobj-=list->objpernode;
			}
			it=it->next;
			free(it->prev);
		}
		if(list->destructor)
		{
			for(size_t k=0;k<list->nobj;++k)
				list->destructor(it->data+k*list->objsize);
		}
		free(it);
		list->i=list->f=0;
		list->nobj=list->nnodes=0;
	}
}
void			dlist_appendtoarray(DListHandle list, ArrayHandle *dst)
{
	DNodeHandle it;
	size_t payloadsize;

	if(!*dst)
		*dst=array_construct(0, list->objsize, 0, 0, list->nnodes*list->objpernode, list->destructor);
	else
	{
		if(dst[0]->esize!=list->objsize)
		{
			LOG_ERROR("dlist_appendtoarray(): dst->esize=%d, list->objsize=%d", dst[0]->esize, list->objsize);
			return;
		}
		ARRAY_APPEND(*dst, 0, 0, 0, list->nnodes*list->objpernode);
	}
	it=list->i;
	payloadsize=list->objpernode*list->objsize;
	for(size_t offset=dst[0]->count;it;)
	{
		memcpy(dst[0]->data+offset*list->objsize, it->data, payloadsize);
		offset+=list->objpernode;
		it=it->next;
	}
	dst[0]->count+=list->nobj;
}

void*			dlist_push_back(DListHandle list, const void *obj)
{
	size_t obj_idx=list->nobj%list->objpernode;
	if(!obj_idx)//need a new node
	{
		DNodeHandle temp=(DNodeHandle)malloc(sizeof(DNodeHeader)+list->objpernode*list->objsize);
		if(list->nnodes)
		{
			list->f->next=temp;
			temp->prev=list->f;
			temp->next=0;
			list->f=temp;
		}
		else
		{
			temp->prev=temp->next=0;
			list->i=list->f=temp;
		}
		++list->nnodes;
	}
	void *p=list->f->data+obj_idx*list->objsize;
	if(obj)
		memcpy(p, obj, list->objsize);
	else
		memset(p, 0, list->objsize);
	++list->nobj;
	return p;
}
void*			dlist_back(DListHandle list)
{
	size_t obj_idx;

	if(!list->nobj)
	{
		LOG_ERROR("dlist_back() called on empty list");
		return 0;
	}
	obj_idx=(list->nobj-1)%list->objpernode;
	return list->f->data+obj_idx*list->objsize;
}
void			dlist_pop_back(DListHandle list)
{
	size_t obj_idx;

	if(!list->nobj)
		LOG_ERROR("dlist_pop_back() called on empty list");
	if(list->destructor)
		list->destructor(dlist_back(list));
	obj_idx=(list->nobj-1)%list->objpernode;
	if(!obj_idx)//last object is first in the last block
	{
		DNodeHandle last=list->f;
		list->f=list->f->prev;
		free(last);
		--list->nnodes;
		if(!list->nnodes)//last object was popped out
			list->i=0;
	}
	--list->nobj;
}

void			dlist_first(DListHandle list, DListItHandle it)
{
	it->list=list;
	it->node=list->i;
	it->obj_idx=0;
}
void			dlist_last(DListHandle list, DListItHandle it)
{
	it->list=list;
	it->node=list->f;
	it->obj_idx=(list->nobj-1)%list->objpernode;
}
void*			dlist_it_deref(DListItHandle it)
{
	if(it->obj_idx>=it->list->nobj)
		LOG_ERROR("dlist_it_deref() out of bounds");
	if(!it->node)
		LOG_ERROR("dlist_it_deref() node == nullptr");
	return it->node->data+it->obj_idx%it->list->objpernode*it->list->objsize;
}
int				dlist_it_inc(DListItHandle it)
{
	++it->obj_idx;
	if(it->obj_idx>=it->list->objpernode)
	{
		it->obj_idx=0;
		if(!it->node||!it->node->next)
			return 0;
			//LOG_ERROR("dlist_it_inc() attempting to read node == nullptr");
		it->node=it->node->next;
	}
	return 1;
}
int				dlist_it_dec(DListItHandle it)
{
	if(it->obj_idx)
		--it->obj_idx;
	else
	{
		if(!it->node||!it->node->prev)
			return 0;
			//LOG_ERROR("dlist_it_dec() attempting to read node == nullptr");
		it->node=it->node->prev;
		it->obj_idx=it->list->objpernode-1;
	}
	return 1;
}
#endif

//BST-map
#if 0
void			map_init(MapHandle map, size_t key_size, size_t val_size, CmpFn cmp_key, void (*destructor)(void*))
{
	map->key_size=key_size;
	map->val_size=val_size;
	map->nnodes=0;
	map->root=0;
	map->cmp_key=cmp_key;
	map->destructor=destructor;
}
void			map_clear_r(MapHandle map, BSTNodeHandle node)
{
	if(node)
	{
		map_clear_r(map, node->left);
		map_clear_r(map, node->right);
		if(map->destructor)
			map->destructor(node->data);
		free(node);
	}
}
BSTNodeHandle*	map_find_r(BSTNodeHandle *node, const void *key, CmpFn cmp_key)
{
	CmpRes result;

	if(!*node)//not found
		return 0;
	result=cmp_key(key, node[0]->data);
	switch(result)
	{
	case RESULT_LESS:
		return map_find_r(&node[0]->left, key, cmp_key);
	case RESULT_GREATER:
		return map_find_r(&node[0]->right, key, cmp_key);
	case RESULT_EQUAL://found
		return node;
	}
	return 0;//unreachable, if comparator is valid
}
BSTNodeHandle*	map_insert_r(BSTNodeHandle *node, const void *key, MapHandle map, const void *val, int *found)
{
	if(!*node)
	{
		*node=(BSTNodeHandle)malloc(sizeof(BSTNodeHeader)+map->key_size+map->val_size);
		node[0]->left=node[0]->right=0;
		memcpy(node[0]->data, key, map->key_size);
		if(val)
			memcpy(node[0]->data+map->key_size, val, map->val_size);
		else
			memset(node[0]->data+map->key_size, 0, map->val_size);
		++map->nnodes;
		if(found)
			*found=0;
		return node;
	}
	CmpRes result=map->cmp_key(key, node[0]->data);
	switch(result)
	{
	case RESULT_LESS:
		return map_insert_r(&node[0]->left, key, map, val, found);
	case RESULT_GREATER:
		return map_insert_r(&node[0]->right, key, map, val, found);
	case RESULT_EQUAL:
		if(found)
			*found=1;
		return node;
	}
	if(found)
		*found=0;
	return 0;
}
/*BSTNodeHandle*	map_insert(MapHandle map, const void *key, const void *val, int *found)
{
	int _found;
	BSTNodeHandle *pn=map_insert_r(&map->root, key, map, val, &_found);
	map->nnodes+=!_found;
	if(found)
		*found=_found;
	return pn;
}//*/
BSTNodeHandle*	map_erase_r(MapHandle map, BSTNodeHandle *node, const void *key, int call_destructor)//https://www.geeksforgeeks.org/binary-search-tree-set-2-delete/
{
	CmpRes result;
	BSTNodeHandle temp;

	if(!*node)
		return node;
	result=map->cmp_key(key, node[0]->data);
	switch(result)
	{
	case RESULT_LESS:
		return map_erase_r(map, &node[0]->left, key, call_destructor);
	case RESULT_GREATER:
		return map_erase_r(map, &node[0]->right, key, call_destructor);
	case RESULT_EQUAL:
		if(!node[0]->left)
		{
			temp=node[0]->right;
			if(map->destructor&&call_destructor)
				map->destructor(node[0]->data);
			free(*node);
			*node=temp;
			--map->nnodes;
			return node;
		}
		if(!node[0]->right)
		{
			temp=node[0]->left;
			if(map->destructor&&call_destructor)
				map->destructor(node[0]->data);
			free(*node);
			*node=temp;
			--map->nnodes;
			return node;
		}

		//both children exist
		temp=node[0]->right;
		while(temp->left)//find leftmost child on right
			temp=temp->left;
		if(map->destructor&&call_destructor)//destroy node
			map->destructor(node[0]->data);
		memcpy(node[0]->data, temp->data, map->key_size+map->val_size);//overwrite erased node contents
		map_erase_r(map, &node[0]->right, temp->data, 0);//erase relocated node from right subtree
		return node;
	}
	return 0;
}

//map rebalance function
static void		map_reb_tree2vine(BSTNodeHandle root)
{
	BSTNodeHandle tail, rest, temp;

	tail=root;
	rest=tail->right;
	while(rest)
	{
		if(rest->left)
		{
			temp=rest->left;
			rest->left=temp->right;
			temp->right=rest;
			rest=temp;
			tail->right=temp;
		}
		else
		{
			tail=rest;
			rest=rest->right;
		}
	}
}
static void		map_reb_compress(BSTNodeHandle root, size_t count)
{
	BSTNodeHandle scanner=root, child;
	for(size_t i=0;i<count;++i)
	{
		child=scanner->right;
		scanner->right=child->right;
		scanner=scanner->right;
		child->right=scanner->left;
		scanner->left=child;
	}
}
static void		map_reb_vine2tree(BSTNodeHandle root, size_t size)
{
	size_t nleaves=size+1-(1<<floor_log2(size+1));
	map_reb_compress(root, nleaves);
	size-=nleaves;
	while(size>1)
	{
		size>>=1;
		map_reb_compress(root, size);
	}
}
void			map_rebalance(MapHandle map)
{
	//https://en.wikipedia.org/wiki/Day–Stout–Warren_algorithm
	BSTNodeHandle pseud;

	pseud=(BSTNodeHandle)malloc(sizeof(BSTNodeHeader)+map->key_size+map->val_size);
	pseud->left=0;
	pseud->right=map->root;
	memset(pseud->data, 0, map->key_size+map->val_size);
	map_reb_tree2vine(pseud);
	map_reb_vine2tree(pseud, map->nnodes);
	map->root=pseud->right;
	free(pseud);
}

void			map_debugprint_r(BSTNodeHandle *node, int depth, void (*callback)(BSTNodeHandle *node, int depth))
{
	if(*node)
	{
		map_debugprint_r(&node[0]->left, depth+1, callback);
		callback(node, depth);
		map_debugprint_r(&node[0]->right, depth+1, callback);
	}
}
#endif

//red-black tree map
#if 1
void			map_init(MapHandle map, size_t esize, MapCmpFn comparator, void (*destructor)(void*))
{
	map->esize=esize;
	map->nnodes=0;
	map->root=0;
	map->comparator=comparator;
	map->destructor=destructor;
}
void			map_clear_r(MapHandle map, RBNodeHandle node)
{
	if(node)
	{
		map_clear_r(map, node->left);
		map_clear_r(map, node->right);
		if(map->destructor)
			map->destructor(node->data);
		free(node);
	}
}
static RBNodeHandle* get_node_addr(MapHandle map, RBNodeHandle node)
{
	if(!node->parent)
		return &map->root;
	if(node==node->parent->left)
		return &node->parent->left;
	if(node==node->parent->right)
		return &node->parent->right;
	return 0;//inconsistent, should be unreachable
}
RBNodeHandle*	map_find(MapHandle map, const void *key)
{
	RBNodeHandle node;
	CmpRes result;

	for(node=map->root;node;)
	{
		result=map->comparator(key, node->data);
		switch(result)
		{
		case RESULT_LESS:
			node=node->left;
			break;
		case RESULT_GREATER:
			node=node->right;
			break;
		case RESULT_EQUAL://found: return node address
			return get_node_addr(map, node);
		}
	}
	return 0;//not found
}
static void		rb_rotateleft(MapHandle map, RBNodeHandle x)
{
	RBNodeHandle y;

	y=x->right;
	x->right=y->left;
	if(y->left)
		y->left->parent=x;
	y->parent=x->parent;
	if(map->root==x)
		map->root=y;
	else if(x==x->parent->left)
		x->parent->left=y;
	else
		x->parent->right=y;
	y->left=x;
	x->parent=y;
}
static void		rb_rotateright(MapHandle map, RBNodeHandle x)
{
	RBNodeHandle y;

	y=x->left;
	x->left=y->right;
	if(y->right)
		y->right->parent=x;
	y->parent=x->parent;
	if(map->root==x)
		map->root=y;
	else if(x==x->parent->right)
		x->parent->right=y;
	else
		x->parent->left=y;
	y->right=x;
	x->parent=y;
}
RBNodeHandle*	map_insert(MapHandle map, const void *key, int *found)
{
	RBNodeHandle x, y, z;
	CmpRes result;

	//rb-insert - Cormen page 315
	x=map->root;
	y=0;
	while(x)
	{
		y=x;
		result=map->comparator(key, x->data);
		switch(result)
		{
		case RESULT_LESS:
			x=x->left;
			break;
		case RESULT_GREATER:
			x=x->right;
			break;
		case RESULT_EQUAL:
			if(found)
				*found=1;
			return get_node_addr(map, x);
		default:
			return 0;
		}
	}
	if(found)
		*found=0;

	z=(RBNodeHandle)malloc(sizeof(RBNodeHeader)+map->esize);
	z->parent=y;
	z->left=z->right=0;
	z->is_red=1;
	memset(z->data, 0, map->esize);
	//memcpy(z->data, key, map->esize);//X  what if sizeof(key) != sizeof(data)
	x=z;

	if(!y)
		map->root=z;
	else
	{
		result=map->comparator(key, y->data);
		switch(result)
		{
		case RESULT_LESS:
			y->left=z;
			break;
		case RESULT_GREATER:
			y->right=z;
			break;
		default:
			free(z);
			return 0;
		}
	}

	//rb-fixup - Cormen page 316
	//https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/src/c%2B%2B98/tree.cc		line 195
	while(x!=map->root&&x->parent->is_red)//all nullptr's are black
	{
		RBNodeHandle xpp;

		xpp=x->parent->parent;
		if(xpp)
		{
			if(x->parent==xpp->left)
			{
				y=xpp->right;//uncle
				if(y&&y->is_red)//case 1
				{
					x->parent->is_red=0;
					y->is_red=0;
					xpp->is_red=1;
					x=xpp;
				}
				else//cases 2 & 3
				{
					if(x==x->parent->right)//case 2
					{
						x=x->parent;
						rb_rotateleft(map, x);
					}
					x->parent->is_red=0;
					xpp->is_red=1;
					rb_rotateright(map, xpp);
				}
			}
			else
			{
				y=xpp->left;//uncle
				if(y&&y->is_red)//case 1
				{
					x->parent->is_red=0;
					y->is_red=0;
					xpp->is_red=1;
					x=xpp;
				}
				else//cases 2 & 3
				{
					if(x==x->parent->left)//case 2
					{
						x=x->parent;
						rb_rotateright(map, x);
					}
					x->parent->is_red=0;
					xpp->is_red=1;
					rb_rotateleft(map, xpp);
				}
			}
		}
	}
	map->root->is_red=0;

	++map->nnodes;
	return get_node_addr(map, z);
}
static void		rb_transplant(MapHandle map, RBNodeHandle u, RBNodeHandle v)
{
	if(!u->parent)
		map->root=v;
	else if(u==u->parent->left)
		u->parent->left=v;
	else
		u->parent->right=v;
	v->parent=u->parent;
}
static RBNodeHandle tree_minimum(RBNodeHandle root)
{
	if(!root)
		return 0;
	while(root->left)
		root=root->left;
	return root;
}
static RBNodeHandle tree_maximum(RBNodeHandle root)
{
	if(!root)
		return 0;
	while(root->right)
		root=root->right;
	return root;
}
static void		rb_deletefixup(MapHandle map, RBNodeHandle x)
{
	RBNodeHandle w;

	while(x!=map->root&&!x->is_red)
	{
		if(x==x->parent->left)
		{
			w=x->parent->right;
			if(w->is_red)
			{
				w->is_red=0;
				x->parent->is_red=1;
				rb_rotateleft(map, x->parent);
				w=x->parent->right;
			}
			if(!w->left->is_red&&!w->right->is_red)
			{
				w->is_red=1;
				x=x->parent;
			}
			else
			{
				if(!w->right->is_red)
				{
					w->left->is_red=0;
					w->is_red=1;
					rb_rotateright(map, w);
					w=x->parent->right;
				}
				w->is_red=x->parent->is_red;
				x->parent->is_red=0;
				w->right->is_red=0;
				rb_rotateleft(map, x->parent);
				x=map->root;
			}
		}
		else
		{
			w=x->parent->left;
			if(w->is_red)
			{
				w->is_red=0;
				x->parent->is_red=1;
				rb_rotateright(map, x->parent);
				w=x->parent->left;
			}
			if(!w->right->is_red&&!w->left->is_red)
			{
				w->is_red=1;
				x=x->parent;
			}
			else
			{
				if(!w->left->is_red)
				{
					w->right->is_red=0;
					w->is_red=1;
					rb_rotateleft(map, w);
					w=x->parent->left;
				}
				w->is_red=x->parent->is_red;
				x->parent->is_red=0;
				w->left->is_red=0;
				rb_rotateright(map, x->parent);
				x=map->root;
			}
		}
	}
}
int				map_erase(MapHandle map, const void *data, RBNodeHandle node)
{
	//https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/src/c%2B%2B98/tree.cc		line 286
#if 1
	RBNodeHandle *root, *leftmost, *rightmost, x, xp, y, z, *r2;
	size_t y_is_red;

	if(node)
		z=node;
	else if(data)
	{
		r2=map_find(map, data);
		if(!r2)
			return 0;
		z=*r2;
	}
	else
	{
		LOG_ERROR("map_erase() usage error: nullptr args");
		return 0;
	}
	
	root=&map->root->parent;
	leftmost=&map->root->left;
	rightmost=&map->root->right;
	y=z;
	x=0;
	xp=0;
	if(!y->left)		//z has at most one non-null child. y == z.
		x=y->right;		//x might be null
	else if(!y->right)	//z has exactly one non-null child. y == z.
		x=y->left;		//x is not null
	else
	{
		y=y->right;//z has two non-null children. Set y to z's successor.  x might be null
		while(y->left)
			y=y->left;
		x=y->right;
	}
	if(y!=z)
	{
		//relink y in place of z.  y is z's successor
		z->left->parent=y;
		y->left=z->left;
		if(y!=z->right)
		{
			xp=y->parent;
			if(x)
				x->parent=y->parent;
			y->parent->left=x;//y must be a child of left
			y->right=z->right;
			z->right->parent=y;
		}
		else
			xp=y;
		if(map->root==z)
			map->root=y;
		else if(z->parent->left==z)
			z->parent->left=y;
		else
			z->parent->right=y;
		y->parent=z->parent;
		y_is_red=y->is_red, y->is_red=z->is_red, z->is_red=y_is_red;
		y=z;
		//y now points to node to be actually deleted
	}
	else//y==z
	{
		xp=y->parent;

		if(x)
			x->parent=y->parent;

		if(map->root==z)
			map->root=x;
		else if(z->parent->left==z)
			z->parent->left=x;
		else
			z->parent->right=x;

		if(*leftmost==z)
		{
			if(!z->right)//z->left must be null also
				leftmost=&z->parent;//makes __leftmost == _M_header if __z == __root
			else
				leftmost=get_node_addr(map, tree_minimum(x));
		}
		if(*rightmost==z)
		{
			if(!z->left)//z->right must be null also
				rightmost=&z->parent;//makes __rightmost == _M_header if __z == __root
			else//x == z->left
				rightmost=get_node_addr(map, tree_maximum(x));
		}
	}
	if(!y->is_red)
	{
		RBNodeHandle w;

		while(x!=map->root&&(!x||!x->is_red))
		{
			if(x==xp->left)
			{
				w=xp->right;
				if(w->is_red)
				{
					w->is_red=0;
					xp->is_red=1;
					rb_rotateleft(map, xp);
					w=xp->right;
				}
				if((!w->left||!w->left->is_red)&&(!w->right||!w->right->is_red))
				{
					w->is_red=1;
					x=xp;
					xp=xp->parent;
				}
				else
				{
					if(!w->right||!w->right->is_red)
					{
						w->left->is_red=0;
						w->is_red=1;
						rb_rotateright(map, w);
						w=xp->right;
					}
					w->is_red=xp->is_red;
					xp->is_red=0;
					if(w->right)
						w->right->is_red=0;
					rb_rotateleft(map, xp);
					break;
				}
			}
			else//same as above with right <-> left
			{
				w=xp->left;
				if(w->is_red)
				{
					w->is_red=0;
					xp->is_red=1;
					rb_rotateright(map, xp);
					w=xp->left;
				}
				if((!w->right||!w->right->is_red)&&(!w->left||!w->left->is_red))
				{
					w->is_red=1;
					x=xp;
					xp=xp->parent;
				}
				else
				{
					if(!w->left||!w->left->is_red)
					{
						w->right->is_red=0;
						w->is_red=1;
						rb_rotateleft(map, w);
						w=xp->left;
					}
					w->is_red=xp->is_red;
					xp->is_red=0;
					if(w->left)
						w->left->is_red=0;
					rb_rotateright(map, xp);
					break;
				}
			}
		}
		if(x)
			x->is_red=0;
	}
	//return y;
	return 1;
#endif

	//Cormen page 324
#if 0
	RBNodeHandle x, y, z, *r2;
	int y_is_red;

	if(node)
		z=node;
	else if(data)
	{
		r2=map_find(map, data);
		if(!r2)
			return 0;
		z=*r2;
	}
	else
		return 0;

	y=z;
	y_is_red=y->is_red;
	if(!z->left)
	{
		x=z->right;
		rb_transplant(map, z, z->right);
	}
	else if(!z->right)
	{
		x=z->left;
		rb_transplant(map, z, z->left);
	}
	else
	{
		y=tree_minimum(z->right);
		y_is_red=y->is_red;
		x=y->right;
		if(y->parent==z)
			x->parent=y;
		else
		{
			rb_transplant(map, y, y->right);
			y->right=z->right;
			y->right->parent=y;
		}
		rb_transplant(map, z, y);
		y->left=z->left;
		y->left->parent=y;
		y->is_red=z->is_red;
	}
	if(!y_is_red)
		rb_deletefixup(map, x);
	return 1;
#endif
}
void			map_debugprint_r(RBNodeHandle *node, int depth, void (*printer)(RBNodeHandle *node, int depth))
{
	if(*node)
	{
		map_debugprint_r(&node[0]->left, depth+1, printer);
		printer(node, depth);
		map_debugprint_r(&node[0]->right, depth+1, printer);
	}
}
#endif