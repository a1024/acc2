#pragma once
#ifndef ACC2_H
#define ACC2_H
#include"util.h"


typedef enum TokenTypeEnum
{
	TT_KEYWORD,
	TT_SYMBOL,
	TT_ID,
	TT_STRING_LITERAL,
	TT_CHAR_LITERAL,
	TT_INT_LITERAL,
	TT_FLOAT_LITERAL,
} TokenType;

#define KEYWORDLIST\
	KEYWORD(extern, 6) KEYWORD(static, 6)\
	KEYWORD(const, 5)\
	KEYWORD(signed, 5) KEYWORD(unsigned, 8)\
	KEYWORD(void, 4) KEYWORD(char, 4) KEYWORD(short, 5) KEYWORD(int, 3) KEYWORD(long, 4) KEYWORD(float, 5) KEYWORD(double, 6)\
	KEYWORD(if, 2) KEYWORD(else, 4)\
	KEYWORD(for, 3)\
	KEYWORD(do, 2) KEYWORD(while, 5)\
	KEYWORD(switch, 6) KEYWORD(case, 4) KEYWORD(default, 7)\
	KEYWORD(break, 5) KEYWORD(continue, 8)\
	KEYWORD(goto, 4) KEYWORD(return, 6)
//TODO enum struct union sizeof typedef
//TODO func ptr
typedef enum KeywordTypeEnum
{
#define KEYWORD(K, L) KT_##K,
	KEYWORDLIST
#undef KEYWORD
	KT_COUNT,
} KeywordType;
extern const char *keyword_str[KT_COUNT];
extern const int keyword_len[KT_COUNT];

//arranged for maximal munch
#define SYMBOLLIST\
	SYMBOL("{", 1, ST_LBRACE) SYMBOL("}", 1, ST_RBRACE)\
	SYMBOL("(", 1, ST_LPR) SYMBOL(")", 1, ST_RPR)\
	SYMBOL("[", 1, ST_LBRACKET) SYMBOL("]", 1, ST_RBRACKET)\
	SYMBOL("...", 3, ST_ELLIPSIS)\
	SYMBOL(".", 1, ST_PERIOD) SYMBOL("->", 2, ST_ARROW)\
	SYMBOL("!", 1, ST_EXCLAMATION) SYMBOL("~", 1, ST_TILDE)\
	SYMBOL("*=", 2, ST_ASSIGN_MUL) SYMBOL("*", 1, ST_ASTERISK) SYMBOL("/=", 2, ST_ASSIGN_DIV) SYMBOL("/", 1, ST_SLASH) SYMBOL("%=", 2, ST_ASSIGN_MOD) SYMBOL("%", 1, ST_PERCENT)\
	SYMBOL("++", 2, ST_INC) SYMBOL("--", 2, ST_DEC)\
	SYMBOL("+=", 2, ST_ASSIGN_ADD) SYMBOL("+", 1, ST_PLUS) SYMBOL("-=", 2, ST_ASSIGN_SUB) SYMBOL("-", 1, ST_MINUS)\
	SYMBOL("<<=", 3, ST_ASSIGN_SHIFT_LEFT) SYMBOL("<<", 2, ST_SHIFT_LEFT) SYMBOL(">>=", 3, ST_ASSIGN_SHIFT_RIGHT) SYMBOL(">>", 2, ST_SHIFT_RIGHT)\
	SYMBOL("<=", 2, ST_LESS_EQUAL) SYMBOL("<", 1, ST_LESS) SYMBOL(">=", 2, ST_GREATER_EQUAL) SYMBOL(">", 1, ST_GREATER)\
	SYMBOL("==", 2, ST_EQUAL) SYMBOL("!=", 2, ST_NOT_EQUAL)\
	SYMBOL("&&", 2, ST_LOGIC_AND)\
	SYMBOL("&=", 2, ST_ASSIGN_AND) SYMBOL("&", 1, ST_AMPERSAND)\
	SYMBOL("^=", 2, ST_ASSIGN_XOR) SYMBOL("^", 1, ST_BITWISE_XOR)\
	SYMBOL("||", 2, ST_LOGIC_OR)\
	SYMBOL("|=", 2, ST_ASSIGN_OR) SYMBOL("|", 1, ST_BITWISE_OR)\
	SYMBOL("?", 1, ST_QUESTION) SYMBOL(":", 1, ST_COLON)\
	SYMBOL("=", 1, ST_ASSIGN)\
	SYMBOL(",", 1, ST_COMMA)\
	SYMBOL(";", 1, ST_SEMICOLON)
typedef enum SymbolTypeEnum
{
#define SYMBOL(S, L, F) F,
	SYMBOLLIST
#undef SYMBOL
	ST_COUNT,
} SymbolType;
extern const char *symbol_str[ST_COUNT];
extern const int symbol_len[ST_COUNT];

typedef struct TokenStruct
{
	TokenType type;
	int lineno;
	union
	{
		KeywordType keyword_type;
		SymbolType symbol_type;
		char *val_str;
		long long val_int;
		double val_float;
	};
} Token;


