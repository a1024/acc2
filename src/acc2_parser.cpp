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

enum	StorageTypeSpecifier
{
	STORAGE_EXTERN,
	STORAGE_STATIC,
	STORAGE_MUTABLE,
	STORAGE_REGISTER,
};
enum	DataType
{
	TYPE_UNASSIGNED,
	TYPE_AUTO,
	TYPE_VOID,
	TYPE_BOOL,
	TYPE_CHAR,
	TYPE_INT,
	TYPE_INT_SIGNED,
	TYPE_INT_UNSIGNED,
	TYPE_FLOAT,
	TYPE_ENUM,
	TYPE_CLASS,
	TYPE_STRUCT,
	TYPE_UNION,
	TYPE_SIMD,//special struct
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_FUNC,
	TYPE_ELLIPSIS,
	TYPE_NAMESPACE,
};
struct	TypeInfo
{
	union
	{
		struct
		{
			unsigned short
				datatype:5,//master attribute
			//	is_unsigned:1,//only for int types
				logalign:3,//in bytes
				is_const:1, is_volatile:1,
				storagetype:2;//one of: extern/static/mutable/register
			//21 bits padding
		};
		int flags;
	};
	int size;//in bytes
	//char *name;				//X  primitive types only, a type can be aliased				//names must be qualified

	//class/struct/union: args points at member types, can include padding as void of nonzero size
	//array/pointers: only one element pointing at pointed type
	//array: 'size' is the count of pointed type
	//functions: 1st element is points at return type
	std::vector<TypeInfo*> args;//TODO: template args

	TypeInfo():flags(0), size(0){}
	TypeInfo(char datatype, char logalign, char is_const, char is_volatile, char storagetype, int size):flags(0), size(size)
	{
		this->datatype=datatype;
		//this->is_unsigned=is_unsigned;
		this->logalign=logalign;
		this->is_const=is_const;
		this->is_volatile=is_volatile;
		this->storagetype=storagetype;
		//this->name=name;
	}
};
struct	TypeInfoCmp
{
	bool operator()(TypeInfo const *a, TypeInfo const *b)
	{
		if(a->datatype!=b->datatype)
			return a->datatype<b->datatype;
		if(a->size!=b->size)
			return a->size<b->size;
		if(a->args.size()!=b->args.size())
			return a->args.size()<b->args.size();
		for(int k=0;k<(int)a->args.size();++k)//functions & pointers only
			if(a->args[k]!=b->args[k])
				return a->args[k]<b->args[k];
		if(a->logalign!=b->logalign)
			return a->logalign<b->logalign;
		if(a->is_const!=b->is_const)
			return a->is_const<b->is_const;
		if(a->is_volatile!=b->is_volatile)
			return a->is_volatile<b->is_volatile;
		//if(a->storagetype!=b->storagetype)
			return a->storagetype<b->storagetype;
	}
};
typedef std::set<TypeInfo*, TypeInfoCmp> TypeDB;
TypeDB			typedb;
TypeInfo*		add_type(TypeInfo &type)
{
	auto t2=new TypeInfo(type);
	bool old=false;
	auto t3=typedb.insert_no_overwrite(t2, &old);
	if(old)
		delete t2;
	return t3;
}

struct			Scope;
typedef std::map<char*, Scope*> DeclLib;
struct			Scope
{
	TypeInfo *info;
	int level;
	DeclLib declarations;
};
//vector of declaration trees
//each element contains all immediately visible identifiers at corresponding scope
//first element is the global scope
std::vector<DeclLib> declstack;
void			scope_enter()
{
	declstack.push_back(declstack.back());//duplicate symbol table
}
void			scope_exit()
{
	declstack.pop_back();
}
Scope*			lookup(char *id)
{
	for(int k=declstack.size()-1;k>=0;--k)
	{
		auto &declarations=declstack[k];
		auto it=declarations.find(id);
		if(it)
			return it->second;
	}
	return nullptr;
}

//typedef std::vector<char*> QName;//qualified name
//struct			QNameCmp
//{
//	bool operator()(QName const &a, QName const &b)
//	{
//		int k=0;
//		for(;k<(int)a.size()&&k<(int)b.size();++k)
//			if(a[k]!=b[k])
//				return a[k]<b[k];
//		return a.size()>b.size();
//	}
//};
//std::set<QName*, QNameCmp> namedb;
struct			VarInfo
{
	TypeInfo *type;
	std::vector<char*> name;//[namespace::class::]name
	union
	{
		char *sdata;
		long long *idata;
		double *fdata;
	};
};
struct			VarInfoCmp
{
	bool operator()(VarInfo const &a, VarInfo const &b)
	{
		for(int k=0;k<(int)a.name.size()&&k<(int)b.name.size();++k)
			if(a.name[k]!=b.name[k])
				return a.name[k]<b.name[k];
		return a.name.size()>b.name.size();
	}
};
std::set<VarInfo*, VarInfoCmp> vardb;//scope?

