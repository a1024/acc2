#include		"acc.h"
#include		<stdio.h>
#include		<stdlib.h>
#include		<stdarg.h>
#include		<string.h>
#include		<sys/stat.h>
#include		<math.h>
#include		<errno.h>
#ifdef _MSC_VER
#include		<Windows.h>//QueryPerformance...
#else
#include		<time.h>//clock_gettime
#define			sprintf_s	snprintf
#define			vsprintf_s	vsnprintf
#define			_HUGE		HUGE_VAL
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
void			memswap(void *p1, void *p2, size_t size)
{
	unsigned char *s1=(unsigned char*)p1, *s2=(unsigned char*)p2, *end=s1+size;
	for(;s1<end;++s1, ++s2)
	{
		const unsigned char t=*s1;
		*s1=*s2;
		*s2=t;
	}
}
void			memreverse(void *p, size_t count, size_t size)
{
	size_t totalsize=count*size;
	unsigned char *s1=(unsigned char*)p, *s2=s1+totalsize-size;
	while(s1<s2)
	{
		memswap(s1, s2, size);
		s1+=size, s2-=size;
	}
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
int				minimum(int a, int b)
{
	return a<b?a:b;
}
int				maximum(int a, int b)
{
	return a>b?a:b;
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
char				first_error_msg[G_BUF_SIZE]={0}, latest_error_msg[G_BUF_SIZE]={0};
int					log_error(const char *file, int line, const char *format, ...)
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
	switch((size_t)p)
	{
	case 0:
	case 0xCCCCCCCC:
	case 0xFEEEFEEE:
	case 0xEEFEEEFE:
	case 0xCDCDCDCD:
	case 0xFDFDFDFD:
	case 0xBAAD0000:
		return 0;
	}
	return 1;
}

int				file_is_readable(const char *filename)//0: not readable, 1: regular file, 2: folder
{
	struct stat info={0};

	int error=stat(filename, &info);
	if(!error)
		return 1+!S_ISREG(info.st_mode);
	return 0;
}
char*			load_utf8(const char *filename, size_t *len)
{
	struct stat info={0};
	FILE *f;

	int error=stat(filename, &info);
	if(error)
	{
		LOG_ERROR("Cannot open %s\n%s", filename, strerror(errno));
		return 0;
	}
	f=fopen(filename, "r, ccs=UTF-8");
	char *str=(char*)malloc(info.st_size+1);
	size_t readbytes=fread(str, 1, info.st_size, f);
	fclose(f);
	str[readbytes]=0;
	if(len)
		*len=readbytes;
	return str;
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
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, DebugInfo debug_info)
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
	arr->debug_info=debug_info;
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
ArrayHandle		array_copy(ArrayHandle *arr, DebugInfo debug_info)
{
	ArrayHandle a2;
	size_t bytesize;

	if(!*arr)
		return 0;
	bytesize=sizeof(ArrayHeader)+arr[0]->cap;
	a2=(ArrayHandle)malloc(bytesize);
	ASSERT_P(a2);
	memcpy(a2, *arr, bytesize);
	a2->debug_info=debug_info;
	return a2;
}
void			array_free(ArrayHandle *arr, void (*destructor)(void*))//can be nullptr
{
	if(*arr&&destructor)
	{
		for(int k=0;k<arr[0]->count;++k)
			destructor(array_at(arr, k));
	}
	free(*arr);
	*arr=0;
}
void			array_clear(ArrayHandle *arr, void (*destructor)(void*))//can be nullptr
{
	if(*arr)
	{
		if(destructor)
		{
			for(int k=0;k<arr[0]->count;++k)
				destructor(array_at(arr, k));
		}
		arr[0]->count=0;
	}
}
void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad)
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
const void*		array_at_const(ArrayConstHandle *arr, int idx)
{
	if(!arr[0])
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
void*			array_back(ArrayHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
const void*		array_back_const(ArrayHandle const *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
#endif

//double-linked array list
#if 1
void			dlist_init(DListHandle list, size_t objsize, size_t objpernode)
{
	list->i=list->f=0;
	list->objsize=objsize;
	list->objpernode=objpernode;
	list->nnodes=list->nobj=0;//empty
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

	dlist_init(dst, src->objsize, src->objpernode);
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
void			dlist_clear(DListHandle list, void (*destructor)(void*))
{
	DNodeHandle it;

	it=list->i;
	if(it)
	{
		while(it->next)
		{
			if(destructor)
			{
				for(int k=0;k<list->objpernode;++k)
					destructor(it->data+k*list->objsize);
				list->nobj-=list->objpernode;
			}
			it=it->next;
			free(it->prev);
		}
		if(destructor)
		{
			for(int k=0;k<list->nobj;++k)
				destructor(it->data+k*list->objsize);
		}
		free(it);
		list->i=list->f=0;
		list->nobj=list->nnodes=0;
	}
}
ArrayHandle		dlist_toarray(DListHandle list)
{
	ArrayHandle arr;
	DNodeHandle it;
	size_t payloadsize;

	arr=array_construct(0, list->objsize, 0, 0, list->nnodes*list->objpernode, __LINE__);
	it=list->i;
	payloadsize=list->objpernode*list->objsize;
	for(size_t offset=0;it;)
	{
		memcpy(arr->data+offset*list->objsize, it->data, payloadsize);
		offset+=list->objpernode;
		it=it->next;
	}
	arr->count=list->nobj;
	return arr;
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
			list->f->next->prev=list->f;
			list->f=list->f->next;
		}
		else
			list->i=list->f=temp;
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
	if(!list->nobj)
		LOG_ERROR("dlist_back() called on empty list");
	size_t obj_idx=(list->nobj-1)%list->objpernode;
	return list->f->data+obj_idx*list->objsize;
}
void			dlist_pop_back(DListHandle list, void (*destructor)(void*))
{
	if(!list->nobj)
		LOG_ERROR("dlist_pop_back() called on empty list");
	if(destructor)
		destructor(dlist_back(list));
	size_t obj_idx=(list->nobj-1)%list->objpernode;
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
	it->obj_idx=list->nobj-1;
}
void*			dlist_it_deref(DListItHandle it)
{
	if(it->obj_idx>=it->list->nobj)
		LOG_ERROR("dlist_it_deref() out of bounds");
	if(!it->node)
		LOG_ERROR("dlist_it_deref() node == nullptr");
	return it->node->data+it->obj_idx%it->list->objpernode*it->list->objsize;
}
void			dlist_it_inc(DListItHandle it)
{
	++it->obj_idx;
	if(it->obj_idx>=it->list->objpernode)
	{
		it->obj_idx=0;
		if(!it->node)
			LOG_ERROR("dlist_it_inc() attempting to read node == nullptr");
		it->node=it->node->next;
	}
}
void			dlist_it_dec(DListItHandle it)
{
	if(it->obj_idx)
		--it->obj_idx;
	else
	{
		if(!it->node)
			LOG_ERROR("dlist_it_dec() attempting to read node == nullptr");
		it->node=it->node->prev;
		it->obj_idx=it->list->objpernode-1;
	}
}
#endif

//BST-map
#if 1
void			map_init(MapHandle map, size_t key_size, size_t val_size, CmpFn cmp_key)
{
	map->key_size=key_size;
	map->val_size=val_size;
	map->nnodes=0;
	map->root=0;
	map->cmp_key=cmp_key;
}
void			map_clear_r(BSTNodeHandle node, void (*destroy_callback)(void*))
{
	if(node)
	{
		map_clear_r(node->left, destroy_callback);
		map_clear_r(node->right, destroy_callback);
		if(destroy_callback)
			destroy_callback(node->data);
		free(node);
	}
}
BSTNodeHandle*	map_find_r(BSTNodeHandle *node, const void *key, CmpFn cmp_key)
{
	if(!*node)
		return 0;
	CmpRes result=cmp_key(key, node[0]->data);
	switch(result)
	{
	case RESULT_LESS:
		return map_find_r(&node[0]->left, key, cmp_key);
	case RESULT_GREATER:
		return map_find_r(&node[0]->right, key, cmp_key);
	case RESULT_EQUAL:
		return node;
	}
	return 0;
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
BSTNodeHandle*	map_erase_r(BSTNodeHandle *node, const void *key, MapHandle map)//https://www.geeksforgeeks.org/binary-search-tree-set-2-delete/
{
	BSTNodeHandle temp;

	if(!*node)
		return node;
	CmpRes result=map->cmp_key(key, node[0]->data);
	switch(result)
	{
	case RESULT_LESS:
		return map_erase_r(&node[0]->left, key, map);
	case RESULT_GREATER:
		return map_erase_r(&node[0]->right, key, map);
	case RESULT_EQUAL:
		if(!node[0]->left)
		{
			temp=node[0]->right;
			free(*node);
			*node=temp;
			--map->nnodes;
			return node;
		}
		if(!node[0]->right)
		{
			temp=node[0]->left;
			free(*node);
			*node=temp;
			--map->nnodes;
			return node;
		}
		temp=node[0]->right;//both children exist
		while(temp->left)//find leftmost child on right
			temp=temp->left;
		memcpy(node[0]->data, temp->data, map->key_size+map->val_size);//overwrite erased node contents
		map_erase_r(&node[0]->right, temp->data, map);//erase relocated node from right subtree
		return node;
	}
	return 0;
}

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
