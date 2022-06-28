#include	"acc.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<math.h>
static const char file[]=__FILE__;

//	#define	PRINT_LEX

const char *str_va_args=0, *str_pragma_once=0;
const char *currentfilename=0, *currentfile=0;
void		lex_error(const char *text, int len, int pos, const char *format, ...);

//string library
Map			strlib={0};
static CmpRes strlib_cmp(const void *left, const void *right)
{
	const char *s1=*(const char**)left, *s2=*(const char**)right;
	int result=strcmp(s1, s2);
	return (result>0)-(result<0);
}
char*		strlib_insert(const char *str, int len)
{
	char *key;
	int found;
	BSTNodeHandle *node;

	if(!strlib.cmp_key)
		map_init(&strlib, sizeof(char*), 0, strlib_cmp);

	node=MAP_INSERT(&strlib, &str, 0, &found);		//search first then allocate
	if(!found)
	{
		if(!len)
			len=strlen(str);
		key=(char*)malloc(len+1);
		memcpy(key, str, len);
		key[len]=0;

		*(char**)node[0]->data=key;
	}
	return *(char**)node[0]->data;
}
static void	strlib_printcallback(BSTNodeHandle *node, int depth)
{
	char *str=*(char**)node[0]->data;
	printf("%4d %*s%s\n", depth, depth, "", str);
}
void		strlib_debugprint()
{
	MAP_DEBUGPRINT(&strlib, strlib_printcallback);
	printf("\n");
}


//string manipulation

