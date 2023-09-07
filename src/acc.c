#include	"acc.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#ifdef _MSC_VER
#include	<Windows.h>
#endif
static const char file[]=__FILE__;

const char	*cmdargs[]=
{
	"hhelp",
	"ooutput",
};

const char *std_includes[]=//hardcoded: a temporary measure		//folders end with slash
{
#ifdef __linux__
//C preprocessor searchpaths: `gcc -print-prog-name=cpp` -v
//C++ preprocessor searchpaths: `gcc -print-prog-name=cc1plus` -v
#ifdef __cplusplus
	"/usr/include/c++/10/"
	"/usr/include/x86_64-linux-gnu/c++/10"
	"/usr/include/c++/10/backward"
#endif
	"/usr/lib/gcc/x86_64-linux-gnu/10/include"
	"/usr/local/include/"
#ifdef __cplusplus
	"/usr/include/x86_64-linux-gnu/"
#endif
	"/usr/include/"//stdio.h, ...

#else//Windows searchpaths

//	"D:/Programs/msys2/mingw64/include/",
//	"D:/Programs/msys2/mingw64/lib/gcc/x86_64-w64-mingw32/11.3.0/include/",

	"C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/include/",
	"C:/Program Files (x86)/Windows Kits/8.1/Include/um/",

#endif
};
PreDef		predefs[]=
{
	{"__STDC__", T_VAL_I32, (const char*)1},

	{"__ACC__", T_VAL_I32, (const char*)1},

//	{"__GNUC__", T_VAL_I32, (const char*)11},//__builtin_...

	{"_MSC_VER", T_VAL_I32, (const char*)1800},
	{"_M_AMD64", T_VAL_I32, (const char*)100},//x86-64		TODO: target option
//	{"_M_IX86", T_VAL_I32, (const char*)600},//x86

	{"_WIN32", T_VAL_I32, (const char*)1},//TODO: make _WIN32 optional
};

Map			macros={0}, lexlib={0};
ArrayHandle includepaths=0;//array of strings

#ifdef _MSC_VER
void		set_console_buffer_size(short w, short h)
{
	COORD coords={w, h};
	void *handle=GetStdHandle(STD_OUTPUT_HANDLE);
	if(handle==INVALID_HANDLE_VALUE)
	{
		printf("Failed to resize console buffer (GetStdHandle): %d\n\n", GetLastError());
		return;
	}
	int success=SetConsoleScreenBufferSize(handle, coords);
	if(!success)
		printf("Failed to resize console buffer (SetConsoleScreenBufferSize): %d\n\n", GetLastError());
}
#else
#define		set_console_buffer_size(...)
#endif

static void	free_str(void *data)
{
	ArrayHandle *str=(ArrayHandle*)data;
	array_free(str);
}

//map v2 (red-black tree) test
#if 0
static CmpRes	cmp_str(const void *key, const void *candidate)
{
	const char *L=*(const char**)key, *R=*(const char**)candidate;
	int result=strcmp(L, R);
	return (result>0)-(result<0);
}
static void		print_map(RBNodeHandle *node, int depth)
{
	char *str=*(char**)node[0]->data;
	printf("%4d %*s%c %s\n", depth, depth<<1, "", node[0]->is_red?'R':'B', str);
}
const char		*rb_test_data[]=
{
	"LOL_1",
	"LOL_2",
	"LOL_3",
	"LOL_4",
	"LOL_5",
	"LOL_6",
	"LOL_7",
	"LOL_8",
	"LOL_9",
	"LOL_10",
};
void			rb_test()
{
	Map m;
	RBNodeHandle *node;
	const char **data;
	int found;

	MAP_INIT(&m, char*, cmp_str, 0);

	printf("RB-Tree insert test:\n");
	found=0;
	for(int k=0;k<COUNTOF(rb_test_data);++k)//insert loop
	{
		node=map_insert(&m, rb_test_data+k, &found);
		ASSERT(node&&*node);
		data=(const char**)node[0]->data;
		*data=rb_test_data[k];
		if(found)
			printf("Found %s\n", rb_test_data[k]);

		printf("\n\nStep %d:\n", k);
		MAP_DEBUGPRINT(&m, print_map);
	}
	
	printf("RB-Tree erase test:\n");
	for(int k=0;k<COUNTOF(rb_test_data);++k)//erase loop
	{
		found=MAP_ERASE_DATA(&m, rb_test_data+k);
		if(found)
			printf("\nErased %s\n", rb_test_data[k]);
		else
			printf("\nFailed to erase %s\n", rb_test_data[k]);

		printf("\nStep %d:\n", k);
		MAP_DEBUGPRINT(&m, print_map);
	}
	
	printf("Cleanup...\n");
	MAP_CLEAR(&m);
	printf("Cleanup successful\n");

	pause();
	exit(0);
}
#endif


