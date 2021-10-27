#include		"acc2.h"
#include		"include/tree.hpp"
#include		"acc2_codegen_x86_64.h"
#include		"include/intrin.h"

//parser forked from OpenC++		http://opencxx.sourceforge.net/

	#define		DEBUG_PARSER

const char*		tokentype2str(CTokenType tokentype)
{
	const char *a=nullptr;
	switch(tokentype)
	{
#define TOKEN(STRING, LABEL, FLAGS)	case LABEL:a=STRING;break;
#include"acc2_keywords_c.h"
#undef TOKEN
	}
	if(a)
		return a;

	switch(tokentype)
	{
#define TOKEN(STRING, LABEL, FLAGS)	case LABEL:return #LABEL;
#include"acc2_keywords_c.h"
#undef TOKEN
	}
	return "<UNDEFINED>";
}
void			token2buf(Token const &t)
{
	switch(t.type)
	{
	case CT_VAL_INTEGER:
	case CT_VAL_CHAR_LITERAL://
		sprintf_s(g_buf, g_buf_size, "%lld", t.idata);
		break;
	case CT_VAL_FLOAT:
		sprintf_s(g_buf, g_buf_size, "%g", t.fdata);
		break;
	case CT_ID:
		if(t.sdata)
			sprintf_s(g_buf, g_buf_size, "%s", t.sdata);
		else
			sprintf_s(g_buf, g_buf_size, "%s:nullptr", tokentype2str(t.type));
		break;
	case CT_VAL_STRING_LITERAL:
	case CT_VAL_WSTRING_LITERAL://
		if(t.sdata)
			sprintf_s(g_buf, g_buf_size, "\"%s\"", t.sdata);
		else
			sprintf_s(g_buf, g_buf_size, "%s:nullptr", tokentype2str(t.type));
		break;
	case CT_INCLUDENAME_STD:
		if(t.sdata)
			sprintf_s(g_buf, g_buf_size, "<%s>", t.sdata);
		else
			sprintf_s(g_buf, g_buf_size, "%s:nullptr", tokentype2str(t.type));
		break;
	default:
		sprintf_s(g_buf, g_buf_size, "%s", tokentype2str(t.type));
		break;
	}
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

//CODEGEN

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
struct			Pe32OptionalHeader// 1 byte aligned
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


//PARSER

enum			StorageTypeSpecifier
{
	STORAGE_EXTERN,
	STORAGE_STATIC,
	STORAGE_MUTABLE,
	STORAGE_REGISTER,
	STORAGE_UNSPECIFIED,
};
enum			DataType
{
	TYPE_UNASSIGNED,
	TYPE_AUTO,//TODO: process auto & remove this flag
	TYPE_VOID,
	TYPE_BOOL,
	TYPE_CHAR,
	TYPE_INT,
	TYPE_INT_SIGNED,
	TYPE_INT_UNSIGNED,
	TYPE_FLOAT,
	TYPE_ENUM,
	TYPE_ENUM_CONST,
	TYPE_CLASS,
	TYPE_STRUCT,
	TYPE_UNION,
	TYPE_SIMD,//special struct
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_REFERENCE,
	TYPE_FUNC,
	TYPE_ELLIPSIS,
	TYPE_NAMESPACE,
};
enum			CallType
{
	CALL_CDECL,
	CALL_STDCALL,
	CALL_THISCALL,
};
enum			AccessType
{
	ACCESS_PUBLIC,
	ACCESS_PRIVATE,
	ACCESS_PROTECTED,
};
struct			IRNode;
struct			TypeInfo//28 bytes		TODO: bitfields
{
	union
	{
		struct
		{
			unsigned short//0SSF RIVC AAAD DDDD
				datatype:5,//master attribute
			//	is_unsigned:1,//X  only for int types
				logalign:3,//in bytes
				is_const:1, is_volatile:1,
				is_inline:1, is_virtual:1, is_friend:1,
				storagetype:2;//one of: extern/static/mutable/register

			unsigned short//0000 0000 0000 AACC
				calltype:2,
				accesstype:2;
		};
		int flags;
	};
	size_t size;//in bytes			what about bitfields?

	//char *name;				//X  primitive types only, a type can be aliased				//names must be qualified

	//links:
	IRNode *body;//for class/struct/union, enum or function

	//class/struct/union: args points at member types, can include padding as void of nonzero size
	//array/pointers: only one element pointing at pointed type
	//array: 'size' is the count of pointed type
	//functions: 1st element is points at return type
	std::vector<TypeInfo*> args;

	std::vector<IRNode*> template_args;

	TypeInfo():flags(0), size(0), body(nullptr){}
	TypeInfo(TypeInfo const &other):flags(other.flags), size(other.size), body(other.body), args(other.args), template_args(other.template_args){}
	TypeInfo(TypeInfo &&other):flags(other.flags), size(other.size), body(other.body), args((std::vector<TypeInfo*>&&)other.args), template_args((std::vector<IRNode*>&&)other.template_args)
	{
		other.flags=0;
		other.size=0;
		other.body=nullptr;
	}
	TypeInfo& operator=(TypeInfo const &other)
	{
		if(this!=&other)
		{
			flags=other.flags;
			size=other.size;
			body=other.body;
			args=other.args;
			template_args=other.template_args;
		}
		return *this;
	}
	TypeInfo& operator=(TypeInfo &&other)
	{
		if(this!=&other)
		{
			flags=other.flags, other.flags=0;
			size=other.size, other.size=0;
			body=other.body, other.body=nullptr;
			args=(std::vector<TypeInfo*>&&)other.args;
			template_args=(std::vector<IRNode*>&&)other.template_args;
		}
		return *this;
	}
	TypeInfo(char datatype, char logalign, char cv_flag, char fvi_flag, char storagetype, char calltype, char accesstype, int size, IRNode *body=nullptr):flags(0), size(size)
	{
		this->datatype=datatype;
		this->logalign=logalign;
		this->is_const=cv_flag&1, this->is_volatile=cv_flag>>1&1;
		this->is_inline=fvi_flag&1, this->is_virtual=fvi_flag>>1&1, this->is_friend=fvi_flag>>2&1;
		this->storagetype=storagetype;

		this->calltype=calltype;
		this->accesstype=accesstype;

		this->size=size;

		this->body=body;
	}
	void set_flags(byte esmr_flag, byte fvi_flag, byte cv_flag)//missing on purpose: datatype, logalign, calltype, accesstype		set them yourself
	{
		if(esmr_flag!=STORAGE_UNSPECIFIED)
			storagetype=esmr_flag;
		is_inline|=fvi_flag&1, is_virtual|=fvi_flag>>1&1, is_friend|=fvi_flag>>2&1;
		is_const|=cv_flag&1, is_volatile|=cv_flag>>1&1;
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
TypeInfo*		add_type(TypeInfo const &type)
{
	auto t2=new TypeInfo(type);
	bool old=false;
	auto t3=typedb.insert_no_overwrite(t2, &old);
	if(old)
		delete t2;
	return t3;
}

typedef unsigned long long u64;
union			VarData
{
	long long idata;
	double fdata;
	float f32data;
	char *data;
	VarData():idata(0){}
	VarData(u64 idata):idata(idata){}
};
struct			NameInfo
{
	TypeInfo *tdata;
	bool is_var;
	VarData vdata;
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
void			scope_declare_entity(char *name, TypeInfo *ptype, bool is_var)
{
	NameInfo info={ptype, is_var, 0};
	scope_global.insert(name, info, true);
}
void			scope_declare_type(char *name, TypeInfo *ptype)//types, functions
{
	NameInfo info={ptype, false, 0};
	scope_global.insert(name, info, true);
}
void			scope_declare_var(char *name, TypeInfo *ptype, long long data)//variables
{
	NameInfo info={ptype, true, VarData(data)};
	scope_global.insert(name, info, true);
}
Name::Node*		scope_lookup(char *id, bool global)
{
#ifdef DEBUG_PARSER
	{
		auto str=find_string("std");
		auto scope=scope_global.find_root(str);
		if(!scope)
			printf("\nscope_lookup(): \'std\' not found in global scope\n");
	}
#endif
	if(global)
		return scope_global.find_root(id);
	auto p=scope_global.find(id);
	if(!p&&scope_global.path.size())
		return scope_global.find_root(id);
	return p;
}

void			print_type(TypeInfo const &type)
{
	int printed=0;
	switch(type.datatype)
	{
#define CASE(TYPE)	case TYPE:printed+=sprintf_s(g_buf+printed, g_buf_size-printed, #TYPE);break;
	CASE(TYPE_UNASSIGNED)
	CASE(TYPE_AUTO)
	CASE(TYPE_VOID)
	CASE(TYPE_BOOL)
	CASE(TYPE_CHAR)
	CASE(TYPE_INT)
	CASE(TYPE_INT_SIGNED)
	CASE(TYPE_INT_UNSIGNED)
	CASE(TYPE_FLOAT)
	CASE(TYPE_ENUM)
	CASE(TYPE_ENUM_CONST)
	CASE(TYPE_CLASS)
	CASE(TYPE_STRUCT)
	CASE(TYPE_UNION)
	CASE(TYPE_SIMD)//special struct
	CASE(TYPE_ARRAY)
	CASE(TYPE_POINTER)
	CASE(TYPE_REFERENCE)
	CASE(TYPE_FUNC)
	CASE(TYPE_ELLIPSIS)
	CASE(TYPE_NAMESPACE)
#undef	CASE
	}
	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "\t S%d A%d %c%c%c%c%c ", type.size, 1<<type.logalign, type.is_const?'C':'-', type.is_volatile?'O':'-', type.is_inline?'I':'-', type.is_virtual?'V':'-', type.is_friend?'F':'-');
	switch(type.storagetype)
	{
	case STORAGE_EXTERN:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "extern");	break;
	case STORAGE_STATIC:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "static");	break;
	case STORAGE_MUTABLE:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "mutable");	break;
	case STORAGE_REGISTER:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "register");	break;
	}
	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, " ");
	switch(type.calltype)
	{
	case CALL_CDECL:		printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "cdecl");		break;
	case CALL_STDCALL:		printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "stdcall");	break;
	case CALL_THISCALL:		printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "thiscall");	break;
	}
	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, " ");
	switch(type.accesstype)
	{
	case ACCESS_PUBLIC:		printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "public");	break;
	case ACCESS_PRIVATE:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "private");	break;
	case ACCESS_PROTECTED:	printed+=sprintf_s(g_buf+printed, g_buf_size-printed, "protected");	break;
	}
}
void			print_var(TypeInfo *ptype, VarData vdata)
{
	if(!ptype)
		printf(" %lld", vdata.idata);
	else
	{
		switch(ptype->datatype)
		{
		case TYPE_UNASSIGNED:
		case TYPE_AUTO:
		case TYPE_VOID:
			break;
		case TYPE_BOOL:
			if(vdata.idata)
				printf(" true");
			else
				printf(" false");
			break;
		case TYPE_CHAR:
			if(ptype->size==1)
				printf(" \'%c\'", (char)vdata.idata);
			else
				printf(" \'\\u%02X\'", (wchar_t)vdata.idata);
			break;
		case TYPE_INT:
		case TYPE_INT_SIGNED:
		case TYPE_INT_UNSIGNED:
			printf(" %lld", vdata.idata);
			break;
		case TYPE_FLOAT:
			if(ptype->size==8)
				printf(" %g", vdata.fdata);
			else
				printf(" %g", (double)vdata.f32data);
			break;
		case TYPE_ENUM:
		case TYPE_ENUM_CONST:
			printf(" %lld", vdata.idata);
			break;
		case TYPE_CLASS:
		case TYPE_STRUCT:
		case TYPE_UNION:
			if(vdata.data)
			{
				for(size_t k=0;k<ptype->size;++k)
				{
					printf(k?"-":" ");
					printf("%02X", vdata.data[k]);
				}
			}
			else
				printf(" <nullptr>");
			break;
		case TYPE_SIMD://special struct
			//TODO
			break;
		case TYPE_ARRAY:
			for(size_t k=0, offset=0;k<ptype->size;++k, offset+=ptype->args[0]->size)
				print_var(ptype->args[0], VarData((u64)(vdata.data+offset)));
			break;
		case TYPE_POINTER:
			printf(" 0x%p", vdata.data);
			break;
		case TYPE_REFERENCE:
			print_var(ptype->args[0], vdata);
			break;
		case TYPE_FUNC://unreachable
		case TYPE_ELLIPSIS:
		case TYPE_NAMESPACE:
			printf(" <UNREACHABLE>");
			break;
		}
	}
}
struct			PrintName
{
	void operator()(char *name, NameInfo const &data, size_t depth)
	{
		//printf("%4d %*c ", depth, depth, '|');//X  padded with spaces
		printf("%4d ", depth);
		for(size_t k=0;k<depth;++k)
			printf("|");
		printf(" ");
		if(data.tdata)
		{
			print_type(*data.tdata);
			printf("%s", g_buf);
		}
		if(name)
			printf(" %s", name);
		else
			printf(" <nullptr>");
		if(data.is_var)
			print_var(data.tdata, data.vdata);
		//if(data.tdata&&data.tdata->datatype==TYPE_ENUM_CONST)
		//	printf(" %d", data.idata);
		printf("\n");
	}
};
void			print_names()
{
	printf("names:\n");
	scope_global.transform_depth_first(PrintName());
	printf("\n");
}