typedef enum NumberTypeEnum
{
	NUM_I32,//no postfix && can fit in 2's complement 32 bits
	NUM_U32,//has postfix U
	NUM_I64,//has postfix LL || from 2^31 to 2^63
	NUM_U64,//has postfix ULL || from 2^31 to 2^64
	NUM_F32,//has point and postfix F
	NUM_F64,//has point
} NumberType;
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
static int	acme_read_number_suffix(const char *text, int len, int *idx, Number *ret)
{
	int start=*idx;
	char U=0, L=0, LL=0, Z=0, F=0;
	int success=1;
	
	for(;*idx<len;)
	{
		switch(text[*idx]&0xDF)
		{
		case 'F':
			if(F)
			{
				lex_error(text, len, start, "Invalid number literal suffix");
				success=0;
				break;
			}
			F=1;
			++*idx;
			continue;
		case 'L':
			if((text[*idx+1]&0xDF)=='L')
			{
				if(LL||L)
				{
					lex_error(text, len, start, "Invalid number literal suffix");
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
					lex_error(text, len, start, "Invalid number literal suffix");
					success=0;
					break;
				}
				L=1;
				++*idx;
			}
			continue;
		case 'U':
			if(U)
			{
				lex_error(text, len, start, "Invalid number literal suffix");
				success=0;
				break;
			}
			U=1;
			++*idx;
			continue;
		case 'Z'://ptrdiff_t/size_t
			if(Z)
			{
				lex_error(text, len, start, "Invalid number literal suffix");
				success=0;
				break;
			}
			Z=1;
			++*idx;
			continue;
		default:
			if(isalnum(text[*idx])||text[*idx]=='_')
			{
				lex_error(text, len, start, "Invalid number literal suffix");
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
			lex_error(text, len, start, "Invalid number literal suffix");
			success=0;
			break;
		}
		ret->type=NUM_F32;
	}
	else
	{
		if(ret->type==NUM_F32||ret->type==NUM_F64)
		{
			if(U||LL||L||Z)
			{
				lex_error(text, len, start, "Invalid number literal suffix");
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
int			acme_read_number(const char *text, int len, int *idx, Number *ret)
{
	unsigned long long tail;
	int start, success, ndigits;

	success=1;
	start=*idx;
	ret->type=NUM_I32;
	if(text[*idx]=='0')
	{
		switch(text[*idx+1]&0xDF)
		{
		case 'B':
			*idx+=2;
			ret->base=BASE2;
			break;
		case 'X':
			*idx+=2;
			ret->base=BASE16;
			break;
		default:
			ret->base=BASE8;
			break;
		}
	}
	else
		ret->base=BASE10;
	
	switch(ret->base)
	{
	case BASE2:case BASE8:case BASE16:
		ret->u64=acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
		if(text[*idx]=='.')
		{
			++*idx;
			ret->type=NUM_F64;
			tail=acme_read_int_basePOT(text, len, idx, ret->base, &ndigits);
			ret->f64=(double)ret->u64+tail*_10pow(-ndigits);
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
		if(text[*idx]=='.')
		{
			++*idx;
			ret->type=NUM_F64;
			tail=acme_read_int_base10(text, len, idx, &ndigits);
			ret->f64=ret->u64+tail*_10pow(-ndigits);
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

//UTF-8
int					codepoint2utf8(int codepoint, char *out)//returns sequence length (up to 4 characters)
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
int					utf8codepoint(const char *in, int *codepoint)//returns sequence length
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
char*				esc2str(const char *s, int size, int *ret_len)
{
	char *ret;
	int ret_size;
	char utf8_seq[5]={0};
	if(!s)
	{
		printf("Internal error: esc2str: string is null.\n");
		return 0;
	}
	ret_size=size+1;
	ret=(char*)malloc(ret_size);
	int kd=0;
	for(int ks=0;ks<size;++ks, ++kd)
	{
		if(s[ks]!='\\')
			ret[kd]=s[ks];
		else
		{
			++ks;
			switch(s[ks])
			{
			case 'a':	ret[kd]='\a';break;
			case 'b':	ret[kd]='\b';break;
			case 'f':	ret[kd]='\f';break;
			case 'n':	ret[kd]='\n';break;
			case 'r':	ret[kd]='\r';break;
			case 't':	ret[kd]='\t';break;
			case 'v':	ret[kd]='\v';break;
			case '\'':	ret[kd]='\'';break;
			case '\"':	ret[kd]='\"';break;
			case '\\':	ret[kd]='\\';break;
			case '\?':	ret[kd]='?';break;
			case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':	//case '8':case '9'://dec codepoint	SHOULD BE OCTAL
			case 'x':case 'u'://hex codepoint
				{
					int hex=s[ks]=='x'||s[ks]=='u';
					ks+=hex;
					int end=ks, ndigits=0;
					int val=(int)acme_read_int_basePOT(s, size, &end, hex?BASE16:BASE8, &ndigits);
				//	int val=acme_read_integer(s, size, hex?16:8, ks, &end);
					int seqlen=codepoint2utf8((int)val, utf8_seq);

					ret_size+=seqlen-1;
					ret=(char*)realloc(ret, ret_size);
					//out.resize(out.size()+seqlen-1);
					memcpy(ret+kd, utf8_seq, seqlen);
					ks=end, kd+=seqlen-1;
				}
				break;
			case '\n'://escaped newline
				break;
			default:
				ret[kd]='?';
				printf("Unsupported escape sequence at %d: \\%c\n", ks, s[ks]);
				break;
			}
		}
	}
	ret_size=kd+1;
	ret=(char*)realloc(ret, ret_size);
	ret[kd]='\0';

	if(ret_len)
		*ret_len=kd;
	return ret;
}
//raw string -> escape sequences
char*				str2esc(const char *s, int size, int *ret_len)
{
	char *ret;
	int ret_size;
	if(!s)
	{
		printf("Internal error: str2esc: string is null.\n");
		return 0;
	}
	if(!size)
		size=strlen(s);
	char codestr[16];

	ret_size=size+1;
	ret=(char*)malloc(ret_size);
	//out.resize(size);

	int kd=0;
	for(int ks=0;ks<size;++ks, ++kd)
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
			++ret_size;
			ret=(char*)realloc(ret, ret_size);
			//out.resize(out.size()+1);

			ret[kd]='\\';
			switch(c)
			{
			case '\0':ret[kd+1]='0';break;
			case '\a':ret[kd+1]='a';break;
			case '\b':ret[kd+1]='b';break;
			case '\f':ret[kd+1]='f';break;
			case '\n':ret[kd+1]='n';break;
			case '\r':ret[kd+1]='r';break;
			case '\t':ret[kd+1]='t';break;
			case '\v':ret[kd+1]='v';break;
			case '\'':ret[kd+1]='\'';break;
			case '\"':ret[kd+1]='\"';break;
			case '\\':ret[kd+1]='\\';break;
			}
			++kd;
			break;
		default:
			if(c>=' '&&c<0x7F)
				ret[kd]=s[ks];
			else
			{
				int codepoint=0;
				int seqlen=utf8codepoint(s+ks, &codepoint);
				int codelen=sprintf_s(codestr, 16, "\\u%04X", codepoint);

				ret_size+=codelen-1;
				ret=(char*)realloc(ret, ret_size);
				memcpy(ret+kd, codestr, codelen);
				//out.replace(kd, 1, codestr, codelen);

				ks+=seqlen-1, kd+=codelen-1;
			}
			break;
		}
	}
	ret=(char*)realloc(ret, ret_size+1);
	ret[ret_size]='\0';

	if(ret_len)
		*ret_len=kd;
	return ret;
}

const char*			describe_char(char c)//uses g_buf
{
	static int idx=0;
	switch(c)
	{
	case '\0':
		return "<0: null terminator>";
	case '\a':
		return "<7: alert>";
	case '\b':
		return "<8: backspace>";
	case '\t':
		return "<9: tab>";
	case '\n':
		return "<10: newline>";
	case '\r':
		return "<13: carriage return>";
	case ' ':
		return "<32: space>";
	case 127:
		return "<127: delete>";
	default:
		{
			int i0=idx;
			if(idx+2>=G_BUF_SIZE)
				idx=0;
			if(c<32)
				idx+=sprintf_s(g_buf+idx, G_BUF_SIZE-idx, "\'\\u%04X\'", c&0xFF);
			else
				idx+=sprintf_s(g_buf+idx, G_BUF_SIZE-idx, "%c", c);
			++idx;
			return g_buf+i0;
		}
	}
}
const char*			lex_tokentype2str(CTokenType tokentype)
{
	const char *a=keywords[tokentype];
	if(a)
		return a;
	switch(tokentype)
	{
#define		TOKEN(STRING, LABEL)	case LABEL:return #LABEL;
#include	"acc_keywords.h"
#undef		TOKEN
	}
	return "<UNDEFINED>";
}
void				lex_token2buf(Token *token)
{
	switch(token->type)
	{
	case T_VAL_I32:
		sprintf_s(g_buf, G_BUF_SIZE, "%lld", token->i);
		break;
	case T_VAL_U32:
		sprintf_s(g_buf, G_BUF_SIZE, "%lluU", token->i);
		break;
	case T_VAL_I64:
		sprintf_s(g_buf, G_BUF_SIZE, "%lluLL", token->i);
		break;
	case T_VAL_U64:
		sprintf_s(g_buf, G_BUF_SIZE, "%lluULL", token->i);
		break;
	case T_VAL_F32:
		sprintf_s(g_buf, G_BUF_SIZE, "%f", token->f32);
		break;
	case T_VAL_F64:
		sprintf_s(g_buf, G_BUF_SIZE, "%g", token->f64);
		break;
	case T_ID:
		if(token->str)
			sprintf_s(g_buf, G_BUF_SIZE, "%s", token->str);
		else
			sprintf_s(g_buf, G_BUF_SIZE, "%s:nullptr", lex_tokentype2str(token->type));
		break;
	case T_VAL_C8:
		{
			char *proc;
			int proc_len;
			char str[9]={0};
			memcpy(str, &token->i, 8);
			int len=strlen(str);
			if(len>1)
				memreverse(str, len, 1);
			proc=str2esc(str, maximum(len, 1), &proc_len);
			sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", proc);
			free(proc);
		}
		break;
	case T_VAL_C32:
		{
			int codepoints[2]={0};
			char utf8[9]={0};
			int ulen=0;
			memcpy(codepoints, &token->i, 8);
			int len=1+(codepoints[1]!=0);
			if(len>1)
				memreverse(codepoints, 2, 4);
			for(int k2=0;k2<len;++k2)
			{
				int c=codepoints[k2];
				ulen+=codepoint2utf8(c, utf8+ulen);
			}
			int proc_len=0;
			char *proc=str2esc(utf8, ulen, &proc_len);
			sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", proc);
			free(proc);
		}
		break;
	case T_VAL_STR:
		if(!token->str)
			sprintf_s(g_buf, G_BUF_SIZE, "%s:nullptr", lex_tokentype2str(token->type));
		else
		{
			//std::string processed;
			int len=0;
			char *proc=str2esc(token->str, strlen(token->str), &len);
			sprintf_s(g_buf, G_BUF_SIZE, "\"%s\"", proc);
			free(proc);
		}
		break;
	case T_VAL_WSTR:
		//TODO
		break;
	case T_INCLUDENAME_STD:
		if(token->str)
			sprintf_s(g_buf, G_BUF_SIZE, "<%s>", token->str);
		else
			sprintf_s(g_buf, G_BUF_SIZE, "%s:nullptr", lex_tokentype2str(token->type));
		break;
	default:
		sprintf_s(g_buf, G_BUF_SIZE, "%s", lex_tokentype2str(token->type));
		break;
	}
}


//lexer
const char		*keywords[]=//order corresponds to enum CTokenType
{
#define			TOKEN(STRING, LABEL)	STRING,
#include		"acc_keywords.h"
#undef			TOKEN
};
typedef union Reg128Union
{
	unsigned long long val[2];
	unsigned char text[16];
} Reg128;
typedef struct LexOptionStruct
{
	Reg128 val, mask;
	int token;
	short len;//keyword length <= 16
	union
	{
		short flags;//endswithnd<<1|endswithnan
		struct
		{
			short
				endswithnan:1,	//followed by non-alphanumeric
				endswithnd:1;	//followed by a non-digit
		};
	};
	const char *name;
} LexOption;
static ArrayHandle *lex_slots=0;
static int lex_slot_less(const void *p1, const void *p2)
{
	int k;
	const char
		*s1=((LexOption*)p1)->name,
		*s2=((LexOption*)p2)->name;

	for(k=0;*s1&&*s2&&*s1==*s2;++k, ++s1, ++s2);
	if(!*s1&&!*s2)
		return 0;

	char ac=s1[k], bc=s2[k];
	if(!ac)
		ac=127;
	if(!bc)
		bc=127;
	return ac<bc;
}
static void	lexer_init()
{
	LexOption opt;

	lex_slots=(ArrayHandle*)malloc(256*sizeof(void*));
	memset(lex_slots, 0, 256*sizeof(void*));
	for(int kt=0;kt<T_NTOKENS;++kt)
	{
		const char *kw=keywords[kt];
		if(kw)
		{
			ArrayHandle *slot=lex_slots+(unsigned char)kw[0];

			memset(&opt, 0, sizeof(opt));
			opt.token=kt;
			opt.len=strlen(kw);
			memcpy(opt.val.text, kw, opt.len);
			for(int k2=0;k2<opt.len;++k2)
				opt.mask.text[k2]=0xFF;
			opt.endswithnan=isalnum(kw[0])||kw[0]=='_';
			if(kt==T_DQUOTE_WIDE)
				opt.endswithnan=0;
#ifdef ACC_CPP
			if(kt==T_DQUOTE_RAW)
				opt.endswithnan=0;
#endif
			opt.endswithnd=opt.len==1&&kw[0]=='.';//only the '.' operator should end with a non-digit
			opt.name=kw;

			if(!*slot)
				ARRAY_ALLOC(LexOption, *slot, 0, 0);
			ARRAY_APPEND(*slot, &opt, 1, 1, 0);
		}
	}
	for(int ks=0;ks<128;++ks)
	{
		ArrayHandle *slot=lex_slots+ks;
		if(*slot)
			qsort(slot[0]->data, slot[0]->count, slot[0]->esize, lex_slot_less);
	}
}

int			get_lineno(const char *text, int pos, int *linestart)
{
	int lineno=0, lineinc=1, lstart=0;
	for(int k=0;k<pos;++k)
	{
		lineinc+=text[k]=='\r';
		if(text[k]=='\n')
		{
			lineno+=lineinc, lineinc=1;
			lstart=k+1;
		}
	}
	if(linestart)
		*linestart=lstart;
	return lineno;
}
void		lex_error(const char *text, int len, int pos, const char *format, ...)
{
	const int viewsize=100;
	va_list args;
	int lineno, linestart;

	lineno=get_lineno(text, pos, &linestart);
	printf("%s(%d:%d) Error: ", currentfilename, lineno+1, pos-linestart);
	if(format)
	{
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}
	else
		printf("Unrecognized text\n");
	printf("\t%.*s\n\n", pos+viewsize>len?len-pos:viewsize, text+pos);
}

#define		DUPLET(A, B)	((unsigned char)(A)|(unsigned char)(B)<<8)
typedef enum LexerStateEnum
{
	IC_NORMAL,
	IC_HASH,
	IC_INCLUDE,
} LexerState;
static int token_flags(char c0, char c1)
{
	int flags=
		(c0==' '||c0=='\t'||c0=='\r'||c0=='\n')|
		(c0=='\n')<<1|
		(c1==' '||c1=='\t'||c1=='\r'||c1=='\n')<<2|
		(c1=='\n')<<3;
	return flags;
}
static void	lex_push_tok(DListHandle tokens, LexOption *opt, const char *p, int k, int linestart, int lineno)
{
	Token *token;
	char c0=k>0?p[k-1]:0, c1=p[k+opt->len];

	token=dlist_push_back(tokens, 0);
	token->type=opt->token;
	token->pos=k;
	token->len=opt->len;
	token->line=lineno;
	token->col=k-linestart;
	token->flags=token_flags(c0, c1);
	//token->ws_before=c0==' '||c0=='\t'||c0=='\r'||c0=='\n';
	//token->nl_before=c0=='\n';
	//token->ws_after=c1==' '||c1=='\t'||c1=='\r'||c1=='\n';
	//token->nl_after=c1=='\n';
	token->i=0;
}
typedef enum StringTypeEnum
{
	STR_ID,
	STR_RAW,
	STR_LITERAL,
	CHAR_LITERAL,
} StringType;
static void lex_push_string(DListHandle tokens, CTokenType toktype, StringType strtype, const char *p, int plen, int k, int len, int linestart, int lineno)
{
	Token *token;
	char *str;
	const char *cstr;
	char c0, c1;

	token=dlist_push_back(tokens, 0);

	c0=k>0?p[k-1]:0, c1=p[k+len];
	token->type=toktype;
	token->pos=k;
	token->len=len;
	token->line=lineno;
	token->col=k-linestart;
	token->flags=token_flags(c0, c1);
	//token->ws_before=c0==' ' ||c0=='\t'||c0=='\r'||c0=='\n';
	//token->nl_before=c0=='\n';
	//token->ws_after=c1==' ' ||c1=='\t'||c1=='\r'||c1=='\n';
	//token->nl_after=c1=='\n';

	if(strtype==STR_ID||strtype==STR_RAW)
		cstr=p+k, str=0;
	else
		str=esc2str(p+k, len, &len), cstr=str;
	
	if(strtype==CHAR_LITERAL)
	{
		token->i=0;
		if(len>8)
			lex_error(p, plen, k, "Over 8 characters in character constant");
		else if(len>1)
		{
			token->type=len>4?T_VAL_I64:T_VAL_I32;
			for(int k2=0;k2<len;++k2)
				token->i|=(unsigned long long)cstr[k2]<<(k2<<3);
		}
		else
			token->i=cstr[0];
	}
	else
		token->str=strlib_insert(cstr, len);

	if(str)
		free(str);
}
static int	lex_skip_str_literal(const char *p, int len, int k, char closingsymbol, int *matched)//initialize matched to false to suppress error
{
	for(;;)
	{
		if(k>=len||p[k]=='\n')
		{
			if(*matched)
			{
				switch(closingsymbol)
				{
				case '\'':	lex_error(p, len, k, "Expected a closing quote \'\\\'\'");			break;
				case '\"':	lex_error(p, len, k, "Expected a closing double quote \'\\\"\'");	break;
				case '>':	lex_error(p, len, k, "Expected a closing chevron \'>\'");			break;
				}
			}
			*matched=0;
			break;
		}
		if(p[k]==closingsymbol)
		{
			*matched=1;
			break;
		}
		if(p[k]=='\\')
			k+=2;//esc newlines were already removed
		else
			++k;
	}
	return k;
}
typedef enum LexFlagsEnum
{
	LEX_NORMAL,
	LEX_INCLUDE_ONCE,
} LexFlags;
typedef struct LexedFileStruct
{
	const char *filename;
	char *text;
	size_t len;
	ArrayHandle tokens;
	LexFlags flags;
} LexedFile;
static CmpRes lexlib_cmp(const void *left, const void *right)
{
	LexedFile const *f1=(LexedFile const*)left, *f2=(LexedFile const*)right;
	return (CmpRes)((f1->filename>f2->filename)-(f1->filename<f2->filename));
}
static void	lexlib_destructor(void *data)
{
	LexedFile *lf=(LexedFile*)data;
	free(lf->text), lf->text=0;
	free(lf->tokens), lf->tokens=0;
}
static int	lex(LexedFile *lf)//returns 1 if succeeded
{
	DList tokens={0};
	Reg128 reg={0};
	LexOption *opt;
	LexerState lex_chevron_state=IC_NORMAL;

	if(!lf->filename)
	{
		LOG_ERROR("lex() lexfile->filename == nullptr");
		return 0;
	}
	lf->text=load_text(lf->filename, &lf->len);
	if(!lf->text)
		return 0;
	if(!lex_slots)
		lexer_init();

	lf->text=(char*)realloc(lf->text, lf->len+16);//look-ahead padding
	char *p=lf->text;
	int len=(int)lf->len;

	unsigned short data=DUPLET(p[0], p[1]);
	for(int k=0;k<len;++k)//remove esc newlines
	{
		//if(p[k]=='\\'&&p[k+1]=='\n')
		if(data==('\\'|'\n'<<8))
		{
			int k2=k+2;
			for(;k2<len&&p[k2]!=' '&&p[k2]!='\t';++k2)
				p[k2-2]=p[k2];
			p[k2-2]=' ', p[k2-1]='\r';//'\r' doesn't break #defines but adjusts line increment
		}
		data<<=8, data|=(unsigned char)p[k+2];
	}

	dlist_init(&tokens, sizeof(Token), 128);
	int k0=-1;
	for(int k=0, lineno=0, linestart=0, lineinc=1;k<len;)//lexer loop
	{
#ifdef PRINT_LEX
		printf("\rk %d text \'%.*s\'", k, 16, p+k);
#endif
		if(k0==k)
		{
			printf("\nLexer stuck k %d text \'%.*s\'. Enter k: \n", k, 16, p+k);
			scanf("%d", &k);
		}
		k0=k;
		ArrayHandle slot=lex_slots[(unsigned char)p[k]];
		if(slot)
		{
			int nslots=array_size(&slot);
			if(nslots)
			{
				for(int ko=0;ko<nslots;++ko)
				{
					opt=(LexOption*)array_at(&slot, ko);
					memcpy(reg.text, p+k, 16);
					reg.val[0]&=opt->mask.val[0];
					reg.val[1]&=opt->mask.val[1];
					if(reg.val[0]==opt->val.val[0]&&reg.val[1]==opt->val.val[1])
					{
						char next=p[k+opt->len];
						if(!opt->endswithnan||!(isalnum(next)|(next=='_')))
							goto lex_keyword;
					}
				}
			}
		}
		switch(p[k])
		{
		case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':case '.':
			{
			lex_number:
				lex_chevron_state=IC_NORMAL;
				Number val;
				int start=k;
				int success=acme_read_number(p, len, &k, &val);
				if(success)
				{
					Token *token=dlist_push_back(&tokens, 0);
					switch(val.type)
					{
					case NUM_I32:
						token->type=T_VAL_I32;
						token->i=val.i64;
						break;
					case NUM_U32:
						token->type=T_VAL_U32;
						token->i=val.u64;
						break;
					case NUM_I64:
						token->type=T_VAL_I64;
						token->i=val.i64;
						break;
					case NUM_U64:
						token->type=T_VAL_U64;
						token->i=val.u64;
						break;
					case NUM_F32:
						token->type=T_VAL_F32;
						token->f32=val.f32;
						break;
					case NUM_F64:
						token->type=T_VAL_F64;
						token->f64=val.f64;
						break;
					}
					token->pos=start;
					token->len=k-start;
					token->line=lineno;
					token->col=start-lineno;
				}
			}
			break;
		case ' ':case '\t':case '\r'://skip whitespace & escaped newlines
			for(;k<len&&p[k]&&(p[k]==' '||p[k]=='\t'||p[k]=='\r');lineinc+=p[k]=='\r', ++k);
			break;
		default://identifiers
			lex_chevron_state=IC_NORMAL;
			if(isalpha(p[k])||p[k]=='_')
			{
				int k2=k+1;
				for(;k2<len&&p[k2]&&(isalnum(p[k2])||p[k2]=='_');++k2);
				lex_push_string(&tokens, T_ID, STR_ID, p, len, k, k2-k, linestart, lineno);
				k=k2;
			}
			else
			{
				lex_error(p, len, k, 0);
				++k;//TODO error recovery
			}
			break;
		}
		continue;

	lex_keyword:
		switch(opt->token)
		{
		case T_LINECOMMENTMARK:
			{
				int k2=k+2;
				for(;k2<len&&p[k2]&&p[k2]!='\n';lineinc+=p[k2]=='\r', ++k2);
				k=k2;
			}
			break;
		case T_BLOCKCOMMENTMARK:
			for(unsigned short data=DUPLET(p[k], p[k+1]);;)
			{
				if(k>=len)
				{
					lex_error(p, len, k, "Expected \'/*\'");
					break;
				}
				if(data==DUPLET('*', '/'))
				{
					k+=2;
					break;
				}
				if((data&0xFF)=='\n')
				{
					++k;
					lineno+=lineinc, lineinc=0, linestart=k;
				}
				else
					++k;
				data<<=8, data|=(unsigned char)p[k+1];
			}
			//{
			//	int k2=k+2;
			//	for(;k2<len&&p[k2]&&!(p[k2]=='*'&&p[k2+1]=='/');lineinc+=p[k2]=='\r', ++k2);
			//	advance=k2+2-k;
			//}
			break;
		case T_NEWLINE:
			lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			++k;
			lineno+=lineinc;
			lineinc=1;
			linestart=k;
			break;
		case T_NEWLINE_ESC:
			++lineinc;
			++k;
			break;
		case T_PERIOD:
			{
				char next=p[k+1];
				if(BETWEEN('0', next, '9'))
					goto lex_number;
				lex_chevron_state=IC_NORMAL;
				lex_push_tok(&tokens, opt, p, k, linestart, lineno);
				k+=opt->len;
			}
			break;
		case T_QUOTE:case T_QUOTE_WIDE:case T_DOUBLEQUOTE:case T_DQUOTE_WIDE://quoted text
			{
				lex_chevron_state=IC_NORMAL;
				int start=k+opt->len;
				char closingsymbol=opt->token==T_QUOTE||opt->token==T_QUOTE_WIDE?'\'':'\"';
				int matched=1;
				k=lex_skip_str_literal(p, len, start, closingsymbol, &matched);
				CTokenType toktype=T_IGNORED;
				StringType strtype=STR_ID;
				switch(opt->token)
				{
				case T_QUOTE:		toktype=T_VAL_C8,	strtype=CHAR_LITERAL;	break;
				case T_QUOTE_WIDE:	toktype=T_VAL_C32,	strtype=CHAR_LITERAL;	break;
				case T_DOUBLEQUOTE:	toktype=T_VAL_STR,	strtype=STR_LITERAL;	break;
				case T_DQUOTE_WIDE:	toktype=T_VAL_WSTR,	strtype=STR_LITERAL;	break;
				}
				lex_push_string(&tokens, toktype, strtype, p, len, start, k-start, linestart, lineno);
				k+=matched;//skip closing cymbol
			}
			break;
#ifdef ACC_CPP
		case T_DQUOTE_RAW:
			break;
#endif
		case T_HASH:
			lex_chevron_state=IC_HASH;
			lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			k+=opt->len;
			break;
		case T_ERROR:
			if(lex_chevron_state==IC_HASH)
			{
				lex_chevron_state=IC_NORMAL;
				lex_push_tok(&tokens, opt, p, k, linestart, lineno);
				int start=k+strlen(keywords[T_ERROR]);
				for(;start<len&&(p[start]==' '||p[start]=='\t'||p[start]=='\r');lineinc+=p[start]=='\r', ++start);
				int end=start+1;
				for(;end<len&&p[end]&&p[end]!='\n';lineinc+=p[end]=='\r', ++end);//BUG: esc nl flags remain in error msg
				lex_push_string(&tokens, T_VAL_STR, STR_LITERAL, p, len, start, end-start, linestart, lineno);
				k=end;
			}
			else
				lex_push_string(&tokens, T_VAL_STR, STR_LITERAL, p, len, k, strlen(keywords[T_ERROR]), linestart, lineno);
			k+=opt->len;
			break;
			case T_LESS:
				if(lex_chevron_state==IC_INCLUDE)
				{
					int k2=k+1;
					for(;k2<len&&p[k2]&&p[k2]!='>'&&p[k2]!='\n';lineinc+=p[k2]=='\r', ++k2);
					if(p[k]=='<'&&p[k2]=='>')
						lex_push_string(&tokens, T_INCLUDENAME_STD, STR_LITERAL, p, len, k+1, k2-(k+1), linestart, lineno);
					else
					{
						//error: unmatched chevron
						lex_error(p, len, k, "Unmatched include chevron: [%d]: %s, [%d] %s\n", k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
						//printf("Lexer: line %d: unmatched include chevron: [%d]: %s, [%d] %s\n", lineno+1, k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
					}
					k=k2+1;
				}
				else
					lex_push_tok(&tokens, opt, p, k, linestart, lineno);
				lex_chevron_state=IC_NORMAL;
				k+=opt->len;
				break;
		case T_INCLUDE:
			if(lex_chevron_state==IC_HASH)
			{
				lex_chevron_state=IC_INCLUDE;
				lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			}
			else
				lex_push_string(&tokens, T_ID, STR_LITERAL, p, len, k, strlen(keywords[T_INCLUDE]), linestart, lineno);
			k+=opt->len;
			break;
		default:
			lex_chevron_state=IC_NORMAL;
			lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			k+=opt->len;
			break;
		}
	}

	lf->tokens=dlist_toarray(&tokens);
	dlist_clear(&tokens, 0);
	return 1;
}

//preprocessor
void pp_error(Token const *token, const char *format, ...)
{
	va_list args;
	if(token)
		printf("%s(%d) ", currentfilename, token->line);
	printf("Error: ");
	if(format)
	{
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}
	else
		printf("Unknown error\n");
}
static void token_stringize(Token const *tokens, int ntokens, int first, int count, Token *out)
{
	Token const *token;
	ArrayHandle text;

	STR_ALLOC(text, 0);
	if(count<=0)
	{
		if(first<ntokens)
		{
			token=tokens+first;
			out->pos=token->pos;
			out->len=0;
			out->line=token->line;
			out->col=token->col;
		}
	}
	else
	{
		token=tokens+first;
		int start=token->pos, end=token[count-1].pos+token[count-1].len;
		out->pos=start;
		out->line=token->line;
		out->col=token->col;
		int len=end-start;
		if(len>0)
		{
			STR_APPEND(text, 0, len, 1);
			int ks=start, kd=0;
			for(;ks<end;)
			{
				char c=currentfile[ks];
				if(c=='\r')
				{
					++ks;
					continue;
				}
				unsigned short word=DUPLET(c, currentfile[ks+1]);
				if(word==DUPLET('/', '/'))
				{
					ks+=2;
					for(;ks<end&&currentfile[ks]!='\n';++ks);
					continue;
				}
				if(word==DUPLET('/', '*'))
				{
					ks+=2;
					for(;ks<end;++ks)
					{
						word=DUPLET(currentfile[ks], currentfile[ks+1]);
						if(word==DUPLET('*', '/'))
						{
							ks+=2;
							break;
						}
					}
					continue;
				}
				*(char*)array_at(&text, kd)=currentfile[ks];
				++ks, ++kd;
			}
			text->count=kd;
			STR_FIT(text);
		}
		char c0=start>0?currentfile[start-1]:0, c1=currentfile[end];
		out->flags=token_flags(c0, c1);
	}
	out->len=text->count;
	out->str=strlib_insert((char*)text->data, out->len);
	array_free(&text, 0);
}
static void token_append2str(Token const *t, ArrayHandle *text)
{
	if(t->type==T_LEXME||t->synth)
		STR_APPEND(*text, t->str, t->len, 1);
	else
	{
		int isstr=t->type==T_VAL_STR||t->type==T_VAL_WSTR,
			ischar=t->type==T_VAL_C8||t->type==T_VAL_C32;
		if(isstr)
			STR_APPEND(*text, "\"", 1, 1);
		if(ischar)
			STR_APPEND(*text, "\'", 1, 1);
		STR_APPEND(*text, currentfile+t->pos, t->len, 1);//BUG what if token is not from current file?
		if(ischar)
			STR_APPEND(*text, "\'", 1, 1);
		if(isstr)
			STR_APPEND(*text, "\"", 1, 1);
	}
}
static void token_paste(Token const *t1, Token const *t2, ArrayHandle *dst)//concatenate
{
	if(!t1||!t2)
	{
		if(t1)
			array_insert(dst, dst[0]->count, t1, 1, 1, 0);
		if(t2)
			array_insert(dst, dst[0]->count, t2, 1, 1, 0);
		return;
	}
	if((t1->type==T_VAL_STR||t1->type==T_VAL_WSTR)&&(t2->type==T_VAL_STR||t2->type==T_VAL_WSTR))
	{
		array_insert(dst, dst[0]->count, t1, 1, 1, 0);
		array_insert(dst, dst[0]->count, t2, 1, 1, 0);
	}
	else
	{
		ArrayHandle text=0;
		token_append2str(t1, &text);
		token_append2str(t2, &text);
		Token *token=array_insert(dst, dst[0]->count, 0, 1, 1, 0);
		token->type=T_LEXME;
		token->ws_before=t1->ws_before;
		token->nl_before=t1->nl_before;
		token->ws_after=t2->ws_after;
		token->nl_after=t2->nl_after;
		token->synth=1;
		token->str=strlib_insert((char*)text->data, text->count);
	}
}

typedef enum MacroArgCountEnum
{
	MACRO_NO_ARGLIST=-1,
	MACRO_EMPTY_ARGLIST=0,
} MacroArgCount;
typedef struct MacroArgStruct
{
	const char *name;//key
	size_t idx;//val
} MacroArg;
static int macro_arg_cmp(const void *left, const void *right)
{
	MacroArg *a1=(MacroArg*)left, *a2=(MacroArg*)right;
	return a1->name<a2->name;//sort by pointer value
}
static int macro_arg_new(ArrayHandle *argnames, const char *name)
{
	MacroArg *arg;
	int duplicate=0;
	int nargs=argnames[0]->count;
	for(int k=0;k<nargs;++k)
	{
		arg=(MacroArg*)array_at(argnames, k);
		if(arg->name==name)
		{
			duplicate=1;
			break;
		}
	}
	if(!duplicate)
	{
		arg=ARRAY_APPEND(*argnames, 0, 1, 1, 0);
		arg->name=name;
		arg->idx=nargs;
	}
	return duplicate;
}
static int macro_arg_lookup(ArrayHandle argnames, const char *name)//returns -1 if not found
{
	int nargs=argnames->count;
	int L=0, R=nargs-1, mid;
	MacroArg *arg;
	while(L<=R)
	{
		mid=(L+R)>>1;
		arg=(MacroArg*)array_at(&argnames, mid);
		if(arg->name<name)
			L=mid+1;
		else if(arg->name>name)
			R=mid-1;
		else
			return mid;
	}
	return -1;
}
int macro_define(Macro *dst, const char *srcfilename, Token const *tokens, int count)
{
	int ret, kt, duplicate;
	Token const *token;
	Token *token2;
	ArrayHandle argnames;

	if(!str_va_args)
		str_va_args=strlib_insert("__VA_ARGS__", 0);

	ARRAY_ALLOC(MacroArg, argnames, 0, 0);

	if(tokens->type!=T_ID)
	{
		pp_error(tokens, "Expected an identifier");
		ret=0;
		goto exit;
	}
	dst->name=tokens->str;
	dst->srcfilename=srcfilename;
	dst->is_va=0;

	ret=1;
	kt=1;//skip macro name
	token=tokens+kt;
	if(token->type!=T_LPR||token->ws_before)//macro has no arglist
		dst->nargs=MACRO_NO_ARGLIST;
	else//macro has arglist
	{
		++kt;//skip LPR
		dst->nargs=MACRO_EMPTY_ARGLIST;
		if(kt>=count)
		{
			pp_error(token, "Improperly terminated macro argument list.");
			ret=0;
			goto exit;
		}
		token=tokens+kt;
		if(token->type!=T_RPR)//macro arglist is not empty
		{
			for(;kt<count;++kt)
			{
				duplicate=0;
				token=tokens+kt;
				if(token->type==T_ELLIPSIS)
				{
					dst->is_va=1;
					duplicate=macro_arg_new(&argnames, str_va_args);
					if(duplicate)
					{
						pp_error(token, "__VA_ARGS__ already declared.");
						ret=0;
						goto exit;
					}
					++kt;//skip ELLIPSIS
					token=tokens+kt;
					if(token->type!=T_RPR)
					{
						pp_error(token, "Expected a closing parenthesis \')\'.");
						ret=0;
						goto exit;
					}
					++kt;//skip RPR
					break;
				}

				if(token->type!=T_ID)
				{
					pp_error(token, "Expected a macro argument name.");
					ret=0;
					goto exit;
				}
				if(token->str==str_va_args)
				{
					pp_error(token, "\'__VA_ARGS__\' is reserved for variadic macro expansion.");
					ret=0;
					goto exit;
				}
				duplicate=macro_arg_new(&argnames, token->str);
				if(duplicate)
				{
					pp_error(token, "Macro argument name appeared before.");
					ret=0;
					goto exit;
				}
				++kt;
				token=tokens+kt;
				if(token->type==T_RPR)
				{
					++kt;//skip RPR
					break;
				}
				if(token->type!=T_COMMA)
				{
					pp_error(token, "Expected a closing parenthesis \')\' or a comma \',\'.");
					ret=0;
					goto exit;
				}
			}
		}
	}

	if(kt>=count)//macro has no definition
		goto exit;

	//macro has a definition
	qsort(argnames->data, argnames->count, argnames->esize, macro_arg_cmp);//sort args by name pointer value for fast lookup of index

	count-=kt;
	ARRAY_ALLOC(Token, dst->tokens, count, 0);
	memcpy(dst->tokens->data, tokens+kt, count*sizeof(Token));

	token2=(Token*)array_at(&dst->tokens, 0);
	for(kt=0;kt<count;++kt, ++token2)
	{
		int idx;
		if(token2->type==T_ID)
		{
			idx=macro_arg_lookup(argnames, token2->str);
			if(idx!=-1)
			{
				token2->type=T_MACRO_ARG;
				token2->i=idx;
			}
		}
		else if(token2->type==T_VA_ARGS)
		{
			idx=macro_arg_lookup(argnames, str_va_args);
			if(idx==-1)
			{
				pp_error(token2, "__VA_ARGS__ is reserved for variadic macros.");
				ret=0;
				goto exit;
			}
			token2->type=T_MACRO_ARG;
			token2->i=argnames->count-1;
		}
	}

	//check macro definition for errors
	token2=(Token*)array_at(&dst->tokens, 0);
	if(token2->type==T_CONCATENATE||((Token*)array_back(&dst->tokens))->type==T_CONCATENATE)
	{
		pp_error(token2, "Token paste operator \'##\' cannot appear at either end of macro definition.");
		ret=0;
		goto exit;
	}
	for(kt=0;kt<count;++kt, ++token2)
	{
		if(token2->type==T_HASH)
		{
			if(kt+1>=count)
			{
				pp_error(token2, "Stringize operator \'#\' cannot appear at the end of macro definition.");
				ret=0;
				goto exit;
			}
			if(token2[1].type!=T_MACRO_ARG)
			{
				pp_error(token2, "Stringize operator \'#\' cannot appear at the end of macro definition.");
				ret=0;
				goto exit;
			}
		}
	}

exit:
	array_free(&argnames, 0);//no destructor needed
	return ret;
}

static ArrayConstHandle eval_tokens=0;
static Token const *eval_token=0;
static int eval_idx=0, eval_end=0;
static MapHandle eval_macros=0;
static CTokenType eval_type=T_IGNORED;
#define	TOKENS_AT(TOKENS, IDX)			((Token*)array_at(&(TOKENS), IDX))
#define	TOKENS_AT_CONST(TOKENS, IDX)	((Token const*)array_at_const(&(TOKENS), IDX))
#define EVAL_DEREF()					TOKENS_AT_CONST(eval_tokens, eval_idx)
static int eval_next()
{
	int inc=eval_idx+1<eval_end;
	if(inc)
	{
		++eval_idx;
		eval_token=EVAL_DEREF();
	//	eval_token=(Token const*)array_at_const(&eval_tokens, eval_idx);
	}
	else
		eval_token=0;
	return inc;
}
static long long eval_ternary();
static long long eval_unary()
{
again:
	switch(eval_token->type)
	{
	case T_VAL_I32:
	case T_VAL_I64:
		return eval_token->i;
	case T_VAL_U32:
	case T_VAL_U64:
		return eval_token->i;//TODO return type struct Number
	case T_LPR:
		if(!eval_next())
		{
			pp_error(eval_token, "Unexpected end of expression at opening parenthesis.");
			break;
		}
		{
			long long result=eval_ternary();
			if(eval_token->type!=T_RPR)
				pp_error(eval_token, "Missing closing parenthesis.");
			if(!eval_next())
				pp_error(eval_token, "Unexpected end of expression.");
			return result;
		}
		break;
	case T_DEFINED:
		if(!eval_next())
		{
			pp_error(0, "Expected an identifier.");
			return 0;
		}
		if(eval_token->type!=T_ID)
		{
			pp_error(eval_token, "Expected an identifier.");
			return 0;
		}
		{
			BSTNodeHandle *h=MAP_FIND(eval_macros, eval_token->str);
			return h!=0;
		}
		break;
	case T_PLUS:
		if(!eval_next())
		{
			pp_error(eval_token, "Unexpected end of expression at \'+\'.");
			return 0;
		}
		goto again;
	case T_MINUS:
		if(!eval_next())
		{
			pp_error(eval_token, "Unexpected end of expression at \'-\'.");
			return 0;
		}
		return -eval_unary();
	case T_EXCLAMATION:
		if(!eval_next())
		{
			pp_error(eval_token, "Unexpected end of expression at \'!\'.");
			return 0;
		}
		return !eval_unary();
	case T_TILDE:
		if(!eval_next())
		{
			pp_error(eval_token, "Unexpected end of expression at \'~\'.");
			return 0;
		}
		return ~eval_unary();
		break;
	default:
		break;
	}
	pp_error(eval_token, "Invalid expression");
	return 0;
}
static long long eval_mul()
{
	long long result=eval_unary(), r2;
	while(eval_token&&(eval_token->type==T_ASTERIX||eval_token->type==T_SLASH||eval_token->type==T_MODULO))
	{
		eval_type=eval_token->type;
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression.");
			break;
		}
		switch(eval_type)
		{
		case T_ASTERIX:
			result*=eval_unary();
			break;
		case T_SLASH:
			r2=eval_unary();
			if(!r2)
			{
				pp_error(eval_token, "Integer division by zero.");
				result=0x80000000;
			}
			else
				result/=r2;
			break;
		case T_MODULO:
			r2=eval_unary();
			if(!r2)
			{
				pp_error(eval_token, "Integer division by zero.");
				result=0x80000000;
			}
			else
				result%=r2;
			break;
		default:
			break;
		}
	}
	return result;
}
static long long eval_sum()
{
	long long result=eval_mul();
	while(eval_token&&(eval_token->type==T_PLUS||eval_token->type==T_MINUS))
	{
		eval_type=eval_token->type;
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression.");
			break;
		}
		if(eval_type==T_PLUS)
			result+=eval_mul();
		else
			result-=eval_mul();
	}
	return result;
}
static long long eval_shift()
{
	long long result=eval_sum();
	while(eval_token&&(eval_token->type==T_SHIFT_LEFT||eval_token->type==T_SHIFT_RIGHT))
	{
		eval_type=eval_token->type;
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression.");
			break;
		}
		switch(eval_type)
		{
		case T_SHIFT_LEFT:
			result<<=eval_sum();
			break;
		case T_SHIFT_RIGHT:
			result>>=eval_sum();
			break;
		default:
			break;
		}
	}
	return result;
}
static long long eval_comp()
{
	long long result=eval_shift();
	while(eval_token&&(eval_token->type==T_LESS||eval_token->type==T_LESS_EQUAL||eval_token->type==T_GREATER||eval_token->type==T_GREATER_EQUAL))
	{
		eval_type=eval_token->type;
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression.");
			break;
		}
		switch(eval_type)
		{
		case T_LESS:
			result=result<eval_shift();
			break;
		case T_LESS_EQUAL:
			result=result<=eval_shift();
			break;
		case T_GREATER:
			result=result>eval_shift();
			break;
		case T_GREATER_EQUAL:
			result=result>=eval_shift();
			break;
		default:
			break;
		}
	}
	return result;
}
static long long eval_eq()
{
	long long result=eval_comp();
	while(eval_token&&(eval_token->type==T_EQUAL||eval_token->type==T_NOT_EQUAL))
	{
		eval_type=eval_token->type;
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression.");
			break;
		}
		if(eval_type==T_EQUAL)
			result=result==eval_comp();
		else
			result=result!=eval_comp();
	}
	return result;
}
static long long eval_and()
{
	long long result=eval_eq();
	while(eval_token&&eval_token->type==T_AMPERSAND)
	{
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression at ^.");
			break;
		}
		result&=eval_eq();
	}
	return result;
}
static long long eval_xor()
{
	long long result=eval_and();
	while(eval_token&&eval_token->type==T_CARET)
	{
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression at ^.");
			break;
		}
		result^=eval_and();
	}
	return result;
}
static long long eval_or()
{
	long long result=eval_xor();
	while(eval_token&&eval_token->type==T_VBAR)
	{
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression at |.");
			break;
		}
		result|=eval_xor();
	}
	return result;
}
static long long eval_logic_and()
{
	long long result=eval_or();
	while(eval_token&&eval_token->type==T_LOGIC_AND)
	{
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression at &&.");
			break;
		}
		result=result&&eval_or();
	}
	return result;
}
static long long eval_logic_or()
{
	long long result=eval_logic_and();
	while(eval_token&&eval_token->type==T_LOGIC_OR)
	{
		if(!eval_next())
		{
			pp_error(0, "Unexpected end of expression at ||.");
			break;
		}
		result=result||eval_logic_and();
	}
	return result;
}
static long long eval_ternary()
{
	long long result;
	Token const *token;
	int level;
	
again:
	result=eval_logic_or();
	if(eval_token&&eval_token->type==T_QUESTION)
	{
		token=eval_token;
		if(!eval_next())
		{
			pp_error(token, "Unexpected end of expression at ?.");
			return result;
		}
		if(!result)//skip till after matching colon
		{
			++eval_idx;
			level=1;
			for(;eval_idx<eval_end;++eval_idx)
			{
				token=EVAL_DEREF();
				level+=(token->type==T_QUESTION)-(token->type==T_COLON);
				if(!level)
				{
					++eval_idx;
					break;
				}
			}
		}
		goto again;
		//result=eval_ternary();
	}
	return result;
}
static long long eval_expr(ArrayConstHandle tokens, int start, int end, MapHandle macros)
{
	if(start>=end)
	{
		Token const *token=0;
		if(start<(int)tokens->count)
			token=(Token const*)array_at_const(&tokens, start);
		pp_error(token, "Expected an expression.");
		return 0;
	}
	eval_tokens=tokens;
	eval_idx=start-1;
	eval_end=end;
	eval_macros=macros;
	eval_next();
	return eval_ternary();
}


