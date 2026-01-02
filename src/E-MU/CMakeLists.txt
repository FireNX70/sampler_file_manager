add_library(
	EMU_FS_drv
	EMU_FS_types.hpp
	EMU_FS_drv.hpp
	EMU_FS_drv.cpp
	fs_common.hpp
	fs_common.cpp
	mkfs.cpp
	fsck.cpp
)

target_link_libraries(
	EMU_FS_drv
	PUBLIC
		min_vfs_base
)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
