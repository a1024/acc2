#include		"acc2.h"
#include		"include/stdio.h"
#include		"include/conio.h"
#include		"include/sys/stat.h"
#include		"include/algorithm"
#include		"include/time.h"

int				compile_status=CS_SUCCESS;

Profiler		prof;
int				prof_on=true;
int				prof_print=true;//

const char		*keywords[]=//order corresponds to enum CTokenType
{
#define			TOKEN(STRING, LABEL, FLAGS)	STRING,
#include		"acc2_keywords_c.h"
#undef			TOKEN
};


StringLibrary	strings;
std::vector<char*> includepaths, libpaths;
char			*projectfolder=nullptr;
std::vector<char*> infilenames;
char			*outfilename=nullptr;
struct			SimpleMacro
{
	int valid;
	char *name;
	long long value;
	SimpleMacro(std::string const &definition)
	{
		int size=definition.size();
		int start=0;
		for(;start<size&&is_whitespace(definition[start]);++start);
		if(start>=size||!is_idstart(definition[start]))
		{
			memset(this, 0, sizeof(*this));
			return;
		}
		int end=start+1;
		for(;end<size&&is_alphanumeric(definition[end]);++end);//TODO: parse simple pre-defined macro definition
		valid=1;
		name=add_string(std::string(definition.data()+start, end-start));
		value=1;
	}
	//bool add(MacroLibrary &macros, std::string const &definition)
	//{
	//	int size=definition.size();
	//	int start=0;
	//	for(;start<size&&is_whitespace(definition[start]);++start);
	//	if(start>=size||!is_idstart(definition[start]))
	//		return false;
	//	int end=start+1;
	//	for(;end<size&&is_alphanumeric(definition[end]);++end);//TODO: parse simple pre-defined macro definition
	//	macro_define_int(macros, add_string(std::string(definition.data()+start, end-start)), 1);
	//	return true;
	//}
};
std::vector<SimpleMacro> predefinedmacros;

