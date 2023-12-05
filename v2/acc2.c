#include"acc2.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdarg.h>
#include<ctype.h>
#include<math.h>
static const char file[]=__FILE__;


//definitions	TODO move to header
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
	KEYWORD(void, 4) KEYWORD(char, 4) KEYWORD(short, 5) KEYWORD(int, 3) KEYWORD(long, 3) KEYWORD(float, 5) KEYWORD(double, 6)\
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
const char *keyword_str[]=
{
#define KEYWORD(K, L) #K,
	KEYWORDLIST
#undef KEYWORD
};
const int keyword_len[]=
{
#define KEYWORD(K, L) L,
	KEYWORDLIST
#undef KEYWORD
};

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
const char *symbol_str[]=
{
#define SYMBOL(S, L, F) S,
	SYMBOLLIST
#undef SYMBOL
};
const int symbol_len[]=
{
#define SYMBOL(S, L, F) L,
	SYMBOLLIST
#undef SYMBOL
};

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


//stringlib
typedef struct StringStruct
{
	const char *str;
	size_t len;
} String;
Map strlib={0};
static CmpRes strlib_cmp(const void *key, const void *pair)
{
	String const *s1=(String const*)key;
	const char *s2=*(const char**)pair;
	size_t k;
	char c1;

	for(k=0;k<s1->len&&*s2&&s1->str[k]==*s2;++k, ++s2);

	c1=k<s1->len?s1->str[k]:0;
	return (c1>*s2)-(c1<*s2);
}
void strlib_destructor(void *data)
{
	char **str=(char**)data;
	free(*str);
	*str=0;
}
char* strlib_insert(const char *str, int len)
{
	String s2={str, len?len:strlen(str)};//str is not always null-terminated, but is sometimes of length 'len' instead
	char *key, **target;
	int found;
	RBNodeHandle *node;

	if(!strlib.comparator)
		map_init(&strlib, sizeof(char*), strlib_cmp, strlib_destructor);

	node=map_insert(&strlib, &s2, &found);		//search first then allocate
	ASSERT(node&&*node);
	target=(char**)node[0]->data;
	if(!found)
	{
		key=(char*)malloc(s2.len+1);
		memcpy(key, str, s2.len);
		key[s2.len]=0;
		*target=key;
	}
	return *target;
}


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
static unsigned long long acme_read_int_base10(const char *text, int len, int *idx, int *ret_ndigits)
{
	unsigned long long val;
	unsigned char ndigits, c;

	val=0, ndigits=0;
	for(;ndigits<20&&*idx<len;++*idx)
	{
		c=text[*idx]-'0';
		if(c<10)
		{
			val*=10;
			val+=c;
			++ndigits;
		}
		else if(c!=(unsigned char)('\''-'0'))//'\'' is ignored in a number
			break;
	}
	if(ret_ndigits)
		*ret_ndigits=ndigits;
	return val;
}
static unsigned long long acme_read_int_basePOT(const char *text, int len, int *idx, NumberBase base, int *ret_ndigits)
{
	static const char
		digit_count_lim[]={64, 22, 0, 16},
		digit_lim[]={2, 8, 0, 10},
		shift[]={1, 3, 0, 4};
	
	unsigned long long val;
	unsigned char ndigits, c;

	val=0, ndigits=0;
	for(;ndigits<digit_count_lim[base]&&*idx<len;++*idx)
	{
		c=text[*idx]-'0';
		if(c<digit_lim[base])
		{
			val<<=shift[base];
			val|=c;
			++ndigits;
		}
		else
		{
			c+='0';
			if(base==BASE16)
			{
				c&=0xDF;
				c-='A';
				if(c<6)
				{
					val<<=4;
					val|=c+10;
					++ndigits;
					continue;
				}
			}
			if(c!='\'')
				break;
		}
	}
	if(ret_ndigits)
		*ret_ndigits=ndigits;
	return val;
}
static int acme_read_number_suffix(const char *text, int len, int *idx, Number *ret)
{
	int start=*idx;
	char U=0, L=0, LL=0, Z=0, F=0;
	int success=1;
	
	for(;*idx<len;)
	{
		switch(text[*idx]&0xDF)
		{
		case 'F'://float literal
			if(F)
			{
				//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
				break;
			}
			F=1;
			++*idx;
			continue;
		case 'L'://long [[long]int/double] literal
			if((text[*idx+1]&0xDF)=='L')
			{
				if(LL||L)
				{
					//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
					success=0;
					break;
				}
				LL=1;
				*idx+=2;
			}
			else
			{
				if(LL||L)
				{
					//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
					success=0;
					break;
				}
				if(ret->type==NUM_F32||F)
				{
					//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
					success=0;
					break;
				}
				if(ret->type==NUM_F64)
					ret->type=NUM_F128;
				L=1;
				++*idx;
			}
			continue;
		case 'U'://unsigned
			if(U)
			{
				//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
				break;
			}
			U=1;
			++*idx;
			continue;
		case 'Z'://ptrdiff_t/size_t
			if(Z)
			{
				//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
				break;
			}
			Z=1;
			++*idx;
			continue;
		case 'I'://integer of a particular size
			{
				++*idx;
				unsigned size=(unsigned)acme_read_int_base10(text, len, idx, 0);
				if(ret->type==NUM_F32||ret->type==NUM_F64||ret->type==NUM_F128)
				{
					//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
					success=0;
				}
				else
				{
					switch(size)
					{
					case 8:		ret->type=NUM_I8	|ret->type&1;	break;
					case 16:	ret->type=NUM_I16	|ret->type&1;	break;
					case 32:	ret->type=NUM_I32	|ret->type&1;	break;
					case 64:	ret->type=NUM_I64	|ret->type&1;	break;
					case 128:	ret->type=NUM_I128	|ret->type&1;	break;
					default:
						//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
						success=0;
						break;
					}
				}
			}
			break;
		default:
			if(isalnum(text[*idx])||text[*idx]=='_')
			{
				//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
			}
			break;
		}
		break;
	}
	if(F)
	{
		switch(ret->type)
		{
		case NUM_F64:
			ret->f32=(float)ret->f64;
		case NUM_F32:
			break;
		default:
			//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
			success=0;
			break;
		}
		ret->type=NUM_F32;
	}
	else
	{
		if(ret->type==NUM_F32||ret->type==NUM_F64)
		{
			if(U||Z||LL||ret->type==NUM_F32&&L)
			{
				//lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
			}
		}
		else if(U)
		{
			if(LL||L)
				ret->type=NUM_U64;
			else if(Z)
				ret->type=NUM_U64;//TODO: size_t: U64 for x86-64, and U32 for x86
			else
			{
				switch(ret->type)
				{
				case NUM_I32:
					ret->type=NUM_U32;
					break;
				case NUM_I64:
					ret->type=NUM_U64;
					break;
				default:
					break;
				}
			}
		}
		else if(LL||L)
		{
			if(ret->type==NUM_I32)
				ret->type=NUM_I64;
			else if(ret->type==NUM_U32)
				ret->type=NUM_U64;
		}
		else if(Z)
			ret->type=NUM_I64;//TODO: ptrdiff_t: I64 for x86-64, and I32 for x86
	}
	return success;
}
int acme_read_number(const char *text, int len, int *idx, Number *ret)
{
	unsigned long long tail;
	int success, start, base, ndigits, overflow;
	double val;

	success=1;
	start=*idx;
	ret->type=NUM_I32;
	if(text[*idx]=='0')
	{
		switch(text[*idx+1])
		{
		case 'B':case 'b':
			*idx+=2;
			ret->base=BASE2, base=2;
			break;
		case 'X':case 'x':
			*idx+=2;
			ret->base=BASE16, base=16;
			break;
		case '.':
			ret->base=BASE10, base=10;
			break;
		default:
			ret->base=BASE8, base=8;
			break;
		}
	}
	else
		ret->base=BASE10, base=10;
	
	switch(ret->base)//FIXME: needs precise integer overflow detection
	{
	case BASE2:case BASE8:case BASE16:
		ret->u64=acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
		val=(double)ret->u64;
		if(overflow=acme_isdigit(text[*idx], base))
		{
			do
			{
				long long v2=acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
				val=val*pow(base, ndigits)+v2;
			}
			while(acme_isdigit(text[*idx], base));
		}
		if(text[*idx]=='.')
		{
			++*idx;
			ret->type=NUM_F64;
			tail=acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
			ret->f64=val+tail*pow(base, -ndigits);
			while(acme_isdigit(text[*idx], base))//ignore the rest
				acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
		}
		else if(overflow)
		{
			//lex_error(text, len, start, "Integer overflow");
			success=0;
		}
		if((text[*idx]&0xDF)=='P')
		{
			++*idx;
			if(ret->type==NUM_I32)
			{
				ret->f64=(double)ret->u64;
				ret->type=NUM_F64;
			}
			int sign=1;
			if(text[*idx]=='-')
				sign=-1;
			*idx+=text[*idx]=='+'||text[*idx]=='-';
			double e=(double)acme_read_int_base10(text, len, idx, &ndigits);
			ret->f64*=pow(2., sign*e);
		}
		break;
	case BASE10:
		ret->u64=acme_read_int_base10(text, len, idx, &ndigits);
		val=(double)ret->u64;
		if(overflow=acme_isdigit(text[*idx], base))
		{
			do
			{
				long long v2=acme_read_int_base10(text, len, idx, &ndigits);
				val=val*pow(base, ndigits)+v2;
			}
			while(acme_isdigit(text[*idx], base));
		}
		if(text[*idx]=='.')
		{
			++*idx;
			ret->type=NUM_F64;
			tail=acme_read_int_base10(text, len, idx, &ndigits);
			ret->f64=val+tail*_10pow(-ndigits);
			while(acme_isdigit(text[*idx], base))//ignore the rest
				acme_read_int_base10(text, len, idx, &ndigits);
		}
		else if(overflow)
		{
			success=0;
			//lex_error(text, len, start, "Integer overflow");
		}
		if((text[*idx]&0xDF)=='E')
		{
			++*idx;
			if(ret->type==NUM_I32)
			{
				ret->f64=(double)ret->u64;
				ret->type=NUM_F64;
			}
			int sign=1;
			if(text[*idx]=='-')
				sign=-1;
			*idx+=text[*idx]=='+'||text[*idx]=='-';
			int e=(int)acme_read_int_base10(text, len, idx, &ndigits);
			ret->f64*=_10pow(sign*e);
		}
		break;
	}
	success&=acme_read_number_suffix(text, len, idx, ret);
	return success;
}


