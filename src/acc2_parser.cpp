#include		"acc2.h"
#include		"include/tree.hpp"
#include		"acc2_codegen_x86_64.h"
#include		"include/intrin.h"

	#define		DEBUG_PARSER

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
	STORAGE_UNSPECIFIED,
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
	TypeInfo(TypeInfo const &other):flags(other.flags), size(other.size), args(other.args){}
	TypeInfo(TypeInfo &&other):flags(other.flags), size(other.size), args((std::vector<TypeInfo*>&&)other.args){}
	TypeInfo& operator=(TypeInfo const &other)
	{
		if(this!=&other)
		{
			flags=other.flags;
			size=other.size;
			args=other.args;
		}
		return *this;
	}
	TypeInfo& operator=(TypeInfo &&other)
	{
		if(this!=&other)
		{
			flags=other.flags, other.flags=0;
			size=other.size, other.size=0;
			args=(std::vector<TypeInfo*>&&)other.args;
		}
		return *this;
	}
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

enum			NameType
{
	NAME_NAMESPACE,
	NAME_STRUCT,//or class
	NAME_UNION,
	NAME_ENUM,
	NAME_TEMPLATE_STRUCT,
	NAME_TEMPLATE_UNION,
	NAME_FUNCTION,
	NAME_TEMPLATE_FUNCTION,
	NAME_VARIABLE,
};
struct			NameInfo
{
	NameType type;
	TypeInfo *tdata;
	std::vector<CTokenType> *template_args;
	//std::vector<CTokenType> template_args;
};
typedef std::maptree<NameInfo, char*> Name;
Name			scope_global;
char			*scope_id_lbrace=nullptr;
void			scope_init()
{
	scope_id_lbrace=add_string("{");
}
Name::Node*		scope_enter(char *name, bool *old=nullptr)
{
	return scope_global.open(name);
}
void			scope_exit()
{
	if(scope_global.path.size()<=0)
	{
		Token t={};
		error_pp(t, "Excess closing brace(s)");//TODO: better error reporting
		return;
	}
	if(*scope_global.get_topmost_key()==scope_id_lbrace)
		scope_global.close_and_erase();
	else
		scope_global.close();
}
void			scope_declare_member(char *name, NameInfo const &s)
{
	scope_global.insert(name, s, true);
}
Name::Node*		scope_lookup(char *id, bool global)
{
	if(global)
		return scope_global.find_root(id);
	return scope_global.find(id);
}
#if 0
std::vector<Name::Container::EType> scope_path;
//const int LOL_1=sizeof(Name);
//typedef std::map<char*, Name> GlobalScope;
//GlobalScope global_scope;
//struct			Name
//{
//	NameType type;
//	std::map<char*, Name*> decl;
//};
char			*scope_id_lbrace=nullptr;
void			scope_init()
{
	scope_id_lbrace=add_string("{");
	scope_path.push_back(Name::Container::EType(scope_id_lbrace, &scope_global));
}
Name::Container::EType scope_enter(char *name, bool *old=nullptr)
{
	assert(scope_path.size()>0);
	auto &current=scope_path.back();
	bool oldflag=false;
	auto ret=current.second->insert_no_overwrite(name, &oldflag);
	//scope_path.push_back(ret
	if(old)
		*old=oldflag;
	return ret;
}
void			scope_exit()
{
	if(scope_path.size()<=1)
	{
		Token t={};//TODO: more detail
		error_pp(t, "Extra closing brace(s)");
		return;
	}
	auto current=scope_path.back();
	if(current.first==scope_id_lbrace)
	{
	}
	scope_path.pop_back();
	current=scope_path.back();
	current=
}
void			scope_declare_member(char *name, NameInfo const &s)
{
}
Name*			scope_lookup(char *id, bool global)
{
	return nullptr;//
}
#endif
#if 0
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

enum			NameType
{
	NAME_NAMESPACE,
	NAME_DATATYPE,
	NAME_VARIABLE,
	NAME_FUNCTION,
};
struct			Name;
typedef std::set<Name*> NameSet;//memory leak fix
typedef std::map<char*, Name*> DeclLib;
struct			Name
{
	unsigned short type, level;
	union
	{
		TypeInfo *tdata;
		VarInfo *vdata;
	};
	DeclLib declarations;
};
NameSet nameset;
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
	if(!declstack.size())
	{
		//TODO: error: end of global scope???
		return;
	}
	declstack.pop_back();
}
void			scope_declare_member(char *name, Name *s)
{
	nameset.insert(s);
	declstack.back().insert(DeclLib::EType(name, s));
}
Name*			decl_lookup_global(char *id)
{
	auto it=declstack[0].find(id);
	if(it)
		return it->second;
	return nullptr;
}
Name*			decl_lookup_local(char *id)
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
Name*			scope_lookup(char *id, bool global)
{
	if(global)
		return decl_lookup_global(id);
	return decl_lookup_local(id);
}
#endif

//struct		IRNode;
//union			IRLink
//{
//	IRNode *p;
//	int i;
//	IRLink(IRNode *p):p(p){}
//	IRLink(int i):i(i){}
//};
struct			IRNode
{
	CTokenType type;
	union
	{
		struct
		{
			unsigned int
				flag_esmr:2,//storage specifier := extern|static|mutable|register
				flag_const:1, flag_volatile:1,//cv qualifier
				flag_friend:1, flag_inline:1, flag_virtual:1;//member specifier
		};
		int flags;
		CTokenType opsign;
	};
	//std::vector<IRLink> children;//X  makes free_tree() more complicated
	std::vector<IRNode*> children;
	union
	{
		TypeInfo *tdata;//type & function
	//	VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long idata;
		double fdata;
		//long long *idata;
		//double *fdata;
	};
	IRNode():type(CT_IGNORED), opsign(CT_IGNORED), tdata(nullptr){}
	IRNode(CTokenType type, CTokenType opsign, void *data=nullptr):type(type), opsign(opsign), tdata((TypeInfo*)data){}
	IRNode(CTokenType type, int opsign, void *data=nullptr):type(type), opsign((CTokenType)opsign), tdata((TypeInfo*)data){}
	//IRNode(CTokenType type, IRNode *c1, IRNode *c2):type(type), opsign(CT_IGNORED), tdata(nullptr)
	//{
	//	children.push_back(c1);
	//	children.push_back(c2);
	//}
	void set(CTokenType type, CTokenType opsign, void *data=nullptr)
	{
		this->type=type;
		this->opsign=opsign;
		tdata=(TypeInfo*)data;
	}
	void set(void *data)
	{
		idata=0;
		sdata=(char*)data;
	}
};
inline void		token2str(CTokenType t, std::string &str)
{
	switch(t)
	{
#define		TOKEN(STR, LABEL, FLAG)		case LABEL:str+=#LABEL;break;
#include	"acc2_keywords_c.h"
#undef		TOKEN
	}
}
void			AST2str(IRNode *root, std::string &str, int depth=0)
{
	sprintf_s(g_buf, g_buf_size, "%5d", depth);
	str+=g_buf;
	str+=' ';
	for(int k=0;k<depth;++k)
		str+='|';
	str+=' ';
	if(!root)
	{
		str+="<nullptr>\n";
		return;
	}
	token2str(root->type, str);
	if(root->opsign!=CT_IGNORED)
	{
		str+='\t';
		token2str(root->opsign, str);
	}
	switch(root->type)
	{
	case CT_ID:
	case CT_CLASS:case CT_STRUCT:case CT_UNION:
	case CT_GOTO:case PT_JUMPLABEL:
	case PT_TEMPLATE_ARG:
	case CT_NAMESPACE:case PT_NAMESPACE_ALIAS:
		str+='\t';
		str+=root->sdata;
		break;
	case CT_VAL_STRING_LITERAL: case CT_VAL_WSTRING_LITERAL:
	case CT_EXTERN:
		str+="\t\"";
		str+=root->sdata;
		str+='\"';
		break;
	case CT_VAL_CHAR_LITERAL:
	case CT_VAL_INTEGER:
		str+='\t';
		sprintf_s(g_buf, g_buf_size, "%d", root->idata);
		str+=g_buf;
		break;
	case CT_VAL_FLOAT:
		str+='\t';
		sprintf_s(g_buf, g_buf_size, "%g", root->fdata);
		str+=g_buf;
		break;
	case PT_TYPE:
		{
			auto type=root->tdata;
			str+='\t';
			switch(type->datatype)
			{
			case TYPE_UNASSIGNED:	str+="<unassigned>";break;//TODO: elaborate
			case TYPE_AUTO:			str+="auto";break;
			case TYPE_VOID:			str+="void";break;
			case TYPE_BOOL:			str+="bool";break;
			case TYPE_CHAR:			str+="char";break;
			case TYPE_INT:			str+="int";break;
			case TYPE_INT_SIGNED:	str+="signed int";break;
			case TYPE_INT_UNSIGNED:	str+="unsigned";break;
			case TYPE_FLOAT:		str+="float";break;
			case TYPE_ENUM:			str+="enum";break;
			case TYPE_CLASS:		str+="class";break;
			case TYPE_STRUCT:		str+="struct";break;
			case TYPE_UNION:		str+="union";break;
			case TYPE_SIMD:			str+="simd";break;
			case TYPE_ARRAY:		str+="array";break;
			case TYPE_POINTER:		str+="pointer";break;
			case TYPE_FUNC:			str+="func";break;
			case TYPE_ELLIPSIS:		str+="...";break;
			case TYPE_NAMESPACE:	str+="namespace";break;
				break;
			}
		}
		break;
	}
	str+='\n';
	for(int k=0;k<(int)root->children.size();++k)
		AST2str(root->children[k], str, depth+1);
}
void			debugprint(IRNode *root)
{
	std::string str;
	AST2str(root, str);
	printf("%s\n\n", str.c_str());
}
namespace		parse//OpenC++		http://opencxx.sourceforge.net/
{
	Expression	*current_ex=nullptr;
	int			current_idx=0, ntokens=0, ir_size=0;
#define			LOOK_AHEAD(K)		current_ex->operator[](current_idx+(K)).type
#define			LOOK_AHEAD_TOKEN(K)	current_ex->operator[](current_idx+(K))
#define			ADVANCE				++current_idx
#define			ADVANCE_BY(K)		current_idx+=K
#define			GET_TOKEN			current_ex->operator[](current_idx++).type
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
	bool		free_tree(IRNode *&root)
	{
		if(root)
		{
			for(int k=0;k<(int)root->children.size();++k)
				free_tree(root->children[k]);
			delete root;
			root=nullptr;
		}
		return false;
	}
#define			INSERT_NONLEAF(ROOT, TEMP, TYPE)		TEMP=ROOT, ROOT=new IRNode(TYPE, CT_IGNORED), ROOT->children.push_back(TEMP);
#define			INSERT_LEAF(ROOT, TYPE, DATA)			ROOT=new IRNode(TYPE, CT_IGNORED, DATA)
#define			INSERT_CHILD(ROOT, CHILDNUM, NEXT)		ROOT->children.push_back(nullptr); if(!NEXT(ROOT->children[CHILDNUM]))return free_tree(ROOT);
	//inline void	insert_nonleaf(IRNode *&root, IRNode *&temp, CTokenType type)
	//{
	//	temp=root;
	//	root=new IRNode(type, CT_IGNORED);
	//	root->children.push_back(temp);
	//}
	//inline int	append_node(IRNode const &node)
	//{
	//	if(ir_size<(int)current_ir->size())
	//		current_ir->growby(1024);
	//	current_ir->operator[](ir_size)=node;
	//	int ret=ir_size;
	//	++ir_size;
	//	return ret;
	//}
	//inline int	append_node(CTokenType type, int parent, IRNode *&node)
	//{
	//	if(ir_size>=(int)current_ir->size())
	//		parse::current_ir->growby(1024);
	//	int ret=ir_size;
	//	++ir_size;
	//	node=&NODE(ret);
	//	node->set(type, parent);
	//	NODE(parent).children.push_back(ret);
	//	return ret;
	//}
	//inline int	insert_node(CTokenType type, int parent, int child, IRNode *&node)
	//{
	//	int root=append_node(type, parent, node);
	//	
	//	NODE(parent).children.back()=root;//turn child node into grand-child
	//	NODE(child).parent=root;
	//	node->children.push_back(child);
	//	return root;
	//}
	//inline void	link_nodes(int parent, int child)
	//{
	//	NODE(parent).children.push_back(child);
	//	NODE(child).parent=parent;
	//}

	//TODO: CHECK FOR BOUNDS

	//void		t_extract_func_args_type(IRNode *root)
	//{
	//}

	//recursive parser declarations
	bool		is_ptr_to_member(int i);
	bool		r_ptr_to_member(IRNode *&root);

