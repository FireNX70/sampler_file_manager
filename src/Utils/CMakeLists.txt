add_library(
	utils
	ints.hpp
	str_util.hpp
	FAT_utils.hpp
	utils.hpp
	testing_helpers.cpp
)

set_target_properties(utils PROPERTIES LINKER_LANGUAGE CXX)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