//strings & UTF-8
int codepoint2utf8(int codepoint, char *out)//returns sequence length (up to 4 characters)
{
	//https://stackoverflow.com/questions/6240055/manually-converting-unicode-codepoints-into-utf-8-and-utf-16
	if(codepoint<0)
	{
		printf("Invalid Unicode codepoint 0x%08X\n", codepoint);
		return 0;
	}
	if(codepoint<0x80)
	{
		out[0]=codepoint;
		return 1;
	}
	if(codepoint<0x800)
	{
		out[0]=0xC0|(codepoint>>6)&0x1F, out[1]=0x80|codepoint&0x3F;
		return 2;
	}
	if(codepoint<0x10000)
	{
		out[0]=0xE0|(codepoint>>12)&0x0F, out[1]=0x80|(codepoint>>6)&0x3F, out[2]=0x80|codepoint&0x3F;
		return 3;
	}
	if(codepoint<0x200000)
	{
		out[0]=0xF0|(codepoint>>18)&0x07, out[1]=0x80|(codepoint>>12)&0x3F, out[2]=0x80|(codepoint>>6)&0x3F, out[3]=0x80|codepoint&0x3F;
		return 4;
	}
	printf("Invalid Unicode codepoint 0x%08X\n", codepoint);
	return 0;
}
int utf8codepoint(const char *in, int *codepoint)//returns sequence length
{
	char c=in[0];
	if(c>=0)
	{
		*codepoint=c;
		return 1;
	}
	if((c&0xE0)==0xC0)
	{
		char c2=in[1];
		if((c2&0xC0)==0x80)
			*codepoint=(c&0x1F)<<6|(c2&0x3F);
		else
		{
			*codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X\n", (int)c, (int)c2);
		}
		return 2;
	}
	if((c&0xF0)==0xE0)
	{
		char c2=in[1], c3=in[2];
		if((c2&0xC0)==0x80&&(c3&0xC0)==0x80)
			*codepoint=(c&0x0F)<<12|(c2&0x3F)<<6|(c3&0x3F);
		else
		{
			*codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X-%02X\n", (int)c, (int)c2, (int)c3);
		}
		return 3;
	}
	if((c&0xF8)==0xF0)
	{
		char c2=in[1], c3=in[2], c4=in[3];
		if((c2&0xC0)==0x80&&(c3&0xC0)==0x80&&(c4&0xC0)==0x80)
			*codepoint=(c&0x07)<<18|(c2&0x3F)<<12|(c3&0x3F)<<6|(c4&0x3F);
		else
		{
			*codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X-%02X-%02X\n", (int)c, (int)c2, (int)c3, (int)c4);
		}
		return 4;
	}
	*codepoint='?';
	printf("Invalid UTF-8 sequence: %02X\n", (int)c);
	return 1;
}

//escape sequences -> raw string
ArrayHandle esc2str(const char *s, int size)
{
	ArrayHandle ret;
	char utf8_seq[5]={0};
	if(!s)
	{
		printf("Internal error: esc2str: string is null.\n");
		return 0;
	}
	ret=array_construct(0, 1, 0, 1, size+1, 0);
	for(int ks=0;ks<size;++ks)
	{
		if(s[ks]!='\\')
			STR_APPEND(ret, s+ks, 1, 1);
		else
		{
			++ks;
			switch(s[ks])
			{
			case 'a':	STR_APPEND(ret, "\a", 1, 1);break;
			case 'b':	STR_APPEND(ret, "\b", 1, 1);break;
			case 'f':	STR_APPEND(ret, "\f", 1, 1);break;
			case 'n':	STR_APPEND(ret, "\n", 1, 1);break;
			case 'r':	STR_APPEND(ret, "\r", 1, 1);break;
			case 't':	STR_APPEND(ret, "\t", 1, 1);break;
			case 'v':	STR_APPEND(ret, "\v", 1, 1);break;
			case '\'':	STR_APPEND(ret, "\'", 1, 1);break;
			case '\"':	STR_APPEND(ret, "\"", 1, 1);break;
			case '\\':	STR_APPEND(ret, "\\", 1, 1);break;
			case '\?':	STR_APPEND(ret, "?", 1, 1);break;
			case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':	//case '8':case '9'://dec codepoint	SHOULD BE OCTAL
			case 'x':case 'u'://hex codepoint
				{
					int hex=s[ks]=='x'||s[ks]=='u';
					ks+=hex;
					int end=ks, ndigits=0;
					int val=(int)acme_read_int_basePOT(s, size, &end, hex?BASE16:BASE8, &ndigits);

					size_t len=ret->count;
					STR_APPEND(ret, 0, 4, 1);
					int seqlen=codepoint2utf8(val, ret->data+len);
					ret->count-=4-seqlen;
					ks=end-1;
				}
				break;
			case '\n'://escaped newline
				break;
			default:
				STR_APPEND(ret, "\\", 1, 1);
				STR_APPEND(ret, s+ks, 1, 1);

				printf("Unsupported escape sequence at %d: \\%c\n", ks, s[ks]);
				break;
			}
		}
	}
	return ret;
}
//raw string -> escape sequences
ArrayHandle str2esc(const char *s, int size)
{
	ArrayHandle ret;
	if(!s)
	{
		printf("Internal error: str2esc: string is null.\n");
		return 0;
	}
	if(!size)
		size=(int)strlen(s);

	ret=array_construct(0, 1, 0, 1, size+1, 0);
	for(int ks=0;ks<size;++ks)
	{
		char c=s[ks];
		switch(c)
		{
		case '\0':
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case '\'':
		case '\"':
		case '\\':
			STR_APPEND(ret, "\\", 1, 1);
			switch(c)
			{
			case '\0':STR_APPEND(ret, "0", 1, 1);break;
			case '\a':STR_APPEND(ret, "a", 1, 1);break;
			case '\b':STR_APPEND(ret, "b", 1, 1);break;
			case '\f':STR_APPEND(ret, "f", 1, 1);break;
			case '\n':STR_APPEND(ret, "n", 1, 1);break;
			case '\r':STR_APPEND(ret, "r", 1, 1);break;
			case '\t':STR_APPEND(ret, "t", 1, 1);break;
			case '\v':STR_APPEND(ret, "v", 1, 1);break;
			case '\'':STR_APPEND(ret, "\'", 1, 1);break;
			case '\"':STR_APPEND(ret, "\"", 1, 1);break;
			case '\\':STR_APPEND(ret, "\\", 1, 1);break;
			}
			break;
		default:
			if(c>=' '&&c<0x7F)
				STR_APPEND(ret, s+ks, 1, 1);
			else
			{
				int codepoint=0;
				int seqlen=utf8codepoint(s+ks, &codepoint);

				array_insert(&ret, ret->count, 0, 0, 1, 16);
				if(codepoint<0x10000)
					ret->count+=sprintf_s(ret->data+ret->count, ret->count, "\\u%04X", codepoint);
				else
					ret->count+=sprintf_s(ret->data+ret->count, ret->count, "\\U%08X", codepoint);

				ks+=seqlen-1;
			}
			break;
		}
	}
	return ret;
}


