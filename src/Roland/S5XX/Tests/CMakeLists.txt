add_library(
	S5XX_FS_test_helpers
	helpers.hpp
	helpers.cpp
)


add_executable(
	S5XX_FS_unit_tests
	fs_drv_unit_tests.cpp
)

target_link_libraries(
	S5XX_FS_unit_tests
	PUBLIC
		utils
		min_vfs_base
)

add_test(S5XX_FS_unit_tests S5XX_FS_unit_tests)

file(COPY Data/base_test_fs.iso DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/S550_CD_SB.img DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/S550_HDD_SB.img DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Data/test_file.s5xx DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


add_executable(
	S5XX_FS_tests
	fs_drv_tests.cpp
)

target_link_libraries(
	S5XX_FS_tests
	PUBLIC
		utils
		S5XX_FS_drv
		S5XX_FS_test_helpers
)

add_test(S5XX_FS_tests S5XX_FS_tests)


add_executable(
	S5XX_FS_thread_tests
	fs_drv_thread_tests.cpp
)

target_link_libraries(
	S5XX_FS_thread_tests
	PUBLIC
		utils
		S5XX_FS_drv
		S5XX_FS_test_helpers
)

add_test(S5XX_FS_thread_tests S5XX_FS_thread_tests)
