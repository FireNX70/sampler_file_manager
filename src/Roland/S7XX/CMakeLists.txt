add_library(
	S7XX_host_FS_utils
	host_FS_utils.hpp
	host_FS_utils.cpp
)


add_library(
	S7XX_FS_drv
	S7XX_FS_types.hpp
	fs_drv_constants.hpp
	fs_drv_helpers.hpp
	fs_drv_helpers.cpp
	S7XX_FS_drv.hpp
	S7XX_FS_drv.cpp
	
	mkfs.cpp
	fsck.cpp
)

target_link_libraries(
	S7XX_FS_drv
	PUBLIC
		utils
		min_vfs_base
		S7XX_host_FS_utils
)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
