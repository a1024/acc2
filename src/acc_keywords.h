#ifdef TOKEN

	TOKEN(0, T_IGNORED)

	TOKEN(0, T_VAL_C8)	//stores 8bit characters (up to 8) (MSVC: 4)
	TOKEN(0, T_VAL_C32)	//stores 32bit characters (up to 2)
	TOKEN(0, T_VAL_I32)	//int
	TOKEN(0, T_VAL_U32)	//unsigned
	TOKEN(0, T_VAL_I64)	//long long
	TOKEN(0, T_VAL_U64)	//unsigned long long
	TOKEN(0, T_VAL_F32)	//float
	TOKEN(0, T_VAL_F64)	//double
	TOKEN(0, T_ID)
	TOKEN(0, T_VAL_STR)
	TOKEN(0, T_VAL_WSTR)//string of 32-bit unicode codepoints
	TOKEN(0, T_INCLUDENAME_STD)

	//type keywords
	TOKEN("typedef", T_TYPEDEF)
	TOKEN("enum", T_ENUM)
	TOKEN("struct", T_STRUCT) TOKEN("union", T_UNION)
#ifdef ACC_CPP
	TOKEN("class", T_CLASS)
	TOKEN("template", T_TEMPLATE) TOKEN("typename", T_TYPENAME)
#endif
	
#ifdef ACC_CPP
	TOKEN("constexpr", T_CONSTEXPR) TOKEN("inline", T_INLINE)
#endif
	TOKEN("extern", T_EXTERN) TOKEN("static", T_STATIC)
#ifdef ACC_CPP
	TOKEN("mutable", T_MUTABLE)
#endif
	TOKEN("const", T_CONST) TOKEN("volatile", T_VOLATILE)
	TOKEN("register", T_REGISTER)
#ifdef ACC_CPP
	TOKEN("auto", T_AUTO) TOKEN("decltype", T_DECLTYPE)
#endif
	TOKEN("signed", T_SIGNED) TOKEN("unsigned", T_UNSIGNED)
	TOKEN("void", T_VOID)
#ifdef ACC_CPP
	TOKEN("bool", T_BOOL)
#endif
	TOKEN("char", T_CHAR) TOKEN("short", T_SHORT)
	TOKEN("int", T_INT) TOKEN("long", T_LONG)
	TOKEN("float", T_FLOAT) TOKEN("double", T_DOUBLE)
#ifdef ACC_CPP
	TOKEN("wchar_t", T_WCHAR_T)
#endif
	TOKEN("__int8", T_INT8) TOKEN("__int16", T_INT16) TOKEN("__int32", T_INT32) TOKEN("__int64", T_INT64)
#ifdef ACC_X86
	TOKEN("__m128", T_M128) TOKEN("__m128i", T_M128I) TOKEN("__m128d", T_M128D)//x86 SIMD
#endif

	//control keywords
	TOKEN("if", T_IF) TOKEN("else", T_ELSE)
	TOKEN("for", T_FOR)
	TOKEN("do", T_DO) TOKEN("while", T_WHILE)
	TOKEN("switch", T_SWITCH) TOKEN("case", T_CASE) TOKEN("default", T_DEFAULT)
	TOKEN("break", T_BREAK) TOKEN("continue", T_CONTINUE)
	TOKEN("goto", T_GOTO)
	TOKEN("return", T_RETURN)
#ifdef ACC_CPP
	TOKEN("try", T_TRY) TOKEN("catch", T_CATCH) TOKEN("throw", T_THROW)
#endif

	//constants
#ifdef ACC_CPP
	TOKEN("nullptr", T_NULLPTR)
	TOKEN("false", T_FALSE) TOKEN("true", T_TRUE)
#endif

	//special
#ifdef ACC_CPP
	TOKEN("new", T_NEW) TOKEN("delete", T_DELETE) TOKEN("this", T_THIS)
	TOKEN("public", T_PUBLIC) TOKEN("private", T_PRIVATE) TOKEN("protected", T_PROTECTED)
	TOKEN("friend", T_FRIEND) TOKEN("virtual", T_VIRTUAL)
	TOKEN("namespace", T_NAMESPACE) TOKEN("using", T_USING)
	TOKEN("static_assert", T_STATIC_ASSERT)
	TOKEN("operator", T_OPERATOR)
	TOKEN("typeid", T_TYPEID)
#endif
	TOKEN("asm", T_ASM_ATT) TOKEN("__asm", T_ASM_INTEL)

	//non-overloadable operators:	.* . :: sizeof ?: typeid

	//symbols & operators
	TOKEN("sizeof", T_SIZEOF)
#ifdef ACC_CPP
	TOKEN("::", T_SCOPE)