	bool		r_name(IRNode *&root);

	enum		DeclaratorKind
	{
		DECLKIND_NORMAL,
		DECLKIND_ARG,
		DECLKIND_CAST,
	};
	bool		r_declarator2(IRNode *&root, int kind, bool recursive, bool should_be_declarator, bool is_statement);
	bool		r_opt_cv_qualify(int &cv_flag);//{bit 1: volatile, bit 0: const}
	bool		r_opt_int_type_or_class_spec(IRNode *&root);
	bool		r_cast(IRNode *&root);
	bool		r_unary(IRNode *&root);
	bool		r_assign_expr(IRNode *&root);
	bool		r_comma_expr(IRNode *&root);

	bool		r_expr_stmt(IRNode *&root);

	bool		r_typedef(IRNode *&root);
	bool		r_if(IRNode *&root);
	bool		r_switch(IRNode *&root);
	bool		r_while(IRNode *&root);
	bool		r_do_while(IRNode *&root);
	bool		r_for(IRNode *&root);
	bool		r_try_catch(IRNode *&root);

	bool		r_logic_or(IRNode *&root);
	bool		r_operator_name(IRNode *&root);

	bool		r_compoundstatement(IRNode *&root);
	bool		r_statement(IRNode *&root);
	bool		r_initialize_expr(IRNode *&root);
	bool		r_template_arglist(IRNode *&root);
	enum		TemplateDeclarationType
	{
		TEMPLATE_UNKNOWN,
		TEMPLATE_DECLARATION,
		TEMPLATE_INSTANTIATION,
		TEMPLATE_SPECIALIZATION,
		TEMPLATE_TYPE_COUNT,
	};
	bool		r_template_decl2(IRNode *&root, TemplateDeclarationType &templatetype);
	bool		r_template_decl(IRNode *&root);
	bool		r_declarators_nonnull(IRNode *&root, bool should_be_declarator, bool is_statement);
	bool		r_using(IRNode *&root);
	bool		r_metaclass_decl(IRNode *&root);
	bool		r_definition(IRNode *&root);
	bool		r_declaration(IRNode *&root);

	//recursive parser functions
	bool		is_template_args()//template.args  :=  '<' any* '>' ('(' | '::')
	{
		int i=0;
		if(LOOK_AHEAD(i++)==CT_LESS)
		{
			int chevron=1;
			while(chevron>0)
			{
				switch(LOOK_AHEAD(i++))
				{
				case CT_LESS:
					++chevron;
					break;
				case CT_GREATER:
					--chevron;
					break;
				case CT_SHIFT_RIGHT:
					chevron-=2;
					break;
				case CT_LPR:
					{
						int paren=1;
						while(paren>0)
						{
							switch(LOOK_AHEAD(i++))
							{
							case CT_LPR:
								++paren;
								break;
							case CT_RPR:
								--paren;
								break;
							case CT_IGNORED:case CT_SEMICOLON:case CT_RBRACE:
								return false;
							}
						}
					}
					break;
				case CT_IGNORED:case CT_SEMICOLON:case CT_RBRACE:
					return false;
				}
			}
			switch(LOOK_AHEAD(0))
			{
			case CT_SCOPE:
			case CT_LPR:
				return true;
			}
		}
		return false;
	}
	bool		moreVarName()
	{
		if(LOOK_AHEAD(0)==CT_SCOPE)
		{
			auto t=LOOK_AHEAD(1);
			return t==CT_ID||t==CT_TILDE||t==CT_OPERATOR;
		}
		return false;
	}
	bool		maybe_typename_or_classtemplate()
	{
		return true;
	}

	bool		r_typespecifier(IRNode *&root, bool check)//type.specifier  :=  [cv.qualify]? (integral.type.or.class.spec | name) [cv.qualify]?
	{
		TypeInfo type={};
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return false;
		if(!r_opt_int_type_or_class_spec(root))
			return false;
		if(!root)
		{
			if(check&&!maybe_typename_or_classtemplate())
				return false;
			if(!r_name(root))
				return false;
			if(!r_opt_cv_qualify(cv_flag))
				return false;
			//TODO: combine cv_flag with id
			return true;
		}
		if(!r_opt_cv_qualify(cv_flag))
			return false;
		type=*root->tdata;
		type.is_const=cv_flag&1;
		type.is_volatile=cv_flag>>1;
		root->tdata=add_type(type);
		return true;
#if 0
		INSERT_LEAF(root, PT_TYPESPEC, nullptr);
		root->children.assign_pod(3, nullptr);

		if(!r_opt_cv_qualify(root->children[0])||!r_opt_int_type_or_class_spec(root->children[1]))
			return free_tree(root);
		if(!root->children[1])
		{
			if(check)
			{
				if(!maybe_typename_or_classtemplate())
					return free_tree(root);
			}
			if(!r_name(root->children[1]))
				return free_tree(root);
		}
		if(!r_opt_cv_qualify(root->children[2]))
			return free_tree(root);
		return true;
#endif
	}
	bool		r_typename(IRNode *&root)//type.name  :=  type.specifier cast.declarator
	{
		if(!r_typespecifier(root, true))
			return false;
		if(!r_declarator2(root, DECLKIND_CAST, false, false, false))
			return false;
		return true;
	}
//sizeof.expr
//  : SIZEOF unary.expr
//  | SIZEOF '(' type.name ')'
	bool		r_sizeof(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_SIZEOF)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_SIZEOF, nullptr);

		int t_idx=current_idx;
		if(LOOK_AHEAD(0)==CT_LPR)
		{
			root->children.push_back(nullptr);
			if(r_typename(root->children[0])&&LOOK_AHEAD(0)==CT_RPR)
			{
				ADVANCE;
				return true;
			}
			free_tree(root->children[0]);
			current_idx=t_idx;//restore idx
		}
		return r_unary(root->children[0])||free_tree(root);
	}
	bool		r_typeid(IRNode *&root)//typeid.expr  :=  TYPEID '(' expression ')'  |  TYPEID '(' type.name ')'
	{
		if(LOOK_AHEAD(0)!=CT_TYPEID)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_TYPEID, nullptr);

		if(LOOK_AHEAD(0)==CT_LPR)
		{
			int t_idx=current_idx;
			ADVANCE;

			root->children.push_back(nullptr);
			if(r_typename(root->children[0])&&LOOK_AHEAD(0)==CT_RPR)
			{
				ADVANCE;
				return true;
			}
			free_tree(root->children[0]);
			current_idx=t_idx;
			if(r_assign_expr(root->children[0])&&LOOK_AHEAD(0)==CT_RPR)
			{
				ADVANCE;
				return true;
			}
			free_tree(root->children[0]);
			current_idx=t_idx;
		}
		return free_tree(root);
	}
	bool		r_func_args(IRNode *&root)//function.arguments  :  empty  |  expression (',' expression)*		assumes that the next token following function.arguments is ')'
	{
		if(LOOK_AHEAD(0)==CT_RPR)
		{
			//INSERT_LEAF(root, CT_VOID, nullptr);
			return true;//TODO: check child for nullptr
		}
		INSERT_LEAF(root, PT_FUNC_ARGS, nullptr);
		for(;;)
		{
			IRNode *child=nullptr;
			if(!r_assign_expr(child))
				return free_tree(root);
			root->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}

//template.args
//  : '<' '>'
//  | '<' template.argument {',' template.argument} '>'
	bool		r_template_args(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_LESS)
			return false;
		ADVANCE;

		INSERT_LEAF(root, PT_TEMPLATE_ARGS, nullptr);

		switch(LOOK_AHEAD(0))
		{
		case CT_GREATER:
		case CT_SHIFT_RIGHT:
			ADVANCE;
			return true;
		}
		for(;;)
		{
			root->children.push_back(nullptr);
			if(!r_typename(root->children.back()))
			{
				if(!r_logic_or(root->children.back()))
					return free_tree(root);
			}
			switch(LOOK_AHEAD(0))
			{
			case CT_GREATER:
				ADVANCE;
				return true;
			case CT_SHIFT_RIGHT:
				ADVANCE;//TODO: return 2 for 2 templates were closed
				return true;
				ADVANCE;
			case CT_COMMA:
				continue;
			default:
				return free_tree(root);
			}
		}
		return true;
	}
	//var.name  :=  ['::']? name2 ['::' name2]*
	//name2  :=  Identifier {template.args}  |  '~' Identifier  |  OPERATOR operator.name
	//if var.name ends with a template type, the next token must be '('
	bool		r_varname(IRNode *&root)
	{
		bool global=LOOK_AHEAD(0)==CT_SCOPE;
		ADVANCE_BY((int)global);
		INSERT_LEAF(root, PT_VAR_REF, nullptr);
		for(;;)
		{
			auto t=&LOOK_AHEAD_TOKEN(0);
			switch(t->type)
			{
			case CT_ID:
				ADVANCE;
				root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));
				if(is_template_args())
				{
					root->children.push_back(nullptr);
					if(!r_template_arglist(root->children.back()))
						return free_tree(root);
				}
				if(moreVarName())
					ADVANCE;
				else
				{
					//TODO: lookup variable reference
					return true;
				}
				break;
			case CT_TILDE:
				ADVANCE;
				t=&LOOK_AHEAD_TOKEN(0);
				if(t->type!=CT_ID)
					return free_tree(root);
				//TODO: lookup class name
				root->children.push_back(new IRNode(PT_DESTRUCTOR, CT_IGNORED));
				ADVANCE;
				return true;
			case CT_OPERATOR:
				root->children.push_back(new IRNode(CT_OPERATOR, CT_IGNORED));
				root->children.back()->children.push_back(nullptr);
				if(!r_operator_name(root->children.back()->children.back()))
					return free_tree(root);
				return true;
			default:
				return free_tree(root);
			}
		}
		return true;
	}
	//userdef.statement
	//	:	UserKeyword '(' function.arguments ')' compound.statement
	//	|	UserKeyword2 '(' arg.decl.list ')' compound.statement
	//	|	UserKeyword3 '(' expr.statement [comma.expression]? ';' [comma.expression] ')' compound.statement
	bool		r_user_definition(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_ID||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		//TODO
		return true;
	}
	bool		r_throw_expr(IRNode *&root)//throw.expression  :=  THROW {expression}
	{
		if(LOOK_AHEAD(0)!=CT_THROW)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_THROW, nullptr);

		switch(LOOK_AHEAD(0))
		{
		case CT_COLON:case CT_SEMICOLON:
			break;
		default:
			root->children.push_back(nullptr);
			if(!r_assign_expr(root->children[0]))
				return free_tree(root);
			break;
		}
		return true;
	}
	bool		is_type_specifier()//returns true if the next is probably a type specifier
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_ID:
		case CT_SCOPE:
		case CT_CONST:
		case CT_VOLATILE:
		case CT_VOID:case CT_BOOL:
		case CT_CHAR:case CT_WCHAR_T:
		case CT_SHORT:case CT_INT:case CT_LONG:
		case CT_SIGNED:case CT_UNSIGNED:
		case CT_FLOAT:case CT_DOUBLE:
		case CT_CLASS:case CT_STRUCT:case CT_UNION:case CT_ENUM:
			return true;
		}
		return false;
	}
	bool		r_allocate_initializer(IRNode *&root)//allocate.initializer  :=  '(' {initialize.expr (',' initialize.expr)* } ')'
	{
		if(LOOK_AHEAD(0)!=CT_LPR)
			return false;
		ADVANCE;
		if(LOOK_AHEAD(0)==CT_RPR)
		{
			//INSERT_LEAF(root, PT_INITIALIZER_LIST, nullptr);
			return true;//TODO: check child for nullptr
		}
		INSERT_LEAF(root, PT_INITIALIZER_LIST, nullptr);
		for(;;)
		{
			IRNode *child=nullptr;
			if(!r_initialize_expr(child))
				return free_tree(root);
			root->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);
		ADVANCE;
		return true;
	}
	bool		r_opt_ptr_operator(IRNode *&root)//ptr.operator  :=  (('*' | '&' | ptr.to.member) {cv.qualify})+
	{
		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t!=CT_ASTERIX&&t!=CT_AMPERSAND&&!is_ptr_to_member(0))
				break;
			if(!root)
				INSERT_LEAF(root, PT_PTR_OPERATOR, nullptr);
			if(t==CT_ASTERIX||t==CT_AMPERSAND)
			{
				ADVANCE;
				root->children.push_back(new IRNode(t, CT_IGNORED));
			}
			else
			{
				root->children.push_back(nullptr);
				if(!r_ptr_to_member(root->children.back()))
					return free_tree(root);
			}
			int cv_flag=0;
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);
			(int&)root->children.back()->opsign=cv_flag;//TODO: encode the pointer operator
		}
		return true;
	}
