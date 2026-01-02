add_library(
	host_fs_drv
	host_drv.hpp
	host_drv.cpp
)

target_link_libraries(
	host_fs_drv
	PUBLIC
		min_vfs_base
)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