//numbers
typedef enum NumberTypeEnum
{
	//integer results:
	NUM_I8,//suffix 'i8' && can fit in 2's complement 8 bits
	NUM_U8,//suffix 'ui8'
	NUM_I16,//suffix 'i16' && can fit in 2's complement 16 bits
	NUM_U16,//suffix 'ui16'

	NUM_I32,//no suffix && can fit in 2's complement 32 bits
	NUM_U32,//has suffix U
	NUM_I64,//has suffix LL || from 2^31 to 2^63
	NUM_U64,//has suffix ULL || from 2^31 to 2^64
	NUM_I128,//has suffix LL || from 2^31 to 2^63
	NUM_U128,//has suffix ULL || from 2^31 to 2^64

	//floating point results:
	NUM_F32,//has point and suffix F
	NUM_F64,//has point
	NUM_F128,//has point and suffix L
} NumberType;
typedef enum NumberBaseEnum
{
	BASE2,
	BASE8,
	BASE10,
	BASE16,
} NumberBase;//fits in 2 bits
typedef struct NumberStruct
{
	NumberType type;
	NumberBase base;
	union
	{
		long long i64;
		unsigned long long u64;
		double f64;
		float f32;
	};
} Number;


typedef enum ASTNodeTypeEnum
{
	NT_TOKEN,
	NT_PROGRAM,
	NT_TYPE,
	NT_FUNCHEADER,
	NT_ARGDECL,
	NT_ARGDECL_ELLIPSIS,
	NT_FUNCBODY,
	NT_SCOPE,
	NT_STMT_IF,
	NT_STMT_FOR,
	NT_STMT_DO_WHILE,
	NT_STMT_WHILE,
	NT_STMT_CONTINUE,
	NT_STMT_BREAK,
	NT_STMT_SWITCH,
	NT_LABEL_CASE,
	NT_LABEL_DEFAULT,
	NT_STMT_GOTO,
	NT_LABEL_TOGO,
	NT_STMT_RETURN,
	NT_OP_POST_INC, NT_OP_POST_DEC, NT_OP_FUNCCALL, NT_OP_SUBSCRIPT, NT_OP_MEMBER, NT_OP_ARROW,
	NT_OP_PRE_INC, NT_OP_PRE_DEC, NT_OP_POS, NT_OP_NEG, NT_OP_LOGIC_NOT, NT_OP_BITWISE_NOT, NT_OP_CAST, NT_OP_DEREFERENCE, NT_OP_ADDRESS_OF,
	NT_OP_MUL, NT_OP_DIV, NT_OP_MOD,
	NT_OP_ADD, NT_OP_SUB,
	NT_OP_SHIFT_LEFT, NT_OP_SHIFT_RIGHT,
	NT_OP_LESS, NT_OP_LESS_EQUAL, NT_OP_GREATER, NT_OP_GREATER_EQUAL,
	NT_OP_EQUAL, NT_OP_NOT_EQUAL,
	NT_OP_BITWISE_AND,
	NT_OP_BITWISE_XOR,
	NT_OP_BITWISE_OR,
	NT_OP_LOGIC_AND,
	NT_OP_LOGIC_OR,
	NT_OP_TERNARY,
	NT_OP_ASSIGN, NT_OP_ASSIGN_ADD, NT_OP_ASSIGN_SUB, NT_OP_ASSIGN_MUL, NT_OP_ASSIGN_DIV, NT_OP_ASSIGN_MOD, NT_OP_ASSIGN_SL, NT_OP_ASSIGN_SR, NT_OP_ASSIGN_AND, NT_OP_ASSIGN_XOR, NT_OP_ASSIGN_OR,
	NT_OP_COMMA,
} ASTNodeType;
typedef struct ASTNodeStruct
{
	ASTNodeType type;
	short nch;
	short indirection_count;//for pointers
	union
	{
		Token t;
		struct
		{
			int nbits;
			char is_unsigned, is_const, is_float, is_extern, is_static;
		};
	};
	struct ASTNodeStruct *ch[];
} ASTNode;


//lexer
ArrayHandle lex(const char *text, int len);
void print_token(Token *t);
void print_tokens(Token *tokens, int count);


//parser
int compile_error(Token *t, int level, const char *format, ...);
#define COMPI(T, F, ...) compile_error(T, 1, F, ##__VA_ARGS__)
#define COMPW(T, F, ...) compile_error(T, 2, F, ##__VA_ARGS__)
#define COMPE(T, F, ...) compile_error(T, 3, F, ##__VA_ARGS__)

ASTNode* parse_program(Token *tokens, int count);
void print_ast(ASTNode *root, int depth);

ASTNode* node_alloc(ASTNodeType type);
void node_free(ASTNode **root);
int node_append_ch(ASTNode **root, ASTNode *ch);
int node_attach_ch(ASTNode **root, ASTNode ***p_leaf, ASTNode *ch);


//bytecode
typedef enum InstructionTypeEnum
{
	IT_LOAD, IT_STORE,
	IT_ADD, IT_SUB,
	IT_MUL, IT_DIV, IT_MOD,
	IT_SLL, IT_SRL, IT_SRA,
	IT_LT, IT_LE, IT_GT, IT_GE, IT_EQ, IT_NE,
	IT_AND, IT_XOR, IT_OR,
	IT_JMP, IT_BLT, ITBLE, IT_BGT, IT_BGE, IT_BEQ, IT_BNE,
	IT_PUSH, IT_POP,
} InstructionType;
//typedef enum ArgTypeEnum
//{
//	ARG_IMM,
//	ARG_REG,
//} ArgType;
//typedef struct EmuArgStruct
//{
//	long long value;
//	short indirection_count;
//	char is_immediate;
//} EmuArg;
typedef struct InstructionStruct
{
	long long val[4];
	InstructionType type;
	short indirection_count[4];
	char is_immediate[4], is_unsigned[4];
} Instruction;
ArrayHandle translate(ASTNode *c_code);


#endif//ACC2_H
