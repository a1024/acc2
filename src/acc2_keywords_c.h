#ifdef TOKEN
//the 3rd argument of the TOKEN() macro is the lex flag:
//	0: no restrictions on following character
//	1: identifier-like keyword: following character shouldn't be an alphanumeric

	TOKEN(0, CT_IGNORED, 0)//preprocessed away		//negative values are ignored too	X

	TOKEN(0, CT_VAL_INTEGER, 0)//idata		//TODO: differentiate between different integer types
	TOKEN(0, CT_VAL_FLOAT, 0)
//	TOKEN(0, CT_VAL_DOUBLE, 0)//TODO: differentiate between float & double
	TOKEN(0, CT_ID, 0)//identifier
	TOKEN(0, CT_VAL_STRING_LITERAL, 0)
	TOKEN(0, CT_VAL_WSTRING_LITERAL, 0)//identical utf-8 like CT_VAL_STRING_LITERAL, just a type for compiler
	TOKEN(0, CT_VAL_CHAR_LITERAL, 0)//ival, stores up to 8 characters (MSVC: 4)
//	TOKEN(0, CT_VAL_WCHAR_LITERAL, 0)
	TOKEN(0, CT_INCLUDENAME_STD, 0)
	TOKEN(0, CT_INCLUDENAME_CUSTOM, 0)//deprecated

	//type keywords
	TOKEN("typedef", CT_TYPEDEF, 1) TOKEN("enum", CT_ENUM, 1)
	TOKEN("struct", CT_STRUCT, 1) TOKEN("class", CT_CLASS, 1) TOKEN("union", CT_UNION, 1)
	TOKEN("template", CT_TEMPLATE, 1) TOKEN("typename", CT_TYPENAME, 1)
	
	TOKEN("constexpr", CT_CONSTEXPR, 1) TOKEN("inline", CT_INLINE, 1)
	TOKEN("extern", CT_EXTERN, 1) TOKEN("static", CT_STATIC, 1) TOKEN("mutable", CT_MUTABLE, 1)
	TOKEN("const", CT_CONST, 1) TOKEN("volatile", CT_VOLATILE, 1)
	TOKEN("register", CT_REGISTER, 1)
	TOKEN("auto", CT_AUTO, 1) TOKEN("decltype", CT_DECLTYPE, 1)
	TOKEN("signed", CT_SIGNED, 1) TOKEN("unsigned", CT_UNSIGNED, 1)
	TOKEN("void", CT_VOID, 1)
	TOKEN("bool", CT_BOOL, 1)
	TOKEN("char", CT_CHAR, 1) TOKEN("short", CT_SHORT, 1)
	TOKEN("int", CT_INT, 1) TOKEN("long", CT_LONG, 1)
	TOKEN("float", CT_FLOAT, 1) TOKEN("double", CT_DOUBLE, 1)
	TOKEN("wchar_t", CT_WCHAR_T, 1)
	TOKEN("__int8", CT_INT8, 1) TOKEN("__int16", CT_INT16, 1) TOKEN("__int32", CT_INT32, 1) TOKEN("__int64", CT_INT64, 1)
	TOKEN("__m128", CT_M128, 1) TOKEN("__m128i", CT_M128I, 1) TOKEN("__m128d", CT_M128D, 1)

	//call convensions
	TOKEN("__cdecl", CT_CDECL, 1)//default
	TOKEN("__thiscall", CT_THISCALL, 1)//class member functions
	TOKEN("__stdcall", CT_STDCALL, 1)//WinAPI

	//constants
	TOKEN("nullptr", CT_NULLPTR, 1)
	TOKEN("false", CT_FALSE, 1) TOKEN("true", CT_TRUE, 1)

	//special
	TOKEN("new", CT_NEW, 1) TOKEN("delete", CT_DELETE, 1) TOKEN("this", CT_THIS, 1)
	TOKEN("public", CT_PUBLIC, 1) TOKEN("private", CT_PRIVATE, 1) TOKEN("protected", CT_PROTECTED, 1)
	TOKEN("friend", CT_FRIEND, 1) TOKEN("virtual", CT_VIRTUAL, 1)
	TOKEN("namespace", CT_NAMESPACE, 1) TOKEN("using", CT_USING, 1)
	TOKEN("static_assert", CT_STATIC_ASSERT, 1)
	TOKEN("operator", CT_OPERATOR, 1)
	TOKEN("typeid", CT_TYPEID, 1)
	TOKEN("asm", CT_ASM_ATT, 1) TOKEN("__asm", CT_ASM_INTEL, 1)
	
	//control keywords
	TOKEN("if", CT_IF, 1) TOKEN("else", CT_ELSE, 1)
	TOKEN("for", CT_FOR, 1)
	TOKEN("do", CT_DO, 1) TOKEN("while", CT_WHILE, 1)
	TOKEN("switch", CT_SWITCH, 1) TOKEN("case", CT_CASE, 1) TOKEN("default", CT_DEFAULT, 1)
	TOKEN("break", CT_BREAK, 1) TOKEN("continue", CT_CONTINUE, 1)
	TOKEN("goto", CT_GOTO, 1)
	TOKEN("return", CT_RETURN, 1)
	TOKEN("try", CT_TRY, 1) TOKEN("catch", CT_CATCH, 1) TOKEN("throw", CT_THROW, 1)

	//non-overloadable operators:	.* . :: sizeof ?: typeid

	//symbols & operators
	TOKEN("sizeof", CT_SIZEOF, 1)
	TOKEN("::", CT_SCOPE, 0)
	TOKEN("{", CT_LBRACE, 0) TOKEN("}", CT_RBRACE, 0)//not an operator
	TOKEN("(", CT_LPR, 0) TOKEN(")", CT_RPR, 0)
	TOKEN("[", CT_LBRACKET, 0) TOKEN("]", CT_RBRACKET, 0)
	TOKEN(".", CT_PERIOD, 0) TOKEN("->", CT_ARROW, 0)
	TOKEN("++", CT_INCREMENT, 0) TOKEN("--", CT_DECREMENT, 0)
	TOKEN("!", CT_EXCLAMATION, 0) TOKEN("~", CT_TILDE, 0)
	TOKEN(".*", CT_DOT_STAR, 0) TOKEN("->*", CT_ARROW_STAR, 0)
	TOKEN("*", CT_ASTERIX, 0) TOKEN("/", CT_SLASH, 0) TOKEN("%", CT_MODULO, 0)
	TOKEN("+", CT_PLUS, 0) TOKEN("-", CT_MINUS, 0)
	TOKEN("<<", CT_SHIFT_LEFT, 0) TOKEN(">>", CT_SHIFT_RIGHT, 0)
	TOKEN("<=>", CT_THREEWAY, 0)//since C++20
	TOKEN("<", CT_LESS, 0) TOKEN("<=", CT_LESS_EQUAL, 0) TOKEN(">", CT_GREATER, 0) TOKEN(">=", CT_GREATER_EQUAL, 0)
	TOKEN("==", CT_EQUAL, 0) TOKEN("!=", CT_NOT_EQUAL, 0)
	TOKEN("&", CT_AMPERSAND, 0) TOKEN("^", CT_CARET, 0) TOKEN("|", CT_VBAR, 0)
	TOKEN("&&", CT_LOGIC_AND, 0) TOKEN("||", CT_LOGIC_OR, 0)
	TOKEN("?", CT_QUESTION, 0) TOKEN(":", CT_COLON, 0)

	TOKEN("=", CT_ASSIGN, 0)
	TOKEN("*=", CT_ASSIGN_MUL, 0) TOKEN("/=", CT_ASSIGN_DIV, 0) TOKEN("%=", CT_ASSIGN_MOD, 0)
	TOKEN("+=", CT_ASSIGN_ADD, 0) TOKEN("-=", CT_ASSIGN_SUB, 0)
	TOKEN("<<=", CT_ASSIGN_SL, 0) TOKEN(">>=", CT_ASSIGN_SR, 0)
	TOKEN("&=", CT_ASSIGN_AND, 0) TOKEN("^=", CT_ASSIGN_XOR, 0) TOKEN("|=", CT_ASSIGN_OR, 0)
	
	TOKEN(",", CT_COMMA, 0) TOKEN(";", CT_SEMICOLON, 0)//comma is overloadable, but semicolon isn't an operator
	TOKEN("...", CT_ELLIPSIS, 0)

	//preprocessor
	TOKEN("#", CT_HASH, 0)
	TOKEN("include", CT_INCLUDE, 1)
	TOKEN("define", CT_DEFINE, 1) TOKEN("undef", CT_UNDEF, 1) TOKEN("##", CT_CONCATENATE, 0) TOKEN("__VA_ARGS__", CT_VA_ARGS, 1)
	TOKEN("ifdef", CT_IFDEF, 1) TOKEN("ifndef", CT_IFNDEF, 1) TOKEN("elif", CT_ELIF, 1) TOKEN("defined", CT_DEFINED, 1) TOKEN("endif", CT_ENDIF, 1)
	TOKEN("error", CT_ERROR, 1)
	TOKEN("pragma", CT_PRAGMA, 1)
	//TOKEN("line", CT_DIRECTIVE_LINE)

	//predefined macros
	TOKEN("__FILE__", CT_FILE, 1) TOKEN("__LINE__", CT_LINE, 1) TOKEN("__DATE__", CT_DATE, 1) TOKEN("__TIME__", CT_TIME, 1) TOKEN("__TIMESTAMP__", CT_TIMESTAMP, 1) TOKEN("__func__", CT_FUNC, 1)
	//TOKEN("__ACC2__", CT_ACC2, 1)

	//lexer
	TOKEN("\'", CT_QUOTE, 0) TOKEN("\"", CT_DOUBLEQUOTE, 0) TOKEN("L\"", CT_DQUOTE_WIDE, 0) TOKEN("R\"", CT_DQUOTE_RAW, 0)
	TOKEN("\n", CT_NEWLINE, 0)
	TOKEN("//", CT_LINECOMMENTMARK, 0) TOKEN("/*", CT_BLOCKCOMMENTMARK, 0)

	//preprocessor flags
	TOKEN(0, CT_MACRO_ARG, 0)
	TOKEN(0, CT_LEXME, 0)//see sdata, result of token pasting

	//parser IR stuff
	TOKEN(0, PT_PROGRAM, 0)
	TOKEN(0, PT_NAMESPACE_ALIAS, 0)
	TOKEN(0, PT_LINKAGE_BODY, 0)
	TOKEN(0, PT_ENUM_BODY, 0)
	TOKEN(0, PT_DECLARATION, 0) TOKEN(0, PT_SIMPLE_DECLARATION, 0)
	TOKEN(0, PT_DECLARATOR, 0) TOKEN(0, PT_DECLARATOR_PARENS, 0)
	TOKEN(0, PT_TYPE, 0) TOKEN(0, PT_TYPESPEC, 0)
	TOKEN(0, PT_VAR_DECL, 0) TOKEN(0, PT_VAR_DEF, 0) TOKEN(0, PT_VAR_REF, 0)
	TOKEN(0, PT_FUNC_DECL, 0) TOKEN(0, PT_FUNC_DEF, 0) TOKEN(0, PT_FUNC_ARGS, 0) TOKEN(0, PT_ARG_DECL, 0) TOKEN(0, PT_ARG_DECLLIST, 0) TOKEN(0, PT_MEMBER_INIT_LIST, 0) TOKEN(0, PR_MEMBER_INIT, 0)
	TOKEN(0, PT_CODE_BLOCK, 0) TOKEN(0, PT_INITIALIZER_LIST, 0)
	TOKEN(0, PT_BASE_SPECIFIER, 0) TOKEN(0, PT_CLASS_BODY, 0) TOKEN(0, PT_CONSTRUCTOR_DECL, 0)
	TOKEN(0, PT_JUMPLABEL, 0)
	TOKEN(0, PT_MEMBER_SPEC, 0)//opsign consists of: {bit 2: friend, bit 1: virtual, bit 0: inline}
	TOKEN(0, PT_STORAGE_SPEC, 0)//opsign is one of: extern/static/mutable/register
	TOKEN(0, PT_CV_QUALIFIER, 0)//opsign consists of: {bit 1: is_volatile, bit 0: is_const}
	TOKEN(0, PT_DESTRUCTOR, 0)
	//...
	TOKEN(0, PT_EXTERN_TEMPLATE, 0)
	TOKEN(0, PT_TEMPLATE_ARG_LIST, 0)//in a template declaration
	TOKEN(0, PT_TEMPLATE_ARGS, 0)//in a template invocation
	TOKEN(0, PT_TEMPLATE_ARG, 0)
	//...
	TOKEN(0, PT_COMMA, 0)
	TOKEN(0, PT_ASSIGN, 0)
	TOKEN(0, PT_CONDITIONAL, 0)
	TOKEN(0, PT_LOGICOR, 0)
	TOKEN(0, PT_LOGICAND, 0)
	TOKEN(0, PT_BITOR, 0)
	TOKEN(0, PT_BITXOR, 0)
	TOKEN(0, PT_BITAND, 0)
	TOKEN(0, PT_EQUALITY, 0)
	TOKEN(0, PT_RELATIONAL, 0)
	TOKEN(0, PT_SHIFT, 0)
	TOKEN(0, PT_ADD, 0)
	TOKEN(0, PT_MUL, 0)
	TOKEN(0, PT_POINTER_TO_MEMBER, 0)//.*, ->*
	TOKEN(0, PT_UNARY_PRE, 0)
	TOKEN(0, PT_PRE_INCREMENT, 0) TOKEN(0, PT_POST_INCREMENT, 0)
	TOKEN(0, PT_PRE_DECREMENT, 0) TOKEN(0, PT_POST_DECREMENT, 0)
	TOKEN(0, PT_FUNC_CALL, 0) TOKEN(0, PT_ARRAY_SUBSCRIPT, 0)
	TOKEN(0, PT_CAST, 0)
	TOKEN(0, PT_NAME, 0)

	TOKEN(0, PT_PTR_OPERATOR, 0)
	TOKEN(0, PT_NEW_DECLARATOR, 0) TOKEN(0, PT_ALLOCATE_TYPE, 0)
	//...

	//parser state 2
	TOKEN(0, PT_ENUM_CONST, 0)
	TOKEN(0, CT_VARIABLE, 0)

	TOKEN(0, CT_NTOKENS, 0)
#endif