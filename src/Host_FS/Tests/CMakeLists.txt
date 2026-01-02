add_executable(
	host_fs_unit_tests
	fs_drv_unit_tests.cpp
)

target_link_libraries(
	host_fs_unit_tests
	PUBLIC
		utils
		min_vfs_testing_helpers
)

add_test(host_fs_unit_tests host_fs_unit_tests)


add_executable(
	host_fs_tests
	fs_drv_tests.cpp
)

target_link_libraries(
	host_fs_tests
	PUBLIC
		utils
		host_fs_drv
		min_vfs_testing_helpers
)

file(COPY Data/read_test_file_1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(host_fs_tests host_fs_tests)