//lexer
static int match(const char *text, int idx, int len, const char *kw)
{
	int k=idx, k2=0;
	for(;k<len&&kw[k2]&&text[k]==kw[k2];++k, ++k2);
	return !kw[k2];
}
ArrayHandle lex(const char *text, int len)
{
	DList list;
	dlist_init(&list, sizeof(Token), 64, 0);
	Token token;
	for(int k=0, lineno=0;k<len;)
	{
		if(text[k]=='*')
			printf("");

		if(isspace(text[k]))
		{
			for(;k<len&&isspace(text[k]);lineno+=text[k]=='\n', ++k);
		}
		else if(isalpha(text[k])||text[k]=='_')
		{
			int matched=0;
			for(int k2=0;k2<KT_COUNT;++k2)
			{
				if(match(text, k, len, keyword_str[k2])&&k+keyword_len[k2]<len&&!isalnum(text[k+keyword_len[k2]]))
				{
					matched=1;
					token.type=TT_KEYWORD;
					token.val_int=k2;
					token.lineno=lineno;
					dlist_push_back1(&list, &token);
					k+=keyword_len[k2];
					break;
				}
			}
			if(!matched)//identifier
			{
				int start=k;
				for(;k<len&&(isalpha(text[k])||text[k]=='_');++k);
				token.type=TT_ID;
				token.val_str=strlib_insert(text+start, k-start);
				token.lineno=lineno;
				dlist_push_back1(&list, &token);
			}
		}
		else if(isdigit(text[k]))//number literal
		{
			Number num;
			int success=acme_read_number(text, len, &k, &num);
			if(!success)
				for(;k<len&&isdigit(text[k]);++k);//skip the number
			else
			{
				if(num.type>=NUM_I8&&num.type<=NUM_U128)
				{
					token.type=TT_INT_LITERAL;
					token.val_int=num.i64;
				}
				else if(num.type==NUM_F32)
				{
					token.type=TT_FLOAT_LITERAL;
					token.val_float=num.f32;
				}
				else if(num.type==NUM_F64||num.type==NUM_F128)
				{
					token.type=TT_FLOAT_LITERAL;
					token.val_float=num.f64;
				}
				token.lineno=lineno;
				dlist_push_back1(&list, &token);
			}
		}
		else if(text[k]=='\"'||text[k]=='\'')
		{
			char quote=text[k];
			++k;
			if(k<len)
			{
				int start=k;
				for(;k<len&&text[k]!=quote&&text[k]!='\n';++k);
				ArrayHandle t2=esc2str(text+start, k-start);
				if(quote=='\"')//string literal
				{
					token.type=TT_STRING_LITERAL;
					token.val_str=strlib_insert((char*)t2->data, (int)t2->count);
					token.lineno=lineno;
					dlist_push_back1(&list, &token);
				}
				else if(quote=='\'')//char literal
				{
					long long result=0;
					for(int k2=0;k2<(int)t2->count&&k2<8;++k2)
						result|=(long long)t2->data[k2]<<(k2<<3);
					token.type=TT_CHAR_LITERAL;
					token.val_int=result;
					token.lineno=lineno;
					dlist_push_back1(&list, &token);
				}
				array_free(&t2);
				lineno+=text[k]=='\n';
				k+=text[k]==quote;
			}
		}
		else//symbols
		{
			int symbol=-1;
			switch(text[k])
			{
			case '{':symbol=ST_LBRACE;break;
			case '}':symbol=ST_RBRACE;break;
			case '(':symbol=ST_LPR;break;
			case ')':symbol=ST_RPR;break;
			case '=':symbol=ST_ASSIGN;break;
			case '*':symbol=ST_ASTERISK;break;
			case '.':
				if(match(text, k, len, symbol_str[ST_ELLIPSIS]))
					symbol=ST_ELLIPSIS;
				else
					symbol=ST_PERIOD;
				break;
			case ',':symbol=ST_COMMA;break;
			case ';':symbol=ST_SEMICOLON;break;
			}
			if(symbol==-1)
				++k;
			else
			{
				token.type=TT_SYMBOL;
				token.val_int=symbol;
				token.lineno=lineno;
				dlist_push_back1(&list, &token);
				k+=symbol_len[symbol];
			}
		}
	}
	ArrayHandle arr=0;
	dlist_appendtoarray(&list, &arr);
	dlist_clear(&list);
	return arr;
}
void print_token(Token *t)
{
	switch(t->type)
	{
	case TT_KEYWORD:
		printf("%s", keyword_str[t->keyword_type]);
		break;
	case TT_SYMBOL:
		printf("%s", symbol_str[t->symbol_type]);
		break;
	case TT_ID:
		printf("%s", t->val_str);
		break;
	case TT_STRING_LITERAL:
	case TT_CHAR_LITERAL:
		{
			ArrayHandle t2=str2esc(t->val_str, (int)strlen(t->val_str));
			char quote=t->type==TT_STRING_LITERAL?'\"':'\'';
			printf("%c%s%c", quote, t2->data, quote);
			array_free(&t2);
		}
		break;
	case TT_INT_LITERAL:
		printf("%lld", t->val_int);
		break;
	case TT_FLOAT_LITERAL:
		printf("%lf", t->val_float);
		break;
	}
}
void print_tokens(Token *tokens, int count)
{
	for(int k=0;k<count;++k)
	{
		Token *t=tokens+k;
		if(k&&t->lineno>tokens[k-1].lineno)
			printf("\n");
		print_token(t);
		if(k+1<count)
			printf(" ");
	}
	printf("\n");
}


