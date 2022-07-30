#ifndef ACC_H
#define ACC_H
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include<stddef.h>//for size_t
#ifdef __cplusplus
extern "C"
{
#endif

	#define		BENCHMARK_LEXER

//utility
#define			SIZEOF(ARR)		(sizeof(ARR)/sizeof(*(ARR)))
#ifndef _MSC_VER
#define	sprintf_s	snprintf
#endif
#define			G_BUF_SIZE	2048
extern char		g_buf[G_BUF_SIZE];

void			memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes);
void			memswap_slow(void *p1, void *p2, size_t size);
void 			memswap(void *p1, void *p2, size_t size, void *temp);
void			memreverse(void *p, size_t count, size_t esize);//calls memswap
void 			memrotate(void *p, size_t byteoffset, size_t bytesize, void *temp);//temp buffer is min(byteoffset, bytesize-byteoffset)
int 			binary_search(const void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*), const void *val, size_t *idx);//returns true if found, otherwise the idx is where val should be inserted, standard bsearch doesn't do this
void 			isort(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*));//binary insertion sort

int				floor_log2(unsigned n);
int				floor_log10(double x);
double			power(double x, int y);
double			_10pow(int n);
int				minimum(int a, int b);
int				maximum(int a, int b);
int				acme_isdigit(char c, char base);

//benchmark
double			time_ms();
#ifdef BENCHMARK_LEXER
#define			BM_DECL				double bm_t1
#define			BM_START()			bm_t1=time_ms()
#define			BM_FINISH(MSG, ...)	bm_t1=time_ms()-bm_t1, printf(MSG, ##__VA_ARGS__)
#else
#define			BM_DECL
#define			BM_START()
#define			BM_FINISH(MSG)
#endif

//error handling
int				log_error(const char *file, int line, const char *format, ...);
int				valid(const void *p);
#define			LOG_ERROR(format, ...)	log_error(file, __LINE__, format, ##__VA_ARGS__)
#define			ASSERT(SUCCESS)			((SUCCESS)!=0||log_error(file, __LINE__, #SUCCESS))
#define			ASSERT_P(POINTER)		(valid(POINTER)||log_error(file, __LINE__, #POINTER " == 0"))

//file I/O
int				file_is_readable(const char *filename);//0: not readable, 1: regular file, 2: folder
char*			load_text(const char *filename, size_t *len);//don't forget to free string


//array
#if 1
#ifdef DEBUG_INFO_STR
typedef const char *DebugInfo;
#else
typedef size_t DebugInfo;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)//no default-constructor for struct with zero-length array
#endif
typedef struct ArrayHeaderStruct
{
	size_t count, esize, cap;//cap is in bytes
	void (*destructor)(void*);
	unsigned char data[];
} ArrayHeader, *ArrayHandle;
typedef const ArrayHeader *ArrayConstHandle;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*));
ArrayHandle		array_copy(ArrayHandle *arr);//shallow
void			array_free(ArrayHandle *arr);
void			array_clear(ArrayHandle *arr);//keeps allocation
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr
void*			array_erase(ArrayHandle *arr, size_t idx, size_t count);

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayConstHandle *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayHandle const *arr);

#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD, DESTRUCTOR)	ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, DESTRUCTOR)
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)			array_insert(&(ARR), (ARR)->count, DATA, COUNT, REP, PAD)
#define			ARRAY_DATA(ARR)			(ARR)->data
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#define			ESTR_ALLOC(TYPE, STR, DATA, LEN)	STR=array_construct(DATA, sizeof(TYPE), LEN, 1, 1, 0)
#define			STR_APPEND(STR, SRC, LEN, REP)		array_insert(&(STR), (STR)->count, SRC, LEN, REP, 1)
#define			STR_FIT(STR)						array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)				*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN)				ESTR_ALLOC(char, STR, 0, LEN)
#define			STR_COPY(STR, DATA, LEN)		ESTR_ALLOC(char, STR, DATA, LEN)
#define			STR_AT(STR, IDX)				ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN)			ESTR_ALLOC(wchar_t, STR, 0, LEN)
#define			WSTR_COPY(STR, DATA, LEN)		ESTR_ALLOC(wchar_t, STR, DATA, LEN)
#define			WSTR_AT(STR, IDX)				ESTR_AT(wchar_t, STR, IDX)
#endif

//double-linked list of identical size arrays,		append-only, no mid-insertion
#if 1
typedef struct DNodeStruct
{
	struct DNodeStruct *prev, *next;
	unsigned char data[];
} DNodeHeader, *DNodeHandle;
typedef struct DListStruct
{
	DNodeHandle i, f;
	size_t
		objsize,	//size of one contained object
		objpernode,	//object count per node,		recommended value 128
		nnodes,		//node count
		nobj;		//total object count
	void (*destructor)(void*);
} DList, *DListHandle;
void			dlist_init(DListHandle list, size_t objsize, size_t objpernode, void (*destructor)(void*));
void			dlist_copy(DListHandle dst, DListHandle src);
void			dlist_clear(DListHandle list);
ArrayHandle		dlist_toarray(DListHandle list);

void*			dlist_push_back(DListHandle list, const void *obj);//shallow copy of obj	TODO dlist_push_back(array)
void*			dlist_back(DListHandle list);//returns address of last object
void			dlist_pop_back(DListHandle list);

//iterator: seamlessly iterate through contained objects
typedef struct DListIteratorStruct
{
	DListHandle list;
	DNodeHandle node;
	size_t obj_idx;
} DListIterator, *DListItHandle;
void			dlist_first(DListHandle list, DListItHandle it);
void			dlist_last(DListHandle list, DListItHandle it);
void*			dlist_it_deref(DListItHandle it);
void			dlist_it_inc(DListItHandle it);
void			dlist_it_dec(DListItHandle it);
#endif