//new.declarator
//  : empty
//  | ptr.operator
//  | {ptr.operator} ('[' comma.expression ']')+
	bool		r_new_declarator(IRNode *&root)
	{
		INSERT_LEAF(root, PT_NEW_DECLARATOR, nullptr);
		root->children.push_back(nullptr);
		if(!r_opt_ptr_operator(root->children[0]))
			return free_tree(root);
		while(LOOK_AHEAD(0)==CT_LBRACKET)
		{
			ADVANCE;//skip '['
			IRNode *child=nullptr;
			if(!r_comma_expr(child))
				return free_tree(root);
			root->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_RBRACKET)
				return free_tree(root);
			ADVANCE;//skip ']'
		}
		return true;
	}
//allocate.type
//  : {'(' function.arguments ')'} type.specifier new.declarator {allocate.initializer}
//  | {'(' function.arguments ')'} '(' type.name ')' {allocate.initializer}
	bool		r_allocate_type(IRNode *&root)//can be inlined
	{
		IRNode *child=nullptr;
		INSERT_LEAF(root, PT_ALLOCATE_TYPE, nullptr);
		if(LOOK_AHEAD(0)==CT_LPR)
		{
			ADVANCE;
			int idx=current_idx;//save
			if(r_typename(root))
			{
				if(GET_TOKEN==CT_RPR)
				{
					if(LOOK_AHEAD(0)!=CT_LPR)
					{
						if(!is_type_specifier())
							return true;
					}
					else if(r_allocate_initializer(child))
					{
						root->children.push_back(child);
						if(LOOK_AHEAD(0)!=CT_LPR)
							return true;
					}
				}
			}
			current_idx=idx;//restore
			if(!r_func_args(child))
				return free_tree(root);
			root->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
		}

		if(LOOK_AHEAD(0)==CT_LPR)
		{
			ADVANCE;
			if(!r_typename(child))
				return free_tree(root);
			root->children.push_back(child);
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
		}
		else
		{
			if(!r_typespecifier(child, false))
				return free_tree(root);
			root->children.push_back(child);
			if(!r_new_declarator(child))
				return free_tree(root);
			root->children.push_back(child);
		}

		if(LOOK_AHEAD(0)==CT_LPR)
		{
			if(!r_allocate_initializer(child))
				return free_tree(root);
			root->children.push_back(child);
		}
		return true;
	}
//allocate.expr
//  : {Scope | userdef.keyword} NEW allocate.type
//  | {Scope} DELETE {'[' ']'} cast.expr
	bool		r_allocate_expr(IRNode *&root)
	{
		auto t=LOOK_AHEAD(0);
		bool global=t==CT_SCOPE;
		if(global)
		{
			ADVANCE;
			//TODO: use global new/delete
			t=LOOK_AHEAD(0);
		}

		if(t==CT_DELETE)
		{
			ADVANCE;
			INSERT_LEAF(root, CT_DELETE, nullptr);
			if(LOOK_AHEAD(0)==CT_LBRACKET)
			{
				ADVANCE;
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				root->opsign=CT_LBRACKET;
			}
			root->children.push_back(nullptr);
			if(!r_cast(root->children[0]))
				return free_tree(root);
			return true;
		}
		else if(t==CT_NEW)
		{
			ADVANCE;
			INSERT_LEAF(root, CT_NEW, nullptr);
			root->children.push_back(nullptr);
			return r_allocate_type(root->children[0])||free_tree(root);
		}

		return false;
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
	bool		r_primary(IRNode *&root)
	{
		auto t=&LOOK_AHEAD_TOKEN(0);
		switch(t->type)
		{
		case CT_VAL_INTEGER:
		case CT_VAL_FLOAT:
		case CT_VAL_STRING_LITERAL:
		case CT_VAL_WSTRING_LITERAL:
		case CT_VAL_CHAR_LITERAL:
			ADVANCE;
			INSERT_LEAF(root, t->type, nullptr);
			root->idata=t->idata;
			break;
		case CT_NULLPTR:
		case CT_THIS:
			ADVANCE;
			INSERT_LEAF(root, t->type, nullptr);
			break;
		case CT_LPR:
			ADVANCE;
			if(!r_comma_expr(root))
				return false;
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);//TODO: proper error messages
			ADVANCE;
			break;
		case CT_TYPEID:
			return r_typeid(root);
		default:
			{
				INSERT_LEAF(root, PT_VAR_DECL, nullptr);
				root->children.push_back(nullptr);
				if(!r_opt_int_type_or_class_spec(root->children[0]))
					return free_tree(root);
				if(root->children[0])//function style cast expression
				{
					if(LOOK_AHEAD(0)!=CT_LPR)
						return free_tree(root);
					root->children.push_back(nullptr);
					if(!r_func_args(root->children[1]))
						return free_tree(root);
					if(LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
				}
				else
				{
					if(!r_varname(root->children[0]))
						return free_tree(root);
					if(LOOK_AHEAD(0)==CT_SCOPE)
					{
						root->children.push_back(nullptr);
						if(!r_user_definition(root->children[1]))
							return free_tree(root);
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
	bool		r_postfix(IRNode *&root)
	{
		IRNode *child=nullptr;
		if(!r_primary(child))
			return false;
		for(auto t=LOOK_AHEAD(0);;t=LOOK_AHEAD(0))
		{
			switch(t)
			{
			case CT_LBRACKET:
				ADVANCE;
				INSERT_LEAF(root, CT_LBRACKET, nullptr);
				root->children.push_back(child);
				root->children.push_back(nullptr);
				if(!r_comma_expr(root->children[1]))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE;
				break;
			case CT_LPR:
				ADVANCE;
				INSERT_LEAF(root, PT_FUNC_CALL, nullptr);
				root->children.push_back(child);
				root->children.push_back(nullptr);
				if(!r_func_args(root->children[1]))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE;
				break;
			case CT_INCREMENT:
				ADVANCE;
				INSERT_LEAF(root, PT_POST_INCREMENT, nullptr);
				root->children.push_back(child);
				break;
			case CT_DECREMENT:
				ADVANCE;
				INSERT_LEAF(root, PT_POST_DECREMENT, nullptr);
				root->children.push_back(child);
				break;
			case CT_PERIOD:
			case CT_ARROW:
				ADVANCE;
				INSERT_LEAF(root, t, nullptr);
				root->children.push_back(child);
				root->children.push_back(nullptr);
				if(!r_varname(root->children[1]))
					return free_tree(root);
				break;
			default:
				root=child;
				return true;
			}
		}
		return true;
	}
	bool		r_unary(IRNode *&root)//unary.expr  :=  postfix.expr  |  ('*'|'&'|'+'|'-'|'!'|'~'|IncOp) cast.expr  |  sizeof.expr  |  allocate.expr  |  throw.expression
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
			{
				ADVANCE;
				INSERT_LEAF(root, t, nullptr);
				root->children.push_back(nullptr);
				if(!r_cast(root->children[0]))
					return free_tree(root);
			}
			break;
		case CT_SIZEOF:
			return r_sizeof(root);
		case CT_THROW:
			return r_throw_expr(root);
			break;
		default:
			if(t==CT_SCOPE)
				t=LOOK_AHEAD(1);
			if(t==CT_NEW||t==CT_DELETE)
				return r_allocate_expr(root);
			else
				return r_postfix(root);
		}
		return true;
	}
	bool		r_cast(IRNode *&root)//cast.expr  :=  unary.expr  |  '(' type.name ')' cast.expr
	{
		IRNode **node=&root;
		for(;;)
		{
			if(LOOK_AHEAD(0)!=CT_LPR)
				return r_unary(*node);
			ADVANCE;
			INSERT_LEAF(*node, PT_CAST, nullptr);
			(*node)->children.push_back(nullptr);
			if(!r_typename((*node)->children.back()))
				return free_tree(*node);
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(*node);
			ADVANCE;
			node=&(*node)->children.back();
		}
		return true;
	}
	bool		r_pm(IRNode *&root)//pm.expr (pointer to member .*, ->*)  :=  cast.expr  | pm.expr PmOp cast.expr
	{
		if(!r_cast(root))
			return free_tree(root);
		auto t=LOOK_AHEAD(0);
		if(t==CT_DOT_STAR||t==CT_ARROW_STAR)
		{
			IRNode *node=nullptr;
			INSERT_NONLEAF(root, node, PT_POINTER_TO_MEMBER);
			do
			{
				ADVANCE;
				if(!r_cast(node))
					return free_tree(root);
				node->opsign=t;
				root->children.push_back(node);
				t=LOOK_AHEAD(0);
			}while(t==CT_DOT_STAR||t==CT_ARROW_STAR);
		}
		return true;
	}
#define		NEXT_CALL	r_pm
#define		FUNC_NAME	r_multiplicative//multiply.expr  :=  pm.expr  |  multiply.expr ('*' | '/' | '%') pm.expr
#define		CONDITION	t==CT_ASTERIX||t==CT_SLASH||t==CT_MODULO
#define		LABEL		PT_MUL
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_multiplicative
#define		FUNC_NAME	r_additive		//additive.expr  :=  multiply.expr  |  additive.expr ('+' | '-') multiply.expr
#define		CONDITION	t==CT_PLUS||t==CT_MINUS
#define		LABEL		PT_ADD
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_additive
#define		FUNC_NAME	r_shift			//shift.expr  :=  additive.expr  |  shift.expr ShiftOp additive.expr
#define		CONDITION	t==CT_SHIFT_LEFT||t==CT_SHIFT_RIGHT
#define		LABEL		PT_SHIFT
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_shift
#define		FUNC_NAME	r_relational	//relational.expr  :=  shift.expr  |  relational.expr (RelOp | '<' | '>') shift.expr
#define		CONDITION	t==CT_LESS||t==CT_LESS_EQUAL||t==CT_GREATER||t==CT_GREATER_EQUAL
#define		LABEL		PT_RELATIONAL
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_relational
#define		FUNC_NAME	r_equality		//equality.expr  :=  relational.expr  |  equality.expr EqualOp relational.expr
#define		CONDITION	t==CT_EQUAL||t==CT_NOT_EQUAL
#define		LABEL		PT_EQUALITY
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_equality
#define		FUNC_NAME	r_bitwise_and	//and.expr  :=  equality.expr  |  and.expr '&' equality.expr
#define		CONDITION	t==CT_AMPERSAND
#define		LABEL		PT_BITAND
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_bitwise_and
#define		FUNC_NAME	r_bitwise_xor	//exclusive.or.expr  :=  and.expr  |  exclusive.or.expr '^' and.expr
#define		CONDITION	t==CT_CARET
#define		LABEL		PT_BITXOR
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_bitwise_xor
#define		FUNC_NAME	r_bitwise_or	//inclusive.or.expr  :=  exclusive.or.expr  |  inclusive.or.expr '|' exclusive.or.expr
#define		CONDITION	t==CT_VBAR
#define		LABEL		PT_BITOR
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_bitwise_or
#define		FUNC_NAME	r_logic_and		//logical.and.expr  :=  inclusive.or.expr  |  logical.and.expr LogAndOp inclusive.or.expr
#define		CONDITION	t==CT_LOGIC_AND
#define		LABEL		PT_LOGICAND
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL

#define		NEXT_CALL	r_logic_and
#define		FUNC_NAME	r_logic_or		//logical.or.expr  :=  logical.and.expr  |  logical.or.expr LogOrOp logical.and.expr		left-to-right
#define		CONDITION	t==CT_LOGIC_OR
#define		LABEL		PT_LOGICOR
#include	"acc2_parser_expr.h"
#undef		FUNC_NAME
#undef		NEXT_CALL
#undef		CONDITION
#undef		LABEL
	bool		r_conditional_expr(IRNode *&root)//  conditional.expr  :=  logical.or.expr {'?' comma.expression ':' conditional.expr}  right-to-left
	{
		if(!r_logic_or(root))
			return free_tree(root);
		if(LOOK_AHEAD(0)==CT_QUESTION)
		{
			ADVANCE;
			IRNode *node=nullptr;
			INSERT_NONLEAF(root, node, PT_CONDITIONAL);
			if(!r_comma_expr(node))
				return free_tree(root);
			root->children.push_back(node);
			if(LOOK_AHEAD(0)!=CT_COLON)
				return free_tree(root);
			if(!r_conditional_expr(node))
				return free_tree(root);
			root->children.push_back(node);
		}
		return true;
	}
	bool		r_assign_expr(IRNode *&root)//assign_expr  :=  conditional.expr [(AssignOp | '=') assign_expr]?	right-to-left		renamed from expression
	{
		if(!r_conditional_expr(root))
			return free_tree(root);
		switch(LOOK_AHEAD(0))
		{
		case CT_ASSIGN:
		case CT_ASSIGN_ADD:case CT_ASSIGN_SUB:
		case CT_ASSIGN_MUL:case CT_ASSIGN_DIV:case CT_ASSIGN_MOD:
		case CT_ASSIGN_XOR:case CT_ASSIGN_OR:case CT_ASSIGN_AND:case CT_ASSIGN_SL:case CT_ASSIGN_SR:
			{
				IRNode *node=nullptr;
				INSERT_NONLEAF(root, node, LOOK_AHEAD(0));
				if(!r_assign_expr(node))
					return free_tree(root);
				root->children.push_back(node);
			}
			break;
		}
		return true;
	}
	bool		r_comma_expr(IRNode *&root)//marker
	{
		if(!r_assign_expr(root))
			return free_tree(root);
		if(LOOK_AHEAD(0)==CT_COMMA)
		{
			IRNode *node=nullptr;
			INSERT_NONLEAF(root, node, PT_COMMA);
			do
			{
				ADVANCE;
				if(!r_assign_expr(node))
					return free_tree(root);
				root->children.push_back(node);
			}while(LOOK_AHEAD(0)==CT_COMMA);
		}
		return true;
	}
	bool		r_initialize_expr(IRNode *&root)//initialize.expr  :=  expression  |  '{' initialize.expr [',' initialize.expr]* [',']? '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return r_assign_expr(root);
		ADVANCE;
		INSERT_LEAF(root, PT_INITIALIZER_LIST, nullptr);
		for(auto t=LOOK_AHEAD(0);t!=CT_RBRACE;t=LOOK_AHEAD(0))
		{
			root->children.push_back(nullptr);
			if(!r_initialize_expr(root->children.back()))
			{
				skip_till_after(CT_RBRACE);
				return free_tree(root);
			}
			switch(LOOK_AHEAD(0))
			{
			case CT_RBRACE:
				ADVANCE;
				break;
			case CT_COMMA:
				ADVANCE;
				continue;
			default:
				skip_till_after(CT_RBRACE);
				return free_tree(root);//error recovery?
			}
			break;
		}
		return true;
	}

//base.specifiers  :=  ':' base.specifier (',' base.specifier)*
//
//base.specifier  :=  {{VIRTUAL} (PUBLIC | PROTECTED | PRIVATE) {VIRTUAL}} name
	bool		r_base_specifiers(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_COLON)
			return false;
		ADVANCE;

		INSERT_LEAF(root, PT_BASE_SPECIFIER, nullptr);

		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t==CT_VIRTUAL)
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_VIRTUAL, CT_IGNORED));
				t=LOOK_AHEAD(0);
			}
			switch(t)
			{
			case CT_PUBLIC:
			case CT_PROTECTED:
			case CT_PRIVATE:
				ADVANCE;
				root->children.push_back(new IRNode(t, CT_IGNORED));
				break;
			}
			if(t==CT_VIRTUAL)
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_VIRTUAL, CT_IGNORED));
				t=LOOK_AHEAD(0);
			}
			root->children.push_back(nullptr);
			if(!r_name(root->children.back()))
				return free_tree(root);
			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}
	bool		r_access_decl(IRNode *&root)//access.decl  :=  name ';'
	{
		if(!r_name(root))
			return false;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		return true;
	}
