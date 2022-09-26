#include"acc.h"
static const char file[]=__FILE__;

typedef enum SymbolTypeEnum
{
	SYM_TYPE,
	SYM_CONST,
	SYM_VAR,
	SYM_FUNC,
} SymbolType;
typedef struct SymbolStruct
{
	char *name;
	SymbolType type;
} Symbol;

static Token *g_current=0, *g_end=0;

static IRHandle		r_declaration()
{
	//Symbol symbol;
	return 0;//
}
static IRHandle		r_definition()
{
	if(g_current>=g_end)
		return 0;
	switch(g_current->type)
	{
	case T_SEMICOLON:
		++g_current;
		break;

	//case T_TYPEDEF:
	//	return r_typedef();

	//case T_EXTERN://extern in C
	//	break;

	case T_STATIC:
	case T_VOLATILE:case T_CONST:
	case T_SIGNED:case T_UNSIGNED:
	case T_VOID:
	case T_CHAR:case T_SHORT:case T_INT:case T_LONG:
	case T_FLOAT:case T_DOUBLE:
	case T_INT8:case T_INT16:case T_INT32:case T_INT64:
	case T_ENUM:case T_STRUCT:case T_UNION:
	case T_ID:
		return r_declaration();
	}
	return 0;
}
IRHandle			parse(ArrayHandle tokens)
{
	g_current=(Token*)tokens->data;
	g_end=g_current+tokens->count;
	return 0;//
}