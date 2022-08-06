#define		ACC_HEADER	"acc.h"
#include	ACC_HEADER
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<math.h>
#include	<time.h>
static const char file[]=__FILE__;

//	#define	PRINT_LEX

	#define		PROFILE_LEXER

#ifdef PROFILE_LEXER
#define			PROF_STAGES\
	PROF_LABEL(START)\
	PROF_LABEL(FINISH)\
	PROF_LABEL(DEFINE)\
	PROF_LABEL(REBALANCE)\
	PROF_LABEL(UNDEF)\
	PROF_LABEL(IFDEF)\
	PROF_LABEL(ELSE)\
	PROF_LABEL(ENDIF)\
	PROF_LABEL(INCLUDE)\
	PROF_LABEL(PRAGMA)\
	PROF_LABEL(ERROR)\
	PROF_LABEL(MACRO_FILE)\
	PROF_LABEL(MACRO_LINE)\
	PROF_LABEL(MACRO_DATE)\
	PROF_LABEL(MACRO_TIME)\
	PROF_LABEL(MACRO_TIMESTAMP)\
	PROF_LABEL(IDENTIFIER_LOOKUP)\
	PROF_LABEL(MACRO_EXPANSION)\
	PROF_LABEL(IDENTIFIER)\
	PROF_LABEL(TOKEN)
enum			ProfilerStage
{
#define			PROF_LABEL(LABEL)	PROF_##LABEL,
	PROF_STAGES
#undef			PROF_LABEL
	PROF_COUNT,
};
const char		*prof_labels[]=
{
#define			PROF_LABEL(LABEL)	#LABEL,
	PROF_STAGES
#undef			PROF_LABEL
};
#undef			PROF_STAGES
int				prof_count[PROF_COUNT]={0};
long long		prof_cycles[PROF_COUNT]={0}, prof_temp=0;
#define			PROF_INIT()		prof_temp=__rdtsc()
#define			PROF(LABEL)		++prof_count[PROF_##LABEL], prof_cycles[PROF_##LABEL]+=__rdtsc()-prof_temp, prof_temp=__rdtsc()
void			prof_end()
{
	long long sum=0;
	int longestLabel=0;
	for(int k=0;k<PROF_COUNT;++k)
		sum+=prof_cycles[k];
	printf("\nPROFILER\nLabel\tvisited\tcycles\tpercentage\tcycles_per_call\n");
	for(int k=0;k<PROF_COUNT;++k)
	{
		int len=strlen(prof_labels[k]);
		if(longestLabel<len)
			longestLabel=len;
	}
	for(int k=0;k<PROF_COUNT;++k)
		printf("%s%*s:\t%d\t%lld\t%.2lf%%\t%lf\n", prof_labels[k], longestLabel-strlen(prof_labels[k]), "", prof_count[k], prof_cycles[k], 100.*prof_cycles[k]/sum, (double)prof_cycles[k]/prof_count[k]);
	printf("\n");
	memset(prof_cycles, 0, sizeof(prof_cycles));
	memset(prof_count, 0, sizeof(prof_count));
}
#else
#define			PROF_INIT()
#define			PROF(...)
#define			prof_end()
#endif


const char	*str_va_args=0, *str_pragma_once=0;
LexedFile	*currentfile=0;
char		*currentdate=0, *currenttime=0, *currenttimestamp=0;
void		lex_error(const char *text, int len, int pos, const char *format, ...);

//string library
Map			strlib={0};
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
typedef struct StringStruct
{
	const char *str;
	size_t len;
} String;
static CmpRes strlib_cmp(const void *key, const void *pair)
{
	String const *s1=(String const*)key;
	const char *s2=*(const char**)pair;
	size_t k;
	char c1;

	for(k=0;k<s1->len&&*s2&&s1->str[k]==*s2;++k, ++s2);

	c1=k<s1->len?s1->str[k]:0;
	return (c1>*s2)-(c1<*s2);

	//int result=strcmp(s1, s2);
	//return (result>0)-(result<0);
}
void		strlib_destructor(void *data)
{
	char **str=(char**)data;
	free(*str);
	*str=0;
}
char*		strlib_insert(const char *str, int len)
{
	String s2={str, len?len:strlen(str)};//str is not always null-terminated, but is sometimes of length 'len' instead
	char *key;
	int found;
	BSTNodeHandle *node;

	if(!strlib.cmp_key)
		map_init(&strlib, sizeof(char*), 0, strlib_cmp, strlib_destructor);

	node=MAP_INSERT(&strlib, &s2, 0, &found);		//search first then allocate
	if(!found)
	{
		key=(char*)malloc(s2.len+1);
		memcpy(key, str, s2.len);
		key[s2.len]=0;
		*(char**)node[0]->data=key;
	}
	return *(char**)node[0]->data;
}


