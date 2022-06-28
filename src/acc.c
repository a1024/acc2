#include	"acc.h"
#include	<stdio.h>
#include	<stdlib.h>

#ifdef _MSC_VER
	#define	PAUSE
#endif

Map			macros={0}, lexlib={0};
ArrayHandle includepaths=0;
void		pause()
{
	int k;

	printf("Enter 0 to continue: ");
	scanf("%d", &k);
}
int			main(int argc, char **argv)
{
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
	printf("argv[0]:\n%s\n", argv[0]);
#endif

	ARRAY_ALLOC(char*, includepaths, 0, 0);
	
	printf("Preprocessing %s\n", argv[1]);
	ArrayHandle tokens=preprocess(argv[1], &macros, includepaths, &lexlib);
	if(!tokens)
	{
		printf("Preprocess failed\n");
		return 1;
	}
	printf("Preprocess result: %d tokens\n", (long long)tokens->count);//%zd doesn't work on MSVC

	printf("\ntokens2text:\n");
	ArrayHandle text=0;
	tokens2text(tokens, &text);
	printf("%s\n", (char*)text->data);

	printf("Cleanup...\n");
	array_free(&text, 0);
	free(tokens);

	acc_cleanup(&lexlib, &strlib);//only before exit

	printf("Quitting...\n");
#ifdef PAUSE
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
