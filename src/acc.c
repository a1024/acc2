#include	"acc.h"
#include	<stdio.h>
#include	<stdlib.h>
#ifdef _MSC_VER
#include	<Windows.h>
#endif

const char *std_includes[]=//hardcoded: a temporary measure		//folders end with slash
{
	"D:/Programs/msys2/mingw64/include/",
//	"C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/include/",
//	"C:/Program Files (x86)/Windows Kits/8.1/Include/um/",
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
void			set_console_buffer_size(short w, short h)
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
#define			set_console_buffer_size(...)
#endif

static void	free_incpath(void *data)
{
	ArrayHandle *str=(ArrayHandle*)data;
	array_free(str);
}
void		pause()
{
	int k;

	printf("Enter 0 to continue: ");
	scanf("%d", &k);
}
int			main(int argc, char **argv)
{
	set_console_buffer_size(120, 9001);
	if(argc<2)
	{
		printf(
			"Usage: acc sources -o output\n"
			"Build %s %s\n\n"
		, __DATE__, __TIME__);
		return 1;
	}
#if 1
	printf("pwd:\n");
#ifdef _MSC_VER
	system("cd");
#else
	system("pwd");
#endif
	printf("argv[0]:\n%s\n\n", argv[0]);
#endif


	//initialize includepaths
	const int nStdIncludes=SIZEOF(std_includes);
	ARRAY_ALLOC(ArrayHandle, includepaths, 0, nStdIncludes, 0, free_incpath);
	for(int k=0;k<nStdIncludes;++k)
	{
		ArrayHandle *includepath=(ArrayHandle*)array_at(&includepaths, k);
		STR_COPY(*includepath, std_includes[k], strlen(std_includes[k]));
	}


	//initialize pre-defined macros (predefs)
	init_dateNtime();
	macros_init(&macros, predefs, SIZEOF(predefs));

	
	printf("Preprocessing %s\n", argv[1]);
	ArrayHandle tokens=preprocess(argv[1], &macros, includepaths, &lexlib);
	if(!tokens)
	{
		printf("Preprocess failed\n");
		return 1;
	}
	printf("Preprocess result: %lld tokens\n", (long long)tokens->count);//%zd doesn't work on MSVC

	printf("\ntokens2text:\n");
	ArrayHandle text=0;
	tokens2text(tokens, &text);
	printf("Save preprocessor output? [Y/N] ");
	char c=0;
	scanf("%c", &c);
	if((c&0xDF)=='Y')
	{
		const char ext[]=".c";
		ArrayHandle filename;
		STR_COPY(filename, currenttimestamp, strlen(currenttimestamp));
		STR_APPEND(filename, ext, sizeof(ext)-1, 1);
		int success=save_text((char*)filename->data, (char*)text->data, text->count);
		array_free(&filename);
		if(success)
			printf("Saved\n");
		else
			printf("Failed to save\n");
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