typedef struct BookmarkStruct
{
	LexedFile *lf;
	int ks, iflevel;
} Bookmark;

static char*	test_include(const char *searchpath, int pathlen, const char *includename, int inclen)
{
	ArrayHandle filename;
	char *str;

	STR_ALLOC(filename, pathlen+inclen);
	memcpy(filename->data, searchpath, pathlen);
	memcpy(filename->data+pathlen, includename, inclen);
	int ret=file_is_readable((char*)filename->data);
	if(ret==1)
		str=strlib_insert((char*)filename->data, filename->count);
	else
		str=0;
	array_free(&filename, 0);
	return str;
}
static char*	find_include(const char *includename, int custom, ArrayHandle includepaths)
{
	char *filename=0;
	int inclen=strlen(includename);
	if(custom)
	{
		int size=strlen(currentfilename);//relative path
		int k=size-1;
		for(;k>=0;--k)
		{
			char c=currentfilename[k];
			if(c=='/'||c=='\\')
				break;
		}
		++k;//include the slash
		filename=test_include(currentfilename, k, includename, inclen);
		if(filename)
			return filename;
	}
	for(int ki=0;ki<(int)includepaths->count;++ki)
	{
		const char *path=*(const char**)array_at(&includepaths, ki);
		filename=test_include(path, strlen(path), includename, inclen);
		if(filename)
			return filename;
	}
	//if(projectfolder)//finally check the current working directory
	//{
	//	filename=test_include(0, 0, includename, inclen);
	//	if(filename)
	//		return filename;
	//}
	return 0;
}
static int skip_till_newline(ArrayConstHandle tokens, int k)
{
	for(;k<(int)tokens->count&&((Token const*)array_at_const(&tokens, k))->type!=T_NEWLINE;++k);
	return k;
}
static int skip_block(MapHandle macros, ArrayConstHandle tokens, int *k, int lastblock)
{
	int k0, ntokens, level;
	Token const *token;

	k0=*k;
	ntokens=tokens->count;
	level=1;
	while(*k<ntokens)
	{
		token=(Token const*)array_at_const(&tokens, *k);
		++*k;//k points at next token
		switch(token->type)
		{
		case T_IF:case T_IFDEF:
			++level;
			break;
		case T_ELIF:
			if(level==1)
			{
				if(lastblock)
					pp_error(token, "#else already appeared. Expected #endif.");
				int start=*k;
				*k=skip_till_newline(tokens, *k);
				long long result=eval_expr(tokens, start, *k, macros);
				*k+=*k<ntokens;//skip newline
				if(result)
					return 1;
			}
			break;
		case T_ELSE:
			if(level==1)
			{
				if(lastblock)
					pp_error(token, "#else already appeared. Expected #endif.");
				int start=*k;
				*k=skip_till_newline(tokens, start);
				if(start<*k)
					pp_error(token, "Unexpected tokens after #else.");
				*k+=*k<ntokens;//skip newline
				return 1;
			}
			break;
		case T_ENDIF:
			--level;
			if(!level)
			{
				int start=*k;
				*k=skip_till_newline(tokens, start);
				if(start<*k)
					pp_error(token, "Unexpected tokens after #endif.");
				*k+=*k<ntokens;//skip newline
				return 0;
			}
			break;
		default:
			break;
		}//end switch
		*k=skip_till_newline(tokens, *k);
		*k+=*k<ntokens;//skip the newline
	}
	if(*k>=ntokens)
	{
		token=(Token const*)array_at_const(&tokens, k0);
		pp_error(token, "Missing #endif");
	}
	return 0;
}


