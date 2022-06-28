#ifndef ACC_H
#define ACC_H
#include<stddef.h>//for size_t
#ifdef __cplusplus
extern "C"
{
#endif

//utility
#ifndef _MSC_VER
#define	sprintf_s	snprintf
#endif
#define			G_BUF_SIZE	2048
extern char		g_buf[G_BUF_SIZE];
void			memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes);
void			memswap(void *p1, void *p2, size_t size);
void			memreverse(void *p, size_t count, size_t size);
int				floor_log2(unsigned n);
int				floor_log10(double x);
double			power(double x, int y);
double			_10pow(int n);
int				minimum(int a, int b);
int				maximum(int a, int b);

//error handling
int				log_error(const char *file, int line, const char *format, ...);
int				valid(const void *p);
#define			LOG_ERROR(format, ...)	log_error(file, __LINE__, format, ##__VA_ARGS__)
#define			ASSERT(SUCCESS)			((SUCCESS)!=0||log_error(file, __LINE__, #SUCCESS))
#define			ASSERT_P(POINTER)		(valid(POINTER)||log_error(file, __LINE__, #POINTER " == 0"))

//file I/O
int				file_is_readable(const char *filename);//0: not readable, 1: regular file, 2: folder
char*			load_utf8(const char *filename, size_t *len);//don't forget to free string


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
	DebugInfo debug_info;
	unsigned char data[];
} ArrayHeader, *ArrayHandle;
typedef const ArrayHeader *ArrayConstHandle;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, DebugInfo debug_info);
ArrayHandle		array_copy(ArrayHandle *arr, DebugInfo debug_info);//shallow
void			array_free(ArrayHandle *arr, void (*destructor)(void*));
void			array_clear(ArrayHandle *arr, void (*destructor)(void*));//keeps allocation
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayConstHandle *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayHandle const *arr);

#ifdef DEBUG_INFO_STR
#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD, DEBUG_INFO)	ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, DEBUG_INFO)
#else
#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD)				ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, __LINE__)
#endif
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)			array_insert(&(ARR), array_size(&(ARR)), DATA, COUNT, REP, PAD)
#define			ARRAY_DATA(ARR)			(ARR)->data
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#ifdef DEBUG_INFO_STR
#define			ESTR_ALLOC(TYPE, STR, LEN, DEBUG_INFO)				STR=array_construct(0, sizeof(TYPE), 0, 1, LEN+1, DEBUG_INFO)
#define			ESTR_COPY(TYPE, STR, SRC, LEN, REP, DEBUG_INFO)		STR=array_construct(SRC, sizeof(TYPE), LEN, REP, 1, DEBUG_INFO)
#else
#define			ESTR_ALLOC(TYPE, STR, LEN)				STR=array_construct(0, sizeof(TYPE), 0, 1, LEN+1, __LINE__)
#define			ESTR_COPY(TYPE, STR, SRC, LEN, REP)		STR=array_construct(SRC, sizeof(TYPE), LEN, REP, 1, __LINE__)
#endif
#define			STR_APPEND(STR, SRC, LEN, REP)			array_insert(&(STR), array_size(&(STR)), SRC, LEN, REP, 1)
#define			STR_FIT(STR)							array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)					*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN, ...)				ESTR_ALLOC(char, STR, LEN, ##__VA_ARGS__)
#define			STR_COPY(STR, SRC, LEN, REP, ...)		ESTR_COPY(char, STR, SRC, LEN, REP, ##__VA_ARGS__)
#define			STR_AT(STR, IDX)						ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN, ...)				ESTR_ALLOC(wchar_t, STR, LEN, ##__VA_ARGS__)
#define			WSTR_COPY(STR, SRC, LEN, REP, ...)		ESTR_COPY(wchar_t, STR, SRC, LEN, REP, ##__VA_ARGS__)
#define			WSTR_AT(STR, IDX)						ESTR_AT(wchar_t, STR, IDX)
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
} DList, *DListHandle;
//DListHandle	dlist_construct(size_t objsize, size_t objpernode);
void			dlist_init(DListHandle list, size_t objsize, size_t objpernode);
void			dlist_copy(DListHandle dst, DListHandle src);
void			dlist_clear(DListHandle list, void (*destructor)(void*));
ArrayHandle		dlist_toarray(DListHandle list);

