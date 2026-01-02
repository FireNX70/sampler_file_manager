add_executable(
	str_util_test
	str_util_test.cpp
)

target_link_libraries(
	str_util_test
	PUBLIC
		utils
)

add_test(str_util_test str_util_test)