struct			IRNode
{
	CTokenType type, opsign;
	int parent;
	std::vector<int> children;
	union
	{
		TypeInfo *tdata;//type & function
		VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long *idata;
		double *fdata;
	};
	//void *data;
	IRNode():type(CT_IGNORED), opsign(CT_IGNORED), parent(-1), tdata(nullptr){}
	IRNode(CTokenType type, int parent, void *data=nullptr):type(type), parent(parent), tdata((TypeInfo*)data){}
	void set(CTokenType type, int parent, void *data=nullptr)
	{
		this->type=type;
		this->parent=parent;
		tdata=(TypeInfo*)data;
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
namespace		parse//OpenC++		http://opencxx.sourceforge.net/
{
	Expression	*current_ex=nullptr;
	std::vector<IRNode> *current_ir=nullptr;
	int			current_idx=0, ntokens=0, ir_size=0;
#define			LOOK_AHEAD(K)		current_ex->operator[](current_idx+(K)).type
#define			NODE(K)				current_ir->operator[](K)
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
	//inline int	append_node(IRNode const &node)
	//{
	//	if(ir_size<(int)current_ir->size())
	//		current_ir->growby(1024);
	//	current_ir->operator[](ir_size)=node;
	//	int ret=ir_size;
	//	++ir_size;
	//	return ret;
	//}
	inline int	append_node(CTokenType type, int parent, IRNode *&node)
	{
		if(ir_size>=(int)current_ir->size())
			parse::current_ir->growby(1024);
		int ret=ir_size;
		++ir_size;
		node=&NODE(ret);
		node->set(type, parent);
		NODE(parent).children.push_back(ret);
		return ret;
	}
	inline int	insert_node(CTokenType type, int parent, int child, IRNode *&node)
	{
		int root=append_node(type, parent, node);
		
		NODE(parent).children.back()=root;//turn child node into grand-child
		NODE(child).parent=root;
		node->children.push_back(child);
		return root;
	}
	inline void	link_nodes(int parent, int child)
	{
		NODE(parent).children.push_back(child);
		NODE(child).parent=parent;
	}

	//TODO: differentiate syntax error from nothing returned

	//recursive parser declarations
	int			r_name(int parent);

	int			r_comma_expr(int parent);
	int			r_declarator2(int parent);
	int			r_opt_cv_qualify(int parent);
	int			r_opt_int_type_or_class_spec(int parent);
	int			r_cast(int parent);
	int			r_unary(int parent);
	int			r_assign_expr(int parent);
	int			r_comma_expr(int parent);

	int			r_expr_stmt(int parent);

	int			r_typedef(int parent);
	int			r_if(int parent);
	int			r_switch(int parent);
	int			r_while(int parent);
	int			r_do_while(int parent);
	int			r_for(int parent);
	int			r_try_catch(int parent);

	int			r_compoundstatement(int parent);
	int			r_statement(int parent);
	int			r_declarators(int parent);
	int			r_template_arglist(int parent);
	enum		TemplateDeclarationType
	{
		TEMPLATE_UNKNOWN,
		TEMPLATE_DECLARATION,
		TEMPLATE_INSTANTIATION,
		TEMPLATE_SPECIALIZATION,
		TEMPLATE_TYPE_COUNT,
	};
	int			r_template_decl2(int parent, TemplateDeclarationType &templatetype);
	int			r_declarators(int parent);

	//recursive parser functions
	int			r_typespecifier(int parent)//type.specifier  :=  [cv.qualify]? (integral.type.or.class.spec | name) [cv.qualify]?
	{
		if(!r_opt_cv_qualify(parent)||!r_opt_int_type_or_class_spec(parent))
			return false;
		return true;
	}
	int			r_typename(int parent)//type.name  :=  type.specifier cast.declarator
	{
		if(!r_typespecifier(parent))
			return false;
		if(!r_declarator2(parent))
			return false;
		return true;
	}
	int			r_sizeof(int parent)
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
	int			r_typeid(int parent)//typeid.expr  :=  TYPEID '(' expression ')'  |  TYPEID '(' type.name ')'
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
			if(r_assign_expr(parent)&&LOOK_AHEAD(0)==CT_RPR)
			{
				++current_idx;
				return true;
			}
			current_idx=t_idx;
		}
		return false;
	}
	int			r_func_args(int parent)//function.arguments  :  empty  |  expression (',' expression)*		assumes that the next token following function.arguments is ')'
	{
		if(LOOK_AHEAD(0)==CT_RPR)
			return true;
		for(;;)
		{
			if(!r_assign_expr(parent))
				return false;
			if(LOOK_AHEAD(0)!=CT_COMMA)
				return true;
			++current_idx;
		}
	}
	//var.name  :=  ['::']? name2 ['::' name2]*
	//name2  :=  Identifier {template.args}  |  '~' Identifier  |  OPERATOR operator.name
	//if var.name ends with a template type, the next token must be '('
	int			r_varname(int parent)
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
	int			r_user_definition(int parent)
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
	int			r_primary(int parent)
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
	int			r_postfix(int parent)
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
	int			r_unary(int parent)//unary.expr  :=  postfix.expr  |  ('*'|'&'|'+'|'-'|'!'|'~'|IncOp) cast.expr  |  sizeof.expr  |  allocate.expr  |  throw.expression
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
	int			r_cast(int parent)//cast.expr  :=  unary.expr  |  '(' type.name ')' cast.expr
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
	int			r_pm(int parent)//pm.expr (pointer to member .*, ->*)  :=  cast.expr  | pm.expr PmOp cast.expr
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
	int			r_multiplicative(int parent)//multiply.expr  :=  pm.expr  |  multiply.expr ('*' | '/' | '%') pm.expr
	{
		if(!r_pm(parent))
			return -1;
		for(auto t=LOOK_AHEAD(0);t==CT_ASTERIX||t==CT_SLASH||t==CT_MODULO;)
		{
			++current_idx;
			if(!r_pm(parent))
				return -1;
			//TODO: bitwise_and node
		}
		return true;
	}
	int			r_additive(int parent)//additive.expr  :=  multiply.expr  |  additive.expr ('+' | '-') multiply.expr
	{
		int child=r_multiplicative(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_PLUS||t==CT_MINUS)			//TODO: apply this change everywhere: next call, comparisons, node type
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_ADD, parent, child, node);
#if 0
			int root=ir_size;
			append_node(PT_ADD, parent);

			//turn child into grand-child
			NODE(parent).children.back()=root;
			NODE(child).parent=root;

