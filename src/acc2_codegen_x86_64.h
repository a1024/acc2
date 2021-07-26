#ifndef ACC2_CODEGEN_X86_64
#define ACC2_CODEGEN_X86_64
#include		"include/vector"
#include		"include/conio.h"
//https://www.youtube.com/watch?v=N9B2KeAWXgE	bitwise - compiler part 1
//https://www.youtube.com/watch?v=Mx29YQ4zAuM	bitwise - compiler part 2

typedef std::vector<byte> MachineCode;
extern MachineCode code;
extern int		code_idx;

enum			RegisterTypeX86
{
	EAX,
	ECX,
	EDX,
	EBX,
	ESP,
	EBP,
	ESI,
	EDI,
};
enum			RegisterTypeX86_64
{
	RAX,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
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
	//RIP,//pseudo-register
};
enum			Mode
{
	MODE_INDIRECT,
	MODE_INDIRECT_DISPLACED_BYTE,
	MODE_INDIRECT_DISPLACED,
	MODE_DIRECT,
};
enum			LogScale
{
	X1, X2, X4, X8,
};
enum			ConditionCode
{
	O,	//		overflow		//https://sandpile.org/x86/cc.htm
	NO,	//		no overflow
	B,	//u		below (not above or equal, carry)
	NB,	//u		not below (above or equal, no carry)
	E,	//u		equal (zero)
	NE,	//u		not equal (not zero)
	NA,	//u		not above (below or equal)
	A,	//u		above (not below or equal)
	S,	//		sign
	NS,	//		no sign
	P,	//		parity (parity even)
	NP,	//		no parity (parity odd)
	L,	//s		less than (not greater than or equal)
	NL,	//s		not less than (greater than or equal)
	NG,	//s		not greater than (less than or equal)
	G,	//s		greater than (not less than or equal)

	NAE=B, C=B,
	AE=NB, NC=NB,
	Z=E,
	NZ=NE,
	BE=NA,
	NBE=A,
	PE=P,
	PO=NP,
	NGE=L,
	GE=NL,
	LE=NG,
	NLE=G,
};

inline void		emit(byte c)
{
	code[code_idx]=c;
	++code_idx;
}
inline void		emit_modrm(byte mod, byte rx, byte rm)
{
	assert(mod<4);
	assert(rx<16);
	assert(rm<16);
	emit(mod<<6|(rx&7)<<3|(rm&7));
	//code[code_idx]=mod<<6|(rx&7)<<3|(rm&7);
	//++code_idx;
	//code.push_back(mod<<6|(rx&7)<<3|(rm&7));
}
inline void		emit4(i64 dword)
{
	auto p=(byte*)&dword;
	code[code_idx  ]=p[0];
	code[code_idx+1]=p[1];
	code[code_idx+2]=p[2];
	code[code_idx+3]=p[3];
	code_idx+=4;
	//code.push_back(p[0]);
	//code.push_back(p[1]);
	//code.push_back(p[2]);
	//code.push_back(p[3]);
}
inline void		emit8(unsigned long long qword)
{
	auto p=(byte*)&qword;
	code[code_idx  ]=p[0];
	code[code_idx+1]=p[1];
	code[code_idx+2]=p[2];
	code[code_idx+3]=p[3];
	code[code_idx+4]=p[4];
	code[code_idx+5]=p[5];
	code[code_idx+6]=p[6];
	code[code_idx+7]=p[7];
	code_idx+=8;
}

//indirect with no displacement			mod=0
//indirect with 1 byte displacement		mod=1
//indirect with multi-byte displacement	mod=2
//if indirect, base register goes into rm (operand), except rsp needs SIB encoding with base=4, index=4, scale=whatever
//direct								mod=3

//eg: add eax, ecx		rx=eax, operand=ecx		rx += operand
inline void		emit_direct(byte rx, byte operand)
{
	emit_modrm(MODE_DIRECT, rx, operand);
}
//eg: add eax, [ecx]	rx=eax, pointer=ecx
inline void		emit_indirect(byte rx, byte pointer)
{
	assert((pointer&7)!=RSP);
	assert((pointer&7)!=RBP);
	emit_modrm(MODE_INDIRECT, rx, pointer);
	//if(operand==EBP)
	//	emit4(0);
}
//eg: add eax, [12345678h]		rx=eax, displacement=12345678h
inline void		emit_displaced(byte rx, unsigned displacement)
{
	emit_modrm(MODE_INDIRECT, rx, RSP);//?
	emit_modrm(X1, RSP, RBP);//?
	emit4(displacement);

	//emit_modrm(MODE_INDIRECT, rx, EBP);
	//emit4(displacement);
}
//eg: add eax, [rip+12345678h]	rx=eax, displacement=12345678h
inline void		emit_indirect_displaced_rip(byte rx, unsigned displacement)//x64 only
{
	emit_modrm(MODE_INDIRECT, rx, EBP);
	emit4(displacement);
}
//eg: add eax, [12345678h]		rx=eax, displacement=12345678h
//inline void		emit_displaced_eip(byte rx, unsigned displacement)
//{
//	emit_modrm(MODE_INDIRECT, rx, EBP);
//	emit4(displacement);
//}

