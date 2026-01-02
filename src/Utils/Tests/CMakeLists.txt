add_executable(
	FAT_utils_test
	FAT_utils_test.cpp
)

file(COPY Data/FAT_test_data.bin DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
	FAT_utils_test
	PUBLIC
		utils
)

add_test(FAT_utils_test FAT_utils_test)