			auto node=&NODE(root);
			node->children.push_back(child);
			//std::vector<int> ops;
			//ops.push_back(child);
#endif
			do
			{
				++current_idx;
				child=r_multiplicative(root);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				//ops.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_PLUS||t==CT_MINUS);
			return root;
		}
		return child;
	}
	int			r_shift(int parent)//shift.expr  :=  additive.expr  |  shift.expr ShiftOp additive.expr
	{
		int child=r_additive(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_SHIFT, parent, child, node);
			do
			{
				++current_idx;
				child=r_additive(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL);
			return root;
		}
		return child;
	}
	int			r_relational(int parent)//relational.expr  :=  shift.expr  |  relational.expr ('<=' | '>=' | '<' | '>') shift.expr
	{
		int child=r_shift(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_RELATIONAL, parent, child, node);
			do
			{
				++current_idx;
				child=r_shift(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL);
			return root;
		}
		return child;
#if 0
		for(auto t=LOOK_AHEAD(0);t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL;)
		{
			++current_idx;
			if(!r_shift(parent))
				return false;
			//TODO: bitwise_and node
		}
		return true;
#endif
	}
	int			r_equality(int parent)//equality.expr  :=  relational.expr  |  equality.expr EqualOp relational.expr
	{
		int child=r_relational(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_EQUAL||t==CT_NOT_EQUAL)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_EQUALITY, parent, child, node);
			do
			{
				++current_idx;
				child=r_relational(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_EQUAL||t==CT_NOT_EQUAL);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_bitwise_and(int parent)//and.expr  :=  equality.expr  |  and.expr '&' equality.expr
	{
		int child=r_equality(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_AMPERSAND)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_BITAND, parent, child, node);
			do
			{
				++current_idx;
				child=r_equality(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_AMPERSAND);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_bitwise_xor(int parent)//inclusive.or.expr  :=  exclusive.or.expr  |  inclusive.or.expr '|' exclusive.or.expr
	{
		int child=r_bitwise_and(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_CARET)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_BITXOR, parent, child, node);
			do
			{
				++current_idx;
				child=r_bitwise_and(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_CARET);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_bitwise_or(int parent)
	{
		int child=r_bitwise_xor(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_VBAR)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_BITOR, parent, child, node);
			do
			{
				++current_idx;
				child=r_bitwise_xor(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_VBAR);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_logic_and(int parent)//logical.and.expr  :=  inclusive.or.expr  |  logical.and.expr LogAndOp inclusive.or.expr
	{
		int child=r_bitwise_or(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_LOGIC_AND)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_LOGICAND, parent, child, node);
			do
			{
				++current_idx;
				child=r_bitwise_or(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_LOGIC_AND);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_logic_or(int parent)//logical.or.expr  :=  logical.and.expr  |  logical.or.expr LogOrOp logical.and.expr		left-to-right
	{
		int child=r_logic_and(parent);
		if(child==-1)
			return -1;
		auto t=LOOK_AHEAD(0);
		if(t==CT_LOGIC_OR)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_LOGICOR, parent, child, node);
			do
			{
				++current_idx;
				child=r_logic_and(parent);
				if(child==-1)
					return -1;
				NODE(child).opsign=t;
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t==CT_LOGIC_OR);
			return root;
		}
		return child;
#if 0
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
#endif
	}
	int			r_conditional_expr(int parent)//  conditional.expr  :=  logical.or.expr {'?' comma.expression ':' conditional.expr}  right-to-left
	{
		int child=r_logic_or(parent);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)==CT_QUESTION)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_CONDITIONAL, parent, child, node);

			++current_idx;
			child=r_comma_expr(parent);
			if(child==-1)
				return -1;
			node->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_COLON)
				return -1;
			child=r_conditional_expr(parent);
			if(child==-1)
				return -1;
			node->children.push_back(child);
			return root;
		}
		return child;
	}
	int			r_assign_expr(int parent)//assign_expr  :=  conditional.expr [(AssignOp | '=') assign_expr]?	right-to-left		renamed from expression
	{
		int child=r_conditional_expr(parent);
		if(child==-1)
			return -1;
		switch(LOOK_AHEAD(0))
		{
		case CT_ASSIGN:
		case CT_ASSIGN_ADD:case CT_ASSIGN_SUB:
		case CT_ASSIGN_MUL:case CT_ASSIGN_DIV:case CT_ASSIGN_MOD:
		case CT_ASSIGN_XOR:case CT_ASSIGN_OR:case CT_ASSIGN_AND:case CT_ASSIGN_SL:case CT_ASSIGN_SR:
			{
				IRNode *node=nullptr;
				int root=insert_node(PT_ASSIGN, parent, child, node);
				child=r_assign_expr(parent);
				if(child==-1)
					return -1;
				node->children.push_back(child);
				return root;
			}
		}
		return child;
	}
	int			r_comma_expr(int parent)
	{
		int child=r_assign_expr(parent);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)==CT_COMMA)
		{
			IRNode *node=nullptr;
			int root=insert_node(PT_COMMA, parent, child, node);
			do
			{
				++current_idx;
				child=r_assign_expr(parent);
				if(child==-1)
					return -1;
				node->children.push_back(child);
			}while(LOOK_AHEAD(0)==CT_COMMA);
			return root;
		}
		return child;
	}
	int			r_initialize_expr(int parent)//initialize.expr  :=  expression  |  '{' initialize.expr [',' initialize.expr]* [',']? '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return r_assign_expr(parent);
		++current_idx;
		IRNode *node=nullptr;
		int root=append_node(PT_INITIALIZER_LIST, parent, node);
		for(auto t=LOOK_AHEAD(0);t!=CT_RBRACE;t=LOOK_AHEAD(0))
		{
			int child=r_initialize_expr(root);
			if(child==-1)
			{
				skip_till_after(CT_RBRACE);
				return -1;//error recovery?
			}
			node->children.push_back(child);
			switch(LOOK_AHEAD(0))
			{
			case CT_RBRACE:
				break;
			case CT_COMMA:
				++current_idx;
				continue;
			default:
				skip_till_after(CT_RBRACE);
				return -1;//error recovery?
			}
			break;
		}
		return root;
	}

	int			r_opt_member_spec(int parent)//member.spec := (friend|inline|virtual)+
	{
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_INLINE:
		case CT_FRIEND:
		case CT_VIRTUAL:
			{
				IRNode *node=nullptr;
				int root=append_node(PT_MEMBER_SPEC, parent, node);
				node->children.push_back(t);
				++current_idx;
				for(;t=LOOK_AHEAD(0);++current_idx)
				{
					switch(t)
					{
					case CT_INLINE:
					case CT_FRIEND:
					case CT_VIRTUAL:
						for(int k=0;k<(int)node->children.size();++k)
						{
							if(t<node->children[k])
							{
								node->children.insert_pod(k, t);
								break;
							}
							if(t==node->children[k])
								break;//TODO: warning: repeated member specifier
						}
						continue;
					default:
						break;
					}
					break;
				}
				return root;
			}
		}
		return -1;
