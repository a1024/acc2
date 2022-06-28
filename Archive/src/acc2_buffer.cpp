#include		"acc2.h"
#include		"include/stdio.h"

const int		g_buf_size=G_BUF_SIZE;
char			g_buf[G_BUF_SIZE]={};

const char*		describe_char(char c)//uses g_buf
{
	static int idx=0;
	switch(c)
	{
	case '\0':
		return "<0: null terminator>";
	case '\a':
		return "<7: alert>";
	case '\b':
		return "<8: backspace>";
	case '\t':
		return "<9: tab>";
	case '\n':
		return "<10: newline>";
	case '\r':
		return "<13: carriage return>";
	case ' ':
		return "<32: space>";
	case 127:
		return "<127: delete>";
	default:
		{
			int i0=idx;
			//printf("describe_char: %02X = %c\n", (int)c, (int)c);//
			if(idx+2>=G_BUF_SIZE)
				idx=0;
			idx+=sprintf_s(g_buf+idx, g_buf_size-idx, "%c", c);
			++idx;
			return g_buf+i0;
		}
	}
}

void			print_buffer(const void *buffer, int bytesize)
{
	const char *b2=(const char*)buffer;
	for(int k=0;k<bytesize-1;++k)
		printf("%02X-", (unsigned)b2[k]&0xFF);
	printf("%02X\n", (unsigned)b2[bytesize-1]&0xFF);
}

//number reader
long long		acme_read_integer(const char *text, int size, int base, int start, int *ret_end)
{
	int k;
	byte temp;
	long long ival=0;
	int hex=-(base==16);

	for(k=start;k<size;++k)
	{
		temp=text[k];
		byte c=temp-'0';
		if(c<10)
		{
			ival*=base;
			ival+=c;
			continue;
		}
		else if(hex&&(c=(temp&0xDF)-'A', c<6))
		{
			ival*=base;
			ival+=10+c;
			//ival+=10+temp-'A';
			continue;
		}
		//else if(hex&(temp-'a'<6))
		//{
		//	ival*=base;
		//	ival+=10+temp-'a';
		//	continue;
		//}
		else if(temp=='\'')//allow quote as separator
			continue;
		break;
	}
	*ret_end=k;
	return ival;
}
static double	acme_read_tail(const char *text, int size, double inv_base, int start, int *ret_end)
{
	int k;
	byte temp;
	double fval=0;

	for(*ret_end=start;*ret_end<size&&(text[*ret_end]>='0'&&text[*ret_end]<='9'||text[*ret_end]>='A'&&text[*ret_end]<='F'||text[*ret_end]>='a'&&text[*ret_end]<='f'||text[*ret_end]=='\'');++*ret_end);
	for(k=*ret_end-1;k>=start;--k)
	{
		temp=text[k];
		byte c=temp-'0';
		if(c<10)
			fval+=c;
		else if(c=(temp&0xDF)-'A', c<6)
			fval+=c+10;
		//else if(temp>='A'&&temp<='F')
		//	fval+=temp-'A'+10;
		//else if(temp>='a'&&temp<='f')
		//	fval+=temp-'a'+10;
		else if(temp=='\'')//allow quote as separator
			continue;
		fval*=inv_base;
	}
	return fval;
}
static char		acme_read_number_pt2(const char *text, int size, int base, double invbase, int start, int *advance, long long *ival, double *fval)
{
	char isfloat=0, neg_exponent;
	int end;
	int exponent;
	char temp;

	*ival=acme_read_integer(text, size, base, start, &end);
	if(end<size&&text[end]=='.')
	{
		int start2=end+1;
		*fval=acme_read_tail(text, size, invbase, start2, &end);
		*fval+=(double)*ival;
		isfloat=1;
	}
	if(end+1<size)
	{
		temp=text[end]&0xDF;
		if(base==10&&temp=='E'||temp=='P')
		{
			start=end+1;
			neg_exponent=text[start]=='-';
			start+=text[start]=='-'||text[start]=='+';

			exponent=(int)acme_read_integer(text, size, base, start, &end);
			if(!isfloat)
				*fval=(double)*ival;
			if(neg_exponent)
				exponent=-exponent;
			*fval*=power((double)base, exponent);
			isfloat=1;
		}
	}
	*advance=end-start;
	return isfloat;
}
char			acme_read_number(const char *text, int size, int k, int *advance, long long *ival, double *fval)//returns  0: integer, 1: float, 2: error
{
	char temp, isfloat;

	assert(ival&&fval);

	isfloat=0;
	*ival=0;
	if(text[k]=='0'&&(k+1==size||is_alphanumeric(text[k+1])))//identify base prefix
	{
		temp=text[k+1]&0xDF;
		if(temp=='X')//hex
			isfloat=acme_read_number_pt2(text, size, 16, 1./16, k+2, advance, ival, fval);
		else if(temp=='B')//bin
			isfloat=acme_read_number_pt2(text, size, 2, 1./2, k+2, advance, ival, fval);
		else if(text[k+1]>='0'&&text[k+1]<='7')//oct	//TODO: "09" should generate an error
			isfloat=acme_read_number_pt2(text, size, 8, 1./8, k+1, advance, ival, fval);
		else if(text[k+1]=='.')//dec	//eg: "0.1"
			isfloat=acme_read_number_pt2(text, size, 10, 1./10, k, advance, ival, fval);
		else
			isfloat=2;
	}
	else//dec
		isfloat=acme_read_number_pt2(text, size, 10, 1./10, k, advance, ival, fval);
	return isfloat;
}