//macro expansion
static void macro_arg_destructor(void *data)
{
	ArrayHandle *tokens=(ArrayHandle*)data;
	array_free(tokens, 0);
}
static int	macro_find_call_extent(Macro *macro, ArrayHandle tokens, int start, int *ret_len, ArrayHandle *args)
{
	if(!*args)
		ARRAY_ALLOC(ArrayHandle, *args, 0, 0);
	else//clear args
		array_clear(args, macro_arg_destructor);
	
	if(macro->nargs==MACRO_NO_ARGLIST)
	{
		*ret_len=1;
		return 1;
	}
	Token *token=TOKENS_AT(tokens, start+1);
	if(token->type!=T_LPR)
	{
		pp_error(token, "Macro should have an argument list with %d arguments.", macro->nargs);
		*ret_len=1;
		return 0;
	}
	int k0=start+2, end=k0, nargs=0, ntokens=(int)tokens->count;
	for(int level=1;end<ntokens&&level>0;++end)
	{
		token=TOKENS_AT(tokens, end);
		level+=(token->type==T_LPR)-(token->type==T_RPR);
		if(level==1&&token->type==T_COMMA||!level&&token->type==T_RPR)//end of an argument expression
		{
			int va_append=macro->is_va&&nargs+1>macro->nargs;//in case of variadic macros, append extra args with commas as the last arg
			int count=end-k0;
			token=TOKENS_AT(tokens, k0);
			if(va_append)
			{
				ArrayHandle *h=array_back(args);
				count+=token->type==T_COMMA;//DEBUG
				ARRAY_APPEND(*h, token, count, 1, 0);
			}
			else
			{
				ArrayHandle *h=ARRAY_APPEND(*args, 0, 1, 1, 0);
				ARRAY_ALLOC(Token, *h, count, 0);
				memcpy(h[0]->data, token, count*sizeof(Token));
			}
			k0=end+1;
			nargs+=!va_append;
		}
	}
	*ret_len=end-start;
	if(token->type!=T_RPR)
	{
		pp_error(token, "Unmatched parenthesis.");
		return 0;
	}
	if(macro->is_va)
	{
		if(nargs<macro->nargs)
		{
			token=TOKENS_AT(tokens, start);
			pp_error(token, "Variadic macro: expected at least %d arguments, got %d instead.", macro->nargs, nargs);
			return 0;
		}
	}
	else
	{
		if(nargs!=macro->nargs)
		{
			token=TOKENS_AT(tokens, start);
			pp_error(token, "Macro: expected %d arguments, got %d instead.", macro->nargs, nargs);
			return 0;
		}
	}
	return 1;
}
//macrodefinition: array of tokens
//kt: index of stringize token '##'
//args2: array of arrays of tokens
//out: result token
static int	macro_stringize(ArrayHandle macrodefinition, int kt, ArrayHandle args2, Token *out)
{
	Token *token=TOKENS_AT(macrodefinition, kt), *next;
	if(kt+1>(int)macrodefinition->count)
	{
		pp_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition	DUPLICATE
		return 0;
	}
	next=TOKENS_AT(macrodefinition, kt+1);
	if(next->type!=T_MACRO_ARG)
	{
		pp_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition
		return 0;
	}
	ArrayHandle *arg=array_at(&args2, (size_t)next->i);
	token=(Token*)arg[0]->data;
	token_stringize(token, arg[0]->count, 0, arg[0]->count, out);//TODO: inline call
	return 1;
}
//t_left: ?
//macrodefinition: array of tokens
//kt: ?
//args2: array of arrays of tokens
//dst: result array of tokens
static void	macro_paste(Token const *t_left, ArrayHandle macrodefinition, int *kt, ArrayHandle args2, ArrayHandle *dst)
{
	Token *second=TOKENS_AT(macrodefinition, *kt+1);
	if(second->type==T_MACRO_ARG)
	{
		ArrayHandle *callarg=array_at(&args2, (size_t)second->i);//callarg is array of tokens
		if(callarg[0]->count)
		{
			token_paste(t_left, (Token*)callarg[0]->data, dst);
			if(callarg[0]->count>1)
			{
				int count=callarg[0]->count-1;
				ARRAY_APPEND(*dst, 0, count, 1, 0);
				Token *tsrc=TOKENS_AT(*callarg, 1), *tdst=TOKENS_AT(*dst, dst[0]->count-1);
				memcpy(tdst, tsrc, count*sizeof(Token));
			}
		}
		else
			token_paste(t_left, 0, dst);
	}
	else if(second->type==T_HASH)
	{
		Token str={0};
		++*kt;
		macro_stringize(macrodefinition, *kt, args2, &str);
		token_paste(t_left, &str, dst);
	}
	else
		token_paste(t_left, second, dst);
}