struct			IRNode
{
	CTokenType type;
	union
	{
		struct
		{
			unsigned int//0000 0000 0000 0000  0000 000V IFVC SSAA
				flag_accesstype:2,//ACCESS_PUBLIC/PRIVATE/PROTECTED
				flag_esmr:2,//storage specifier := extern|static|mutable|register
				flag_const:1, flag_volatile:1,//cv qualifier
				flag_friend:1, flag_inline:1, flag_virtual:1;//member specifier
		};
		int flags;
		CTokenType opsign;
	};
	std::vector<IRNode*> children;
	union
	{
		TypeInfo *tdata;//type & function
	//	VarInfo *vdata;//variable

		//literals
		char *sdata;
		long long idata;
		double fdata;
	};
	IRNode():type(CT_IGNORED), opsign(CT_IGNORED), idata(0){}
	IRNode(CTokenType type, CTokenType opsign, void *data=nullptr):type(type), opsign(opsign), idata((size_t&)data){}
	IRNode(CTokenType type, int opsign, void *data=nullptr):type(type), opsign((CTokenType)opsign), idata((size_t&)data){}
	void set(CTokenType type, CTokenType opsign, void *data=nullptr)
	{
		this->type=type;
		this->opsign=opsign;
		idata=(size_t&)data;
	}
	void set(void *data)
	{
		idata=(size_t&)data;
	}
};
//inline void		tokentype2str(CTokenType t, std::string &str)
//{
//	switch(t)
//	{
//#define		TOKEN(STR, LABEL, FLAG)		case LABEL:str+=#LABEL;break;
//#include	"acc2_keywords_c.h"
//#undef		TOKEN
//	}
//}
void			AST2str(IRNode *root, std::string &str, int depth=0)
{
	//sprintf_s(g_buf, g_buf_size, "%5d %*c ", depth, depth, '|');
	//str+=g_buf;
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
	str+=tokentype2str(root->type);
	//if(root->opsign!=CT_IGNORED)
	//{
	//	str+='\t';
	//	tokentype2str(root->opsign, str);
	//}
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
		sprintf_s(g_buf, g_buf_size, "\t%lld", root->idata);
		str+=g_buf;
		break;
	case CT_VAL_FLOAT:
		sprintf_s(g_buf, g_buf_size, "\t%g", root->fdata);
		str+=g_buf;
		break;
	case PT_TYPE:
		str+='\t';
		if(root->tdata)
		{
			print_type(*root->tdata);
			str+=g_buf;
		}
		else
			str+="NULL_TYPE";
		break;
	case PT_MUL:
	case PT_ADD:
	case PT_SHIFT:
	case PT_RELATIONAL:
	case PT_EQUALITY:
	case PT_BITAND:
	case PT_BITXOR:
	case PT_BITOR:
	case PT_LOGICAND:
	case PT_LOGICOR:
		str+='\t';
		str+=tokentype2str(root->opsign);
		break;
	case PT_ENUM_CONST:
		sprintf_s(g_buf, g_buf_size, "\t%s\t%d", root->sdata, root->flags);
		str+=g_buf;
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
	printf("tree:\n%s\n", str.c_str());
}
//namespace		parse//OpenC++		http://opencxx.sourceforge.net/
//{
	Expression	*current_ex=nullptr;
	int			current_idx=0, ntokens=0, error_reporting=true;
#define			LOOK_AHEAD_TOKEN(K)		current_ex->at(current_idx+(K))
//#define		LOOK_AHEAD_TOKEN(K)		current_ex->operator[](current_idx+(K))
#define			LOOK_AHEAD(K)			LOOK_AHEAD_TOKEN(K).type
#define			ADVANCE					++current_idx
#define			ADVANCE_BY(K)			current_idx+=K
//#define		CHECK_STMT				if(current_idx>=ntokens)return false;
//#define		CHECK_STMT_ROOT(ROOT)	if(current_idx>=ntokens)return free_tree(ROOT);
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
	void		free_tree_not_an_error(IRNode *&root)//TODO: use only free_tree, because it will be used withoout error anyway, at deeper calls
	{
		if(root)
		{
			for(int k=0;k<(int)root->children.size();++k)
				free_tree_not_an_error(root->children[k]);
			delete root;
			root=nullptr;
		}
	}
	bool		scope_error(IRNode *&root)
	{
		skip_till_after(CT_RBRACE);
		scope_exit();
		return free_tree(root);
	}
#define			INSERT_NONLEAF(ROOT, TEMP, TYPE)	TEMP=ROOT, ROOT=new IRNode(TYPE, CT_IGNORED), ROOT->children.push_back(TEMP);
#define			INSERT_LEAF(ROOT, TYPE, DATA)		ROOT=new IRNode(TYPE, CT_IGNORED, DATA)

	void		print_neighborhood()
	{
		int nbsize=100;
		int k=current_idx-(nbsize>>1), end=current_idx+(nbsize>>1);
		if(k<0)
			k=0;
		if(end>=ntokens)
			end=ntokens;
		printf("\n\nToken debug print (cursor marked with \'@\'):\n");
		for(;k<end;++k)
		{
			auto &t=current_ex->at(k);
			token2buf(t);
			if(k==current_idx)
				printf("@@@%s@@@ ", g_buf);
			else
				printf("%s ", g_buf);
			if(t.type==CT_SEMICOLON)
				printf("\n");
		}
		printf("\n\n");
	}

	//TODO: error reporting

	//TODO: only top-level functions can call free_tree()

	//recursive parser declarations
	enum		DeclaratorKind
	{
		DECLKIND_NORMAL,
		DECLKIND_ARG,
		DECLKIND_CAST,
	};
	bool		r_declarator2(IRNode *&root, TypeInfo type, int kind, bool recursive, bool should_be_declarator, bool is_statement, bool is_var);//initialized type is passed by value
	bool		r_opt_int_type_or_class_spec(IRNode *&root, TypeInfo &type);
	bool		r_cast(IRNode *&root);
	bool		r_unary(IRNode *&root);
	bool		r_assign_expr(IRNode *&root);
	bool		r_comma_expr(IRNode *&root);

	bool		r_typedef(IRNode *&root);

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
	bool		r_declarators_nonnull(IRNode *&root, TypeInfo &type, bool should_be_declarator, bool is_statement, bool is_var);
	bool		r_using(IRNode *&root);
	bool		r_metaclass_decl(IRNode *&root);
	bool		r_declaration(IRNode *&root);
	bool		r_definition(IRNode *&root);

	//recursive parser functions
	bool		is_template_args()//template.args  :=  '<' any* '>' ('(' | '::')		can be inlined but no need
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
	bool		moreVarName()//can be inlined but no need
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
	
	bool		r_opt_member_spec(int &fvi_flag)//member.spec := (friend|inline|virtual)+		bitfield: {bit2: friend, bit1: virtual, bit0: inline}
	{
		for(auto t=LOOK_AHEAD(0);t==CT_INLINE||t==CT_VIRTUAL||t==CT_FRIEND;)
		{
			ADVANCE;
			fvi_flag|=int(t==CT_FRIEND)<<2|int(t==CT_VIRTUAL)<<1|int(t==CT_INLINE);
			t=LOOK_AHEAD(0);
		}
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
	}
