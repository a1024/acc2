#include		"acc2.h"
#include		"acc2_codegen_x86_64.h"
int				code_idx=0;
MachineCode		code;


//experimental
static void		append_write_binary(std::string &text, int n, int nbits)
{
	if(nbits>0)
	{
		int size=text.size()+2;
		text.resize(size+nbits);
		text[size-2]='0';
		text[size-1]='b';
		//text+="0b";
		for(int idx=0, k=nbits-1;k>=0;++idx, --k)
			text[size+idx]='0'+(n>>k&1);
	}
}
void			pause()
{
	char c=_getch();
	if((c&0xDF)=='X')
	{
		printf("Press 'X' again to quit.\n");
		c=_getch();
		if((c&0xDF)=='X')
			exit(0);
	}
}
void			codegen_test()
{
	char c=0;
	printf("Code generation test\n");
	printf("\tx: quit\n");
	printf("Generating test instructions...\n");
	code.resize(4096);
	code_idx=0;
	//code.clear();
#if 0
	for(int dst=0;dst<16;++dst)//x86 (32bit)
	{
		for(int src=0;src<16;++src)
		{
#if 1
			emit_add_to_reg();
			emit_direct(dst, src);
			emit_add_to_reg();
			emit_indirect_indexed(dst, src, dst, SCALE_X4);
			if(src!=ESP&&src!=EBP)
			{
				emit_add_to_reg();
				emit_indirect(dst, src);
				emit_add_to_reg();
				emit_indirect_displaced_byte(dst, src, 0x12);
				emit_add_to_reg();
				emit_indirect_displaced(dst, src, 0x12345678);
			}
#endif
#if 0
			emit_add_reversed();
			emit_direct(dst, src);
			if(src!=ESP&&src!=EBP)
			{
				emit_add_reversed();
				emit_indirect(dst, src);
				emit_add_reversed();
				emit_indirect_displaced_byte(dst, src, 0x12);
				emit_add_reversed();
				emit_indirect_displaced(dst, src, 0x12345678);
			}
#endif
		}
	}
#endif
#if 1
	EMIT_MOV_R_I(RBX, 0x0123456789ABCDEF);
	EMIT_MOV_RAX_OFF(0x0123456789ABCDEF);
	EMIT_MOV_OFF_RAX(0x0123456789ABCDEF);
	EMIT_I(JMP, 0x1234);
	EMIT_C_I(J, NZ, 0x1234);
	EMIT_D_I(ADD, 0x12345678, 0xDEADBEEF);
	EMIT_RIPD_I(ADD, 0x12345678, 0xDEADBEEF);
	for(int dst=0;dst<16;++dst)//x64
	{
		EMIT_X_R(MUL, dst);
		if((dst&7)!=RSP)
		{
			EMIT_X_MD1(MUL, dst, 0x12);
			EMIT_X_MD(MUL, dst, 0x12345678);
			if((dst&7)!=RBP)
			{
				EMIT_X_M(MUL, dst);
				EMIT_X_SIB(MUL, dst, R8, X4);
				EMIT_X_SIBD1(MUL, dst, R8, X4, 0x12);
				EMIT_X_SIBD(MUL, dst, R8, X4, 0x12345678);
			}
		}
		EMIT_R_I(ADD, dst, 0xDEADBEEF);
		if((dst&7)!=RSP)
		{
			EMIT_MD1_I(ADD, dst, 0x12, 0xDEADBEEF);
			EMIT_MD_I(ADD, dst, 0x12345678, 0xDEADBEEF);
			if((dst&7)!=RBP)
				EMIT_SIB_I(ADD, dst, R8, X4, 0xDEADBEEF);
		}
		EMIT_R_RIPD(ADD, dst, 0x12345678);
		EMIT_R_D(ADD, dst, 0x12345678);
		EMIT_RIPD_R(ADD, 0x12345678, dst);
		EMIT_D_R(ADD, 0x12345678, dst);
		//emit_rex(dst, 0);
		//emit_add_to_reg();
		//emit_indirect_displaced_rip(dst, 0x12345678);
		//
		//emit_rex(dst, 0);
		//emit_add_to_reg();
		//emit_displaced(dst, 0x12345678);
		for(int src=0;src<16;++src)
		{
			if(code_idx+1000>=(int)code.size())
				code.resize(code.size()+1000);
			EMIT_R_R(ADD, dst, src);
			//emit_rex(dst, src);
			//emit_add_to_reg();
			//emit_direct(dst, src);

			if((src&7)!=RBP)
			{
				EMIT_R_SIB(ADD, dst, src, dst, X4);
				EMIT_R_SIBD1(ADD, dst, src, dst, X4, 0x12);
				EMIT_R_SIBD(ADD, dst, src, dst, X4, 0x12345678);
			}
			//else
			//	EMIT_R_SIBD1(ADD, dst, src, dst, X4, 0);

			//emit_rex_indexed(dst, src, dst);
			//emit_add_to_reg();
			//if((src&7)==RBP)
			//	emit_indirect_indexed_displaced_byte(dst, src, dst, X4, 0);
			//else
			//	emit_indirect_indexed(dst, src, dst, X4);
			
			if((dst&7)!=RBP)
			{
				EMIT_SIB_R(ADD, dst, src, X4, src);
				EMIT_SIBD1_R(ADD, dst, src, X4, 0x12, src);
				EMIT_SIBD_R(ADD, dst, src, X4, 0x12345678, src);
			}
			//else
			//	EMIT_SIBD1_R(ADD, dst, X4, src, src, 0);

			if((src&7)!=RSP&&(src&7)!=RBP)
			{
				EMIT_R_M(ADD, dst, src);
				//emit_rex(dst, src);
				//emit_add_to_reg();
				//emit_indirect(dst, src);
				
				EMIT_R_MD1(ADD, dst, src, 0x12);
				//emit_rex(dst, src);
				//emit_add_to_reg();
				//emit_indirect_displaced_byte(dst, src, 0x12);
				
				EMIT_R_MD(ADD, dst, src, 0x12345678);
				//emit_rex(dst, src);
				//emit_add_to_reg();
				//emit_indirect_displaced(dst, src, 0x12345678);
			}
			if((dst&7)!=RSP&&(dst&7)!=RBP)
			{
				EMIT_M_R(ADD, dst, src);
				EMIT_MD1_R(ADD, dst, 0x12, src);
				EMIT_MD_R(ADD, dst, 0x12345678, src);
			}
			if((src&7)==RSP)
				EMIT_R_SIB(ADD, dst, src, RSP, X1);
			//{
			//	emit_rex(dst, src);
			//	emit_add_to_reg();
			//	emit_indirect_indexed(dst, src, RSP, X1);
			//}
		}
	}
#endif
	code.resize(code_idx);
	printf("Press a key to save binary\n");
	pause();
	save_file("D:/C/ACC2/codegen.bin", true, code.data(), code.size());
	
#if 0
	printf("Writing test instructions...\n");
	std::string text;
	for(int k=0;k<(int)code.size();++k)
	{
		sprintf_s(g_buf, g_buf_size, "%02X-", (int)code[k]);
		text+=g_buf;
		if(!(k&31))
			text+='\n';
	}
	//for(int k=0;k<(int)code.size();k+=2)//fixed 2-byte instructions only
	//{
	//	sprintf_s(g_buf, g_buf_size, "0x%02X 0x%02X\t", (int)code[k], (int)code[k+1]);
	//	text+=g_buf;
	//	append_write_binary(text, code[k], 8);
	//	text+='\t';
	//	append_write_binary(text, code[k+1], 8);
	//	text+='\n';
	//}
	printf("Press a key to save text\n");
	pause();
	printf("Saving...\n");
	save_file("D:/C/ACC2/codegen.txt", false, text.c_str(), text.size());
#endif
	printf("\nDONE.\nPress a key to continue with the COMPILER.\n\n");
	pause();
}