//string manipulation

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
		case 'F'://float literal
			if(F)
			{
				lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
					lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
					lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
					success=0;
					break;
				}
				if(ret->type==NUM_F32||F)
				{
					lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
				lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
				success=0;
				break;
			}
			U=1;
			++*idx;
			continue;
		case 'Z'://ptrdiff_t/size_t
			if(Z)
			{
				lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
					lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
						lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
						success=0;
						break;
					}
				}
			}
			break;
		default:
			if(isalnum(text[*idx])||text[*idx]=='_')
			{
				lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
			lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
				lex_error(text, len, start, "Invalid number literal suffix \'...%.*s\'", *idx+1-start, text+start);
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
			lex_error(text, len, start, "Integer overflow");
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
			lex_error(text, len, start, "Integer overflow");
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
int				codepoint2utf8(int codepoint, char *out)//returns sequence length (up to 4 characters)
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
int				utf8codepoint(const char *in, int *codepoint)//returns sequence length
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
ArrayHandle		esc2str(const char *s, int size)
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
ArrayHandle		str2esc(const char *s, int size)
{
	ArrayHandle ret;
	if(!s)
	{
		printf("Internal error: str2esc: string is null.\n");
		return 0;
	}
	if(!size)
		size=strlen(s);

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

const char*		describe_char(char c)//uses g_buf
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
const char*		lex_tokentype2str(CTokenType tokentype)
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
void			lex_token2buf(Token *token)
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
	case T_VAL_C8://char literal
		{
			ArrayHandle proc;
			char str[9]={0};

			memcpy(str, &token->i, 8);
			int len=strlen(str);
			len+=!len;
			if(len>1)
				memreverse(str, len, 1);
			proc=str2esc(str, len);
			sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", (char*)proc->data);
			array_free(&proc);
		}
		break;
	case T_VAL_CW://wide char literal
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
			ArrayHandle proc=str2esc(utf8, ulen);
			sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", (char*)proc->data);
			array_free(&proc);
		}
		break;
	case T_VAL_STR://string literal
		if(!token->str)
			sprintf_s(g_buf, G_BUF_SIZE, "%s:nullptr", lex_tokentype2str(token->type));
		else
		{
			ArrayHandle proc=str2esc(token->str, strlen(token->str));
			sprintf_s(g_buf, G_BUF_SIZE, "\"%s\"", (char*)proc->data);
			array_free(&proc);
		}
		break;
	case T_VAL_WSTR://wide string literal
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
static ArrayHandle *lex_slots=0;//array of arrays of LexOption's
static int lex_slot_threeway(const void *p1, const void *p2)
{
	const char
		*s1=((LexOption const*)p1)->name,
		*s2=((LexOption const*)p2)->name;

	for(;*s1&&*s2&&*s1==*s2;++s1, ++s2);
	if(!*s1&&!*s2)
		return 0;

	char ac=*s1, bc=*s2;
	if(!ac)
		ac=127;
	if(!bc)
		bc=127;
	return (ac>bc)-(ac<bc);
}
static int lex_slot_less(const void *p1, const void *p2)
{
	const char
		*s1=((LexOption const*)p1)->name,
		*s2=((LexOption const*)p2)->name;

	for(;*s1&&*s2&&*s1==*s2;++s1, ++s2);
	if(!*s1&&!*s2)
		return 0;

	char ac=*s1, bc=*s2;
	if(!ac)
		ac=127;
	if(!bc)
		bc=127;
	return ac<bc;
}
static void	lexer_init()
{
	LexOption *opt;

	lex_slots=(ArrayHandle*)malloc(256*sizeof(ArrayHandle));
	memset(lex_slots, 0, 256*sizeof(ArrayHandle));
	for(int kt=0;kt<T_NTOKENS;++kt)
	{
		const char *kw=keywords[kt];
		if(kw)
		{
			ArrayHandle *slot=lex_slots+(unsigned char)kw[0];
			if(!*slot)
				ARRAY_ALLOC(LexOption, *slot, 0, 0, 0, 0);
			opt=(LexOption*)ARRAY_APPEND(*slot, 0, 1, 1, 0);

			opt->token=kt;
			opt->len=strlen(kw);
			memcpy(opt->val.text, kw, opt->len);
			for(int k2=0;k2<opt->len;++k2)
				opt->mask.text[k2]=0xFF;
			opt->endswithnan=isalnum(kw[0])||kw[0]=='_';
			if(kt==T_DQUOTE_WIDE)
				opt->endswithnan=0;
#ifdef ACC_CPP
			if(kt==T_DQUOTE_RAW)
				opt->endswithnan=0;
#endif
			opt->endswithnd=opt->len==1&&kw[0]=='.';//only the '.' operator should end with a non-digit
			opt->name=kw;
		}
	}
	for(int ks=0;ks<128;++ks)
	{
		ArrayHandle *slot=lex_slots+ks;
		if(*slot)
			isort(slot[0]->data, slot[0]->count, slot[0]->esize, lex_slot_threeway);
			//qsort(slot[0]->data, slot[0]->count, slot[0]->esize, lex_slot_less);
	}

	//DEBUG LEXER
#if 0
	printf("Sorted keywords:\n");
	for(int ks=0;ks<128;++ks)
	{
		ArrayHandle *slot=lex_slots+ks;
		if(*slot)
		{
			for(int k2=0;k2<slot[0]->count;++k2)
			{
				opt=(LexOption*)array_at(slot, k2);
				printf("\t%s\n", opt->name);
			}
		}
	}
#endif
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
	printf("%s(%d:%d) Error: ", currentfile->filename, lineno+1, pos-linestart);
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
	ArrayHandle str;
	const char *cstr;
	char c0, c1;
	
	if(strtype==STR_ID||strtype==STR_RAW)
		cstr=p+k, str=0;
	else
	{
		str=esc2str(p+k, len);
		cstr=(char*)str->data;
		len=(int)str->count;
	}

	token=dlist_push_back(tokens, 0);

	c0=k>0?p[k-1]:0, c1=p[k+len];
	if((c0=='\''||c0=='\"')&&k>1)
		c0=p[k-2];
	if(c1=='\''||c1=='\"')
		c1=p[k+len+1];//lexed text is over-allocated
	
	token->type=toktype;
	token->pos=k;
	token->len=len;
	token->line=lineno;
	token->col=k-linestart;
	token->flags=token_flags(c0, c1);
	
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
		array_free(&str);
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
static CmpRes lexlib_cmp(const void *left, const void *right)
{
	LexedFile const *f1=(LexedFile const*)left, *f2=(LexedFile const*)right;
	return (CmpRes)((f1->filename>f2->filename)-(f1->filename<f2->filename));
}
static void	lexlib_destructor(void *data)
{
	LexedFile *lf=(LexedFile*)data;
	array_free(&lf->text);
	array_free(&lf->tokens);
}
static int	lex(LexedFile *lf)//returns 1 if succeeded
{
	DList tokens={0};//list of Token's
	Reg128 reg={0};
	LexOption *opt;
	LexerState lex_chevron_state=IC_NORMAL;
	BM_DECL;

	BM_START();
	if(lf->filename)
	{
		if(lf->text)
			LOG_ERROR("lex() warning: file was probably already lexed: filename=%p, text=%p", lf->filename, lf->text);
		else
			lf->text=load_text(lf->filename, 16);//look-ahead padding
	}
	if(!lf->text)
	{
		LOG_ERROR("lex() Failed to read file: filename=%p, text=%p", lf->filename, lf->text);
		return 0;
	}

	if(!lex_slots)//init lexer
		lexer_init();

	char *p=(char*)lf->text->data;
	int len=(int)lf->text->count;

	unsigned short data=DUPLET(p[0], p[1]);
	for(int k=0;k<len;++k)//remove esc newlines		who in their right mind would break an identifier or even an operator with an escaped newline?!
	{
		if(data==DUPLET('\\', '\n'))
			p[k]=' ', p[k+1]='\r';//'\r' doesn't break #defines, but adjusts line increment
		data>>=8, data|=(unsigned char)p[k+2]<<8;//text is over-allocated by 16 chars
	}

	dlist_init(&tokens, sizeof(Token), 128, 0);
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
					case NUM_I8:	token->type=T_VAL_C8,	token->i=val.i64;break;
					case NUM_U8:	token->type=T_VAL_UC8,	token->i=val.i64;break;
					case NUM_I16:	token->type=T_VAL_I16,	token->i=val.i64;break;
					case NUM_U16:	token->type=T_VAL_U32,	token->i=val.i64;break;
					case NUM_I32:	token->type=T_VAL_I32,	token->i=val.i64;break;
					case NUM_U32:	token->type=T_VAL_U32,	token->i=val.u64;break;
					case NUM_I64:	token->type=T_VAL_I64,	token->i=val.i64;break;
					case NUM_U64:	token->type=T_VAL_U64,	token->i=val.u64;break;

					case NUM_I128:	token->type=T_VAL_I128,	token->u=val.u64;break;
					case NUM_U128:	token->type=T_VAL_U128,	token->u=val.u64;break;

					case NUM_F32:	token->type=T_VAL_F32,	token->f32=val.f32;break;
					case NUM_F64:	token->type=T_VAL_F64,	token->f64=val.f64;break;
					case NUM_F128:	token->type=T_VAL_F128,	token->f64=val.f64;break;
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
			k+=2;
			for(unsigned short data=DUPLET(p[k], p[k+1]);;)
			{
				if(k>=len)
				{
					lex_error(p, len, k, "Expected \'*/\'");
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
					lineno+=lineinc, lineinc=1, linestart=k;
				}
				else
					++k;
				data>>=8, data|=(unsigned char)p[k+1]<<8;
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
				case T_QUOTE_WIDE:	toktype=T_VAL_CW,	strtype=CHAR_LITERAL;	break;
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
				int start=k+opt->len;
				for(;start<len&&(p[start]==' '||p[start]=='\t'||p[start]=='\r');lineinc+=p[start]=='\r', ++start);
				int end=start+1;
				for(;end<len&&p[end]&&p[end]!='\n';lineinc+=p[end]=='\r', ++end);//BUG: esc nl flags remain in error msg
				lex_push_string(&tokens, T_VAL_STR, STR_LITERAL, p, len, start, end-start, linestart, lineno);
				k=end;
			}
			else
			{
				lex_push_string(&tokens, T_VAL_STR, STR_LITERAL, p, len, k, opt->len, linestart, lineno);
				k+=opt->len;
			}
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
			{
				//if(lex_chevron_state==IC_HASH)
				//	lex_error(p, len, k, "Hash followed by a chevron");
				lex_push_tok(&tokens, opt, p, k, linestart, lineno);
				k+=opt->len;
			}
			lex_chevron_state=IC_NORMAL;
			break;
		case T_INCLUDE:
			if(lex_chevron_state==IC_HASH)
			{
				lex_chevron_state=IC_INCLUDE;
				lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			}
			else
				lex_push_string(&tokens, T_ID, STR_LITERAL, p, len, k, opt->len, linestart, lineno);
			k+=opt->len;
			break;
		default:
			lex_chevron_state=IC_NORMAL;
			lex_push_tok(&tokens, opt, p, k, linestart, lineno);
			k+=opt->len;
			break;
		}
	}

	dlist_appendtoarray(&tokens, &lf->tokens);
	dlist_clear(&tokens);
	BM_FINISH("Lex \'%s\':\t%lfms\n", lf->filename, bm_t1);

	BM_START();
	map_rebalance(&strlib);
	BM_FINISH("Rebalance strlib:\t%lfms\n", bm_t1);
	return 1;
}