//eg: add eax, [rcx + 12h]	rx=eax, base=ecx, displacement=12h
inline void		emit_indirect_displaced_byte(byte rx, byte base, byte displacement)
{
	assert((base&7)!=RSP);
	emit_modrm(MODE_INDIRECT_DISPLACED_BYTE, rx, base);
	emit(displacement);
	//code[code_idx]=displacement;
	//++code_idx;
	//code.push_back(displacement);
}
//eg: add eax, [rcx + 12345678h]	rx=eax, base=ecx, displacement=12345678h
inline void		emit_indirect_displaced(byte rx, byte base, unsigned displacement)
{
	assert((base&7)!=RSP);
	emit_modrm(MODE_INDIRECT_DISPLACED, rx, base);
	emit4(displacement);
}
//eg: add eax, [ecx + 4*edx]	rx=eax, base=ecx, index=edx, scale=X4
inline void		emit_indirect_indexed(byte rx, byte base, byte index, LogScale logscale)
{
	assert(logscale<4);
	assert((base&7)!=EBP);
	emit_modrm(MODE_INDIRECT, rx, RSP);
	emit_modrm(logscale, index, base);
}
//eg: add eax, [ecx + 4*edx + 12h]			rx=eax, base=ecx, index=edx, scale=X4, displacement=12h
inline void		emit_indirect_indexed_displaced_byte(byte rx, byte base, byte index, LogScale logscale, byte displacement)
{
	emit_modrm(MODE_INDIRECT_DISPLACED_BYTE, rx, RSP);
	emit_modrm(logscale, index, base);
	emit(displacement);
	//code[code_idx]=displacement;
	//++code_idx;
	//code.push_back(displacement);
}
//eg: add eax, [ecx + 4*edx + 12345678h]	rx=eax, base=ecx, index=edx, scale=X4, displacement=12345678h
inline void		emit_indirect_indexed_displaced(byte rx, byte base, byte index, LogScale logscale, unsigned displacement)
{
	emit_modrm(MODE_INDIRECT_DISPLACED, rx, RSP);
	emit_modrm(logscale, index, base);
	emit4(displacement);
}

inline void		emit_rex_indexed(byte rx, byte base, byte index)//64bit only
{
	emit(0x48|(rx>>3)<<2|(index>>3)<<1|base>>3);
	//code[code_idx]=0x48|(rx>>3)<<2|(index>>3)<<1|base>>3;
	//++code_idx;
	//code.push_back(0x48|(rx>>3)<<2|(index>>3)<<1|base>>3);
}
inline void		emit_rex(byte rx, byte base)//64bit only
{
	emit(0x48|(rx>>3)<<2|base>>3);
	//code[code_idx]=0x48|(rx>>3)<<2|base>>3;
	//++code_idx;
	//code.push_back(0x48|(rx>>3)<<2|base>>3);
}

#define	EMIT_I(OP, IMM)						emit_rex(0, 0), emit_##OP##_I(), emit4(IMM)
#define	EMIT_C_I(OP, CC, IMM)				emit_##OP##_CI(CC), emit4(IMM)

#define	EMIT_R_R(OP, DST, SRC)				emit_rex(DST, SRC), emit_##OP##_R(), emit_direct(DST, SRC)
#define	EMIT_R_M(OP, DST, SRC)				emit_rex(DST, SRC), emit_##OP##_R(), emit_indirect(DST, SRC)
#define	EMIT_R_MD1(OP, DST, SRC, SRC_DISP)	emit_rex(DST, SRC), emit_##OP##_R(), emit_indirect_displaced_byte(DST, SRC, SRC_DISP)
#define	EMIT_R_MD(OP, DST, SRC, SRC_DISP)	emit_rex(DST, SRC), emit_##OP##_R(), emit_indirect_displaced(DST, SRC, SRC_DISP)
#define	EMIT_R_SIB(OP, DST, SRC_BASE, SRC_IDX, SRC_SCALE)				emit_rex_indexed(DST, SRC_BASE, SRC_IDX), emit_##OP##_R(), emit_indirect_indexed(DST, SRC_BASE, SRC_IDX, SRC_SCALE)
#define	EMIT_R_SIBD1(OP, DST, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)	emit_rex_indexed(DST, SRC_BASE, SRC_IDX), emit_##OP##_R(), emit_indirect_indexed_displaced_byte(DST, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)
#define	EMIT_R_SIBD(OP, DST, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)	emit_rex_indexed(DST, SRC_BASE, SRC_IDX), emit_##OP##_R(), emit_indirect_indexed_displaced(DST, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)
#define EMIT_R_RIPD(OP, DST, SRC_DISP)		emit_rex(DST, 0), emit_##OP##_R(), emit_indirect_displaced_rip(DST, SRC_DISP)
#define	EMIT_R_D(OP, DST, SRC_DISP)			emit_rex(DST, 0), emit_##OP##_R(), emit_displaced(DST, SRC_DISP)

