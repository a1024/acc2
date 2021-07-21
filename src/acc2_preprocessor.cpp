#include		"acc2.h"
#include		"include/math.h"
#include		"include/algorithm"
#include		"include/stack"

const char		*p_va_args=nullptr;//mainteined by preprocessor

char			*currentfilename=nullptr;//maintained by the include system
std::string		*currentfile=nullptr;

void			compile_error(Token const &token, const char *format, ...)
{
	printf("%s(%d:%d) Error: ", currentfilename, token.line+1, token.col+1);
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
void			compile_warning(Token const &token, const char *format, ...)
{
	printf("%s(%d:%d) Warning: ", currentfilename, token.line+1, token.col+1);
	if(format)
	{
		vprintf(format, (char*)(&format+1));
		printf("\n");
	}
	else
		printf("Unknown error.\n");
	if(compile_status<CS_WARNINGS_ONLY)
		compile_status=CS_WARNINGS_ONLY;
}

char*			add_string(std::string &str)
{
	bool old=false;
	auto ret=strings.insert_no_overwrite(str.data(), &old);
	if(!old)
		str.move_and_destroy();
	return ret;
}

//macro expansion
static void		token_stringize(std::vector<Token> const &tokens, int first, int count, Token &out)
{
	out={CT_VAL_STRING_LITERAL};
	out.synthesized=true;
	std::string text;
	if(count<=0)//empty range
	{
		if(first<(int)tokens.size())
		{
			auto pt=tokens.data()+first;
			out.pos=pt->pos;
			out.line=pt->line;
			out.col=pt->col;
		}
	}
	else//non-empty range
	{
		auto pt=tokens.data()+first;
		auto last=pt+count-1;
		int start=pt->pos, end=last->pos+last->len;
		out.pos=start;
		out.line=pt->line;
		out.col=pt->col;

		int len=end-start;
		if(len>0)
		{
			text.resize(len);
			int kd=0;
			for(int ks=start;ks<end;)
			{
				char c=currentfile->operator[](ks);
				if(c==flag_ignore||c==flag_esc_nl)
				{
					++ks;
					continue;
				}
				auto word=load16(currentfile->data()+ks);
				if(word=='//')
				{
					ks+=2;
					for(;ks<end&&currentfile->operator[](ks)!='\n';++ks);
					continue;
				}
				if(word=='*/')
				{
					ks+=2;
					for(;ks<end;++ks)
					{
						word=load16(currentfile->data()+ks);
						if(word=='/*')
						{
							ks+=2;
							break;
						}
					}
					continue;
				}
				text[kd]=currentfile->operator[](ks);
				++kd, ++ks;
			}
			text.resize(kd);
		}
		
		char c0=0, c1=currentfile->operator[](end);
		if(start>0)
			c0=currentfile->operator[](start-1);
		out.flags=token_flags(c0, c1);
	}
	out.len=text.size();
	out.sdata=add_string(text);//add empty string
}
static void		token_paste(Token const *t1, Token const *t2, std::vector<Token> &dst, int &kd)//concatenate
{
	if(t1)
	{
		if(t2)
		{
			if(t1->type==CT_VAL_STRING_LITERAL&&t2->type==CT_VAL_STRING_LITERAL)
			{
				dst[kd]=*t1;
				++kd;
				dst[kd]=*t2;
			}
			else
			{
				std::string text;

				if(t1->type==CT_LEXME||t1->synthesized)
					text.append(t1->sdata);
				else
				{
					bool isstr=t1->type==CT_VAL_STRING_LITERAL, ischar=t1->type==CT_VAL_CHAR_LITERAL;
					if(isstr)
						text+='\"';
					if(ischar)
						text+='\'';
					text.append(currentfile->data()+t1->pos, t1->len);
					if(ischar)
						text+='\'';
					if(isstr)
						text+='\"';
				}

				if(t2->type==CT_LEXME||t2->synthesized)
					text.append(t2->sdata);
				else
				{
					bool isstr=t2->type==CT_VAL_STRING_LITERAL, ischar=t2->type==CT_VAL_CHAR_LITERAL;
					if(isstr)
						text+='\"';
					if(ischar)
						text+='\'';
					text.append(currentfile->data()+t2->pos, t2->len);
					if(ischar)
						text+='\'';
					if(isstr)
						text+='\"';
				}

				auto &out=dst[kd];
				out.type=CT_LEXME;
				out.prevwhitespace=t1->prevwhitespace;
				out.prevnewline=t1->prevnewline;
				out.nextwhitespace=t2->nextwhitespace;
				out.nextnewline=t2->nextnewline;
				out.synthesized=true;
				out.sdata=add_string(text);
			}
		}
		else
			dst[kd]=*t1;
	}
	else
	{
		if(t2)
			dst[kd]=*t2;
		else
			--kd;//kd will be incremented back, equivalent to adding CT_IGNORED
	}
}

void			macro_define(MacroLibrary &macros, const char *extern_name, long long data, CTokenType tokentype)
{
	MacroLibrary::EType macro;
	macro.first=add_string(extern_name);
	macro.second.srcfilename=add_string("<pre-defined>");
	macro.second.nargs=MACRO_NO_ARGLIST;
	macro.second.is_va=false;
	if(tokentype!=CT_IGNORED)
	{
		Token token=
		{
			tokentype,
			0, 0, 0, 0,
			{0},
			{nullptr},
		};
		token.idata=data;
		if(tokentype==CT_VAL_STRING_LITERAL)
			token.sdata=add_string(token.sdata);
		macro.second.definition.push_back(token);
	}
	macros.insert(std::move(macro));
}

typedef std::vector<Token> MacroCallArgument;
static int		macro_define(MacroLibrary &macros, Token const *tokens, int remaining)//tokens after '#define'
{
	int len=0;
	for(;len<remaining&&tokens[len].type!=CT_NEWLINE;++len);//find length of macro definition
	auto ptok=tokens;
	if(ptok->type!=CT_ID)
		compile_error(*ptok, "Expected an identifier.");
	else
	{
		bool overwrite=false;
		auto p=macros.insert(MacroLibrary::EType(ptok->sdata, MacroDefinition()), &overwrite);
		if(overwrite)
			compile_warning(*ptok, "Macro redefinition.");
		p->second.define(currentfilename, tokens, len);
	}
	return len;
}
static bool		macro_find_call_extent(MacroLibrary::EType const &macro, Token const *tokens, int size, int start, int &len, std::vector<MacroCallArgument> &args, Token const *cont=nullptr, int cont_remaining=0)//don't call on error
{
	args.clear();
	if(macro.second.nargs==MACRO_NO_ARGLIST)//macro has an arglist
	{
		len=1;
		return true;
	}
	auto ptok=tokens+start+1;
	if(ptok->type!=CT_LPR)
	{
		compile_error(*ptok, "Macro should have an argument list with %d arguments.", macro.second.nargs);
		len=1;
		return false;
	}
	int end=start+2, k0=end;
	int nargs=0;
	for(int level=1;end<size&&level>0;++end)//find call length
	{
		ptok=tokens+end;
		level+=(ptok->type==CT_LPR)-(ptok->type==CT_RPR);
		if(level==1&&ptok->type==CT_COMMA||!level&&ptok->type==CT_RPR)
		{
			bool va_append=macro.second.is_va&&nargs+1>macro.second.nargs;//in case of variadic macros, append extra args with commas as the last arg
			if(va_append)
				args.back().append(tokens+k0, end+(ptok->type==CT_COMMA)-k0);//BROKEN
			else
				args.push_back(MacroCallArgument(tokens+k0, end-k0));
			k0=end+1;
			nargs+=!va_append;
		}
	}
	len=end-start;
	if(ptok->type!=CT_RPR)
	{
		compile_error(*ptok, "Unmatched parenthesis.");
		return false;
	}
	if(nargs!=macro.second.nargs)
	{
		if(macro.second.is_va)
			compile_error(*ptok, "Variadic macro has %d arguments instead of at least %d.", macro.second.nargs);
		else
			compile_error(*ptok, "Macro has %d arguments instead of %d.", macro.second.nargs);
		return false;
	}
	return true;
}
struct			MacroCall
{
	MacroLibrary::EType const *macro;
	int kt;//macro definition index
	std::vector<MacroCallArgument> args;
	int kt2;//macro call argument index, use on args when macrodefinition[kt] is an arg
	int expansion_start;//dst index
	bool done;

	MacroCall():macro(nullptr), kt(0), kt2(0), done(false){}
	MacroCall(MacroCall &&other)
	{
		set(other.macro, other.kt, std::move(other.args), other.kt2, other.expansion_start);
	}
	MacroCall(MacroLibrary::EType const *macro, int kt, std::vector<MacroCallArgument> &&args, int kt2, int expansion_start)
	{
		set(macro, kt, std::move(args), kt2, expansion_start);
	}
	void set(MacroLibrary::EType const *macro, int kt, std::vector<MacroCallArgument> &&args, int kt2, int expansion_start)
	{
		this->macro=macro;
		this->kt=kt;
		this->args=std::move(args);
		this->kt2=kt2;
		this->expansion_start=expansion_start;
		done=false;
	}
};
static bool		macro_stringize(std::vector<Token> const &macrodefinition, int kt, std::vector<MacroCallArgument> const &args2, Token &out)
{
	auto &token=macrodefinition[kt];
	if(kt+1>=(int)macrodefinition.size())
	{
		compile_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition
		return false;
	}
	auto &next=macrodefinition[kt+1];
	if(next.type!=CT_MACRO_ARG)
	{
		compile_error(token, "Stringize operator can only be applied to a macro argument.");//error in definition
		return false;
	}
	auto &arg=args2[(int)next.idata];
	token_stringize(arg, 0, arg.size(), out);
	return true;
}
static void		macro_paste(Token const *t_left, std::vector<Token> const &macrodefinition, int &kt, std::vector<MacroCallArgument> &args2, std::vector<Token> &dst, int &kd)
{
	auto &second=macrodefinition[kt+1];
	if(second.type==CT_MACRO_ARG)
	{
		auto &callarg=args2[(int)second.idata];
		if(callarg.size())
		{
			token_paste(t_left, callarg.data(), dst, kd);
			if(callarg.size()>1)
			{
				dst.resize(dst.size()+callarg.size()-1);
				memcpy(dst.data()+kd+1, callarg.data()+1, (callarg.size()-1)*sizeof(Token));
			}
			kd+=callarg.size();
		}
		else
		{
			token_paste(t_left, nullptr, dst, kd);
			++kd;
		}
	}
	else if(second.type==CT_HASH)
	{
		Token out;
		++kt;
		macro_stringize(macrodefinition, kt, args2, out);
		token_paste(t_left, &out, dst, kd);
		++kd;
	}
	else
	{
		token_paste(t_left, &second, dst, kd);
		++kd;
	}
}
static void		macro_expand(MacroLibrary const &macros, MacroLibrary::EType const &macro, Token const *src, int srcsize, int &ks, std::vector<Token> &dst, int &kd)//can resize dst, returns expanded token count
{
	int len=0;
	std::vector<MacroCallArgument> args;
	if(!macro_find_call_extent(macro, src, srcsize, ks, len, args))
	{
		ks+=len;
		compile_error(src[ks], "Invalid macro call.");//redundant error message
		return;
	}
	int start=kd, end=kd+len;
	std::vector<MacroCall> context;
	context.push_back(MacroCall(&macro, 0, std::move(args), 0, kd));
	
	while(context.size())
	{
	macro_expand_again:
		auto &top=context.back();
		if(top.done)
			goto macro_expand_endcall;
		auto macro2=top.macro;
		int &kt=top.kt;
		auto &args2=top.args;
		auto &macrodefinition=macro2->second.definition;
		int nmacrotokens=macrodefinition.size();
		for(;kt<nmacrotokens;)//macro expansion loop
		{
			auto &token=macrodefinition[kt];
			if(token.type==CT_HASH)//stringize call-argument
			{
				macro_stringize(macrodefinition, kt, args2, dst[kd]);//X check if next is concatenate
				++kd, kt+=2;
			}
			else if(kt+1<nmacrotokens&&macrodefinition[kt+1].type==CT_CONCATENATE)//anything can be concatenated
			{
				++kt;
				if(kt+1>=nmacrotokens)
				{
					auto &next=macrodefinition[kt];
					compile_error(next, "Token paste operator cannot be at the end of macro.");//error in definition
					break;
				}
				Token const *t_left=nullptr;
				if(token.type==CT_MACRO_ARG)
				{
					auto &callarg=args2[(int)token.idata];
					if(callarg.size()>1)
					{
						dst.resize(dst.size()+callarg.size()-1);
						memcpy(dst.data()+kd, callarg.data(), (callarg.size()-1)*sizeof(Token));
						kd+=callarg.size()-1;
					}
					if(callarg.size()>0)
						t_left=&callarg.back();
				}
				else
					t_left=&token;
				macro_paste(t_left, macrodefinition, kt, args2, dst, kd);
				kt+=2;
			}
			else if(token.type==CT_CONCATENATE)
			{
				if(kt+1>=nmacrotokens)
				{
					compile_error(token, "Token paste operator cannot be at the end of macro.");//error in definition
					break;
				}
				int kd2=kd-1;
				for(;kd2>=top.expansion_start&&dst[kd2].type<=CT_IGNORED||dst[kd2].type==CT_NEWLINE;--kd2);
				if(kd2>=top.expansion_start)
				{
					kd=kd2;
					macro_paste(dst.data()+kd, macrodefinition, kt, args2, dst, kd);
				}
				else
					macro_paste(nullptr, macrodefinition, kt, args2, dst, kd);//
				++kd, kt+=2;
			}
			else if(token.type==CT_MACRO_ARG)//normal macro call arg
			{
				auto &arg=args2[(int)token.idata];
				int &kt2=top.kt2;
				for(;kt2<(int)arg.size();)//copy tokens from arg, while checking the arg for macro calls
				{
					auto &token2=arg[kt2];
					if(token2.type==CT_ID)
					{
						auto macro3=macros.find(token2.sdata);
						if(macro3)
						{
							int len2=0;
							if(macro_find_call_extent(*macro3, arg.data(), arg.size(), kt2, len2, args))
							{
								dst.resize(dst.size()+macro3->second.definition.size());
								kt2+=len2;//advance top.kt2 by call length because it will return here
								context.push_back(MacroCall(macro3, 0, std::move(args), 0, kd));//request to expand this argument
								goto macro_expand_again;//should resume here after macro is expanded
							}
							//else
							//	compile_error(dst[kd2-1], "Invalid macro call.");//redundant error message
						}
					}
					dst[kd]=arg[kt2];
					++kd, ++kt2;
				}
				kt2=0;//reset arg index when done
				++kt;
			}
			else//copy token from macro definition
			{
				dst[kd]=token;
				++kd, ++kt;
			}
		}//end loop
		for(int kd2=top.expansion_start;kd2<kd;++kd2)//check the new expansion for macro calls with recursion guard
		{
			auto &token2=dst[kd2];
			switch(token2.type)
			{
			case CT_ID:
				{
					auto macro3=macros.find(token2.sdata);
					if(macro3)
					{
						for(int km=0;km<(int)context.size();++km)//recursion guard
							if(macro3->first==context[km].macro->first)
								goto macro_expand_skip;

						//complete parens in dst
						int level=0;
						int kd3=start;
						for(;kd3<kd;++kd3)
						{
							auto &token3=dst[kd3];
							level+=(token3.type==CT_LPR)-(token3.type==CT_RPR);
						}
						for(int ks2=ks+len;ks2<srcsize;++ks2, ++kd3)
						{
							auto &token3=src[ks2];
							level+=(token3.type==CT_LPR)-(token3.type==CT_RPR);
							if(kd3>=(int)dst.size())
								dst.push_back(src[ks2]);
							else
								dst[kd3]=src[ks2];
							if(!level)//should be down for do while behavior
							{
								++kd3;
								break;
							}
						}
						if(kd<kd3)
							len+=kd3-kd;

						int len2=0;
						if(macro_find_call_extent(*macro3, dst.data(), kd3, kd2, len2, args))//X  what if the rest is in src?
						{
							dst.resize(dst.size()+macro3->second.definition.size());
							kd=kd2;//go back
							top.done=true;//this keeps track of previous macro calls, to guard against infinite cyclic expansion
							context.push_back(MacroCall(macro3, 0, std::move(args), 0, kd));//request to expand this argument
							goto macro_expand_again;
						}
					macro_expand_skip:
						;
					}
				}
				break;
			case CT_FILE:
				token2.type=CT_VAL_STRING_LITERAL;
				token2.sdata=currentfilename;
				break;
			case CT_LINE:
				{
					token2.type=CT_VAL_INTEGER;
					int kd3=kd2-1;
					for(;kd3>=0;--kd3)
						if(dst[kd3].type==CT_NEWLINE)
							break;
					if(kd3>=0)
						token2.idata=dst[kd3].line+1;
					else
						token2.idata=0;
				}
				break;
			}
		}
	macro_expand_endcall:
		context.pop_back();
	}//end while
	for(int kt=start;kt<kd;++kt)//lex the lexme's
	{
		auto &token=dst[kt];
		if(token.type==CT_LEXME)
		{
			LexFile temp_lf;
			temp_lf.text=token.sdata;
			lex(temp_lf);
			int ntokens=temp_lf.expr.size();
			if(!ntokens)
				token.type=CTokenType(-token.type);
			else if(ntokens>1)
				compile_error(token, "Error line %d: Invalid token paste result:\n%s\n", token.sdata);
			else
			{
				auto &t2=temp_lf.expr[0];
				token.type=t2.type;
				token.idata=t2.idata;
			}
		}
	}
	ks+=len;
}

inline void		skip_till_newline(std::vector<Token> const &tokens, int &kt)
{
	for(;kt<(int)tokens.size()&&tokens[kt].type!=CT_NEWLINE;++kt);
}
inline void		skip_till_after_newline(std::vector<Token> const &tokens, int &kt)
{
	skip_till_newline(tokens, kt);
	kt+=kt<(int)tokens.size();
}

//preprocessor blocks
enum PPOpPrec
{
	OPP_ANCHOR,		//operand
	OPP_NOOP,
	OPP_UNARY_PRE,	// ! ~ + -
	OPP_MUL,		// * / %
	OPP_ADD,		// + -
	OPP_SHIFT,		// << >>
	OPP_LESS,		// < <= > >=
	OPP_EQUAL,		// == !=
	OPP_BITAND,		// &
	OPP_BITXOR,		// ^
	OPP_BITOR,		// |
	OPP_LOGICAND,	// &&
	OPP_LOGICOR,	// ||
};
static int		ppop_precedence(CTokenType tokentype, bool bin)
{
	switch(tokentype)
	{
	case CT_VAL_INTEGER:
		return OPP_ANCHOR;

	case CT_EXCLAMATION:
	case CT_TILDE:
		return OPP_UNARY_PRE;

	case CT_PLUS:
	case CT_MINUS:
		if(bin)
			return OPP_ADD;
		return OPP_UNARY_PRE;

	case CT_ASTERIX:
	case CT_SLASH:
	case CT_MODULO:
		return OPP_MUL;

	case CT_SHIFT_LEFT:
	case CT_SHIFT_RIGHT:
		return OPP_SHIFT;

	case CT_LESS:
	case CT_LESS_EQUAL:
	case CT_GREATER:
	case CT_GREATER_EQUAL:
	case CT_EQUAL:
	case CT_NOT_EQUAL:
		return OPP_LESS;

	case CT_AMPERSAND:
		return OPP_BITAND;
	case CT_CARET:
		return OPP_BITXOR;
	case CT_VBAR:
		return OPP_BITOR;
	case CT_LOGIC_AND:
		return OPP_LOGICAND;
	case CT_LOGIC_OR:
		return OPP_LOGICOR;
	}
	return OPP_NOOP;
}
inline long long find_next_anchor(std::vector<Token> const &tokens, int start, int end, int &kn)//kn must be inisialized
{
	Token const *token=nullptr;
	for(;kn<end;++kn)//find next anchor
	{
		token=&tokens[kn];
		if(token->type==CT_VAL_INTEGER)
			return token->idata;
	}
	if(kn==end)//reached the end
	{
		--kn;
		for(;kn>=start;--kn)//find last anchor
		{
			token=&tokens[kn];
			if(token->type==CT_VAL_INTEGER)
				return token->idata;
		}
	}
	if(kn>=start&&kn<end)
		compile_error(tokens[kn], "Expected an integer.");
	else if(start<end)
		compile_error(tokens[start], "Expected an integer.");
	return 0;
}
inline bool		is_binary(std::vector<Token> const &tokens, int start, int lk)//NOTE[1]: this works because there are no unary-post operators in the c preprocessor
{
	int lk2=lk-1;
	for(;lk2>=start;--lk2)
		if(tokens[lk2].type>CT_IGNORED)
			break;
	return lk2>=start&&ppop_precedence(tokens[lk2].type, true)<=OPP_NOOP;//left is binary only if there is something right before it that is not an operator. See note [1].
}
static long long eval_expr_flat(std::vector<Token> &tokens, int start, int end)//only for expressions withoud unary-post operators
{
	Token *token=nullptr;
	int kn=start, kn0=-1;
	long long result=find_next_anchor(tokens, start, end, kn);

	for(;;)//main eval loop
	{
		int lk=kn-1, rk=kn+1;
		for(;lk>=start;--lk)//find left operator
			if(tokens[lk].type>CT_IGNORED)
				break;
		for(;rk<end;++rk)//find right operator
			if(tokens[rk].type>CT_IGNORED)
				break;
		bool dir_left=false, left_bin=false;
		if(lk>=start)//found left operator
		{
			auto lop=&tokens[lk];
			left_bin=is_binary(tokens, start, lk);
			int lp=ppop_precedence(lop->type, left_bin);
			if(lp<=OPP_NOOP)
			{
				compile_error(tokens[lk], "Expected an operator.");//need better error diagnostic
				goto eval_flat_no_left;
			}
			if(rk<end)//both left & right found: select which one to take
			{
				auto rop=&tokens[rk];
				int rp=ppop_precedence(rop->type, true);//Always bin. See note [1] above.
				if(rp<=OPP_NOOP)
				{
					compile_error(tokens[rk], "Expected an operator.");//need better error diagnostic
					goto eval_flat_no_right;
				}
				dir_left=lp<=rp;//on precedence match, prefer left
			}
			else
			eval_flat_no_right:
				dir_left=true;
		}
		else
		{
		eval_flat_no_left:
			if(rk<end)//found right operator
				dir_left=false;
			else//no operators found
				break;
		}
		if(dir_left)
		{
			auto lop=&tokens[lk];
			if(left_bin)
			{
				kn0=lk-1;
				for(;kn0>=start;--kn0)//find previous anchor
					if(tokens[kn0].type>CT_IGNORED)
						break;
				if(tokens[kn0].type!=CT_VAL_INTEGER)
				{
					compile_error(tokens[kn0], "Expected an integer.");
					if(kn0>=start)
						tokens[kn0].type=CTokenType(-tokens[kn0].type);
					lop->type=CTokenType(-lop->type);
				}
				auto op1=&tokens[kn0], op2=&tokens[kn];
				switch(lop->type)
				{
				case CT_PLUS:			op1->idata=op1->idata+op2->idata;break;
				case CT_MINUS:			op1->idata=op1->idata-op2->idata;break;
				case CT_ASTERIX:		op1->idata=op1->idata*op2->idata;break;
				case CT_SLASH:
					if(op2->idata)
						op1->idata=op1->idata/op2->idata;
					else
					{
						compile_error(*op2, "Division by zero.");
						op1->type=CT_VAL_FLOAT;
						op1->fdata=_HUGE;
					}
					break;
				case CT_MODULO:
					if(op2->idata)
						op1->idata=op1->idata%op2->idata;
					else
					{
						compile_warning(*op2, "Division by zero.");
						op1->idata=0;
					}
					break;
				case CT_SHIFT_LEFT:		op1->idata=op1->idata<<op2->idata;break;
				case CT_SHIFT_RIGHT:	op1->idata=op1->idata>>op2->idata;break;
				case CT_LESS:			op1->idata=op1->idata<op2->idata;break;
				case CT_LESS_EQUAL:		op1->idata=op1->idata<=op2->idata;break;
				case CT_GREATER:		op1->idata=op1->idata>op2->idata;break;
				case CT_GREATER_EQUAL:	op1->idata=op1->idata>=op2->idata;break;
				case CT_EQUAL:			op1->idata=op1->idata==op2->idata;break;
				case CT_NOT_EQUAL:		op1->idata=op1->idata!=op2->idata;break;
				case CT_AMPERSAND:		op1->idata=op1->idata&op2->idata;break;
				case CT_CARET:			op1->idata=op1->idata^op2->idata;break;
				case CT_VBAR:			op1->idata=op1->idata|op2->idata;break;
				case CT_LOGIC_AND:		op1->idata=op1->idata&&op2->idata;break;
				case CT_LOGIC_OR:		op1->idata=op1->idata||op2->idata;break;
				}
				op2->type=CTokenType(-op2->type);
				kn=kn0;
			}
			else
			{
				auto op1=&tokens[kn];
				switch(lop->type)
				{
				case CT_EXCLAMATION:op1->idata=!op1->idata;	break;
				case CT_TILDE:		op1->idata=~op1->idata;	break;
				case CT_PLUS:								break;
				case CT_MINUS:		op1->idata=-op1->idata;	break;
				}
			}
			lop->type=CTokenType(-lop->type);
		}
		else
		{
			++kn;
			find_next_anchor(tokens, start, end, kn);
		}
	}//end eval loop
	return result;
}
inline int		find_question_or_colon(std::vector<Token> &tokens, int start)
{
	int ntokens=tokens.size();
	for(;start<ntokens;++start)//find question/colon/end
	{
		auto token=&tokens[start];
		if(token->type==CT_QUESTION||token->type==CT_COLON)
			break;
	}
	return start;
}
static long long eval_expr_ternary(std::vector<Token> &tokens, int start, int end)//recursive
{
	int ntokens=tokens.size();
	int qend=find_question_or_colon(tokens, start);//find question/colon/end
	long long result=eval_expr_flat(tokens, start, end);
	if(qend<end&&tokens[qend].type==CT_QUESTION)
	{
		int separator=qend+1;
		int level=1;
		for(;separator<ntokens;++separator)//find matching colon after qend
		{
			auto token=&tokens[separator];
			level+=(token->type==CT_QUESTION)-(token->type==CT_COLON);
			if(!level)
				break;
		}
		if(result)
			result=eval_expr_ternary(tokens, qend+1, separator);
		else
		{
			qend=find_question_or_colon(tokens, separator+1);
			result=eval_expr_ternary(tokens, separator+1, qend);
		}
	}
	return result;
}
static long long eval_expr(MacroLibrary const &macros, std::vector<Token> const &tokens, int start, int end)
{
	int ntokens=end-start;
	std::vector<Token> t2(ntokens);
	{
		MacroLibrary::EType const *macro=nullptr;
		int kd=0;
		for(int ks=start;ks<end;)//expand macros & resolve 'defined' operator
		{
			auto token=&tokens[ks];
			if(token->type==CT_DEFINED)
			{
				if(ks+1>=end)
					compile_error(*token, "\'defined\': Expected an identifier.");
				else
				{
					++ks;
					token=&tokens[ks];
					auto token2=&t2[kd];
					token2->type=CT_VAL_INTEGER;
					token2->idata=macros.find(token->sdata)!=0;
					++kd;
				}
				++ks;
			}
			else if(token->type==CT_ID&&(macro=macros.find(token->sdata)))
				macro_expand(macros, *macro, tokens.data(), tokens.size(), ks, t2, kd);
			else
			{
				if(token->type>CT_IGNORED)
				{
					t2[kd]=*token;
					++kd;
				}
				++ks;
			}
		}
		t2.resize(kd);
	}
	ntokens=t2.size();
	if(!ntokens)
	{
		auto token=&tokens[start-(start>=(int)tokens.size())];
		compile_error(*token, "Expected an integer expression.");
		return 0;
	}
	long long result=0;
	Token *token=nullptr;
	for(;;)//evaluation loop
	{
		int toplevel=0, fstart=0, fend=ntokens;
		int level=0;
		bool peak=false;
		for(int k=0;k<ntokens;++k)
		{
			token=&t2[k];
			if(token->type==CT_LPR)
			{
				++level;
				if(toplevel<level)
					toplevel=level, fstart=k, peak=true;
			}
			else if(token->type==CT_RPR)
			{
				--level;
				if(peak)
					fend=k, peak=false;
				if(level<0)
					compile_error(*token, "Unmatched closing parenthesis \')\'.");
			}
		}
		if(level)//TODO: check what happens next here
		{
			auto token=&t2[0];
			if(level<0)
				compile_error(*token, "Extra unmatched closing parentheses \')\' in expression.");
			else
				compile_error(*token, "Extra unmatched opening parentheses \'(\' in expression.");
		}
		if(toplevel)//clear parentheses
		{
			t2[fstart].type=CTokenType(-t2[fstart].type);
			t2[fend].type=CTokenType(-t2[fend].type);
		}

		result=eval_expr_ternary(t2, fstart, fend);

		if(!toplevel)
			break;
	}
	return result;
}

static int		skip_block(MacroLibrary const &macros, std::vector<Token> const &tokens, int &k, bool lastblock)//returns 0 if block ends, 1 if more blocks follow
{
	int k0=k;
	int ntokens=tokens.size(), level=1;
	for(;k<ntokens;)
	{
		auto token=&tokens[k];
		++k;
		if(token->type==CT_HASH)
		{
			token=&tokens[k];
			++k;//k points at next token
			switch(token->type)
			{
			case CT_IF:case CT_IFDEF:
				++level;
				break;
			case CT_ELIF:
				if(lastblock)
					compile_error(*token, "#else already appeared. Expected #endif.");
				else if(level==1)
				{
					int start=k;
					skip_till_newline(tokens, k);
					auto result=eval_expr(macros, tokens, start, k);
					k+=k<ntokens;//skip newline
					if(result)
						return 1;
				}
				break;
			case CT_ELSE:
				if(lastblock)
					compile_error(*token, "#else already appeared. Expected #endif.");
				else if(level==1)
				{
					int start=k;
					skip_till_newline(tokens, k);
					if(start<k)
						compile_error(tokens[start], "Unexpected tokens after #else.");
					k+=k<ntokens;//skip newline
					return 1;
				}
				break;
			case CT_ENDIF:
				--level;
				if(!level)
				{
					int start=k;
					skip_till_newline(tokens, k);
					if(start<k)
						compile_error(tokens[start], "Unexpected tokens after #endif.");
					k+=k<ntokens;//skip newline
					return 0;
				}
				break;
			}//end switch
			skip_till_after_newline(tokens, k);
		}//end for
	}
	if(k>=ntokens)
	{
		auto token=&tokens[k0];
		compile_error(*token, "Unexpected #endif.");
	}
	return 0;
}

//include system
//typedef std::set<LexFile*> LexFileSet;
typedef std::map<char*, LexFile*> LexLibrary;//filename -> content
//static LexFileSet lexedfiles;
static LexLibrary lexlibrary;
struct			Bookmark
{
	LexFile *lf;
	int ks, iflevel;
	Bookmark(LexFile *lf, int ks, int iflevel):lf(lf), ks(ks), iflevel(iflevel){}
};
static char*	test_include(const char *searchpath, const char *includename)
{
	std::string filename=searchpath;
	filename+=includename;
	int ret=file_is_readable(filename.c_str());
	if(ret==1)
		return add_string(filename);
	return 0;
}
static char*	find_include(const char *includename, int custom)
{
	char *filename=nullptr;
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
		std::string currentfolder(currentfilename, k);
		filename=test_include(currentfolder.c_str(), includename);
		if(filename)
			return filename;
	}
	for(int ki=0;ki<(int)includepaths.size();++ki)
	{
		filename=test_include(includepaths[ki], includename);
		if(filename)
			return filename;
	}
	if(projectfolder)//finally check the current working directory
	{
		filename=test_include(nullptr, includename);
		if(filename)
			return filename;
	}
	return nullptr;
}