//struct printer
enum			DataType
{
	D_CHAR, D_UCHAR,
	D_SHORT, D_USHORT,
	D_INT, D_UINT,
	D_LONG, D_ULONG,
	D_FLOAT, D_DOUBLE,
	D_M128,
	D_POINTER,
	//D_STRING,
};
struct			MemberInfo
{
	DataType type;
	int count;
	//void set(DataType type, int count){this->type=type, this->count=count;}
};
#define			MAX_MEMBERS	1024
MemberInfo		dt_buf[MAX_MEMBERS];
static unsigned	align_idx(unsigned idx, int pot_size)
{
	auto mask=pot_size-1;
	idx=(idx&~mask)+(pot_size&-((idx&mask)!=0));
	return idx;
}
//static const void* align_pointer_const(const void* p, int pot_size)
//{
//	auto val=(unsigned)p;
//	auto mask=pot_size-1;
//	val=(val&~mask)+(pot_size&-((val&mask)!=0));
//	return (void*)val;
//}
static void		print_struct_append(int &bytesize, int &biggestmember, int newmemberbytesize, int count)
{
	int lognewmsize=floor_log2(newmemberbytesize), msizemask=newmemberbytesize-1;
	bytesize=(bytesize&~msizemask)+(((bytesize&msizemask)!=0)<<lognewmsize);//align struct size
	bytesize+=newmemberbytesize*count;//add member(s)
	if(biggestmember<newmemberbytesize)
		biggestmember=newmemberbytesize;
}
//print struct: object description
//p: pointer
//c: char
//C: unsigned char
//s: short
//S: unsigned short
//i: int
//I: unsigned
//l: long long
//L: unsigned long long
//f: float
//d: double
void			print_struct(const void *objects, const char *desc, int count, const char **mnames)
{
	int nmembers=0;
	int desc_len=strlen(desc);
	int bytesize=0, biggestmember=1;
	for(int k=0;desc[k];)//interpret description
	{
		char c=desc[k];
		int size=1;
		switch(c)
		{
		case 'p':
			dt_buf[nmembers].type=D_POINTER, size=sizeof(void*);
			break;
		case 'c':case 'C':
			dt_buf[nmembers].type=DataType(D_CHAR+(c=='C')), size=sizeof(char);
			break;
		case 's':case 'S':
			dt_buf[nmembers].type=DataType(D_SHORT+(c=='C')), size=sizeof(short);
			break;
		case 'i':case 'I':
			dt_buf[nmembers].type=DataType(D_INT+(c=='C')), size=sizeof(int);
			break;
		case 'l':case 'L':
			dt_buf[nmembers].type=DataType(D_LONG+(c=='C')), size=sizeof(long long);
			break;
		case 'f':
			dt_buf[nmembers].type=D_FLOAT, size=sizeof(float);
			break;
		case 'd':
			dt_buf[nmembers].type=D_DOUBLE, size=sizeof(double);
			break;
		case 'm':
			dt_buf[nmembers].type=D_M128, size=16;
			break;
		}
		int advance=0;
		long long ival=0;
		double fval=0;
		char ret=acme_read_number(desc, desc_len, k+1, &advance, &ival, &fval);
		if(!ival)
			dt_buf[nmembers].count=1;
		else
			dt_buf[nmembers].count=(int)ival;
		print_struct_append(bytesize, biggestmember, size, dt_buf[nmembers].count);
		++nmembers;
		if(nmembers>=MAX_MEMBERS)
			return;
		k+=1+advance;
	}
	int logbigmsize=floor_log2(biggestmember), bigmask=biggestmember-1;
	bytesize=(bytesize&~bigmask)+(((bytesize&bigmask)!=0)<<logbigmsize);//align struct size
#if 0
	for(int k=0;desc[k];++k)//interpret description
	{
		switch(desc[k])
		{
		case 'i':case 'u':
			if(desc[k+1]=='8')
			{
				dt_buf[nmembers]=D_CHAR+(desc[k]=='u');
				print_struct_append(bytesize, biggestmember, 1, 1);
				++k;
			}
			else if(desc[k+1]=='1'&&desc[k+2]=='6')
			{
				dt_buf[nmembers]=D_SHORT+(desc[k]=='u');
				print_struct_append(bytesize, biggestmember, sizeof(short), 1);
				k+=2;
			}
			else if(desc[k+1]=='3'&&desc[k+2]=='2')
			{
				dt_buf[nmembers]=D_INT+(desc[k]=='u');
				print_struct_append(bytesize, biggestmember, sizeof(int), 1);
				k+=2;
			}
			else if(desc[k+1]=='6'&&desc[k+2]=='4')
			{
				dt_buf[nmembers]=D_LONG+(desc[k]=='u');
				print_struct_append(bytesize, biggestmember, sizeof(long long), 1);
				k+=2;
			}
			break;
		case 'f':
			dt_buf[nmembers]=D_FLOAT;
			print_struct_append(bytesize, biggestmember, sizeof(float), 1);
			break;
		case 'd':
			dt_buf[nmembers]=D_DOUBLE;
			print_struct_append(bytesize, biggestmember, sizeof(double), 1);
			break;
		case 'p':
			dt_buf[nmembers]=D_POINTER;
			print_struct_append(bytesize, biggestmember, sizeof(void*), 1);
			break;
		case 'c':
			{
				++k;
				auto val=acme_read_integer(desc, desc_len, 10, k, &k);
				print_struct_append(bytesize, biggestmember, 1, val);
				dt_buf[nmembers]=D_STRING+val;
			}
			break;
		}
		++nmembers;
	}
#endif
	auto buffer=(const char*)objects;
	for(int ko=0, idx=0;ko<count;++ko)//object loop
	{
		printf("Object %d/%d:\n", ko+1, count);
		for(int km=0;km<nmembers;++km)//member loop
		{
			if(mnames)
				printf("%s: ", mnames[km]);
			auto &info=dt_buf[km];
			switch(info.type)
			{
			case D_POINTER:
				idx=align_idx(idx, sizeof(void*));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(void*))
					printf("\t0x%p\n", *(void**)(buffer+idx));
				//pointer=(const char*)align_pointer_const(pointer, sizeof(void*));
				//for(int ke=0;ke<info.count;++ke, pointer+=sizeof(void*))
				//	printf("\t0x%p\n", *(void**)pointer);
				break;
			case D_CHAR:
				{
					printf("\t");
					auto b0=buffer+idx;
					int len=0;
					char passed_null=false, valid=true;
					for(int ke=0;ke<info.count-1;++ke, ++idx)
					{
						int val=buffer[idx]&0xFF;
						printf("%02X-", val);
						len+=val&&!passed_null;
						valid=val&&passed_null;
						passed_null|=!val;
					}
					int val=*(buffer+idx)&0xFF;
					printf("%02X\n", val);
					++idx;
					if(valid)
						printf("\t%.*s\n\n", len, b0);
				}
				//for(int ke=0;ke<info.count;++ke, ++idx)
				//	printf("\t0x%02X = %s\n", (unsigned)*(buffer+idx)&0xFF, describe_char(*(buffer+idx)));
				break;
			case D_UCHAR:
				for(int ke=0;ke<info.count;++ke, ++idx)
				{
					int val=buffer[idx]&0xFF;
					printf("\t0x%02X = %d\n", val, val);
				}
				break;
			case D_SHORT:
				idx=align_idx(idx, sizeof(short));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(short))
					printf("\t0x%04X = %d\n", (int)*(short*)(buffer+idx)&0xFFFF, (int)*(short*)(buffer+idx));
				break;
			case D_USHORT:
				idx=align_idx(idx, sizeof(short));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(short))
				{
					int val=(unsigned)*(short*)(buffer+idx)&0xFFFF;
					printf("\t0x%04X = %ud\n", val, val);
				}
				break;
			case D_INT:
				idx=align_idx(idx, sizeof(int));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(int))
				{
					int val=*(int*)(buffer+idx);
					printf("\t0x%08X = %d\n", val, val);
				}
				break;
			case D_UINT:
				idx=align_idx(idx, sizeof(int));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(int))
				{
					unsigned val=*(unsigned*)(buffer+idx);
					printf("\t0x%08X = %ud\n", val, val);
				}
				break;
			case D_LONG:
				idx=align_idx(idx, sizeof(long long));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(long long))
				{
					auto lval=*(long long*)(buffer+idx);
					printf("\t0x%016llX = %lld\n", lval, lval);
				}
				break;
			case D_ULONG:
				idx=align_idx(idx, sizeof(long long));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(long long))
				{
					auto val=*(unsigned long long*)(buffer+idx);
					printf("\t0x%016llX = %llud\n", val, val);
				}
				break;
			case D_FLOAT:
				idx=align_idx(idx, sizeof(float));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(float))
				{
					auto &fval=*(float*)(buffer+idx);
					printf("\t0x%08X = %g\n", (unsigned&)fval, (double)fval);
				}
				break;
			case D_DOUBLE:
				idx=align_idx(idx, sizeof(double));
				for(int ke=0;ke<info.count;++ke, idx+=sizeof(double))
				{
					auto &fval=*(double*)(buffer+idx);
					printf("\t0x%016llX = %g\n", (long long&)fval, fval);
				}
				break;
			case D_M128:
				idx=align_idx(idx, 16);
				for(int ke=0;ke<info.count;++ke, idx+=16)
				{
					auto b0=buffer+idx;
					printf("\n\t");
					for(int kc=15;kc>=0;--kc)
						printf("%02X", kc);
					printf("\n");

					printf("\t");
					for(int kc=15;kc>=0;--kc)
						printf("%02X", b0[kc]&0xFF);
					printf("\n");

					printf("\t");
					for(int kc=3;kc>=0;--kc)
						printf("%g ", (double)((float*)b0)[kc]);
					printf("\n");

					printf("\t");
					for(int kc=1;kc>=0;--kc)
						printf("%g ", ((double*)b0)[kc]);
					printf("\n");

					printf("\t");
					for(int kc=0;kc<16;++kc)
						printf("%c", b0[kc]);
					printf("\n\n");
				}
				break;
			}
		}
		printf("\n");
	}
	printf("\n");
}

