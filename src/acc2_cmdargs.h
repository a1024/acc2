#ifdef CMDARG
	CMDARG(0, CA_NONE),

	CMDARG("--help", CA_HELP),

	CMDARG("--see", CA_SEE),
	CMDARG("--macros", CA_MACROS),
	CMDARG("--include", CA_INCLUDE),
	CMDARG("--lib", CA_LIB),
	CMDARG("--project", CA_PROJECT),
	CMDARG("--in", CA_INPUT),
	CMDARG("--prof", CA_PROFILER),

	CMDARG("--preprocess", CA_PREPROCESS),//mutually exclusive, ordered
	CMDARG("--compile", CA_COMPILE),
	CMDARG("--assemble", CA_ASSEMBLE),
	CMDARG("--link", CA_LINK),
	CMDARG("--out", CA_OUTPUT),
#endif