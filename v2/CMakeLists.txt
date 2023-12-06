# CMakeList.txt : CMake project for acc2, include source and define project specific logic here.

# Add source to this project's executable.
add_executable(acc2
	acc2.c
	codegen.c
	parser.c
	translate.c
	util.c
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET acc2 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