void			preprocess(MacroLibrary &macros, LexFile &lf)//LexFile::text is optional, LexFile::filename must be provided		TODO: pass predefined macros
{
	if(!lf.filename)
	{
		printf("INTERNAL ERROR: Preprocessor: Expression filename is NULL.\n");
		compile_status=CS_FATAL_ERRORS;
		return;
	}
	const char *p_once=nullptr;
	{
		p_va_args=add_string(keywords[CT_VA_ARGS]);
		p_once=add_string("once");
		prof.add("prepare strings");
	}
	
#if 0//lexer test
	long long t1=__rdtsc();
	lex(lf);
	long long t2=__rdtsc();
	printf("Lexed in %lld cycles\n", t2-t1);
	prof.add("lex source");
#else
	bool lexed_before=false;
	auto clf_pair=&lexlibrary.insert_no_overwrite(LexLibrary::EType(lf.filename, nullptr), &lexed_before);
	LexFile *clf;
	if(lexed_before)
		clf=clf_pair->second;
	else
	{
		clf=clf_pair->second=new LexFile();//'new' calls constructors
		clf->filename=lf.filename;
		clf->flags=EXPR_INCLUDE_ONCE;

		if(file_is_readable(clf->filename)!=1)
		{
			printf("Error: Cannot open \'%s\'.\n", clf->filename);
			return;
		}

		lex(*clf);
		prof.add("lex source");
	}

	std::stack<Bookmark> bookmarks;
	bookmarks.push(Bookmark(clf, 0, 0));
	int kd=0;

	while(bookmarks.size())
	{
	preprocess_start:
		clf=bookmarks.top().lf;//current lexed file
		int &ks=bookmarks.top().ks;//source token idx

		currentfilename=clf->filename;
		currentfile=&clf->text;

		lf.expr.resize(kd+clf->expr.size()-ks);
		MacroLibrary::EType *macro=nullptr;
		int &iflevel=bookmarks.top().iflevel;
		int ntokens=clf->expr.size();
		for(;ks<ntokens;)//preprocessor loop
		{
			auto token=&clf->expr[ks];
			switch(token->type)//TODO: directives should only work at the beginning of line
			{
			case CT_HASH:
#if 1
				++ks;
				if(ks>=ntokens)
				{
					compile_error(*token, "Unexpected end of file. Expected a preprocessor directive.");
					continue;
				}
				token=&clf->expr[ks];
				++ks;//to point at next token
				switch(token->type)
				{
				case CT_DEFINE:
					ks+=macro_define(macros, clf->expr.data()+ks, ntokens-ks);//TODO: pass vector to use operator[], use skip_till_newline()
					ks+=ks<ntokens&&clf->expr[ks].type==CT_NEWLINE;//skip newline
					break;
				case CT_UNDEF:
					token=&clf->expr[ks];
					if(token->type!=CT_ID)
						compile_error(*token, "Expected an identifier after #undef.");
					else
					{
						int result=macros.erase(token->sdata);
						if(result==-1)
							compile_error(*token, "\'%s\' is already a defined macro.", token->sdata);
						++ks;
						int start=ks;
						skip_till_newline(clf->expr, ks);
						if(start<ks)
							compile_error(clf->expr[start], "Expected a single token after #undef.");
						ks+=ks<ntokens;
					}
					break;

				case CT_IFDEF:
				case CT_IFNDEF:
					token=&clf->expr[ks];
					if(token->type!=CT_ID)
						compile_error(*token, "Expected an identifier.");
					else
					{
						if(!macros.find(token->sdata)!=(clf->expr[ks-1].type==CT_IFNDEF))
							iflevel+=skip_block(macros, clf->expr, ks, false);//skip till #elif true/else/endif
						else
						{
							++iflevel;
							++ks;
							int start=ks;
							skip_till_newline(clf->expr, ks);
							if(start<ks)
								compile_error(clf->expr[start], "Expected a single token after #ifdef/ifndef.");
							ks+=ks<ntokens;
						}
					}
					break;
				case CT_IF:
					{
						int start=ks;
						skip_till_newline(clf->expr, ks);
						auto result=eval_expr(macros, clf->expr, start, ks);
						if(result)
						{
							++iflevel;
							ks+=ks<ntokens;
						}
						else
							iflevel+=skip_block(macros, clf->expr, ks, false);//skip till #elif true/else/endif
					}
					break;
				case CT_ELIF:
				case CT_ELSE:
					{
						int start=ks;
						skip_till_newline(clf->expr, ks);
						bool extratokens=start<ks, expected=iflevel>0, lastblock=token->type==CT_ELSE;
						if(expected)
						{
							skip_block(macros, clf->expr, ks, lastblock);//just skip till #endif
							--iflevel;
						}
						else//no else/elif expected
							compile_error(*token, "Unexpected #else/elif.");
						if(lastblock&&extratokens)
							compile_error(clf->expr[start], "Unexpected tokens after #else.");
						if(!expected)
							ks+=ks<ntokens;//skip newline
					}
					break;
				case CT_ENDIF:
					{
						if(!iflevel)
							compile_error(*token, "Unmatched #endif.");
						else
							--iflevel;
						int start=ks;
						skip_till_newline(clf->expr, ks);
						if(start<ks)
							compile_error(clf->expr[start], "Unexpected tokens after #endif.");
						ks+=ks<ntokens;
					}
					break;

				case CT_INCLUDE:
					token=&clf->expr[ks];
					if(token->type==CT_INCLUDENAME_STD||token->type==CT_VAL_STRING_LITERAL)
					{
						char *filename=find_include(token->sdata, token->type==CT_VAL_STRING_LITERAL);
						if(!filename)
						{
							compile_error(*token, "Cannot open include file \'%s\'.", token->sdata);
							skip_till_after_newline(clf->expr, ks);
						}
						else
						{
							bool found=false;
							auto &lf_pair=lexlibrary.insert_no_overwrite(LexLibrary::EType(filename, nullptr), &found);
							if(!found)//not lexed before
							{
								lf_pair.second=new LexFile();
								lf_pair.second->filename=filename;
								lex(*lf_pair.second);
							}
							skip_till_after_newline(clf->expr, ks);
							if(!(lf_pair.second->flags&EXPR_INCLUDE_ONCE))//not marked with '#pragma once'
							{
								bookmarks.push(Bookmark(lf_pair.second, 0, 0));
								goto preprocess_start;
							}
						}
					}
					else
					{
						compile_error(*token, "Expected include file name.");
						skip_till_after_newline(clf->expr, ks);
					}
					break;
				case CT_PRAGMA:
					token=&clf->expr[ks];//TODO: check for end of file
					if(token->type==CT_ID)
					{
						if(token->sdata==p_once)//'#pragma once': header is included once per source
							clf->flags|=EXPR_INCLUDE_ONCE;
					}
					skip_till_after_newline(clf->expr, ks);
					break;
				case CT_ERROR:
					if(ks<ntokens&&clf->expr[ks].type==CT_VAL_STRING_LITERAL)
					{
						auto token2=&clf->expr[ks];
						compile_error(*token, "%s", token2->sdata);//error message may contain '%'
					}
					else
						compile_error(*token, "#error directive found.");
					skip_till_after_newline(clf->expr, ks);
					break;
				default:
					compile_error(*token, "Expected a preprocessor directive (define/undef/if/ifdef/ifndef/pragma/error).");
					skip_till_after_newline(clf->expr, ks);
					break;
				}
#endif
				break;
			case CT_FILE:
				{
					auto dtok=&lf.expr[kd];
					*dtok=*token;
					dtok->type=CT_VAL_STRING_LITERAL;
					dtok->sdata=currentfilename;
					++kd, ++ks;
				}
				break;
			case CT_LINE:
				{
					auto dtok=&lf.expr[kd];
					*dtok=*token;
					dtok->type=CT_VAL_INTEGER;
					dtok->idata=dtok->line+1;
					++kd, ++ks;
				}
				break;
			case CT_ID:
				if(macro=macros.find(token->sdata))
					macro_expand(macros, *macro, clf->expr.data(), clf->expr.size(), ks, lf.expr, kd);
				else
				{
					if(token->type>CT_IGNORED)
					{
						lf.expr[kd]=*token;
						++kd;
					}
					++ks;
				}
				break;
			default:
				if(token->type>CT_IGNORED&&(kd==0||token->type!=CT_NEWLINE||lf.expr[kd-1].type!=CT_NEWLINE))
				{
					lf.expr[kd]=*token;
					++kd;
				}
				++ks;
				break;
			}
		}//end preprocess loop
		if(iflevel>0)
			compile_error(clf->expr.back(), "End of file reached. Expected %s #endif\'s.", iflevel);
		bookmarks.pop();
	}//end file loop
	prof.add("preprocess");

	lf.expr.resize(kd);
	prof.add("resize");

	for(int k=0;k<(int)lexlibrary.size();++k)//reset '#pragma once' so that lexed headers are included in an another source
		lexlibrary.data()[k].second->flags=EXPR_NORMAL;
	prof.add("reset lexedfiles flags");
#endif
}
void			expr2text(Expression const &ex, std::string &text)
{
	text.clear();
	bool spaceprinted=false;
	for(int kt=0;kt<(int)ex.size();++kt)
	{
		auto &token=ex[kt];
		if(token.type<=CT_IGNORED)
			continue;

		if(!spaceprinted&&token.prevwhitespace&&!token.prevnewline)
			text+=' ';
		spaceprinted=false;

		auto keyword=keywords[token.type];
		if(keyword)
			text+=keyword;
		else
		{
			switch(token.type)
			{
			case CT_VAL_INTEGER:
				sprintf_s(g_buf, g_buf_size, "%lld", token.idata);
				text+=g_buf;
				break;
			case CT_VAL_FLOAT:
				sprintf_s(g_buf, g_buf_size, "%lf", token.fdata);
				text+=g_buf;
				break;
			case CT_ID:
				text+=token.sdata;
				break;
			case CT_VAL_CHAR_LITERAL:
				{
					text+='\'';
					std::string processed;
					str2esc(token.sdata, token.len, processed);
					text+=processed;
					text+='\'';
				}
				break;
			case CT_VAL_STRING_LITERAL:
			case CT_VAL_WSTRING_LITERAL:
				{
					if(token.type==CT_VAL_WSTRING_LITERAL)
						text+='L';
					text+='\"';
					std::string processed;
					str2esc(token.sdata, token.len, processed);
					text+=processed;
					text+='\"';
				}
				break;
			case CT_INCLUDENAME_CUSTOM:
				text+='\"';
				text+=token.sdata;
				text+='\"';
				break;
			case CT_INCLUDENAME_STD:
				text+='<';
				text+=token.sdata;
				text+='>';
				break;
			}
		}
		if(token.nextwhitespace&&!token.nextnewline)
		{
			text+=' ';
			spaceprinted=true;
		}
	}
}
void			lexedfiles_destroy()
{
	for(int k=0;k<(int)lexlibrary.size();++k)
		delete lexlibrary.data()[k].second;//'delete' calls destructors
	lexlibrary.clear();
}
void			stringlib_destroy()
{
	for(int k=0;k<(int)strings.size();++k)
		_aligned_free(strings.data()[k]);
	strings.clear();
}