#include		"acc2.h"
#include		"acc2_codegen_x86_64.h"
#include		"include/intrin.h"
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

struct			MzHeader
{
	char e_magic[2];	//"MZ"
	unsigned short
		e_cblp,		//0x0090	Bytes on last page of file
		e_cp,		//0x0003	Pages in file
		e_crlc,		//0x0000	Relocations
		e_cparhdr,	//0x0004	Size of header in paragraphs
		e_minalloc,	//0x0000	Minimum extra paragraphs needed
		e_maxalloc,	//0xFFFF	Maximum extra paragraphs needed
		e_ss,		//0x0000	Initial (relative) SS value
		e_sp,		//0x00B8	Initial SP value
		e_csum,		//0x0000	Checksum
		e_ip,		//0x0000	Initial IP value
		e_cs,		//0x0000	Initial (relative) CS value
		e_lfarlc,	//0x0040	File address of relocation table
		e_ovno;		//0x0000	Overlay number
	byte e_res[8];	//{0}		Reserved
	unsigned short
		e_oemid,	//0x0000	OEM identifier (for e_oeminfo)
		e_oeminfo;	//0x0000	OEM information; e_oemid specific
	byte e_res2[20];//{0}
	unsigned e_lfanew;//0x00000080	File address of the new exe header
};
MzHeader		defaultmzheader=
{
	{'M', 'Z'},
	0x0090, 0x0003,
	0x0000, 0x0004,
	0x0000, 0xFFFF,
	0x0000, 0x00B8,
	0x0000, 0x0000,
	0x0000, 0x0040,
	0x0000, {0},
	0x0000, 0x0000,
	{0},
	0x00000080,
};
const byte		msdosprogram[]=
{
	0x0E,				//push cs
	0x1F,				//pop ds
	0xBA, 0x0E, 0x00,	//mov dx,0x0E
	0xB4, 0x09,			//mov ah,0x09
	0xCD, 0x21,			//int 0x21		//syscall ah=09h: display string
	0xB8, 0x01, 0x4C,	//mov ax,0x4C01
	0xCD, 0x21,			//int 0x21		//syscall ah=4Ch: quit

	//data
//	0x54,				//push sp			//0x54=='T'
//	0x68, 0x69, 0x73,	//push word 0x7369	//0x68=='h', 0x69=='i', 0x73=='s'...
//	0x20, 0x70, 0x72,	//and [bx+si+0x72],dh
//	0x6F,				//outsw
//	0x67, 0x72, 0x61,	//jc 0x7a
//	0x6D,				//insw
//	0x20, 0x63, 0x61,	//and [bp+di+0x61],ah
//
//	0x6E,				//outsb edx,ds:esi
//	0x6E,				//outsb edx,ds:esi
//	0x6F,				//outsb edx,ds:esi
};
const char		msdos_msg[]="This program cannot be run in DOS mode.\r\r\n$\0\0\0\0\0\0";
void			emit_msdosstub()
{
	memcpy(code.data()+code_idx, &defaultmzheader, sizeof(MzHeader));
	code_idx+=sizeof(MzHeader);
	memcpy(code.data()+code_idx, msdosprogram, sizeof(msdosprogram));
	code_idx+=sizeof(msdosprogram);
	memcpy(code.data()+code_idx, msdos_msg, sizeof(msdos_msg));
	code_idx+=sizeof(msdos_msg);
}
struct			PEHeader//wiki.osdev.org/PE
{
	char magic[4];//"PE\0\0"
	unsigned short machine, numberofsections;
	unsigned timedatestamp, pointertosymboltable, numberofsymbols;
	unsigned short sizeofoptionalheader, characteristics;
};
struct Pe32OptionalHeader// 1 byte aligned
{
	unsigned short mMagic; // 0x010b - PE32, 0x020b - PE32+ (64 bit)
	byte mMajorLinkerVersion, mMinorLinkerVersion;
	unsigned
		mSizeOfCode,
		mSizeOfInitializedData,
		mSizeOfUninitializedData,
		mAddressOfEntryPoint,
		mBaseOfCode,
		mBaseOfData,
		mImageBase,
		mSectionAlignment,
		mFileAlignment;
	unsigned short
		mMajorOperatingSystemVersion,
		mMinorOperatingSystemVersion,
		mMajorImageVersion,
		mMinorImageVersion,
		mMajorSubsystemVersion,
		mMinorSubsystemVersion;
	unsigned
		mWin32VersionValue,
		mSizeOfImage,
		mSizeOfHeaders,
		mCheckSum;
	unsigned short
		mSubsystem,
		mDllCharacteristics;
	unsigned
		mSizeOfStackReserve,
		mSizeOfStackCommit,
		mSizeOfHeapReserve,
		mSizeOfHeapCommit,
		mLoaderFlags,
		mNumberOfRvaAndSizes;
};

//Programming an x64 compiler from scratch - part 2		https://www.youtube.com/watch?v=Mx29YQ4zAuM		3:32:00 - code generation
//Programming an x64 compiler from scratch - part 3		https://www.youtube.com/watch?v=olSZ0d-nksE
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
	OPERAND_REG_TEMP='E',		//temp reg
	OPERAND_REG_REFERENCE='R',	//reference to a variable
