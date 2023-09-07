#pragma once
#ifndef ACC_H
#define ACC_H
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include"util.h"
#ifdef __cplusplus
extern "C"
{
#endif


	#define		BENCHMARK_LEXER


//benchmark
#ifdef BENCHMARK_LEXER
#define			BM_DECL				double bm_t1
#define			BM_START()			bm_t1=time_ms()
#define			BM_FINISH(MSG, ...)	bm_t1=time_ms()-bm_t1, printf(MSG, ##__VA_ARGS__)
#else
#define			BM_DECL
#define			BM_START()
#define			BM_FINISH(MSG)
#endif


//acc
extern char		*currentdate, *currenttime, *currenttimestamp;
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
		unsigned flags;//lexme<<7|base<<5|synth<<4|nl_after<<3|ws_after<<2|nl_before<<1|ws_before
		struct
		{
			unsigned
				ws_before:1, nl_before:1,
				ws_after:1, nl_after:1,
				synth:1,
				base:2,//NumberBase
				lexme:1;//FIXME: remove this flag
		};
	};
	union
	{
		long long i;
		unsigned long long u;
		double f64;	//lossy
		float f32;	//lossy
		char *str;	//see strlib
		int *wstr;
	};
} Token;

typedef enum LexFlagsEnum
{
	LEX_NORMAL,
	LEX_INCLUDE_ONCE,
} LexFlags;
typedef struct LexedFileStruct
{
	char *filename;//belongs to strlib
	ArrayHandle
		text,//string, must be over-allocated by 16 bytes
		tokens;//array of tokens
	LexFlags flags;
	int filename_len;
} LexedFile;

typedef enum MacroArgCountEnum
{
	MACRO_NO_ARGLIST=-1,
	MACRO_EMPTY_ARGLIST=0,
} MacroArgCount;
typedef struct MacroStruct
{
	char *name;//THE KEY, should be the first attribute, belongs to strlib
	LexedFile *srcfile;//check for nullptr (in pre-defined macros)
	int nargs,//enum MacroArgCount
		is_va;
	ArrayHandle tokens;
} Macro;

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


#if 0
typedef enum DataTypeEnum
{
	TYPE_UNASSIGNED,
	TYPE_VOID,
	TYPE_CHAR,
	TYPE_INT,
	TYPE_UINT,
	TYPE_FLOAT,
	TYPE_ENUM,
	TYPE_ENUM_CONST,
	TYPE_STRUCT,
	TYPE_UNION,
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_FUNC,
	TYPE_ELLIPSIS,
} DataType;
typedef enum CallTypeEnum
{
	CALL_CDECL,
	CALL_STDCALL,
} CallType;
struct IRNodeStruct;
typedef struct TypeInfoStruct
{
	union
	{
		struct
		{
			unsigned char
				datatype,
				lgbytealign:4,//selects from {1, 2, 4, 8, 16, 32, 64, 128} bytes align
				is_const:1,
				is_volatile:1,
				is_winapi:1,//false: __cdecl, true: __stdcall
				is_extern:1;//false: static, true: extern
		};
		unsigned flags;
	};
	size_t size;
	struct IRNodeStruct *body;
} TypeInfo;
#endif
typedef struct IRNodeStruct
{
	Token token;
	int nNodes;
	struct IRNodeStruct *nodes[];
} IRNode, *IRHandle;
IRHandle	parse(ArrayHandle tokens);


void		acc_cleanup(MapHandle lexlib, MapHandle strings);

void		tokens2text(ArrayHandle tokens, ArrayHandle *str);


#ifdef __cplusplus
}
#endif
#endif//ACC_H
