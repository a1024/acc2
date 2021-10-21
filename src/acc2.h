#ifndef ACC2_H
#define ACC2_H
#include		"include/stdio.h"
#include		"include/string"
#include		"include/set"
#include		"include/map"
#include		"include/intrin.h"

	//TODO: compile correctly on 64 bit too

#define			SIZEOF(STATIC_ARRAY)	(sizeof(STATIC_ARRAY)/sizeof(*STATIC_ARRAY))
typedef long long i64;

#define				G_BUF_SIZE	2048
extern const int	g_buf_size;
extern char			g_buf[G_BUF_SIZE];

//windows
typedef union		_LARGE_INTEGER
{
	struct
	{
		unsigned LowPart;
		int HighPart;
	};
	long long QuadPart;
} LARGE_INTEGER;
extern "C"
{
	int __stdcall QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
	int __stdcall QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
}


//[PROFILER]
#if 1
//comment or uncomment
//	#define PROFILER_CYCLES

//select one or nothing (profiler on screen)
//	#define PROFILER_TITLE
	#define PROFILER_CMD

//comment or uncomment
//	#define PROFILER_CLIPBOARD

//select one
	#define TIMING_USE_QueryPerformanceCounter
//	#define	TIMING_USE_rdtsc
//	#define TIMING_USE_GetProcessTimes	//~16ms resolution
//	#define TIMING_USE_GetTickCount		//~16ms resolution
//	#define TIMING_USE_timeGetTime		//~16ms resolution

//END OF [PROFILER SETTINGS]


//time-measuring functions
#ifdef __GNUC__
#define	__rdtsc	__builtin_ia32_rdtsc
#endif
inline double		time_sec()
{
#ifdef TIMING_USE_QueryPerformanceCounter
	static long long t=0;
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	t=li.QuadPart;
	QueryPerformanceCounter(&li);
	return (double)li.QuadPart/t;
#elif defined TIMING_USE_rdtsc
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	return (double)__rdtsc()*0.001/li.QuadPart;//pre-multiplied by 1000
#elif defined TIMING_USE_GetProcessTimes
	FILETIME create, exit, kernel, user;
	int success=GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user);
	if(success)
//#ifdef PROFILER_CYCLES
	{
		const auto hns2sec=100e-9;
		return hns2ms*(unsigned long long&)user;
	//	return hns2ms*(unsigned long long&)kernel;
	}
//#else
//	{
//		SYSTEMTIME t;
//		success=FileTimeToSystemTime(&user, &t);
//		if(success)
//			return t.wHour*3600000.+t.wMinute*60000.+t.wSecond*1000.+t.wMilliseconds;
//		//	return t.wHour*3600.+t.wMinute*60.+t.wSecond+t.wMilliseconds*0.001;
//	}
//#endif
	SYS_CHECK();
	return -1;
#elif defined TIMING_USE_GetTickCount
	return (double)GetTickCount()*0.001;//the number of milliseconds that have elapsed since the system was started
#elif defined TIMING_USE_timeGetTime
	return (double)timeGetTime()*0.001;//system time, in milliseconds
#endif
}
inline double		time_ms()
{
#ifdef TIMING_USE_QueryPerformanceCounter
	static long long t=0;
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	t=li.QuadPart;
	QueryPerformanceCounter(&li);
	return (double)li.QuadPart*1000./t;
#elif defined TIMING_USE_rdtsc
	static LARGE_INTEGER li={};
	QueryPerformanceFrequency(&li);
	return (double)__rdtsc()/li.QuadPart;//pre-multiplied by 1000
#elif defined TIMING_USE_GetProcessTimes
	FILETIME create, exit, kernel, user;
	int success=GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user);
	if(success)
//#ifdef PROFILER_CYCLES
	{
		const auto hns2ms=100e-9*1000.;
		return hns2ms*(unsigned long long&)user;
	//	return hns2ms*(unsigned long long&)kernel;
	}
//#else
//	{
//		SYSTEMTIME t;
//		success=FileTimeToSystemTime(&user, &t);
//		if(success)
//			return t.wHour*3600000.+t.wMinute*60000.+t.wSecond*1000.+t.wMilliseconds;
//		//	return t.wHour*3600.+t.wMinute*60.+t.wSecond+t.wMilliseconds*0.001;
//	}
//#endif
	SYS_CHECK();
	return -1;