typedef enum
{
#define			CMDARG(STRING, LABEL)	LABEL
#include		"acc2_cmdargs.h"
#undef			CMDARG
} CmdArgType;
const char		*cmdargs[]=
{
#define			CMDARG(STRING, LABEL)	STRING
#include		"acc2_cmdargs.h"
#undef			CMDARG
};
int				showhelp=0, action=CA_NONE;
void			proc_pause()
{
	_getch();
}
void			print_summary()
{
	printf("\nSUMMARY:\n");
	if(includepaths.size())
	{
		printf("Include path(s):\n");
		for(int k=0;k<(int)includepaths.size();++k)
			printf("\t%s\n", includepaths[k]);
	}
	else
		printf("No include paths\n");

	if(libpaths.size())
	{
		printf("Library path(s):\n");
		for(int k=0;k<(int)libpaths.size();++k)
			printf("\t%s\n", libpaths[k]);
	}
	else
		printf("No library paths\n");

	if(projectfolder)
		printf("Project folder: %s\n", projectfolder);
	else
		printf("Project folder: <unspecified>\n");

	if(infilenames.size())
	{
		printf("Input file(s):\n");
		for(int k=0;k<(int)infilenames.size();++k)
			printf("\t%s\n", infilenames[k]);
	}
	else
		printf("Input files: <unspecified>\n");

	if(outfilename)
		printf("Output filename: %s\n", outfilename);
	else
		printf("Output filename: <unspecified>\n");
	printf("\n");

}
void			print_usage()
{
	printf(
		"Usage:\n"
		"\n"
		"--help               Print usage.\n"
		"\n"
		"--see     <file>     Specify a file with commandline arguments\n"
		"                     (one command per line).\n"
		"--macros             Specify pre-defined macros for all sources.\n"
		"--include <folders>  Add include paths.\n"
		"--lib     <folders>  Add library paths.\n"
		"--project <folder>   (Optional) Specify the folder where input files are\n"
		"                     located. If not specified, the current working directory\n"
		"                     is used.\n"
		"--in      <files>    Add input files (paths relative to the folder specified\n"
		"                     in --project command).\n"
		"--prof               Use performance profiler.\n"
		"\n"
		"Choose at most one of the following commands:\n"
		"--out     <file>     (Optional) Specify the executable name.\n"
		"--preprocess         The specified sources are to be just preprocessed.\n"
		"--compile            The specified sources are to be just compiled (assembly\n"
		"                     output).\n"
		"--assemble           An object file is generated for each specified input file.\n"
		"--link               The input files are object files to be linked.\n"
		"\n"
		);
}
void			parse_args(std::vector<std::string> const &args)
{
	//if(!skip)
	//	printf("reading %d args from start\n", args.size());

	CmdArgType currentarg=CA_NONE;
	std::string str;
	const char *pointer=nullptr;
	for(int k=0;k<(int)args.size();++k)
	{
		auto &arg=args[k];
		printf("[%d] %s\n", k, arg);//
		if(arg.size()>2&&arg[0]=='-'&&arg[1]=='-')
		{
			switch(arg[2])
			{
			case 'a':
				if(!strcmp(arg.data()+3, cmdargs[CA_ASSEMBLE]+3))
				{
					if(action)
					{
						printf("Invalid command args: Command %s is incompatible with %s.\n", cmdargs[action], arg);
						break;
					}
					action=CA_ASSEMBLE;
					continue;
				}
				break;
			case 'c':
				if(!strcmp(arg.data()+3, cmdargs[CA_COMPILE]+3))
				{
					if(action)
					{
						printf("Invalid command args: Command %s is incompatible with %s.\n", cmdargs[action], arg);
						break;
					}
					action=CA_COMPILE;
					continue;
				}
				break;
			case 'h':
					 if(!strcmp(arg.data()+3, cmdargs[CA_HELP		]+3)){showhelp=1;continue;}
				break;
			case 'i':
					 if(!strcmp(arg.data()+3, cmdargs[CA_INCLUDE	]+3)){currentarg=CA_INCLUDE;continue;}
				else if(!strcmp(arg.data()+3, cmdargs[CA_INPUT		]+3)){currentarg=CA_INPUT;continue;}
				break;
			case 'l':
					 if(!strcmp(arg.data()+3, cmdargs[CA_LIB		]+3)){currentarg=CA_LIB;continue;}
				else if(!strcmp(arg.data()+3, cmdargs[CA_LINK		]+3))
				{
					if(action)
					{
						printf("Invalid command args: Command %s is incompatible with %s.\n", cmdargs[action], arg);
						break;
					}
					action=CA_LINK;
					continue;
				}
				break;
			case 'm':
					 if(!strcmp(arg.data()+3, cmdargs[CA_MACROS		]+3)){currentarg=CA_MACROS;continue;}
				break;
			case 'o':
					 if(!strcmp(arg.data()+3, cmdargs[CA_OUTPUT		]+3)){currentarg=CA_OUTPUT, action=CA_OUTPUT;continue;}
				break;
			case 'p':
				if(!strcmp(arg.data()+3, cmdargs[CA_PREPROCESS]+3))
				{
					if(action)
					{
						printf("Invalid command args: Command %s is incompatible with %s.\n", cmdargs[action], arg);
						break;
					}
					action=CA_PREPROCESS;
					continue;
				}
				else if(!strcmp(arg.data()+3, cmdargs[CA_PROFILER	]+3))
				{
					prof_print=true;
					//if(!prof_on)
					//{
					//	prof.start();
					//	prof_on=true;
					//}
					continue;
				}
				else if(!strcmp(arg.data()+3, cmdargs[CA_PROJECT	]+3)){currentarg=CA_PROJECT;continue;}
				break;
			case 's':
					 if(!strcmp(arg.data()+3, cmdargs[CA_SEE		]+3)){currentarg=CA_SEE;continue;}
				else if(!strcmp(arg.data()+3, cmdargs[CA_INPUT		]+3)){currentarg=CA_INPUT;continue;}
				break;
			}
		}
		else//not a command
		{
			switch(currentarg)
			{
			case CA_SEE:
				{
					std::vector<std::string> lines;
					if(!open_textfile_lines(arg.c_str(), lines))
					{
						printf("Couldn't open command args file \"%s\"\n", arg.c_str());
						break;
					}
					printf("command file: %d args\n", lines.size());//
					parse_args(lines);
				}
				break;
			case CA_MACROS:
				predefinedmacros.push_back(SimpleMacro(arg));
				break;
			case CA_NONE:
			case CA_INPUT:
				//infilenames.push_back(std::string());
				assign_path_or_name(str, arg.c_str(), arg.size(), 0);
				infilenames.push_back(add_string(str));
				break;
			case CA_INCLUDE:
				//includepaths.push_back(std::string());
				assign_path_or_name(str, arg.c_str(), arg.size(), 1);
				includepaths.push_back(add_string(str));
				break;
			case CA_LIB:
				//libpaths.push_back(std::string());
				assign_path_or_name(str, arg.c_str(), arg.size(), 1);
				libpaths.push_back(add_string(str));
				break;
			case CA_PROJECT:
				if(projectfolder)
					printf("Invalid command args: Multiple project folders provided. Expected one project folder.\n\t%s\n\t%s\n", projectfolder, arg.c_str());
				else
				{
					assign_path_or_name(str, arg.c_str(), arg.size(), 1);
					projectfolder=add_string(str);
				}
				break;
			case CA_OUTPUT:
				if(outfilename)
					printf("Invalid command args: Multiple output filenames provided. Expected one output filename.\n\t%s\n\t%s\n", outfilename, arg.c_str());
				else
				{
					assign_path_or_name(str, arg.c_str(), arg.size(), 0);
					outfilename=add_string(str);
				}
				break;
			default:
				break;
			}
		}
	}//end for
}