int				codepoint2utf8(int codepoint, char *out)//returns sequence length
{
	//https://stackoverflow.com/questions/6240055/manually-converting-unicode-codepoints-into-utf-8-and-utf-16
	if(codepoint<0)
	{
		printf("Invalid Unicode codepoint 0x%08X\n", codepoint);
		return 0;
	}
	if(codepoint<0x80)
	{
		out[0]=codepoint;
		return 1;
	}
	if(codepoint<0x800)
	{
		out[0]=0xC0|(codepoint>>6)&0x1F, out[1]=0x80|codepoint&0x3F;
		return 2;
	}
	if(codepoint<0x10000)
	{
		out[0]=0xE0|(codepoint>>12)&0x0F, out[1]=0x80|(codepoint>>6)&0x3F, out[2]=0x80|codepoint&0x3F;
		return 3;
	}
	if(codepoint<0x200000)
	{
		out[0]=0xF0|(codepoint>>18)&0x07, out[1]=0x80|(codepoint>>12)&0x3F, out[2]=0x80|(codepoint>>6)&0x3F, out[3]=0x80|codepoint&0x3F;
		return 4;
	}
	printf("Invalid Unicode codepoint 0x%08X\n", codepoint);
	return 0;
}
int				utf8codepoint(const char *in, int &codepoint)//returns sequence length
{
	char c=in[0];
	if(c>=0)
	{
		codepoint=c;
		return 1;
	}
	if((c&0xE0)==0xC0)
	{
		char c2=in[1];
		if((c2&0xC0)==0x80)
			codepoint=(c&0x1F)<<6|(c2&0x3F);
		else
		{
			codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X\n", (int)c, (int)c2);
		}
		return 2;
	}
	if((c&0xF0)==0xE0)
	{
		char c2=in[1], c3=in[2];
		if((c2&0xC0)==0x80&&(c3&0xC0)==0x80)
			codepoint=(c&0x0F)<<12|(c2&0x3F)<<6|(c3&0x3F);
		else
		{
			codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X-%02X\n", (int)c, (int)c2, (int)c3);
		}
		return 3;
	}
	if((c&0xF8)==0xF0)
	{
		char c2=in[1], c3=in[2], c4=in[3];
		if((c2&0xC0)==0x80&&(c3&0xC0)==0x80&&(c4&0xC0)==0x80)
			codepoint=(c&0x07)<<18|(c2&0x3F)<<12|(c3&0x3F)<<6|(c4&0x3F);
		else
		{
			codepoint='?';
			printf("Invalid UTF-8 sequence: %02X-%02X-%02X-%02X\n", (int)c, (int)c2, (int)c3, (int)c4);
		}
		return 4;
	}
	codepoint='?';
	printf("Invalid UTF-8 sequence: %02X\n", (int)c);
	return 1;
}