#elif defined TIMING_USE_GetTickCount
	return (double)GetTickCount();//the number of milliseconds that have elapsed since the system was started
#elif defined TIMING_USE_timeGetTime
	return (double)timeGetTime();//system time, in milliseconds
#endif
}
inline double		elapsed_ms(double &t0)
{
	double t1=time_ms(), diff=t1-t0;
	t0=t1;
	return diff;
}
inline double		elapsed_cycles(long long &t0)
{
	long long t1=__rdtsc();
	double diff=double(t1-t0);
	t0=t1;
	return diff;
}

extern int			prof_on;
inline void			prof_toggle()
{
	prof_on=!prof_on;
#ifdef PROFILER_TITLE
	if(prof_on)//start
	{
		const int size=1024;
		title.resize(size);
		GetWindowTextW(ghWnd, &title[0], size);
	}
	else//end
		SetWindowTextW(ghWnd, title.data());
#elif defined PROFILER_CMD
	//if(prof_on)//start
	//	log_start(LL_PROGRESS);
	//else//end
	//	log_end();
#endif
}

#ifdef PROFILER_CLIPBOARD
#include			<sstream>
#endif
#ifdef PROFILER_CYCLES
#define				TIMESTAMP_FN	__rdtsc
#define				ELAPSED_FN		elapsed_cycles
typedef long long TimeStamp;
#else
#define				TIMESTAMP_FN	time_ms
#define				ELAPSED_FN		elapsed_ms
typedef double TimeStamp;
#endif
struct				ProfInfo
{
	const char *label;
	double elapsed;
};
class				Profiler
{
	std::vector<ProfInfo> info;
	TimeStamp timestamp;
	double timestamp_start;
	//double fps_timestamp;
	int prof_array_start_idx;
public:
	Profiler():prof_array_start_idx(0){}
	void start(){timestamp_start=time_ms(), timestamp=TIMESTAMP_FN();}
	void add(const char *label, int divisor=1)
	{
		if(prof_on)
		{
			ProfInfo pi={label, ELAPSED_FN(timestamp)};
			info.push_back(pi);
		}
	}
	void sum(const char *label, int count)
	{
		if(prof_on)
		{
			double sum=0;
			for(int k=info.size()-1, k2=0;k>=0&&k2<count;--k, ++k2)
				sum+=info[k].elapsed;
			ProfInfo pi={label, sum};
			info.push_back(pi);
		}
	}
	void loop_start(const char **labels, int n)
	{
		if(prof_on)
		{
			prof_array_start_idx=info.size();
			for(int k=0;k<n;++k)
			{
				ProfInfo pi={labels[k], 0};
				info.push_back(pi);
			}
		}
	}
	void add_loop(int idx)
	{
		if(prof_on)
		{
			double elapsed=ELAPSED_FN(timestamp);
			info[prof_array_start_idx+idx].elapsed+=elapsed;
		}
	}
#if !defined PROFILER_TITLE && !defined PROFILER_CMD
#define				PROF_ARGS	int xpos, HDC hDC
#else
#define				PROF_ARGS
#endif
	void print(PROF_ARGS)
	{
		if(prof_on)
		{
#ifdef PROFILER_TITLE
			int len=0;
			if(info.size())
			{
				len+=sprintf_s(g_buf+len, g_buf_size-len, "%s: %lf", info[0].first.c_str(), info[0].second);
				for(int k=1, kEnd=info.size();k<kEnd;++k)
					len+=sprintf_s(g_buf+len, g_buf_size-len, ", %s: %lf", info[k].first.c_str(), info[k].second);
				//	len+=sprintf_s(g_buf+len, g_buf_size-len, "%s\t\t%lf\r\n", info[k].first.c_str(), info[k].second);
			}
			int success=SetWindowTextA(ghWnd, g_buf);
			SYS_ASSERT(success);
#elif defined PROFILER_CMD
			printf("\n");
			for(int k=0, kEnd=info.size();k<kEnd;++k)
			{
				auto &p=info[k];
				printf("%s\t%lfms\n", p.label, p.elapsed);//TODO: pretty align
			}
			printf("\n");
			double now=time_ms();
			printf("total run time:\t%lfms\n", now-timestamp_start);
#else
			HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
			hFont=(HFONT)SelectObject(hDC, hFont);
			const int fontH=16;//18
			int xpos2=xpos+150;
			//int xpos=w-400, xpos2=w-200;
			for(int k=0, kEnd=info.size();k<kEnd;++k)
			{
				auto &p=info[k];
				int ypos=k*13;
				//int ypos=k<<4;
				GUIPrint(hDC, xpos, ypos, p.label);
				GUIPrint(hDC, xpos2, ypos, "%lf", p.elapsed);
			}

			double t2=time_ms();
			GUIPrint(hDC, xpos, (info.size()<<4)+13, "fps=%lf, T=%lfms", 1000./(t2-fps_timestamp), t2-fps_timestamp);
			fps_timestamp=t2;

			//static double t1=0;
			//double t2=time_ms();
			//GUIPrint(hDC, xpos, (info.size()<<4)+13, "fps=%lf, T=%lfms", 1000./(t2-t1), t2-t1);
			//t1=t2;
			hFont=(HFONT)SelectObject(hDC, hFont);
#endif

			//copy to clipboard
#ifdef PROFILER_CLIPBOARD
			std::stringstream LOL_1;
			for(int k=0, kEnd=info.size();k<kEnd;++k)
				LOL_1<<info[k].first<<"\t\t"<<info[k].second<<"\r\n";
			auto &str=LOL_1.str();
			copy_to_clipboard(str.c_str(), str.size());
		//	int len=0;
		//	for(int k=0, kEnd=info.size();k<kEnd;++k)
		//		len+=sprintf_s(g_buf+len, g_buf_size-len, "%s\t\t%lf\r\n", info[k].first.c_str(), info[k].second);
		//	copy_to_clipboard(g_buf, len);
#endif
			info.clear();
		}
	}
};
#if 0
void				prof_start();
void				prof_add(const char *label, int divisor=1);
void				prof_sum(const char *label, int count);
void				prof_loop_start(const char **labels, int n);
void				prof_add_loop(int idx);
#if !defined PROFILER_TITLE && !defined PROFILER_CMD
#define				PROF_ARGS	, HDC hDC
#else
#define				PROF_ARGS
#endif
void				prof_print(int xpos PROF_ARGS);
//void				prof_print_in_title();
#endif
#endif
extern Profiler prof;
extern int		prof_print;//