typedef struct MacroCallStruct
{
	Macro *macro;
	ArrayHandle args;//array of arrays of tokens
	int kt,//macro definition index
		kt2,//macro call argument index, use on args when macrodefinition[kt] is an arg
		expansion_start,//dst index
		done;
} MacroCall;
static void macrocall_destructor(void *data)
{
	MacroCall *call=(MacroCall*)data;
	array_free(&call->args, macro_arg_destructor);
}
static void	macro_expand(MapHandle macros, Macro *macro, ArrayHandle src, int *ks, ArrayHandle *dst)
{
	int len;
	DList context;//a stack of macro calls
	MacroCall *topcall;

	dlist_init(&context, sizeof(MacroCall), 4);
	topcall=(MacroCall*)dlist_push_back(&context, 0);
	if(!macro_find_call_extent(macro, src, *ks, &len, &topcall->args))
	{
		ks+=len;
		//error_pp(TOKENS_AT(src, *ks), "Invalid macro call.");//redundant error message
		dlist_clear(&context, macrocall_destructor);
		return;
	}
	
	ARRAY_APPEND(*dst, 0, 0, 1, len);

	while(context.nobj)
	{
	macro_expand_again:
		topcall=(MacroCall*)dlist_back(&context);
		if(topcall->done)
		{
			dlist_pop_back(&context, macrocall_destructor);
			continue;
		}
		Macro *m2=topcall->macro;
		ArrayHandle definition=m2->tokens;
		while(topcall->kt<(int)definition->count)
		{
			Token *token=TOKENS_AT(definition, topcall->kt);
			if(token->type==T_HASH)//stringize call-argument
			{
				Token *out=array_insert(dst, dst[0]->count, 0, 1, 1, 0);
				macro_stringize(definition, topcall->kt, topcall->args, out);//X  check if next is concatenate
				topcall->kt+=2;
			}
			else if(topcall->kt+1<(int)definition->count&&TOKENS_AT(definition, topcall->kt+1)->type==T_CONCATENATE)//anything can be concatenated
			{
				++topcall->kt;
				if(topcall->kt+1>=(int)definition->count)
				{
					Token *next=TOKENS_AT(definition, topcall->kt);
					pp_error(next, "Token paste operator cannot be at the end of macro.");//error in definition
					break;
				}
				Token const *t_left=0;
				if(token->type==T_MACRO_ARG)
				{
					ArrayHandle *callarg2=array_at(&topcall->args, (size_t)token->i);
					if(callarg2[0]->count>1)
					{
						int count=callarg2[0]->count-1;
						ARRAY_APPEND(*dst, 0, count, 1, 0);
						memcpy(TOKENS_AT(*dst, dst[0]->count-count), callarg2[0]->data, count*sizeof(Token));
					}
					if(callarg2[0]->count>0)
						t_left=(Token const*)array_back(callarg2);
				}
				else
					t_left=token;
				macro_paste(t_left, definition, &topcall->kt, topcall->args, dst);
				topcall->kt+=2;
			}
			else if(token->type==T_CONCATENATE)
			{
				if(topcall->kt+1>=(int)definition->count)
				{
					pp_error(token, "Token paste operator cannot be at the end of macro.");//error in definition
					break;
				}
				int kd2=dst[0]->count-1;
				CTokenType temp;
				for(;kd2>=topcall->expansion_start&&((temp=TOKENS_AT(*dst, kd2)->type)<=T_IGNORED||temp==T_NEWLINE);--kd2);
				if(kd2>=topcall->expansion_start)
				{
					dst[0]->count=kd2;
					macro_paste(TOKENS_AT(*dst, dst[0]->count), definition, &topcall->kt, topcall->args, dst);
				}
				else
					macro_paste(0, definition, &topcall->kt, topcall->args, dst);//
				++dst[0]->count;
				topcall->kt+=2;
			}
			else if(token->type==T_MACRO_ARG)//normal macro call arg
			{
				ArrayHandle *arg=array_at(&topcall->args, (size_t)token->i);
				for(;topcall->kt2<(int)arg[0]->count;)//copy tokens from arg, while checking the arg for macro calls
				{
					Token *token2=TOKENS_AT(*arg, topcall->kt2);
					if(token2->type==T_ID)
					{
						BSTNodeHandle *result=MAP_FIND(macros, token2->str);
						if(result)
						{
							Macro *macro3=(Macro*)result[0]->data;
							int len2=0;
							ArrayHandle args=0;
							if(macro_find_call_extent(macro3, *arg, topcall->kt2, &len2, &args))
							{
								ARRAY_APPEND(*dst, 0, macro3->tokens->count, 1, 0);
								topcall->kt2+=len2;//advance top.kt2 by call length because it will return here

								topcall=dlist_push_back(&context, 0);//request to expand this argument
								topcall->macro=macro3;
								topcall->args=args;
								topcall->kt2=0;
								topcall->expansion_start=dst[0]->count;
								
								goto macro_expand_again;//should resume here after macro is expanded
							}
							//else
							//	error_pp(TOKENS_AT(*dst, kd2-1), "Invalid macro call.");//redundant error message
						}
					}
					memcpy(TOKENS_AT(*dst, dst[0]->count), TOKENS_AT(*arg, topcall->kt2), sizeof(Token));
					++dst[0]->count;
					++topcall->kt2;
				}
				topcall->kt2=0;//reset arg index when done
				++topcall->kt;
			}
			else//copy token from macro definition
			{
				memcpy(TOKENS_AT(*dst, dst[0]->count), token, sizeof(Token));
				++dst[0]->count;
				++topcall->kt;
			}
		}
	}
}