//stack - a single-linked list		USE DLIST INSTEAD
#if 0
typedef struct SNodeStruct
{
	struct SNodeStruct *prev;
	unsigned char data[];
} SNodeHeader, *SNodeHandle;
typedef struct StackStruct
{
	SNodeHandle top;
	size_t count, esize;
} Stack;
#endif

//ordered BST-map
#if 1
typedef struct BSTNodeStruct
{
	struct BSTNodeStruct *left, *right;
	unsigned char data[];//key then value
} BSTNodeHeader, *BSTNodeHandle;
typedef enum CmpResEnum
{
	RESULT_LESS=-1,
	RESULT_EQUAL,
	RESULT_GREATER,
} CmpRes;
typedef CmpRes (*CmpFn)(const void *key, const void *pair);//the search key is always on left
typedef struct MapStruct
{
	size_t
		key_size,	//size of key in bytes
		val_size,	//size of value in bytes
		nnodes;		//stored object count
	BSTNodeHandle root;
	CmpFn cmp_key;
	void (*destructor)(void*);//key and value are packed consequtively
} Map, *MapHandle;
typedef Map const *MapConstHandle;
void			map_init(MapHandle map, size_t key_size, size_t val_size, CmpFn cmp_key, void (*destructor)(void*));
void			map_rebalance(MapHandle map);

BSTNodeHandle*	map_find_r(BSTNodeHandle *node, const void *key, CmpFn cmp_key);
BSTNodeHandle*	map_insert_r(BSTNodeHandle *node, const void *key, MapHandle map, const void *val, int *found);
BSTNodeHandle*	map_erase_r(MapHandle map, BSTNodeHandle *node, const void *key, int call_destructor);
void			map_clear_r(MapHandle map, BSTNodeHandle node);
void			map_debugprint_r(BSTNodeHandle *node, int depth, void (*callback)(BSTNodeHandle *node, int depth));

#define			MAP_INIT(MAP, KEYTYPE, PAIRTYPE, CMP, DESTRUCTOR)	map_init(MAP, sizeof(KEYTYPE), sizeof(PAIRTYPE)-sizeof(KEYTYPE), CMP, DESTRUCTOR)
#define			MAP_FIND(MAP, KEY)					map_find_r(&(MAP)->root, KEY, (MAP)->cmp_key)
#define			MAP_INSERT(MAP, KEY, VAL, PFOUND)	map_insert_r(&(MAP)->root, KEY, MAP, VAL, PFOUND)
#define			MAP_INSERT_PAIR(MAP, PAIR, PFOUND)	map_insert_r(&(MAP)->root, PAIR, MAP, (unsigned char*)(PAIR)+(MAP)->key_size, PFOUND)
#define			MAP_ERASE(MAP, KEY)					map_erase_r(MAP, &(MAP)->root, KEY, 1)
#define			MAP_CLEAR(MAP)						map_clear_r(MAP, (MAP)->root), (MAP)->root=0, (MAP)->nnodes=0
#define			MAP_DEBUGPRINT(MAP, CALLBACK)		map_debugprint_r(&(MAP)->root, 0, CALLBACK)
#endif


//acc
#define			CASE_MASK			0xDF
#define			BETWEEN(LO, X, HI)	((unsigned)((X)-LO)<(unsigned)(HI+1-LO))
typedef enum CTokenTypeEnum//should include cpp,		separate enum for asm
{
#define		TOKEN(STRING, LABEL)	LABEL,
#include	"acc_keywords.h"
#undef		TOKEN
} CTokenType;
extern const char *keywords[];
typedef enum NumberBaseEnum
{
	BASE2,
	BASE8,
	BASE10,
	BASE16,
} NumberBase;//fits in 2 bits
typedef struct TokenStruct//32 bytes
{
	CTokenType type;
	int pos, len, line, col;
	union
	{
		int flags;//lexme<<7|base<<5|synth<<4|nl_after<<3|ws_after<<2|nl_before<<1|ws_before
		struct
		{
			int ws_before:1, nl_before:1,
				ws_after:1, nl_after:1,
				synth:1,
				base:2,//NumberBase
				lexme:1;
		};
	};
	union
	{
		long long i;
		unsigned long long u;
		double f64;	//lossy
		float f32;	//lossy
		char *str;
		int *wstr;
	};
} Token;

typedef struct MacroStruct
{
	const char
		*name,			//THE KEY, should be the first attribute, belongs to strlib
		*srcfilename;	//belongs to strlib
	int nargs,//enum MacroArgCount
		is_va;
	ArrayHandle tokens;
} Macro;
int macro_define(Macro *dst, const char *srcfilename, Token const *tokens, int count);//tokens points at after '#define', undef macro on error
extern Map	strlib;//don't clear strlib until the program quits		TODO: pass as argument
char*		strlib_insert(const char *str, int len);
void		strlib_debugprint();

void		init_dateNtime();

typedef struct PreDefStruct//FIXME pre-defined macros cannot have arguments and can have 1 token max in definition
{
	const char *name;
	CTokenType type;
	union
	{
		const char *str;
		const int *wstr;
		long long i;
		unsigned long long u;
		double f64;	//lossy
		float f32;	//lossy
	};
} PreDef;
void		macros_init(MapHandle macros, PreDef *preDefs, int nPreDefs);
ArrayHandle preprocess(const char *filename, MapHandle macros, ArrayHandle includepaths, MapHandle lexlib);
void		acc_cleanup(MapHandle lexlib, MapHandle strings);

void		tokens2text(ArrayHandle tokens, ArrayHandle *str);


#ifdef __cplusplus
}
#endif
#endif//ACC_H
