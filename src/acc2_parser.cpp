#include		"acc2.h"
#include		"acc2_codegen_x86_64.h"
const char*		token2str(CTokenType tokentype)
{
	switch(tokentype)
	{
#define TOKEN(STRING, LABEL, FLAGS)	case LABEL:return #LABEL;
#include"acc2_keywords_c.h"
#undef TOKEN
	}
	return "<UNDEFINED>";
}
/*void			error_parse(Token const &token, const char *format, ...)
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
}//*/
//initial ideas
#if 0
//struct		FuncDecl
//{
//	const char *name;
//	std::vector<CTokenType> args;
//};
enum			LinkageType
{
	LINK_EXTERN_MANGLED,
	LINK_STATIC_MANGLED,
	LINK_EXTERN_C,
};
enum			CallType
{
	CALL_STD,
	CALL_CDECL,
	CALL_THIS,
};
struct			VarDefinition
{
	std::vector<Expression> decl;
	LinkageType linkagetype;
};
struct			FuncDefinition
{
	std::vector<Expression> decl, body;
	LinkageType linkagetype;
	CallType calltype;
};
typedef std::map<const char*, std::vector<Expression>> TypeDefLibrary;
typedef std::map<const char*, VarDefinition> VarLibrary;
typedef std::map<const char*, FuncDefinition> FuncLibrary;
typedef std::map<CTokenType, FuncDefinition> OpLibrary;
static TypeDefLibrary typedefs;
static VarLibrary gvars;
static FuncLibrary funcs;
static OpLibrary operators;
static std::vector<const char*> currentnamespace;//works like a stack
#endif

//Programming an x64 compiler from scratch - part 2		https://www.youtube.com/watch?v=Mx29YQ4zAuM		3:32:00 - code generation
#if 1
//char			register_allocation_order[]=
//{
//	RAX, RCX, RBX, 
//};
int				first_reg=R15;
inline int		get_next_register(int reg)
{
	assert(reg>R8);
	return reg-1;
}
//char register_alloc_order[]=
//{
//	R15,
//	R14,
//	R13,
//	R12,
//	R11,
//	R10,
//	R9,
//	R8,
//};
//inline int		get_register(int index)
//{
//	assert(index<SIZEOF(register_alloc_order));
//	return register_alloc_order[index];
//}
//inline int		get_next_register(int index)
//{
//	assert(index<SIZEOF(register_alloc_order));
//	return register_alloc_order[index+1];
//
//	//assert(r<=R15);
//	//return r+1+(r==RDX-1);
//	//return r+1;
//}
/*int			next_register=0;
int				allocate_register()
{
	assert(next_register<=R15);
	return next_register++;
}
void			free_register()
{
	assert(next_register>RAX);
	--next_register;
}//*/
static Expression const *current_expr=nullptr;//set by read_token()
static int token_idx=0;
static Token const *prev_token=nullptr, *current_token=nullptr;
inline bool		read_token()
{
	while(token_idx<(int)current_expr->size())
	{
		prev_token=current_token;
		current_token=&current_expr->operator[](token_idx);
		++token_idx;
		if(current_token->type>CT_IGNORED&&current_token->type<=CT_ACC2)
			return true;
	}
	return false;
}
inline Token const* peek_token(Token const *c)
{
	int peek=c-current_expr->data();
	++peek;
	while(peek<(int)current_expr->size())
	{
		Token const *token=&current_expr->operator[](peek);
		if(token->type>CT_IGNORED&&token->type<=CT_ACC2)
			return token;
		++peek;
	}
	return nullptr;
}
inline void		seek2token(Token const *token)
{
	current_token=token;
	token_idx=token+1-current_expr->data();
}