//error handling
int compile_error(Token *t, int level, const char *format, ...)
{
	va_list args;
	const char *levelstr="INFO";
	switch(level)
	{
	case 1:levelstr="INFO";break;
	case 2:levelstr="WARNING";break;
	case 3:levelstr="ERROR";break;
	}
	printf("(%d) %s: ", t->lineno+1, levelstr);
	va_start(args, format);
	vprintf(format, args);
	va_end(format);
	printf("\n");
	return 0;
}
#define COMPI(T, F, ...) compile_error(T, 1, F, ##__VA_ARGS__)
#define COMPW(T, F, ...) compile_error(T, 2, F, ##__VA_ARGS__)
#define COMPE(T, F, ...) compile_error(T, 3, F, ##__VA_ARGS__)


//parser
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
	NT_OP_ARROW, NT_OP_MEMBER, NT_OP_SUBSCRIPT, NT_OP_FUNCCALL, NT_OP_POST_INC, NT_OP_POST_DEC,
	NT_OP_ADDRESS_OF, NT_OP_DEREFERENCE, NT_OP_CAST, NT_OP_LOGIC_NOT, NT_OP_BITWISE_NOT, NT_OP_POS, NT_OP_NEG, NT_OP_PRE_INC, NT_OP_PRE_DEC,
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
static ASTNode* node_alloc(ASTNodeType type)
{
	ASTNode *n=(ASTNode*)malloc(sizeof(ASTNode));
	if(!n)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	memset(n, 0, sizeof(ASTNode));
	n->type=type;
	return n;
}
static void node_free(ASTNode **root)
{
	if(*root)
	{
		for(int k=0;k<root[0]->nch;++k)
			node_free(root[0]->ch+k);
		free(*root);
		*root=0;
	}
}
static int node_append_ch(ASTNode **root, ASTNode *ch)//as an array
{
	int nch=root[0]->nch+1;
	void *p=realloc(*root, sizeof(ASTNode)+nch*sizeof(void*));
	if(!p)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	*root=(ASTNode*)p;
	root[0]->ch[root[0]->nch]=ch;
	root[0]->nch=nch;
	return 1;
}
static int node_attach_ch(ASTNode **root, ASTNode ***p_leaf, ASTNode *ch)//as a linked-list
{
	int ret=1;
	if(!*root)
	{
		*root=ch;
		*p_leaf=root;
	}
	else
	{
		ret=node_append_ch(*p_leaf, ch);
		*p_leaf=ch->ch;
	}
	return ret;
}
#define CHECKIDX(RET)\
	if(*idx>=count)\
	{\
		COMPE(tokens+*idx-1, "Unexpected EOF");\
		return RET;\
	}

//forward declarations
static ASTNode* parse_expr_commalist(Token *tokens, int *idx, int count);
static ASTNode* parse_declaration(Token *tokens, int *idx, int count, int global);
static ASTNode* parse_code(Token *tokens, int *idx, int count);


//the parser
static int is_type(KeywordType t)
{
	switch(t)
	{
	case KT_const:
	case KT_signed:
	case KT_unsigned:
	case KT_void:
	case KT_char:
	case KT_short:
	case KT_int:
	case KT_long:
	case KT_float:
	case KT_double:
		return 1;
	}
	return 0;
}
static ASTNode* parse_type(Token *tokens, int *idx, int count)
{
	Token *t;
	char invalid=0, is_const=0, is_unsigned=-1, nbytes=0, is_float=0, is_extern=0, is_static=0;
	t=tokens+*idx;
	for(;*idx<count&&t->type==TT_KEYWORD;++*idx, t=tokens+*idx)
	{
		switch(t->keyword_type)
		{
		case KT_extern:
			if(is_static)
			{
				invalid=1;
				COMPE(t, "Invalid storage type");
			}
			is_extern=1;
			break;
		case KT_static:
			if(is_extern)
			{
				invalid=1;
				COMPE(t, "Invalid storage type");
			}
			is_static=1;
			break;
		case KT_unsigned:
			if(is_float||is_unsigned!=-1)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			is_unsigned=1;
			break;
		case KT_signed:
			if(is_float||is_unsigned!=-1)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			is_unsigned=0;
			break;
		case KT_const:
			is_const|=1;
			break;
		case KT_void:
		case KT_char:
		case KT_int:
		case KT_float:
		case KT_double:
			if(nbytes)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			switch(t->keyword_type)
			{
			case KT_void:	nbytes=-1;break;
			case KT_char:	nbytes=1;break;
			case KT_int:	nbytes=4;break;
			case KT_float:	nbytes=4, is_float=1;break;
			case KT_double:	nbytes=8, is_float=1;break;
			}
			break;
		}
	}
	ASTNode *n=node_alloc(NT_TYPE);
	n->nbits=nbytes==-1?0:nbytes<<3;//-1 was void, so zero
	n->is_unsigned=is_unsigned==1;
	n->is_const=is_const;
	n->is_float=is_float;
	n->is_extern=is_extern;
	n->is_static=is_static;
	return n;
}
static ASTNode* parse_funcheader(Token *tokens, int *idx, int count)
{
	//funcheader := type (vardecl|funcdecl) [COMMA (vardecl|funcdecl) ]*

	//AST: funcheader -> type* -> ...

	Token *t;
	ASTNode *n_header=node_alloc(NT_FUNCHEADER);
	CHECKIDX(n_header)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR)//void
	{
		ASTNode *ch=node_alloc(NT_TYPE);//zero nbits means void
		node_append_ch(&n_header, ch);
		return n_header;
	}
	while(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
	{
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_ELLIPSIS)
		{
			ASTNode *n_name=node_alloc(NT_ARGDECL_ELLIPSIS);
			node_append_ch(&n_header, n_name);
			++*idx;

			CHECKIDX(n_header)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Ellipsis must terminate function header");
				return n_header;
			}
		}
		else
		{
			ASTNode *n_type=parse_type(tokens, idx, count);
			node_append_ch(&n_header, n_type);
			CHECKIDX(n_header)
			t=tokens+*idx;
			
			ASTNode *n_name=node_alloc(NT_ARGDECL);
			int indirection_count=0;
			while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
			{
				++indirection_count;
				++*idx;

				CHECKIDX(n_type)
				t=tokens+*idx;
			}
			n_name->indirection_count=indirection_count;
			if(t->type==TT_ID)
			{
				n_name->t=*t;
				++*idx;

				CHECKIDX(n_header)
				t=tokens+*idx;
			}
			if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//TODO funcpointer
			{
				//...
			}
			node_append_ch(n_header->ch+n_header->nch-1, n_name);
		}

		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA))
			break;
		++*idx;

		if(*idx>=count)
			break;
		t=tokens+*idx;
	}
	return n_header;
}

