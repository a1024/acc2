2021-07-21
ACC2 - A C++ compiler (work in progress)

Usage:
--help
	Print usage.

--see     <file>
	Specify a file with commandline arguments (one command per line).

--macros
	Specify pre-defined macros for all sources.

--include <folders>
	Add include paths.

--lib     <folders>    TODO
	Add library paths.

--project <folder>
	(Optional) Specify the folder where input files are located.
	If not specified, the current working directory is used.

--in      <files>
	Add input files (paths relative to the folder specified in --project command).

--prof
	Use performance profiler.


Choose at most one of the following commands:
--out     <file>     (Optional) Specify the executable name.
--preprocess         The specified sources are to be just preprocessed.

TODO:
--compile            The specified sources are to be just compiled (assembly output).
--assemble           An object file is generated for each specified input file.
--link               The input files are object files to be linked.


Development started on 2021-07-09.