ArrayHandle preprocess(const char *filename, MapHandle macros, ArrayHandle includepaths, MapHandle lexlib)
{
	LexedFile *lf;
	char *unique_fn;
	DList tokens, bookmarks;
	Bookmark *bm;
	Macro *macro;
	ArrayHandle ret;
	int success;

	if(!filename)
	{
		pp_error(0, "Filename is not provided");
		return 0;
	}
	if(!str_pragma_once)
		str_pragma_once=strlib_insert("once", 0);
	if(!lexlib->key_size)
		MAP_INIT(lexlib, const char*, LexedFile, lexlib_cmp);

	unique_fn=strlib_insert(filename, 0);
	BSTNodeHandle *result=MAP_FIND(lexlib, unique_fn);
	if(!result)
	{
		result=MAP_INSERT(lexlib, &unique_fn, 0, 0);
		lf=(LexedFile*)result[0]->data;
		success=lex(lf);
	}
	else
		lf=(LexedFile*)result[0]->data;

	if(!lf||!lf->tokens)
		return 0;

	dlist_init(&bookmarks, sizeof(Bookmark), 16);
	bm=(Bookmark*)dlist_push_back(&bookmarks, 0);
	bm->lf=lf;
	bm->ks=0;
	bm->iflevel=0;

	dlist_init(&tokens, sizeof(Token), 128);

	while(bookmarks.nobj>0)
	{
	preprocess_start:
		bm=(Bookmark*)dlist_back(&bookmarks);
		lf=bm->lf;

		currentfilename=lf->filename;
		currentfile=lf->text;

		int ntokens=lf->tokens->count;
		while(bm->ks<ntokens)
		{
			Token *token=(Token*)array_at(&lf->tokens, bm->ks);
			switch(token->type)
			{
			case T_HASH://TODO: directives should only work at the beginning of line
				++bm->ks;//skip HASH
				if(bm->ks>=ntokens)
				{
					pp_error(token, "Unexpected end of file. Expected a preprocessor directive.");
					continue;
				}
				token=(Token*)array_at(&lf->tokens, bm->ks);
				switch(token->type)
				{
				case T_DEFINE:
					{
						int k, redefinition;

						++bm->ks;//skip DEFINE
						if(bm->ks>=ntokens)
						{
							pp_error(token, "Unexpected end of file.");
							break;
						}
						token=(Token*)array_at(&lf->tokens, bm->ks);
						k=skip_till_newline(lf->tokens, bm->ks);
						if(token->type!=T_ID)
							pp_error(token, "Expected an identifier.");
						else
						{
							redefinition=0;
							macro=(Macro*)MAP_INSERT(macros, token->str, 0, &redefinition)[0]->data;//insert can return null only if comparator is ill-defined
							if(redefinition)
								pp_error(token, "Macro redifinition.");
							macro_define(macro, currentfilename, token, k-bm->ks);
						}
						bm->ks=k+(((Token*)array_at(&lf->tokens, k))->type==T_NEWLINE);//skip possible newline
					}
					break;
				case T_UNDEF:
					++bm->ks;//skip UNDEF
					if(bm->ks>=ntokens)
					{
						pp_error(token, "Unexpected end of file.");
						break;
					}
					token=(Token*)array_at(&lf->tokens, bm->ks);
					if(token->type!=T_ID)
						pp_error(token, "Expected an identifier.");
					else
					{
						MAP_ERASE(macros, token->str);
						++bm->ks;//skip ID
					}
					break;
				case T_IFDEF:
				case T_IFNDEF:
					++bm->ks;
					if(bm->ks>=ntokens)
					{
						pp_error(token, "Unexpected end of file.");
						break;
					}
					token=(Token*)array_at(&lf->tokens, bm->ks);
					if(token->type!=T_ID)
						pp_error(token, "Expected an identifier.");
					else
					{
						BSTNodeHandle *result=MAP_FIND(macros, token->str);
						int ndef=((Token*)array_at(&lf->tokens, bm->ks-1))->type==T_IFNDEF;
						if((!result)!=ndef)
							bm->iflevel+=skip_block(macros, lf->tokens, &bm->ks, 0);
						else
						{
							++bm->iflevel;
							++bm->ks;
							int start=bm->ks;
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							if(start<bm->ks)
								pp_error(token, "Expected a single token after #ifdef/ifndef.");
							bm->ks+=bm->ks<ntokens;
						}
					}
					break;
				case T_IF:
					{
						++bm->ks;//skip IF
						int start=bm->ks;
						skip_till_newline(lf->tokens, bm->ks);
						long long result=eval_expr(lf->tokens, start, bm->ks, macros);
						if(result)
						{
							++bm->iflevel;
							bm->ks+=bm->ks<ntokens;
						}
						else
							bm->iflevel+=skip_block(macros, lf->tokens, &bm->ks, 0);//skip till #elif true/else/endif
					}
					break;
				case T_ELIF:
				case T_ELSE:
					{
						++bm->ks;
						int start=bm->ks;
						bm->ks=skip_till_newline(lf->tokens, bm->ks);
						int extratokens=start<bm->ks, expected=bm->iflevel>0, lastblock=token->type==T_ELSE;
						if(expected)
						{
							skip_block(macros, lf->tokens, &bm->ks, lastblock);//just skip till #endif
							--bm->iflevel;
						}
						else//no else/elif expected
							pp_error(token, "Unexpected #else/elif.");
						if(lastblock&&extratokens)
							pp_error(TOKENS_AT(lf->tokens, start), "Unexpected tokens after #else.");
						if(!expected)
							bm->ks+=bm->ks<ntokens;//skip newline
					}
					break;
				case T_ENDIF:
					{
						++bm->ks;//skip ENDIF
						if(!bm->iflevel)
							pp_error(token, "Unmatched #endif.");
						else
							--bm->iflevel;
						int start=bm->ks;
						bm->ks=skip_till_newline(lf->tokens, bm->ks);
						if(start<bm->ks)
							pp_error(TOKENS_AT(lf->tokens, start), "Unexpected tokens after #endif.");
						bm->ks+=bm->ks<ntokens;
					}
					break;

				case T_INCLUDE:
					++bm->ks;//skip INCLUDE
					if(bm->ks>=ntokens)
					{
						pp_error(token, "Unexpected end of file.");
						break;
					}
					token=TOKENS_AT(lf->tokens, bm->ks);
					if(token->type==T_INCLUDENAME_STD||token->type==T_VAL_STR)
					{
						char *filename=find_include(token->str, token->type==T_VAL_STR, includepaths);
						if(!filename)
						{
							pp_error(token, "Cannot open include file \'%s\'.", token->str);
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							bm->ks+=bm->ks<ntokens;
						}
						else
						{
							int found=0;
							BSTNodeHandle *result=MAP_INSERT(lexlib, filename, 0, &found);
							LexedFile *lf2=(LexedFile*)result[0]->data;
							if(!found)//not lexed before
								lex(lf2);
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							bm->ks+=bm->ks<ntokens;
							if(!(lf2->flags&LEX_INCLUDE_ONCE))//not marked with '#pragma once'
							{
								bm=dlist_push_back(&bookmarks, 0);
								bm->lf=lf2;
								goto preprocess_start;
							}
						}
					}
					else
					{
						pp_error(token, "Expected include file name.");
						skip_till_newline(lf->tokens, bm->ks);
						bm->ks+=bm->ks<ntokens;
					}
					break;
				case T_PRAGMA:
					++bm->ks;//skip INCLUDE
					if(bm->ks>=ntokens)
					{
						pp_error(token, "Unexpected end of file.");
						break;
					}
					token=TOKENS_AT(lf->tokens, bm->ks);
					if(token->type==T_ID)
					{
						if(token->str==str_pragma_once)//'#pragma once': header is included once per source
							lf->flags|=LEX_INCLUDE_ONCE;
					}
					bm->ks=skip_till_newline(lf->tokens, bm->ks);
					bm->ks+=bm->ks<ntokens;
					break;
				case T_ERROR:
					{
						++bm->ks;//skip ERROR
						if(bm->ks>=ntokens)
						{
							pp_error(token, "Unexpected end of file.");
							break;
						}
						token=TOKENS_AT(lf->tokens, bm->ks);
						if(bm->ks<ntokens&&token->type==T_VAL_STR)
						{
							pp_error(token, "%s", token->str);//error message may contain '%'
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							bm->ks+=bm->ks<ntokens;
						}
						else
						{
							int start=bm->ks;
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							pp_error(TOKENS_AT(lf->tokens, start-1), "%.*s", bm->ks-start, currentfile+start);
							bm->ks+=bm->ks<ntokens;//skip newline
						}
					}
					break;
				default:
					pp_error(token, "Expected a preprocessor directive.");
					bm->ks=skip_till_newline(lf->tokens, bm->ks);
					bm->ks+=bm->ks<ntokens;
					break;
				}
				break;//case T_HASH		#...
			case T_MACRO_FILE:
				{
					++bm->ks;
					Token *token2=dlist_push_back(&tokens, 0);

					memcpy(token2, token, sizeof(Token));
					token2->type=T_VAL_STR;
					token2->len=strlen(currentfilename);
					token2->str=strlib_insert(currentfilename, token2->len);
					token2->synth=1;

					//token2->type=T_VAL_STR;
					//token2->str=currentfilename;
					//token2->pos=token->pos;
					//token2->len=strlen(currentfilename);
					//token2->line=token->line;
					//token2->col=token->col;
					//token2->flags=token->flags;
					//token2->synth=1;
				}
				break;
			case T_MACRO_LINE:
				{
					++bm->ks;
					Token *token2=dlist_push_back(&tokens, 0);

					memcpy(token2, token, sizeof(Token));
					token2->type=T_VAL_I32;
					token2->i=token->line;
					token2->synth=1;
				}
				break;
			case T_ID:
				{
					BSTNodeHandle *result=MAP_FIND(macros, token->str);
					if(result)
					{
						ArrayHandle dst=0;
						macro=(Macro*)result[0]->data;
						macro_expand(macros, macro, lf->tokens, &bm->ks, &dst);
						if(dst)
						{
							for(int kt=0;kt<(int)dst->count;++kt)
							{
								Token *token2=TOKENS_AT(dst, kt);
								dlist_push_back(&tokens, token2);//TODO dlist_push_back(array)
							}
						}
					}
					else
					{
						dlist_push_back(&tokens, token);
						++bm->ks;
					}
				}
				break;
			case T_NEWLINE:
			case T_NEWLINE_ESC:
				++bm->ks;
				break;
			default:
				if(token->type>T_IGNORED)
					dlist_push_back(&tokens, token);
				++bm->ks;
				break;
			}
		}//end file loop
		if(bm->iflevel>0)
			pp_error((Token*)array_back(&lf->tokens), "End of file reached. Expected %s #endif\'s.", bm->iflevel);
		dlist_pop_back(&bookmarks, 0);
	}//end preprocess loop

	ret=dlist_toarray(&tokens);
	dlist_clear(&tokens, 0);
	return ret;
}

