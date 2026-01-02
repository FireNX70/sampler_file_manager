add_library(
	EMU_FS_testing_helpers
	Helpers.hpp
	Helpers.cpp
)

add_executable(
	EMU_FS_unit_tests
	fs_drv_unit_tests.cpp
)

target_link_libraries(
	EMU_FS_unit_tests
	PUBLIC
		utils
		min_vfs_base
		EMU_FS_testing_helpers
)

add_test(EMU_FS_unit_tests EMU_FS_unit_tests)


add_executable(
	EMU_FS_tests
	fs_test_data.hpp
	EMU_FS_tests.cpp
)

target_link_libraries(
	EMU_FS_tests
	PUBLIC
		utils
		EMU_FS_drv
		EMU_FS_testing_helpers
)

add_test(EMU_FS_tests EMU_FS_tests)

file(COPY Data/base_test_fs.img DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/test_file DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


add_executable(
	EMU_FS_thread_tests
	EMU_FS_thread_tests.cpp
)

target_link_libraries(
	EMU_FS_thread_tests
	PUBLIC
		utils
		EMU_FS_drv
		EMU_FS_testing_helpers
)

add_test(EMU_FS_thread_tests EMU_FS_thread_tests)