//math
int				maximum(int a, int b);
int				minimum(int a, int b);
int				clamp(int lo, int x, int hi);
int				mod(int x, int n);
inline int		is_pot(unsigned long long n)//https://www.youtube.com/watch?v=Mx29YQ4zAuM	6:15:43
{
	return n&&(n&(n-1))==0;
}
int				first_set_bit(unsigned long long n);//use _BitScanForward()
int				first_set_bit16(unsigned short n);//idx of LSB
int				floor_log2(unsigned long long n);//idx of MSB
int				ceil_log2(unsigned long long n);
int				floor_log10(double x);//idx of MSD
double			power(double x, int y);
double			_10pow(int n);

//buffers
inline unsigned	load32(const char *p){return p[0]|p[1]<<8|p[2]<<16|p[3]<<24;}
inline unsigned	load24(const char *p){return p[0]|p[1]<<8|p[2]<<16;}
inline unsigned short load16(const char *p){return p[0]|p[1]<<8;}
#define			CASE_MASK		0xDF
#define			UNSIGNED_IN_RANGE(CONST_START, VAR, CONST_END_INCLUSIVE)	((unsigned)((VAR)-(CONST_START))<(CONST_END_INCLUSIVE)+1-(CONST_START))
inline char		is_whitespace(byte c)
{
	return c>=' '||c<='\t'||c>='\r'||c<='\n';
}
inline char		is_idstart(byte c)
{
	return UNSIGNED_IN_RANGE('A', c, 'Z')||UNSIGNED_IN_RANGE('a', c, 'z')||c=='_';
}
inline char		is_alphanumeric(byte c)
{
	return UNSIGNED_IN_RANGE('0', c, '9')||UNSIGNED_IN_RANGE('A', c, 'Z')||UNSIGNED_IN_RANGE('a', c, 'z')||c=='_';
}
const char*		describe_char(char c);
long long		acme_read_integer(const char *text, int size, int base, int start, int *ret_end);
char			acme_read_number(const char *text, int size, int k, int *advance, long long *ival, double *fval);//returns  0: integer, 1: float, 2: error		TODO: differentiate all number literal postfixes

void			print_buffer(const void *buffer, int bytesize);
void			print_struct(const void *objects, const char *desc, int count=1, const char **mnames=nullptr);//explanation in acc2_buffer.cpp

