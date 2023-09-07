# CMakeList.txt : CMake project for acc, include source and define project specific logic here.

add_executable(acc
	"acc.c"
	"parser.c"
	"preprocessor.c"
	"util.c"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET acc PROPERTY CXX_STANDARD 20)
endif()