#if 0
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
					append_node(PT_MEMBER_SPEC, parent);
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
#endif
	}
	int			r_opt_storage_spec(int parent)//storage.spec := extern|static|mutable|register
	{
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_EXTERN:
		case CT_STATIC:
		case CT_MUTABLE:
		case CT_REGISTER:
			{
				IRNode *node=nullptr;
				int root=append_node(PT_MEMBER_SPEC, parent, node);
				node->opsign=t;
				++current_idx;
				return root;
			}
		}
		return -1;
#if 0
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
#endif
	}
	int			r_opt_cv_qualify(int parent)//cv.qualify  :=  (const|volatile)+
	{
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_CONST:
		case CT_VOLATILE:
			{
				IRNode *node=nullptr;
				int root=append_node(PT_MEMBER_SPEC, parent, node);
				(int&)node->opsign|=1<<int(t==CT_VOLATILE);
				++current_idx;
				for(;t=LOOK_AHEAD(0);++current_idx)
				{
					switch(t)
					{
					case CT_CONST:
					case CT_VOLATILE:
						(int&)node->opsign|=1<<int(t==CT_VOLATILE);
						continue;
					default:
						break;
					}
					break;
				}
				return root;
			}
		}
		return -1;
#if 0
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
#endif
	}
	int			r_opt_int_type_or_class_spec(int parent)//int.type.or.class.spec := (char|wchar_t|int|short|long|signed|unsigned|float|double|void|bool)+
	{
		int root=-1;
		IRNode *node=nullptr;
		TypeInfo type;
		Scope *scope;
		short long_count=0, int_count=0;
		for(;;)
		{
			auto token=&current_ex->operator[](current_idx);
			switch(token->type)					//TODO: correct type spec order
			{
			case CT_AUTO:
			case CT_SIGNED:case CT_UNSIGNED:
			case CT_VOID:
			case CT_BOOL:
			case CT_CHAR:case CT_SHORT:case CT_INT:case CT_LONG:
			case CT_FLOAT:case CT_DOUBLE:
			case CT_WCHAR_T:
			case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
			case CT_ID:
				switch(token->type)
				{
				case CT_AUTO:
					if(type.datatype==TYPE_UNASSIGNED)
						type.datatype=TYPE_AUTO;
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_SIGNED:
					if(type.datatype==TYPE_UNASSIGNED||type.datatype==TYPE_INT)
					{
						type.datatype=TYPE_INT_SIGNED;
						type.size=4;
					}
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_UNSIGNED:
					if(type.datatype==TYPE_UNASSIGNED||type.datatype==TYPE_INT)
					{
						type.datatype=TYPE_INT_UNSIGNED;
						type.size=4;
					}
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_VOID:
					if(type.datatype==TYPE_UNASSIGNED)
						type.datatype=TYPE_VOID;
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_BOOL:
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_BOOL;
						type.size=1;
					}
					break;
				case CT_CHAR:
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.datatype=TYPE_CHAR;//TODO: reject 'unsigned int char'
						type.size=1;
						break;
					default:
						//TODO: error: invalid type spec combination
						break;
					}
					break;
				case CT_SHORT:
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
						type.datatype=TYPE_INT;
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.size=2;
						break;
					default:
						//TODO: error: invalid type spec combination
						break;
					}
					break;
				case CT_INT:
					++int_count;
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
						type.datatype=TYPE_INT;
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.size=4;
						break;
					default:
						//TODO: error: invalid type spec combination
						break;
					}
					break;
				case CT_LONG:
					++long_count;
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
						type.datatype=TYPE_INT;
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						switch(type.size)
						{
						case 0:
							type.size=4;
							break;
						case 4:
							type.size=8;
							break;
						case 8:
							//TODO: error: invalid type spec combination
							break;
						}
						break;
					case TYPE_FLOAT:
						if(type.size==8)
							type.size=16;
						else
						{
							//TODO: error: invalid type spec combination
						}
						break;
					default:
						//TODO: error: invalid type spec combination
						break;
					}
					break;
				case CT_FLOAT:
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=CT_FLOAT;
						type.size=4;
					}
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_DOUBLE://TODO: differentiate long double (ok) & int double (error)
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=CT_FLOAT;
						type.size=8;
					}
					else if(type.datatype==TYPE_INT&&long_count==1&&!int_count)
					{
						type.datatype=CT_FLOAT;
						type.size=16;
					}
					else
					{
						//TODO: error: invalid type spec combination
					}
					break;
				case CT_WCHAR_T:
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_CHAR;
						type.size=4;
					}
					break;
				case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_CHAR;
						switch(token->type)
						{
						case CT_INT8: type.size=1;break;
						case CT_INT16:type.size=2;break;
						case CT_INT32:type.size=4;break;
						case CT_INT64:type.size=8;break;
						}
					}
					break;
				case CT_ID:
					scope=lookup(token->sdata);//check if identifier==class/struct/union in this/global scope
					if(!scope)
					{
						//error: unexpected identifier
						return -1;
					}
					switch(scope->info->datatype)
					{
					case TYPE_ENUM:
					case TYPE_CLASS:
					case TYPE_STRUCT:
					case TYPE_UNION:
					case TYPE_SIMD://special struct
						{
							if(type.datatype!=TYPE_UNASSIGNED)
								return -1;
							//merge with type			//TODO: unroll the struct to typedb at declaration
							auto typeinfo=scope->info;
							type.datatype=typeinfo->datatype;
							if(type.logalign<typeinfo->logalign)
								type.logalign=typeinfo->logalign;
							type.size=typeinfo->size;
						}
						break;
					default:
						//error: identifier is not an enum/class/struct/union
						return -1;
					}
					break;
				}
				if(root==-1)
					root=append_node(PT_TYPE, parent, node);
				//append_node(t, root);
				continue;
			}
			break;
		}
		node->tdata=add_type(type);
		return root;
	}

	int			r_simple_declaration(int parent)//condition controlling expression of switch/while/if
	{
		int c_cv=-1, c_ic=-1, c_name=-1;
		if((c_cv=r_opt_cv_qualify(parent))==-1||(c_ic=r_opt_int_type_or_class_spec(parent))==-1)
			return -1;
		if((c_name=r_name(parent))==-1)
			return -1;

		int c_decl=r_declarator2(parent);
		if(c_decl==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_ASSIGN)//TODO: make sense of this
			return -1;
		++current_idx;

		int c_expr=r_assign_expr(parent);
		if(c_expr==-1)
			return -1;

		return -1;//X  always fails
	}
	int			r_condition(int parent)//condition  :=  simple.declaration  |  comma.expresion
	{
		int t_idx=current_idx, i_idx=ir_size;
		int child=r_simple_declaration(parent);
		if(child!=-1)
			return child;
		current_idx=t_idx, ir_size=i_idx;//restore state
		return r_comma_expr(parent);
	}

	int			r_typedef(int parent)//typedef.stmt  :=  TYPEDEF type.specifier declarators ';'
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
	int			r_if(int parent)//if.statement  :=  IF '(' condition ')' statement { ELSE statement }
	{
		if(LOOK_AHEAD(0)!=CT_IF||LOOK_AHEAD(1)!=CT_LPR)
			return -1;
		current_idx+=2;

		IRNode *node=nullptr;
		int root=append_node(CT_IF, parent, node);

		int child=r_condition(root);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_RPR)
			return -1;
		link_nodes(root, child);

		child=r_statement(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		if(LOOK_AHEAD(0)==CT_ELSE)
		{
			++current_idx;
			child=r_statement(root);
			if(child==-1)
				return -1;
			link_nodes(root, child);
		}
		return root;
	}
	int			r_switch(int parent)//switch.statement  :=  SWITCH '(' condition ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_SWITCH||LOOK_AHEAD(1)!=CT_LPR)
			return -1;
		current_idx+=2;

		IRNode *node=nullptr;
		int root=append_node(CT_SWITCH, parent, node);

		int child=r_condition(root);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_RPR)
			return -1;
		link_nodes(root, child);

		child=r_statement(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		return root;
	}
	int			r_while(int parent)//while.statement  :=  WHILE '(' condition ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_WHILE||LOOK_AHEAD(1)!=CT_LPR)
			return -1;
		current_idx+=2;

		IRNode *node=nullptr;
		int root=append_node(CT_WHILE, parent, node);

		int child=r_condition(root);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_RPR)
			return -1;
		link_nodes(root, child);

		child=r_statement(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		return root;
	}
	int			r_do_while(int parent)//do.statement  :=  DO statement WHILE '(' comma.expression ')' ';'
	{
		if(LOOK_AHEAD(0)!=CT_DO)
			return -1;
		++current_idx;

		IRNode *node=nullptr;
		int root=append_node(CT_DO, parent, node);

		int child=r_statement(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);
		
		if(LOOK_AHEAD(0)!=CT_WHILE||LOOK_AHEAD(1)!=CT_LPR)
			return -1;
		current_idx+=2;

		child=r_comma_expr(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);
		
		if(LOOK_AHEAD(0)!=CT_RPR||LOOK_AHEAD(1)!=CT_SEMICOLON)
			return -1;
		current_idx+=2;

		return root;
	}
	int			r_for(int parent)//for.statement  :=  FOR '(' expr.statement {comma.expression} ';' {comma.expression} ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_FOR||LOOK_AHEAD(1)!=CT_LPR)
			return -1;
		current_idx+=2;

		IRNode *node=nullptr;
		int root=append_node(CT_FOR, parent, node);

		int child=r_expr_stmt(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);
		if(LOOK_AHEAD(0)==CT_SEMICOLON)
		{
			++current_idx;
			IRNode *n2=nullptr;
			child=append_node(PT_COMMA, root, n2);
		}
		else
		{
			child=r_comma_expr(root);
			if(child==-1)
				return -1;
			link_nodes(root, child);
		}
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return -1;
		
		if(LOOK_AHEAD(0)==CT_RPR)
		{
			++current_idx;
			IRNode *n2=nullptr;
			child=append_node(PT_COMMA, root, n2);
		}
		else
		{
			child=r_comma_expr(root);
			if(child==-1)
				return -1;
			link_nodes(root, child);
		}
		if(LOOK_AHEAD(0)!=CT_RPR)
			return -1;

		child=r_statement(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		return root;
	}
	int			r_arg_declaration(int parent)//arg.declaration  :=  {userdef.keyword | REGISTER} type.specifier arg.declarator {'=' initialize.expr}
	{
		if(LOOK_AHEAD(0)==CT_REGISTER)
		{
			++current_idx;
			//ignore register
		}

		IRNode *node=nullptr;
		int root=append_node(CT_FOR, parent, node);

		int child=r_typespecifier(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		child=r_declarator2(root);
		if(child==-1)
			return -1;
		link_nodes(root, child);

		if(LOOK_AHEAD(0)==CT_ASSIGN)
		{
			++current_idx;
			child=r_initialize_expr(root);
			if(child==-1)
				return -1;
			link_nodes(root, child);
		}

		return root;
	}
//try.statement  :=  TRY compound.statement (exception.handler)+ ';'
//
//exception.handler  :=  CATCH '(' (arg.declaration | Ellipsis) ')' compound.statement
	int			r_try_catch(int parent)
	{
		if(LOOK_AHEAD(0)!=CT_TRY)
			return -1;
		++current_idx;

		IRNode *node=nullptr;
		int root=append_node(CT_FOR, parent, node);

		int child=r_compoundstatement(root);
		if(child==-1)
			return -1;

		do
		{
			if(LOOK_AHEAD(0)!=CT_CATCH||LOOK_AHEAD(1)!=CT_LPR)
				return -1;
			current_idx+=2;

			if(LOOK_AHEAD(0)==CT_ELLIPSIS)
			{
				++current_idx;
				IRNode *n2=nullptr;
				child=append_node(CT_ELLIPSIS, parent, n2);
			}
			else
			{
				child=r_arg_declaration(root);
			}
		}while(LOOK_AHEAD(0)==CT_CATCH);

		return root;
	}

	int			r_ptr_to_member(int parent)
	{
		int root=-1;
		if(LOOK_AHEAD(0)==CT_SCOPE)
		{
			++current_idx;
			//TODO: search only global scope
		}
		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t!=CT_ID)
				return -1;
			++current_idx;
			t=LOOK_AHEAD(0);
			if(t==CT_LESS)
			{
				int child=r_template_arglist(parent);
				if(!child)
					return -1;
				//TODO: template call lookup
			}
			else
			{
				//TODO: simple name lookup
			}
			t=LOOK_AHEAD(0);
			if(t!=CT_SCOPE)
				return -1;
			++current_idx;
			t=LOOK_AHEAD(0);
			if(t==CT_ASTERIX)
			{
				++current_idx;
				break;
			}
		}
		return root;
	}
	bool		is_ptr_to_member(int i)//ptr.to.member  :=  {'::'} (identifier {'<' any* '>'} '::')+ '*'
	{
		auto t0=LOOK_AHEAD(i++);
		if(t0==CT_SCOPE)
			t0=LOOK_AHEAD(i++);
		while(t0==CT_ID)
		{
			t0=LOOK_AHEAD(i++);
			if(t0==CT_LESS)
			{
				int n=1;
				while(n>0)
				{
					auto t1=LOOK_AHEAD(i++);
					switch(t1)
					{
					case CT_LESS:
						++n;
						break;
					case CT_GREATER:
						--n;
						break;
					case CT_SHIFT_RIGHT:
						n-=2;
						break;
					case CT_LPR:
						{
							int m=1;
							while(m>0)
							{
								auto t2=LOOK_AHEAD(i++);
								switch(t2)
								{
								case CT_LPR:
									++m;
									break;
								case CT_RPR:
									--m;
									break;
								case CT_IGNORED:case CT_SEMICOLON:case CT_RBRACE:
									return false;//TODO: syntax error
								}
							}
						}
						break;
					}
				}
				t0=LOOK_AHEAD(i++);
			}//if '<'
			if(t0!=CT_SCOPE)
				return false;
			t0=LOOK_AHEAD(i++);
			if(t0==CT_ASTERIX)
				return true;
		}
		return false;
	}
	int			r_ptr_operator(int parent)//ptr.operator  :=  (('*' | '&' | ptr.to.member) {cv.qualify})+
	{
		int root=-1;
		IRNode *node=nullptr;
		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t!=CT_ASTERIX&&t!=CT_AMPERSAND&&!is_ptr_to_member(0))
				break;
			if(t==CT_ASTERIX||t==CT_AMPERSAND)
			{
				++current_idx;
				append_node(t, parent, node);
			}
			else
			{
				int child=r_ptr_to_member(parent);
				if(child==-1)
					return -1;
			}
		}
		return root;
	}
	int			r_cast_operator_name(int parent)
	{
		int c_cv=r_opt_cv_qualify(parent);
		if(c_cv==-1)
			return -1;
		int child=r_opt_int_type_or_class_spec(parent);
		if(child==-1)
		{
			child=r_name(parent);
			if(child==-1)
				return -1;
		}
		int c_cv2=r_opt_cv_qualify(parent);
		int c_ptr=r_ptr_operator(parent);
		if(c_ptr==-1)
			return -1;
		return -1;//TODO: complete this
	}
	int			r_operator_name(int parent)
	{
		auto t=LOOK_AHEAD(0);
		IRNode *node=nullptr;
		switch(t)
		{
		case CT_ARROW:
		case CT_INCREMENT:case CT_DECREMENT:
		case CT_TILDE:case CT_EXCLAMATION:

		case CT_ARROW_STAR:

		case CT_ASTERIX:case CT_SLASH:case CT_MODULO:
		case CT_PLUS:case CT_MINUS:
		case CT_SHIFT_LEFT:case CT_SHIFT_RIGHT:
		case CT_THREEWAY:
		case CT_LESS:case CT_LESS_EQUAL:case CT_GREATER:case CT_GREATER_EQUAL:
		case CT_EQUAL:case CT_NOT_EQUAL://!= is deprecated?
		case CT_AMPERSAND:case CT_CARET:case CT_VBAR:
		case CT_LOGIC_AND:case CT_LOGIC_OR:

		case CT_ASSIGN:
		case CT_ASSIGN_MUL:case CT_ASSIGN_DIV:case CT_ASSIGN_MOD:
		case CT_ASSIGN_ADD:case CT_ASSIGN_SUB:
		case CT_ASSIGN_SL:case CT_ASSIGN_SR:
		case CT_ASSIGN_AND:case CT_ASSIGN_XOR:case CT_ASSIGN_OR:

		case CT_COMMA:
			++current_idx;
			return append_node(t, parent, node);

		case CT_NEW:case CT_DELETE:
			if(LOOK_AHEAD(1)==CT_LBRACKET&&LOOK_AHEAD(2)==CT_RBRACKET)
			{
				current_idx+=3;
				int child=append_node(t, parent, node);
				node->opsign=CT_LBRACKET;
				return child;
			}
			++current_idx;
			return append_node(t, parent, node);

		case CT_LPR:
			if(LOOK_AHEAD(1)==CT_RPR)
			{
				current_idx+=2;
				return append_node(t, parent, node);
			}
			break;
		case CT_LBRACKET:
			if(LOOK_AHEAD(1)==CT_RBRACKET)
			{
				current_idx+=2;
				return append_node(t, parent, node);
			}
			break;

		case CT_VAL_STRING_LITERAL://user-defined literal (since C++11)
			if(!strlen(current_ex->operator[](current_idx).sdata))//TODO: compare pointers instead
			{
				++current_idx;
				return append_node(t, parent, node);
			}
			break;
		}
		return r_cast_operator_name(parent);
	}
