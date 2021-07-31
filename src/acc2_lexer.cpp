#include		"acc2.h"
#include		"include/algorithm"
#include		"include/tmmintrin.h"
#include		"include/conio.h"

void			error_lex(int line0, int col0, const char *format, ...)
{
	printf("%s(%d:%d) Error: ", currentfilename, line0+1, col0+1);
	if(format)
	{
		vprintf(format, (char*)(&format+1));
		printf("\n");
	}
	else
		printf("Unknown error.\n");
	if(compile_status<CS_ERRORS)
		compile_status=CS_ERRORS;
}
const char		lexflags[]=//redundant
{
#define		TOKEN(STRING, LABEL, FLAGS)	FLAGS,
#include"acc2_keywords_c.h"
#undef		TOKEN
};
struct			LexOption
{
	__m128i option, mask;
	int token;
	short len;
	union
	{
		short flags;//endswithnd<<1|endswithnan
		struct{short endswithnan:1, endswithnd:1;};//followed by non-alphanumeric, followed by a non-digit
	};
	const char *name;
};
typedef std::vector<LexOption, 16> LexSlot;
LexSlot			slots[128];
inline int		strcmp_idx(const char *s1, const char *s2)//returns -1 if identical, otherwise the index where character start to differ
{
	int k=0;
	for(;*s1&&*s2&&*s1==*s2;++k, ++s1, ++s2);
	if(!*s1&&!*s2)
		return -1;
	return k;
}
bool			operator<(LexOption const &a, LexOption const &b)
{
	int k=strcmp_idx(a.name, b.name);
	if(k==-1)
		return 0;
	char ac=a.name[k], bc=b.name[k];
	if(!ac)
		ac=127;
	if(!bc)
		bc=127;
	return ac<bc;
}
void			init_lexer()
{
	__m128i ones=_mm_set1_epi32(-1);
	for(int kt=0;kt<CT_NTOKENS;++kt)
	{
		auto keyword=keywords[kt];
		if(keyword)
		{
			short len=(short)strlen(keyword);
			char endswithnd=len==1&&keyword[0]=='.';//only '.' should end with a non-digit
			LexOption opt=
			{
				_mm_setzero_si128(),
				_mm_setzero_si128(),
				kt,
				len,
				endswithnd<<1|(is_alphanumeric(keyword[0])&&kt!=CT_DQUOTE_WIDE&&kt!=CT_DQUOTE_RAW),//only alphanumeric keywords should be followed by non-alphanumeric
				keyword,
			};
			memcpy(&opt.option, keyword, opt.len);
			opt.mask=_mm_cmpeq_epi8(opt.option, _mm_setzero_si128());
			opt.mask=_mm_xor_si128(opt.mask, ones);
			slots[keyword[0]].push_back(opt);
		}//if keyword
	}
	for(int k=0;k<128;++k)
	{
		auto &slot=slots[k];
		if(slot.size()>1)
			std::sort(slot.begin(), slot.end());
	}
}
inline int		lex_match(const char *p, int k, LexOption *&popt, int &advance)
{
	auto &slot=slots[p[k]];
	int nopts=slot.size();
	if(nopts)
	{
		__m128i val=_mm_loadu_si128((__m128i*)(p+k));//over-allocated text
		for(int ko=0;ko<nopts;++ko)
		{
			auto &opt=slot[ko];
			__m128i v2=_mm_and_si128(val, opt.mask);
			v2=_mm_cmpeq_epi8(v2, opt.option);
			int result=_mm_movemask_epi8(v2);
			if(result==0xFFFF)
			{
				char next=p[k+opt.len];
				if(!opt.endswithnan||!(UNSIGNED_IN_RANGE('0', next, '9')|UNSIGNED_IN_RANGE('A', next, 'Z')|UNSIGNED_IN_RANGE('a', next, 'z')|(next=='_')))
				{
					popt=&opt;
					advance=opt.len;
					return true;
				}
			}
			//if(result==0xFFFF&&!(opt.endswithnan&&is_alphanumeric(p[k+opt.len])&-lexflags[opt.token]))
			//if(result==0xFFFF&&(!opt.endswithnan||!is_alphanumeric(p[k+opt.len])))
			//{
			//	popt=&opt;
			//	advance=opt.len;
			//	return true;
			//}
		}
	}
	return false;
}
inline void		lex_push_tok(const char *p, int k, int linestart, int lineno, Expression &ex, LexOption *popt)
{
	char c0=0, c1=p[k+popt->len];
	if(k>0)
		c0=p[k-1];
	Token token=
	{
		(CTokenType)popt->token,
		k, lineno, k-linestart, popt->len,
		token_flags(c0, c1),
		nullptr,
	};
	ex.push_back(token);
}
inline void		lex_push_string(const char *p, int k, int linestart, int lineno, Expression &ex, int len, CTokenType tokentype, char c0, char c1, bool raw)
{
	Token token=
	{
		tokentype,
		k, lineno, k-linestart, len,
		token_flags(c0, c1),
		nullptr,
	};

	std::string processed;
	if(raw)
		processed.append(p+k, len);
	else
	{
		esc2str(p+k, len, processed);
		token.len=processed.size();//processed size
	}

	token.sdata=add_string(processed);

	ex.push_back(token);
}
enum LexerState
{
	IC_NORMAL,
	IC_HASH,
	IC_INCLUDE,
};
void			lex(LexFile &lf)//utf8 text is modified to remove esc newlines
{
	long long cycles1=__rdtsc();
	if(!lf.text.size())//open file
	{
		if(!lf.filename)
		{
			printf("INTERNAL ERROR: Lexer: Filename is NULL.\n");
			compile_status=CS_FATAL_ERRORS;
			return;
		}
		auto ret=open_txt(lf.filename, lf.text);
		if(!ret)
		{
			printf("INTERNAL ERROR: Lexer: Cannot open \"%s\"\n", lf.filename);
			compile_status=CS_FATAL_ERRORS;
			return;
		}
		//prof.add("open source file");
	}
	if(lf.filename)//guard against synthetic lex calls
	{
		currentfilename=lf.filename;
		currentfile=&lf.text;
	}

	int size=lf.text.size();
	lf.text.insert_pod(size, '\0', 16);//overallocate text by 16 null pointers
	auto p=lf.text.data();
	
	unsigned short data=load16(p);
	for(int k=0;k<size;++k)//remove esc newlines		TODO: normalize newlines
	{
		if(data=='\n\\')
		//if(p[k]=='\\'&&p[k+1]=='\n')
		{
			int k2=k+2;
			for(;k2<size&&p[k2]!=' '&&p[k2]!='\t';++k2)
				p[k2-2]=p[k2];
			p[k2-2]=flag_ignore, p[k2-1]=flag_esc_nl;
		}
		data<<=8, data|=(byte)p[k+2];
	}

	LexOption *popt;
	LexerState state_include_chevrons=IC_NORMAL;
	for(int k=0, lineno=0, linestart=0, lineinc=1, advance=0;k<size;k+=advance)//lexer loop
	{
		if(lex_match(p, k, popt, advance))
		{
			switch(popt->token)
			{
			case CT_LINECOMMENTMARK://skip comments
				{
					int k2=k+2;
					for(;k2<size&&p[k2]&&p[k2]!='\n';lineinc+=p[k2]==flag_esc_nl, ++k2);
					advance=k2-k;
				}
				break;
			case CT_BLOCKCOMMENTMARK:
				{
					int k2=k+2;
					for(;k2<size&&p[k2]&&!(p[k2]=='*'&&p[k2+1]=='/');lineinc+=p[k2]==flag_esc_nl, ++k2);
					advance=k2+2-k;
				}
				break;
			case CT_PERIOD:
				{
					char c=p[k+1];
					if(c>='0'&&c<='9')
						goto lex_read_number;
					state_include_chevrons=IC_NORMAL;
					lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
				}
				break;
			case CT_QUOTE:case CT_DOUBLEQUOTE:case CT_DQUOTE_WIDE://quoted text
				{
					state_include_chevrons=IC_NORMAL;
					int start=k+1+(popt->token==CT_DQUOTE_WIDE);
					int k2=start;
					bool closingwithquote=popt->token==CT_QUOTE;
					bool matched=true;
					for(;k2<size;lineinc+=p[k2]==flag_esc_nl, ++k2)
					{
						if(closingwithquote)
						{
							if(p[k2]=='\'')
								break;
						}
						else
						{
							if(p[k2]=='\"')
								break;
						}
						if(p[k2]=='\n')//esc nl are already removed
						{
							error_lex(lineno, k-linestart, "Unmatched quotes: [%d]: %s, [%d] %s", k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
							//printf("Lexer: line %d: unmatched quotes: [%d]: %s, [%d] %s\n", lineno+1, k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
							matched=false;
							break;
						}
						k2+=p[k2]=='\\'&&(p[k2+1]=='\''||p[k2+1]=='\"'||p[k2+1]=='\\');
					}
					advance=k2+1-k;
					if(matched)
					{
						CTokenType tokentype=CT_IGNORED;
						switch(popt->token)
						{
						case CT_QUOTE:			tokentype=CT_VAL_CHAR_LITERAL;break;
						case CT_DOUBLEQUOTE:	tokentype=CT_VAL_STRING_LITERAL;break;
						case CT_DQUOTE_WIDE:	tokentype=CT_VAL_WSTRING_LITERAL;break;
						}
						lex_push_string(p, start, linestart, lineno, lf.expr, k2-start, tokentype, k>0?p[k-1]:0, p[k2+1], false);
					}
				}
				break;
			case CT_DQUOTE_RAW://raw string literal R"delim(...)delim"
				{
					state_include_chevrons=IC_NORMAL;
					int dstart=k+2, dend=dstart;
					bool illformed=false;
					for(;dend<size&&p[dend]!='(';++dend)//find delimiter & opening parenthesis
					{
						char c=p[dend];
						if(c=='\n'||c==' '||c=='\t'||c=='\\')
						{
							error_lex(lineno, dend-linestart, "Ill-formed raw string literal. The delimiter should not contain whitespace or backslash.");
							//printf("Error line %d: Ill-formed raw string literal.\n", lineno+1);
							illformed=true;
							break;
						}
					}
					if(dend-dstart>16)
					{
						error_lex(lineno, dstart-linestart, "Raw string literal delimiter is longer than 16 characters.");
						//printf("Error line %d: Raw string literal delimiter is longer than 16 characters.\n", lineno+1);
						illformed=true;
					}
					int start, end;
					if(illformed)
					{
						start=dstart;
						for(end=start+1;end<size&&p[dend]!='\n'&&p[dend]!='\"';++end);
						lex_push_string(p, start, linestart, lineno, lf.expr, end-start, CT_VAL_STRING_LITERAL, k>0?p[k-1]:0, p[end+1], false);
					}
					else
					{
						start=dend+1;

						__m128i mask=_mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
						int dlen=dend-dstart;
						__m128i m_len=_mm_set1_epi8(dlen);
						mask=_mm_cmplt_epi8(mask, m_len);

						__m128i delimiter=_mm_loadu_si128((__m128i*)(p+dstart));
						delimiter=_mm_and_si128(delimiter, mask);

						illformed=true;
						for(end=start;end<size;++end)
						{
							if(p[end]==')')
							{
								__m128i text=_mm_loadu_si128((__m128i*)(p+end+1));
								text=_mm_and_si128(text, mask);
								text=_mm_cmpeq_epi8(text, delimiter);
								int result=_mm_movemask_epi8(text);
								if(result==0xFFFF&&p[end+1+dlen]=='\"')
								{
									illformed=false;
									break;
								}
							}
						}
						if(!illformed)
							advance=end+1+dlen+1-k;//skip ) delimiter "
						else
							advance=end+1-k;
						lex_push_string(p, start, linestart, lineno, lf.expr, end-start, CT_VAL_STRING_LITERAL, k>0?p[k-1]:0, p[k+advance], true);
					}
				}
				break;
			case CT_HASH:
				lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
				state_include_chevrons=IC_HASH;
				break;
			case CT_ERROR:
				if(state_include_chevrons==IC_HASH)
				{
					state_include_chevrons=IC_NORMAL;
					lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
					int start=k+strlen(keywords[CT_ERROR]);
					for(;start<size&&(p[start]==' '||p[start]=='\t'||p[start]==flag_ignore||p[start]==flag_esc_nl);lineinc+=p[start]==flag_esc_nl, ++start);
					int end=start+1;
					for(;end<size&&p[end]&&p[end]!='\n';lineinc+=p[end]==flag_esc_nl, ++end);//BUG: esc nl flags remain in error msg
					advance=end-k;
					lex_push_string(p, start, linestart, lineno, lf.expr, end-start, CT_VAL_STRING_LITERAL, k>0?p[start-1]:0, p[end], false);
				}
				else
					lex_push_string(p, k, linestart, lineno, lf.expr, 5, CT_ID, k>0?p[k-1]:0, p[k+7], false);
				break;
			case CT_INCLUDE:
				if(state_include_chevrons==IC_HASH)
				{
					state_include_chevrons=IC_INCLUDE;
					lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
				}
				else
					lex_push_string(p, k, linestart, lineno, lf.expr, 7, CT_ID, k>0?p[k-1]:0, p[k+7], false);
				break;
			case CT_LESS:
				if(state_include_chevrons==IC_INCLUDE)
				{
					int k2=k+1;
					for(;k2<size&&p[k2]&&p[k2]!='>'&&p[k2]!='\n';lineinc+=p[k2]==flag_esc_nl, ++k2);
					advance=k2+1-k;
					if(p[k]=='<'&&p[k2]=='>')
						lex_push_string(p, k+1, linestart, lineno, lf.expr, k2-(k+1), CT_INCLUDENAME_STD, k>0?p[k-1]:0, p[k2+1], false);
					else
					{
						//error: unmatched chevron
						error_lex(lineno, k-linestart, "Unmatched include chevron: [%d]: %s, [%d] %s\n", k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
						//printf("Lexer: line %d: unmatched include chevron: [%d]: %s, [%d] %s\n", lineno+1, k+1-linestart, describe_char(p[k]), k2-linestart, describe_char(p[k2]));
					}
				}
				else
					lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
				state_include_chevrons=IC_NORMAL;
				break;
			default:
				state_include_chevrons=IC_NORMAL;
				lex_push_tok(p, k, linestart, lineno, lf.expr, popt);
				break;
			}
		}
		else
		{
			switch(p[k])
			{
			case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':case '.'://numbers
				{
				lex_read_number:
					state_include_chevrons=IC_NORMAL;
					long long ival;
					double fval;
					char ret=acme_read_number(p, size, k, &advance, &ival, &fval);
					if(!advance)//
					{
						error_lex(lineno, k-linestart, "INTERNAL ERROR while parsing a number at k=%d (advance = 0):\n\t%.*s\n\n", k, 16, p+k);
						//printf("Internal error while parsing a number at k=%d (advance = 0):\n\t%.*s\n", k, 16, p+k);
						_getch();
					}
					if(ret<2)
					{
						char c0=0, c1=p[k+advance];
						if(k>0)
							c0=p[k-1];
						Token token=
						{
							ret?CT_VAL_FLOAT:CT_VAL_INTEGER,
							k, lineno, k-linestart, advance,
							token_flags(c0, c1),
							nullptr,
						};
						if(ret)
							token.fdata=fval;
						else
							token.idata=ival;
						lf.expr.push_back(token);
					}
				}
				break;
			case ' ':case '\t':case 1:case 2://skip whitespace & escaped newlines
				{
					int k2=k;
					for(;k2<size&&p[k2]&&(p[k2]==' '||p[k2]=='\t'||p[k2]==flag_ignore||p[k2]==flag_esc_nl);lineinc+=p[k2]==flag_esc_nl, ++k2);
					advance=k2-k;
				}
				break;
			default://identifiers
				state_include_chevrons=IC_NORMAL;
				if(p[k]>='A'&&p[k]<='Z'||p[k]>='a'&&p[k]<='z'||p[k]=='_')
				{
					int k2=k+1;
					for(;k2<size&&p[k2]&&is_alphanumeric(p[k2]);++k2);
					advance=k2-k;
					lex_push_string(p, k, linestart, lineno, lf.expr, advance, CT_ID, k>0?p[k-1]:0, p[k2], false);
				}
				else//error: unrecognized text
				{
					error_lex(lineno, k-linestart, "Unrecognized text at %d: \'%s\'", describe_char(p[k]));
					//printf("Lexer: error line %d: [%d] %s is unrecognized\n", lineno+1, k+1-linestart, describe_char(p[k]));
					advance=1;
				}
				break;
			}
		}
		if(p[k]==flag_esc_nl)
			++lineinc;
		else if(p[k]=='\n')
		{
			lineno+=lineinc;
			lineinc=1;
			linestart=k+1;
		}
	}
	long long cycles2=__rdtsc();
	if(prof_print)
	{
		if(lf.filename)
			printf("\nLexed \'%s\' in %lld cycles\n", lf.filename, cycles2-cycles1);
		else if(size<=16)
			printf("\nLexed \'%s\' in %lld cycles\n", lf.text.c_str(), cycles2-cycles1);
		else
			printf("\nLexed \'%.*s...\' in %lld cycles\n", 16, lf.text.c_str(), cycles2-cycles1);
	}
}