//	OPERAND_FRAME_OFFSET,		//rbp+offset			person.age: rbp+offset+age_offset
//	OPERAND_ADDRESS,			//address
};
#if 1
unsigned		reg_state=0;//0xFF00;//true means free, false means taken
int				get_new_reg()
{
	unsigned long k=0;
	char free_registers_flag=_BitScanForward(&k, reg_state);
	assert(free_registers_flag);

	//assert2(reg_state, "Out of registers.");
	//int k=first_set_bit16(reg_state);

	reg_state&=~(1<<k);//clear bit
	return k;
}
void			reset_regstate()
{
	const char available_regs[]=
	{
		RCX,
		RBX,
		RSI,
		RDI,
		R8,
		R9,
		R10,
		R11,
		R12,
		R13,
		R14,
		R15,
	};
	reg_state=0;
	int nregs=SIZEOF(available_regs);
	for(int k=0;k<nregs;++k)
		reg_state|=1<<available_regs[k];
	//reg_state=0xFF00;
}
inline void		free_register(char &reg)
{
	if(reg>=0)
	{
		assert(!(reg_state&1<<reg));//should be cleared
		reg_state|=1<<reg;//set bit
		reg=-1;
	}
}
#else
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
#endif
struct			Operand
{
	byte type;
	//byte islvalue;
	//7 bytes padding
	union
	{
		struct
		{
			union
			{
				char reg;
				//3 bytes padding
				unsigned frame_offset;
			};
			const char *id;
		};
		i64 addr;
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
	//void mov2reg()
	//{
	//	assert2(type!=OPERAND_UNINITIALIZED, "Uninitialized operand.");
	//	if(type==OPERAND_IMM8)
	//	{
	//		int k=get_new_reg();
	//		EMIT_MOV_R_I(k, imm8);
	//		type=OPERAND_REG_TEMP;
	//		imm8=k;
	//	}
	//}
};

struct			Variable
{
	char reg;
	bool initialized;
	Variable():reg(0), initialized(false){}
	Variable(char reg):reg(reg), initialized(false){}
};
typedef std::map<const char*, Variable> VarRegLib;
static std::vector<VarRegLib> variables;//latest scope at the end
//static VarRegLib variables;
VarRegLib::EType* find_variable(const char *name)
{
	for(int ks=variables.size()-1;ks>=0;--ks)
	{
		auto pv=variables[ks].find(name);
		if(pv)
			return pv;
	}
	return nullptr;
}

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
			auto id_var=find_variable(parse_temp_token->sdata);//reference to an int
			//auto id_var=variables.find(parse_temp_token->sdata);
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
						EMIT_R_R(MOV, k, dst->reg);
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
						EMIT_R_R(MOV, k, dst->reg);
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
						EMIT_R_R(SUB, dst->reg, next.reg);
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
					EMIT_R_R(MOV, k, dst->reg);
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
	}
}
VarRegLib::EType *parse_temp_pvar=nullptr;
static void		parse_assignment(Operand *dst)
{
	auto identifier=current_token;
	if(identifier&&identifier->type==CT_ID)
	{
		auto assignment=peek_token(current_token);
		auto id_var=find_variable(identifier->sdata);
		//auto id_var=variables.find(identifier->sdata);
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
					//if(op.type==OPERAND_REG_REFERENCE)
						EMIT_R_R(MOV, id_var->second.reg, op.reg);
					//else
					//{
					//	free_register(id_var->second.reg);//invalidates references
					//	id_var->second.reg=op.reg;
					//}
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
		switch(current_token->type)
		{
		case CT_INT://declaration
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
						auto &id_reg=variables.back().insert_no_overwrite(VarRegLib::EType(id_token->sdata, Variable()), &old);
						if(old)
							error_pp(*id_token, "Redifinition of \'%s\'.", id_token->sdata);
						else if(op.type==OPERAND_REG_REFERENCE)
						{
							id_reg.second.reg=get_new_reg();
							EMIT_R_R(MOV, id_reg.second.reg, op.reg);
							parse_temp_pvar=find_variable(op.id);
							//parse_temp_pvar=variables.find(op.id);
							id_reg.second.initialized=parse_temp_pvar&&parse_temp_pvar->second.initialized;
							if(!id_reg.second.initialized)
								error_pp(current_token[-1], "Uninitialized variable is used: \'%s\'", op.id);
						}
						else
						{
							id_reg.second.initialized=true;
							if(op.type==OPERAND_IMM8)
							{
								id_reg.second.reg=get_new_reg();
								EMIT_MOV_R_I(id_reg.second.reg, op.imm8);
							}
							else
								id_reg.second.reg=op.reg;
						}
					}
					else//declaration
					{
						bool old=false;
						auto &id_reg=variables.back().insert_no_overwrite(VarRegLib::EType(id_token->sdata, op.reg), &old);
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
			break;
		//case CT_IF:
		//	break;
		//case CT_FOR:
		//	break;
		default:
			parse_assignment(&op);
			if(op.type!=OPERAND_REG_REFERENCE)
				free_register(op.reg);
			break;
		}

		if(current_token->type!=CT_SEMICOLON)
		{
			error_pp(*current_token, "Expected a semicolon \';\'.");
			break;
		}
	}while(read_token());//skip semicolon
}
static void		parse_globalstatement()
{
	switch(current_token->type)
	{
	case CT_EXTERN:
		break;
	case CT_STATIC:
	case CT_CONST:
	case CT_AUTO:
	case CT_SIGNED:
	case CT_UNSIGNED:
	case CT_VOID:
	case CT_BOOL:
	case CT_CHAR:
	case CT_SHORT:
	case CT_INT:
	case CT_LONG:
	case CT_FLOAT:
	case CT_DOUBLE:
	case CT_WCHAR_T:
	case CT_INT8:
	case CT_INT16:
	case CT_INT32:
	case CT_INT64:
	//case CT_M128:
	//case CT_M128I:
	//case CT_M128D:
		break;
	case CT_CLASS:
	case CT_STRUCT:
	case CT_UNION:
		break;
	case CT_ENUM:
		break;
	case CT_TYPEDEF:
		break;
	}
}
static void		parse_program()
{
}
/*void			parse_eval_rec(Expression const &expr)
{
	current_expr=&expr, token_idx=0;

	code_idx=0;
	code.resize(1024);
	
//	emit_msdosstub();//

	reset_regstate();
	variables.assign(1, VarRegLib());
	read_token();
	parse_statements();

	code.resize(code_idx);
}//*/

