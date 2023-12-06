#include"acc2.h"
static const char file[]=__FILE__;

typedef enum RegisterIdxEnum
{
	RAX, RCX, RDX, RBX,
	RSP, RBP, RSI, RDI,
	R8,  R9,  R10, R11,
	R12, R13, R14, R15,
	//RIP,//pseudo-register
} RegisterIdx;