//class.member
//  : (PUBLIC | PROTECTED | PRIVATE) ':'
//  | user.access.spec
//  | ';'
//  | typedef.stmt
//  | template.decl
//  | using.declaration
//  | metaclass.decl
//  | declaration
//  | access.decl
	bool		r_class_member(IRNode *&root)
	{
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_PRIVATE:
		case CT_PROTECTED:
		case CT_PUBLIC:
			ADVANCE;
			INSERT_LEAF(root, t, nullptr);
			if(LOOK_AHEAD(0)!=CT_COLON)
				return free_tree(root);
			break;
		//case UserKeyword4://user.access.spec
		//	break;
		case CT_SEMICOLON:
			break;
		case CT_TYPEDEF:
			if(!r_typedef(root))
				return false;
			break;
		case CT_TEMPLATE:
			if(!r_template_decl(root))
				return false;
			break;
		case CT_USING:
			if(!r_using(root))
				return false;
			break;
		case CT_CLASS:
		case CT_STRUCT:
		case CT_UNION:
		case CT_ENUM:
			if(!r_metaclass_decl(root))
				return false;
			break;
		default:
			{
				int idx=current_idx;
				if(r_declaration(root))
					return true;
				current_idx=idx;
				return r_access_decl(root);
			}
			break;
		}
		return true;
	}
	bool		r_class_body(IRNode *&root)//class.body  :=  '{' (class.member)* '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return false;
		ADVANCE;
		scope_enter(scope_id_lbrace);

		INSERT_LEAF(root, PT_CLASS_BODY, nullptr);

		while(LOOK_AHEAD(0)!=CT_RBRACE)
		{
			root->children.push_back(nullptr);
			if(!r_class_member(root->children.back()))
			{
				skip_till_after(CT_RBRACE);
				scope_exit();
				return free_tree(root);
			}
		}
		ADVANCE;
		scope_exit();
		return true;
	}