//preprocessor
void pp_error(Token const *token, const char *format, ...)
{
	va_list args;
	if(token)
		printf("%s(%d) ", currentfile->filename, token->line+1);
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
static void token_append2str(ArrayHandle *dst, Token const *token)
{
	int len;
	const char *kw=keywords[token->type];
	if(kw)
		STR_APPEND(*dst, kw, strlen(kw), 1);
	else
	{
		int printed=0;
		switch(token->type)
		{
		case T_VAL_C8:
		case T_VAL_UC8://TODO multi-character literal
			{
				ArrayHandle proc;
				char s[9]={0};
				memcpy(s, &token->i, 8);
				len=(int)strlen(s);
				if(len>1)
					memreverse(s, len, 1);
				len+=!len;
				proc=str2esc(s, len);
				printed=sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", (char*)proc->data);
				free(proc);
			}
			break;
		case T_VAL_CW:
			//TODO
			break;
		case T_VAL_I16:case T_VAL_U16:
		case T_VAL_I32:
		case T_VAL_I64:
			printed=sprintf_s(g_buf, G_BUF_SIZE, "%lld", token->i);
			break;
		case T_VAL_U32:
		case T_VAL_U64:
			printed=sprintf_s(g_buf, G_BUF_SIZE, "%llu", token->u);
			break;
		case T_VAL_F32:
			printed=sprintf_s(g_buf, G_BUF_SIZE, "%f", token->f32);
			break;
		case T_VAL_F64:
			printed=sprintf_s(g_buf, G_BUF_SIZE, "%g", token->f64);
			break;
		case T_VAL_STR:
			{
				ArrayHandle proc;
				proc=str2esc(token->str, token->len);
				printed=sprintf_s(g_buf, G_BUF_SIZE, "\"%s\"", (char*)proc->data);
				array_free(&proc);
			}
			break;
		case T_VAL_WSTR:
			//TODO
			break;
		case T_ID:
			printed=sprintf_s(g_buf, G_BUF_SIZE, "%s", token->str);
			break;
		default:
			pp_error(token, "Cannot token-paste %s", lex_tokentype2str(token->type));
			break;
		}
		if(printed)
			STR_APPEND(*dst, g_buf, printed, 1);
		else//FIXME: better conversion to string
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
			STR_APPEND(*dst, kw, len, 1);
		}
	}
#if 0
	if(t->type==T_LEXME||t->synth)//X  synth doesn't mean string
		STR_APPEND(*dst, t->str, t->len, 1);
	else
	{
		int isstr=t->type==T_VAL_STR||t->type==T_VAL_WSTR,
			ischar=t->type==T_VAL_C8||t->type==T_VAL_UC8||t->type==T_VAL_CW||t->type==T_VAL_UCW;
		if(isstr)
			STR_APPEND(*dst, "\"", 1, 1);
		if(ischar)
			STR_APPEND(*dst, "\'", 1, 1);
		if(t->type==T_ID)
			STR_APPEND(*dst, t->str, t->len, 1);
		else
			STR_APPEND(*dst, currentfile+t->pos, t->len, 1);//X  what if the token is not from current file?
		if(ischar)
			STR_APPEND(*dst, "\'", 1, 1);
		if(isstr)
			STR_APPEND(*dst, "\"", 1, 1);
	}
#endif
}
static void token_stringize(Token const *tokens, int ntokens, int first, int count, const char *srctext, size_t srctext_len, Token *out)
{
	Token const *token;
	ArrayHandle text;

	STR_ALLOC(text, 0);
	if(count<=0)
	{
		if(first<ntokens)
		{
			token=tokens+first;
			out->type=T_VAL_STR;
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
		out->type=T_VAL_STR;
		out->pos=start;
		out->line=token->line;
		out->col=token->col;
		int len=end-start;
		if(len>0)
		{
			ARRAY_APPEND(text, 0, 0, 1, len+1);
			//STR_APPEND(text, 0, len, 1);
			int ks=start, kd=0;
			for(;ks<end;)
			{
				if(ks>=(int)srctext_len)
				{
					pp_error(token, "Stringize operator out of bounds of source text: ks=%d, len=%d", ks, (int)srctext_len);
					break;
				}
				char c=srctext[ks];
				if(c=='\r')
				{
					++ks;
					continue;
				}
				unsigned short word=DUPLET(c, srctext[ks+1]);
				if(word==DUPLET('/', '/'))
				{
					ks+=2;
					for(;ks<end&&srctext[ks]!='\n';++ks);
					continue;
				}
				if(word==DUPLET('/', '*'))
				{
					ks+=2;
					for(;ks<end;++ks)
					{
						word=DUPLET(srctext[ks], srctext[ks+1]);
						if(word==DUPLET('*', '/'))
						{
							ks+=2;
							break;
						}
					}
					continue;
				}
				STR_APPEND(text, srctext+ks, 1, 1);
				//text->data[kd]=srctext[ks];
				//*(char*)array_at(&text, kd)=srctext[ks];
				++ks, ++kd;
			}
			//text->count=kd;
			//STR_FIT(text);
		}
		char c0=start>0?srctext[start-1]:0, c1=srctext[end];
		out->flags=token_flags(c0, c1);
	}
	out->len=(int)text->count;
	out->str=strlib_insert((char*)text->data, (int)text->count);
	array_free(&text);
}
static void token_paste(Token const *t1, Token const *t2, const char *srctext, ArrayHandle *dst)//concatenate
{
	if(!t1||!t2)
	{
		if(t1)
			array_insert(dst, dst[0]->count, t1, 1, 1, 0);
		if(t2)
			array_insert(dst, dst[0]->count, t2, 1, 1, 0);
	}
//	else if((t1->type==T_COMMA||t1->type==T_VAL_STR||t1->type==T_VAL_WSTR)&&(t2->type==T_VAL_STR||t2->type==T_VAL_WSTR))
	else if((t1->type==T_VAL_STR||t1->type==T_VAL_WSTR)&&(t2->type==T_VAL_STR||t2->type==T_VAL_WSTR))//string literals are concatenated by the parser
	{
		if(t1->type!=t2->type)
		{
			const char n[]="a string literal", w[]="a wide string literal";
			pp_error(t1, "Cannot token-paste %s and %s", t1->type==T_VAL_STR?n:w, t2->type==T_VAL_STR?n:w);
			return;
		}
		array_insert(dst, dst[0]->count, t1, 1, 1, 0);
		array_insert(dst, dst[0]->count, t2, 1, 1, 0);
	}
	else
	{
		LexedFile lf2={0};
		STR_ALLOC(lf2.text, 0);
		token_append2str(&lf2.text, t1);
		token_append2str(&lf2.text, t2);
		ARRAY_APPEND(lf2.text, 0, 0, 1, 16);//look-ahead pad
		lf2.tokens=*dst;
		int success=lex(&lf2);
		*dst=lf2.tokens;
		array_free(&lf2.text);
#if 0
		Token *token=array_insert(dst, dst[0]->count, 0, 1, 1, 0);//FIXME wrong file?
		token->type=T_LEXME;
		token->ws_before=t1->ws_before;
		token->nl_before=t1->nl_before;
		token->ws_after=t2->ws_after;
		token->nl_after=t2->nl_after;
		token->synth=1;
		token->str=strlib_insert((char*)text->data, text->count);
#endif
	}
}

static CmpRes macro_cmp(const void *left, const void *right)
{
	const char **s1=(const char**)left, **s2=(const char**)right;
	return (CmpRes)((*s1>*s2)-(*s1<*s2));
}
static void macro_destructor(void *data)
{
	Macro *macro=(Macro*)data;
	array_free(&macro->tokens);
}
static void macros_debugprint_callback(BSTNodeHandle *node, int depth)
{
	Macro const *macro=(Macro const*)node[0]->data;
	printf("%4d %*s%p %s\n", depth, depth, "", macro->name, macro->name);
}
void		macros_debugprint(MapHandle macros)
{
	printf("All macros:\n");
	MAP_DEBUGPRINT(macros, macros_debugprint_callback);
	printf("\n");
}

typedef struct MacroArgStruct
{
	const char *name;//key
	size_t idx;//val
} MacroArg;
static int macro_arg_sort_threeway(const void *left, const void *right)
{
	MacroArg *a1=(MacroArg*)left, *a2=(MacroArg*)right;
	return (a1->name>a2->name)-(a1->name<a2->name);//sort by pointer value
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
		arg=(MacroArg*)ARRAY_APPEND(*argnames, 0, 1, 1, 0);
		arg->name=name;
		arg->idx=nargs;
	}
	return duplicate;
}
static int macro_arg_search_threeway(const void *left, const void *right)
{
	MacroArg const *a1=(MacroArg const*)left;
	const char **a2=(const char**)right;
	return (a1->name>*a2)-(a1->name<*a2);
}
int macro_define(Macro *dst, LexedFile *srcfile, Token const *tokens, int count)//tokens points at after '#define', undef macro on error
{
	int ret, kt, duplicate;
	Token const *token;
	Token *token2;
	ArrayHandle argnames;//array of MacroArg's

	if(!str_va_args)
		str_va_args=strlib_insert("__VA_ARGS__", 0);

	ARRAY_ALLOC(MacroArg, argnames, 0, 0, 0, 0);//no destructor needed

	if(tokens->type!=T_ID)
	{
		pp_error(tokens, "Expected an identifier");
		ret=0;
		goto exit;
	}
	dst->name=tokens->str;//<- this assignment is redundant
	dst->srcfile=srcfile;
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
					if(duplicate)//unreachable
					{
						pp_error(token, "Ellipsis (...) was already declared.");
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
				++dst->nargs;

				++kt;//skip ID
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
	count-=kt;
	ARRAY_ALLOC(Token, dst->tokens, tokens+kt, count, 0, 0);

	if(argnames->count)//translate macro args
	{
		isort(argnames->data, argnames->count, argnames->esize, macro_arg_sort_threeway);//sort args by name pointer value for fast lookup of arg index
		for(kt=0;kt<count;++kt)
		{
			int idx;
			MacroArg *arg;

			token2=(Token*)array_at(&dst->tokens, kt);
			if(token2->type==T_ID)
			{
				if(binary_search(argnames->data, argnames->count, argnames->esize, macro_arg_search_threeway, &token2->str, &idx))
				{
					token2->type=T_MACRO_ARG;
					arg=(MacroArg*)array_at(&argnames, idx);
					token2->i=arg->idx;
				}
			}
			else if(token2->type==T_VA_ARGS)
			{
				if(!binary_search(argnames->data, argnames->count, argnames->esize, macro_arg_search_threeway, &str_va_args, &idx))
				{
					pp_error(token2, "__VA_ARGS__ is reserved for variadic macros.");
					ret=0;
					goto exit;
				}
				token2->type=T_MACRO_ARG;
				arg=(MacroArg*)array_at(&argnames, idx);
				token2->i=arg->idx;
			}
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
	array_free(&argnames);
	return ret;
}


//macro expansion
#define	TOKENS_AT(TOKENS, IDX)			((Token*)array_at(&(TOKENS), IDX))
#define	TOKENS_AT_CONST(TOKENS, IDX)	((Token const*)array_at_const(&(TOKENS), IDX))
static void macro_arg_destructor(void *data)
{
	ArrayHandle *tokens=(ArrayHandle*)data;
	array_free(tokens);
}
static int	macro_find_call_extent(Macro *macro, ArrayHandle tokens, int start, int *ret_len, ArrayHandle *args)//start points at macro name in source call
{
	Token *token;

	if(!*args)
		ARRAY_ALLOC(ArrayHandle, *args, 0, 0, 0, macro_arg_destructor);
	else//clear args
		array_clear(args);
	
	if(macro->nargs==MACRO_NO_ARGLIST)
	{
		*ret_len=1;
		return 1;
	}
	int ntokens=(int)tokens->count;
	if(start+1>=ntokens)
	{
		*ret_len=1;
		return 2;//macro has an arglist, called without an arglist, not an error, simply leave the identifier as it is
	}
	token=TOKENS_AT(tokens, start+1);
	if(token->type!=T_LPR)
	{
		*ret_len=1;
		return 2;//macro has an arglist, called without an arglist, not an error, simply leave the identifier as it is
	}
	int k0=start+2, end=k0, nargs=0, level=1;
	for(;end<ntokens&&level>0;++end)
	{
		token=TOKENS_AT(tokens, end);
		level+=(token->type==T_LPR)-(token->type==T_RPR);
		if(level==1&&token->type==T_COMMA||!level&&token->type==T_RPR)//end of an argument expression
		{
			ArrayHandle *dsttokens;
			int count=end-k0;
			token=TOKENS_AT(tokens, k0);
			if(macro->is_va&&nargs>macro->nargs)//if macro is variadic, append extra args with commas as the last arg
			{
				int withcomma=k0>0&&token[-1].type==T_COMMA;
				dsttokens=array_back(args);
				ARRAY_APPEND(*dsttokens, token-withcomma, count+withcomma, 1, 0);
			}
			else if(count||nargs)//if macro is called with empty parens, then nargs==0
			{
				dsttokens=ARRAY_APPEND(*args, 0, 1, 1, 0);
				ARRAY_ALLOC(Token, *dsttokens, token, count, 0, 0);
				++nargs;
				if(!macro->is_va&&macro->nargs<nargs)
					break;
			}
			k0=end+1;
		}
	}
	*ret_len=end-start;
	if(level)
	{
		pp_error(token, "%d unmatched parentheses.", level);
		return 0;
	}
	if(macro->is_va)
	{
		if(nargs<macro->nargs)
		{
			token=TOKENS_AT(tokens, start);
			pp_error(token, "Macro \'%s\' needs >= %d args, got %d.", macro->name, macro->nargs, nargs);
			return 0;
		}
	}
	else
	{
		if(nargs!=macro->nargs)
		{
			token=TOKENS_AT(tokens, start);
			pp_error(token, "Macro \'%s\' needs %d args, got %d.", macro->name, macro->nargs, nargs);
			return 0;
		}
	}
	return 1;
}
//macrodefinition: array of tokens
//kt: index of stringize token '#'
//args2: array of arrays of tokens
//out: result token
static int	macro_stringize(Macro *macro, int kt, ArrayHandle args2, Token *out)
{
	const char *srctext;
	size_t srctext_len;
	Token *token, *next;

	if(macro->srcfile)
	{
		srctext=(char*)macro->srcfile->text->data;
		srctext_len=macro->srcfile->text->count;
	}
	else
	{
		srctext=0;
		srctext_len=0;
	}
	token=TOKENS_AT(macro->tokens, kt);
	if(kt+1>(int)macro->tokens->count)
	{
		pp_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition	DUPLICATE
		return 0;
	}
	next=TOKENS_AT(macro->tokens, kt+1);
	if(next->type!=T_MACRO_ARG)
	{
		pp_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition
		return 0;
	}
	ArrayHandle *arg=(ArrayHandle*)array_at(&args2, (size_t)next->i);
	token=(Token*)arg[0]->data;
	token_stringize(token, arg[0]->count, 0, arg[0]->count, srctext, srctext_len, out);//TODO: inline call
	return 1;
}
//token paste '##': concatenate tokens
//t_left: first token
//macrodefinition: array of tokens
//kt: index of '##' followed by second token
//args2: array of arrays of tokens
//dst: result array of tokens
static void	macro_paste(Token const *t_left, Macro *macro, int *kt, ArrayHandle args2, ArrayHandle *dst)
{
	const char *srctext=macro->srcfile?macro->srcfile->text->data:0;
	Token *second=TOKENS_AT(macro->tokens, *kt+1);
	if(second->type==T_MACRO_ARG)
	{
		ArrayHandle *callarg=(ArrayHandle*)array_at(&args2, (size_t)second->i);//callarg is array of tokens
		if(callarg&&callarg[0]->count)
		{
			token_paste(t_left, (Token*)callarg[0]->data, srctext, dst);
			if(callarg[0]->count>1)
			{
				int count=callarg[0]->count-1;
				ARRAY_APPEND(*dst, 0, count, 1, 0);
				Token *tsrc=TOKENS_AT(*callarg, 1), *tdst=TOKENS_AT(*dst, dst[0]->count-1);
				memcpy(tdst, tsrc, count*sizeof(Token));
			}
		}
		else
			token_paste(t_left, 0, srctext, dst);
	}
	else if(second->type==T_HASH)
	{
		Token str={0};
		++*kt;
		macro_stringize(macro, *kt, args2, &str);
		token_paste(t_left, &str, srctext, dst);
	}
	else
		token_paste(t_left, second, srctext, dst);
}

typedef struct MacroCallStruct
{
	Macro *macro;
	ArrayHandle
		args,//array of arrays of tokens
		remainder;//array of tokens appended to dst after expansion
	int kt,//macro definition index
		kt2,//macro call argument index, use on args when macrodefinition[kt] is an arg
		expansion_start,//dst index
		done;
} MacroCall;
static void macrocall_destructor(void *data)
{
	MacroCall *call=(MacroCall*)data;
	array_free(&call->args);
	array_free(&call->remainder);
}
static void macro_expand_file		(Token const *src, Token *dst)
{
	memcpy(dst, src, sizeof(Token));
	dst->type=T_VAL_STR;
	dst->len=strlen(currentfile->filename);
	//dst->str=strlib_insert(currentfile->filename, dst->len);//currentfilename should already be in strlib
	dst->str=currentfile->filename;
	dst->synth=1;
}
static void macro_expand_line		(Token const *src, Token *dst)
{
	memcpy(dst, src, sizeof(Token));
	dst->type=T_VAL_I32;
	dst->i=src->line;
	dst->synth=1;
}
static void macro_expand_date		(Token const *src, Token *dst)
{
	memcpy(dst, src, sizeof(Token));
	dst->type=T_VAL_STR;
	dst->str=currentdate;
	dst->synth=1;
}
static void macro_expand_time		(Token const *src, Token *dst)
{
	memcpy(dst, src, sizeof(Token));
	dst->type=T_VAL_STR;
	dst->str=currenttime;
	dst->synth=1;
}
static void macro_expand_timestamp	(Token const *src, Token *dst)
{
	memcpy(dst, src, sizeof(Token));
	dst->type=T_VAL_STR;
	dst->str=currenttimestamp;
	dst->synth=1;
}
static int	macro_expand(MapHandle macros, Macro *macro, ArrayHandle src, int *ks, ArrayHandle *dst)//returns ntokens in call
{
	int len;//initial macro call length
	int success;
	DList context;//a stack of macro calls
	MacroCall *topcall;
	Macro *m2;
	Token *token;

	dlist_init(&context, sizeof(MacroCall), 4, macrocall_destructor);
	topcall=(MacroCall*)dlist_push_back(&context, 0);
	topcall->macro=macro;
	success=macro_find_call_extent(macro, src, *ks, &len, &topcall->args);
	if(!success)
	{
		*ks+=len;
		dlist_clear(&context);
		return len;
	}
	if(success==2)
	{
		token=TOKENS_AT(src, *ks);
		ARRAY_APPEND(*dst, token, 1, 1, 0);
		*ks+=len;
		dlist_clear(&context);
		return len;
	}
	
	ARRAY_APPEND(*dst, 0, 0, 1, len);

	while(context.nobj)
	{
	macro_expand_again:
		topcall=(MacroCall*)dlist_back(&context);
		if(topcall->done)
		{
			dlist_pop_back(&context);
			continue;
		}
		m2=topcall->macro;
		ArrayHandle definition=m2->tokens;
		if(!definition)
		{
			dlist_pop_back(&context);
			continue;
		}
		while(topcall->kt<(int)definition->count)
		{
			token=TOKENS_AT(definition, topcall->kt);
			if(token->type==T_HASH)//stringize call-argument
			{
				Token *out=ARRAY_APPEND(*dst, 0, 1, 1, 0);
				macro_stringize(m2, topcall->kt, topcall->args, out);//X  check if next is concatenate
				topcall->kt+=2;
			}
			else if(topcall->kt+1<(int)definition->count&&TOKENS_AT(definition, topcall->kt+1)->type==T_CONCATENATE)//anything can be concatenated
			{
				++topcall->kt;//skip token, now points at CONCATENATE
				if(topcall->kt+1>=(int)definition->count)
				{
					Token *next=TOKENS_AT(definition, topcall->kt);
					pp_error(next, "Token paste operator cannot be at the end of macro.");//error in definition
					break;
				}
				Token const *t_left=0;
				if(token->type==T_MACRO_ARG)
				{
					ArrayHandle *callarg2=(ArrayHandle*)array_at(&topcall->args, (size_t)token->i);
					if(callarg2&&*callarg2)
					{
						if(callarg2[0]->count>1)
						{
							int count=callarg2[0]->count-1;
							ARRAY_APPEND(*dst, callarg2[0]->data, count, 1, 0);
						}
						if(callarg2[0]->count>0)
							t_left=(Token const*)array_back(callarg2);
					}
				}
				else if(token->type==T_COMMA)//comma with __VA_ARGS__ extension
				{
					Token *second=TOKENS_AT(definition, topcall->kt+1);
					if(m2->is_va&&second->type==T_MACRO_ARG&&second->i==m2->nargs)
					{
						ArrayHandle *callarg=(ArrayHandle*)array_at(&topcall->args, (size_t)second->i);//callarg is array of tokens
						if(callarg&&*callarg)//don't append the comma if __VA_ARGS__ is empty
						{
							ARRAY_APPEND(*dst, token, 1, 1, 0);
							ARRAY_APPEND(*dst, callarg[0]->data, callarg[0]->count, 1, 0);
						}
						topcall->kt+=2;//skip CONCATENATE and MACRO_ARG
						continue;
					}
				}
				else
					t_left=token;
				macro_paste(t_left, m2, &topcall->kt, topcall->args, dst);
				topcall->kt+=2;//skip CONCATENATE and MACRO_ARG
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
					token=TOKENS_AT(*dst, dst[0]->count);
					macro_paste(token, m2, &topcall->kt, topcall->args, dst);
				}
				else
					macro_paste(0, m2, &topcall->kt, topcall->args, dst);//
				++dst[0]->count;
				topcall->kt+=2;
			}
			else if(token->type==T_MACRO_ARG)//normal macro call arg
			{
				ArrayHandle *arg=(ArrayHandle*)array_at(&topcall->args, (size_t)token->i);
				if(arg&&*arg)
				{
					for(;topcall->kt2<(int)arg[0]->count;)//copy tokens from arg, while checking the arg for macro calls
					{
						token=TOKENS_AT(*arg, topcall->kt2);
						if(token->type==T_ID)
						{
							BSTNodeHandle *result=MAP_FIND(macros, &token->str);
							if(result)
							{
								Macro *macro3=(Macro*)result[0]->data;
								int len2=0;
								ArrayHandle args=0;
								success=macro_find_call_extent(macro3, *arg, topcall->kt2, &len2, &args);
								if(success==2)
								{
									array_free(&args);
									ARRAY_APPEND(*dst, token, 1, 1, 0);
								}
								else if(success)
								{
									if(macro3->tokens)
										ARRAY_APPEND(*dst, 0, 0, 1, macro3->tokens->count);
									topcall->kt2+=len2;//advance top.kt2 by call length because it will return here

									topcall=dlist_push_back(&context, 0);//request to expand this argument
									topcall->macro=macro3;
									topcall->args=args;
									topcall->kt2=0;
									topcall->expansion_start=dst[0]->count;
								
									goto macro_expand_again;//should resume here after macro is expanded
								}
							}
						}
						token=TOKENS_AT(*arg, topcall->kt2);
						ARRAY_APPEND(*dst, token, 1, 1, 0);
						++topcall->kt2;
					}
				}
				topcall->kt2=0;//reset arg index when done
				++topcall->kt;
			}
			else//copy token from macro definition
			{
				ARRAY_APPEND(*dst, token, 1, 1, 0);
				++topcall->kt;
			}
		}//end while

		if(topcall->remainder)
			ARRAY_APPEND(*dst, topcall->remainder->data, topcall->remainder->count, 1, 0);

		for(int kd2=topcall->expansion_start;kd2<(int)dst[0]->count;++kd2)//check the new expansion for macro calls with recursion guard
		{
			token=TOKENS_AT(*dst, kd2);
			switch(token->type)
			{
			case T_ID:
				{
					BSTNodeHandle *result=MAP_FIND(macros, &token->str);
					if(result)
					{
						Macro *macro3;
						DListIterator it;
						MacroCall *call;

						macro3=(Macro*)result[0]->data;
						dlist_last(&context, &it);//recursion guard: iterate through previous unfinished calls, if macro appeared before: don't expand macro
						while(dlist_it_dec(&it))//start with previous call
						{
							call=(MacroCall*)dlist_it_deref(&it);
							if(call->macro->name==macro3->name)
							{
								pp_error(token, "Stopped expanding cyclic macro \'%s\'", token->str);
								goto macro_expand_skip;
							}
						}

						if(macro3->nargs!=MACRO_NO_ARGLIST)//write to dst from src till matching closing parenthesis
						{
							int level, kd3;

							level=0, kd3=0;
							for(;kd3<(int)dst[0]->count;++kd3)
							{
								Token *token3=TOKENS_AT(*dst, kd3);
								level+=(token3->type==T_LPR)-(token3->type==T_RPR);
							}
							if(level>0)
							{
								for(;*ks+len<(int)src->count;++len, ++kd3)
								{
									Token *token3=TOKENS_AT(src, *ks+len);
									level+=(token3->type==T_LPR)-(token3->type==T_RPR);
									ARRAY_APPEND(*dst, token3, 1, 1, 0);
									if(!level)//should be below the loop body, for a 'do while' behavior
									{
										++kd3;//skip RPR
										break;
									}
								}
							}
						}
						
						ArrayHandle args=0;
						int len2;
						success=macro_find_call_extent(macro3, *dst, kd2, &len2, &args);
						if(success==1)
						{
							if(macro3->tokens)
								ARRAY_APPEND(*dst, 0, 0, 1, macro3->tokens->count);
							topcall->done=1;//keep previous macro calls, to guard against infinite cyclic expansion

							topcall=dlist_push_back(&context, 0);//request to expand this argument
							topcall->macro=macro3;
							topcall->args=args;
							topcall->kt2=0;
							topcall->expansion_start=dst[0]->count;

							kd2+=len2;
							if(kd2<(int)dst[0]->count)//go back and save remainder
							{
								token=TOKENS_AT(*dst, kd2);
								ARRAY_ALLOC(Token, topcall->remainder, token, dst[0]->count-kd2, 0, 0);
								dst[0]->count=kd2-len2;
							}
							goto macro_expand_again;
						}
					macro_expand_skip:
						;
					}
				}
				break;
			case T_MACRO_FILE:		macro_expand_file(token, (Token*)ARRAY_APPEND(*dst, 0, 1, 1, 0));		break;
			case T_MACRO_LINE:		macro_expand_line(token, (Token*)ARRAY_APPEND(*dst, 0, 1, 1, 0));		break;
			case T_MACRO_DATE:		macro_expand_date(token, (Token*)ARRAY_APPEND(*dst, 0, 1, 1, 0));		break;
			case T_MACRO_TIME:		macro_expand_time(token, (Token*)ARRAY_APPEND(*dst, 0, 1, 1, 0));		break;
			case T_MACRO_TIMESTAMP:	macro_expand_timestamp(token, (Token*)ARRAY_APPEND(*dst, 0, 1, 1, 0));	break;
			}
		}//end for (post expansion loop)
		dlist_pop_back(&context);
	}//end while (big expansion loop)
	*ks+=len;
	return len;
}


//preprocessor expr eval
static MapHandle eval_macros=0;
static ArrayHandle eval_tokens=0;
static int
	eval_idx=0,//points at next token
	eval_end=0,
	eval_syntharray=0;//set to true after macro expansion (means the tokens array was replaced, and needs to be freed at the end)
static Token const *eval_token=0;//current token, check for nullptr before reading
static CTokenType eval_type=T_IGNORED;
#define EVAL_DEREF()	TOKENS_AT_CONST(eval_tokens, eval_idx)
static void eval_next()//check if eval_token == nullptr
{
	int inc=eval_idx<eval_end;
	if(inc)
	{
		eval_token=EVAL_DEREF();
		++eval_idx;
	}
	else
		eval_token=0;
	//return eval_idx<eval_end;//return value: true if there are tokens after the newly read token, not always needed
}
static long long eval_ternary();
static long long eval_unary()
{
again:
	if(!eval_token)
	{
		pp_error(eval_token, "Expected an expression.");
		return 0;
	}
	switch(eval_token->type)
	{
	case T_ID://must be a macro
		{
			BSTNodeHandle *node;
			Macro *macro;
			ArrayHandle dst;
			
			//if(!strcmp(currentfile->filename, "D:/Programs/msys2/mingw64/include/_mingw.h"))//MARKER
			//	eval_token=eval_token;

			node=MAP_FIND(eval_macros, &eval_token->str);
			if(!node)//undefined macros evaluate to zero
			{
				eval_next();//skip ID
				return 0;
			}
			ARRAY_ALLOC(Token, dst, 0, 0, 0, 0);
			eval_idx-=eval_idx>0;//eval_idx points at next token
			macro=(Macro*)node[0]->data;
			macro_expand(eval_macros, macro, eval_tokens, &eval_idx, &dst);
			if(eval_idx<eval_end)
			{
				eval_token=EVAL_DEREF();
				ARRAY_APPEND(dst, eval_token, eval_end-eval_idx, 1, 0);
			}
			if(eval_syntharray)
				array_free(&eval_tokens);
			eval_tokens=dst;
			eval_idx=0;
			eval_end=dst->count;
			eval_syntharray=1;
			eval_next();
			return eval_unary();
		}
		break;
	case T_VAL_I32:
	case T_VAL_I64:
		{
			long long result=eval_token->i;
			eval_next();
			return result;
		}
		break;
	case T_VAL_U32:
	case T_VAL_U64:
		{
			unsigned long long result=eval_token->u;//TODO return type: struct Number
			eval_next();
			return result;
		}
		break;
	case T_LPR:
		{
			//Token const *t2=eval_token;//MARKER
			//int idx=eval_idx;

			eval_next();//skip LPR
			long long result=eval_ternary();
			if(!eval_token||eval_token->type!=T_RPR)
				pp_error(eval_token, "Missing closing parenthesis.");
			eval_next();//skip RPR
			return result;
		}
		break;
	case T_DEFINED:
		{
			BSTNodeHandle *node;
			int nparens=0;
			eval_next();//skip DEFINED
			while(eval_token&&eval_token->type==T_LPR)//skip and count opening parentheses
			{
				++nparens;
				eval_next();
			}
			if(eval_token->type!=T_ID)
			{
				pp_error(eval_token, "Expected an identifier.");
				return 0;
			}
			node=MAP_FIND(eval_macros, &eval_token->str);
			eval_next();//skip ID
			if(nparens>0)
			{
				while(nparens>0&&eval_token&&eval_token->type==T_RPR)//skip and count closing parentheses
				{
					--nparens;
					eval_next();
				}
				if(nparens>0)
				{
					pp_error(eval_token, "Missing closing parenthesis \')\'.");
					return 0;
				}
			}
			return node!=0;
		}
		break;
	case T_PLUS:
		eval_next();
		goto again;
	case T_MINUS:
		eval_next();
		return -eval_unary();
	case T_EXCLAMATION:
		eval_next();
		return !eval_unary();
	case T_TILDE:
		eval_next();
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
		eval_next();//skip operator
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
		eval_next();//skip operator
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
		eval_next();//skip operator
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
		eval_next();//skip operator
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
		eval_next();//skip operator
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
		eval_next();//skip operator
		result&=eval_eq();
	}
	return result;
}
static long long eval_xor()
{
	long long result=eval_and();
	while(eval_token&&eval_token->type==T_CARET)
	{
		eval_next();//skip operator
		result^=eval_and();
	}
	return result;
}
static long long eval_or()
{
	long long result=eval_xor();
	while(eval_token&&eval_token->type==T_VBAR)
	{
		eval_next();//skip operator
		result|=eval_xor();
	}
	return result;
}
static long long eval_logic_and()
{
	long long result=eval_or();
	while(eval_token&&eval_token->type==T_LOGIC_AND)
	{
		eval_next();//skip operator
		long long r2=eval_or();
		result=result&&r2;//beware of short-circuit
	}
	return result;
}
static long long eval_logic_or()
{
	long long result=eval_logic_and();
	while(eval_token&&eval_token->type==T_LOGIC_OR)
	{
		eval_next();//skip operator
		long long r2=eval_logic_and();
		result=result||r2;//beware of short-circuit
	}
	return result;
}
static long long eval_ternary()
{
	long long result=eval_logic_or();
	if(eval_token&&eval_token->type==T_QUESTION)
	{
		eval_next();//skip QUESTION
		if(result)
		{
			result=eval_ternary();//true branch
			if(!eval_token||eval_token->type!=T_COLON)
				pp_error(eval_token, "Expected a colon \':\'.");
			eval_ternary();//skip false branch
		}
		else
		{
			eval_ternary();//skip true branch
			if(!eval_token||eval_token->type!=T_COLON)
				pp_error(eval_token, "Expected a colon \':\'.");
			result=eval_ternary();//false branch
		}
	}
	return result;
}
static long long eval_expr(ArrayHandle tokens, int start, int end, MapHandle macros)
{
	if(start>=end)
	{
		Token const *token=0;
		if(start<(int)tokens->count)
			token=(Token const*)array_at_const(&tokens, start);
		pp_error(token, "Expected an expression.");
		return 0;
	}
	eval_macros=macros;
	eval_tokens=tokens;
	eval_idx=start;
	eval_end=end;
	eval_syntharray=0;
	eval_next();
	long long result=eval_ternary();
	if(eval_syntharray)
		array_free(&eval_tokens);
	return result;
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
	array_free(&filename);
	return str;
}
static char*	find_include(const char *includename, int custom, ArrayHandle includepaths)
{
	char *filename=0;
	int inclen=strlen(includename);
	if(custom)
	{
		int size=strlen(currentfile->filename);//relative path
		int k=size-1;
		for(;k>=0;--k)
		{
			char c=currentfile->filename[k];
			if(c=='/'||c=='\\')
				break;
		}
		++k;//include the slash
		filename=test_include(currentfile->filename, k, includename, inclen);
		if(filename)
			return filename;
	}
	for(int ki=0;ki<(int)includepaths->count;++ki)
	{
		ArrayHandle const *path=(ArrayHandle const*)array_at(&includepaths, ki);
		filename=test_include(path[0]->data, path[0]->count, includename, inclen);
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
static int		skip_till_newline(ArrayConstHandle tokens, int k)
{
	for(;k<(int)tokens->count&&((Token const*)array_at_const(&tokens, k))->type!=T_NEWLINE;++k);
	return k;
}
static int		skip_block(MapHandle macros, ArrayHandle tokens, int *k, int lastblock)
{
	int k0, ntokens, level;
	Token const *token;

	k0=*k;
	ntokens=tokens->count;
	level=1;
	while(*k<ntokens)
	{
		token=(Token const*)array_at_const(&tokens, *k);
		if(token->type==T_HASH)
		{
			++*k;
			token=(Token const*)array_at_const(&tokens, *k);
			++*k;//k points at next token
			switch(token->type)
			{
			case T_IF:case T_IFDEF:case T_IFNDEF:
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
		}
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


//char *DEBUG_filename=0;//MARKER
ArrayHandle		preprocess(const char *filename, MapHandle macros, ArrayHandle includepaths, MapHandle lexlib)
{
	LexedFile *lf;
	char *unique_fn;
	DList tokens, bookmarks;
	Bookmark *bm;
	Token *token;
	Macro *macro;
	ArrayHandle ret;
	long long bytes_processed;
	int success;
	BM_DECL;
	
	BM_START();
	PROF_INIT();
	if(!filename)
	{
		pp_error(0, "Filename is not provided");
		return 0;
	}
	if(!str_pragma_once)
		str_pragma_once=strlib_insert("once", 0);

	if(!macros->cmp_key)
		MAP_INIT(macros, char*, Macro, macro_cmp, macro_destructor);

	if(!includepaths)
		ARRAY_ALLOC(ArrayHandle, includepaths, 0, 0, 0, 0);

	if(!lexlib->key_size)
		MAP_INIT(lexlib, const char*, LexedFile, lexlib_cmp, lexlib_destructor);

	unique_fn=strlib_insert(filename, 0);
	BSTNodeHandle *result=MAP_FIND(lexlib, &unique_fn);
	if(!result)
	{
		result=MAP_INSERT(lexlib, &unique_fn, 0, 0);
		lf=(LexedFile*)result[0]->data;
		lf->filename_len=(int)strlen(lf->filename);
		success=lex(lf);
		if(success)
		{
			currentfile=lf;
			bytes_processed=lf->text->count;
		}
	}
	else
	{
		lf=(LexedFile*)result[0]->data;
		bytes_processed=0;
	}

	if(!lf||!lf->tokens)
		return 0;

	dlist_init(&bookmarks, sizeof(Bookmark), 16, 0);//no destructor
	bm=(Bookmark*)dlist_push_back(&bookmarks, 0);
	bm->lf=lf;
	bm->ks=0;
	bm->iflevel=0;

	dlist_init(&tokens, sizeof(Token), 128, 0);//no destructor
	PROF(START);

	while(bookmarks.nobj>0)
	{
	preprocess_start:
		bm=(Bookmark*)dlist_back(&bookmarks);
		currentfile=lf=bm->lf;

		//if(currentfile->filename==DEBUG_filename)//MARKER
		//	token=0;

		int ntokens=lf->tokens->count;
		while(bm->ks<ntokens)
		{
			token=TOKENS_AT(lf->tokens, bm->ks);

			//if(currentfile->filename==DEBUG_filename)//MARKER
			//{
			//	if(token->type==T_HASH)
			//	{
			//		Token *t2=TOKENS_AT(lf->tokens, bm->ks+1);
			//		if(t2->type==T_ERROR)
			//			token=token;
			//	}
			//}

			switch(token->type)
			{
			case T_HASH://TODO: directives should only work at the beginning of line
				++bm->ks;//skip HASH
				if(bm->ks>=ntokens)
				{
					pp_error(token, "Unexpected end of file. Expected a preprocessor directive.");
					continue;
				}
				token=TOKENS_AT(lf->tokens, bm->ks);

				//if(currentfile->filename==DEBUG_filename&&token->line>=2145)//MARKER
				//	token=token;

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
						token=TOKENS_AT(lf->tokens, bm->ks);
						k=skip_till_newline(lf->tokens, bm->ks);
						if(token->type!=T_ID)
							pp_error(token, "Expected an identifier.");
						else
						{
							redefinition=0;
							BSTNodeHandle *node=MAP_INSERT(macros, &token->str, 0, &redefinition);//insert can return null only if comparator is ill-defined
							macro=(Macro*)node[0]->data;
							if(redefinition)
								pp_error(token, "Macro \'%s\' redefined.", token->str);
							macro_define(macro, currentfile, token, k-bm->ks);
						}
						bm->ks=k+(k<ntokens);//skip newline
					}
					PROF(DEFINE);
					map_rebalance(macros);
					PROF(REBALANCE);
					break;
				case T_UNDEF:
					++bm->ks;//skip UNDEF
					if(bm->ks>=ntokens)
					{
						pp_error(token, "Unexpected end of file.");
						break;
					}
					token=TOKENS_AT(lf->tokens, bm->ks);
					if(token->type!=T_ID)
						pp_error(token, "Expected an identifier.");
					else
					{
						MAP_ERASE(macros, &token->str);
						++bm->ks;//skip ID
					}
					PROF(UNDEF);
					break;
				case T_IFDEF:
				case T_IFNDEF:
					{
						int ndef=token->type==T_IFNDEF;
						++bm->ks;
						if(bm->ks>=ntokens)
						{
							pp_error(token, "Unexpected end of file.");
							break;
						}
						token=TOKENS_AT(lf->tokens, bm->ks);
						if(token->type!=T_ID)
							pp_error(token, "Expected an identifier.");
						else
						{
							BSTNodeHandle *result=MAP_FIND(macros, &token->str);
							bm->ks+=bm->ks<ntokens;
							if((!result)!=ndef)
								bm->iflevel+=skip_block(macros, lf->tokens, &bm->ks, 0);
							else
							{
								++bm->iflevel;
								int start=bm->ks;
								bm->ks=skip_till_newline(lf->tokens, bm->ks);
								if(start<bm->ks)
									pp_error(token, "Expected a single token after #ifdef/ifndef.");
								bm->ks+=bm->ks<ntokens;
							}
						}
					}
					PROF(IFDEF);
					break;
				case T_IF:
					{
						++bm->ks;//skip IF
						int start=bm->ks;
						bm->ks=skip_till_newline(lf->tokens, bm->ks);
						long long result=eval_expr(lf->tokens, start, bm->ks, macros);
						bm->ks+=bm->ks<ntokens;//skip newline
						if(result)
							++bm->iflevel;
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
					PROF(ELSE);
					break;
				case T_ENDIF:
					{
						++bm->ks;//skip ENDIF
						if(!bm->iflevel)
							pp_error(token, "Unexpected #endif.");
						else
							--bm->iflevel;
						int start=bm->ks;
						bm->ks=skip_till_newline(lf->tokens, bm->ks);
						if(start<bm->ks)
							pp_error(TOKENS_AT(lf->tokens, start), "Unexpected tokens after #endif.");
						bm->ks+=bm->ks<ntokens;
					}
					PROF(ENDIF);
					break;

				case T_INCLUDE:
					{
						char *inc, custom;
						int k;
						++bm->ks;//skip INCLUDE
						if(bm->ks>=ntokens)
						{
							pp_error(token, "Unexpected end of file.");
							break;
						}
						token=TOKENS_AT(lf->tokens, bm->ks);

						inc=0;
						if(token->type==T_ID)
						{
							BSTNodeHandle *result=MAP_FIND(macros, &token->str);
							if(result)
							{
								ArrayHandle dst;

								ARRAY_ALLOC(Token, dst, 0, 0, 0, 0);
								macro=(Macro*)result[0]->data;
								macro_expand(macros, macro, lf->tokens, &bm->ks, &dst);
								if(!dst||dst->count!=1)
									pp_error(token, "Expected include file name.");
								else
								{
									Token *t2=(Token*)dst->data;
									if(t2->type!=T_INCLUDENAME_STD&&t2->type!=T_VAL_STR)
										pp_error(t2, "Expected include file name.");
									else
									{
										inc=t2->str;
										custom=t2->type==T_VAL_STR;
									}
								}
								array_free(&dst);
							}
						}
						else
						{
							if(token->type!=T_INCLUDENAME_STD&&token->type!=T_VAL_STR)
								pp_error(token, "Expected include file name.");
							else
							{
								inc=token->str;
								custom=token->type==T_VAL_STR;
							}
							bm->ks+=bm->ks<ntokens;//skip include name
						}

						k=skip_till_newline(lf->tokens, bm->ks);//skip include preprocessor directive
						if(bm->ks<k)
							pp_error(token, "Text after #include directive.");
						bm->ks+=k<ntokens;//skip newline

						if(inc)
						{
							char *filename2=find_include(inc, custom, includepaths);
						
							//if(!strcmp(inc, "vadefs.h"))//MARKER
							//	DEBUG_filename=filename2;

							if(!filename2)
								pp_error(token, "Cannot open include file \'%s\'.", inc);
							else//found the include file
							{
								int found=0;
								BSTNodeHandle *result=MAP_INSERT(lexlib, &filename2, 0, &found);
								LexedFile *lf2=(LexedFile*)result[0]->data;
								if(!found)//not lexed before
								{
									lf2->filename_len=(int)strlen(lf2->filename);
									success=lex(lf2);
									if(success)
									{
										//currentfile=lf2;
										bytes_processed+=lf2->text->count;
									}
								}
								//if(!found&&success||!(lf2->flags&LEX_INCLUDE_ONCE))//not found or not marked with '#pragma once',	the 'not found' part can be removed, since it's the preprocessor that reads #pragma once
								if(!(lf2->flags&LEX_INCLUDE_ONCE))//not marked with '#pragma once' by a previous preprocessor pass
								{
									bm=dlist_push_back(&bookmarks, 0);
									bm->lf=lf2;
									goto preprocess_start;
								}
							}
						}
						PROF(INCLUDE);
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
					PROF(PRAGMA);
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
						/*	{
								ArrayHandle text=0;//MARKER
								ret=0;
								dlist_appendtoarray(&tokens, &ret);
								tokens2text(ret, &text);
								array_free(&ret);
								printf("%s", text->data);
								array_free(&text);
							}//*/

							pp_error(token, "%s", token->str);//error message may contain '%'
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							bm->ks+=bm->ks<ntokens;
						}
						else
						{
							int start=bm->ks;
							bm->ks=skip_till_newline(lf->tokens, bm->ks);
							pp_error(TOKENS_AT(lf->tokens, start-1), "%.*s", bm->ks-start, (char*)currentfile->text->data+start);
							bm->ks+=bm->ks<ntokens;//skip newline
						}
					}
					PROF(ERROR);
					break;
				default:
					pp_error(token, "Expected a preprocessor directive.");
					bm->ks=skip_till_newline(lf->tokens, bm->ks);
					bm->ks+=bm->ks<ntokens;
					PROF(ERROR);
					break;
				}
				break;//case T_HASH		#...
			case T_MACRO_FILE:
				++bm->ks;//skip __FILE__
				macro_expand_file(token, (Token*)dlist_push_back(&tokens, 0));
				PROF(MACRO_FILE);
				break;
			case T_MACRO_LINE:
				++bm->ks;//skip __LINE__
				macro_expand_line(token, (Token*)dlist_push_back(&tokens, 0));
				PROF(MACRO_LINE);
				break;
			case T_MACRO_DATE:
				++bm->ks;//skip __DATE__
				macro_expand_date(token, (Token*)dlist_push_back(&tokens, 0));
				PROF(MACRO_DATE);
				break;
			case T_MACRO_TIME:
				++bm->ks;//skip __TIME__
				macro_expand_time(token, (Token*)dlist_push_back(&tokens, 0));
				PROF(MACRO_TIME);
				break;
			case T_MACRO_TIMESTAMP:
				++bm->ks;//skip __TIMESTAMP__
				macro_expand_timestamp(token, (Token*)dlist_push_back(&tokens, 0));
				PROF(MACRO_TIMESTAMP);
				break;
			case T_ID:
				{
					BSTNodeHandle *result=MAP_FIND(macros, &token->str);
					PROF(IDENTIFIER_LOOKUP);
					if(result)
					{
						ArrayHandle dst;

						ARRAY_ALLOC(Token, dst, 0, 0, 0, 0);
						macro=(Macro*)result[0]->data;

						//if(!strcmp(macro->name, "_In_opt_z_"))//MARKER
						//	macro=macro;

						macro_expand(macros, macro, lf->tokens, &bm->ks, &dst);
						if(dst&&dst->count)
						{
							token=TOKENS_AT(dst, dst->count-1);
							token->nl_after=0;
							token->ws_after=0;

							for(int kt=0;kt<(int)dst->count;++kt)//TODO dlist_push_back(array)
							{
								token=TOKENS_AT(dst, kt);
								dlist_push_back(&tokens, token);
							}
						}
						array_free(&dst);
						PROF(MACRO_EXPANSION);
					}
					else
					{
						dlist_push_back(&tokens, token);
						++bm->ks;
						PROF(IDENTIFIER);
					}
				}
				break;
			case T_NEWLINE:
			case T_NEWLINE_ESC:
				++bm->ks;
				PROF(TOKEN);
				break;
			default:
				if(token->type>T_IGNORED)
					dlist_push_back(&tokens, token);
				++bm->ks;
				PROF(TOKEN);
				break;
			}
		}//end file loop
		if(bm->iflevel>0)
		{
			token=(Token*)array_back(&lf->tokens);
			pp_error(token, "End of file reached. Expected %d #endif\'s.", bm->iflevel);
		}
		dlist_pop_back(&bookmarks);//no destructor
	}//end preprocess loop

	ret=0;
	dlist_appendtoarray(&tokens, &ret);
	dlist_clear(&tokens);
	PROF(FINISH);
	BM_FINISH("Preprocess \'%s\':\t%lfms\n", filename, bm_t1);
	printf("Processed %lld bytes (%lf MB/s)\n", bytes_processed, (double)bytes_processed/(1024*1.024*bm_t1));
	prof_end();
	return ret;
}

void		init_dateNtime()
{
	time_t t_now=time(0);
#ifdef _MSC_VER
	struct tm t_formatted={0};
	int error=localtime_s(&t_formatted, &t_now);
	struct tm *ts=&t_formatted;
#else
	struct tm *ts=localtime(&t_now);
#endif
	int printed;

	const char *weekdays[]={"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
	printed=sprintf_s(g_buf, G_BUF_SIZE, "%d-%02d-%02d%s", 1900+ts->tm_year, 1+ts->tm_mon, ts->tm_mday, weekdays[ts->tm_wday]);//FIXME: these are not standard
	//size_t printed=strftime(g_buf, G_BUF_SIZE, "%Y-%m-%e%a", ts);
	currentdate=strlib_insert(g_buf, printed);

	printed=sprintf_s(g_buf, G_BUF_SIZE, "%02d:%02d:%02d%s", ts->tm_hour%12, ts->tm_min, ts->tm_sec, ts->tm_hour<12?"AM":"PM");
	//size_t printed=strftime(g_buf, G_BUF_SIZE, "%H:%M:%S", ts);
	currenttime=strlib_insert(g_buf, printed);

	printed=sprintf_s(g_buf, G_BUF_SIZE, "%d-%02d-%02d%s %02d%02d%02d%s", 1900+ts->tm_year, 1+ts->tm_mon, ts->tm_mday, weekdays[ts->tm_wday], ts->tm_hour%12, ts->tm_min, ts->tm_sec, ts->tm_hour<12?"AM":"PM");
	currenttimestamp=strlib_insert(g_buf, printed);
}
void		macros_init(MapHandle macros, PreDef *preDefs, int nPreDefs)
{
	BSTNodeHandle *node;
	Macro *macro;
	int tokenCount;
	Token *token;

	MAP_INIT(macros, char*, Macro, macro_cmp, macro_destructor);
	for(int k=0;k<nPreDefs;++k)
	{
		int found=0;
		char *unique_name=strlib_insert(preDefs[k].name, strlen(preDefs[k].name));
		node=MAP_INSERT(macros, &unique_name, 0, &found);
		macro=(Macro*)node[0]->data;
		macro->srcfile=0;//pre-defined macro
		macro->nargs=MACRO_NO_ARGLIST;
		macro->is_va=0;
		tokenCount=preDefs[k].type!=T_IGNORED;
		ARRAY_ALLOC(Token, macro->tokens, 0, tokenCount, 0, 0);
		if(tokenCount)
		{
			token=(Token*)array_at(&macro->tokens, 0);
			token->type=preDefs[k].type;
			switch(preDefs[k].type)
			{
			case T_VAL_STR:
				token->str=strlib_insert(preDefs[k].str, strlen(preDefs[k].str));
				break;
			case T_VAL_I32:
				token->i=preDefs[k].i;
				break;
			default:
				printf("Error: Pre-defined macro of unsupported type %d", preDefs[k].type);
				break;
			}
		}
	}
}
void		acc_cleanup(MapHandle lexlib, MapHandle strings)
{
	MAP_CLEAR(lexlib);
	MAP_CLEAR(strings);
}

void		tokens2text(ArrayHandle tokens, ArrayHandle *str)
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
		token_append2str(str, token);
#if 0
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
			case T_VAL_C8:
			case T_VAL_UC8://TODO multi-character literal
				{
					ArrayHandle proc;
					char s[9]={0};
					memcpy(s, &token->i, 8);
					len=strlen(s);
					len+=!len;
					if(len>1)
						memreverse(s, len, 1);
					proc=str2esc(s, len);
					printed=sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", (char*)proc->data);
					free(proc);
				}
				//printed=sprintf_s(g_buf, G_BUF_SIZE, "\'%s\'", describe_char((char)token->i));
				break;
			case T_VAL_CW:
				//TODO
				break;
			case T_VAL_I16:case T_VAL_U16:
			case T_VAL_I32:
			case T_VAL_I64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%lld", token->i);
				break;
			case T_VAL_U32:
			case T_VAL_U64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%llu", token->u);
				break;
			case T_VAL_F32:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%f", token->f32);
				break;
			case T_VAL_F64:
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%g", token->f64);
				break;
			case T_VAL_STR:
				{
					ArrayHandle proc=str2esc(token->str, strlen(token->str));
					printed=sprintf_s(g_buf, G_BUF_SIZE, "\"%s\"", (char*)proc->data);
					array_free(&proc);
				}
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
#endif
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