//escape sequences -> raw string
void			esc2str(const char *s, int size, std::string &out)
{
	if(!s)
	{
		printf("Internal error: esc2str: string is null.\n");
		return;
	}
	char utf8_seq[5]={};
	out.resize(size);
	int kd=0;
	for(int ks=0;ks<size;++ks, ++kd)
	{
		if(s[ks]!='\\')
			out[kd]=s[ks];
		else
		{
			++ks;
			switch(s[ks])
			{
			case 'a':	out[kd]='\a';break;
			case 'b':	out[kd]='\b';break;
			case 'f':	out[kd]='\f';break;
			case 'n':	out[kd]='\n';break;
			case 'r':	out[kd]='\r';break;
			case 't':	out[kd]='\t';break;
			case 'v':	out[kd]='\v';break;
			case '\'':	out[kd]='\'';break;
			case '\"':	out[kd]='\"';break;
			case '\\':	out[kd]='\\';break;
			case '\?':	out[kd]='?';break;
			case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'://dec codepoint	SHOULD BE OCTAL
			case 'x':case 'u'://hex codepoint
				{
					int hex=s[ks]=='x'||s[ks]=='u';
					ks+=hex;
					int end=0;
					auto val=acme_read_integer(s, size, hex?16:10, ks, &end);
					int seqlen=codepoint2utf8((int)val, utf8_seq);
					out.resize(out.size()+seqlen-1);
					memcpy(out.data()+kd, utf8_seq, seqlen);
					ks=end, kd+=seqlen-1;
				}
				break;
			default:
				out[kd]='?';
				printf("Unsupported escape sequence at %d: \\%c\n", ks, s[ks]);
				break;
			}
		}
	}
	out.resize(kd);
}
//raw string -> escape sequences
void			str2esc(const char *s, int size, std::string &out)
{
	if(!s)
	{
		printf("Internal error: str2esc: string is null.\n");
		return;
	}
	if(!size)
		size=strlen(s);
	char codestr[16];
	out.resize(size);
	int kd=0;
	for(int ks=0;ks<size;++ks, ++kd)
	{
		char c=s[ks];
		switch(c)
		{
		case '\0':
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case '\'':
		case '\"':
		case '\\':
			out.resize(out.size()+1);
			out[kd]='\\';
			switch(c)
			{
			case '\0':out[kd+1]='0';break;
			case '\a':out[kd+1]='a';break;
			case '\b':out[kd+1]='b';break;
			case '\f':out[kd+1]='f';break;
			case '\n':out[kd+1]='n';break;
			case '\r':out[kd+1]='r';break;
			case '\t':out[kd+1]='t';break;
			case '\v':out[kd+1]='v';break;
			case '\'':out[kd+1]='\'';break;
			case '\"':out[kd+1]='\"';break;
			case '\\':out[kd+1]='\\';break;
			}
			++kd;
			break;
		default:
			if(c>=' '&&c<0x7F)
				out[kd]=s[ks];
			else
			{
				int codepoint=0;
				int seqlen=utf8codepoint(s+ks, codepoint);
				int codelen=sprintf_s(codestr, 16, "\\u%04X", codepoint);
				out.replace(kd, 1, codestr, codelen);
				ks+=seqlen-1, kd+=codelen-1;
			}
			break;
		}
	}
}