//class.spec
//  : {userdef.keyword} class.key class.body
//  | {userdef.keyword} class.key name {class.body}
//  | {userdef.keyword} class.key name ':' base.specifiers class.body
//
//class.key  :=  CLASS | STRUCT | UNION
	bool		r_class_spec(IRNode *&root)
	{
		//TODO: understand UserKeyword
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_CLASS:
		case CT_STRUCT:
		case CT_UNION:
			break;
		default:
			return false;
		}
		bool body_is_obligatory=true;
		auto t2=&LOOK_AHEAD_TOKEN(1);
		if(t2->type==CT_ID)
		{
			ADVANCE_BY(2);
			INSERT_LEAF(root, t, t2->sdata);//children: {???}
			//TODO: record the class name in current scope for lookup

			if(body_is_obligatory=LOOK_AHEAD(0)==CT_COLON)//inheritance
			{
				root->children.push_back(nullptr);
				if(!r_base_specifiers(root->children.back()))
					return free_tree(root);
			}
		}
		else//anonymous class
		{
			ADVANCE;
			INSERT_LEAF(root, t, nullptr);//TODO: encode class name as a number (impossible for an identifier), if necessary
		}

		if(LOOK_AHEAD(0)==CT_LBRACE)
		{
			root->children.push_back(nullptr);
			if(!r_class_body(root->children.back()))
				return free_tree(root);
		}
		else if(body_is_obligatory)
			return free_tree(root);
		return true;
	}

	bool		r_enum_body(IRNode *&root)//enum.body  :=  Identifier {'=' expression} (',' Identifier {'=' expression})* {','}
	{
		INSERT_LEAF(root, PT_ENUM_BODY, nullptr);//children: {CT_ID*}
		for(;;)
		{
			auto t=&LOOK_AHEAD_TOKEN(0);
			if(t->type==CT_RBRACE)
				break;
			if(t->type!=CT_ID)
				return free_tree(root);
			ADVANCE;
			root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));

			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				ADVANCE;
				auto label=root->children.back();
				label->children.push_back(nullptr);
				if(!r_assign_expr(label->children.back()))
				{
					skip_till_after(CT_RBRACE);
					return free_tree(root);
				}
			}

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}
//enum.spec
//  : ENUM Identifier
//  | ENUM {Identifier} '{' {enum.body} '}'
	bool		r_enum_spec(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_ENUM)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_ENUM, nullptr);//children: {???}

		auto t=&LOOK_AHEAD_TOKEN(0);
		if(t->type==CT_ID)
		{
			ADVANCE;
			root->sdata=t->sdata;
			//TODO: add enum name for qualified lookup

			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type==CT_LBRACE)
				ADVANCE;
			else
				return true;
		}
		if(t->type!=CT_LBRACE)
			return free_tree(root);

		root->children.push_back(nullptr);
		if(!r_enum_body(root->children.back()))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_RBRACE)
			return free_tree(root);
		ADVANCE;

		return true;
	}

	bool		r_opt_member_spec(int &fvi_flag)//member.spec := (friend|inline|virtual)+		bitfield: {bit2: friend, bit1: virtual, bit0: inline}
	{
		for(auto t=LOOK_AHEAD(0);t==CT_INLINE||t==CT_VIRTUAL||t==CT_FRIEND;)
		{
			ADVANCE;
			fvi_flag|=int(t==CT_FRIEND)<<2|int(t==CT_VIRTUAL)<<1|int(t==CT_INLINE);
			t=LOOK_AHEAD(0);
		}
	/*	auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_INLINE:
		case CT_FRIEND:
		case CT_VIRTUAL:
			{
				fvi_flag|=int(t==CT_FRIEND)<<2|int(t==CT_VIRTUAL)<<1|int(t==CT_INLINE);
				//INSERT_LEAF(root, PT_MEMBER_SPEC, nullptr);
				//(int&)root->opsign=int(t==CT_FRIEND)<<2|int(t==CT_VIRTUAL)<<1|int(t==CT_INLINE);
				ADVANCE;
				for(;t=LOOK_AHEAD(0);ADVANCE)
				{
					switch(t)
					{
					case CT_INLINE:
					case CT_FRIEND:
					case CT_VIRTUAL:
						{
							int mask=int(t==CT_FRIEND)<<2|int(t==CT_VIRTUAL)<<1|int(t==CT_INLINE);
							if(root->opsign&mask)
							{
								//TODO: warning: repeated member specifier
							}
							(int&)root->opsign|=mask;
						}
						continue;
					default:
						break;
					}
					break;
				}
			}
		}//*/
		return true;
	}
	bool		r_opt_storage_spec(StorageTypeSpecifier &esmr_flag)//storage.spec := extern|static|mutable|register
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_EXTERN:
			esmr_flag=STORAGE_EXTERN;
			break;
		case CT_STATIC:
			esmr_flag=STORAGE_STATIC;
			break;
		case CT_MUTABLE:
			esmr_flag=STORAGE_MUTABLE;
			break;
		case CT_REGISTER:
			esmr_flag=STORAGE_REGISTER;
			break;
		default:
			esmr_flag=STORAGE_UNSPECIFIED;
			//esmr_flag=STORAGE_EXTERN;
			break;
		}
	/*	auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_EXTERN:
		case CT_STATIC:
		case CT_MUTABLE:
		case CT_REGISTER:
			//INSERT_LEAF(root, t, nullptr);
			INSERT_LEAF(root, PT_STORAGE_SPEC, nullptr);
			root->opsign=t;
			ADVANCE;
			break;
		}//*/
		return true;
	}
	bool		r_opt_cv_qualify(int &cv_flag)//cv.qualify  :=  (const|volatile)+		//{bit 1: volatile, bit 0: const}
	{
		for(auto t=LOOK_AHEAD(0);t==CT_CONST||t==CT_VOLATILE;)
		{
			ADVANCE;
			cv_flag|=1<<int(t==CT_VOLATILE);
			t=LOOK_AHEAD(0);
		}
		return true;
#if 0
		auto t=LOOK_AHEAD(0);
		switch(t)
		{
		case CT_CONST:
		case CT_VOLATILE:
			{
				ADVANCE;
				INSERT_LEAF(root, PT_MEMBER_SPEC, nullptr);
				(int&)root->opsign=1<<int(t==CT_VOLATILE);
				for(;t=LOOK_AHEAD(0);ADVANCE)
				{
					switch(t)
					{
					case CT_CONST:
					case CT_VOLATILE:
						(int&)root->opsign|=1<<int(t==CT_VOLATILE);
						continue;
					default:
						break;
					}
					break;
				}
			}
		}
		return true;
#endif
	}
	bool		r_opt_int_type_or_class_spec(IRNode *&root)//int.type.or.class.spec := (char|wchar_t|int|short|long|signed|unsigned|float|double|void|bool)+  |  class.spec  |  enum.spec
	{
		root=nullptr;//
		TypeInfo type;
		Name::Node *scope=nullptr;
		short long_count=0, int_count=0;
		for(;;)
		{
			auto token=&LOOK_AHEAD_TOKEN(0);
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
				ADVANCE;
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
						type.datatype=TYPE_FLOAT;
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
						type.datatype=TYPE_FLOAT;
						type.size=8;
					}
					else if(type.datatype==TYPE_INT&&long_count==1&&!int_count)
					{
						type.datatype=TYPE_FLOAT;
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
					scope=scope_lookup(token->sdata, false);//check if identifier==class/struct/union in this/global scope		TODO: support global lookup
					if(!scope)
					{
						ADVANCE_BY(-1);
						if(root)
							root->set(add_type(type));
						return true;//not declared yet
						
						//return free_tree(root);//error: unexpected identifier
					}
					switch(scope->data.tdata->datatype)
					{
					case TYPE_ENUM:
					case TYPE_CLASS:
					case TYPE_STRUCT:
					case TYPE_UNION:
					case TYPE_SIMD://special struct
						{
							if(type.datatype!=TYPE_UNASSIGNED)
								return free_tree(root);
							//merge with type			//TODO: unroll the struct to typedb at declaration
							auto typeinfo=scope->data.tdata;
							type.datatype=typeinfo->datatype;
							if(type.logalign<typeinfo->logalign)
								type.logalign=typeinfo->logalign;
							type.size=typeinfo->size;
						}
						break;
					default:
						//error: identifier is not an enum/class/struct/union
						return free_tree(root);
					}
					break;
				}
				if(root==nullptr)
					INSERT_LEAF(root, PT_TYPE, nullptr);
				continue;
			case CT_CLASS:
			case CT_STRUCT:
			case CT_UNION://or UserKeyword
				return r_class_spec(root);
			case CT_ENUM:
				return r_enum_spec(root);
			}
			break;
		}//end for
		if(root)
			root->set(add_type(type));
		return true;
	}

	bool		r_simple_declaration(IRNode *&root)//condition controlling expression of switch/while/if
	{
		int cv_flag=0;
		IRNode *ic=nullptr, *name=nullptr;
		if(!r_opt_cv_qualify(cv_flag)||!r_opt_int_type_or_class_spec(ic))
			return free_tree(ic);
		if(!r_name(name))
			return free_tree(ic), free_tree(name);

		IRNode *decl=nullptr;
		if(!r_declarator2(decl, DECLKIND_NORMAL, false, true, true))
			return free_tree(ic), free_tree(name), free_tree(decl);
		if(LOOK_AHEAD(0)!=CT_ASSIGN)//TODO: make sense of this
			return free_tree(ic), free_tree(name), free_tree(decl);
		ADVANCE;

		IRNode *expr;
		if(!r_assign_expr(expr))
			return free_tree(ic), free_tree(name), free_tree(decl), free_tree(expr);

		//TODO: join nodes

		return true;
	}
	bool		r_condition(IRNode *&root)//condition  :=  simple.declaration  |  comma.expresion
	{
		int t_idx=current_idx;
		if(!r_simple_declaration(root))
			return false;//TODO: minimal required calls to free_tree()
		current_idx=t_idx;//restore state
		return r_comma_expr(root);
		//return r_comma_expr(root)||free_tree(root);//X  not required here
	}

	bool		r_typedef(IRNode *&root)//typedef.stmt  :=  TYPEDEF type.specifier declarators ';'
	{
		if(LOOK_AHEAD(0)!=CT_TYPEDEF)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_TYPEDEF, nullptr);

		root->children.push_back(nullptr);
		if(!r_typespecifier(root->children.back(), false))
			return free_tree(root);

		if(!r_declarators_nonnull(root, true, false)||LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;

#ifdef DEBUG_PARSER
		debugprint(root);//
#endif

		//if(root->children[0]&&root->children[1])//type & declarations node
		//{
		//	auto rtype=add_type(*root->children[0]->tdata);
		//	for(int k=0;k<root->children[1]->children.size();++k)
		//	{
		//		auto declk=root->children[1]->children[k];
		//		if(declk->children.size()==2&&declk->children[1])//function decl
		//		{
		//			for(int k2=0;declk->children[1]->children.size();++k2)//X  func args could be function pointers
		//			{
		//				auto argtype=declk->children[1]->children[k2];
		//			}
		//		}
		//	}
		//}
		return true;
	}
	bool		r_if(IRNode *&root)//if.statement  :=  IF '(' condition ')' statement { ELSE statement }
	{
		if(LOOK_AHEAD(0)!=CT_IF||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, CT_IF, nullptr);

		IRNode *child=nullptr;
		if(!r_condition(child))
			return free_tree(root);
		root->children.push_back(child);

		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);

		if(!r_statement(child))
			return free_tree(root);
		root->children.push_back(child);

		if(LOOK_AHEAD(0)==CT_ELSE)
		{
			ADVANCE;
			if(!r_statement(child))
				return free_tree(root);
			root->children.push_back(child);
		}
		return true;
	}
	bool		r_switch(IRNode *&root)//switch.statement  :=  SWITCH '(' condition ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_SWITCH||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, CT_SWITCH, nullptr);

		IRNode *child=nullptr;
		if(!r_condition(child))
			return free_tree(root);
		root->children.push_back(child);

		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);

		if(!r_statement(child))
			return free_tree(root);
		root->children.push_back(child);

		return true;
	}
	bool		r_while(IRNode *&root)//while.statement  :=  WHILE '(' condition ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_WHILE||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, CT_WHILE, nullptr);

		IRNode *child=nullptr;
		if(!r_condition(child))
			return free_tree(root);
		root->children.push_back(child);

		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);
		
		if(!r_statement(child))
			return free_tree(root);
		root->children.push_back(child);

		return true;
	}
	bool		r_do_while(IRNode *&root)//do.statement  :=  DO statement WHILE '(' comma.expression ')' ';'
	{
		if(LOOK_AHEAD(0)!=CT_DO)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_DO, nullptr);
		root->children.assign_pod(2, nullptr);

		if(!r_statement(root->children[0]))
			return free_tree(root);
		
		if(LOOK_AHEAD(0)!=CT_WHILE||LOOK_AHEAD(1)!=CT_LPR)
			return free_tree(root);
		ADVANCE_BY(2);
		
		if(!r_comma_expr(root->children[1]))
			return free_tree(root);
		
		if(LOOK_AHEAD(0)!=CT_RPR||LOOK_AHEAD(1)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE_BY(2);

		return true;
	}
	bool		r_for(IRNode *&root)//for.statement  :=  FOR '(' expr.statement {comma.expression} ';' {comma.expression} ')' statement
	{
		if(LOOK_AHEAD(0)!=CT_FOR||LOOK_AHEAD(1)!=CT_LPR)
			return false;
		ADVANCE_BY(2);
		
		INSERT_LEAF(root, CT_FOR, nullptr);//children: {initialization, condition, increment, statement}, check for nullptr
		root->children.assign_pod(4, nullptr);
		
		if(!r_expr_stmt(root->children[0]))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON&&!r_comma_expr(root->children[1]))
			return free_tree(root);
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;
		
		if(LOOK_AHEAD(0)!=CT_RPR&&!r_comma_expr(root->children[2]))
			return free_tree(root);
		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);
		ADVANCE;

		if(!r_statement(root->children[3]))
			return free_tree(root);

		return true;
	}
	bool		r_arg_declaration(IRNode *&root)//arg.declaration  :=  {userdef.keyword | REGISTER} type.specifier arg.declarator {'=' initialize.expr}
	{
		INSERT_LEAF(root, PT_ARG_DECL, nullptr);//children: {type, argname [, default value]}

		switch(LOOK_AHEAD(0))
		{
		case CT_REGISTER:
			ADVANCE;
			//ignore register
			break;
		//case CT_CDECL:
		//case CT_THISCALL:
		//case CT_STDCALL:
		//	root->children.push_back(new IRNode(LOOK_AHEAD(0), CT_IGNORED));
		//	break;
		}
		root->children.push_back(nullptr);
		
		if(!r_typespecifier(root->children.back(), true))
			return free_tree(root);

		root->children.push_back(nullptr);
		if(!r_declarator2(root->children.back(), DECLKIND_ARG, false, true, false))
			return free_tree(root);
		if(!root->children.back())//TODO: differentiate between: no parens, and ()
			root->children.pop_back();

		if(LOOK_AHEAD(0)==CT_ASSIGN)
		{
			ADVANCE;
			root->children.push_back(nullptr);
			if(!r_initialize_expr(root->children.back()))
				return free_tree(root);
		}

		return true;
	}
//try.statement  :=  TRY compound.statement (exception.handler)+ ';'
//
//exception.handler  :=  CATCH '(' (arg.declaration | Ellipsis) ')' compound.statement
	bool		r_try_catch(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_TRY)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_TRY, nullptr);//children: {statement, {arg, handlerstmt}+}

		root->children.push_back(nullptr);
		if(!r_compoundstatement(root->children[0]))
			return free_tree(root);

		do
		{
			if(LOOK_AHEAD(0)!=CT_CATCH||LOOK_AHEAD(1)!=CT_LPR)
				return free_tree(root);
			ADVANCE_BY(2);

			if(LOOK_AHEAD(0)==CT_ELLIPSIS)
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_ELLIPSIS, CT_IGNORED));
			}
			else
			{
				root->children.push_back(nullptr);
				if(!r_arg_declaration(root->children.back()))
					return free_tree(root);
			}

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);

			root->children.push_back(nullptr);
			if(!r_compoundstatement(root->children.back()))
				return free_tree(root);
		}while(LOOK_AHEAD(0)==CT_CATCH);

		return true;
	}

	bool		r_ptr_to_member(IRNode *&root)//ptr.to.member  :=  {'::'} (identifier {template.args} '::')+ '*'
	{
		if(LOOK_AHEAD(0)==CT_SCOPE)
		{
			ADVANCE;
			//TODO: search only global scope
		}
		for(;;)
		{
			auto t=&LOOK_AHEAD_TOKEN(0);
			if(t->type!=CT_ID)
				return free_tree(root);
			ADVANCE;

			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type==CT_LESS)
			{
				if(!r_template_arglist(root))//
					return free_tree(root);
				//TODO: template call lookup
			}
			else
			{
				//TODO: simple name lookup
			}
			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type!=CT_SCOPE)
				return free_tree(root);
			ADVANCE;
			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type==CT_ASTERIX)
			{
				ADVANCE;
				break;
			}
		}
		return true;
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
	bool		r_cast_operator_name(IRNode *&root)//cast.operator.name  :=  {cv.qualify} (integral.type.or.class.spec | name) {cv.qualify} {(ptr.operator)*}
	{
		INSERT_LEAF(root, PT_CAST, nullptr);

		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		root->children.push_back(nullptr);
		if(!r_opt_int_type_or_class_spec(root->children.back()))
			return free_tree(root);
		if(!root->children.back())
		{
			if(!r_name(root->children.back()))
				return free_tree(root);
		}
		root->children.push_back(nullptr);
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);
		
		root->children.push_back(nullptr);
		if(!r_opt_ptr_operator(root->children.back()))
			return free_tree(root);

		return true;//TODO: complete this
	}
	bool		r_operator_name(IRNode *&root)
	{
		auto t=LOOK_AHEAD(0);
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
			ADVANCE;
			INSERT_LEAF(root, t, nullptr);
			return true;

		case CT_NEW:case CT_DELETE:
			ADVANCE;
			INSERT_LEAF(root, t, nullptr);
			if(LOOK_AHEAD(0)==CT_LBRACKET&&LOOK_AHEAD(1)==CT_RBRACKET)
			{
				ADVANCE_BY(2);
				root->children.push_back(new IRNode(CT_LBRACKET, CT_IGNORED));
				return true;
			}
			return true;

		case CT_LPR:
			if(LOOK_AHEAD(1)==CT_RPR)
			{
				ADVANCE_BY(2);
				INSERT_LEAF(root, t, nullptr);
				return true;
			}
			break;
		case CT_LBRACKET:
			if(LOOK_AHEAD(1)==CT_RBRACKET)
			{
				ADVANCE_BY(2);
				INSERT_LEAF(root, t, nullptr);
				return true;
			}
			break;

		case CT_VAL_STRING_LITERAL://user-defined literal (since C++11)
			if(!strlen(current_ex->operator[](current_idx).sdata))//TODO: compare pointers instead
			{
				ADVANCE;
				INSERT_LEAF(root, t, nullptr);
				return true;
			}
			break;
		}
		return r_cast_operator_name(root);
	}
