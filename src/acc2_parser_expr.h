#ifdef FUNC_NAME
bool		FUNC_NAME(IRNode *&root)//shift.expr  :=  additive.expr  |  shift.expr ShiftOp additive.expr
{
	if(!NEXT_CALL(root))
		return free_tree(root);
	auto t=LOOK_AHEAD(0);
	if(CONDITION)
	{
		IRNode *node=nullptr;
		INSERT_NONLEAF(root, node, LABEL);
		do
		{
			ADVANCE;
			if(!NEXT_CALL(node))
				return free_tree(root);
			node->opsign=t;
			root->children.push_back(node);
			t=LOOK_AHEAD(0);
		}while(CONDITION);
	}
	return true;
}
#endif