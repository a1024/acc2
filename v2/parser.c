#include"acc2.h"
#include<stdio.h>
#include<stdlib.h>
static const char file[]=__FILE__;

//parser
ASTNode* node_alloc(ASTNodeType type)
{
	ASTNode *n=(ASTNode*)malloc(sizeof(ASTNode));
	if(!n)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	memset(n, 0, sizeof(ASTNode));
	n->type=type;
	return n;
}
void node_free(ASTNode **root)
{
	if(*root)
	{
		for(int k=0;k<root[0]->nch;++k)
			node_free(root[0]->ch+k);
		free(*root);
		*root=0;
	}
}
int node_append_ch(ASTNode **root, ASTNode *ch)//as an array
{
	int nch=root[0]->nch+1;
	void *p=realloc(*root, sizeof(ASTNode)+nch*sizeof(void*));
	if(!p)
	{
		LOG_ERROR("Allocation error");
		return 0;
	}
	*root=(ASTNode*)p;
	root[0]->ch[root[0]->nch]=ch;
	root[0]->nch=nch;
	return 1;
}
int node_attach_ch(ASTNode **root, ASTNode ***p_leaf, ASTNode *ch)//as a linked-list
{
	int ret=1;
	if(!*root)
	{
		*root=ch;
		*p_leaf=root;
	}
	else
	{
		ret=node_append_ch(*p_leaf, ch);
		*p_leaf=ch->ch;
	}
	return ret;
}
#define CHECKIDX(RET)\
	if(*idx>=count)\
	{\
		COMPE(tokens+*idx-1, "Unexpected EOF");\
		return RET;\
	}

//forward declarations
static ASTNode* parse_expr_commalist(Token *tokens, int *idx, int count);
static ASTNode* parse_declaration(Token *tokens, int *idx, int count, int global);
static ASTNode* parse_code(Token *tokens, int *idx, int count);


//the parser
static int is_type(KeywordType t)
{
	switch(t)
	{
	case KT_const:
	case KT_signed:
	case KT_unsigned:
	case KT_void:
	case KT_char:
	case KT_short:
	case KT_int:
	case KT_long:
	case KT_float:
	case KT_double:
		return 1;
	}
	return 0;
}
static ASTNode* parse_type(Token *tokens, int *idx, int count)
{
	Token *t;
	char invalid=0, is_const=0, is_unsigned=-1, nbytes=0, is_float=0, is_extern=0, is_static=0;
	t=tokens+*idx;
	for(;*idx<count&&t->type==TT_KEYWORD;++*idx, t=tokens+*idx)
	{
		switch(t->keyword_type)
		{
		case KT_extern:
			if(is_static)
			{
				invalid=1;
				COMPE(t, "Invalid storage type");
			}
			is_extern=1;
			break;
		case KT_static:
			if(is_extern)
			{
				invalid=1;
				COMPE(t, "Invalid storage type");
			}
			is_static=1;
			break;
		case KT_unsigned:
			if(is_float||is_unsigned!=-1)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			is_unsigned=1;
			break;
		case KT_signed:
			if(is_float||is_unsigned!=-1)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			is_unsigned=0;
			break;
		case KT_const:
			is_const|=1;
			break;
		case KT_void:
		case KT_char:
		case KT_int:
		case KT_float:
		case KT_double:
			if(nbytes)
			{
				invalid=1;
				COMPE(t, "Invalid type");
			}
			switch(t->keyword_type)
			{
			case KT_void:	nbytes=-1;break;
			case KT_char:	nbytes=1;break;
			case KT_int:	nbytes=4;break;
			case KT_float:	nbytes=4, is_float=1;break;
			case KT_double:	nbytes=8, is_float=1;break;
			}
			break;
		}
	}
	ASTNode *n=node_alloc(NT_TYPE);
	n->nbits=nbytes==-1?0:nbytes<<3;//-1 was void, so zero
	n->is_unsigned=is_unsigned==1;
	n->is_const=is_const;
	n->is_float=is_float;
	n->is_extern=is_extern;
	n->is_static=is_static;
	return n;
}
static ASTNode* parse_funcheader(Token *tokens, int *idx, int count)
{
	//funcheader := type (vardecl|funcdecl) [COMMA (vardecl|funcdecl) ]*

	//AST: funcheader -> type* -> ...

	Token *t;
	ASTNode *n_header=node_alloc(NT_FUNCHEADER);
	CHECKIDX(n_header)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR)//void
	{
		ASTNode *ch=node_alloc(NT_TYPE);//zero nbits means void
		node_append_ch(&n_header, ch);
		return n_header;
	}
	while(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
	{
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_ELLIPSIS)
		{
			ASTNode *n_name=node_alloc(NT_ARGDECL_ELLIPSIS);
			node_append_ch(&n_header, n_name);
			++*idx;

			CHECKIDX(n_header)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Ellipsis must terminate function header");
				return n_header;
			}
		}
		else
		{
			ASTNode *n_type=parse_type(tokens, idx, count);
			node_append_ch(&n_header, n_type);
			CHECKIDX(n_header)
			t=tokens+*idx;
			
			ASTNode *n_name=node_alloc(NT_ARGDECL);
			int indirection_count=0;
			while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
			{
				++indirection_count;
				++*idx;

				CHECKIDX(n_type)
				t=tokens+*idx;
			}
			n_name->indirection_count=indirection_count;
			if(t->type==TT_ID)
			{
				n_name->t=*t;
				++*idx;

				CHECKIDX(n_header)
				t=tokens+*idx;
			}
			if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//TODO funcpointer
			{
				//...
			}
			node_append_ch(n_header->ch+n_header->nch-1, n_name);
		}

		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA))
			break;
		++*idx;

		if(*idx>=count)
			break;
		t=tokens+*idx;
	}
	return n_header;
}

