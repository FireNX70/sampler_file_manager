add_library(
	min_vfs_base
	min_vfs_base.hpp
	min_vfs_base.cpp
	filesystem.cpp
	stream.cpp
	dentry.cpp
)

add_library(
	min_vfs
	min_vfs.hpp
	min_vfs.cpp
)

if(WIN32)
	target_sources(min_vfs PRIVATE path_concat_helpers.cpp)
endif()

target_link_libraries(
	min_vfs
	PUBLIC
		min_vfs_base
		host_fs_drv
		EMU_FS_drv
		S7XX_FS_drv
		S5XX_FS_drv
)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