//enum			OperandTypeExt//TODO: use register numbers, immediate types have negative values
//{
//	OPERAND_IMM8=-3,
//	OPERAND_IMM4=-2,
//	OPERAND_IMM1=-1,
//};
enum			OperandType
{
	OPERAND_UNINITIALIZED=0,
	OPERAND_IMM8='8',
	OPERAND_REG_TEMP='E',//temp reg
	OPERAND_REG_REFERENCE='R',//reference to a variable
};
char			used_registers[16]={};
int				get_new_reg()
{
	int k=0;
	for(;k<16;++k)
	{
		auto &rk=used_registers[k];
		if(k==RAX||k==RDX)//use 14 of 16 x86-64 registers
			continue;
		++rk;
		if(rk==1)
			break;
	}
	assert2(k<16, "Out of registers.");
	return k;
}
struct			Operand
{
	byte type;
	//byte islvalue;
	//7 bytes padding
	union
	{
		char reg;
		struct
		{
			unsigned addr;
			const char *id;
		};
		//struct
		//{
		//	char reg;
		//	//3 bytes padding
		//	const char *id;
		//};
		i64 imm8;
	};
	Operand():type(OPERAND_UNINITIALIZED), imm8(0){}
	//Operand():type(OPERAND_UNINITIALIZED), islvalue(false), imm8(0){}
	void mov2reg()
	{
		assert2(type!=OPERAND_UNINITIALIZED, "Uninitialized operand.");
		if(type==OPERAND_IMM8)
		{
			int k=get_new_reg();
			EMIT_MOV_R_I(k, imm8);
			type=OPERAND_REG_TEMP;
			imm8=k;
		}
	}
};
void			reset_regstate()
{
	memset(used_registers, 0, 16);
}
inline void		free_register(char &reg)
{
	if(reg>=0)
	{
		used_registers[reg]=0;
		reg=-1;
	}
}

struct			Variable
{
	char reg;
	bool initialized;
	Variable():reg(0), initialized(false){}
	Variable(char reg):reg(reg), initialized(false){}
};
typedef std::map<const char*, Variable> VarRegLib;
static VarRegLib variables;

static i64		parse_temp_result=0;
static Token const *parse_temp_token=nullptr;