//precedence:
//	postfix		(highest prec)
//	prefix
//	multiplicative
//	additive
//	shift
//	inequality
//	equality
//	bitwise and
//	bitwise xor
//	bitwise or
//	logic and
//	logic or
//	ternary conditional
//	assignment
//	comma list	(lowest prec)
static int is_postfix_op(SymbolType t)
{
	switch(t)
	{
	case ST_INC://postfix increment
	case ST_DEC://postfix decrement
	case ST_LPR://function call
	case ST_LBRACKET://subscript
	case ST_PERIOD://struct member
	case ST_ARROW://pointer member
		return 1;
	}
	return 0;
}
static int is_prefix_op(Token *tokens, int idx, int count)
{
	Token *t;
	t=tokens+idx;
	if(t->type==TT_SYMBOL)
	{
		switch(t->symbol_type)
		{
		case ST_LPR://possible cast operator
			++idx;
			if(idx<count)
			{
				t=tokens+idx;
				if(t->type==TT_KEYWORD&&is_type(t->keyword_type))
					return 1;
			}
			break;
		case ST_INC://preinc
		case ST_DEC://predec
		case ST_PLUS://pos
		case ST_MINUS://neg
		case ST_EXCLAMATION://logic not
		case ST_TILDE://bitwise not
		case ST_ASTERISK://dereference
		case ST_AMPERSAND://address of
			return 1;
		}
	}
	return 0;
}
static ASTNode* parse_postfix(Token *tokens, int *idx, int count)//check if postfix before calling
{
	ASTNode *n_root=0;
	Token *t;
	t=tokens+*idx;
	switch(t->type)//handle main expression
	{
	case TT_SYMBOL:
		if(t->symbol_type==ST_LPR)
		{
			++*idx;

			CHECKIDX(0);
			n_root=parse_expr_commalist(tokens, idx, count);
			
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Expected \')\'");
				return n_root;
			}
			++*idx;
		}
		break;
	case TT_ID:
	case TT_STRING_LITERAL:
	case TT_CHAR_LITERAL:
	case TT_INT_LITERAL:
	case TT_FLOAT_LITERAL:
		n_root=node_alloc(NT_TOKEN);
		n_root->t=*t;
		++*idx;
		break;
	}
	if(!n_root)
	{
		COMPE(t, "Expected an expression");
		return 0;
	}

	//handle postfix
	CHECKIDX(n_root)
	t=tokens+*idx;
	while(t->type==TT_SYMBOL&&is_postfix_op(t->symbol_type))
	{
		ASTNode *n_post;
		switch(t->symbol_type)
		{
		case ST_INC://postfix increment
		case ST_DEC://postfix decrement
			n_post=node_alloc(t->symbol_type==ST_INC?NT_OP_POST_INC:NT_OP_POST_DEC);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;
			break;
		case ST_LPR://function call
		case ST_LBRACKET://subscript
			//id LBRACKET expr_commalist RBRACKET
			//id LPR expr_commalist RPR

			//AST: funccall -> func commalist
			//AST: subscript -> id commalist
			n_post=node_alloc(t->symbol_type==ST_LPR?NT_OP_FUNCCALL:NT_OP_SUBSCRIPT);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;

			CHECKIDX(n_root)
			n_post=parse_expr_commalist(tokens, idx, count);
			node_append_ch(&n_root, n_post);
				
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(n_root->type==NT_OP_FUNCCALL)
			{
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_root;
				}
			}
			else
			{
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACKET))
				{
					COMPE(t, "Expected \']\'");
					return n_root;
				}
			}
			++*idx;
			break;
		case ST_PERIOD://struct member
		case ST_ARROW://pointer member
			n_post=node_alloc(t->symbol_type==ST_PERIOD?NT_OP_MEMBER:NT_OP_ARROW);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;
			
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(t->type!=TT_ID)
			{
				COMPE(t, "Expected an identifier");
				return n_root;
			}
			n_post=node_alloc(NT_TOKEN);
			n_post->t=*t;
			node_append_ch(&n_root, n_post);
			++*idx;
			break;
		}
		CHECKIDX(n_root)
		t=tokens+*idx;
	}
	return n_root;
}
static ASTNode* parse_prefix(Token *tokens, int *idx, int count)
{
	ASTNode *n_root=0, **n_leaf=0;
	Token *t;
	t=tokens+*idx;
	while(is_prefix_op(tokens, *idx, count))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_LPR:
			{
				++*idx;
				parse_type(tokens, idx, count);
				ASTNode *n_pre=node_alloc(type);
				node_attach_ch(&n_root, &n_leaf, n_pre);
				
				CHECKIDX(n_root)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_root;
				}
				++*idx;
			}
			continue;
		case ST_INC:		type=NT_OP_PRE_INC;	break;
		case ST_DEC:		type=NT_OP_PRE_DEC;	break;
		case ST_PLUS:		type=NT_OP_POS;		break;
		case ST_MINUS:		type=NT_OP_NEG;		break;
		case ST_EXCLAMATION:	type=NT_OP_BITWISE_NOT;	break;
		case ST_TILDE:		type=NT_OP_LOGIC_NOT;	break;
		case ST_ASTERISK:	type=NT_OP_DEREFERENCE;	break;
		case ST_AMPERSAND:	type=NT_OP_ADDRESS_OF;	break;
		}
		ASTNode *n_pre=node_alloc(type);
		node_attach_ch(&n_root, &n_leaf, n_pre);
		++*idx;

		CHECKIDX(n_root)
		t=tokens+*idx;
	}
	ASTNode *n_post=parse_postfix(tokens, idx, count);
	if(!n_root)
		return n_post;
	node_append_ch(n_leaf, n_post);
	return n_root;
}
static ASTNode* parse_multiplicative(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_prefix(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_ASTERISK||t->symbol_type==ST_SLASH||t->symbol_type==ST_PERCENT))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_ASTERISK:	type=NT_OP_MUL;	break;
		case ST_SLASH:		type=NT_OP_DIV;	break;
		case ST_PERCENT:	type=NT_OP_MOD;	break;
		}
		ASTNode *n_or=node_alloc(type);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_prefix(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_ASTERISK||t->symbol_type==ST_SLASH||t->symbol_type==ST_PERCENT));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_additive(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_multiplicative(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_PLUS||t->symbol_type==ST_MINUS))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_PLUS?NT_OP_ADD:NT_OP_SUB);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_multiplicative(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_PLUS||t->symbol_type==ST_MINUS));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_shift(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_additive(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_SHIFT_LEFT||t->symbol_type==ST_SHIFT_RIGHT))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_SHIFT_LEFT?NT_OP_SHIFT_LEFT:NT_OP_SHIFT_RIGHT);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_additive(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_SHIFT_LEFT||t->symbol_type==ST_SHIFT_RIGHT));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_inequality(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_shift(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_LESS||t->symbol_type==ST_LESS_EQUAL||t->symbol_type==ST_GREATER||t->symbol_type==ST_GREATER_EQUAL))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_LESS:		type=NT_OP_LESS;		break;
		case ST_LESS_EQUAL:	type=NT_OP_LESS_EQUAL;		break;
		case ST_GREATER:	type=NT_OP_GREATER;		break;
		case ST_GREATER_EQUAL:	type=NT_OP_GREATER_EQUAL;	break;
		}
		ASTNode *n_or=node_alloc(type);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_shift(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_LESS||t->symbol_type==ST_LESS_EQUAL||t->symbol_type==ST_GREATER||t->symbol_type==ST_GREATER_EQUAL));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_equality(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_inequality(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_EQUAL||t->symbol_type==ST_NOT_EQUAL))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_EQUAL?NT_OP_EQUAL:NT_OP_NOT_EQUAL);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_inequality(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_EQUAL||t->symbol_type==ST_NOT_EQUAL));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_and(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_equality(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_AMPERSAND)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_AND);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_equality(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_AMPERSAND);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_xor(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_and(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_XOR)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_XOR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_and(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_XOR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_or(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_xor(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_OR)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_OR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_xor(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_OR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_logic_and(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_or(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_AND)
	{
		ASTNode *n_or=node_alloc(NT_OP_LOGIC_AND);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_or(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_AND);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_logic_or(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_logic_and(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_OR)
	{
		ASTNode *n_or=node_alloc(NT_OP_LOGIC_OR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_logic_and(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_OR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_expr_ternary(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_logic_or(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_QUESTION)
	{
		ASTNode *n_ternary=node_alloc(NT_OP_TERNARY);
		node_append_ch(&n_ternary, n_expr);//condition
		++*idx;

		CHECKIDX(n_ternary)
		n_expr=parse_expr_commalist(tokens, idx, count);
		node_append_ch(&n_ternary, n_expr);//expr_true

		CHECKIDX(n_ternary)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
		{
			COMPE(t, "Expected \':\'");
			return n_ternary;
		}
		++*idx;

		n_expr=parse_expr_ternary(tokens, idx, count);
		node_append_ch(&n_ternary, n_expr);//expr_false
		return n_ternary;
	}
	return n_expr;
}
static int is_assign_op(SymbolType t)
{
	switch(t)
	{
	case ST_ASSIGN:
	case ST_ASSIGN_ADD:
	case ST_ASSIGN_SUB:
	case ST_ASSIGN_MUL:
	case ST_ASSIGN_DIV:
	case ST_ASSIGN_MOD:
	case ST_ASSIGN_SHIFT_LEFT:
	case ST_ASSIGN_SHIFT_RIGHT:
	case ST_ASSIGN_AND:
	case ST_ASSIGN_XOR:
	case ST_ASSIGN_OR:
		return 1;
	}
	return 0;
}
static ASTNode* parse_expr_assign(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_expr_ternary(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&is_assign_op(t->symbol_type))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_ASSIGN:			type=NT_OP_ASSIGN;	break;
		case ST_ASSIGN_ADD:		type=NT_OP_ASSIGN_ADD;	break;
		case ST_ASSIGN_SUB:		type=NT_OP_ASSIGN_SUB;	break;
		case ST_ASSIGN_MUL:		type=NT_OP_ASSIGN_MUL;	break;
		case ST_ASSIGN_DIV:		type=NT_OP_ASSIGN_DIV;	break;
		case ST_ASSIGN_MOD:		type=NT_OP_ASSIGN_MOD;	break;
		case ST_ASSIGN_SHIFT_LEFT:	type=NT_OP_ASSIGN_SL;	break;
		case ST_ASSIGN_SHIFT_RIGHT:	type=NT_OP_ASSIGN_SR;	break;
		case ST_ASSIGN_AND:		type=NT_OP_ASSIGN_AND;	break;
		case ST_ASSIGN_XOR:		type=NT_OP_ASSIGN_XOR;	break;
		case ST_ASSIGN_OR:		type=NT_OP_ASSIGN_OR;	break;
		}
		ASTNode *n_assign=node_alloc(type);
		node_append_ch(&n_assign, n_expr);
		++*idx;
		n_expr=parse_expr_assign(tokens, idx, count);
		node_append_ch(&n_assign, n_expr);
		return n_assign;
	}
	return n_expr;
}
static ASTNode* parse_expr_commalist(Token *tokens, int *idx, int count)
{
	//precedence:
	//	postfix		(highest prec)
	//	prefix
	//	multiplicative
	//	additive
	//	shift
	//	inequality
	//	comparison
	//	bitwise and
	//	bitwise xor
	//	bitwise or
	//	logic and
	//	logic or
	//	ternary conditional
	//	assignment
	//	comma list	(lowest prec)

	//expr [COMMA expr]

	//AST: comma -> expr_assign*
	Token *t;
	ASTNode *n_expr=parse_expr_assign(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA)
	{
		ASTNode *n_comma=node_alloc(NT_OP_COMMA);
		node_append_ch(&n_comma, n_expr);
		do
		{
			++*idx;
			n_expr=parse_expr_assign(tokens, idx, count);
			node_append_ch(&n_comma, n_expr);

			CHECKIDX(n_comma)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA);
		return n_comma;
	}
	return n_expr;
}
static int is_typespecifier(Token *tokens, int idx, int count)
{
	Token *t=tokens+idx;
	if(t->type==TT_KEYWORD)
	{
		switch(t->keyword_type)
		{
		case KT_extern:
		case KT_static:
		case KT_const:
		case KT_signed:
		case KT_unsigned:
		case KT_void:
		case KT_char:
		case KT_int:
		case KT_long:
		case KT_float:
		case KT_double:
			return 1;
		}
	}
	return 0;
}
static ASTNode* parse_stmt_decl(Token *tokens, int *idx, int count)
{
	Token *t;
	if(is_typespecifier(tokens, *idx, count))
		return parse_declaration(tokens, idx, count, 0);
	ASTNode *n_expr=parse_expr_commalist(tokens, idx, count);
	
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
	{
		COMPE(t, "Expected \';\'");
		return n_expr;
	}
	++*idx;

	return n_expr;
}
static int is_label_togo(Token *tokens, int idx, int count)
{
	Token *t;
	if(idx>=count)
		return 0;
	t=tokens+idx;
	if(t->type!=TT_ID)
		return 0;
	++idx;
	if(idx>=count)
		return 0;
	t=tokens+idx;
	return t->type==TT_SYMBOL&&t->symbol_type==ST_COLON;
}
static ASTNode* parse_statement(Token *tokens, int *idx, int count)
{
	Token *t=tokens+*idx;
	if(t->type==TT_KEYWORD)
	{
		switch(t->keyword_type)
		{
		case KT_if:
			{
				//IF LPR condition RPR truebody [ELSE falsebody]
				
				//AST: stmt_if -> condition truebody [falsebody]
				ASTNode *n_if=node_alloc(NT_STMT_IF);
				++*idx;

				CHECKIDX(n_if)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_if;
				}
				++*idx;

				CHECKIDX(n_if)
				ASTNode *n_condition=parse_expr_commalist(tokens, idx, count);//if condition
				node_append_ch(&n_if, n_condition);
				
				CHECKIDX(n_if)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_if;
				}
				++*idx;

				CHECKIDX(n_if)
				t=tokens+*idx;
				ASTNode *n_truebody=parse_statement(tokens, idx, count);//if body
				node_append_ch(&n_if, n_truebody);

				CHECKIDX(n_if)
				t=tokens+*idx;
				if(t->type==TT_KEYWORD&&t->keyword_type==KT_else)//optional else with body
				{
					++*idx;
					CHECKIDX(n_if)
					t=tokens+*idx;
					ASTNode *n_falsebody=parse_statement(tokens, idx, count);//else body
					node_append_ch(&n_if, n_falsebody);
				}
				return n_if;
			}
			break;
		case KT_for:
			{
				//FOR LPR init_decl cond_expr SEMICOLON inc_expr RPR loopbody

				//AST: stmt_for -> init cond inc body
				ASTNode *n_for=node_alloc(NT_STMT_FOR);
				++*idx;

				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_init=parse_stmt_decl(tokens, idx, count);//for initialization
				node_append_ch(&n_for, n_init);

				CHECKIDX(n_for)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);//for condition
				node_append_ch(&n_for, n_cond);

				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_inc=parse_expr_commalist(tokens, idx, count);//for increment
				node_append_ch(&n_for, n_inc);
				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);//for body
				node_append_ch(&n_for, n_loopbody);

				return n_for;
			}
			break;
		case KT_do:
			{
				//DO loopbody WHILE LPR cond RPR SEMICOLON

				//AST: stms_doWhile -> body condition
				ASTNode *n_doWhile=node_alloc(NT_STMT_DO_WHILE);
				++*idx;
				CHECKIDX(n_doWhile)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);
				node_append_ch(&n_doWhile, n_loopbody);

				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_KEYWORD&&t->keyword_type==KT_while))
				{
					COMPE(t, "Expected \'while\'");
					return n_doWhile;
				}
				++*idx;
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_doWhile;
				}
				++*idx;

				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_doWhile, n_cond);
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_doWhile;
				}
				++*idx;
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_doWhile;
				}
				++*idx;
				return n_doWhile;
			}
			break;
		case KT_while:
			{
				//WHILE LPR condition RPR loopbody
				
				//AST: stmt_while -> condition body
				ASTNode *n_while=node_alloc(NT_STMT_WHILE);
				++*idx;
				
				CHECKIDX(n_while)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_while;
				}
				++*idx;

				CHECKIDX(n_while)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_while, n_cond);
				
				CHECKIDX(n_while)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_while;
				}
				++*idx;
				
				CHECKIDX(n_while)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);
				node_append_ch(&n_while, n_loopbody);
				return n_while;
			}
			break;
		case KT_continue:
			{
				//CONTINUE SEMICOLON

				//AST: continue
				ASTNode *n_continue=node_alloc(NT_STMT_CONTINUE);
				++*idx;
				
				CHECKIDX(n_continue)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_continue;
				}
				++*idx;
				return n_continue;
			}
			break;
		case KT_break:
			{
				//BREAK SEMICOLON

				//AST: break
				ASTNode *n_break=node_alloc(NT_STMT_CONTINUE);
				++*idx;
				
				CHECKIDX(n_break)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_break;
				}
				++*idx;
				return n_break;
			}
			break;
		case KT_switch:
			{
				//Node: code without case/default labels is valid but never gets executed
				//to declare variables, use braces

				//SWITCH LPR cond RPR LBRACE [(CASE expr)|DEFAULT COLON code]+ RBRACE

				//AST: switch -> cond cases*
				ASTNode *n_switch=node_alloc(NT_STMT_SWITCH);
				++*idx;
				
				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_switch, n_cond);

				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LBRACE))
				{
					COMPE(t, "Expected \'{\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				ASTNode *n_body=parse_code(tokens, idx, count);

				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE))
				{
					COMPE(t, "Expected \'}\'");
					return n_switch;
				}
				++*idx;
				return n_switch;
			}
			break;
		case KT_case:
			{
				//CASE expr COLON

				//AST: case ->expr
				ASTNode *n_case=node_alloc(NT_LABEL_CASE);
				++*idx;

				CHECKIDX(n_case)
				ASTNode *n_body=parse_expr_ternary(tokens, idx, count);
				
				CHECKIDX(n_case)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
				{
					COMPE(t, "Expected \':\'");
					return n_case;
				}
				++*idx;

				return n_case;
			}
			break;
		case KT_default:
			{
				//DEFAULT COLON
				ASTNode *n_default=node_alloc(NT_LABEL_DEFAULT);
				++*idx;

				CHECKIDX(n_default)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
				{
					COMPE(t, "Expected \':\'");
					return n_default;
				}
				++*idx;

				return n_default;
			}
			break;
		case KT_goto:
			{
				//GOTO identifier SEMICOLON

				//AST: goto -> id
				ASTNode *n_goto=node_alloc(NT_STMT_GOTO);
				++*idx;

				CHECKIDX(n_goto)
				t=tokens+*idx;
				if(t->type!=TT_ID)
				{
					COMPE(t, "Expected an identifier");
					return n_goto;
				}
				ASTNode *n_label=node_alloc(NT_TOKEN);
				n_label->t=*t;
				++*idx;
				
				CHECKIDX(n_goto)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_goto;
				}
				++*idx;
				return n_goto;
			}
			break;
		case KT_return:
			{
				//RETURN commalist SEMICOLON

				//AST: return -> expr
				ASTNode *n_ret=node_alloc(NT_STMT_RETURN);
				++*idx;

				ASTNode *n_expr=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_ret, n_expr);
				
				CHECKIDX(n_ret)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_ret;
				}
				++*idx;

				return n_ret;
			}
			break;
		default://can be decl_expr
			return parse_stmt_decl(tokens, idx, count);
		}
	}
	else if(t->type==TT_SYMBOL)
	{
		switch(t->symbol_type)
		{
		case ST_LBRACE:
			{
				ASTNode *n_scope=node_alloc(NT_SCOPE);
				++*idx;

				CHECKIDX(n_scope)
				ASTNode *n_code=parse_code(tokens, idx, count);
				node_append_ch(&n_scope, n_code);

				CHECKIDX(n_scope)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_scope;
				}
				++*idx;

				return n_scope;
			}
			break;
		default://can be decl_expr
			return parse_stmt_decl(tokens, idx, count);
		}
	}
	else if(t->type==TT_ID&&is_label_togo(tokens, *idx, count))
	{
		//id COLON

		//AST: LABEL_TOGO -> id
		ASTNode *n_label=node_alloc(NT_LABEL_TOGO);
		ASTNode *n_id=node_alloc(NT_TOKEN);
		n_id->t=*t;
		++*idx;

		CHECKIDX(n_label)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
		{
			COMPE(t, "Expected \'(\'");
			return n_label;
		}
		++*idx;
		return n_label;
	}
	return parse_stmt_decl(tokens, idx, count);
}
static ASTNode* parse_code(Token *tokens, int *idx, int count)
{
	ASTNode *n_code=node_alloc(NT_FUNCBODY);
	while(*idx<count)
	{
		Token *t=tokens+*idx;
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE)
			break;
		int idx0=*idx;
		ASTNode *n_decl=parse_statement(tokens, idx, count);
		if(n_decl)
			node_append_ch(&n_code, n_decl);
		if(idx==idx0)
		{
			COMPE(tokens+*idx, "INTERNAL ERROR: PARSER STUCK");
			++*idx;
		}
	}
	return n_code;
}
static ASTNode* parse_declaration(Token *tokens, int *idx, int count, int global)
{
	//declaration :=
	//	type id LPR funcheader RPR LBRACE CODE BBRACE
	//	type (vardecl|funcdecl) [COMMA (vardecl|funcdecl) ]* SEMICOLON

	//AST: type -> funcdef | declaration*		TODO func ptr
	
	short indirection_count=0;
	Token *t;
	ASTNode *n_type=parse_type(tokens, idx, count);

	CHECKIDX(n_type)
	t=tokens+*idx;
	while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
	{
		++indirection_count;
		++*idx;

		CHECKIDX(n_type)
		t=tokens+*idx;
	}
	if(t->type!=TT_ID)
	{
		COMPE(t, "Expected an identifier");
		return n_type;
	}
	ASTNode *n_name=node_alloc(NT_TOKEN);
	n_name->indirection_count=indirection_count;
	n_name->t=*t;
	node_append_ch(&n_type, n_name);
	++*idx;
	CHECKIDX(n_type)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//function header
	{
		++*idx;
		CHECKIDX(n_type)
		ASTNode *n_header=parse_funcheader(tokens, idx, count);
		node_append_ch(n_type->ch+n_type->nch-1, n_header);
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
		{
			COMPE(t, "Expeted \')\'");
			return n_type;
		}
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(global&&t->type==TT_SYMBOL&&t->symbol_type==ST_LBRACE)//funcbody
		{
			n_type->indirection_count=n_name->indirection_count;//move indirection_count from funcname to return_type
			n_name->indirection_count=0;
			++*idx;

			CHECKIDX(n_type)
			ASTNode *n_body=parse_code(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_body);

			CHECKIDX(n_type)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE))
				COMPE(t, "Expected \'}\'");
			++*idx;
			return n_type;
		}
	}
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_ASSIGN)//initialization
	{
		++*idx;
		CHECKIDX(n_type)
		ASTNode *n_init=parse_expr_ternary(tokens, idx, count);
		node_append_ch(n_type->ch+n_type->nch-1, n_init);

		CHECKIDX(n_type)
		t=tokens+*idx;
	}
	while(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA)
	{
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
		{
			++indirection_count;
			++*idx;

			CHECKIDX(n_type)
			t=tokens+*idx;
		}
		if(t->type!=TT_ID)
		{
			COMPE(t, "Expected an identifier");
			break;
		}
		ASTNode *n_name=node_alloc(NT_TOKEN);
		n_name->indirection_count=indirection_count;
		n_name->t=*t;
		node_append_ch(&n_type, n_name);
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//function header
		{
			++*idx;
			ASTNode *n_header=parse_funcheader(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_header);
			CHECKIDX(n_type)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Expeted \')\'");
				return n_type;
			}
			++*idx;
			CHECKIDX(n_type)
			t=tokens+*idx;
		}
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_ASSIGN)//initialization
		{
			++*idx;
			CHECKIDX(n_type)
			ASTNode *n_init=parse_expr_ternary(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_init);
		}
	}
	if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
	{
		COMPE(t, "Expeted \';\'");
		return n_type;
	}
	++*idx;
	return n_type;
}
ASTNode* parse_program(Token *tokens, int count)//program := declaration*
{
	ASTNode *n_prog=node_alloc(NT_PROGRAM);
	for(int idx=0;idx<count;)
	{
		int idx0=idx;
		ASTNode *n_decl=parse_declaration(tokens, &idx, count, 1);
		if(n_decl)
			node_append_ch(&n_prog, n_decl);
		if(idx==idx0)
		{
			COMPE(tokens+idx, "INTERNAL ERROR: PARSER STUCK");
			++idx;
		}
	}
	return n_prog;
}