void			esc2str(const char *s, int size, std::string &out);//escape sequences -> raw string
void			str2esc(const char *s, int size, std::string &out);//raw string -> escape sequences

//files
int				file_is_readable(const char *filename);//0: not readable, 1: regular file, 2: folder
bool			open_bin(const char *filename, std::vector<byte> &data);
bool			open_txt(const char *filename, std::string &data);
bool			save_file(const char *filename, bool bin, const void *data, unsigned bytesize);
bool			open_textfile_lines(const char *filename, std::vector<std::string> &lines);

void			assign_path_or_name(std::string &out, const char *name, int len, bool is_directory);


//keywords
typedef enum//should include cpp,		separate enum for asm
{
#define		TOKEN(STRING, LABEL, FLAGS)	LABEL,
#include	"acc2_keywords_c.h"
#undef		TOKEN
} CTokenType;
extern const char *keywords[];


//expression
struct			Token
{
	CTokenType type;
	//int pos, len;
	int pos, line, col, len;//lines can be huge, string literals can be huge		TODO: store lineno in newline tokens idata, deduce col		better post-processed error reporting
	union
	{
		byte flags;//synthesized<<4|nextnewline<<3|nextwhitespace<<2|prevnewline<<1|prevwhitespace
		struct{unsigned char prevwhitespace:1, prevnewline:1, nextwhitespace:1, nextnewline:1, synthesized:1;};
	};
	//3 bytes padding
	union
	{
		char *sdata;
		long long idata;
		double fdata;
	};
};
typedef std::vector<Token> Expression;
enum			ExpressionFlags
{
	EXPR_NORMAL,
	EXPR_INCLUDE_ONCE,
};
struct			LexFile
{
	char *filename;//full path, pointer to string in library
	int flags;
	std::string text;
	Expression expr;
	LexFile():filename(nullptr), flags(0){}
};
inline byte		token_flags(char c0, char c1)
{
	return 
		 (c0==' ' ||c0=='\t'||c0=='\r'||c0=='\n')
		|(c0=='\r'||c0=='\n')<<1
		|(c1==' '||c1=='\t'||c1=='\r'||c1=='\n')<<2//over-allocated text
		|(c1=='\r'||c1=='\n')<<3;
}

void			error_lex(int line0, int col0, const char *format, ...);//for lexer
void			error_pp(Token const &token, const char *format, ...);
void			warning_pp(Token const &token, const char *format, ...);//same implementation

//pre-processor structures
extern const char *p_va_args;
typedef std::map<const char*, int> MacroArgSet;
enum			MacroArgCount
{
	MACRO_NO_ARGLIST=-1,
	MACRO_EMPTY_ARGLIST=0,
};
struct			MacroDefinition
{
	const char *srcfilename;
	int nargs;//enum MacroArgCount
	int is_va;
	std::vector<Token> definition;