//name : {'::'} name2 ('::' name2)*
//
//name2
//  : Identifier {template.args}
//  | '~' Identifier
//  | OPERATOR operator.name {template.args}
//  | decltype '(' assign_expr ')'			//C++11
	bool		r_name_lookup(IRNode *&root)//TODO: use the lookup system here, and store the referenced namespace/type/object/function/method(s)
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
			if(t->type==CT_DECLTYPE)//C++11
			{
				if(LOOK_AHEAD(1)!=CT_LPR)
					return free_tree(root);
				ADVANCE_BY(2);

				root->children.push_back(new IRNode(CT_DECLTYPE, CT_IGNORED));

				root->children.push_back(nullptr);
				if(!r_assign_expr(root->children.back()))//TODO: deduce type
					return free_tree(root);

				if(LOOK_AHEAD(0)!=CT_RPR)
					return free_tree(root);
				ADVANCE;

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
			default:
				return free_tree(root);
			}
		}
		return true;
	}
	bool		r_typespecifier(IRNode *&root, bool check)//type.specifier  :=  [cv.qualify]? (integral.type.or.class.spec | name) [cv.qualify]?
	{
		TypeInfo type;
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return false;
		if(!r_opt_int_type_or_class_spec(root, type))
			return false;
		if(root)
		{
			if(!r_opt_cv_qualify(cv_flag))
				return false;
			type.set_flags(STORAGE_UNSPECIFIED, 0, cv_flag);
			root->tdata=add_type(type);
		}
		else
		{
			if(check&&!maybe_typename_or_classtemplate())
				return false;
			if(!r_name_lookup(root))
				return false;
			if(!r_opt_cv_qualify(cv_flag))
				return false;
			//type.set_flags(STORAGE_UNSPECIFIED, 0, cv_flag);//X  type is uninitialized
			//root->tdata=add_type(type);

			//TODO: combine cv_flag with id		requires name system
		}
		return true;
	}
	bool		r_typename(IRNode *&root)//type.name  :=  type.specifier cast.declarator
	{
		if(!r_typespecifier(root, true))
			return false;

		root->children.push_back(nullptr);
		if(!r_declarator2(root->children.back(), *root->tdata, DECLKIND_CAST, false, false, false, false))
			return free_tree(root);

		return true;
	}
	bool		r_func_args(IRNode *&root)//function.arguments  :  empty  |  expression (',' expression)*		assumes that the next token following function.arguments is ')'
	{
		INSERT_LEAF(root, PT_FUNC_ARGS, nullptr);//TODO: less ambiguous parse token name
		if(LOOK_AHEAD(0)==CT_RPR)
			return true;
		for(;;)
		{
			root->children.push_back(nullptr);
			if(!r_assign_expr(root->children.back()))
				return free_tree(root);

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
	bool		r_varname_nonnull(IRNode *&root)
	{
		char global=LOOK_AHEAD(0)==CT_SCOPE;
		ADVANCE_BY(global);
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
				{
					ADVANCE;
					root->children.push_back(new IRNode(CT_SCOPE, CT_IGNORED));//TODO: reduce all this to a lookup
				}
				else
				{
					//TODO: lookup variable reference
					return true;
				}
				break;
			case CT_TILDE://obj.std::vector<int>::~vector();
				ADVANCE;
				t=&LOOK_AHEAD_TOKEN(0);
				if(t->type!=CT_ID||LOOK_AHEAD(1)!=CT_LPR||LOOK_AHEAD(2)!=CT_RPR)
					return free_tree(root);
				ADVANCE_BY(3);

				//TODO: lookup class name
				root->children.push_back(new IRNode(PT_DESTRUCTOR, CT_IGNORED));
				root->children.push_back(new IRNode(CT_ID, CT_IGNORED, t->sdata));
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
	bool		r_opt_ptr_operator_nonnull(IRNode *&root, TypeInfo &type, bool amp_allowed)//ptr.operator  :=  (('*' | '&' | ptr.to.member) {cv.qualify})+		TODO: encode as lookup & remove root arg
	{
		for(;;)
		{
			auto t=LOOK_AHEAD(0);
			if(t==CT_ASTERIX)
			{
				if(!amp_allowed)
					break;
			}
			else if(t!=CT_ASTERIX&&!is_ptr_to_member(0))
				break;

			auto pt=add_type(type);
			type=TypeInfo();
			type.args.push_back(pt);

			if(t==CT_ASTERIX||t==CT_AMPERSAND)
			{
				if(t==CT_ASTERIX&&!amp_allowed)
					return false;
				ADVANCE;
				root->children.push_back(new IRNode(t, CT_IGNORED));
				if(t==CT_ASTERIX)
					type.datatype=TYPE_POINTER;
				else
				{
					if(type.args[0]->datatype==TYPE_REFERENCE)
						return false;
					type.datatype=TYPE_REFERENCE;//TODO: or address?
				}
			}
			else
			{
				//TODO: support "*classname::member_pointer"

				//r_ptr_to_member() inlined				//ptr.to.member  :=  {'::'} (identifier {template.args} '::')+ '*'
				if(LOOK_AHEAD(0)==CT_SCOPE)
				{
					ADVANCE;
					//TODO: search only global scope		really?
				}
				for(;;)
				{
					auto token=&LOOK_AHEAD_TOKEN(0);
					if(token->type!=CT_ID)
						return false;
					ADVANCE;

					root->children.push_back(new IRNode(CT_ID, CT_IGNORED, token->sdata));//
					auto data=scope_lookup(token->sdata, false);

					if(LOOK_AHEAD(0)==CT_LESS)
					{
						root->children.push_back(nullptr);
						if(!r_template_arglist(root->children.back()))//
							return false;
						//TODO: template call lookup
					}
					else
					{
						//TODO: simple name lookup
					}

					if(LOOK_AHEAD(0)!=CT_SCOPE)
						return false;
					ADVANCE;

					if(LOOK_AHEAD(0)==CT_ASTERIX)
					{
						ADVANCE;
						break;
					}
				}
				//end of r_ptr_to_member() inlined
			}
			int cv_flag=0;
			if(!r_opt_cv_qualify(cv_flag))
				return false;
			type.set_flags(STORAGE_UNSPECIFIED, 0, cv_flag);
			(int&)root->children.back()->opsign=cv_flag;//TODO: encode the pointer operator
		}
		return true;
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
//allocate.expr
//  : {Scope | userdef.keyword} NEW allocate.type
//  | {Scope} DELETE {'[' ']'} cast.expr
//
//allocate.expr
//  : {Scope | userdef.keyword} NEW {'(' function.arguments ')'} type.specifier {ptr.operator} ('[' comma.expression ']')+	{allocate.initializer}
//  : {Scope | userdef.keyword} NEW	{'(' function.arguments ')'} '(' type.name ')'											{allocate.initializer}
//  | {Scope}					DELETE {'[' ']'} cast.expr
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
			INSERT_LEAF(root, CT_DELETE, nullptr);

			if(LOOK_AHEAD(1)!=CT_LBRACKET)
				ADVANCE;
			else
			{
				if(LOOK_AHEAD(2)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE_BY(3);

				root->opsign=CT_LBRACKET;
			}

			root->children.push_back(nullptr);
			if(!r_cast(root->children.back()))
				return free_tree(root);

			return true;
		}
		else if(t==CT_NEW)
		{
			ADVANCE;
			INSERT_LEAF(root, CT_NEW, nullptr);

			//r_allocate_type() inlined
//allocate.type
//  : {'(' function.arguments ')'} type.specifier new.declarator	{allocate.initializer}
//  | {'(' function.arguments ')'} '(' type.name ')'				{allocate.initializer}
			if(LOOK_AHEAD(0)==CT_LPR)
			{
				ADVANCE;
				//either func_args (either empty or ass_expr*) or typename
				root->children.push_back(nullptr);
				if(r_typename(root->children.back()))//check if typename or func_args in parens
				{
					if(LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
					ADVANCE;
				}
				else
				{
					if(!r_func_args(root->children.back()))//must be func_args in parens
						return free_tree(root);

					if(LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
					ADVANCE;

					if(LOOK_AHEAD(0)!=CT_LPR)//either    typespecifier new_declarator    or '('typename')'
						goto r_allocate_type_choice1;
					ADVANCE;
				
					root->children.push_back(nullptr);
					if(!r_typename(root->children.back()))//must be typename
						return free_tree(root);

					if(LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
					ADVANCE;
				}
			}
			else
			{
			r_allocate_type_choice1:
				root->children.push_back(nullptr);
				if(!r_typespecifier(root->children.back(), false))
					return free_tree(root);
				TypeInfo type=*root->tdata;

				//r_new_declarator() inlined
//new.declarator
//  : empty
//  | ptr.operator
//  | {ptr.operator} ('[' comma.expression ']')+
				root->children.push_back(new IRNode(PT_NEW_DECLARATOR, CT_IGNORED));
				auto child=root->children.back();
				//INSERT_LEAF(child, PT_NEW_DECLARATOR, nullptr);
				if(!r_opt_ptr_operator_nonnull(child, type, false))
					return free_tree(root);
				while(LOOK_AHEAD(0)==CT_LBRACKET)
				{
					ADVANCE;//skip '['

					child->children.push_back(new IRNode(CT_LBRACKET, CT_IGNORED));
					child->children.back()->children.push_back(nullptr);
					if(!r_comma_expr(child->children.back()->children.back()))
						return free_tree(root);

					if(LOOK_AHEAD(0)!=CT_RBRACKET)
						return free_tree(root);
					ADVANCE;//skip ']'
				}
				//end of r_new_declarator() inlined
			}
			root->children.push_back(nullptr);
			if(!r_allocate_initializer(root->children.back()))
				return free_tree(root);
			if(!root->children.back())
				root->children.pop_back();
			//end of r_allocate_type() inlined
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
		int t_idx=0;
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
			//r_typeid() inlined		//typeid.expr  :=  TYPEID '(' expression ')'  |  TYPEID '(' type.name ')'
			if(LOOK_AHEAD(1)!=CT_LPR)
				return false;
			ADVANCE_BY(2);

			INSERT_LEAF(root, CT_TYPEID, nullptr);

			root->children.push_back(nullptr);

			t_idx=current_idx;//save
			if(r_typename(root->children.back()))
				goto r_typeid_done;

			current_idx=t_idx;//restore
			if(!r_assign_expr(root->children.back()))
				return free_tree(root);

		r_typeid_done:
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;

			//end of r_typeid() inlined
			break;
		default:
			{
				INSERT_LEAF(root, PT_CAST, nullptr);
				root->children.push_back(nullptr);
				TypeInfo type;
				if(!r_opt_int_type_or_class_spec(root->children.back(), type))
					return free_tree(root);
				root->tdata=add_type(type);
				if(root->children.back())//function style cast expression
				{
					if(LOOK_AHEAD(0)!=CT_LPR)
						return free_tree(root);
					root->children.push_back(nullptr);
					if(!r_func_args(root->children.back()))
						return free_tree(root);
					if(LOOK_AHEAD(0)!=CT_RPR)
						return free_tree(root);
				}
				else
				{
					root->children.pop_back();
					root->type=PT_VAR_REF;
					if(!r_varname_nonnull(root))
						return free_tree(root);
					if(LOOK_AHEAD(0)==CT_SCOPE)
					{
						root->children.push_back(nullptr);
						if(!r_user_definition(root->children.back()))
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
		for(auto t=LOOK_AHEAD(0);;t=LOOK_AHEAD(0))//primary(args)[comma]	should be:  SUBSCRIPT(CALL(primary, args), comma)
		{
			switch(t)
			{
			case CT_LBRACKET:
				ADVANCE;
				INSERT_LEAF(root, PT_ARRAY_SUBSCRIPT, nullptr);//primary[comma1][comma2] should be:  SUBSCRIPT(SUBSCRIPT(primary, comma1), comma2)
				root->children.push_back(child);
				root->children.push_back(nullptr);
				if(!r_comma_expr(root->children.back()))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE;
				child=root;
				continue;
			case CT_LPR:
				ADVANCE;
				INSERT_LEAF(root, PT_FUNC_CALL, nullptr);//primary(args1)(args2) should be:  CALL(CALL(primary, args1), args2)
				root->children.push_back(child);
				root->children.push_back(nullptr);
				if(!r_func_args(root->children.back()))
					return free_tree(root);
				if(LOOK_AHEAD(0)!=CT_RBRACKET)
					return free_tree(root);
				ADVANCE;
				child=root;
				continue;
			case CT_INCREMENT:
				ADVANCE;
				INSERT_LEAF(root, PT_POST_INCREMENT, nullptr);
				root->children.push_back(child);
				child=root;
				continue;
			case CT_DECREMENT:
				ADVANCE;
				INSERT_LEAF(root, PT_POST_DECREMENT, nullptr);
				root->children.push_back(child);
				child=root;
				continue;
			case CT_PERIOD:
			case CT_ARROW:
				ADVANCE;
				INSERT_LEAF(root, t, nullptr);
				root->children.push_back(child);
				root->children.push_back(new IRNode(PT_VAR_REF, CT_IGNORED));
				if(!r_varname_nonnull(root->children.back()))
					return free_tree(root);
				child=root;
				continue;
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
				if(!r_cast(root->children.back()))
					return free_tree(root);
			}
			break;
		case CT_SIZEOF:
			//r_sizeof() inlined
//sizeof.expr
//  : SIZEOF unary.expr
//  | SIZEOF '(' type.name ')'
			ADVANCE;

			INSERT_LEAF(root, CT_SIZEOF, nullptr);
			
			root->children.push_back(nullptr);
			if(LOOK_AHEAD(0)==CT_LPR)
			{
				int t_idx=current_idx;
				ADVANCE;
				if(r_typename(root->children.back()))
					goto r_sizeof_done;
				current_idx=t_idx;//restore idx
			}
			if(!r_unary(root->children.back()))
				return free_tree(root);
		r_sizeof_done:
			//end of r_sizeof() inlined
			break;
		case CT_THROW:
			//r_throw_expr() inlined			//throw.expression  :=  THROW {expression}
			ADVANCE;

			INSERT_LEAF(root, CT_THROW, nullptr);

			switch(LOOK_AHEAD(0))
			{
			case CT_COLON:case CT_SEMICOLON:
				break;
			default:
				root->children.push_back(nullptr);
				if(!r_assign_expr(root->children.back()))
					return free_tree(root);
				break;
			}
			//end of r_throw_expr() inlined
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

	bool		r_opt_int_type_or_class_spec(IRNode *&root, TypeInfo &type)//int.type.or.class.spec := (char|wchar_t|int|short|long|signed|unsigned|float|double|void|bool)+  |  class.spec  |  enum.spec			type & root? assemble yourself
	{
		Name::Node *scope=nullptr;
		short long_count=0, int_count=0;
		for(;;)
		{
			auto token=&LOOK_AHEAD_TOKEN(0);
			switch(token->type)
			{
			case CT_CONST:case CT_VOLATILE:
			case CT_AUTO:
			case CT_SIGNED:case CT_UNSIGNED:
			case CT_VOID:
			case CT_BOOL:
			case CT_CHAR:case CT_SHORT:case CT_INT:case CT_LONG:
			case CT_FLOAT:case CT_DOUBLE:
			case CT_WCHAR_T:
			case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
			case CT_ID:
#if 1//region
				switch(token->type)
				{
				case CT_CONST://"long const long" should work, cv_qualifiers can appear many times, same effect
					ADVANCE;
					type.is_const=true;
					break;
				case CT_VOLATILE:
					ADVANCE;
					type.is_volatile=true;
					break;
				case CT_AUTO:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
						type.datatype=TYPE_AUTO;
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_SIGNED:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED||type.datatype==TYPE_INT)
					{
						type.datatype=TYPE_INT_SIGNED;
						type.size=4, type.logalign=2;
					}
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_UNSIGNED:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED||type.datatype==TYPE_INT)
					{
						type.datatype=TYPE_INT_UNSIGNED;
						type.size=4, type.logalign=2;
					}
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_VOID:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
						type.datatype=TYPE_VOID, type.logalign=0;
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_BOOL:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_BOOL;
						type.size=1, type.logalign=0;
					}
					break;
				case CT_CHAR:
					ADVANCE;
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.datatype=TYPE_CHAR;//TODO: reject 'unsigned int char'
						type.size=1, type.logalign=0;
						break;
					default:
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_SHORT:
					ADVANCE;
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
						type.datatype=TYPE_INT;
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.size=2, type.logalign=1;
						break;
					default:
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_INT:
					ADVANCE;
					++int_count;
					switch(type.datatype)
					{
					case TYPE_UNASSIGNED:
						type.datatype=TYPE_INT;
					case TYPE_INT:
					case TYPE_INT_SIGNED:
					case TYPE_INT_UNSIGNED:
						type.size=4, type.logalign=2;
						break;
					default:
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_LONG:
					ADVANCE;
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
							type.size=4, type.logalign=2;
							break;
						case 4:
							if(long_count==2)
								type.size=8, type.logalign=3;
							break;
						case 8:
							//TODO: error: invalid type spec combination
							return free_tree(root);
						}
						break;
					case TYPE_FLOAT:
						if(type.size==8)
							type.size=16, type.logalign=4;
						else
						{
							//TODO: error: invalid type spec combination
							return free_tree(root);
						}
						break;
					default:
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_FLOAT:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_FLOAT;
						type.size=4, type.logalign=2;
					}
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_DOUBLE://TODO: differentiate long double (ok) & int double (error)
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_FLOAT;
						type.size=8, type.logalign=3;
					}
					else if(type.datatype==TYPE_INT&&long_count==1&&!int_count)
					{
						type.datatype=TYPE_FLOAT;
						type.size=16, type.logalign=4;
					}
					else
					{
						//TODO: error: invalid type spec combination
						return free_tree(root);
					}
					break;
				case CT_WCHAR_T:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_CHAR;
						type.size=4, type.logalign=2;
					}
					break;
				case CT_INT8:case CT_INT16:case CT_INT32:case CT_INT64:
					ADVANCE;
					if(type.datatype==TYPE_UNASSIGNED)
					{
						type.datatype=TYPE_CHAR;
						switch(token->type)
						{
						case CT_INT8: type.size=1, type.logalign=0;break;
						case CT_INT16:type.size=2, type.logalign=1;break;
						case CT_INT32:type.size=4, type.logalign=2;break;
						case CT_INT64:type.size=8, type.logalign=3;break;
						}
					}
					break;
				case CT_ID:
					{
						scope=scope_lookup(token->sdata, false);//check if identifier is a datatype in this/global scope		TODO: support global lookup
						if(!scope||scope->data.is_var)
							goto r_opt_int_type_or_class_spec_done;
						if(type.datatype!=TYPE_UNASSIGNED)
							return free_tree(root);
						ADVANCE;
						type=*scope->data.tdata;
					}
					break;
				}
#endif
				if(root==nullptr)
					INSERT_LEAF(root, PT_TYPE, nullptr);
				continue;
			case CT_CLASS://TODO: unroll the struct to typedb at declaration
			case CT_STRUCT:
			case CT_UNION://or UserKeyword
#if 1//region
				{
					if(root)
						return free_tree(root);

					//r_class_spec() inlined
	//class.spec
	//  : {userdef.keyword} class.key							 class.body
	//  | {userdef.keyword} class.key name						{class.body}
	//  | {userdef.keyword} class.key name ':' base.specifiers	 class.body
	//
	//class.key  :=  CLASS | STRUCT | UNION

					//TODO: understand UserKeyword		ignore userdef.keyword for now
				
					auto t=LOOK_AHEAD(0);
					auto t2=&LOOK_AHEAD_TOKEN(1);
					bool body_is_obligatory=true, named_spec=t2->type==CT_ID;
					if(named_spec)
					{
						ADVANCE_BY(2);
						INSERT_LEAF(root, t, t2->sdata);//children: {member*}

						//incomplete type definition
						type.datatype=TYPE_VOID;
						scope_declare_type(root->sdata, add_type(type));

						if(body_is_obligatory=LOOK_AHEAD(0)==CT_COLON)//inheritance
						{
							//r_base_specifiers() inlined
//base.specifiers  :=  ':' base.specifier (',' base.specifier)*
//
//base.specifier  :=  {{VIRTUAL} (PUBLIC | PROTECTED | PRIVATE) {VIRTUAL}} name
							ADVANCE;
							root->children.push_back(new IRNode(PT_BASE_SPECIFIER, CT_IGNORED));
							auto node=root->children.back();
							for(;;)
							{
								auto t=LOOK_AHEAD(0);
								if(t==CT_VIRTUAL)
								{
									ADVANCE;
									node->children.push_back(new IRNode(CT_VIRTUAL, CT_IGNORED));
									t=LOOK_AHEAD(0);
								}
								switch(t)
								{
								case CT_PUBLIC:
								case CT_PROTECTED:
								case CT_PRIVATE:
									ADVANCE;
									node->children.push_back(new IRNode(t, CT_IGNORED));
									break;
								}
								if(t==CT_VIRTUAL)
								{
									ADVANCE;
									node->children.push_back(new IRNode(CT_VIRTUAL, CT_IGNORED));
									t=LOOK_AHEAD(0);
								}
								node->children.push_back(nullptr);
								if(!r_name_lookup(node->children.back()))
									return free_tree(root);
								if(LOOK_AHEAD(0)!=CT_COMMA)
									break;
								ADVANCE;
							}
							//end of r_base_specifiers() inlined
						}
					}
					else//anonymous class
					{
						ADVANCE;
						INSERT_LEAF(root, t, scope_id_lbrace);
						
						//TODO: encode class name as an impossible identifier, if necessary
					}
					switch(token->type)
					{
					case CT_CLASS:	type.datatype=TYPE_CLASS;	break;
					case CT_STRUCT:	type.datatype=TYPE_STRUCT;	break;
					case CT_UNION:	type.datatype=TYPE_UNION;	break;//or UserKeyword
					}

					if(LOOK_AHEAD(0)!=CT_LBRACE)
					{
						if(body_is_obligatory)
							return free_tree(root);
					}
					else
					{
						//r_class_body() inlined			class.body  :=  '{' (class.member)* '}'
						ADVANCE;
						scope_enter(root->sdata);

						for(char accesstype=type.datatype==TYPE_CLASS?ACCESS_PRIVATE:ACCESS_PUBLIC;LOOK_AHEAD(0)!=CT_RBRACE;)
						{
							//r_class_member() inlined
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
							t=LOOK_AHEAD(0);
							switch(t)
							{
							case CT_PRIVATE:
							case CT_PROTECTED:
							case CT_PUBLIC:
								ADVANCE;
								switch(t)
								{
								case CT_PRIVATE:	accesstype=ACCESS_PRIVATE;	break;
								case CT_PROTECTED:	accesstype=ACCESS_PROTECTED;break;
								case CT_PUBLIC:		accesstype=ACCESS_PUBLIC;	break;
								}
								//INSERT_LEAF(root->children.back(), t, nullptr);
								if(LOOK_AHEAD(0)!=CT_COLON)
									return scope_error(root);
								ADVANCE;
								break;
							//case UserKeyword4://user.access.spec
							//	break;
							case CT_SEMICOLON:
								ADVANCE;
								break;
							case CT_TYPEDEF:
								root->children.push_back(nullptr);
								if(!r_typedef(root->children.back()))
									return scope_error(root);
								root->children.back()->flag_accesstype=accesstype;
								break;
							case CT_TEMPLATE:
								root->children.push_back(nullptr);
								if(!r_template_decl(root->children.back()))
									return scope_error(root);
								root->children.back()->flag_accesstype=accesstype;
								break;
							case CT_USING:
								root->children.push_back(nullptr);
								if(!r_using(root->children.back()))
									return scope_error(root);
								root->children.back()->flag_accesstype=accesstype;
								break;
							//case CT_CLASS://X  not metaclass		ignore for now
							//case CT_STRUCT:
							//case CT_UNION:
							//case CT_ENUM:
							//	if(!r_metaclass_decl(root))
							//		return false;
							//	break;
							default:
								{
									root->children.push_back(nullptr);
									int idx=current_idx;
									if(!r_declaration(root->children.back()))
									{
										free_tree(root->children.back());//not an error
										current_idx=idx;
										//return r_access_decl(root->children.back());//inlined
				
										//r_access_decl() inlined			access.decl  :=  name ';'
										if(!r_name_lookup(root->children.back()))
											return scope_error(root);

										if(LOOK_AHEAD(0)!=CT_SEMICOLON)
											return scope_error(root);
										ADVANCE;
										//end of r_access_decl() inlined
									}
									root->children.back()->flag_accesstype=accesstype;
								}
								break;
							}
							//end of r_class_member() inlined
						}
						ADVANCE;
						scope_exit();
						//end of r_class_body() inlined

						type.body=root;
						type.size=1;				//calculate size & logalign
						IRNode *node=nullptr;
						TypeInfo *ptype=nullptr;
						if(type.datatype==TYPE_UNION)
						{
							for(size_t k=0;k<root->children.size();++k)
							{
								node=root->children[k];
								if(node->type==PT_TYPE)
								{
									ptype=node->tdata;
									if(type.size<ptype->size)
										type.size=ptype->size;
									if(type.logalign<ptype->logalign)
										type.logalign=ptype->logalign;
								}
							}
						}
						else//class/struct
						{
							size_t mask=0;
							for(size_t k=0;k<root->children.size();++k)
							{
								node=root->children[k];
								if(node->type==PT_TYPE)
								{
									ptype=node->tdata;
									mask=(1<<ptype->logalign)-1;
									type.size+=mask;
									type.size&=~mask;
									type.size+=ptype->size;

									if(type.logalign<ptype->logalign)
										type.logalign=ptype->logalign;
								}
							}
							mask=(1<<type.logalign)-1;
							type.size+=mask;
							type.size&=~mask;
						}
						ptype=add_type(type);
						if(named_spec)
							scope_declare_type(root->sdata, ptype);//should overwrite the incomplete type
					}
					//end of r_class_spec() inlined
				}
#endif
				break;
			case CT_ENUM:
#if 1//region
				{
					if(root)
						return free_tree(root);

					//r_enum_spec() inlined
//enum.spec
//  : ENUM  Identifier
//  | ENUM {Identifier} '{' {enum.body} '}'
					ADVANCE;

					INSERT_LEAF(root, CT_ENUM, nullptr);//children: {???}

					auto t=&LOOK_AHEAD_TOKEN(0);
					if(t->type==CT_ID)
					{
						ADVANCE;
						root->sdata=t->sdata;

						t=&LOOK_AHEAD_TOKEN(0);
						if(t->type!=CT_LBRACE)
							goto r_enum_spec_finish;
					}

					if(t->type!=CT_LBRACE)
						return free_tree(root);
					ADVANCE;
					//no enter/exit scope in enum

					//r_enum_body() inlined				//enum.body  :=  Identifier {'=' expression} (',' Identifier {'=' expression})* {','}
					type.datatype=TYPE_ENUM_CONST;
					auto ptype=add_type(type);
					for(int k=0;;++k)
					{
						auto t=&LOOK_AHEAD_TOKEN(0);
						if(t->type==CT_RBRACE)
							break;
						if(t->type!=CT_ID)
							return free_tree(root);
						ADVANCE;

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
							//TODO: evaluate const integral assign_expr
							//k=...;
						}
						root->children.push_back(new IRNode(PT_ENUM_CONST, k, t->sdata));
						scope_declare_var(t->sdata, ptype, k);

						if(LOOK_AHEAD(0)!=CT_COMMA)
							break;
						ADVANCE;
					}
					//end of r_enum_body() inlined

					if(LOOK_AHEAD(0)!=CT_RBRACE)
						return free_tree(root);
					ADVANCE;

				r_enum_spec_finish:
					//end of r_enum_spec() inlined

					type.datatype=TYPE_ENUM;

					type.logalign=ceil_log2(root->children.size());
					if(type.logalign>3)
						type.logalign-=3;
					else
						type.logalign=0;
					type.size=1<<type.logalign;//non-standard: enum has minimum size to contain all declared constants

					type.body=root;
					ptype=add_type(type);
					if(root->sdata)//add enum name for qualified lookup
						scope_declare_type(root->sdata, ptype);
				}
#endif
				break;
			}
			break;
		}//end for
	r_opt_int_type_or_class_spec_done:
		if(long_count>2||int_count>1)
			return free_tree(root);
		//if(root)
		//	root->set(add_type(type));
		return true;
	}

//condition
//  : {cv.qualify} (int.type.or.class.spec | name) declarator2 '=' assign.expr		//<- simple.declaration
//  | comma.expresion
	bool		r_condition(IRNode *&root)
	{
		int t_idx=current_idx;

		//r_simple_declaration() inlined			//condition controlling expression of switch/while/if
		INSERT_LEAF(root, PT_SIMPLE_DECLARATION, nullptr);
		TypeInfo type;
		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			goto r_condition_choice2;

		root->children.push_back(nullptr);
		if(!r_opt_int_type_or_class_spec(root->children.back(), type))
			goto r_condition_choice2;

		if(!root)
		{
			root->children.push_back(nullptr);
			if(!r_name_lookup(root->children.back()))
				goto r_condition_choice2;
			//TODO: initialize the type with the name
		}
		type.set_flags(STORAGE_UNSPECIFIED, 0, cv_flag);
		root->tdata=add_type(type);
		
		root->children.push_back(nullptr);
		if(!r_declarator2(root->children.back(), *root->tdata, DECLKIND_NORMAL, false, true, true, true))
			goto r_condition_choice2;

		if(LOOK_AHEAD(0)!=CT_ASSIGN)
			goto r_condition_choice2;
		ADVANCE;
		root->children.push_back(new IRNode(CT_ASSIGN, CT_IGNORED));

		root->children.push_back(nullptr);
		if(!r_assign_expr(root->children.back()))
			goto r_condition_choice2;

		return true;
		//end of r_simple_declaration() inlined

	r_condition_choice2:
		free_tree_not_an_error(root);
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

		if(!r_declarators_nonnull(root, *root->children.back()->tdata, true, false, false))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;

//#ifdef DEBUG_PARSER
//		debugprint(root);//
//#endif

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
	bool		r_arg_declaration(IRNode *&root)//arg.declaration  :=  {userdef.keyword | REGISTER} type.specifier arg.declarator {'=' initialize.expr}
	{
		//INSERT_LEAF(root, PT_ARG_DECL, nullptr);//children: {type, argname [, default value]}

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
		//	ADVANCE;
		//	break;
		}
#ifdef DEBUG_PARSER
		int t_idx=current_idx;
	r_arg_declaration_again:
		if(!r_typespecifier(root, true)||!root->tdata)
		{
			print_neighborhood();
			current_idx=t_idx;
			free_tree(root);
			goto r_arg_declaration_again;
		}
#else
		if(!r_typespecifier(root, true))
			return false;
#endif

		root->children.push_back(nullptr);
		if(!r_declarator2(root->children.back(), *root->tdata, DECLKIND_ARG, false, true, false, true))
			return free_tree(root);

		if(LOOK_AHEAD(0)==CT_ASSIGN)
		{
			ADVANCE;
			root->children.push_back(new IRNode(CT_ASSIGN, CT_IGNORED));
			root->children.back()->children.push_back(nullptr);
			if(!r_initialize_expr(root->children.back()->children.back()))
				return free_tree(root);
		}

		return true;
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

		//r_cast_operator_name() inlined			//cast.operator.name  :=  {cv.qualify} (integral.type.or.class.spec | name) {cv.qualify} {(ptr.operator)*}
		INSERT_LEAF(root, PT_CAST, nullptr);

		int cv_flag=0;
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		TypeInfo type;
		root->children.push_back(nullptr);
		if(!r_opt_int_type_or_class_spec(root->children.back(), type))
			return free_tree(root);
		if(!root->children.back())
		{
			if(!r_name_lookup(root->children.back()))
				return free_tree(root);

			//TODO: initialize the type based on this name		requires name system
		}

		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);

		type.set_flags(STORAGE_UNSPECIFIED, 0, cv_flag);
		
		if(!r_opt_ptr_operator_nonnull(root, type, true))//TODO: organize what gets inserted into the parse tree
			return free_tree(root);

		root->tdata=add_type(type);
		//end of r_cast_operator_name() inlined

		return true;
	}

	bool		r_const_decl(IRNode *&root, TypeInfo &type)
	{
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);

		if(!r_declarators_nonnull(root, type, false, false, true))//TODO: decl these variables as const
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;

		return true;
	}
//declaration.statement
//  : {storage.spec} {cv.qualify} integral.type.or.class.spec {cv.qualify} {declarators} ';'	//<- integral.decl.statement
//  | {storage.spec} {cv.qualify} name {cv.qualify} declarators ';'								//<- other.decl.statement
//  | cv.qualify {'*'} Identifier '=' expression {',' declarators} ';'							//<- const.declaration
//
//decl.head
//  : {storage.spec} {cv.qualify}
	bool		r_decl_stmt(IRNode *&root)//1 call
	{
		//INSERT_LEAF(root, PT_VAR_DECL, nullptr);//children: {storage, cv, integral, int/const/other decl}		TODO: improve this

		TypeInfo type;
		StorageTypeSpecifier esmr_flag=STORAGE_EXTERN;//TODO: storage specifier
		int cv_flag=0;
		if(!r_opt_storage_spec(esmr_flag)||!r_opt_cv_qualify(cv_flag)||!r_opt_int_type_or_class_spec(root, type)||!root)//not optional
			return free_tree(root);

		if(esmr_flag!=STORAGE_UNSPECIFIED)
		{
			//if(!r_int_decl_stmt(root->children.back()))//inlined
			//	return free_tree(root);

			//r_int_decl_stmt() inlined				//integral.decl.statement  :=  decl.head integral.type.or.class.spec {cv.qualify} {declarators} ';'
			//INSERT_LEAF(root, PT_VAR_DECL, nullptr);
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);

			type.set_flags(esmr_flag, 0, cv_flag);
			root->tdata=add_type(type);

			if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			{
				if(!r_declarators_nonnull(root, *root->tdata, false, true, true))
					return free_tree(root);

				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
					return free_tree(root);
			}
			ADVANCE;
			//end of r_int_decl_stmt() inlined

			return true;
		}
		root->children.push_back(nullptr);

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
			break;
		}
		if(cv_flag&&(t==CT_ID&&assign||t==CT_ASTERIX))
		{
			type.set_flags(esmr_flag, 0, cv_flag);
			root->tdata=add_type(type);

			if(!r_const_decl(root->children.back(), *root->tdata))
				return free_tree(root);
			return true;
		}
		//if(!r_other_decl_stmt(root->children.back()))//inlined
		//	return free_tree(root);

		//r_other_decl_stmt() inlined			//other.decl.statement  :=  decl.head name {cv.qualify} declarators ';'
		INSERT_LEAF(root, PT_VAR_DECL, nullptr);

		root->children.push_back(nullptr);
		if(!r_name_lookup(root->children.back()))
			return free_tree(root);
		
		if(!r_opt_cv_qualify(cv_flag))
			return free_tree(root);
		
		type.set_flags(esmr_flag, 0, cv_flag);
		root->tdata=add_type(type);

		if(!r_declarators_nonnull(root, type, false, false, true))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		//end of r_other_decl_stmt() inlined

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
		{
			ADVANCE;
			return true;
		}

		int t_idx=current_idx;//save state
		if(r_decl_stmt(root))//the only call
			return true;

		current_idx=t_idx;//restore state
		if(!r_comma_expr(root))
			return false;

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return false;
		ADVANCE;

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
			//return r_if(root);

			//r_if() inlined			//if.statement  :=  IF '(' condition ')' statement { ELSE statement }
			if(LOOK_AHEAD(1)!=CT_LPR)
				return false;
			ADVANCE_BY(2);

			INSERT_LEAF(root, CT_IF, nullptr);//children: {condition, body, [else body]}

			root->children.push_back(nullptr);
			if(!r_condition(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
			
			root->children.push_back(nullptr);
			if(!r_statement(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)==CT_ELSE)
			{
				ADVANCE;
				root->children.push_back(nullptr);
				if(!r_statement(root->children.back()))
					return free_tree(root);
			}
			//end of r_if() inlined
			break;
		case CT_SWITCH:
			//return r_switch(root);//inlined
			
			//r_switch() inlined		//switch.statement  :=  SWITCH '(' condition ')' statement
			if(LOOK_AHEAD(1)!=CT_LPR)
				return false;
			ADVANCE_BY(2);

			INSERT_LEAF(root, CT_SWITCH, nullptr);

			root->children.push_back(nullptr);
			if(!r_condition(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
			
			root->children.push_back(nullptr);
			if(!r_statement(root->children.back()))
				return free_tree(root);
			//end of r_switch() inlined
			break;
		case CT_WHILE:
			//return r_while(root);

			//r_while() inlined			//while.statement  :=  WHILE '(' condition ')' statement
			if(LOOK_AHEAD(1)!=CT_LPR)
				return false;
			ADVANCE_BY(2);

			INSERT_LEAF(root, CT_WHILE, nullptr);//children: {condition, body}
			
			root->children.push_back(nullptr);
			if(!r_condition(root->children.back()))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
		
			root->children.push_back(nullptr);
			if(!r_statement(root->children.back()))
				return free_tree(root);
			//end of r_while() inlined
			break;
		case CT_DO:
			//return r_do_while(root);//inlined

			//r_do_while() inlined			//do.statement  :=  DO statement WHILE '(' comma.expression ')' ';'
			ADVANCE;

			INSERT_LEAF(root, CT_DO, nullptr);//children: {body, condition}
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
			//end of r_do_while() inlined
			break;
		case CT_FOR:
			//return r_for(root);//inlined

			//r_for() inlined			//for.statement  :=  FOR '(' expr.statement {comma.expression} ';' {comma.expression} ')' statement
			if(LOOK_AHEAD(1)!=CT_LPR)
				return false;
			ADVANCE_BY(2);
			
			INSERT_LEAF(root, CT_FOR, nullptr);//children: {initialization, condition, increment, statement}, check for nullptr
			root->children.assign_pod(4, nullptr);
			
			if(!r_expr_stmt(root->children[0]))
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			{
				if(!r_comma_expr(root->children[1]))
					return free_tree(root);
			}
			if(LOOK_AHEAD(0)!=CT_SEMICOLON)
				return free_tree(root);
			ADVANCE;
			
			if(LOOK_AHEAD(0)!=CT_RPR)
			{
				if(!r_comma_expr(root->children[2]))
					return free_tree(root);
			}
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;

			if(!r_statement(root->children[3]))
				return free_tree(root);
			//end of r_for() inlined
			break;
		case CT_TRY:
			//return r_try_catch(root);//inlined

			{
				char ellipsis=false;
				//r_try_catch() inlined
//try.statement  :=  TRY compound.statement (exception.handler)+ ';'
//
//exception.handler  :=  CATCH '(' (arg.declaration | Ellipsis) ')' compound.statement
				ADVANCE;

				INSERT_LEAF(root, CT_TRY, nullptr);//children: {statement, {arg, handlerstmt}+}

				root->children.push_back(nullptr);
				if(!r_compoundstatement(root->children.back()))
					return free_tree(root);

				do
				{
					if(LOOK_AHEAD(0)!=CT_CATCH||LOOK_AHEAD(1)!=CT_LPR)
						return free_tree(root);
					ADVANCE_BY(2);

					if(LOOK_AHEAD(0)==CT_ELLIPSIS)
					{
						if(ellipsis)
							return free_tree(root);
						ADVANCE;
						root->children.push_back(new IRNode(CT_ELLIPSIS, CT_IGNORED));
						ellipsis=true;
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
				//end of r_try_catch() inlined
			}
			break;
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

				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
				{
					root->children.push_back(nullptr);
					if(!r_comma_expr(root->children.back()))
						return free_tree(root);
				}

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
			if(!r_assign_expr(root->children.back()))//evaluate compile time expr
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
		INSERT_LEAF(root, PT_ARG_DECLLIST, nullptr);
		auto t=LOOK_AHEAD(0);
		if(t==CT_RPR)
			return true;
		for(;;)
		{
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
			{
				ADVANCE;
				t=LOOK_AHEAD(0);
			}
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
	bool		r_arg_decllist_or_init(IRNode *&root, bool &is_args, bool maybe_init)//1 call
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
	bool		r_opt_throw_decl(IRNode *&root)//throw.decl  :=  THROW '(' (name {','})* {name} ')'			1 call
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
				if(!r_name_lookup(root->children.back()))
					return free_tree(root);
			}

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
		}
		return true;
	}
	bool		r_member_initializers(IRNode *&root)//member.initializers  :=  ':' member.init (',' member.init)*
	{
		if(LOOK_AHEAD(0)!=CT_COLON)
			return false;

		INSERT_LEAF(root, PT_MEMBER_INIT_LIST, nullptr);

		for(;;)
		{
			//r_member_init() inlined			//member.init  :=  name '(' function.arguments ')'
			root->children.push_back(nullptr);
			auto child=root->children.back();
			INSERT_LEAF(child, PR_MEMBER_INIT, nullptr);//TODO: root==name, children==function.arguments

			child->children.push_back(nullptr);
			if(!r_name_lookup(child->children.back()))
				return free_tree(root);

			//TODO: qualified name lookup

			if(LOOK_AHEAD(0)!=CT_LPR)
				return free_tree(root);
			ADVANCE;

			child->children.push_back(nullptr);
			if(!r_func_args(child->children.back()))
				return free_tree(root);
		
			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;
			//end of r_member_init() inlined

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
	}
//declarator
//  : (ptr.operator)* ('(' declarator ')' | name) ('[' comma.expression ']')* {func.args.or.init}
//
//func.args.or.init
//  : '(' arg.decl.list.or.init ')' {cv.qualify} {throw.decl} {member.initializers}
//
//  Note: We assume that '(' declarator ')' is followed by '(' or '['.
//	This is to avoid accepting a function call F(x) as a type F and a declarator x.
//	This assumption is ignored if should_be_declarator is true.
//
//  Note: An argument declaration list and a function-style initializer take a different Ptree structure.
//	e.g.
//	    int f(char) ==> .. [f ( [[[char] 0]] )]
//	    Point f(1)  ==> .. [f [( [1] )]]
//
//  Note: is_statement changes the behavior of rArgDeclListOrInit().
	bool		r_declarator2(IRNode *&root, TypeInfo type, int kind, bool recursive, bool should_be_declarator, bool is_statement, bool is_var)//TypeInfo is already initialized
	{
		if(recursive)
			INSERT_LEAF(root, PT_DECLARATOR_PARENS, nullptr);//children: {???}			TODO: no declaration if has nothing
		else
			INSERT_LEAF(root, PT_DECLARATOR, nullptr);

		//TODO: don't push back to root optional stuff
		if(!r_opt_ptr_operator_nonnull(root, type, true))
			return free_tree(root);
		auto ptype=add_type(type);

		auto t=LOOK_AHEAD(0);
		if(t==CT_LPR)
		{
			ADVANCE;

			root->children.push_back(nullptr);
			if(!r_declarator2(root->children.back(), *ptype, kind, true, true, false, is_var))//recursive
				return free_tree(root);

			if(LOOK_AHEAD(0)!=CT_RPR)
				return free_tree(root);
			ADVANCE;

			if(!should_be_declarator)
			{
				if(kind==DECLKIND_NORMAL&&root->children.size()==1)
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
			{
				ADVANCE;
				root->children.push_back(new IRNode(t, CT_IGNORED));
			}
			if(LOOK_AHEAD(0)==CT_ID&&LOOK_AHEAD(1)!=CT_SCOPE)
			{
				//definition with simple identifier in current scope
				root->children.push_back(new IRNode(CT_ID, CT_IGNORED, LOOK_AHEAD_TOKEN(0).sdata));
				ADVANCE;
				scope_declare_entity(root->children.back()->sdata, ptype, is_var);
				//if(is_var)
				//	scope_declare_var(root->children.back()->sdata, ptype, 0);
				//else
				//	scope_declare_type(root->children.back()->sdata, ptype);
			}
			else
			{
				root->children.push_back(nullptr);
				if(!r_name_lookup(root->children.back()))//lookup for static variables like "int *ClassName::StaticVar=0;"
					return free_tree(root);
			}
		}

		for(bool brackets=false;;)
		{
			t=LOOK_AHEAD(0);
			if(t==CT_LPR)//function arglist
			{
				if(brackets)
					return free_tree(root);//T t[2](int) is invalid
				ADVANCE;

				bool is_args=true;
				root->children.push_back(nullptr);
				if(!r_arg_decllist_or_init(root->children.back(), is_args, is_statement))//the only call
					return free_tree(root);
				
				if(LOOK_AHEAD(0)!=CT_RPR)
					return free_tree(root);
				ADVANCE;

				int cv_flag=0;
				if(is_args)
				{
					if(!r_opt_cv_qualify(cv_flag))
						return free_tree(root);
				}
				
				if(LOOK_AHEAD(0)==CT_THROW)
				{
					root->children.push_back(nullptr);
					if(!r_opt_throw_decl(root->children.back()))//the only call
						return free_tree(root);
				}

				if(LOOK_AHEAD(0)==CT_COLON)//constructor member initializers
				{
					root->children.push_back(nullptr);
					if(!r_member_initializers(root->children.back()))
						return free_tree(root);
				}

				break;//T f(int)(char) is invalid
			}
			else if(t==CT_LBRACKET)//array
			{
				brackets=true;
				ADVANCE;
				auto child=root->children.back();
				root->children.back()=new IRNode(PT_ARRAY_SUBSCRIPT, CT_IGNORED);
				root->children.back()->children.push_back(child);

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
	bool		r_declarators_nonnull(IRNode *&root, TypeInfo &type, bool should_be_declarator, bool is_statement, bool is_var)//declarators := declarator.with.init (',' declarator.with.init)*
	{
		for(;;)
		{
			//r_declaratorwithinit() inlined		//declarator.with.init  :=  ':' expression  |  declarator {'=' initialize.expr | ':' expression}
			root->children.push_back(nullptr);
			if(LOOK_AHEAD(0)==CT_COLON)//bit field
			{
				ADVANCE;

				if(!r_assign_expr(root->children.back()))
					return free_tree(root);
			}
			else
			{
				if(!r_declarator2(root->children.back(), type, DECLKIND_NORMAL, false, should_be_declarator, is_statement, is_var))
					return free_tree(root);
				switch(LOOK_AHEAD(0))
				{
				case CT_ASSIGN://init
					ADVANCE;
					root->children.push_back(new IRNode(CT_ASSIGN, CT_IGNORED));
					root->children.back()->children.push_back(nullptr);
					if(!r_initialize_expr(root->children.back()->children.back()))
						return free_tree(root);
					break;
				case CT_COLON://bit field
					ADVANCE;
					root->children.push_back(new IRNode(CT_COLON, CT_IGNORED));
					root->children.back()->children.push_back(nullptr);
					if(!r_assign_expr(root->children.back()->children.back()))
						return free_tree(root);
					break;
				}
			}
			//end of r_declaratorwithinit() inlined

			if(LOOK_AHEAD(0)!=CT_COMMA)
				break;
			ADVANCE;
		}
		return true;
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
//declaration
//  : integral.declaration
//  | const.declaration
//  | other.declaration
//
//decl.head
//  : {member.spec} {storage.spec} {member.spec} {cv.qualify}
//
//integral.declaration
//  : decl.head integral.type.or.class.spec {cv.qualify} declarators (';' | function.body)
//  | decl.head integral.type.or.class.spec {cv.qualify} ';'
//  | decl.head integral.type.or.class.spec {cv.qualify} ':' expression ';'
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
		//INSERT_LEAF(root, PT_DECLARATION, nullptr);//children: {???}		TODO: organize structure
		//root->children.assign(2, nullptr);
		
		TypeInfo type;
		StorageTypeSpecifier esmr_flag=STORAGE_EXTERN;//TODO: pass the type instead of additional int flags, to save stack
		int fvi_flag=0, cv_flag=0;
		if(!r_opt_member_spec(fvi_flag)||!r_opt_storage_spec(esmr_flag))
			return false;
		if(!fvi_flag)
		{
			if(!r_opt_member_spec(fvi_flag))
				return false;
		}
		
		if(!r_opt_cv_qualify(cv_flag))
			return false;

		if(!r_opt_int_type_or_class_spec(root, type))
			return false;

		if(root)
		{
			//root->children.push_back(nullptr);
			//if(!r_int_declaration(root->children.back()))//got inlined
			//	return free_tree(root);

			//r_int_declaration() inlined			no CFG here
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);
			
			type.set_flags(esmr_flag, fvi_flag, cv_flag);
			root->tdata=add_type(type);

			switch(LOOK_AHEAD(0))
			{
			case CT_SEMICOLON:
				ADVANCE;
				break;
			case CT_COLON://bit field
				ADVANCE;
				root->children.push_back(new IRNode(CT_COLON, CT_IGNORED));

				root->children.push_back(nullptr);
				if(!r_assign_expr(root->children.back()))
					return free_tree(root);

				if(LOOK_AHEAD(0)!=CT_SEMICOLON)
					return free_tree(root);
				ADVANCE;
				break;
			default:
				if(!r_declarators_nonnull(root, *root->tdata, true, false, true))
					return free_tree(root);
				if(LOOK_AHEAD(0)==CT_SEMICOLON)
					ADVANCE;
				else
				{
					root->children.push_back(nullptr);
					if(!r_compoundstatement(root->children.back()))//function body
						return free_tree(root);
				}
				break;
			}

		/*	//type traversal
			auto type=root->tdata;
			for(int kd=0;kd<root->children.size();++kd)
			{
				auto child=root->children[kd];
				if(child->type!=PT_DECLARATOR)//internal error: expected a declarator
					return free_tree(root);
				for(int ke=0;ke<child->children.size();++ke)
				{
					auto c2=child->children[ke];
					switch(c2->type)
					{
					case CT_ASTERIX:
					case CT_AMPERSAND:
					case PT_DECLARATOR_PARENS:
					case CT_ASSIGN:
						break;
					}
				}
			}//*/
			return true;
		}
		
		INSERT_LEAF(root, PT_DECLARATION, nullptr);

		auto t=LOOK_AHEAD(0);
		root->children.push_back(nullptr);
		if(*(root->children.end()-3)&&(t==CT_ID&&LOOK_AHEAD(1)==CT_ASSIGN||t==CT_ASTERIX))
		{
			type.set_flags(esmr_flag, fvi_flag, cv_flag);
			root->tdata=add_type(type);

			return r_const_decl(root->children.back(), *root->tdata);
		}
		//return r_other_decl(root->children.back());//inlined

		//r_other_decl() inlined			no CFG here
		root->children.push_back(new IRNode(PT_VAR_DECL, CT_IGNORED));
		auto r2=root->children.back();

		r2->children.push_back(nullptr);
		if(!r_name_lookup(r2->children.back()))
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
			if(!r_opt_cv_qualify(cv_flag))
				return free_tree(root);

			type.set_flags(esmr_flag, fvi_flag, cv_flag);
			root->tdata=add_type(type);

			if(!r_declarators_nonnull(r2, *root->tdata, false, false, true))
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
		//end of r_other_decl() inlined
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
			INSERT_LEAF(root, t, LOOK_AHEAD_TOKEN(1).sdata);
		//	INSERT_LEAF(root, PT_TEMPLATE_ARG, LOOK_AHEAD_TOKEN(1).sdata);
			ADVANCE_BY(2);
			if(LOOK_AHEAD(0)==CT_ASSIGN)
			{
				ADVANCE;
				root->children.push_back(new IRNode(CT_ASSIGN, CT_IGNORED));
				root->children.back()->children.push_back(nullptr);
				if(!r_typename(root->children.back()->children.back()))
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
			if(!r_declarator2(root->children.back(), *root->tdata, DECLKIND_ARG, false, true, false, false))//TODO: set is_var properly
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

	bool		r_linkage_body(IRNode *&root, char *name)//linkage.body  :=  '{' (definition)* '}'
	{
		if(LOOK_AHEAD(0)!=CT_LBRACE)
			return false;
		ADVANCE;

		scope_enter(name?name:scope_id_lbrace);

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
		scope_exit();
		return true;
	}
//namespace.spec
//  :  NAMESPACE Identifier definition
//  |  NAMESPACE { Identifier } linkage.body
	bool		r_namespace_spec(IRNode *&root)//1 call
	{
		if(LOOK_AHEAD(0)!=CT_NAMESPACE)
			return false;
		ADVANCE;

		INSERT_LEAF(root, CT_NAMESPACE, nullptr);

		auto t=&LOOK_AHEAD_TOKEN(0);
		if(t->type==CT_ID)
		{
			ADVANCE;

			TypeInfo type;
			type.datatype=TYPE_NAMESPACE;
			scope_declare_type(t->sdata, add_type(type));

			root->sdata=t->sdata;
			t=&LOOK_AHEAD_TOKEN(0);
		}
		if(t->type!=CT_LBRACE)
			return free_tree(root);

		root->children.push_back(nullptr);
		return r_linkage_body(root->children.back(), root->sdata);
	}
	bool		r_namespace_alias(IRNode *&root)//namespace.alias  :=  NAMESPACE Identifier '=' Identifier ';'		1 call
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
		ADVANCE;
		INSERT_LEAF(root, CT_USING, nullptr);//children: {[namespace], name}

		if(LOOK_AHEAD(0)==CT_NAMESPACE)
		{
			ADVANCE;
			root->children.push_back(new IRNode(CT_NAMESPACE, CT_IGNORED));
		}

		root->children.push_back(nullptr);
		if(!r_name_lookup(root->children.back()))
			return free_tree(root);

		if(LOOK_AHEAD(0)!=CT_SEMICOLON)
			return free_tree(root);
		ADVANCE;

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

		INSERT_LEAF(root, t, t2->sdata);//children: {???}

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
		Token *t=nullptr;
		switch(LOOK_AHEAD(0))
		{
		case CT_SEMICOLON://null declaration
			ADVANCE;
			break;

		case CT_TYPEDEF:
			return r_typedef(root);

		case CT_TEMPLATE:
			return r_template_decl(root);

		//case CT_ENUM://X  not metaclass		ignore for now
		//case CT_STRUCT:
		//case CT_CLASS:
		//case CT_UNION:
		//	return r_metaclass_decl(root);

		case CT_EXTERN:
			t=&LOOK_AHEAD_TOKEN(1);
			switch(t->type)
			{
			case CT_VAL_STRING_LITERAL:
				//r_linkage_spec() inlined
//linkage.spec
//  :  EXTERN StringL definition
//  |  EXTERN StringL linkage.body
				ADVANCE_BY(2);

				INSERT_LEAF(root, CT_EXTERN, t->sdata);//children: {definition/linkage_body}

				root->children.push_back(nullptr);
				if(LOOK_AHEAD(0)==CT_LBRACE)
				{
					if(!r_linkage_body(root->children.back(), nullptr))
						return free_tree(root);
				}
				else if(!r_definition(root->children.back()))
					return free_tree(root);
				//end of r_linkage_spec() inlined
				break;
			case CT_TEMPLATE:
				//return r_extern_template_decl(root);//inlined

				//r_extern_template_decl() inlined		//extern.template.decl  :=  EXTERN TEMPLATE declaration
				ADVANCE_BY(2);

				INSERT_LEAF(root, PT_EXTERN_TEMPLATE, nullptr);

				root->children.push_back(nullptr);
				if(!r_declaration(root->children.back()))
					return free_tree(root);
				//end of r_extern_template_decl() inlined
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
			case CT_ENUM:case CT_STRUCT:case CT_CLASS:case CT_UNION:
			case CT_ID:
				return r_declaration(root);
			default:
				parse_error("Expected an extern declaration");
				return false;
			}
			break;

		case CT_NAMESPACE:
			if(LOOK_AHEAD(1)==CT_ASSIGN)
				return r_namespace_alias(root);//the only call
			return r_namespace_spec(root);//the only call

		case CT_USING:
			return r_using(root);

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
		case CT_ENUM:case CT_STRUCT:case CT_CLASS:case CT_UNION:
		case CT_ID:
			return r_declaration(root);

		default:
			parse_error("Expected a declaration");
			return false;
		}
		return true;
	}
//}
void			parse_cplusplus(Expression &ex, IRNode *&root)//OpenC++
{
	Token nulltoken={CT_IGNORED};
	ex.insert_pod(ex.size(), nulltoken, 16);
	current_ex=&ex;
	ntokens=ex.size()-16;
	current_idx=0;

	INSERT_LEAF(root, PT_PROGRAM, nullptr);//node at 0 is always PT_PROGRAM, everything stems from there
	try
	{
		for(;current_idx<ntokens;)
		{
			root->children.push_back(nullptr);
			auto &node=root->children.back();
			for(;current_idx<ntokens&&!r_definition(node);)
			{
				printf("Error\n");//
				skip_till_after(CT_SEMICOLON);
			}
	#ifdef DEBUG_PARSER
			debugprint(root->children.back());//
			print_names();
	#endif
		}
	}
	catch(...)
	//catch(const char*)
	{
		printf("Error: Unexpected end of file.\n");
	}
}
#undef			LOOK_AHEAD
#undef			LOOK_AHEAD_TOKEN
#undef			ADVANCE
#undef			ADVANCE_BY
#undef			INSERT_NONLEAF
#undef			INSERT_LEAF

void			dump_code(const char *filename){save_file(filename, true, code.data(), code.size());}

void			benchmark()
{
	prof.add("bm start");

	//benchmark code

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

	free_tree(root);
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