add_library(
	min_vfs_testing_helpers
	helpers.cpp
)

target_link_libraries(
	min_vfs_testing_helpers
	PUBLIC
		min_vfs
)

add_executable(
	min_vfs_tests
	min_vfs_tests.cpp
)

target_link_libraries(
	min_vfs_tests
	PUBLIC
		utils
		min_vfs
		min_vfs_testing_helpers
)

add_test(min_vfs_tests min_vfs_tests)

file(COPY Data/base_test_fs.img FOLLOW_SYMLINK_CHAIN DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/test_S7XX_fs.img FOLLOW_SYMLINK_CHAIN DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