void print_ast(ASTNode *root, int depth)
{
	if(!root)
		return;
	printf("%*s@", depth<<1, "");//indentation
	switch(root->type)
	{
	case NT_TOKEN:
		for(int k=0;k<root->indirection_count;++k)
			printf("*");
		print_token(&root->t);
		break;
	case NT_PROGRAM:
		printf("program");
		break;
	case NT_TYPE:
		if(root->is_extern)
			printf("extern ");
		if(root->is_static)
			printf("static ");
		if(root->is_const)
			printf("const ");
		if(root->is_float)
		{
			if(root->nbits==32)
				printf("float");
			else
				printf("double");
		}
		else
		{
			if(root->is_unsigned)
				printf("unsigned ");
			switch(root->nbits)
			{
			case  0:printf("void");break;
			case  8:printf("char");break;
			case 16:printf("short");break;
			case 32:printf("int");break;
			case 64:printf("long");break;
			default:printf("bitfield:%d", root->nbits);break;
			}
		}
		break;
	case NT_FUNCHEADER:
		printf("funcheader");
		break;
	case NT_ARGDECL:
		for(int k=0;k<root->indirection_count;++k)
			printf("*");
		print_token(&root->t);
		printf(" (funcarg)");
		break;
	case NT_ARGDECL_ELLIPSIS:
		printf("... (ellipsis_arg)");
		break;
	case NT_FUNCBODY:
		printf("funcbody");
		break;
	case NT_SCOPE:
		printf("scope{}");
		break;
	case NT_STMT_IF:
		printf("if");
		break;
	case NT_STMT_FOR:
		printf("for");
		break;
	case NT_STMT_DO_WHILE:
		printf("doWhile");
		break;
	case NT_STMT_WHILE:
		printf("while");
		break;
	case NT_STMT_CONTINUE:
		printf("continue");
		break;
	case NT_STMT_BREAK:
		printf("break");
		break;
	case NT_STMT_SWITCH:
		printf("switch");
		break;
	case NT_LABEL_CASE:
		printf("case");
		break;
	case NT_LABEL_DEFAULT:
		printf("default");
		break;
	case NT_STMT_GOTO:
		printf("goto");
		break;
	case NT_LABEL_TOGO:
		printf("label:");
		break;
	case NT_STMT_RETURN:
		printf("return");
		break;
	case NT_OP_ARROW:
		printf("->");
		break;
	case NT_OP_MEMBER:
		printf(". (member)");
		break;
	case NT_OP_SUBSCRIPT:
		printf("[]");
		break;
	case NT_OP_FUNCCALL:
		printf("funccall");
		break;
	case NT_OP_POST_INC:
		printf("post++");
		break;
	case NT_OP_POST_DEC:
		printf("post--");
		break;
	case NT_OP_ADDRESS_OF:
		printf("&address_of");
		break;
	case NT_OP_DEREFERENCE:
		printf("*dereference");
		break;
	case NT_OP_CAST:
		printf("(cast)...");
		break;
	case NT_OP_LOGIC_NOT:
		printf("!logic_not");
		break;
	case NT_OP_BITWISE_NOT:
		printf("~bitwise_not");
		break;
	case NT_OP_POS:
		printf("+pos");
		break;
	case NT_OP_NEG:
		printf("-neg");
		break;
	case NT_OP_PRE_INC:
		printf("pre++");
		break;
	case NT_OP_PRE_DEC:
		printf("pre--");
		break;
	case NT_OP_MUL:
		printf("a*b (mul)");
		break;
	case NT_OP_DIV:
		printf("a/b (div)");
		break;
	case NT_OP_MOD:
		printf("a%%b (mod)");
		break;
	case NT_OP_ADD:
		printf("a+b (add)");
		break;
	case NT_OP_SUB:
		printf("a-b (sub)");
		break;
	case NT_OP_SHIFT_LEFT:
		printf("a<<b (shift_left)");
		break;
	case NT_OP_SHIFT_RIGHT:
		printf("a>>b (shift_right)");
		break;
	case NT_OP_LESS:
		printf("a<b (less)");
		break;
	case NT_OP_LESS_EQUAL:
		printf("a<=b (less_equal)");
		break;
	case NT_OP_GREATER:
		printf("a>b (greater)");
		break;
	case NT_OP_GREATER_EQUAL:
		printf("a>=b (greater_equal)");
		break;
	case NT_OP_EQUAL:
		printf("a==b (equal)");
		break;
	case NT_OP_NOT_EQUAL:
		printf("a!=b (not_equal)");
		break;
	case NT_OP_BITWISE_AND:
		printf("a&b (bitwise_and)");
		break;
	case NT_OP_BITWISE_XOR:
		printf("a^b (bitwise_xor)");
		break;
	case NT_OP_BITWISE_OR:
		printf("a|b (bitwise_or)");
		break;
	case NT_OP_LOGIC_AND:
		printf("a&&b (logic_and)");
		break;
	case NT_OP_LOGIC_OR:
		printf("a||b (logic_or)");
		break;
	case NT_OP_TERNARY:
		printf("a?b:c (ternary)");
		break;
	case NT_OP_ASSIGN:
		printf("a=b (assign)");
		break;
	case NT_OP_ASSIGN_ADD:
		printf("a+=b (assign_add)");
		break;
	case NT_OP_ASSIGN_SUB:
		printf("a-=b (assign_sub)");
		break;
	case NT_OP_ASSIGN_MUL:
		printf("a*=b (assign_mul)");
		break;
	case NT_OP_ASSIGN_DIV:
		printf("a/=b (assign_div)");
		break;
	case NT_OP_ASSIGN_MOD:
		printf("a%%=b (assign_mod)");
		break;
	case NT_OP_ASSIGN_SL:
		printf("a<<=b (assign_sl)");
		break;
	case NT_OP_ASSIGN_SR:
		printf("a>>=b (assign_sr)");
		break;
	case NT_OP_ASSIGN_AND:
		printf("a&=b (assign_and)");
		break;
	case NT_OP_ASSIGN_XOR:
		printf("a^=b (assign_xor)");
		break;
	case NT_OP_ASSIGN_OR:
		printf("a|=b (assign_or)");
		break;
	case NT_OP_COMMA:
		printf("a,b (comma)");
		break;
	default:
		printf("UNRECOGNIZED NODETYPE: %d", root->type);
		break;
	}
	printf("\n");
	++depth;
	for(int k=0;k<root->nch;++k)
		print_ast(root->ch[k], depth);
}

int main(int argc, char **argv)
{
	printf("ACC2\n");
#ifndef _DEBUG
	if(argc!=2)
	{
		printf("Usage: %s input.c\n", argv[0]);
		return 1;
	}
	const char *fn=argv[1];
#else
	const char *fn="C:/Projects/acc/input.c";
#endif
	ArrayHandle text=load_file(fn, 0, 0, 1);

	printf("\nLexing %s\n", fn);
	ArrayHandle tokens=lex((char*)text->data, (int)text->count);
	printf("Lex result:\n");//
	print_tokens((Token*)tokens->data, (int)tokens->count);//

	printf("\nParsing...\n");
	ASTNode *ast=parse_program((Token*)tokens->data, (int)tokens->count);
	printf("Parse result (AST):\n");//
	print_ast(ast, 0);//
	printf("\n");

	node_free(&ast);
	array_free(&tokens);
	array_free(&text);
#ifdef _DEBUG
	printf("Done.\n");
	pause();
#endif
	return 0;
}