#define	EMIT_M_R(OP, DST, SRC)				emit_rex(SRC, DST), emit_##OP##_M(), emit_direct(SRC, DST)
#define	EMIT_MD1_R(OP, DST, DST_DISP, SRC)	emit_rex(SRC, DST), emit_##OP##_M(), emit_indirect_displaced_byte(SRC, DST, DST_DISP)
#define	EMIT_MD_R(OP, DST, DST_DISP, SRC)	emit_rex(SRC, DST), emit_##OP##_M(), emit_indirect_displaced(SRC, DST, DST_DISP)
#define	EMIT_SIB_R(OP, DST_BASE, DST_IDX, DST_SCALE, SRC)				emit_rex_indexed(SRC, DST_BASE, DST_IDX), emit_##OP##_M(), emit_indirect_indexed(SRC, DST_BASE, DST_IDX, DST_SCALE)
#define	EMIT_SIBD1_R(OP, DST_BASE, DST_IDX, DST_SCALE, DST_DISP, SRC)	emit_rex_indexed(SRC, DST_BASE, DST_IDX), emit_##OP##_M(), emit_indirect_indexed_displaced_byte(SRC, DST_BASE, DST_IDX, DST_SCALE, DST_DISP)
#define	EMIT_SIBD_R(OP, DST_BASE, DST_IDX, DST_SCALE, DST_DISP, SRC)	emit_rex_indexed(SRC, DST_BASE, DST_IDX), emit_##OP##_M(), emit_indirect_indexed_displaced(SRC, DST_BASE, DST_IDX, DST_SCALE, DST_DISP)
#define EMIT_RIPD_R(OP, DST_DISP, SRC)		emit_rex(SRC, 0), emit_##OP##_M(), emit_indirect_displaced_rip(SRC, DST_DISP)
#define	EMIT_D_R(OP, DST_DISP, SRC)			emit_rex(SRC, 0), emit_##OP##_M(), emit_displaced(SRC, DST_DISP)

#define	EMIT_R_I(OP, DST, IMM)				emit_rex(0, DST), emit_##OP##_I(), emit_direct(extension_##OP##_I, DST), emit4(IMM)
#define	EMIT_M_I(OP, DST, IMM)				emit_rex(0, DST), emit_##OP##_I(), emit_indirect(extension_##OP##_I, DST), emit4(IMM)
#define	EMIT_MD1_I(OP, DST, DST_DISP, IMM)	emit_rex(0, DST), emit_##OP##_I(), emit_indirect_displaced_byte(extension_##OP##_I, DST, DST_DISP), emit4(IMM)
#define	EMIT_MD_I(OP, DST, DST_DISP, IMM)	emit_rex(0, DST), emit_##OP##_I(), emit_indirect_displaced(extension_##OP##_I, DST, DST_DISP), emit4(IMM)
#define	EMIT_SIB_I(OP, DST_BASE, DST_IDX, DST_SCALE, IMM)				emit_rex_indexed(0, DST_BASE, DST_IDX), emit_##OP##_I(), emit_indirect_indexed(extension_##OP##_I, DST_BASE, DST_IDX, DST_SCALE), emit4(IMM)
#define	EMIT_SIBD1_I(OP, DST_BASE, DST_IDX, DST_SCALE, DST_DISP, IMM)	emit_rex_indexed(0, DST_BASE, DST_IDX), emit_##OP##_I(), emit_indirect_indexed_displaced_byte(extension_##OP##_I, DST_BASE, DST_IDX, DST_SCALE, DST_DISP), emit4(IMM)
#define	EMIT_SIBD_I(OP, DST_BASE, DST_IDX, DST_SCALE, DST_DISP, IMM)	emit_rex_indexed(0, DST_BASE, DST_IDX), emit_##OP##_I(), emit_indirect_indexed_displaced(extension_##OP##_I, DST_BASE, DST_IDX, DST_SCALE, DST_DISP), emit4(IMM)
#define EMIT_RIPD_I(OP, DST_DISP, IMM)		emit_rex(0, 0), emit_##OP##_I(), emit_indirect_displaced_rip(extension_##OP##_I, DST_DISP), emit4(IMM)
#define	EMIT_D_I(OP, DST_DISP, IMM)			emit_rex(0, 0), emit_##OP##_I(), emit_displaced(extension_##OP##_I, DST_DISP), emit4(IMM)