/*namespace		resources
{
	const int
		statement1[]={CT_INT, CT_ID, CT_ASSIGN, -1, CT_SEMICOLON},
		statement2[]={-1, CT_SEMICOLON};
	const int
		assignment1[]={-2},
		assignment2[]={CT_ID, CT_ASSIGN, -1};
	const int
		summation1[]={-3},
		summation2[]={-2, CT_PLUS, -3},
		summation3[]={-2, CT_MINUS, -3};
	const int
		product1[]={-4},
		product2[]={-3, CT_ASTERIX, -4},
		product3[]={-3, CT_SLASH, -4},
		product4[]={-3, CT_MODULO, -4};
	const int
		unary1[]={CT_VAL_INTEGER},
		unary2[]={CT_ID},
		unary3[]={CT_LPR, 0, CT_RPR};
}//*/

enum			StorageSpecifierType
{
	STORAGE_EXTERN,
	STORAGE_STATIC,
	STORAGE_MUTABLE,
	STORAGE_REGISTER,
};
enum			DataType
{
	DATA_AUTO,
	DATA_INT,
	DATA_FLOAT,
	DATA_SIMD,
	DATA_CLASS,
	DATA_STRUCT,
	DATA_UNION,
};
union			SpecifierInfo//cast to int		TODO: pointer info
{
	struct
	{
		int datatype:4,
			logalign:3,//in bytes
			is_const:1, is_volatile:1,
			storagetype:2;
	};
	int flags;
	SpecifierInfo():flags(0){}
	SpecifierInfo(int datatype, int logalign, int is_const, int storagetype):flags(0)
	{
		this->datatype=datatype;
		this->logalign=logalign;
		this->is_const=is_const;
		this->storagetype=storagetype;
	}
};
struct			IRNode
{
	CTokenType type;
	int parent;
	std::vector<int> children;
	//void *data;
	IRNode():type(CT_IGNORED), parent(-1){}
	IRNode(CTokenType type, int parent):type(type), parent(parent){}
	void set(CTokenType type, int parent)
	{
		this->type=type;
		this->parent=parent;
	}
	//void set(CTokenType type, int parent, void *data=nullptr)
	//{
	//	this->type=type;
	//	this->parent=parent;
	//	this->data=data;
	//}

	//CTokenType type;
	//std::vector<IRNode*> children;
	//IRNode():type(CT_IGNORED){}
	//IRNode(CTokenType type):type(type){}

	//CTokenType type;
	//int nchildren;
	//IRNode *children,
	//	*next;
	//IRNode():type(CT_IGNORED), nchildren(0), children(nullptr), next(nullptr){}
};
namespace		parse//OpenC++
{
	Expression	*current_ex=nullptr;
	std::vector<IRNode> *current_ir=nullptr;
	int			current_idx=0, ntokens=0, ir_size=0;
#define			LOOK_AHEAD(K)		current_ex->operator[](current_idx+(K)).type
	inline void	append_node(IRNode const &node)
	{
		if(ir_size<(int)current_ir->size())
			current_ir->growby(1024);
		current_ir->operator[](ir_size)=node;
		++ir_size;
	}
	inline void	append_node(CTokenType type, int parent)
	{
		if(ir_size<(int)current_ir->size())
			current_ir->growby(1000);
		current_ir->operator[](parent).children.push_back(ir_size);
		current_ir->operator[](ir_size).set(type, parent);
		++ir_size;
	}
	inline void	skip_till_after(CTokenType tokentype)
	{
		for(;current_idx<ntokens&&LOOK_AHEAD(0)!=tokentype;++current_idx);
		current_idx+=LOOK_AHEAD(0)==tokentype;
	}
	void		parse_error(const char *msg)
	{
		error_pp(current_ex->operator[](current_idx), "%s", msg);
		skip_till_after(CT_SEMICOLON);
	}