static void		parse_assignment(Operand *dst);
static void		parse_unary(Operand *dst)
{
	switch(current_token->type)
	{
	case CT_VAL_INTEGER:
		dst->type=OPERAND_IMM8;
		dst->imm8=current_token->idata;
		read_token();//skip the integer
		break;
	case CT_ID:
		{
			parse_temp_token=current_token;
			read_token();//skip the variable
			auto id_var=variables.find(parse_temp_token->sdata);//reference to an int
			if(!id_var)
				error_pp(*parse_temp_token, "Undeclared identifier: \'%s\'.", parse_temp_token->sdata);
			else
			{
				dst->type=OPERAND_REG_REFERENCE;
				dst->reg=id_var->second.reg;
				dst->id=id_var->first;

				//dst->type=OPERAND_REG_TEMP;
				//dst->reg=get_new_reg();
				//EMIT_R_R(MOV, dst->reg, id_var->second.reg);
			}
		}
		break;
	case CT_LPR:
		read_token();
		parse_assignment(dst);
		if(current_token->type==CT_RPR)
			read_token();//skip closing parenthesis
		else
			error_pp(*current_token, "Parenthesis mismatch.");
		break;
	default:
		error_pp(*current_token, "Unexpected token: %d: %s", current_token->type, token2str(current_token->type));
		break;
	}
}
static void		parse_product(Operand *dst)
{
	parse_unary(dst);
	while(current_token->type==CT_ASTERIX||current_token->type==CT_SLASH||current_token->type==CT_MODULO)
	{
		Operand next;
		switch(current_token->type)
		{
		case CT_ASTERIX:
			parse_temp_token=current_token;
			read_token();//skip operator
			parse_unary(&next);
			if(dst->type==OPERAND_IMM8)
			{
				if(next.type==OPERAND_IMM8)	//dst=imm, next=imm
					dst->imm8*=next.imm8;
				else						//dst=imm, next=reg
				{
					dst->type=OPERAND_REG_TEMP;
					if(is_pot(dst->imm8))//dst.reg=next.reg<<log2(dst->imm)
					{
						if(next.type==OPERAND_REG_REFERENCE)
						{
							int k=get_new_reg();
							EMIT_R_R(MOV, k, next.reg);
							next.imm8=k;
						}
						EMIT_R_I1(SHL, next.reg, floor_log2(dst->imm8));
						dst->imm8=next.reg;
					}
					else//dst.reg=next.reg*dst->imm
					{
						EMIT_MOV_R_I(RAX, dst->imm8);
						EMIT_X_R(MUL, next.reg);//{hi=RDX|lo=RAX} = RAX * operand
						if(next.type==OPERAND_REG_REFERENCE)
							dst->imm8=get_new_reg();
						else//next is temp
							dst->imm8=next.reg;//take the temp reg
						EMIT_R_R(MOV, dst->reg, RAX);				//EXTRA MOV
					}
				}
			}
			else//dst->type == temp or reg
			{
				if(next.type==OPERAND_IMM8)	//dst=reg, next=imm
				{
					if(is_pot(next.imm8))//dst<<=log2(next.imm)
					{
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							int k=get_new_reg();
							EMIT_R_R(MOV, k, dst->reg);
							dst->imm8=k;
						}
						EMIT_R_I1(SHL, dst->reg, floor_log2(next.imm8));
					}
					else//dst*=next.imm
					{
						EMIT_MOV_R_I(RAX, next.imm8);
						EMIT_X_R(MUL, dst->reg);//SAWPPED
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							dst->imm8=get_new_reg();
						}
						EMIT_R_R(MOV, dst->reg, RAX);
					}
				}
				else						//dst=reg, next=reg
				{
					EMIT_R_R(MOV, RAX, dst->reg);
					EMIT_X_R(MUL, next.reg);
					if(dst->type==OPERAND_REG_REFERENCE)
					{
						dst->type=OPERAND_REG_TEMP;
						if(next.type==OPERAND_REG_TEMP)
							dst->imm8=next.reg;
						else//next is ref
							dst->imm8=get_new_reg();
					}
					else if(next.type==OPERAND_REG_TEMP)
						free_register(next.reg);
					EMIT_R_R(MOV, dst->reg, RAX);
				}
			}
			break;
		case CT_SLASH:
			parse_temp_token=current_token;
			read_token();//skip operator
			parse_unary(&next);
			if(dst->type==OPERAND_IMM8)
			{
				if(next.type==OPERAND_IMM8)	//dst=imm, next=imm
				{
					if(next.imm8)
						dst->imm8/=next.imm8;
					else
						error_pp(*parse_temp_token, "Integer division by zero.");
				}
				else						//dst=imm, next=reg		dst.reg=dst->imm/next.reg
				{
					EMIT_MOV_R_I(RAX, dst->imm8);
					EMIT_X_R(DIV, next.reg);//{hi=RDX|lo=RAX} = RAX * operand
					dst->type=OPERAND_REG_TEMP;
					if(next.type==OPERAND_REG_REFERENCE)
						dst->imm8=get_new_reg();
					else//next is temp
						dst->imm8=next.reg;//take the temp reg
					EMIT_R_R(MOV, dst->reg, RAX);
				}
			}
			else//dst->type == temp or reg
			{
				if(next.type==OPERAND_IMM8)	//dst=reg, next=imm
				{
					if(is_pot(dst->imm8))//dst.reg>>=log2(next.imm)
					{
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							int k=get_new_reg();
							EMIT_R_R(MOV, k, dst->reg);
							dst->imm8=k;
						}
						EMIT_R_I1(SLR, dst->reg, floor_log2(next.imm8));
						//EMIT_R_I1(SAR, dst->reg, floor_log2(next.imm8));
					}
					else//dst.reg=dst->reg/next.imm
					{
						EMIT_R_R(MOV, RAX, dst->reg);
						EMIT_MOV_R_I(RDX, next.imm8);
						EMIT_X_R(DIV, RDX);
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							dst->imm8=get_new_reg();
						}
						EMIT_R_R(MOV, dst->reg, RAX);
					}
				}
				else						//dst=reg, next=reg
				{
					EMIT_R_R(MOV, RAX, dst->reg);
					EMIT_X_R(DIV, next.reg);
					if(dst->type==OPERAND_REG_REFERENCE)
					{
						dst->type=OPERAND_REG_TEMP;
						if(next.type==OPERAND_REG_TEMP)
							dst->imm8=next.reg;
						else//next is ref
							dst->imm8=get_new_reg();
					}
					else if(next.type==OPERAND_REG_TEMP)
						free_register(next.reg);
					EMIT_R_R(MOV, dst->reg, RAX);
				}
			}
			break;
		case CT_MODULO:
			parse_temp_token=current_token;
			read_token();//skip operator
			parse_unary(&next);
			if(dst->type==OPERAND_IMM8)
			{
				if(next.type==OPERAND_IMM8)	//dst=imm, next=imm
				{
					if(next.imm8)
						dst->imm8%=next.imm8;
					else
						error_pp(*parse_temp_token, "Integer division by zero.");
				}
				else						//dst=imm, next=reg
				{
					EMIT_MOV_R_I(RAX, dst->imm8);
					EMIT_X_R(DIV, next.reg);//{hi=RDX|lo=RAX} = RAX * operand
					dst->type=OPERAND_REG_TEMP;
					if(next.type==OPERAND_REG_REFERENCE)
						dst->imm8=get_new_reg();
					else//next is temp
						dst->imm8=next.reg;//take the temp reg
					EMIT_R_R(MOV, dst->reg, RDX);
				}
			}
			else//dst->type == temp or reg
			{
				if(next.type==OPERAND_IMM8)	//dst=reg, next=imm
				{
					if(is_pot(next.imm8))//dst.reg>>=log2(next.imm)
					{
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							int k=get_new_reg();
							EMIT_R_R(MOV, k, dst->reg);
							dst->imm8=k;
						}
						EMIT_R_I(AND, dst->reg, next.imm8-1);
					}
					else
					{
						EMIT_R_R(MOV, RAX, dst->reg);
						EMIT_MOV_R_I(RDX, next.imm8);
						EMIT_X_R(DIV, RDX);
						if(dst->type==OPERAND_REG_REFERENCE)
						{
							dst->type=OPERAND_REG_TEMP;
							dst->imm8=get_new_reg();
						}
						EMIT_R_R(MOV, dst->reg, RDX);
					}
				}
				else						//dst=reg, next=reg
				{
					EMIT_R_R(MOV, RAX, dst->reg);
					EMIT_X_R(DIV, next.reg);
					if(dst->type==OPERAND_REG_REFERENCE)
					{
						dst->type=OPERAND_REG_TEMP;
						if(next.type==OPERAND_REG_TEMP)
							dst->imm8=next.reg;
						else//next is ref
							dst->imm8=get_new_reg();
					}
					else if(next.type==OPERAND_REG_TEMP)
						free_register(next.reg);
					EMIT_R_R(MOV, dst->reg, RDX);
				}
			}
			break;
		}
	}
}
static void		parse_summation(Operand *dst)
{
	parse_product(dst);
	while(current_token->type==CT_PLUS||current_token->type==CT_MINUS)
	{
		Operand next;
		switch(current_token->type)
		{
		case CT_PLUS://dst + next
			read_token();//skip +/-
			parse_product(&next);
			switch(dst->type)
			{
			case OPERAND_IMM8:
				switch(next.type)
				{
				case OPERAND_IMM8:			//dst=imm, next=imm
					dst->imm8+=next.imm8;
					break;
				case OPERAND_REG_TEMP:		//dst=imm, next=temp
					dst->type=OPERAND_REG_TEMP;
					dst->imm8=next.reg;
					EMIT_R_I(ADD, next.reg, dst->imm8);//SWAPPED
					break;
				case OPERAND_REG_REFERENCE:	//dst=imm, next=ref
					{
						dst->type=OPERAND_REG_TEMP;
						int k=get_new_reg();
						EMIT_MOV_R_I(k, dst->imm8);
						dst->imm8=k;
						EMIT_R_R(ADD, dst->reg, next.reg);
					}
					break;
				}
				break;
			case OPERAND_REG_TEMP:
				switch(next.type)
				{
				case OPERAND_IMM8:			//dst=temp, next=imm
					EMIT_R_I(ADD, dst->reg, next.imm8);
					break;
				case OPERAND_REG_TEMP:		//dst=temp, next=temp
					EMIT_R_R(ADD, dst->reg, next.reg);
					free_register(next.reg);
					break;
				case OPERAND_REG_REFERENCE:	//dst=temp, next=ref
					EMIT_R_R(ADD, dst->reg, next.reg);
					break;
				}
				break;
			case OPERAND_REG_REFERENCE:
				dst->type=OPERAND_REG_TEMP;
				switch(next.type)
				{
				case OPERAND_IMM8:			//dst=ref, next=imm
					{
						int k=get_new_reg();
						EMIT_MOV_R_I(k, dst->reg);
						dst->imm8=k;
						EMIT_R_I(ADD, dst->reg, next.imm8);
					}
					break;
				case OPERAND_REG_TEMP:		//dst=ref, next=temp
					EMIT_R_R(ADD, next.reg, dst->reg);//SWAPPED
					dst->reg=next.reg;
					break;
				case OPERAND_REG_REFERENCE:	//dst=ref, next=ref
					{
						int k=get_new_reg();
						EMIT_MOV_R_I(k, dst->reg);
						dst->imm8=k;
						EMIT_R_R(ADD, dst->reg, next.reg);
					}
					break;
				}
				break;
			}
			break;
		case CT_MINUS://dst - next
			read_token();//skip +/-
			parse_product(&next);
			switch(dst->type)
			{
			case OPERAND_IMM8:
				switch(next.type)
				{
				case OPERAND_IMM8:			//dst=imm, next=imm
					dst->imm8-=next.imm8;
					break;
				case OPERAND_REG_TEMP:		//dst=imm, next=temp
				case OPERAND_REG_REFERENCE:	//dst=imm, next=ref
					{
						dst->type=OPERAND_REG_TEMP;
						int k=get_new_reg();
						EMIT_MOV_R_I(k, dst->imm8);
						dst->imm8=k;
						EMIT_R_R(SUB, dst->imm8, next.reg);
					}
					break;
				}
				break;
			case OPERAND_REG_TEMP:
				switch(next.type)
				{
				case OPERAND_IMM8:			//dst=temp, next=imm
					//EMIT_R_I(ADD, dst->reg, -next.imm8);
					EMIT_R_I(SUB, dst->reg, next.imm8);
					break;
				case OPERAND_REG_TEMP:		//dst=temp, next=temp
					EMIT_R_R(SUB, dst->reg, next.reg);
					free_register(next.reg);
					break;
				case OPERAND_REG_REFERENCE:	//dst=temp, next=ref
					EMIT_R_R(SUB, dst->reg, next.reg);
					break;
				}
				break;
			case OPERAND_REG_REFERENCE:
				{
					dst->type=OPERAND_REG_TEMP;
					int k=get_new_reg();
					EMIT_MOV_R_I(k, dst->reg);
					dst->imm8=k;
					switch(next.type)
					{
					case OPERAND_IMM8:			//dst=ref, next=imm
						//EMIT_R_I(ADD, dst->reg, -next.imm8);
						EMIT_R_I(SUB, dst->reg, next.imm8);
						break;
					case OPERAND_REG_TEMP:		//dst=ref, next=temp
					case OPERAND_REG_REFERENCE:	//dst=ref, next=ref
						EMIT_R_R(ADD, dst->reg, next.reg);
						break;
					}
				}
				break;
			}
			break;
		}
		free_register(next.reg);
	}
}
VarRegLib::EType *parse_temp_pvar=nullptr;
static void		parse_assignment(Operand *dst)
{
	auto identifier=current_token;
	if(identifier&&identifier->type==CT_ID)
	{
		auto assignment=peek_token(current_token);
		auto id_var=variables.find(identifier->sdata);
		if(!id_var)
			error_pp(*identifier, "Undeclared identifier.");
		else if(assignment)
		{
			if(assignment->type==CT_ASSIGN)
			{
				seek2token(assignment);
				read_token();
				Operand op;
				parse_assignment(&op);
				if(op.type==OPERAND_IMM8)
					EMIT_MOV_R_I(id_var->second.reg, op.imm8);
				else if(id_var->second.reg!=op.reg)
				{
					free_register(id_var->second.reg);
					id_var->second.reg=op.reg;
				}
				dst->type=OPERAND_REG_REFERENCE;
				dst->reg=id_var->second.reg;
				dst->id=id_var->first;
			}
			else//no assignment, parse starting from the identifier
				parse_summation(dst);
		}
	}
	else
		parse_summation(dst);
}
static void		parse_statements()
{
	do
	{
		Operand op;
		if(current_token->type==CT_INT)
		{
			read_token();//skip type
			auto id_token=current_token;
			if(id_token->type!=CT_ID)
				error_pp(*id_token, "Expected an identifier.");
			else//new variable declaration
			{
				read_token();//skip id
				if(current_token->type==CT_ASSIGN)//assignment declaration
				{
					read_token();//skip assignment operator
					parse_assignment(&op);
		
					bool old=false;
					auto &id_reg=variables.insert_no_overwrite(VarRegLib::EType(id_token->sdata, Variable()), &old);
					if(old)
						error_pp(*id_token, "Redifinition of \'%s\'.", id_token->sdata);
					else
					{
						if(op.type==OPERAND_REG_REFERENCE)
						{
							parse_temp_pvar=variables.find(op.id);
							if(!parse_temp_pvar||!parse_temp_pvar->second.initialized)
								error_pp(current_token[-1], "Uninitialized variable is used: \'%s\'", op.id);
						}
						else
							op.mov2reg();
						id_reg.second.reg=op.reg;
						id_reg.second.initialized=true;
					}
				}
				else//declaration
				{
					bool old=false;
					auto &id_reg=variables.insert_no_overwrite(VarRegLib::EType(id_token->sdata, op.reg), &old);
					if(old)
						error_pp(*id_token, "Redifinition of \'%s\'.", id_token->sdata);
					else
					{
						id_reg.second.reg=get_new_reg();
						id_reg.second.initialized=false;
					}
				}
			}
		}
		else
		{
			parse_assignment(&op);
			if(op.type!=OPERAND_REG_REFERENCE)
				free_register(op.reg);
		}

		if(current_token->type!=CT_SEMICOLON)
		{
			error_pp(*current_token, "Expected a semicolon \';\'.");
			break;
		}
	}while(read_token());//skip semicolon
}
void			parse_eval_rec(Expression const &expr)
{
	current_expr=&expr, token_idx=0;

	code_idx=0;
	code.resize(1024);
	read_token();

	reset_regstate();
	parse_statements();

	code.resize(code_idx);
}
#endif
void			dump_code(const char *filename)
{
	save_file(filename, true, code.data(), code.size());
}

void			compile(LexFile &lf)
{
	printf("\nParser\n\n");

	i64 t1=0, t2=0;

	t1=__rdtsc();
	parse_eval_rec(lf.expr);
	t2=__rdtsc();

	printf("\n\n");
	printf("parse_eval_rec:\t %20lld cycles\n", t2-t1);
	printf("binary size = %d bytes\n\n", code.size());
	
	printf("DONE.\n");
	printf("Press a key to save binary file (overwrite codegen.bin).\n");
	printf("Press X to quit.\n");

	prof.print();
	pause();

	dump_code("D:/C/ACC2/codegen.bin");
	exit(0);//
}