//name : {'::'} name2 ('::' name2)*
//
//name2
//  : Identifier {template.args}
//  | '~' Identifier
//  | OPERATOR operator.name {template.args}
	bool		r_name(IRNode *&root)
	{
		INSERT_LEAF(root, PT_NAME, nullptr);//children: {::?, name(s)}
		auto t=&LOOK_AHEAD_TOKEN(0);
		if(t->type==CT_SCOPE)
		{
			ADVANCE;
			//TODO: search global scope only
			root->children.push_back(new IRNode(CT_SCOPE, CT_IGNORED));
		}
		else
		{
			//TODO: search all nested scopes upwards
			if(t->type==CT_DECLTYPE)
			{
				ADVANCE;
				if(LOOK_AHEAD(0)!=CT_LPR)
					return free_tree(root);
				root->children.push_back(new IRNode(CT_DECLTYPE, CT_IGNORED));
				root->children.push_back(nullptr);
				if(!r_name(root->children.back()))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_RPR)
					return free_tree(root);
				return true;
			}
		}
		for(;;)
		{
			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type==CT_TEMPLATE)
			{
				ADVANCE;
				t=&LOOK_AHEAD_TOKEN(0);//TODO: make sure template can be skipped here
			}
			
			switch(t->type)
			{
			case CT_ID:
				ADVANCE;
				root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));
				t=&LOOK_AHEAD_TOKEN(0);
				if(t->type==CT_LESS)
				{
					root->children.push_back(nullptr);
					if(!r_template_arglist(root->children.back()))
						return free_tree(root);
					t=&LOOK_AHEAD_TOKEN(0);
				}
				if(t->type!=CT_SCOPE)
					return true;
				ADVANCE;
				root->children.push_back(new IRNode(CT_SCOPE, CT_IGNORED));
				break;
			case CT_TILDE:
				ADVANCE;
				root->children.push_back(new IRNode(CT_TILDE, CT_IGNORED));
				t=&LOOK_AHEAD_TOKEN(0);
				if(t->type!=CT_ID)
					return free_tree(root);
				ADVANCE;
				root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));
				return true;
			case CT_OPERATOR:
				{
					ADVANCE;
					root->children.push_back(new IRNode(CT_OPERATOR, CT_IGNORED));
					root->children.push_back(nullptr);
					if(!r_operator_name(root->children.back()))
						return free_tree(root);
					t=&LOOK_AHEAD_TOKEN(0);
					if(t->type==CT_LESS)
					{
						root->children.push_back(nullptr);
						if(!r_template_arglist(root->children.back()))
							return free_tree(root);
					}
					return true;
				}
				break;
			default:
				return free_tree(root);
			}
		}
		return true;
	}
	bool		r_int_decl_stmt(IRNode *&root)//integral.decl.statement  :=  decl.head integral.type.or.class.spec {cv.qualify} {declarators} ';'
	{
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);//TODO: pass cv_q to r_int_decl_stmt()
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);
		if(LOOK_AHEAD(0)==CT_SEMICOLON)
		{
			ADVANCE;
			return true;
		}
		if(!r_declarators_nonnull(root, false, true))
		//root->children.push_back(nullptr);
		//if(!r_declarators(root->children.back(), false, true))
			return free_tree(root);
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;
		return true;
	}
	bool		r_const_decl(IRNode *&root)
	{
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);
		if(!r_declarators_nonnull(root, false, false))//TODO: decl these variables as const
		//root->children.push_back(nullptr);
		//if(!r_declarators(root->children.back(), false, false))
			return false;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;
		return true;
	}
	bool		r_other_decl(IRNode *&root)//inlined
	{
#if 0
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);

		root->children.push_back(nullptr);
		if(!r_name(root->children.back()))
			return free_tree(root);
		
		root->children.push_back(nullptr);
		if(!r_opt_cv_qualify(root->children.back()))
			return free_tree(root);

		if(root->children.back()!=nullptr)
			root->children.push_back(nullptr);
		if(!r_declarators(root->children.back(), false, false))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
#endif

		return true;
	}
	bool		r_other_decl_stmt(IRNode *&root)//other.decl.statement  :=  decl.head name {cv.qualify} declarators ';'
	{
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);

		root->children.push_back(nullptr);
		if(!r_name(root->children.back()))
			return free_tree(root);
		
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		//if(root->children.back()!=nullptr)
		//	root->children.push_back(nullptr);
		//if(!r_declarators(root->children.back(), false, false))
		if(!r_declarators_nonnull(root, false, false))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);

		return true;
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
	bool		r_decl_stmt(IRNode *&root)
	{
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);//children: {storage, cv, integral, int/const/other decl}		TODO: improve this

		StorageTypeSpecifier esmr_flag=STORAGE_EXTERN;//TODO: storage specifier
		int cv_flag=0;
		bool cv_specified=false;
		if(!r_opt_storage_spec(esmr_flag)||!r_opt_cv_qualify(cv_flag)||!r_opt_int_type_or_class_spec(root->children[1]))
			return free_tree(root);
		root->flags=0;
		if(esmr_flag==STORAGE_UNSPECIFIED)
			root->flag_esmr=STORAGE_EXTERN;
		else
			root->flag_esmr=esmr_flag;
		root->flag_const=cv_flag&1;
		root->flag_volatile=cv_flag>>1&1;
		//IRNode *storage=nullptr, *cv=nullptr, *integral=nullptr;
		//if(!r_opt_storage_spec(storage)||!r_opt_cv_qualify(cv)||r_opt_int_type_or_class_spec(integral))
		//	return free_tree(storage), free_tree(cv), free_tree(integral);

		root->children.push_back(nullptr);
		if(esmr_flag!=STORAGE_UNSPECIFIED)
		{
			if(!r_int_decl_stmt(root->children.back()))
				return free_tree(root);
			return true;
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
		if(cv_flag&&(t==CT_ID&&assign||t==CT_ASTERIX))
		{
			if(!r_const_decl(root->children.back()))
				return free_tree(root);
			return true;
		}
		if(!r_other_decl_stmt(root->children.back()))
			return free_tree(root);
		return true;
	}
//expr.statement
//  : ';'
//  | declaration.statement
//  | comma.expression ';'
//  | openc++.postfix.expr
//  | openc++.primary.exp
	bool		r_expr_stmt(IRNode *&root)
	{
		if(LOOK_AHEAD(0)==CT_SEMICOLON)
			return true;
		int t_idx=current_idx;//save state
		if(r_decl_stmt(root))
			return true;
		current_idx=t_idx;//restore state
		if(!r_comma_expr(root))
			return false;
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return false;
		return true;
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
	bool		r_statement(IRNode *&root)
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_LBRACE:
			return r_compoundstatement(root);
		case CT_TYPEDEF:
			return r_typedef(root);
		case CT_IF:
			return r_if(root);
		case CT_SWITCH:
			return r_switch(root);
		case CT_WHILE:
			return r_while(root);
		case CT_DO:
			return r_do_while(root);
		case CT_FOR:
			return r_for(root);
		case CT_TRY:
			return r_try_catch(root);
		case CT_BREAK:
			ADVANCE;
			INSERT_LEAF(root, CT_BREAK, nullptr);
			break;
		case CT_CONTINUE:
			ADVANCE;
			INSERT_LEAF(root, CT_CONTINUE, nullptr);
			break;
		case CT_RETURN:
			{
				ADVANCE;
				INSERT_LEAF(root, CT_RETURN, nullptr);

				if(LOOK_AHEAD(0)==CT_SEMICOLON)
					return true;
				root->children.push_back(nullptr);
				if(!r_comma_expr(root->children.back()))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
					return free_tree(root);//TODO: error: expected a semicolon
				ADVANCE;
			}
			break;
		case CT_GOTO:
			{
				ADVANCE;
				if(LOOK_AHEAD(0)!=CT_ID||LOOK_AHEAD(1)!=CT_SEMICOLON)
					return false;
				auto t=&LOOK_AHEAD_TOKEN(0);
				ADVANCE_BY(2);
				INSERT_LEAF(root, CT_GOTO, t->sdata);//TODO: maintain labels inside this function
			}
			break;
		case CT_CASE:
			ADVANCE;
			INSERT_LEAF(root, CT_CASE, nullptr);
			root->children.push_back(nullptr);
			if(!r_assign_expr(root->children.back()))
				return free_tree(root);
			if(LOOK_AHEAD(0)!=CT_COLON)
			{
				//TODO: error: expected a colon
				return free_tree(root);
			}
			ADVANCE;
			break;
		case CT_DEFAULT:
			if(LOOK_AHEAD(1)!=CT_COLON)
			{
				//TODO: error: expected a colon
				return false;
			}
			ADVANCE_BY(2);
			INSERT_LEAF(root, CT_DEFAULT, nullptr);
			break;
		case CT_ID://label for goto
			if(LOOK_AHEAD(1)==CT_COLON)
			{
				INSERT_LEAF(root, PT_JUMPLABEL, LOOK_AHEAD_TOKEN(0).sdata);
				ADVANCE_BY(2);
				return true;//non-standard: statement not required after label
			}
			//no break
		default:
			return r_expr_stmt(root);
		}
		return true;
	}
	bool		r_compoundstatement(IRNode *&root)//compound.statement  :=  '{' (statement)* '}'
	{
		auto t=LOOK_AHEAD(0);
		if(t!=CT_LBRACE)
			return false;
		//delayed advance
		scope_enter(scope_id_lbrace);

		t=LOOK_AHEAD(1);
		if(t!=CT_RBRACE)
		{
			ADVANCE;
			INSERT_LEAF(root, PT_CODE_BLOCK, nullptr);
			do
			{
				root->children.push_back(nullptr);
				if(!r_statement(root->children.back()))
				{
					skip_till_after(CT_RBRACE);//TODO: error: expected a statement
					return free_tree(root);
				}
				t=LOOK_AHEAD(0);
			}while(t!=CT_RBRACE);
		}
		ADVANCE;
		scope_exit();
		return true;
	}

	bool		r_arg_decllist(IRNode *&root)//arg.decl.list  :=  empty  |  arg.declaration ( ',' arg.declaration )* {{ ',' } Ellipses}
	{
		INSERT_LEAF(root, PT_DECL_LIST, nullptr);
		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t==CT_RPR)
			{
				root->children.push_back(new IRNode(CT_VOID, CT_IGNORED));
				break;
			}
			if(t==CT_ELLIPSIS)
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_ELLIPSIS, CT_IGNORED));
				break;
			}
			root->children.push_back(nullptr);
			if(!r_arg_declaration(root->children.back()))
				return free_tree(root);
			t=LOOK_AHEAD(0);
			if(t==CT_RPR)
				break;
			if(t==CT_COMMA)
				ADVANCE;
			else if(t!=CT_RPR&&t!=CT_ELLIPSIS)
				return free_tree(root);
		}
		return true;
	}