void str_destructor(void *data)
{
	char **str=(char**)data;
	free(*str);
	*str=0;
}
void acc_cleanup(MapHandle lexlib, MapHandle strings)
{
	MAP_CLEAR(lexlib, lexlib_destructor);
	MAP_CLEAR(strings, str_destructor);
}

void tokens2text(ArrayHandle tokens, ArrayHandle *str)
{
	int ntokens;
	Token *token;
	int spaceprinted;

	if(!*str)
		STR_ALLOC(*str, 0);
	ntokens=(int)array_size(&tokens);
	spaceprinted=0;
	for(int k=0;k<ntokens;++k)
	{
		token=(Token*)array_at(&tokens, k);
		if(!spaceprinted&&token->ws_before)
			STR_APPEND(*str, " ", 1, 1);
		spaceprinted=0;
		const char *kw=keywords[token->type];
		int len;
		if(kw)
		{
			len=strlen(kw);
			STR_APPEND(*str, kw, len, 1);
		}
		else
		{
			int printed=0;
			switch(token->type)
			{
			case T_VAL_C8://TODO multi-character literal
				{
					char s[9]={0};
					memcpy(s, &token->i, 8);
					len=strlen(s);
					len+=!len;
					if(len>1)
						memreverse(s, len, 1);
					char *proc=str2esc(s, len, 0);
					printed=sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", proc);
					free(proc);
				}
				//printed=sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", describe_char((char)token->i));
				break;
			case T_VAL_C32:
				//TODO
				break;
			case T_VAL_I32:
			case T_VAL_I64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%lld", token->i);
				break;
			case T_VAL_U32:
			case T_VAL_U64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%llu", token->i);
				break;
			case T_VAL_F32:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%ff", token->f32);
			case T_VAL_F64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%g", token->f64);
			case T_VAL_STR:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "\"%s\"", token->str);
				break;
			case T_VAL_WSTR:
				//TODO
				break;
			case T_ID:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%s", token->str);
				break;
			case T_INCLUDENAME_STD:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "<%s>", token->str);
				break;
			default:
				break;
			}
			if(printed)
				STR_APPEND(*str, g_buf, printed, 1);
			else
			{
				switch(token->type)
				{
#define		TOKEN(STR, LABEL)	case LABEL:kw=#LABEL;break;
#include	"acc_keywords.h"
#undef		TOKEN
				default:
					kw="T_ILLEGAL";
					break;
				}
				len=strlen(kw);
				STR_APPEND(*str, kw, len, 1);
			}
		}
		if(token->nl_after)
		{
			STR_APPEND(*str, "\n", 1, 1);
			spaceprinted=1;
		}
		else if(token->ws_after)
		{
			STR_APPEND(*str, " ", 1, 1);
			spaceprinted=1;
		}
	}
}