//name : {'::'} name2 ('::' name2)*
//
//name2
//  : Identifier {template.args}
//  | '~' Identifier
//  | OPERATOR operator.name {template.args}
	int			r_name(int parent)
	{
		auto t=LOOK_AHEAD(0);
		if(t==CT_SCOPE)
		{
			++current_idx;
			//TODO: search global scope only
		}
		else if(t==CT_DECLTYPE)//TODO: search all nested scopes upwards
		{
			++current_idx;
			if(LOOK_AHEAD(0)!=CT_LPR)
				return -1;
			int child=r_name(parent);
			if(child==-1)
				return -1;
			if(LOOK_AHEAD(0)!=CT_RPR)
				return -1;
			return child;
		}
		for(;;)
		{
			t=LOOK_AHEAD(0);
			++current_idx;
			switch(t)
			{
			case CT_TEMPLATE:
				t=LOOK_AHEAD(0);
				++current_idx;
				break;
			case CT_ID:
				t=LOOK_AHEAD(0);
				if(t==CT_LESS)
				{
					int child=r_template_arglist(parent);
					if(child==-1)
						return -1;
					//TODO: template call
					t=LOOK_AHEAD(0);
				}
				else
				{
					//TODO: simple name
				}
				if(t==CT_SCOPE)
				{
					++current_idx;
				}
				else
				{
					return -1;//TODO: return name is now qualified
				}
				break;
			case CT_TILDE:
				if(LOOK_AHEAD(0)!=CT_ID)
					return -1;
				//TODO: destructor
				return -1;//TODO: return name is now qualified
			case CT_OPERATOR:
				{
					int child=r_operator_name(parent);
					if(child==-1)
						return -1;
					t=LOOK_AHEAD(0);
					if(t==CT_LESS)
					{
						int child=r_template_arglist(parent);
						if(child==-1)
							return -1;
					}
					else
					{
					}
					//TODO: complete this
					return -1;//true
				}
				break;
			default:
				return -1;
			}
		}
		return -1;
	}
	int			r_int_decl_stmt(int parent)//integral.decl.statement  :=  decl.head integral.type.or.class.spec {cv.qualify} {declarators} ';'
	{
		int c_cv=r_opt_cv_qualify(parent);
		if(LOOK_AHEAD(0)==CT_SEMICOLON)
		{
			++current_idx;
			return -1;//TODO: debug this
		}
		int child=r_declarators(parent);
		if(child==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return -1;
		++current_idx;
		return -1;//TODO: debug this
	}
	int			r_const_decl(int parent)
	{
		int c_decl=r_declarators(parent);//TODO: decl these variables as const
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return -1;						//TODO: differentiate between syntax error and no node return
		++current_idx;
		return -1;//sic
	}
	int			r_other_decl(int parent)//other.decl.statement  :=  decl.head name {cv.qualify} declarators ';'
	{
		int c_name=r_name(parent);
		if(c_name==-1)
			return -1;
		int c_cv=r_opt_cv_qualify(parent);
		int c_decl=r_declarators(parent);
		if(c_decl==-1)
			return -1;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return -1;
		return -1;//sic
		//return parent;//never return parent
	}
//declaration.statement
//  : decl.head integral.type.or.class.spec {cv.qualify} {declarators} ';'
//  | decl.head name {cv.qualify} declarators ';'
//  | const.declaration
//
//decl.head
//  : {storage.spec} {cv.qualify}
//
//const.declaration
//  : cv.qualify {'*'} Identifier '=' expression {',' declarators} ';'
	int			r_decl_stmt(int parent)
	{
		int c_st=-1, c_cv, c_ic=-1;
		if((c_st=r_opt_storage_spec(parent))==-1||(c_cv=r_opt_cv_qualify(parent))==-1||(c_ic=r_opt_int_type_or_class_spec(parent))==-1)
			return -1;
		IRNode *node=nullptr;
		int root=insert_node(PT_VAR_DECL, parent, c_st!=-1?c_st : c_cv!=-1?c_cv:c_ic, node);
		int child=-1;
		if(c_ic!=-1)
		{
			child=r_int_decl_stmt(root);
			if(child==-1)
				return -1;
			link_nodes(root, child);
			return root;
		}
		auto t=LOOK_AHEAD(0);
		bool assign=false;
		switch(LOOK_AHEAD(1))
		{
		case CT_ASSIGN:
		case CT_ASSIGN_MUL:case CT_ASSIGN_DIV:case CT_ASSIGN_MOD:
		case CT_ASSIGN_ADD:case CT_ASSIGN_SUB:
		case CT_ASSIGN_SL:case CT_ASSIGN_SR:
		case CT_ASSIGN_AND:case CT_ASSIGN_XOR:case CT_ASSIGN_OR:
			assign=true;
		}
		if(c_cv!=-1&&(t==CT_ID&&assign||t==CT_ASTERIX))
			child=r_const_decl(root);
		else
			child=r_other_decl(root);
		return root;
	}
//expr.statement
//  : ';'
//  | declaration.statement
//  | comma.expression ';'
//  | openc++.postfix.expr
//  | openc++.primary.exp
	int			r_expr_stmt(int parent)
	{
		if(LOOK_AHEAD(0)==CT_SEMICOLON)
			return -1;
		int t_idx=current_idx, ir_idx=ir_size;//save parser state
		int root=r_decl_stmt(parent);
		if(root!=-1)
			return root;
		current_idx=t_idx, ir_size=ir_idx;//reload parser state
		root=r_comma_expr(parent);
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return -1;
		return root;
	}
//statement
//  : compound.statement
//  | typedef.stmt
//  | if.statement
//  | switch.statement
//  | while.statement
//  | do.statement
//  | for.statement
//  | try.statement
//  | BREAK ';'
//  | CONTINUE ';'
//  | RETURN { comma.expression } ';'
//  | GOTO Identifier ';'
//  | CASE expression ':' statement
//  | DEFAULT ':' statement
//  | Identifier ':' statement
//  | expr.statement
	int			r_statement(int parent)
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_LBRACE:
			return r_compoundstatement(parent);
		case CT_TYPEDEF:
			return r_typedef(parent);
		case CT_IF:
			return r_if(parent);
		case CT_SWITCH:
			return r_switch(parent);
		case CT_WHILE:
			return r_while(parent);
		case CT_DO:
			return r_do_while(parent);
		case CT_FOR:
			return r_for(parent);
		case CT_TRY:
			return r_try_catch(parent);
		case CT_BREAK:
		case CT_CONTINUE:
			{
				IRNode *node=nullptr;
				return append_node(LOOK_AHEAD(0), parent, node);
			}
		case CT_RETURN:
			{
				++current_idx;
				IRNode *node=nullptr;
				int root=append_node(CT_RETURN, parent, node);
				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
					r_comma_expr(root);
				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
					return -1;//TODO: error: expected a semicolon		TODO: clean parent from inserted children
				++current_idx;
				return root;
			}
			break;
		case CT_GOTO:
			{
				++current_idx;
				if(LOOK_AHEAD(0)!=CT_ID||LOOK_AHEAD(1)!=CT_SEMICOLON)
					return -1;
				IRNode *node=nullptr;
				int root=append_node(CT_GOTO, parent, node);
				node->sdata=current_ex->operator[](current_idx).sdata;//TODO: maintain labels inside this function
				current_idx+=2;
			}
			break;
		case CT_CASE:
			{
				++current_idx;
				IRNode *node=nullptr;
				int root=append_node(CT_CASE, parent, node);
				int child=r_assign_expr(root);
				if(LOOK_AHEAD(0)!=CT_COLON)
				{
					//TODO: error: expected a colon
					return -1;
				}
				++current_idx;
				link_nodes(root, child);//TODO: use link_nodes() instead of IRNode::children.push_back()
				return root;
			}
		case CT_DEFAULT:
			{
				++current_idx;
				if(LOOK_AHEAD(0)!=CT_COLON)
				{
					//TODO: error: expected a colon
					return -1;
				}
				IRNode *node=nullptr;
				int root=append_node(CT_DEFAULT, parent, node);
			}
			break;
		case CT_ID://label for goto
			if(LOOK_AHEAD(1)==CT_COLON)
			{
				IRNode *node=nullptr;
				int root=append_node(PT_JUMPLABEL, parent, node);
				node->sdata=current_ex->operator[](current_idx).sdata;
				current_idx+=2;
				return root;
				//return r_statement(parent);//non-standard: statement not required after label
			}
			//no break
		default:
			return r_expr_stmt(parent);
		}
		return -1;
	}
	int			r_compoundstatement(int parent)//compound.statement  :=  '{' (statement)* '}'
	{
		auto t=LOOK_AHEAD(0);
		if(t!=CT_LBRACE)
			return -1;
		++current_idx;
		t=LOOK_AHEAD(0);
		if(t!=CT_RBRACE)
		{
			IRNode *node=nullptr;
			int root=append_node(PT_CODE_BLOCK, parent, node);
			do
			{
				int child=r_statement(parent);
				if(child==-1)
				{
					skip_till_after(CT_RBRACE);//TODO: error: expected a statement
					return -1;
				}
				node->children.push_back(child);
				t=LOOK_AHEAD(0);
			}while(t!=CT_RBRACE);
			return root;
		}
		return -1;
	}
	int			r_declarator2(int parent)
	{
		return -1;
	}
	int			r_declaratorwithinit(int parent)//declarator.with.init  :=  ':' expression  |  declarator {'=' initialize.expr | ':' expression}
	{
		if(LOOK_AHEAD(0)==CT_COLON)//bit field
		{
			if(!r_assign_expr(parent))
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
	int			r_declarators(int parent)//declarators := declarator.with.init (',' declarator.with.init)*
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
	int			r_declaration(int parent)
	{
		int decl_idx=ir_size;
		IRNode *node=nullptr;
		append_node(CT_IGNORED, parent, node);
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
			//return r_opt_int_type_or_class_spec(decl_idx);
		return true;
	}

	//temp.arg.declaration
	//	:	CLASS Identifier ['=' type.name]
	//	|	type.specifier arg.declarator ['=' additive.expr]
	//	|	template.decl2 CLASS Identifier ['=' type.name]
	int			r_template_arg_decl(int parent)
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
		return true;
	}
	int			r_template_arglist(int parent)//temp.arg.list  :=  empty  |  temp.arg.declaration (',' temp.arg.declaration)*
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
	int			r_template_decl2(int parent, TemplateDeclarationType &templatetype)
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
	int			r_template_decl(int parent)
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
	int			r_definition(int parent)//OpenC++
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

	IRNode *node=nullptr;
	parse::append_node(PT_PROGRAM, -1, node);//node at 0 is always CT_PROGRAM, everything stems from there
	parse::r_definition(0);
	for(;parse::current_idx<(int)ex.size();)
	{
		for(;parse::current_idx<(int)ex.size()&&!parse::r_definition(0);)
			parse::skip_till_after(CT_SEMICOLON);
	}
	parse::current_ir->resize(parse::ir_size);
}