#define	EMIT_X_R(OP, SRC)					emit_rex(0, SRC), emit_##OP##_X(), emit_direct(extension_##OP##_X, SRC)
#define	EMIT_X_M(OP, SRC)					emit_rex(0, SRC), emit_##OP##_X(), emit_indirect(extension_##OP##_X, SRC)
#define	EMIT_X_MD1(OP, SRC, SRC_DISP)		emit_rex(0, SRC), emit_##OP##_X(), emit_indirect_displaced_byte(extension_##OP##_X, SRC, SRC_DISP)
#define	EMIT_X_MD(OP, SRC, SRC_DISP)		emit_rex(0, SRC), emit_##OP##_X(), emit_indirect_displaced(extension_##OP##_X, SRC, SRC_DISP)
#define	EMIT_X_SIB(OP, SRC_BASE, SRC_IDX, SRC_SCALE)				emit_rex_indexed(0, SRC_BASE, SRC_IDX), emit_##OP##_X(), emit_indirect_indexed(extension_##OP##_X, SRC_BASE, SRC_IDX, SRC_SCALE)
#define	EMIT_X_SIBD1(OP, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)	emit_rex_indexed(0, SRC_BASE, SRC_IDX), emit_##OP##_X(), emit_indirect_indexed_displaced_byte(extension_##OP##_X, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)
#define	EMIT_X_SIBD(OP, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)		emit_rex_indexed(0, SRC_BASE, SRC_IDX), emit_##OP##_X(), emit_indirect_indexed_displaced(extension_##OP##_X, SRC_BASE, SRC_IDX, SRC_SCALE, SRC_DISP)
#define EMIT_X_RIPD(OP, SRC_DISP)			emit_rex(0, 0), emit_##OP##_X(), emit_indirect_displaced_rip(extension_##OP##_X, SRC_DISP)
#define	EMIT_X_D(OP, SRC_DISP)				emit_rex(0, 0), emit_##OP##_X(), emit_displaced(extension_##OP##_X, SRC_DISP)

#define	EMIT_MOV_R_I(DST, IMM)				emit_rex(0, DST), emit(0xB8+((DST)&7)), emit8(IMM)
#define	EMIT_MOV_RAX_OFF(SRC_IMM)			emit_rex(0, 0), emit(0xA1), emit8(SRC_IMM)
#define	EMIT_MOV_OFF_RAX(DST_IMM)			emit_rex(0, 0), emit(0xA1), emit8(DST_IMM)

#define OP1R(OP, CODE)			inline void emit_##OP##_R(){emit(CODE);}
#define OP1M(OP, CODE)			inline void emit_##OP##_M(){emit(CODE);}
#define OP1I(OP, CODE, EXT)		inline void emit_##OP##_I(){emit(CODE);} enum{extension_##OP##_I=EXT};
#define OP1X(OP, CODE, EXT)		inline void emit_##OP##_X(){emit(CODE);} enum{extension_##OP##_X=EXT};
#define OP1CI1(OP, CODE)		inline void emit_##OP##_CI(byte cc){emit(CODE+cc);}
//#define OP1CI1(OP, CODE, EXT)	inline void emit_##OP##_CI(byte cc){emit(CODE+cc);} enum{extension_##OP##_CI=EXT};
#define OP2CI(OP, CODE)			inline void emit_##OP##_CI(byte cc){emit(0x0F), emit(CODE+cc);}

//Instructions
OP1R(MOV, 0x8B)
OP1M(MOV, 0x89)

OP1R(XCHG, 0x87)
OP1M(XCHG, 0x87)

OP1R(ADD, 0x03)
OP1M(ADD, 0x01)
OP1I(ADD, 0x81, 0x00)

OP1R(SUB, 0x2B)
OP1M(SUB, 0x29)
OP1I(SUB, 0x81, 0x05)

OP1R(AND, 0x23)
OP1M(AND, 0x21)
OP1I(AND, 0x81, 0x04)

OP1X(MUL, 0xF7, 0x04)

OP1X(DIV, 0xF7, 0x06)

OP1I(JMP, 0xE9, 0x00)
//OP2CI1(J, 0x70)
OP2CI(J, 0x80)
#endif