void*			dlist_push_back(DListHandle list, const void *obj);//shallow copy of obj	TODO dlist_push_back(array)
void*			dlist_back(DListHandle list);//returns address of last object
void			dlist_pop_back(DListHandle list, void (*destructor)(void*));

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
typedef CmpRes (*CmpFn)(const void *left, const void *right);
typedef struct MapStruct
{
	size_t
		key_size,	//size of key in bytes
		val_size,	//size of value in bytes
		nnodes;		//stored object count
	BSTNodeHandle root;
	CmpFn cmp_key;
} Map, *MapHandle;
typedef Map const *MapConstHandle;
void			map_init(MapHandle map, size_t key_size, size_t val_size, CmpFn cmp_key);
void			map_rebalance(MapHandle map);

BSTNodeHandle*	map_find_r(BSTNodeHandle *node, const void *key, CmpFn cmp_key);
BSTNodeHandle*	map_insert_r(BSTNodeHandle *node, const void *key, MapHandle map, const void *val, int *found);
BSTNodeHandle*	map_erase_r(BSTNodeHandle *node, const void *key, MapHandle map);
void			map_clear_r(BSTNodeHandle node, void (*destroy_callback)(void*));
void			map_debugprint_r(BSTNodeHandle *node, int depth, void (*callback)(BSTNodeHandle *node, int depth));

#define			MAP_INIT(MAP, KEYTYPE, PAIRTYPE, CMP)	map_init(MAP, sizeof(KEYTYPE), sizeof(PAIRTYPE)-sizeof(KEYTYPE), CMP)
#define			MAP_FIND(MAP, KEY)					map_find_r(&(MAP)->root, KEY, (MAP)->cmp_key)
#define			MAP_INSERT(MAP, KEY, VAL, PFOUND)	map_insert_r(&(MAP)->root, KEY, MAP, VAL, PFOUND)
#define			MAP_INSERT_PAIR(MAP, PAIR, PFOUND)	map_insert_r(&(MAP)->root, PAIR, MAP, (unsigned char*)(PAIR)+(MAP)->key_size, PFOUND)
#define			MAP_ERASE(MAP, KEY)					map_erase_r(&(MAP)->root, KEY, MAP)
#define			MAP_CLEAR(MAP, DTOR_CB)				map_clear_r((MAP)->root, DTOR_CB), (MAP)->root=0, (MAP)->nnodes=0
#define			MAP_DEBUGPRINT(MAP, CALLBACK)		map_debugprint_r(&(MAP)->root, 0, CALLBACK)
#endif


//acc
#define			CASE_MASK			0xDF
#define			BETWEEN(LO, X, HI)	((unsigned)(X-LO)<(unsigned)(HI+1-LO))
typedef enum TokenTypeEnum//should include cpp,		separate enum for asm
{
#define		TOKEN(STRING, LABEL)	LABEL,
#include	"acc_keywords.h"
#undef		TOKEN
} TokenType;
extern const char *keywords[];
typedef enum NumberBaseEnum
{
	BASE2,
	BASE8,
	BASE10,
	BASE16,
} NumberBase;
typedef struct TokenStruct//32 bytes
{
	TokenType type;
	int pos, len, line, col;
	union
	{
		int flags;//lexme<<7|base<<5|synth<<4|nl_after<<3|ws_after<<2|nl_before<<1|ws_before
		struct
		{
			int ws_before:1, nl_before:1,
				ws_after:1, nl_after:1,
				synth:1, base:2,
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
		*name,
		*srcfilename;
	int nargs,//enum MacroArgCount
		is_va;
	ArrayHandle tokens;
} Macro;
int macro_define(Macro *dst, const char *srcfilename, Token const *tokens, int count);//tokens points at after '#define', undef macro on error

extern Map	strlib;//don't clear strlib until the program quits
char*		strlib_insert(const char *str, int len);
void		strlib_debugprint();
ArrayHandle preprocess(const char *filename, MapHandle macros, ArrayHandle includepaths, MapHandle lexlib);
void		tokens2text(ArrayHandle tokens, ArrayHandle *str);


#ifdef __cplusplus
}
#endif
#endif//ACC_H