void			dump_code(const char *filename)
{
	save_file(filename, true, code.data(), code.size());
}

struct			DummyNode
{
	CTokenType type, opsign;
	DummyNode *parent, *child, *next;
	union
	{
		TypeInfo *tdata;//type & function
		VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long *idata;
		double *fdata;
	};
	DummyNode():type(CT_IGNORED), opsign(CT_IGNORED), parent(nullptr), child(nullptr), next(nullptr), tdata(nullptr){}
};
void			benchmark()
{
	const int testsize=1024*1024;
	
	prof.add("bm start");
	{
		std::vector<IRNode> ir;
		parse::current_ir=&ir;
		parse::ir_size=0;
		parse::current_ir->resize(1024);
		prof.add("prepare");

		IRNode *node=nullptr;
		for(int k=0;k<testsize;++k)
			parse::append_node(CT_IGNORED, k-1, node);
		prof.add("build");
	}
	prof.add("destroy");

	{
		auto tree=new DummyNode, node=tree;
		for(int k=0;k<testsize;++k)
		{
			node->next=new DummyNode;
			node=node->next;
		}
		prof.add("build");

		node=tree;
		auto n2=node->next;
		for(int k=0;k<testsize;++k)
		{
			delete node;
			node=n2, n2=node->next;
		}
		prof.add("destroy");
	}
	prof.print();
	_getch();
	exit(0);
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