//extern "C" void assembly_test();
//static int	current_depth=0;
//void			depth_rec()
//{
//	++current_depth;
//	printf("current depth = %d\r", current_depth);
//	depth_rec();
//}

int				main(int argc, const char **argv)
{
	//depth_rec();//will crash, eventually

	prof.start();

	//const int LOL_1=1?5:1 ? 0?1:0 : 1?3:1;
	//codegen_test();
	//__debugbreak();
	//assembly_test();//

	printf("ACC2 built on %s, %s\n\n", __DATE__, __TIME__);//started on 2021-07-09
	if(argc==1)
	{
#if defined _MSC_VER || defined __ACC2__
		print_usage();//
		proc_pause();
#endif
		return 0;
	}
	
	printf("initializing...\n");
	init_lexer();
	prof.add("init lexer");

	printf("parsing args...\n");
	std::vector<std::string> args;
	for(int k=1;k<argc;++k)
		args.push_back(argv[k]);
	prof.add("args2vec");
	parse_args(args);
	prof.add("parse args");

	print_summary();//
	if(!infilenames.size())
	{
		printf("No input files specified\n");
		_getch();
		return 0;
	}

	time_t t=time(nullptr);//get date & time 
	struct tm tm={};
	localtime_s(&tm, &t);

	int nsources=infilenames.size();
	for(int ks=0;ks<nsources;++ks)//source loop
	{
		LexFile lf;
		printf("\n%d/%d: \"%s\"...\n", ks+1, nsources, infilenames[ks]);
		if(projectfolder)
		{
			std::string filename=projectfolder;
			filename+=infilenames[ks];
			lf.filename=add_string(filename);
		}
		else
			lf.filename=infilenames[ks];
		//printf("%s\n", lf.filename);//

		MacroLibrary macros;
		macro_define(macros, "__ACC2__", 1, CT_VAL_INTEGER);//pre-defined macros
		macro_define(macros, "__cplusplus", 199711, CT_VAL_INTEGER);
		
		const char *weekdays[]={"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
		sprintf_s(g_buf, g_buf_size, "%d-%02d-%02d%s", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, weekdays[tm.tm_wday]);//TODO: standard format
		macro_define(macros, "__DATE__", (long long)g_buf, CT_VAL_STRING_LITERAL);
		sprintf_s(g_buf, g_buf_size, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
		macro_define(macros, "__TIME__", (long long)g_buf, CT_VAL_STRING_LITERAL);

		int nmacros=predefinedmacros.size();
		for(int km=0;km<nmacros;++km)
			macro_define(macros, predefinedmacros[km].name, predefinedmacros[km].value, CT_VAL_INTEGER);
		prof.add("preparations");

		preprocess(macros, lf);

		compile(lf);//

#if 0
		switch(action)
		{
		case CA_PREPROCESS:	//c/c++ -> preprocessed c/c++
		case CA_COMPILE:	//c/c++ -> asm
		case CA_ASSEMBLE:	//c/c++/asm -> object file(s)
		case CA_LINK:		//object file(s) -> executable
		case CA_OUTPUT:		//c/c++/asm -> executable
			{
				std::string str;
				expr2text(lf.expr, str);
				prof.add("tokens -> text");

				time_t t=time(nullptr);
				struct tm tm={};
				localtime_s(&tm, &t);
				sprintf_s(g_buf, g_buf_size, "out_%d.%s", ks, "cpp");
				//sprintf_s(g_buf, g_buf_size, "%d%02d%02d_%02d%02d%02d.%s", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, "cpp");
				std::string filename=projectfolder;
				filename+=g_buf;
				
				save_file(filename.c_str(), false, str.data(), str.size());
				prof.add("save");
			}
			break;
		}
#endif
	}//end source loop
	lexedfiles_destroy();
	stringlib_destroy();
	prof.add("free strings");

#if 0
	switch(compile_status)
	{
	case CS_SUCCESS:		printf("Build successful.\n");				break;
	case CS_WARNINGS_ONLY:	printf("Build successful with warnings.\n");break;
	case CS_ERRORS:			printf("Build failed.\n");					break;
	case CS_FATAL_ERRORS:	printf("Internal error occurred.\n");		break;
	}
#endif

	if(prof_print)
		prof.print();
	_getch();
	return 0;
}