//precedence:
//	postfix		(highest prec)
//	prefix
//	multiplicative
//	additive
//	shift
//	inequality
//	equality
//	bitwise and
//	bitwise xor
//	bitwise or
//	logic and
//	logic or
//	ternary conditional
//	assignment
//	comma list	(lowest prec)
static int is_postfix_op(SymbolType t)
{
	switch(t)
	{
	case ST_INC://postfix increment
	case ST_DEC://postfix decrement
	case ST_LPR://function call
	case ST_LBRACKET://subscript
	case ST_PERIOD://struct member
	case ST_ARROW://pointer member
		return 1;
	}
	return 0;
}
static int is_prefix_op(Token *tokens, int idx, int count)
{
	Token *t;
	t=tokens+idx;
	if(t->type==TT_SYMBOL)
	{
		switch(t->symbol_type)
		{
		case ST_LPR://possible cast operator
			++idx;
			if(idx<count)
			{
				t=tokens+idx;
				if(t->type==TT_KEYWORD&&is_type(t->keyword_type))
					return 1;
			}
			break;
		case ST_INC://preinc
		case ST_DEC://predec
		case ST_PLUS://pos
		case ST_MINUS://neg
		case ST_EXCLAMATION://logic not
		case ST_TILDE://bitwise not
		case ST_ASTERISK://dereference
		case ST_AMPERSAND://address of
			return 1;
		}
	}
	return 0;
}
static ASTNode* parse_postfix(Token *tokens, int *idx, int count)//check if postfix before calling
{
	ASTNode *n_root=0;
	Token *t;
	t=tokens+*idx;
	switch(t->type)//handle main expression
	{
	case TT_SYMBOL:
		if(t->symbol_type==ST_LPR)
		{
			++*idx;

			CHECKIDX(0);
			n_root=parse_expr_commalist(tokens, idx, count);
			
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Expected \')\'");
				return n_root;
			}
			++*idx;
		}
		break;
	case TT_ID:
	case TT_STRING_LITERAL:
	case TT_CHAR_LITERAL:
	case TT_INT_LITERAL:
	case TT_FLOAT_LITERAL:
		n_root=node_alloc(NT_TOKEN);
		n_root->t=*t;
		++*idx;
		break;
	}
	if(!n_root)
	{
		COMPE(t, "Expected an expression");
		return 0;
	}

	//handle postfix
	CHECKIDX(n_root)
	t=tokens+*idx;
	while(t->type==TT_SYMBOL&&is_postfix_op(t->symbol_type))
	{
		ASTNode *n_post;
		switch(t->symbol_type)
		{
		case ST_INC://postfix increment
		case ST_DEC://postfix decrement
			n_post=node_alloc(t->symbol_type==ST_INC?NT_OP_POST_INC:NT_OP_POST_DEC);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;
			break;
		case ST_LPR://function call
		case ST_LBRACKET://subscript
			//id LBRACKET expr_commalist RBRACKET
			//id LPR expr_commalist RPR

			//AST: funccall -> func commalist
			//AST: subscript -> id commalist
			n_post=node_alloc(t->symbol_type==ST_LPR?NT_OP_FUNCCALL:NT_OP_SUBSCRIPT);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;

			CHECKIDX(n_root)
			n_post=parse_expr_commalist(tokens, idx, count);
			node_append_ch(&n_root, n_post);
				
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(n_root->type==NT_OP_FUNCCALL)
			{
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_root;
				}
			}
			else
			{
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACKET))
				{
					COMPE(t, "Expected \']\'");
					return n_root;
				}
			}
			++*idx;
			break;
		case ST_PERIOD://struct member
		case ST_ARROW://pointer member
			n_post=node_alloc(t->symbol_type==ST_PERIOD?NT_OP_MEMBER:NT_OP_ARROW);
			node_append_ch(&n_post, n_root);
			n_root=n_post;
			++*idx;
			
			CHECKIDX(n_root)
			t=tokens+*idx;
			if(t->type!=TT_ID)
			{
				COMPE(t, "Expected an identifier");
				return n_root;
			}
			n_post=node_alloc(NT_TOKEN);
			n_post->t=*t;
			node_append_ch(&n_root, n_post);
			++*idx;
			break;
		}
		CHECKIDX(n_root)
		t=tokens+*idx;
	}
	return n_root;
}
static ASTNode* parse_prefix(Token *tokens, int *idx, int count)
{
	ASTNode *n_root=0, **n_leaf=0;
	Token *t;
	t=tokens+*idx;
	while(is_prefix_op(tokens, *idx, count))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_LPR:
			{
				++*idx;
				parse_type(tokens, idx, count);
				ASTNode *n_pre=node_alloc(type);
				node_attach_ch(&n_root, &n_leaf, n_pre);
				
				CHECKIDX(n_root)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_root;
				}
				++*idx;
			}
			continue;
		case ST_INC:		type=NT_OP_PRE_INC;	break;
		case ST_DEC:		type=NT_OP_PRE_DEC;	break;
		case ST_PLUS:		type=NT_OP_POS;		break;
		case ST_MINUS:		type=NT_OP_NEG;		break;
		case ST_EXCLAMATION:	type=NT_OP_BITWISE_NOT;	break;
		case ST_TILDE:		type=NT_OP_LOGIC_NOT;	break;
		case ST_ASTERISK:	type=NT_OP_DEREFERENCE;	break;
		case ST_AMPERSAND:	type=NT_OP_ADDRESS_OF;	break;
		}
		ASTNode *n_pre=node_alloc(type);
		node_attach_ch(&n_root, &n_leaf, n_pre);
		++*idx;

		CHECKIDX(n_root)
		t=tokens+*idx;
	}
	ASTNode *n_post=parse_postfix(tokens, idx, count);
	if(!n_root)
		return n_post;
	node_append_ch(n_leaf, n_post);
	return n_root;
}
static ASTNode* parse_multiplicative(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_prefix(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_ASTERISK||t->symbol_type==ST_SLASH||t->symbol_type==ST_PERCENT))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_ASTERISK:	type=NT_OP_MUL;	break;
		case ST_SLASH:		type=NT_OP_DIV;	break;
		case ST_PERCENT:	type=NT_OP_MOD;	break;
		}
		ASTNode *n_or=node_alloc(type);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_prefix(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_ASTERISK||t->symbol_type==ST_SLASH||t->symbol_type==ST_PERCENT));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_additive(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_multiplicative(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_PLUS||t->symbol_type==ST_MINUS))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_PLUS?NT_OP_ADD:NT_OP_SUB);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_multiplicative(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_PLUS||t->symbol_type==ST_MINUS));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_shift(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_additive(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_SHIFT_LEFT||t->symbol_type==ST_SHIFT_RIGHT))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_SHIFT_LEFT?NT_OP_SHIFT_LEFT:NT_OP_SHIFT_RIGHT);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_additive(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_SHIFT_LEFT||t->symbol_type==ST_SHIFT_RIGHT));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_inequality(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_shift(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_LESS||t->symbol_type==ST_LESS_EQUAL||t->symbol_type==ST_GREATER||t->symbol_type==ST_GREATER_EQUAL))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_LESS:		type=NT_OP_LESS;		break;
		case ST_LESS_EQUAL:	type=NT_OP_LESS_EQUAL;		break;
		case ST_GREATER:	type=NT_OP_GREATER;		break;
		case ST_GREATER_EQUAL:	type=NT_OP_GREATER_EQUAL;	break;
		}
		ASTNode *n_or=node_alloc(type);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_shift(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_LESS||t->symbol_type==ST_LESS_EQUAL||t->symbol_type==ST_GREATER||t->symbol_type==ST_GREATER_EQUAL));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_equality(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_inequality(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&(t->symbol_type==ST_EQUAL||t->symbol_type==ST_NOT_EQUAL))
	{
		ASTNode *n_or=node_alloc(t->symbol_type==ST_EQUAL?NT_OP_EQUAL:NT_OP_NOT_EQUAL);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_inequality(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&(t->symbol_type==ST_EQUAL||t->symbol_type==ST_NOT_EQUAL));
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_and(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_equality(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_AMPERSAND)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_AND);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_equality(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_AMPERSAND);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_xor(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_and(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_XOR)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_XOR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_and(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_XOR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_bitwise_or(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_xor(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_OR)
	{
		ASTNode *n_or=node_alloc(NT_OP_BITWISE_OR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_xor(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_BITWISE_OR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_logic_and(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_bitwise_or(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_AND)
	{
		ASTNode *n_or=node_alloc(NT_OP_LOGIC_AND);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_bitwise_or(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_AND);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_logic_or(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_logic_and(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_OR)
	{
		ASTNode *n_or=node_alloc(NT_OP_LOGIC_OR);
		node_append_ch(&n_or, n_expr);
		do
		{
			++*idx;

			CHECKIDX(n_expr)
			n_expr=parse_logic_and(tokens, idx, count);
			node_append_ch(&n_or, n_expr);

			CHECKIDX(n_expr)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_LOGIC_OR);
		return n_or;
	}
	return n_expr;
}
static ASTNode* parse_expr_ternary(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_logic_or(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_QUESTION)
	{
		ASTNode *n_ternary=node_alloc(NT_OP_TERNARY);
		node_append_ch(&n_ternary, n_expr);//condition
		++*idx;

		CHECKIDX(n_ternary)
		n_expr=parse_expr_commalist(tokens, idx, count);
		node_append_ch(&n_ternary, n_expr);//expr_true

		CHECKIDX(n_ternary)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
		{
			COMPE(t, "Expected \':\'");
			return n_ternary;
		}
		++*idx;

		n_expr=parse_expr_ternary(tokens, idx, count);
		node_append_ch(&n_ternary, n_expr);//expr_false
		return n_ternary;
	}
	return n_expr;
}
static int is_assign_op(SymbolType t)
{
	switch(t)
	{
	case ST_ASSIGN:
	case ST_ASSIGN_ADD:
	case ST_ASSIGN_SUB:
	case ST_ASSIGN_MUL:
	case ST_ASSIGN_DIV:
	case ST_ASSIGN_MOD:
	case ST_ASSIGN_SHIFT_LEFT:
	case ST_ASSIGN_SHIFT_RIGHT:
	case ST_ASSIGN_AND:
	case ST_ASSIGN_XOR:
	case ST_ASSIGN_OR:
		return 1;
	}
	return 0;
}
static ASTNode* parse_expr_assign(Token *tokens, int *idx, int count)
{
	Token *t;
	ASTNode *n_expr=parse_expr_ternary(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&is_assign_op(t->symbol_type))
	{
		ASTNodeType type=NT_TOKEN;
		switch(t->symbol_type)
		{
		case ST_ASSIGN:			type=NT_OP_ASSIGN;	break;
		case ST_ASSIGN_ADD:		type=NT_OP_ASSIGN_ADD;	break;
		case ST_ASSIGN_SUB:		type=NT_OP_ASSIGN_SUB;	break;
		case ST_ASSIGN_MUL:		type=NT_OP_ASSIGN_MUL;	break;
		case ST_ASSIGN_DIV:		type=NT_OP_ASSIGN_DIV;	break;
		case ST_ASSIGN_MOD:		type=NT_OP_ASSIGN_MOD;	break;
		case ST_ASSIGN_SHIFT_LEFT:	type=NT_OP_ASSIGN_SL;	break;
		case ST_ASSIGN_SHIFT_RIGHT:	type=NT_OP_ASSIGN_SR;	break;
		case ST_ASSIGN_AND:		type=NT_OP_ASSIGN_AND;	break;
		case ST_ASSIGN_XOR:		type=NT_OP_ASSIGN_XOR;	break;
		case ST_ASSIGN_OR:		type=NT_OP_ASSIGN_OR;	break;
		}
		ASTNode *n_assign=node_alloc(type);
		node_append_ch(&n_assign, n_expr);
		++*idx;
		n_expr=parse_expr_assign(tokens, idx, count);
		node_append_ch(&n_assign, n_expr);
		return n_assign;
	}
	return n_expr;
}
static ASTNode* parse_expr_commalist(Token *tokens, int *idx, int count)
{
	//precedence:
	//	postfix		(highest prec)
	//	prefix
	//	multiplicative
	//	additive
	//	shift
	//	inequality
	//	comparison
	//	bitwise and
	//	bitwise xor
	//	bitwise or
	//	logic and
	//	logic or
	//	ternary conditional
	//	assignment
	//	comma list	(lowest prec)

	//expr [COMMA expr]

	//AST: comma -> expr_assign*
	Token *t;
	ASTNode *n_expr=parse_expr_assign(tokens, idx, count);
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA)
	{
		ASTNode *n_comma=node_alloc(NT_OP_COMMA);
		node_append_ch(&n_comma, n_expr);
		do
		{
			++*idx;
			n_expr=parse_expr_assign(tokens, idx, count);
			node_append_ch(&n_comma, n_expr);

			CHECKIDX(n_comma)
			t=tokens+*idx;
		}while(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA);
		return n_comma;
	}
	return n_expr;
}
static int is_typespecifier(Token *tokens, int idx, int count)
{
	Token *t=tokens+idx;
	if(t->type==TT_KEYWORD)
	{
		switch(t->keyword_type)
		{
		case KT_extern:
		case KT_static:
		case KT_const:
		case KT_signed:
		case KT_unsigned:
		case KT_void:
		case KT_char:
		case KT_int:
		case KT_long:
		case KT_float:
		case KT_double:
			return 1;
		}
	}
	return 0;
}
static ASTNode* parse_stmt_decl(Token *tokens, int *idx, int count)
{
	Token *t;
	if(is_typespecifier(tokens, *idx, count))
		return parse_declaration(tokens, idx, count, 0);
	ASTNode *n_expr=parse_expr_commalist(tokens, idx, count);
	
	CHECKIDX(n_expr)
	t=tokens+*idx;
	if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
	{
		COMPE(t, "Expected \';\'");
		return n_expr;
	}
	++*idx;

	return n_expr;
}
static int is_label_togo(Token *tokens, int idx, int count)
{
	Token *t;
	if(idx>=count)
		return 0;
	t=tokens+idx;
	if(t->type!=TT_ID)
		return 0;
	++idx;
	if(idx>=count)
		return 0;
	t=tokens+idx;
	return t->type==TT_SYMBOL&&t->symbol_type==ST_COLON;
}
static ASTNode* parse_statement(Token *tokens, int *idx, int count)
{
	Token *t=tokens+*idx;
	if(t->type==TT_KEYWORD)
	{
		switch(t->keyword_type)
		{
		case KT_if:
			{
				//IF LPR condition RPR truebody [ELSE falsebody]
				
				//AST: stmt_if -> condition truebody [falsebody]
				ASTNode *n_if=node_alloc(NT_STMT_IF);
				++*idx;

				CHECKIDX(n_if)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_if;
				}
				++*idx;

				CHECKIDX(n_if)
				ASTNode *n_condition=parse_expr_commalist(tokens, idx, count);//if condition
				node_append_ch(&n_if, n_condition);
				
				CHECKIDX(n_if)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_if;
				}
				++*idx;

				CHECKIDX(n_if)
				t=tokens+*idx;
				ASTNode *n_truebody=parse_statement(tokens, idx, count);//if body
				node_append_ch(&n_if, n_truebody);

				CHECKIDX(n_if)
				t=tokens+*idx;
				if(t->type==TT_KEYWORD&&t->keyword_type==KT_else)//optional else with body
				{
					++*idx;
					CHECKIDX(n_if)
					t=tokens+*idx;
					ASTNode *n_falsebody=parse_statement(tokens, idx, count);//else body
					node_append_ch(&n_if, n_falsebody);
				}
				return n_if;
			}
			break;
		case KT_for:
			{
				//FOR LPR init_decl cond_expr SEMICOLON inc_expr RPR loopbody

				//AST: stmt_for -> init cond inc body
				ASTNode *n_for=node_alloc(NT_STMT_FOR);
				++*idx;

				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_init=parse_stmt_decl(tokens, idx, count);//for initialization
				node_append_ch(&n_for, n_init);

				CHECKIDX(n_for)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);//for condition
				node_append_ch(&n_for, n_cond);

				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_inc=parse_expr_commalist(tokens, idx, count);//for increment
				node_append_ch(&n_for, n_inc);
				CHECKIDX(n_for)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_for;
				}
				++*idx;

				CHECKIDX(n_for)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);//for body
				node_append_ch(&n_for, n_loopbody);

				return n_for;
			}
			break;
		case KT_do:
			{
				//DO loopbody WHILE LPR cond RPR SEMICOLON

				//AST: stms_doWhile -> body condition
				ASTNode *n_doWhile=node_alloc(NT_STMT_DO_WHILE);
				++*idx;
				CHECKIDX(n_doWhile)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);
				node_append_ch(&n_doWhile, n_loopbody);

				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_KEYWORD&&t->keyword_type==KT_while))
				{
					COMPE(t, "Expected \'while\'");
					return n_doWhile;
				}
				++*idx;
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_doWhile;
				}
				++*idx;

				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_doWhile, n_cond);
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_doWhile;
				}
				++*idx;
				
				CHECKIDX(n_doWhile)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_doWhile;
				}
				++*idx;
				return n_doWhile;
			}
			break;
		case KT_while:
			{
				//WHILE LPR condition RPR loopbody
				
				//AST: stmt_while -> condition body
				ASTNode *n_while=node_alloc(NT_STMT_WHILE);
				++*idx;
				
				CHECKIDX(n_while)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_while;
				}
				++*idx;

				CHECKIDX(n_while)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_while, n_cond);
				
				CHECKIDX(n_while)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_while;
				}
				++*idx;
				
				CHECKIDX(n_while)
				ASTNode *n_loopbody=parse_statement(tokens, idx, count);
				node_append_ch(&n_while, n_loopbody);
				return n_while;
			}
			break;
		case KT_continue:
			{
				//CONTINUE SEMICOLON

				//AST: continue
				ASTNode *n_continue=node_alloc(NT_STMT_CONTINUE);
				++*idx;
				
				CHECKIDX(n_continue)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_continue;
				}
				++*idx;
				return n_continue;
			}
			break;
		case KT_break:
			{
				//BREAK SEMICOLON

				//AST: break
				ASTNode *n_break=node_alloc(NT_STMT_CONTINUE);
				++*idx;
				
				CHECKIDX(n_break)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_break;
				}
				++*idx;
				return n_break;
			}
			break;
		case KT_switch:
			{
				//Node: code without case/default labels is valid but never gets executed
				//to declare variables, use braces

				//SWITCH LPR cond RPR LBRACE [(CASE expr)|DEFAULT COLON code]+ RBRACE

				//AST: switch -> cond cases*
				ASTNode *n_switch=node_alloc(NT_STMT_SWITCH);
				++*idx;
				
				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				ASTNode *n_cond=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_switch, n_cond);

				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
				{
					COMPE(t, "Expected \')\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LBRACE))
				{
					COMPE(t, "Expected \'{\'");
					return n_switch;
				}
				++*idx;
				
				CHECKIDX(n_switch)
				ASTNode *n_body=parse_code(tokens, idx, count);

				CHECKIDX(n_switch)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE))
				{
					COMPE(t, "Expected \'}\'");
					return n_switch;
				}
				++*idx;
				return n_switch;
			}
			break;
		case KT_case:
			{
				//CASE expr COLON

				//AST: case ->expr
				ASTNode *n_case=node_alloc(NT_LABEL_CASE);
				++*idx;

				CHECKIDX(n_case)
				ASTNode *n_body=parse_expr_ternary(tokens, idx, count);
				
				CHECKIDX(n_case)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
				{
					COMPE(t, "Expected \':\'");
					return n_case;
				}
				++*idx;

				return n_case;
			}
			break;
		case KT_default:
			{
				//DEFAULT COLON
				ASTNode *n_default=node_alloc(NT_LABEL_DEFAULT);
				++*idx;

				CHECKIDX(n_default)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_COLON))
				{
					COMPE(t, "Expected \':\'");
					return n_default;
				}
				++*idx;

				return n_default;
			}
			break;
		case KT_goto:
			{
				//GOTO identifier SEMICOLON

				//AST: goto -> id
				ASTNode *n_goto=node_alloc(NT_STMT_GOTO);
				++*idx;

				CHECKIDX(n_goto)
				t=tokens+*idx;
				if(t->type!=TT_ID)
				{
					COMPE(t, "Expected an identifier");
					return n_goto;
				}
				ASTNode *n_label=node_alloc(NT_TOKEN);
				n_label->t=*t;
				++*idx;
				
				CHECKIDX(n_goto)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_goto;
				}
				++*idx;
				return n_goto;
			}
			break;
		case KT_return:
			{
				//RETURN commalist SEMICOLON

				//AST: return -> expr
				ASTNode *n_ret=node_alloc(NT_STMT_RETURN);
				++*idx;

				ASTNode *n_expr=parse_expr_commalist(tokens, idx, count);
				node_append_ch(&n_ret, n_expr);
				
				CHECKIDX(n_ret)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
				{
					COMPE(t, "Expected \';\'");
					return n_ret;
				}
				++*idx;

				return n_ret;
			}
			break;
		default://can be decl_expr
			return parse_stmt_decl(tokens, idx, count);
		}
	}
	else if(t->type==TT_SYMBOL)
	{
		switch(t->symbol_type)
		{
		case ST_LBRACE:
			{
				ASTNode *n_scope=node_alloc(NT_SCOPE);
				++*idx;

				CHECKIDX(n_scope)
				ASTNode *n_code=parse_code(tokens, idx, count);
				node_append_ch(&n_scope, n_code);

				CHECKIDX(n_scope)
				t=tokens+*idx;
				if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
				{
					COMPE(t, "Expected \'(\'");
					return n_scope;
				}
				++*idx;

				return n_scope;
			}
			break;
		default://can be decl_expr
			return parse_stmt_decl(tokens, idx, count);
		}
	}
	else if(t->type==TT_ID&&is_label_togo(tokens, *idx, count))
	{
		//id COLON

		//AST: LABEL_TOGO -> id
		ASTNode *n_label=node_alloc(NT_LABEL_TOGO);
		ASTNode *n_id=node_alloc(NT_TOKEN);
		n_id->t=*t;
		++*idx;

		CHECKIDX(n_label)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR))
		{
			COMPE(t, "Expected \'(\'");
			return n_label;
		}
		++*idx;
		return n_label;
	}
	return parse_stmt_decl(tokens, idx, count);
}
static ASTNode* parse_code(Token *tokens, int *idx, int count)
{
	ASTNode *n_code=node_alloc(NT_FUNCBODY);
	while(*idx<count)
	{
		Token *t=tokens+*idx;
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE)
			break;
		int idx0=*idx;
		ASTNode *n_decl=parse_statement(tokens, idx, count);
		if(n_decl)
			node_append_ch(&n_code, n_decl);
		if(*idx==idx0)
		{
			COMPE(tokens+*idx, "INTERNAL ERROR: PARSER STUCK");
			++*idx;
		}
	}
	return n_code;
}
static ASTNode* parse_declaration(Token *tokens, int *idx, int count, int global)
{
	//declaration :=
	//	type id LPR funcheader RPR LBRACE CODE BBRACE
	//	type (vardecl|funcdecl) [COMMA (vardecl|funcdecl) ]* SEMICOLON

	//AST: type -> funcdef | declaration*		TODO func ptr
	
	short indirection_count=0;
	Token *t;
	ASTNode *n_type=parse_type(tokens, idx, count);

	CHECKIDX(n_type)
	t=tokens+*idx;
	while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
	{
		++indirection_count;
		++*idx;

		CHECKIDX(n_type)
		t=tokens+*idx;
	}
	if(t->type!=TT_ID)
	{
		COMPE(t, "Expected an identifier");
		return n_type;
	}
	ASTNode *n_name=node_alloc(NT_TOKEN);
	n_name->indirection_count=indirection_count;
	n_name->t=*t;
	node_append_ch(&n_type, n_name);
	++*idx;
	CHECKIDX(n_type)
	t=tokens+*idx;
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//function header
	{
		++*idx;
		CHECKIDX(n_type)
		ASTNode *n_header=parse_funcheader(tokens, idx, count);
		node_append_ch(n_type->ch+n_type->nch-1, n_header);
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
		{
			COMPE(t, "Expeted \')\'");
			return n_type;
		}
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(global&&t->type==TT_SYMBOL&&t->symbol_type==ST_LBRACE)//funcbody
		{
			n_type->indirection_count=n_name->indirection_count;//move indirection_count from funcname to return_type
			n_name->indirection_count=0;
			++*idx;

			CHECKIDX(n_type)
			ASTNode *n_body=parse_code(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_body);

			CHECKIDX(n_type)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RBRACE))
				COMPE(t, "Expected \'}\'");
			++*idx;
			return n_type;
		}
	}
	if(t->type==TT_SYMBOL&&t->symbol_type==ST_ASSIGN)//initialization
	{
		++*idx;
		CHECKIDX(n_type)
		ASTNode *n_init=parse_expr_ternary(tokens, idx, count);
		node_append_ch(n_type->ch+n_type->nch-1, n_init);

		CHECKIDX(n_type)
		t=tokens+*idx;
	}
	while(t->type==TT_SYMBOL&&t->symbol_type==ST_COMMA)
	{
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		while(t->type==TT_SYMBOL&&t->symbol_type==ST_ASTERISK)//TODO parse pointer to array/funcptr	int (*x)[10];
		{
			++indirection_count;
			++*idx;

			CHECKIDX(n_type)
			t=tokens+*idx;
		}
		if(t->type!=TT_ID)
		{
			COMPE(t, "Expected an identifier");
			break;
		}
		ASTNode *n_name=node_alloc(NT_TOKEN);
		n_name->indirection_count=indirection_count;
		n_name->t=*t;
		node_append_ch(&n_type, n_name);
		++*idx;
		CHECKIDX(n_type)
		t=tokens+*idx;
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_LPR)//function header
		{
			++*idx;
			ASTNode *n_header=parse_funcheader(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_header);
			CHECKIDX(n_type)
			t=tokens+*idx;
			if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_RPR))
			{
				COMPE(t, "Expeted \')\'");
				return n_type;
			}
			++*idx;
			CHECKIDX(n_type)
			t=tokens+*idx;
		}
		if(t->type==TT_SYMBOL&&t->symbol_type==ST_ASSIGN)//initialization
		{
			++*idx;
			CHECKIDX(n_type)
			ASTNode *n_init=parse_expr_ternary(tokens, idx, count);
			node_append_ch(n_type->ch+n_type->nch-1, n_init);
		}
	}
	if(!(t->type==TT_SYMBOL&&t->symbol_type==ST_SEMICOLON))
	{
		COMPE(t, "Expeted \';\'");
		return n_type;
	}
	++*idx;
	return n_type;
}
ASTNode* parse_program(Token *tokens, int count)//program := declaration*
{
	ASTNode *n_prog=node_alloc(NT_PROGRAM);
	for(int idx=0;idx<count;)
	{
		int idx0=idx;
		ASTNode *n_decl=parse_declaration(tokens, &idx, count, 1);
		if(n_decl)
			node_append_ch(&n_prog, n_decl);
		if(idx==idx0)
		{
			COMPE(tokens+idx, "INTERNAL ERROR: PARSER STUCK");
			++idx;
		}
	}
	return n_prog;
}

