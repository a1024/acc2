#include"acc2.h"
static const char file[]=__FILE__;

typedef struct SymbolStruct
{
	char *name;
	ASTNode *definition;
} Symbol;
static CmpRes symbol_cmp(const void *key, const void *candidate)
{
	const char *p1, *p2;

	p1=(const char*)key;
	p2=(const char*)candidate;
	return (p1>p2)-(p1<2);
}
typedef struct ContextStruct
{
	ArrayHandle symboltables;
	DList code;
} Context;
static void free_map(void *p)
{
	Map *map=(Map*)p;
	MAP_CLEAR(map);
}
static void translate_rec(Context *ctx, ASTNode *code)
{
	switch(code->type)
	{
	case NT_PROGRAM:
		return;
	case NT_TYPE:
		{
			Map *symboltable=(Map*)array_at(ctx->symboltables, ctx->symboltables->count-1);
			for(int k=0;k<code->nch;++k)
			{
				ASTNode *n=code->ch[k];
				if(n->type!=NT_TOKEN)
				{
					COMPE(&n->t, "Expected an identifier");
					continue;
				}
				int found=0;
				RBNodeHandle *hNode=map_insert(symboltable, &n->t.val_str, &found);
				if(found)
				{
					COMPE(&n->t, "\'%s\' redefinition", n->t.val_str);
					continue;
				}
				Symbol *symbol=(Symbol*)hNode[0]->data;
				symbol->name=n->t.val_str;
				symbol->definition=n;
				if(n->nch)
				{
					int kc=0;
					ASTNode *n2=n->ch[kc];
					if(n2->type==NT_FUNCHEADER)
					{
						//read funcheader
						if(kc+1<n->nch)
						{
							++kc;
							n2=n->ch[kc];
						}
					}
					if(n2->type==NT_FUNCBODY)
					{
						Map *st2=(Map*)ARRAY_APPEND(ctx->symboltables, 0, 1, 1, 0);
						MAP_INIT(st2, Symbol, symbol_cmp, 0);
						
						for(int k=0;k<n2->nch;++k)//parse funcbody
							translate_rec(ctx, n2->ch[k]);

						array_erase(&ctx->symboltables, ctx->symboltables->count-1, 1);
					}
					else//initialization
					{
						translate_rec(ctx, n2);
						Instruction const *instr=(Instruction*)dlist_back(&ctx->code);
						//assign the result to where?
					}
				}
			}
		}
		return;
	}
}
ArrayHandle translate(ASTNode *root)
{
	Context ctx={0};
	dlist_init(&ctx.code, sizeof(Instruction), 16, 0);
	ARRAY_ALLOC(Map, ctx.symboltables, 0, 1, 0, free_map);
	MAP_INIT((Map*)ARRAY_LAST(ctx.symboltables), Symbol, symbol_cmp, 0);

	for(int k=0;k<root->nch;++k)
		translate_rec(&ctx, root->ch[k]);

	array_free(&ctx.symboltables);
	ArrayHandle ret=0;
	dlist_appendtoarray(&ctx.code, &ret);
	dlist_clear(&ctx.code);
	return ret;
}