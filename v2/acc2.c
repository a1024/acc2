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

	TT_PROGRAM,
} TokenType;

#define KEYWORDLIST\
	KEYWORD(extern, 6)\
	KEYWORD(static, 6)\
	KEYWORD(const, 5)\
	KEYWORD(char, 4)\
	KEYWORD(int, 3)\
	KEYWORD(float, 5)\
	KEYWORD(double, 6)\
	KEYWORD(return, 6)
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

#define SYMBOLLIST\
	SYMBOL("{", 1, ST_LBRACE)\
	SYMBOL("}", 1, ST_RBRACE)\
	SYMBOL("(", 1, ST_LPR)\
	SYMBOL(")", 1, ST_RPR)\
	SYMBOL("=", 1, ST_ASSIGN)\
	SYMBOL("*", 1, ST_ASTERISK)\
	SYMBOL("...", 3, ST_ELLIPSIS)\
	SYMBOL(".", 1, ST_PERIOD)\
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
				if(match(text, k, len, keyword_str[k2]))
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
void print_tokens(Token *tokens, int count)
{
	for(int k=0;k<count;++k)
	{
		Token *t=tokens+k;
		if(k&&t->lineno>tokens[k-1].lineno)
			printf("\n");
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
				ArrayHandle t2=str2esc(t->val_str, strlen(t->val_str));
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
	printf("(%d) %s: ", t->lineno, levelstr);
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
typedef struct ASTNodeStruct
{
	Token t;
	int nbits;
	char is_const, is_float;
	int nch;
	struct ASTNodeStruct *ch[];
} ASTNode;
static ASTNode* node_alloc(TokenType type)
{
	ASTNode *n=(ASTNode*)malloc(sizeof(ASTNode));
	if(!n)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	memset(n, 0, sizeof(ASTNode));
	n->t.type=type;
	return n;
}
static int node_append_ch(ASTNode **n, ASTNode *t)
{
	int nch=n[0]->nch+1;
	void *p=realloc(*n, sizeof(ASTNode)+nch*sizeof(void*));
	if(!p)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	*n=(ASTNode*)p;
	n[0]->ch[n[0]->nch]=t;
	n[0]->nch=nch;
	return 1;
}

static ASTNode* parse_type(Token *tokens, int *idx, int count)
{
}
static ASTNode* parse_declaration(Token *tokens, int *idx, int count)
{
	//declaration :=
	//	type id LPR funcheader RPR LBRACE CODE BBRACE
	//	type (vardecl|funcdecl) [COMMA (vardecl|funcdecl) ]* SEMICOLON

	//AST: type -> func | declaration*
	Token *t;
	if(*idx>=count)
		return 0;

	int is_const=0, nbytes=0, is_float=0, invalid=0;
	t=tokens+*idx;
	for(;*idx<count&&t->type==TT_KEYWORD;++*idx, t=tokens+*idx)
	{
		switch(t->keyword_type)
		{
		case KT_const:
			is_const|=1;
			break;
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
			case KT_char:	nbytes=1;break;
			case KT_int:	nbytes=4;break;
			case KT_float:	nbytes=4, is_float=1;break;
			case KT_double:	nbytes=8, is_float=1;break;
			}
			break;
		}
	}
	ASTNode *root;
}
ASTNode* parse_program(Token *tokens, int count)//program := declaration*
{
	ASTNode *root=node_alloc(TT_PROGRAM);
	for(int idx=0;idx<count;)
	{
		ASTNode *n=parse_declaration(tokens, &idx, count);
		if(n)
			node_append_ch(&root, n);
	}
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
	ArrayHandle tokens=lex((char*)text->data, (int)text->count);
	print_tokens((Token*)tokens->data, (int)tokens->count);//
	
	array_free(&tokens);
	array_free(&text);
#ifdef _DEBUG
	printf("Done.\n");
	pause();
#endif
	return 0;
}
