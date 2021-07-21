#ifdef TOKEN
	TOKEN(0, CT_IGNORED),//negative values are ignored too

	TOKEN(0, CT_VAL_INTEGER),//idata		//TODO: differentiate between different integer types
	TOKEN(0, CT_VAL_FLOAT),
//	TOKEN(0, CT_VAL_DOUBLE),//TODO: differentiate between float & double
	TOKEN(0, CT_ID),//identifier
	TOKEN(0, CT_VAL_STRING_LITERAL),
	TOKEN(0, CT_VAL_WSTRING_LITERAL),//identical utf-8 like CT_VAL_STRING_LITERAL, just a type for compiler
	TOKEN(0, CT_VAL_CHAR_LITERAL),//TODO: ival, stores up to 8 characters (MSVC: 4)
	TOKEN(0, CT_INCLUDENAME_STD),
	TOKEN(0, CT_INCLUDENAME_CUSTOM),//deprecated

	//type keywords
	TOKEN("typedef", CT_TYPEDEF), TOKEN("enum", CT_ENUM),
	TOKEN("struct", CT_STRUCT), TOKEN("class", CT_CLASS), TOKEN("union", CT_UNION),
	TOKEN("template", CT_TEMPLATE), TOKEN("typename", CT_TYPENAME),
	
	TOKEN("constexpr", CT_CONSTEXPR), TOKEN("inline", CT_INLINE), TOKEN("friend", CT_FRIEND),
	TOKEN("extern", CT_EXTERN), TOKEN("static", CT_STATIC),
	TOKEN("register", CT_REGISTER), TOKEN("volatile", CT_VOLATILE),
	TOKEN("const", CT_CONST),
	TOKEN("auto", CT_AUTO), TOKEN("decltype", CT_DECLTYPE),
	TOKEN("signed", CT_SIGNED), TOKEN("unsigned", CT_UNSIGNED),
	TOKEN("void", CT_VOID),
	TOKEN("bool", CT_BOOL),
	TOKEN("char", CT_CHAR), TOKEN("short", CT_SHORT),
	TOKEN("int", CT_INT), TOKEN("long", CT_LONG),
	TOKEN("float", CT_FLOAT), TOKEN("double", CT_DOUBLE),
	TOKEN("wchar_t", CT_WCHAR_T),
	TOKEN("__int8", CT_INT8), TOKEN("__int16", CT_INT16), TOKEN("__int32", CT_INT32), TOKEN("__int64", CT_INT64),
	TOKEN("__m128", CT_M128), TOKEN("__m128i", CT_M128I), TOKEN("__m128d", CT_M128D),

	//constants
	TOKEN("nullptr", CT_NULLPTR),
	TOKEN("false", CT_FALSE), TOKEN("true", CT_TRUE),

	//special
	TOKEN("new", CT_NEW), TOKEN("delete", CT_DELETE), TOKEN("this", CT_THIS),
	TOKEN("public", CT_PUBLIC), TOKEN("private", CT_PRIVATE), TOKEN("protected", CT_PROTECTED),
	TOKEN("namespace", CT_NAMESPACE),
	TOKEN("static_assert", CT_STATIC_ASSERT),
	TOKEN("operator", CT_OPERATOR),
	TOKEN("asm", CT_ASM_ATT), TOKEN("__asm", CT_ASM_INTEL),
	
	//control keywords
	TOKEN("if", CT_IF), TOKEN("else", CT_ELSE),
	TOKEN("for", CT_FOR),
	TOKEN("do", CT_DO), TOKEN("while", CT_WHILE),
	TOKEN("switch", CT_SWITCH), TOKEN("case", CT_CASE), TOKEN("default", CT_DEFAULT),
	TOKEN("break", CT_BREAK), TOKEN("continue", CT_CONTINUE),
	TOKEN("goto", CT_GOTO),
	TOKEN("return", CT_RETURN),
	TOKEN("try", CT_TRY), TOKEN("catch", CT_CATCH), TOKEN("throw", CT_THROW),

	//symbols & operators
	TOKEN("sizeof", CT_SIZEOF),
	TOKEN("{", CT_LBRACE), TOKEN("}", CT_RBRACE),
	TOKEN("(", CT_LPR), TOKEN(")", CT_RPR),
	TOKEN("[", CT_LBRACKET), TOKEN("]", CT_RBRACKET),
	TOKEN(".", CT_PERIOD), TOKEN("->", CT_ARROW),
	TOKEN("++", CT_INCREMENT), TOKEN("--", CT_DECREMENT),
	TOKEN("!", CT_EXCLAMATION), TOKEN("~", CT_TILDE),
	TOKEN("+", CT_PLUS), TOKEN("-", CT_MINUS), TOKEN("*", CT_ASTERIX), TOKEN("/", CT_SLASH), TOKEN("%", CT_MODULO),
	TOKEN("<<", CT_SHIFT_LEFT), TOKEN(">>", CT_SHIFT_RIGHT),
	TOKEN("<", CT_LESS), TOKEN("<=", CT_LESS_EQUAL), TOKEN(">", CT_GREATER), TOKEN(">=", CT_GREATER_EQUAL),
	TOKEN("==", CT_EQUAL), TOKEN("!=", CT_NOT_EQUAL),
	TOKEN("&", CT_AMPERSAND), TOKEN("^", CT_CARET), TOKEN("|", CT_VBAR),
	TOKEN("&&", CT_LOGIC_AND), TOKEN("||", CT_LOGIC_OR),
	TOKEN("?", CT_QUESTION), TOKEN(":", CT_COLON),

	TOKEN("=", CT_ASSIGN), TOKEN("+=", CT_ASSIGN_ADD), TOKEN("-=", CT_ASSIGN_SUB),
	TOKEN("*=", CT_ASSIGN_MUL), TOKEN("/=", CT_ASSIGN_DIV), TOKEN("%=", CT_ASSIGN_MOD),
	TOKEN("^=", CT_ASSIGN_XOR), TOKEN("|=", CT_ASSIGN_OR), TOKEN("&=", CT_ASSIGN_AND), TOKEN("<<=", CT_ASSIGN_SL), TOKEN(">>=", CT_ASSIGN_SR),
	
	TOKEN(",", CT_COMMA), TOKEN(";", CT_SEMICOLON),
	TOKEN("...", CT_ELLIPSIS),

	//preprocessor
	TOKEN("#", CT_HASH),
	TOKEN("include", CT_INCLUDE),
	TOKEN("define", CT_DEFINE), TOKEN("undef", CT_UNDEF), TOKEN("##", CT_CONCATENATE), TOKEN("__VA_ARGS__", CT_VA_ARGS),
	TOKEN("ifdef", CT_IFDEF), TOKEN("ifndef", CT_IFNDEF), TOKEN("elif", CT_ELIF), TOKEN("defined", CT_DEFINED), TOKEN("endif", CT_ENDIF),
	TOKEN("error", CT_ERROR),
	TOKEN("pragma", CT_PRAGMA),
	//TOKEN("line", CT_LINE),

	//predefined macros
	TOKEN("__FILE__", CT_FILE), TOKEN("__LINE__", CT_LINE), TOKEN("__DATE__", CT_DATE), TOKEN("__TIME__", CT_TIME), TOKEN("__TIMESTAMP__", CT_TIMESTAMP), TOKEN("__func__", CT_FUNC),
	TOKEN("__ACC2__", CT_ACC2),

	//lexer
	TOKEN("\'", CT_QUOTE), TOKEN("\"", CT_DOUBLEQUOTE), TOKEN("L\"", CT_DQUOTE_WIDE), TOKEN("R\"", CT_DQUOTE_RAW),
	TOKEN("\n", CT_NEWLINE),
	TOKEN("//", CT_LINECOMMENTMARK), TOKEN("/*", CT_BLOCKCOMMENTMARK),

	//preprocessor flags
	TOKEN(0, CT_MACRO_ARG),
	TOKEN(0, CT_LEXME),//see sdata, result of token pasting

	TOKEN(0, CT_NTOKENS),
#endif
