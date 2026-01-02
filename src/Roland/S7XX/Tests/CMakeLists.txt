add_library(
	S7XX_FS_test_helpers
	Helpers.cpp
)

add_executable(
	S7XX_host_FS_tests
	host_FS_tests.cpp
)

target_link_libraries(
	S7XX_host_FS_tests
	PUBLIC
		S7XX_host_FS_utils
		S7XX_FS_test_helpers
)

add_test(S7XX_host_FS_tests S7XX_host_FS_tests)


add_executable(
	S7XX_FS_unit_tests
	fs_drv_unit_tests.cpp
)

#We don't wanna link against the driver, we're gonna include it directly
target_link_libraries(
	S7XX_FS_unit_tests
	PUBLIC
		min_vfs_base
		S7XX_host_FS_utils
)

add_test(S7XX_FS_unit_tests S7XX_FS_unit_tests)


add_executable(
	S7XX_FS_tests
	fs_drv_tests.cpp
)

target_link_libraries(
	S7XX_FS_tests
	PUBLIC
		S7XX_FS_drv
		S7XX_FS_test_helpers
)

file(COPY Data/test_fs.img DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/ref_host_fs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(S7XX_FS_tests S7XX_FS_tests)


add_executable(
	S7XX_FS_thread_tests
	fs_drv_thread_tests.cpp
)

target_link_libraries(
	S7XX_FS_thread_tests
	PUBLIC
		utils
		S7XX_FS_drv
		S7XX_FS_test_helpers
)

add_test(S7XX_FS_thread_tests S7XX_FS_thread_tests)