#endif
	TOKEN("{", T_LBRACE) TOKEN("}", T_RBRACE)//not an operator
	TOKEN("(", T_LPR) TOKEN(")", T_RPR)
	TOKEN("[", T_LBRACKET) TOKEN("]", T_RBRACKET)
	TOKEN(".", T_PERIOD) TOKEN("->", T_ARROW)
	TOKEN("++", T_INCREMENT) TOKEN("--", T_DECREMENT)
	TOKEN("!", T_EXCLAMATION) TOKEN("~", T_TILDE)
#ifdef ACC_CPP
	TOKEN(".*", T_DOT_STAR) TOKEN("->*", T_ARROW_STAR)
#endif
	TOKEN("*", T_ASTERIX) TOKEN("/", T_SLASH) TOKEN("%", T_MODULO)
	TOKEN("+", T_PLUS) TOKEN("-", T_MINUS)
	TOKEN("<<", T_SHIFT_LEFT) TOKEN(">>", T_SHIFT_RIGHT)
#ifdef ACC_CPP
	TOKEN("<=>", T_THREEWAY)//since C++20
#endif
	TOKEN("<", T_LESS) TOKEN("<=", T_LESS_EQUAL) TOKEN(">", T_GREATER) TOKEN(">=", T_GREATER_EQUAL)
	TOKEN("==", T_EQUAL) TOKEN("!=", T_NOT_EQUAL)
	TOKEN("&", T_AMPERSAND) TOKEN("^", T_CARET) TOKEN("|", T_VBAR)
	TOKEN("&&", T_LOGIC_AND) TOKEN("||", T_LOGIC_OR)
	TOKEN("?", T_QUESTION) TOKEN(":", T_COLON)

	TOKEN("=", T_ASSIGN)
	TOKEN("*=", T_ASSIGN_MUL) TOKEN("/=", T_ASSIGN_DIV) TOKEN("%=", T_ASSIGN_MOD)
	TOKEN("+=", T_ASSIGN_ADD) TOKEN("-=", T_ASSIGN_SUB)
	TOKEN("<<=", T_ASSIGN_SL) TOKEN(">>=", T_ASSIGN_SR)
	TOKEN("&=", T_ASSIGN_AND) TOKEN("^=", T_ASSIGN_XOR) TOKEN("|=", T_ASSIGN_OR)
	
	TOKEN(",", T_COMMA) TOKEN(";", T_SEMICOLON)//comma is overloadable, but semicolon isn't an operator
	TOKEN("...", T_ELLIPSIS)

	//call convensions
#ifdef ACC_WIN32
	TOKEN("__cdecl", T_CDECL)//default
	TOKEN("__stdcall", T_STDCALL)//WinAPI
#ifdef ACC_CPP
	TOKEN("__thiscall", T_THISCALL)//class member functions
#endif
#endif

	//preprocessor
	TOKEN("#", T_HASH)
	TOKEN("include", T_INCLUDE)
	TOKEN("define", T_DEFINE) TOKEN("undef", T_UNDEF) TOKEN("##", T_CONCATENATE) TOKEN("__VA_ARGS__", T_VA_ARGS)
	TOKEN("ifdef", T_IFDEF) TOKEN("ifndef", T_IFNDEF) TOKEN("elif", T_ELIF) TOKEN("defined", T_DEFINED) TOKEN("endif", T_ENDIF)
	TOKEN("error", T_ERROR)
	TOKEN("pragma", T_PRAGMA)
	TOKEN("line", T_LINE)//sets line counter

	//predefined macros
	TOKEN("__FILE__", T_MACRO_FILE) TOKEN("__LINE__", T_MACRO_LINE)
	TOKEN("__DATE__", T_MACRO_DATE) TOKEN("__TIME__", T_MACRO_TIME) TOKEN("__TIMESTAMP__", T_TIMESTAMP)
#ifdef ACC_CPP
	TOKEN("__func__", T_FUNC)
#endif

	//lexer
	TOKEN("\'", T_QUOTE) TOKEN("\"", T_DOUBLEQUOTE)
	TOKEN("L\'", T_QUOTE_WIDE) TOKEN("L\"", T_DQUOTE_WIDE)
#ifdef ACC_CPP
	TOKEN("R\"", T_DQUOTE_RAW)
#endif
	TOKEN("\n", T_NEWLINE)
	TOKEN("\r", T_NEWLINE_ESC)
	TOKEN("//", T_LINECOMMENTMARK) TOKEN("/*", T_BLOCKCOMMENTMARK)

	//preprocessor
	TOKEN(0, T_LEXME)
	TOKEN(0, T_MACRO_ARG)

	TOKEN(0, T_NTOKENS)

#endif