//arg.decl.list.or.init
//  : arg.decl.list
//  | function.arguments
//
//  This rule accepts function.arguments to parse declarations like: Point p(1, 3);
//  "(1, 3)" is arg.decl.list.or.init.
	bool		r_arg_decllist_or_init(IRNode *&root, bool &is_args, bool maybe_init)
	{
		int idx=current_idx;//save state
		if(maybe_init)
		{
			if(r_func_args(root))
			{
				if(LOOK_AHEAD(0)==CT_RPR)
				{
					is_args=false;
					return true;
				}
			}
			current_idx=idx;//restore state
			return is_args=r_arg_decllist(root);
		}
		if(is_args=r_arg_decllist(root))
			return true;
		current_idx=idx;//restore state
		return r_func_args(root);
	}
	bool		r_opt_throw_decl(IRNode *&root)//throw.decl  :=  THROW '(' (name {','})* {name} ')'
	{
		if(LOOK_AHEAD(0)==CT_THROW)
		{
			ADVANCE;

			if(LOOK_AHEAD(0)!=CT_RPR)
				return false;
			ADVANCE;

			INSERT_LEAF(root, CT_THROW, nullptr);
			for(;;)
			{
				auto t=LOOK_AHEAD(0);
				if(t==CT_IGNORED)//what.
					return false;
				
				if(t==CT_RPR)
					break;

				root->children.push_back(nullptr);
				if(!r_name(root->children.back()))
					return free_tree(root);
			}

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
		}
		return true;
	}
	bool		r_member_init(IRNode *&root)//member.init  :=  name '(' function.arguments ')'
	{
		INSERT_LEAF(root, PR_MEMBER_INIT, nullptr);

		root->children.push_back(nullptr);
		if(!r_name(root->children.back()))
			return free_tree(root);

		//TODO: qualified name lookup

		if(LOOK_AHEAD(0)!=CT_LPR)
			return free_tree(root);
		ADVANCE;

		root->children.push_back(nullptr);
		if(!r_func_args(root->children.back()))
			return free_tree(root);
		
		if(LOOK_AHEAD(0)!=CT_RPR)
			return free_tree(root);
		ADVANCE;

		return true;
	}
	bool		r_member_initializers(IRNode *&root)//member.initializers  :=  ':' member.init (',' member.init)*
	{
		if(LOOK_AHEAD(0)!=CT_COLON)
			return false;

		INSERT_LEAF(root, PT_MEMBER_INIT_LIST, nullptr);

		for(;;)
		{
			root->children.push_back(nullptr);
			if(!r_member_init(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}
//declarator
//  : (ptr.operator)* (name | '(' declarator ')') ('[' comma.expression ']')* {func.args.or.init}
//
//func.args.or.init
//  : '(' arg.decl.list.or.init ')' {cv.qualify} {throw.decl} {member.initializers}
//
//  Note: We assume that '(' declarator ')' is followed by '(' or '['.
//	This is to avoid accepting a function call F(x) as a pair of a type F and a declarator x.
//	This assumption is ignored if should_be_declarator is true.
//
//  Note: An argument declaration list and a function-style initializer take a different Ptree structure.
//	e.g.
//	    int f(char) ==> .. [f ( [[[char] 0]] )]
//	    Point f(1)  ==> .. [f [( [1] )]]
//
//  Note: is_statement changes the behavior of rArgDeclListOrInit().
	bool		r_declarator2(IRNode *&root, int kind, bool recursive, bool should_be_declarator, bool is_statement)
	{
		if(recursive)
			INSERT_LEAF(root, PT_DECLARATION_PARENS, nullptr);//children: {???}			TODO: no declaration if has nothing
		else
			INSERT_LEAF(root, PT_DECLARATION, nullptr);

		root->children.push_back(nullptr);//TODO: don't push back to root optional stuff
		if(!r_opt_ptr_operator(root->children.back()))
			return free_tree(root);
		if(!root->children.back())
			root->children.pop_back();

		auto t=LOOK_AHEAD(0);
		if(t==CT_LPR)
		{
			ADVANCE;
			root->children.push_back(nullptr);
			if(!r_declarator2(root->children.back(), kind, true, true, false)||LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;

			if(!should_be_declarator)
			{
				if(kind==DECLKIND_NORMAL&&!root->children[0])
				{
					t=LOOK_AHEAD(0);
					if(t!=CT_LPR&&t!=CT_LBRACKET)
						return free_tree(root);
				}
			}
		}
		else if(kind!=DECLKIND_CAST&&(kind==DECLKIND_NORMAL||t==CT_ID||t==CT_SCOPE))
		{
			//if this is an argument declarator, "int (*)()" is valid
			if(t==CT_CDECL||t==CT_THISCALL||t==CT_STDCALL)
				ADVANCE;
			root->children.push_back(nullptr);
			if(!r_name(root->children.back()))
				return free_tree(root);
		}

		for(;;)
		{
			t=LOOK_AHEAD(0);
			if(t==CT_LPR)//function
			{
				ADVANCE;

				bool is_args=true;
				if(LOOK_AHEAD(0)==CT_RPR)
				{
					//f(void)
				}
				else
				{
					root->children.push_back(nullptr);
					if(!r_arg_decllist_or_init(root->children.back(), is_args, is_statement)||LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
				}
				ADVANCE;

				int cv_flag=0;
				if(is_args)
				{
					if(!r_opt_cv_qualify(cv_flag))
						return free_tree(root);
				}
				
				root->children.push_back(nullptr);
				if(!r_opt_throw_decl(root->children.back()))
					return free_tree(root);
				if(!root->children.back())
					root->children.pop_back();

				if(LOOK_AHEAD(0)==CT_COLON)
				{
					root->children.push_back(nullptr);
					if(!r_member_initializers(root->children.back()))
						return free_tree(root);
				}

				break;//T f(int)(char) is invalid
			}
			else if(t==CT_LBRACKET)//array
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_LBRACKET, CT_IGNORED));

				if(LOOK_AHEAD(0)!=CT_RBRACKET)
				{
					root->children.back()->children.push_back(nullptr);
					if(!r_comma_expr(root->children.back()->children.back()))
						return free_tree(root);
				}
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE;
			}
			else
				break;
		}
		if(kind==DECLKIND_ARG)
		{
			root->type=PT_ARG_DECL;
			if(!root->children.size())
				root->children.push_back(new IRNode(CT_VOID, CT_IGNORED));
		}
		else if(!root->children.size())
			free_tree(root);
		return true;
	}
	bool		r_declaratorwithinit(IRNode *&root, bool should_be_declarator, bool is_statement)//declarator.with.init  :=  ':' expression  |  declarator {'=' initialize.expr | ':' expression}		can be inlined
	{
		if(LOOK_AHEAD(0)==CT_COLON)//bit field
		{
			ADVANCE;

			if(!r_assign_expr(root))
				return false;
		}
		else
		{
			if(!r_declarator2(root, DECLKIND_NORMAL, false, should_be_declarator, is_statement))
				return false;
			switch(LOOK_AHEAD(0))
			{
			case CT_ASSIGN:
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_initialize_expr(root->children.back()))
					return false;
				break;
			case CT_COLON://bit field
				ADVANCE;
				break;
			default:
				break;
			}
		}
		return true;
	}
	bool		r_declarators_nonnull(IRNode *&root, bool should_be_declarator, bool is_statement)//declarators := declarator.with.init (',' declarator.with.init)*
	{
		//INSERT_LEAF(root, PT_DECLARATORS, nullptr);
		for(;;)
		{
			root->children.push_back(nullptr);
			if(!r_declaratorwithinit(root->children.back(), should_be_declarator, is_statement))
				return false;
				//return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}
	bool		is_constructor_or_decl()//returns true for an declaration like: T (a);
	{
		if(LOOK_AHEAD(0)!=CT_LPR)
			return false;
		switch(LOOK_AHEAD(1))
		{
		case CT_ASTERIX:
		case CT_AMPERSAND:
		case CT_LPR:
			return false;
		case CT_CONST:
		case CT_VOLATILE:
			return true;
		}
		return !is_ptr_to_member(1);
	}
	bool		r_constructor_decl(IRNode *&root)//constructor.decl  :=  '(' {arg.decl.list} ')' {cv.qualify} {throw.decl} {member.initializers} {'=' Constant}
	{
		if(LOOK_AHEAD(0)!=CT_LPR)
			return false;
		ADVANCE;

		INSERT_LEAF(root, PT_FUNC_DECL, nullptr);
		//INSERT_LEAF(root, PT_CONSTRUCTOR_DECL, nullptr);

		if(LOOK_AHEAD(0)!=CT_RPR)
		{
			root->children.push_back(nullptr);
			if(!r_arg_decllist(root->children.back())||LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
		}
		ADVANCE;
		
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		if(LOOK_AHEAD(0)==CT_COLON)
		{
			root->children.push_back(nullptr);
			if(!r_member_initializers(root->children.back()))
				return free_tree(root);
		}
		if(LOOK_AHEAD(0)==CT_ASSIGN)
		{
			ADVANCE;
			auto t=LOOK_AHEAD(0);
			switch(t)
			{
			case CT_DEFAULT:
			case CT_DELETE://TODO: check for other keywords
				root->children.push_back(new IRNode(t, CT_IGNORED));
				break;
			default:
				return free_tree(root);
			}
		}
		return true;
	}
//declaration
//  : integral.declaration
//  | const.declaration
//  | other.declaration
//
//decl.head
//  : {member.spec} {storage.spec} {member.spec} {cv.qualify}
//
//integral.declaration
//  : integral.decl.head declarators (';' | function.body)
//  | integral.decl.head ';'
//  | integral.decl.head ':' expression ';'
//
//integral.decl.head
//  : decl.head integral.type.or.class.spec {cv.qualify}
//
//other.declaration
//  : decl.head name {cv.qualify} declarators (';' | function.body)
//  | decl.head name constructor.decl (';' | function.body)
//  | FRIEND name ';'
//
//const.declaration
//  : cv.qualify {'*'} Identifier '=' expression {',' declarators} ';'
	bool		r_declaration(IRNode *&root)
	{
		INSERT_LEAF(root, PT_DECLARATION, nullptr);//children: {???}		TODO: organize structure
		//root->children.assign(2, nullptr);
		
		StorageTypeSpecifier esmr_flag=STORAGE_EXTERN;//TODO: storage specifier
		int fvi_flag=0;
		if(!r_opt_member_spec(fvi_flag)||!r_opt_storage_spec(esmr_flag)||!r_opt_member_spec(fvi_flag))
			return free_tree(root);

		//if(!root->children[0])
		//{
		//	root->children.push_back(nullptr);
		//	if(!r_opt_member_spec(root->children.back()))
		//		return false;
		//}
		
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		root->children.push_back(nullptr);
		if(!r_opt_int_type_or_class_spec(root->children.back()))
			return free_tree(root);

		if(root->children.back())
		{
			//root->children.push_back(nullptr);
			//if(!r_int_declaration(root->children.back()))//got inlined
			//	return free_tree(root);

			//r_int_declaration() inlined
			int cv_flag=0;
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);

			switch(LOOK_AHEAD(0))
			{
			case CT_SEMICOLON:
				ADVANCE;
				break;
			case CT_COLON://bit field
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_assign_expr(root->children.back())||LOOK_AHEAD(0)!=CT_SEMICOLON)
					return free_tree(root);
				ADVANCE;
				break;
			default:
				if(!r_declarators_nonnull(root, true, false))
				//root->children.push_back(nullptr);
				//if(!r_declarators_nonnull(root->children.back(), true, false))
					return free_tree(root);
				if(LOOK_AHEAD(0)==CT_SEMICOLON)
					ADVANCE;
				else
				{
					root->children.push_back(nullptr);
					if(!r_compoundstatement(root->children.back()))
						return free_tree(root);
				}
				break;
			}
			return true;
		}

		auto t=LOOK_AHEAD(0);
		root->children.push_back(nullptr);
		if(*(root->children.end()-3)&&(t==CT_ID&&LOOK_AHEAD(1)==CT_ASSIGN||t==CT_ASTERIX))
			return r_const_decl(root->children.back());
		//return r_other_decl(root->children.back());//inlined below

		auto r2=root->children.back();

		INSERT_LEAF(r2, PT_VAR_DECL, nullptr);

		r2->children.push_back(nullptr);
		if(!r_name(r2->children.back()))
			return free_tree(root);

		if(!*(root->children.end()-3)&&is_constructor_or_decl())
		{
			r2->children.push_back(nullptr);
			if(!r_constructor_decl(r2->children.back()))
				return free_tree(root);
		}
		else if(root->children[0]&&LOOK_AHEAD(0)==CT_SEMICOLON)
		{
			//friend name ;
			if(root->children[0]->type==CT_FRIEND)
			{
				ADVANCE;
				return true;
			}
			return free_tree(root);
		}
		else
		{
			int cv_flag=0;
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);

			if(!r_declarators_nonnull(r2, false, false))
			//r2->children.push_back(nullptr);
			//if(!r_declarators(r2->children.back(), false, false))
				return free_tree(root);
		}

		if(LOOK_AHEAD(0)==CT_SEMICOLON)
			ADVANCE;
		else
		{
			r2->children.push_back(nullptr);
			if(!r_compoundstatement(r2->children.back()))
				return free_tree(root);
		}
		return true;
	}

	//temp.arg.declaration
	//	:	CLASS Identifier ['=' type.name]
	//	|	type.specifier arg.declarator ['=' additive.expr]
	//	|	template.decl2 CLASS Identifier ['=' type.name]
	bool		r_template_arg_decl(IRNode *&root)
	{
		auto t=LOOK_AHEAD(0);
		if(t==CT_CLASS&&LOOK_AHEAD(1)==CT_ID)
		{
			INSERT_LEAF(root, PT_TEMPLATE_ARG, LOOK_AHEAD_TOKEN(1).sdata);
			ADVANCE_BY(2);
			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_typename(root->children.back()))
					return free_tree(root);
			}
		}
		else if(t==CT_TEMPLATE)
		{
			ADVANCE;
			auto templatetype=TEMPLATE_UNKNOWN;
			if(!r_template_decl2(root, templatetype))
				return false;
			t=LOOK_AHEAD(0);
			if(t!=CT_CLASS||LOOK_AHEAD(1)!=CT_ID)
				return free_tree(root);
			ADVANCE_BY(1+(t==CT_CLASS));
			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_typename(root->children.back()))
					return free_tree(root);
			}
		}
		else
		{
			if(!r_typespecifier(root, true))
				return false;
			root->children.push_back(nullptr);
			if(!r_declarator2(root->children.back(), DECLKIND_ARG, false, true, false))
				return free_tree(root);
			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_additive(root->children.back()))
					return free_tree(root);
			}
		}
		return true;
	}
	bool		r_template_arglist(IRNode *&root)//temp.arg.list  :=  empty  |  temp.arg.declaration (',' temp.arg.declaration)*
	{
		auto t=LOOK_AHEAD(0);
		if(t==CT_GREATER||t==CT_SHIFT_RIGHT)//TODO: absorb one chevron in case of >>, or leave a flag
			return true;
		INSERT_LEAF(root, PT_TEMPLATE_ARG_LIST, nullptr);
		for(;;)
		{
			root->children.push_back(nullptr);
			if(!r_template_arg_decl(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
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
	bool		r_template_decl2(IRNode *&root, TemplateDeclarationType &templatetype)
	{
		if(LOOK_AHEAD(0)!=CT_TEMPLATE)
			return false;
		ADVANCE;
		INSERT_LEAF(root, CT_TEMPLATE, nullptr);
		if(LOOK_AHEAD(0)!=CT_LESS)//template instantiation
		{
			templatetype=TEMPLATE_INSTANTIATION;
			return true;
		}
		ADVANCE;

		root->children.push_back(nullptr);
		if(!r_template_arglist(root->children.back()))
			return free_tree(root);
		auto &t=LOOK_AHEAD(0);				//TODO: don't modify the source code
		switch(t)
		{
		case CT_LESS:
			break;
		case CT_SHIFT_RIGHT:
			t=CT_LESS;
			break;
		default:
			return free_tree(root);
		}
		while(LOOK_AHEAD(0)==CT_TEMPLATE)//ignore nested template	TODO: support nested templates
		{
			ADVANCE;
			if(LOOK_AHEAD(0)!=CT_LESS)
				break;
			ADVANCE;
			root->children.push_back(nullptr);
			if(!r_template_arglist(root->children.back()))
				return free_tree(root);
			auto t2=LOOK_AHEAD(0);
			if(t2!=CT_GREATER&&t2!=CT_SHIFT_RIGHT)
			{
				ADVANCE;
				return free_tree(root);
			}
		}
		if(root->children[0])
			templatetype=TEMPLATE_SPECIALIZATION;//template < > declaration
		else
			templatetype=TEMPLATE_DECLARATION;//template < ... > declaration
		return true;
	}
//template.decl
//  : TEMPLATE '<' temp.arg.list '>' declaration
//  | TEMPLATE declaration
//  | TEMPLATE '<' '>' declaration
	bool		r_template_decl(IRNode *&root)
	{
		auto templatetype=TEMPLATE_UNKNOWN;
		if(!r_template_decl2(root, templatetype))
			return false;

		root->children.push_back(nullptr);
		if(!r_declaration(root->children.back()))
			return free_tree(root);
		//switch(templatetype)
		//{
		//case TEMPLATE_INSTANTIATION:
		//	//TODO
		//	break;
		//case TEMPLATE_SPECIALIZATION:
		//case TEMPLATE_DECLARATION:
		//	//TODO
		//	break;
		//default:
		//	//error_pp(templatetoken, "Unknown template");
		//	break;//recover
		//}
		return true;
	}
	bool		r_extern_template_decl(IRNode *&root)//extern.template.decl  :=  EXTERN TEMPLATE declaration
	{
		if(LOOK_AHEAD(0)!=CT_EXTERN||LOOK_AHEAD(1)!=CT_TEMPLATE)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, PT_EXTERN_TEMPLATE, nullptr);

		root->children.push_back(nullptr);
		return r_declaration(root->children.back());
	}

	bool		r_linkage_body(IRNode *&root)//linkage.body  :=  '{' (definition)* '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return false;
		ADVANCE;

		INSERT_LEAF(root, PT_LINKAGE_BODY, nullptr);

		while(LOOK_AHEAD(0)!=CT_RBRACE)
		{
			root->children.push_back(nullptr);
			if(!r_definition(root->children.back()))
			{
				skip_till_after(CT_RBRACE);
				return false;
			}
		}
		ADVANCE;
		return true;
	}
//namespace.spec
//  :  NAMESPACE Identifier definition
//  |  NAMESPACE { Identifier } linkage.body
	bool		r_namespace_spec(IRNode *&root)
	{
		if(LOOK_AHEAD(0)!=CT_NAMESPACE)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_NAMESPACE, nullptr);//children: {definition}, sdata=name
		root->children.push_back(nullptr);

		auto t=&LOOK_AHEAD_TOKEN(0);
		if(t->type==CT_ID)
		{
			ADVANCE;
			root->sdata=t->sdata;
			t=&LOOK_AHEAD_TOKEN(0);
		}
		if(t->type!=CT_LBRACE)
			return free_tree(root);
		return r_linkage_body(root);
	}
	bool		r_namespace_alias(IRNode *&root)//namespace.alias  :=  NAMESPACE Identifier '=' Identifier ';'
	{
		auto t=&LOOK_AHEAD_TOKEN(1);
		if(LOOK_AHEAD(0)!=CT_NAMESPACE||t->type!=CT_ID||LOOK_AHEAD(2)!=CT_ASSIGN)
			return false;
		ADVANCE_BY(3);

		INSERT_LEAF(root, PT_NAMESPACE_ALIAS, t->sdata);//children: {newname, name[0]::...::name[n-1]}
		if(LOOK_AHEAD(0)==CT_SCOPE)
		{
			ADVANCE;
			root->children.push_back(new IRNode(CT_SCOPE, CT_IGNORED));
		}

		for(;;)
		{
			t=&LOOK_AHEAD_TOKEN(0);
			if(t->type!=CT_ID)
				return free_tree(root);
			ADVANCE;
			root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));
			if(LOOK_AHEAD(0)!=CT_SCOPE)
				break;
			ADVANCE;
		}
		
		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);

		return true;
	}
	bool		r_using(IRNode *&root)//using.declaration : USING { NAMESPACE } name ';'
	{
		if(LOOK_AHEAD(0)!=CT_USING)
			return false;
		INSERT_LEAF(root, CT_USING, nullptr);//children: {[namespace], name}
		root->children.assign_pod(2, nullptr);
		if(LOOK_AHEAD(0)==CT_NAMESPACE)
		{
			ADVANCE;
			root->children[0]=new IRNode(CT_NAMESPACE, CT_IGNORED);
		}
		if(!r_name(root->children[1])||LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;
		return true;
	}

//linkage.spec
//  :  EXTERN StringL definition
//  |  EXTERN StringL linkage.body
	bool		r_linkage_spec(IRNode *&root)
	{
		auto t=&LOOK_AHEAD_TOKEN(1);
		if(LOOK_AHEAD(0)!=CT_EXTERN||t->type!=CT_VAL_STRING_LITERAL)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, CT_EXTERN, t->sdata);//children: {definition/linkage_body}
		root->children.push_back(nullptr);

		if(LOOK_AHEAD(0)==CT_LBRACE)
		{
			if(!r_linkage_body(root->children.back()))
				return free_tree(root);
		}
		else if(r_definition(root->children.back()))
			return free_tree(root);
		return true;
	}