void print_ast(ASTNode *root, int depth)
{
	if(!root)
		return;
	printf("%*s@", depth<<1, "");//indentation
	switch(root->type)
	{
	case NT_TOKEN:
		for(int k=0;k<root->indirection_count;++k)
			printf("*");
		print_token(&root->t);
		break;
	case NT_PROGRAM:
		printf("program");
		break;
	case NT_TYPE:
		if(root->is_extern)
			printf("extern ");
		if(root->is_static)
			printf("static ");
		if(root->is_const)
			printf("const ");
		if(root->is_float)
		{
			if(root->nbits==32)
				printf("float");
			else
				printf("double");
		}
		else
		{
			if(root->is_unsigned)
				printf("unsigned ");
			switch(root->nbits)
			{
			case  0:printf("void");break;
			case  8:printf("char");break;
			case 16:printf("short");break;
			case 32:printf("int");break;
			case 64:printf("long");break;
			default:printf("bitfield:%d", root->nbits);break;
			}
		}
		break;
	case NT_FUNCHEADER:
		printf("funcheader");
		break;
	case NT_ARGDECL:
		for(int k=0;k<root->indirection_count;++k)
			printf("*");
		print_token(&root->t);
		printf(" (funcarg)");
		break;
	case NT_ARGDECL_ELLIPSIS:
		printf("... (ellipsis_arg)");
		break;
	case NT_FUNCBODY:
		printf("funcbody");
		break;
	case NT_SCOPE:
		printf("scope{}");
		break;
	case NT_STMT_IF:
		printf("if");
		break;
	case NT_STMT_FOR:
		printf("for");
		break;
	case NT_STMT_DO_WHILE:
		printf("doWhile");
		break;
	case NT_STMT_WHILE:
		printf("while");
		break;
	case NT_STMT_CONTINUE:
		printf("continue");
		break;
	case NT_STMT_BREAK:
		printf("break");
		break;
	case NT_STMT_SWITCH:
		printf("switch");
		break;
	case NT_LABEL_CASE:
		printf("case");
		break;
	case NT_LABEL_DEFAULT:
		printf("default");
		break;
	case NT_STMT_GOTO:
		printf("goto");
		break;
	case NT_LABEL_TOGO:
		printf("label:");
		break;
	case NT_STMT_RETURN:
		printf("return");
		break;
	case NT_OP_ARROW:
		printf("->");
		break;
	case NT_OP_MEMBER:
		printf(". (member)");
		break;
	case NT_OP_SUBSCRIPT:
		printf("[]");
		break;
	case NT_OP_FUNCCALL:
		printf("funccall");
		break;
	case NT_OP_POST_INC:
		printf("post++");
		break;
	case NT_OP_POST_DEC:
		printf("post--");
		break;
	case NT_OP_ADDRESS_OF:
		printf("&address_of");
		break;
	case NT_OP_DEREFERENCE:
		printf("*dereference");
		break;
	case NT_OP_CAST:
		printf("(cast)...");
		break;
	case NT_OP_LOGIC_NOT:
		printf("!logic_not");
		break;
	case NT_OP_BITWISE_NOT:
		printf("~bitwise_not");
		break;
	case NT_OP_POS:
		printf("+pos");
		break;
	case NT_OP_NEG:
		printf("-neg");
		break;
	case NT_OP_PRE_INC:
		printf("pre++");
		break;
	case NT_OP_PRE_DEC:
		printf("pre--");
		break;
	case NT_OP_MUL:
		printf("a*b (mul)");
		break;
	case NT_OP_DIV:
		printf("a/b (div)");
		break;
	case NT_OP_MOD:
		printf("a%%b (mod)");
		break;
	case NT_OP_ADD:
		printf("a+b (add)");
		break;
	case NT_OP_SUB:
		printf("a-b (sub)");
		break;
	case NT_OP_SHIFT_LEFT:
		printf("a<<b (shift_left)");
		break;
	case NT_OP_SHIFT_RIGHT:
		printf("a>>b (shift_right)");
		break;
	case NT_OP_LESS:
		printf("a<b (less)");
		break;
	case NT_OP_LESS_EQUAL:
		printf("a<=b (less_equal)");
		break;
	case NT_OP_GREATER:
		printf("a>b (greater)");
		break;
	case NT_OP_GREATER_EQUAL:
		printf("a>=b (greater_equal)");
		break;
	case NT_OP_EQUAL:
		printf("a==b (equal)");
		break;
	case NT_OP_NOT_EQUAL:
		printf("a!=b (not_equal)");
		break;
	case NT_OP_BITWISE_AND:
		printf("a&b (bitwise_and)");
		break;
	case NT_OP_BITWISE_XOR:
		printf("a^b (bitwise_xor)");
		break;
	case NT_OP_BITWISE_OR:
		printf("a|b (bitwise_or)");
		break;
	case NT_OP_LOGIC_AND:
		printf("a&&b (logic_and)");
		break;
	case NT_OP_LOGIC_OR:
		printf("a||b (logic_or)");
		break;
	case NT_OP_TERNARY:
		printf("a?b:c (ternary)");
		break;
	case NT_OP_ASSIGN:
		printf("a=b (assign)");
		break;
	case NT_OP_ASSIGN_ADD:
		printf("a+=b (assign_add)");
		break;
	case NT_OP_ASSIGN_SUB:
		printf("a-=b (assign_sub)");
		break;
	case NT_OP_ASSIGN_MUL:
		printf("a*=b (assign_mul)");
		break;
	case NT_OP_ASSIGN_DIV:
		printf("a/=b (assign_div)");
		break;
	case NT_OP_ASSIGN_MOD:
		printf("a%%=b (assign_mod)");
		break;
	case NT_OP_ASSIGN_SL:
		printf("a<<=b (assign_sl)");
		break;
	case NT_OP_ASSIGN_SR:
		printf("a>>=b (assign_sr)");
		break;
	case NT_OP_ASSIGN_AND:
		printf("a&=b (assign_and)");
		break;
	case NT_OP_ASSIGN_XOR:
		printf("a^=b (assign_xor)");
		break;
	case NT_OP_ASSIGN_OR:
		printf("a|=b (assign_or)");
		break;
	case NT_OP_COMMA:
		printf("a,b (comma)");
		break;
	default:
		printf("UNRECOGNIZED NODETYPE: %d", root->type);
		break;
	}
	printf("\n");
	++depth;
	for(int k=0;k<root->nch;++k)
		print_ast(root->ch[k], depth);
}