	//recursive parser declarations
	bool		r_comma_expr(int parent);
	bool		r_declarator2(int parent);
	bool		r_opt_cv_qualify(int parent);
	bool		r_opt_int_type_or_class_spec(int parent);
	bool		r_cast(int parent);
	bool		r_unary(int parent);
	bool		r_expression(int parent);
	bool		r_comma_expr(int parent);
	enum		TemplateDeclarationType
	{
		TEMPLATE_UNKNOWN,
		TEMPLATE_DECLARATION,
		TEMPLATE_INSTANTIATION, 
		TEMPLATE_SPECIALIZATION,
		TEMPLATE_TYPE_COUNT,
	};
	bool		r_template_decl2(int parent, TemplateDeclarationType &templatetype);

	//recursive parser functions
	bool		r_typespecifier(int parent)//type.specifier  :=  [cv.qualify]? (integral.type.or.class.spec | name) [cv.qualify]?
	{
		if(!r_opt_cv_qualify(parent)||!r_opt_int_type_or_class_spec(parent))
			return false;
		return true;
	}
	bool		r_typename(int parent)//type.name  :=  type.specifier cast.declarator
	{
		if(!r_typespecifier(parent))
			return false;
		if(!r_declarator2(parent))
			return false;
		return true;
	}
	bool		r_sizeof(int parent)
	{
		int t_idx=current_idx;
		if(LOOK_AHEAD(0)==CT_LPR)
		{
			if(r_typename(parent)&&LOOK_AHEAD(0)==CT_RPR)
			{
				++current_idx;
				return true;
			}
			current_idx=t_idx;//restore idx
		}
		return r_unary(parent);
	}
	bool		r_typeid(int parent)//typeid.expr  :=  TYPEID '(' expression ')'  |  TYPEID '(' type.name ')'
	{
		if(LOOK_AHEAD(0)!=CT_TYPEID)
			return false;
		++current_idx;
		if(LOOK_AHEAD(0)==CT_LPR)
		{
			int t_idx=current_idx;
			++current_idx;
			if(r_typename(parent)&&LOOK_AHEAD(0)==CT_RPR)//TODO: global 'suppresses errors'
			{
				++current_idx;
				return true;
			}
			current_idx=t_idx;
			if(r_expression(parent)&&LOOK_AHEAD(0)==CT_RPR)
			{
				++current_idx;
				return true;
			}
			current_idx=t_idx;
		}
		return false;
	}
	bool		r_func_args(int parent)//function.arguments  :  empty  |  expression (',' expression)*		assumes that the next token following function.arguments is ')'
	{
		if(LOOK_AHEAD(0)==CT_RPR)
			return true;
		for(;;)
		{
			if(!r_expression(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_COMMA)
				return true;
			++current_idx;
		}
	}
	//var.name  :=  ['::']? name2 ['::' name2]*
	//name2  :=  Identifier {template.args}  |  '~' Identifier  |  OPERATOR operator.name
	//if var.name ends with a template type, the next token must be '('
	bool		r_varname(int parent)
	{
		if(LOOK_AHEAD(0)==CT_SCOPE)
		{
			//TODO: search global scope
		}
		for(;;)
		{
			switch(LOOK_AHEAD(0))
			{
			case CT_ID:
				break;
			case CT_TILDE:
				break;
			//case CT_OPERATOR:
			//	break;
			default:
				return false;
			}
		}
	}
	//userdef.statement
	//	:	UserKeyword '(' function.arguments ')' compound.statement
	//	|	UserKeyword2 '(' arg.decl.list ')' compound.statement
	//	|	UserKeyword3 '(' expr.statement [comma.expression]? ';' [comma.expression] ')' compound.statement
	bool		r_user_definition(int parent)
	{
		if(LOOK_AHEAD(0)!=CT_ID||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		//TODO
		return true;
	}
	
	//primary.exp
	//	: Constant
	//	| CharConst
	//	| WideCharConst
	//	| StringL
	//	| WideStringL
	//	| THIS
	//	| var.name
	//	| '(' comma.expression ')'
	//	| integral.type.or.class.spec '(' function.arguments ')'
	//	| typeid.expr
	//	| openc++.primary.exp
	//
	//openc++.primary.exp := var.name '::' userdef.statement
	bool		r_primary(int parent)
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_VAL_INTEGER:
		case CT_VAL_FLOAT:
		case CT_VAL_STRING_LITERAL:
		case CT_VAL_WSTRING_LITERAL:
		case CT_VAL_CHAR_LITERAL:
			//TODO: leaf node
			++current_idx;
			break;
		case CT_THIS:
			//TODO: leaf node
			++current_idx;
			break;
		case CT_LPR:
			++current_idx;
			if(!r_comma_expr(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_RPR)
				return false;//TODO: proper error messages
			++current_idx;
			break;
		case CT_TYPEID:
			return r_typeid(parent);
		default:
			{
				int i_idx=ir_size;
				if(!r_opt_int_type_or_class_spec(parent))
					return false;
				if(i_idx>=ir_size)
					i_idx=-1;
				if(i_idx!=-1)//function style cast expression
				{
					if(LOOK_AHEAD(0)!=CT_LPR)
						return false;
					if(!r_func_args(parent))
						return false;
					if(LOOK_AHEAD(0)!=CT_RPR)
						return false;
				}
				else
				{
					if(!r_varname(parent))
						return false;
					if(LOOK_AHEAD(0)==CT_SCOPE)
					{
						if(!r_user_definition(parent))
							return false;
					}
				}
			}
			break;
		}
		return true;
	}
	//postfix.exp
	//	: primary.exp
	//	| postfix.expr '[' comma.expression ']'
	//	| postfix.expr '(' function.arguments ')'
	//	| postfix.expr '.' var.name
	//	| postfix.expr ArrowOp var.name
	//	| postfix.expr IncOp
	//	| openc++.postfix.expr
	//
	//openc++.postfix.expr
	//	: postfix.expr '.' userdef.statement
	//	| postfix.expr ArrowOp userdef.statement
	//
	//Note: function-style casts are accepted as function calls.
	bool		r_postfix(int parent)
	{
		if(!r_primary(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);;)
		{
			switch(t)
			{
			case CT_LBRACKET:
				++current_idx;
				if(!r_comma_expr(parent))
					return false;
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return false;
				++current_idx;
				break;
			case CT_LPR:
				++current_idx;
				if(!r_func_args(parent))
					return false;
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return false;
				++current_idx;
				break;
			case CT_INCREMENT:
			case CT_DECREMENT:
				//TODO: apply post
				++current_idx;
				break;
			case CT_PERIOD:
			case CT_ARROW:
				++current_idx;
				if(!r_varname(parent))
					return false;
				break;
			default:
				return true;
			}
			t=LOOK_AHEAD(0);
		}
		return true;
	}
	bool		r_unary(int parent)//unary.expr  :=  postfix.expr  |  ('*'|'&'|'+'|'-'|'!'|'~'|IncOp) cast.expr  |  sizeof.expr  |  allocate.expr  |  throw.expression
	{
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_ASTERIX:
		case CT_AMPERSAND:
		case CT_PLUS:
		case CT_MINUS:
		case CT_EXCLAMATION:
		case CT_TILDE:
		case CT_INCREMENT:
		case CT_DECREMENT:
			++current_idx;
			if(!r_cast(parent))
				return false;
			//TODO: apply the unary operator
			break;
		case CT_SIZEOF:
			return r_sizeof(parent);
		case CT_THROW:
			//TODO?
			break;
		default:
			//if is_allocate_expr: r_allocate
			return r_postfix(parent);
		}
		return true;
	}
	bool		r_cast(int parent)//cast.expr  :=  unary.expr  |  '(' type.name ')' cast.expr
	{
		if(LOOK_AHEAD(0)!=CT_LPR)
			return r_unary(parent);
		++current_idx;
		if(!r_typename(parent))
			return false;
		if(LOOK_AHEAD(0)!=CT_RPR)
			return false;
		++current_idx;
		return true;
	}
	bool		r_pm(int parent)//pm.expr (pointer to member .*, ->*)  :=  cast.expr  | pm.expr PmOp cast.expr
	{
		if(!r_cast(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_DOT_STAR||t==CT_ARROW_STAR;)
		{
			++current_idx;
			if(!r_cast(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_multiplicative(int parent)//multiply.expr  :=  pm.expr  |  multiply.expr ('*' | '/' | '%') pm.expr
	{
		if(!r_pm(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_ASTERIX||t==CT_SLASH||t==CT_MODULO;)
		{
			++current_idx;
			if(!r_pm(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_additive(int parent)//additive.expr  :=  multiply.expr  |  additive.expr ('+' | '-') multiply.expr
	{
		if(!r_multiplicative(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_PLUS||t==CT_MINUS;)
		{
			++current_idx;
			if(!r_multiplicative(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_shift(int parent)//shift.expr  :=  additive.expr  |  shift.expr ShiftOp additive.expr
	{
		if(!r_additive(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL;)
		{
			++current_idx;
			if(!r_additive(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_relational(int parent)//relational.expr  :=  shift.expr  |  relational.expr ('<=' | '>=' | '<' | '>') shift.expr
	{
		if(!r_shift(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL;)
		{
			++current_idx;
			if(!r_shift(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_equality(int parent)//equality.expr  :=  relational.expr  |  equality.expr EqualOp relational.expr
	{
		if(!r_relational(parent))
			return false;
		for(auto t=LOOK_AHEAD(0);t==CT_EQUAL||t==CT_NOT_EQUAL;)
		{
			++current_idx;
			if(!r_relational(parent))
				return false;
			//TODO: bitwise_and node
			t=LOOK_AHEAD(0);
		}
		return true;
	}
	bool		r_bitwise_and(int parent)//and.expr  :=  equality.expr  |  and.expr '&' equality.expr
	{
		if(!r_equality(parent))
			return false;
		for(;LOOK_AHEAD(0)==CT_AMPERSAND;)
		{
			++current_idx;
			if(!r_equality(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
	}
	bool		r_bitwise_xor(int parent)//inclusive.or.expr  :=  exclusive.or.expr  |  inclusive.or.expr '|' exclusive.or.expr
	{
		if(!r_bitwise_and(parent))
			return false;
		for(;LOOK_AHEAD(0)==CT_CARET;)
		{
			++current_idx;
			if(!r_bitwise_and(parent))
				return false;
			//TODO: bitwise_xor node
		}
		return true;
	}
	bool		r_bitwise_or(int parent)
	{
		if(!r_bitwise_xor(parent))
			return false;
		for(;LOOK_AHEAD(0)==CT_VBAR;)
		{
			++current_idx;
			if(!r_bitwise_xor(parent))
				return false;
			//TODO: bitwise_or node
		}
		return true;
	}
	bool		r_logic_and(int parent)//logical.and.expr  :=  inclusive.or.expr  |  logical.and.expr LogAndOp inclusive.or.expr
	{
		if(!r_bitwise_or(parent))
			return false;
		for(;LOOK_AHEAD(0)==CT_LOGIC_AND;)
		{
			++current_idx;
			if(!r_bitwise_or(parent))
				return false;
			//TODO: logic_and node
		}
		return true;
	}
	bool		r_logic_or(int parent)//logical.or.expr  :=  logical.and.expr  |  logical.or.expr LogOrOp logical.and.expr		left-to-right
	{
		if(!r_logic_and(parent))
			return false;
		for(;LOOK_AHEAD(0)==CT_LOGIC_OR;)
		{
			++current_idx;
			if(!r_logic_and(parent))
				return false;
			//TODO: logic_or node
		}
		return true;
	}
	bool		r_conditional_expr(int parent)//  conditional.expr  :=  logical.or.expr {'?' comma.expression ':' conditional.expr}  right-to-left
	{
		if(!r_logic_or(parent))
			return false;
		if(LOOK_AHEAD(0)==CT_QUESTION)
		{
			++current_idx;
			if(!r_comma_expr(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_COLON)
				return false;
			if(!r_conditional_expr(parent))
				return false;
		}
		return true;
	}
	bool		r_expression(int parent)//expression  :=  conditional.expr [(AssignOp | '=') expression]?	right-to-left
	{
		if(!r_conditional_expr(parent))
			return false;
		switch(LOOK_AHEAD(0))
		{
		case CT_ASSIGN:
		case CT_ASSIGN_ADD:case CT_ASSIGN_SUB:
		case CT_ASSIGN_MUL:case CT_ASSIGN_DIV:case CT_ASSIGN_MOD:
		case CT_ASSIGN_XOR:case CT_ASSIGN_OR:case CT_ASSIGN_AND:case CT_ASSIGN_SL:case CT_ASSIGN_SR:
			if(!r_expression(parent))
				return false;
			break;
		}
		return true;
	}
	bool		r_comma_expr(int parent)
	{
		if(!r_expression(parent))
			return false;
		while(LOOK_AHEAD(0)==CT_COMMA)
		{
			++current_idx;
			if(!r_expression(parent))
				return false;
		}
		return true;
	}
	bool		r_initialize_expr(int parent)//initialize.expr  :=  expression  |  '{' initialize.expr [',' initialize.expr]* [',']? '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return r_expression(parent);
		++current_idx;
		for(auto t=LOOK_AHEAD(0);t!=CT_RBRACE;)
		{
			if(!r_initialize_expr(parent))
			{
				skip_till_after(CT_RBRACE);
				return true;//error recovery
			}
			switch(LOOK_AHEAD(0))
			{
			case CT_RBRACE:
				break;
			case CT_COMMA:
				++current_idx;
				continue;
			default:
				skip_till_after(CT_RBRACE);
				return true;//error recovery
			}
			break;
		}
		return true;
	}

	bool		r_opt_member_spec(int parent)//member.spec := (friend|inline|virtual)+
	{
		int idx=-1;
		for(;current_idx<ntokens;++current_idx)
		{
			switch(LOOK_AHEAD(0))
			{
			case CT_FRIEND:
			case CT_INLINE:
			case CT_VIRTUAL:
				if(idx==-1)
				{
					idx=ir_size;
					append_node(CT_MEMBER_SPEC, parent);
				}
				append_node(LOOK_AHEAD(0), idx);
				//p->children.push_back(new IRNode(LOOK_AHEAD(0)));
				continue;
			default:
				break;
			}
			break;
		}
		return true;
	}
	bool		r_opt_storage_spec(int parent)//storage.spec := extern|static|mutable|register
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_EXTERN:
		case CT_STATIC:
		case CT_MUTABLE:
		case CT_REGISTER:
			append_node(LOOK_AHEAD(0), parent);
			//p->children.push_back(new IRNode(LOOK_AHEAD(0)));
			++current_idx;
			break;
		}
		return true;
	}
	bool		r_opt_cv_qualify(int parent)
	{
		int idx=-1;
		for(;current_idx<ntokens;++current_idx)
		{
			switch(LOOK_AHEAD(0))
			{
			case CT_CONST:
			case CT_VOLATILE:
				if(idx==-1)
				{
					idx=ir_size;
					append_node(CT_CV_QUALIFIER, parent);
				}
				append_node(LOOK_AHEAD(0), idx);
				continue;
			}
			break;
		}
		return true;
	}
	bool		r_opt_int_type_or_class_spec(int parent)//int.type.or.class.spec := (char|wchar_t|int|short|long|signed|unsigned|float|double|void|bool)+
	{
		int idx=-1;
		for(;;)
		{
			switch(LOOK_AHEAD(0))
			{
			case CT_AUTO:
			case CT_SIGNED:case CT_UNSIGNED:
			case CT_VOID:
			case CT_BOOL:
			case CT_CHAR:case CT_SHORT:case CT_INT:case CT_LONG:
			case CT_FLOAT:case CT_DOUBLE:
			case CT_WCHAR_T:
			case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
				if(idx==-1)
				{
					idx=ir_size;
					append_node(CT_TYPE, parent);
				}
				append_node(LOOK_AHEAD(0), idx);
				continue;
			//case CT_ID:
			//	//TODO: check if class/struct/union in this scope
			//	if(idx==-1)
			//	{
			//		idx=ir_size;
			//		append_node(CT_TYPE, parent);
			//	}
			//	append_node(LOOK_AHEAD(0), idx);
			//	continue;
			}
			break;
		}
		return true;
	}
	bool		r_compoundstatement(int parent)
	{
		return true;
	}
	bool		r_declarator2(int parent)
	{
		return true;
	}
	bool		r_declaratorwithinit(int parent)//declarator.with.init  :=  ':' expression  |  declarator {'=' initialize.expr | ':' expression}
	{
		if(LOOK_AHEAD(0)==CT_COLON)//bit field
		{
			if(!r_expression(parent))
				return false;
		}
		else
		{
			if(!r_declarator2(parent))
				return false;
			switch(LOOK_AHEAD(0))
			{
			case CT_ASSIGN:
				++current_idx;
				if(!r_initialize_expr(parent))
					return false;
				break;
			case CT_COLON:
				++current_idx;
				break;
			default:
				break;
			}
		}
		return true;
	}
	bool		r_declarators(int parent)//declarators := declarator.with.init (',' declarator.with.init)*
	{
		for(;;)
		{
			if(!r_declaratorwithinit(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
		}
		return true;
	}
	bool		r_declaration(int parent)
	{
		int decl_idx=ir_size;
		append_node(CT_IGNORED, parent);
		int member=ir_size;
		if(!r_opt_member_spec(decl_idx))
			return false;
		if(member>=ir_size)
			member=-1;
		if(!r_opt_storage_spec(decl_idx))
			return false;
		if(member==-1)
		{
			if(!r_opt_member_spec(decl_idx))
				return false;
		}
		int cv_idx=ir_size;
		r_opt_cv_qualify(decl_idx);
		if(cv_idx>=ir_size)
			cv_idx=-1;

		int opt_spec=ir_size;
		if(!r_opt_int_type_or_class_spec(decl_idx))
			return false;
		if(opt_spec>=ir_size)
			opt_spec=-1;
		if(opt_spec!=-1)
		{
			//int declaration
			int cv_idx2=ir_size;
			r_opt_cv_qualify(parent);
			if(cv_idx2>=ir_size)
				cv_idx2=-1;
			switch(LOOK_AHEAD(0))
			{
			case CT_SEMICOLON:
				break;
			case CT_COLON:
				break;
			default:
				if(!r_declarators(parent))
					return false;
				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
				{
					if(!r_compoundstatement(parent))
						return false;
				}
				break;
			}
		}
			//return r_int_decl(decl_idx);
		return true;
	}
	bool		r_typedef(int parent)//typedef.stmt  :=  TYPEDEF type.specifier declarators ';'
	{
		if(LOOK_AHEAD(0)!=CT_TYPEDEF)
			return false;
		++current_idx;
		if(!r_typespecifier(parent))
			return false;
		if(!r_declarators(parent))
			return false;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return false;
		++current_idx;
		return true;
	}

	//temp.arg.declaration
	//	:	CLASS Identifier ['=' type.name]
	//	|	type.specifier arg.declarator ['=' additive.expr]
	//	|	template.decl2 CLASS Identifier ['=' type.name]
	bool		r_template_arg_decl(int parent)
	{
		auto t=LOOK_AHEAD(0);
		if(t==CT_CLASS&&LOOK_AHEAD(1)==CT_ID)
		{
			current_idx+=2;
			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				if(!r_typename(parent))
					return false;
			}
		}
		else if(t==CT_TEMPLATE)
		{
			auto templatetype=TEMPLATE_UNKNOWN;
			if(!r_template_decl2(parent, templatetype))
				return false;
			auto t0=LOOK_AHEAD(0), t1=LOOK_AHEAD(1);
			if(t0!=CT_CLASS||t1!=CT_ID)
				return false;
			current_idx+=1+(t0==CT_CLASS);
		}
		else
		{
		}
	}
	bool		r_template_arglist(int parent)//temp.arg.list  :=  empty  |  temp.arg.declaration (',' temp.arg.declaration)*
	{
		auto t=LOOK_AHEAD(0);
		if(t==CT_GREATER||t==CT_SHIFT_RIGHT)
			return true;
		if(!r_template_arg_decl(parent))
			return false;
		while(LOOK_AHEAD(0)==CT_COMMA)
		{
			++current_idx;
			if(!r_template_arg_decl(parent))
				return false;
		}
		return true;
	}
	//template.decl
	//	:	TEMPLATE '<' temp.arg.list '>' declaration
	//	|	TEMPLATE declaration
	//	|	TEMPLATE '<' '>' declaration
	//
	//The second case is an explicit template instantiation.  declaration must be a class declaration.  For example,
	//
	//     template class Foo<int, char>;
	//
	// explicitly instantiates the template Foo with int and char.
	//
	// The third case is a specialization of a template function.  declaration must be a function template.  For example,
	//
	//     template <> int count(String x) { return x.length; }
	//
	bool		r_template_decl2(int parent, TemplateDeclarationType &templatetype)
	{
		if(LOOK_AHEAD(0)!=CT_TEMPLATE)
			return false;
		++current_idx;
		if(LOOK_AHEAD(0)!=CT_LESS)//template instantiation
		{
			templatetype=TEMPLATE_INSTANTIATION;
			return true;
		}
		++current_idx;
		int arg_idx=ir_size;
		if(!r_template_arglist(parent))
			return false;
		if(arg_idx>=ir_size)
			arg_idx=-1;
		auto &t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_LESS:
			break;
		case CT_SHIFT_RIGHT:
			t=CT_LESS;
			break;
		default:
			return false;
		}
		while(LOOK_AHEAD(0)==CT_TEMPLATE)//ignore nested template	TODO: support nested templates
		{
			++current_idx;
			if(LOOK_AHEAD(0)!=CT_LESS)
				break;
			++current_idx;
			if(!r_template_arglist(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_GREATER)
			{
				++current_idx;
				return false;
			}
		}
		if(arg_idx==-1)
			templatetype=TEMPLATE_SPECIALIZATION;//template < > declaration
		else
			templatetype=TEMPLATE_DECLARATION;//template < ... > declaration
		return true;
	}
	bool		r_template_decl(int parent)
	{
		auto &templatetoken=current_ex->operator[](current_idx);
		auto templatetype=TEMPLATE_UNKNOWN;
		if(!r_template_decl2(parent, templatetype))
			return false;
		if(!r_declaration(parent))
			return false;
		switch(templatetype)
		{
		case TEMPLATE_INSTANTIATION:
			//TODO
			break;
		case TEMPLATE_SPECIALIZATION:
		case TEMPLATE_DECLARATION:
			//TODO
			break;
		default:
			error_pp(templatetoken, "Unknown template");
			break;//recover
		}
		return true;
	}
	bool		r_definition(int parent)//OpenC++
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_SEMICOLON://null declaration
			++current_idx;
			break;
		case CT_TYPEDEF:
			return r_typedef(parent);
		case CT_TEMPLATE:
			return r_template_decl(parent);
		case CT_CLASS:
		case CT_STRUCT:
		case CT_UNION:
			break;
		case CT_EXTERN:
			switch(LOOK_AHEAD(1))
			{
			case CT_VAL_STRING_LITERAL:
				break;
			case CT_TEMPLATE:
				break;
				//variable/function declaration
			case CT_CONSTEXPR:case CT_INLINE:
			case CT_STATIC:case CT_REGISTER:
			case CT_VOLATILE:case CT_CONST:
			case CT_AUTO:
			case CT_SIGNED:case CT_UNSIGNED:
			case CT_VOID:
			case CT_BOOL:
			case CT_CHAR:case CT_SHORT:case CT_INT:case CT_LONG:
			case CT_FLOAT:case CT_DOUBLE:
			case CT_WCHAR_T:case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
				return r_declaration(parent);
			default:
				parse_error("Expected an extern declaration");
				return false;
			}
			break;
		case CT_NAMESPACE:
			break;
		case CT_USING:
			break;

			//variable/function declaration
		case CT_CONSTEXPR:
		case CT_INLINE:
		case CT_STATIC:
		case CT_REGISTER:
		case CT_VOLATILE:
		case CT_CONST:
		case CT_AUTO:
		case CT_SIGNED:
		case CT_UNSIGNED:
		case CT_VOID:
		case CT_BOOL:
		case CT_CHAR:
		case CT_SHORT:
		case CT_INT:
		case CT_LONG:
		case CT_FLOAT:
		case CT_DOUBLE:
		case CT_WCHAR_T:
		case CT_INT8:
		case CT_INT16:
		case CT_INT32:
		case CT_INT64:
			return r_declaration(parent);

		default:
			parse_error("Expected a declaration");
			return false;
		}
		return true;
	}
#undef			LOOK_AHEAD
}
void			parse_cplusplus(Expression &ex, std::vector<IRNode> &ir)//OpenC++
{
	Token nulltoken={CT_IGNORED};
	ex.insert_pod(ex.size(), nulltoken, 16);
	parse::current_ex=&ex;
	parse::ntokens=ex.size()-16;
	parse::current_idx=0;
	parse::current_ir=&ir;
	parse::ir_size=0;
	parse::current_ir->resize(1024);

	parse::append_node(CT_PROGRAM, -1);//node at 0 is always CT_PROGRAM, everything stems from there
	parse::r_definition(0);
	for(;parse::current_idx<(int)ex.size();)
	{
		for(;parse::current_idx<(int)ex.size()&&!parse::r_definition(0);)
			parse::skip_till_after(CT_SEMICOLON);
		//node=node->next;
	}
	parse::current_ir->resize(parse::ir_size);
}

void			dump_code(const char *filename)
{
	save_file(filename, true, code.data(), code.size());
}

void			compile(LexFile &lf)
{
	printf("\nParser\n\n");

	i64 t1=0, t2=0;

	t1=__rdtsc();
	current_expr=&lf.expr, token_idx=0;

	code_idx=0;
	code.resize(1024);
	
//	emit_msdosstub();//

	reset_regstate();
	variables.assign(1, VarRegLib());
	read_token();
	parse_statements();

	code.resize(code_idx);
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