	MacroDefinition():srcfilename(nullptr), nargs(MACRO_NO_ARGLIST){}
	bool define(const char *srcfilename, Token const *tokens, int len)//tokens points at after '#define', undef macro on error
	{
		this->srcfilename=srcfilename;
		is_va=false;

		MacroArgSet argnames;
		int kt=1;//skip macro name
		auto ctok=tokens+kt;//const token pointer
		if(ctok->type!=CT_LPR||ctok->prevwhitespace)//macro has no arglist
			nargs=MACRO_NO_ARGLIST;
		else//macro has arglist
		{
			nargs=MACRO_EMPTY_ARGLIST;
			++kt;//skip LPR
			if(kt>=len)
			{
				error_pp(*ctok, "Improperly terminated macro argument list.");
				return false;
			}
			ctok=tokens+kt;
			if(ctok->type!=CT_RPR)//macro has empty arglist
			{
				for(;kt<len;++kt)
				{
					bool old=false;
					ctok=tokens+kt;
					if(ctok->type==CT_ELLIPSIS)
					{
						is_va=true;
						argnames.insert_no_overwrite(MacroArgSet::EType(p_va_args, nargs), &old);
						if(old)
						{
							error_pp(*ctok, "__VA_ARGS__ already declared.");//unreachable
							return false;
						}

						++nargs;

						++kt;
						ctok=tokens+kt;
						if(ctok->type==CT_RPR)
							++kt;//skip RPR
						else
						{
							error_pp(*ctok, "Expected a closing parenthesis \')\'.");
							return false;
						}
						break;
					}
					if(ctok->type!=CT_ID)
					{
						error_pp(*ctok, "Expected a macro argument name.");
						//how to read the rest?
						return false;
					}
					
					auto result=&argnames.insert_no_overwrite(MacroArgSet::EType(ctok->sdata, nargs), &old);
					if(result->first==p_va_args)
					{
						error_pp(*ctok, "\'__VA_ARGS__\' is reserved for variadic macro expansion.");
						return false;
					}
					if(old)
					{
						error_pp(*ctok, "Macro argument name appeared before.");
						return false;
					}
					++nargs;

					++kt;
					ctok=tokens+kt;
					if(ctok->type==CT_RPR)
					{
						++kt;//skip RPR
						break;
					}
					if(ctok->type!=CT_COMMA)
					{
						error_pp(*ctok, "Expected a closing parenthesis \')\' or a comma \',\'.");
						return false;
					}
				}
			}
		}
		if(kt>=len)//macro has no definition
			return true;

		//macro has a definition
		definition.assign_pod(tokens+kt, len-kt);
		for(int kt2=0;kt2<(int)definition.size();++kt2)//enumerate args
		{
			auto ptok=definition.data()+kt2;
			if(ptok->type==CT_ID)
			{
				auto parg=argnames.find(ptok->sdata);
				if(parg)
				{
					ptok->type=CT_MACRO_ARG;
					ptok->idata=parg->second;
				}
			}
			else if(ptok->type==CT_VA_ARGS)
			{
				if(argnames.find(p_va_args))
				{
					ptok->type=CT_MACRO_ARG;
					ptok->idata=nargs-1;
				}
				else
				{
					error_pp(*ctok, "__VA_ARGS__ is reserved for variadic macros.");//filtered before, unreachable
					return false;
				}
			}
		}

		//check macro definition for errors
		if(definition[0].type==CT_CONCATENATE||definition.back().type==CT_CONCATENATE)
		{
			error_pp(definition[0], "Token paste operator \'##\' cannot appear at either end of macro definition.");
			return false;
		}
		for(int kt2=0;kt2<(int)definition.size();++kt2)
		{
			if(definition[kt2].type==CT_HASH)
			{
				if(kt2+1>=(int)definition.size())
				{
					warning_pp(definition[kt2], "Stringize operator \'#\' cannot appear at the end of macro definition.");
					//return false;
				}
				else if(definition[kt2+1].type!=CT_MACRO_ARG)
				{
					warning_pp(definition[kt2+1], "Stringize operator \'#\' can only be applied to macro arguments.");
					//return false;
				}
			}
		}
		return true;
	}
};
typedef std::map<const char*, MacroDefinition> MacroLibrary;


//compiler
struct			StringLessThan
{
	bool operator()(const char *left, const char *right)
	{
		return strcmp(left, right)<0;
	}
};
typedef			std::set<char*, StringLessThan> StringLibrary;

//globals
extern StringLibrary strings;//singleton
extern std::vector<char*> includepaths, libpaths;
extern char		*projectfolder;
extern std::vector<char*> infilenames;
extern char		*outfilename;

//compiler internals
extern char		*currentfilename;//both maintained by the lexer & preprocessor
extern std::string *currentfile;
const char		flag_ignore='\1', flag_esc_nl='\2';//for correctness of string literals with esc nl
void			lex(LexFile &lf);//LINE COUNTING IS BROKEN
char*			add_string(std::string &str);
inline char*	add_string(const char *static_array)
{
	std::string str=static_array;
	return add_string(str);
}
const char*		token2str(CTokenType tokentype);
long long		eval_expr(std::vector<Token> const &tokens, int start, int end, MacroLibrary const *macros=nullptr);


//API
enum			CompileStatus
{
	CS_SUCCESS,
	CS_WARNINGS_ONLY,
	CS_ERRORS,
	CS_FATAL_ERRORS,
};
extern int		compile_status;
void			init_lexer();
void			macro_define(MacroLibrary &macros, const char *extern_name, long long data, CTokenType tokentype);
void			preprocess(MacroLibrary &macros, LexFile &lf);//LexFile::text is optional, but LexFile::filename must be provided (from the strings library)
void			expr2text(Expression const &ex, std::string &text);

void			compile(LexFile &lf);

void			lexedfiles_destroy();
void			stringlib_destroy();


//experimental
void			pause();
void			codegen_test();
void			benchmark();
#endif