//metaclass.decl  :=  METACLASS Identifier {{':'} Identifier {'(' meta.arguments ')'}} ';'
//
//  OpenC++ allows two kinds of syntax:
//
//  metaclass <metaclass> <class>(...);
//  metaclass <metaclass>;
//  metaclass <class> : <metaclass>(...);		// for backward compatibility		TODO: remove this line
	bool		r_metaclass_decl(IRNode *&root)
	{
		auto t=LOOK_AHEAD(0);
		if(t!=CT_ENUM&&t!=CT_STRUCT&&t!=CT_CLASS&&t!=CT_UNION)
			return false;
		auto t2=&LOOK_AHEAD_TOKEN(1);
		if(t2->type!=CT_ID)
			return false;
		ADVANCE_BY(2);

		INSERT_LEAF(root, PT_DECLARATION, nullptr);//children: {???}
		root->children.assign_pod(2);

		root->children[0]=new IRNode(t, CT_IGNORED);
		root->children[1]=new IRNode(CT_ID, CT_IGNORED, t2->sdata);
		t2=&LOOK_AHEAD_TOKEN(0);
		switch(t2->type)
		{
		case CT_ID:
			ADVANCE;
			root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t2->sdata));
			break;
		case CT_COLON:
			ADVANCE;
			root->children.push_back(new IRNode(CT_COLON, CT_IGNORED));

			t2=&LOOK_AHEAD_TOKEN(0);
			if(t2->type!=CT_ID)
				return free_tree(root);
			ADVANCE;
			root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t2->sdata));
			break;
		case CT_SEMICOLON:
			break;
		default:
			return free_tree(root);
		}
		return true;
	}
	bool		r_definition(IRNode *&root)
	{
		switch(LOOK_AHEAD(0))
		{
		case CT_SEMICOLON://null declaration
			ADVANCE;
			break;

		case CT_TYPEDEF:
			return r_typedef(root);

		case CT_TEMPLATE:
			return r_template_decl(root);

		case CT_ENUM:
		case CT_STRUCT:
		case CT_CLASS:
		case CT_UNION:
			return r_metaclass_decl(root);

		case CT_EXTERN:
			switch(LOOK_AHEAD(1))
			{
			case CT_VAL_STRING_LITERAL:
				return r_linkage_spec(root);
			case CT_TEMPLATE:
				return r_extern_template_decl(root);

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
				return r_declaration(root);
			default:
				parse_error("Expected an extern declaration");
				return false;
			}
			break;

		case CT_NAMESPACE:
			if(LOOK_AHEAD(1)==CT_ASSIGN)
				return r_namespace_alias(root);
			return r_namespace_spec(root);

		case CT_USING:
			return r_using(root);

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
#ifdef DEBUG_PARSER
			{
				bool res=r_declaration(root);
				debugprint(root);//
				return res;
			}
#else
			return r_declaration(root);
#endif

		default:
			parse_error("Expected a declaration");
			return false;
		}
		return true;
	}
#undef			LOOK_AHEAD
}
void			parse_cplusplus(Expression &ex, IRNode *&root)//OpenC++
{
	Token nulltoken={CT_IGNORED};
	ex.insert_pod(ex.size(), nulltoken, 16);
	parse::current_ex=&ex;
	parse::ntokens=ex.size()-16;
	parse::current_idx=0;
	parse::ir_size=0;

	INSERT_LEAF(root, PT_PROGRAM, nullptr);//node at 0 is always CT_PROGRAM, everything stems from there
	//root->children.push_back(nullptr);
	//parse::r_definition(root->children.back());
	for(;parse::current_idx<parse::ntokens;)
	{
		root->children.push_back(nullptr);
		for(;parse::current_idx<parse::ntokens&&!parse::r_definition(root->children.back());)
			parse::skip_till_after(CT_SEMICOLON);
	}
}

void			dump_code(const char *filename){save_file(filename, true, code.data(), code.size());}

struct			TestNode
{
	CTokenType type, opsign;
	TestNode *parent, *child, *next;
	union
	{
		TypeInfo *tdata;//type & function
		//VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long *idata;
		double *fdata;
	};
	TestNode():type(CT_IGNORED), opsign(CT_IGNORED), parent(nullptr), child(nullptr), next(nullptr), tdata(nullptr){}
};
struct			TestNode2
{
	CTokenType type, opsign;
	TestNode2 *parent;
	std::vector<TestNode2*> children;
	union
	{
		TypeInfo *tdata;//type & function
		//VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long *idata;
		double *fdata;
	};
	TestNode2():type(CT_IGNORED), opsign(CT_IGNORED), parent(nullptr), tdata(nullptr){}
};
void			benchmark()
{
	const int testsize=1024*1024;
	
	prof.add("bm start");
#if 0
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
#endif

	{
		auto tree=new TestNode, node=tree;
		for(int k=0;k<testsize;++k)
		{
			node->next=new TestNode;
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

	{
		auto tree=new TestNode2, node=tree;
		for(int k=0;k<testsize;++k)
		{
			node->children.push_back(new TestNode2);
			node=node->children[0];
		}
		prof.add("build");

		node=tree;
		TestNode2 *n2=nullptr;
		for(int k=0;k<testsize;++k)
		{
			n2=node->children[0];
			delete node;
			node=n2;
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
#if 1
	prof.add("entry");
	{
		int kd=0;
		for(int ks=0;ks<(int)lf.expr.size();)
		{
			switch(lf.expr[ks].type)
			{
			case CT_IGNORED:
			case CT_NEWLINE:
				++ks;
				break;
			default:
				lf.expr[kd]=lf.expr[ks];
				++ks, ++kd;
				break;
			}
		}
		lf.expr.resize(kd);
	}
	scope_init();
	prof.add("prepare");

	IRNode *root=nullptr;
	parse_cplusplus(lf.expr, root);
	prof.add("parse");

	std::string str;
	AST2str(root, str);
	prof.add("tree2str");

	save_file("D:/C/ACC2/ast.txt", false, str.c_str(), str.size());

	parse::free_tree(root);
	prof.add("free tree");

	prof.print();
	pause();
#endif
#if 0
	i64 t1=0, t2=0;

	t1=__rdtsc();

	current_expr=&lf.expr, token_idx=0;

	code_idx=0;
	code.resize(1024);
	
	//emit_msdosstub();//

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
#endif
	exit(0);//
}