int			main(int argc, char **argv)
{
	int argidx, argchoice;
	ArrayHandle infilenames;
	const char *outfilename;

	//set_console_buffer_size(120, 9001);
	//rb_test();

#ifdef _DEBUG
	const char *_input="C:/Projects/acc/input.c";
	outfilename="C:/Projects/acc/out.c";

	ARRAY_ALLOC(ArrayHandle, infilenames, 0, 0, 1, free_str);
	ArrayHandle *_fn=(ArrayHandle*)ARRAY_APPEND(infilenames, 0, 1, 1, 0);
	STR_COPY(*_fn, _input, strlen(_input));
#else
	infilenames=0;
	outfilename=0;
	for(argidx=1;;++argidx)
	{
		argchoice=acme_getopt(argc, argv, &argidx, cmdargs, COUNTOF(cmdargs));
		switch(argchoice)
		{
		case OPT_ENDOFARGS:
			break;
		case OPT_INVALIDARG:
			printf(
				"Error: Unrecognized argument \'%s\'\n",
				argv[argidx]
			);
			continue;
		case OPT_NOMATCH:
			{
				if(!infilenames)
					ARRAY_ALLOC(ArrayHandle, infilenames, 0, 0, 1, 0);
				ArrayHandle *fn=(ArrayHandle*)ARRAY_APPEND(infilenames, argv+argidx, 1, 1, 0);
				STR_COPY(*fn, argv[argidx], strlen(argv[argidx]));
			}
			continue;
		case 0://help
			printf(
				"Usage: acc infilename(s) -o outfilename\n"
			);
			return 0;
		case 1://output
			++argidx;
			if(argidx>=argc)
			{
				printf("Error: expected output file name after \'-o\'.\n");
				return 1;
			}
			outfilename=argv[argidx];
			continue;
		}
		break;
	}
	if(!infilenames)
	{
		printf("Error: no input files.\n");
		return 1;
	}
	if(infilenames->count>1)//
	{
		printf("Error: ACC is still in development. Multiple files not supported yet.\n");
		return 1;
	}
#endif
#if 0
	if(argc!=2)
	{
		printf(
#if 0
			"Usage: acc sources -o output\n"
#else
			"Usage: acc sourcefile\n"
#endif
			"Build %s %s\n\n"
		, __DATE__, __TIME__);
		return 1;
	}
#endif
#if 0
	printf("Current directory:\n");
#ifdef _MSC_VER
	system("cd");
#else
	system("pwd");
#endif
	printf("argv[0]:\n%s\n\n", argv[0]);
#endif


	//initialize includepaths
	const int nStdIncludes=COUNTOF(std_includes);
	ARRAY_ALLOC(ArrayHandle, includepaths, 0, nStdIncludes, 0, free_str);
	for(int k=0;k<nStdIncludes;++k)
	{
		ArrayHandle *includepath=(ArrayHandle*)array_at(&includepaths, k);
		STR_COPY(*includepath, std_includes[k], strlen(std_includes[k]));
	}


	//initialize pre-defined macros (predefs)
	init_dateNtime();
	macros_init(&macros, predefs, COUNTOF(predefs));

	ArrayHandle *infilename=(ArrayHandle*)array_at(&infilenames, 0);
	printf("Preprocessing %s\n", (char*)infilename[0]->data);
	ArrayHandle tokens=preprocess((char*)infilename[0]->data, &macros, includepaths, &lexlib);
	if(!tokens)
	{
		printf("Preprocess failed\n");
		return 1;
	}
	printf("Preprocess result: %lld tokens\n", (long long)tokens->count);//%zd doesn't work on MSVC

#if 1
	int nlines_before=0, nlines_after=0;
	for(size_t k=0;k<tokens->count;++k)
	{
		Token *t=(Token*)array_at(&tokens, k);
		nlines_before+=t->nl_before;
		nlines_after+=t->nl_after;
	}
	printf("nlines_before: %d\n", nlines_before);
	printf("nlines_after:  %d\n", nlines_after);
#endif

	printf("\ntokens2text:\n");
	ArrayHandle text=0;
	tokens2text(tokens, &text);

	{
		const char ext[]=".c";
		ArrayHandle filename;
		if(outfilename)
			STR_COPY(filename, outfilename, strlen(outfilename));
		else
		{
			STR_COPY(filename, currenttimestamp, strlen(currenttimestamp));
			STR_APPEND(filename, ext, sizeof(ext)-1, 1);
		}
		printf("Save preprocessor output as \'%s\'? [Y/N] ", filename->data);
		char c=0;
		while(scanf(" %c", &c)<1);
		if((c&0xDF)=='Y')
		{
			int success=(int)save_file((char*)filename->data, (char*)text->data, text->count, 0);
			if(success)
				printf("Saved\n");
			else
				printf("Failed to save\n");
		}
		else
			printf("Save aborted\n");
		array_free(&filename);
	}
	//printf("%s\n", (char*)text->data);

	printf("Cleanup...\n");
	array_free(&text);
	free(tokens);

	acc_cleanup(&lexlib, &strlib);//only before exit

	printf("Quitting...\n");
#ifdef _MSC_VER
	pause();
#endif
	
	//string library test
#if 0
	strlib_insert("LOL_1", 0);
	strlib_insert("LOL_2", 0);
	strlib_insert("LOL_3", 0);
	strlib_insert("LOL_4", 0);
	strlib_insert("LOL_5", 0);
	strlib_insert("LOL_6", 0);
	strlib_insert("LOL_7", 0);
	strlib_insert("LOL_8", 0);

	printf("Original:\n");
	strlib_debugprint();

	map_rebalance(&strlib);

	printf("Rebalanced:\n");
	strlib_debugprint();
#endif

	return 0;
}
