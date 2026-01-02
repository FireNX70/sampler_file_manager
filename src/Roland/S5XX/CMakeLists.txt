add_library(
	S5XX_FS_drv
	S5XX_FS_types.hpp
	S5XX_FS_drv.hpp
	S5XX_FS_drv.cpp
	fsck.cpp
)

target_link_libraries(
	S5XX_FS_drv
	PUBLIC
		utils
		min_vfs_base
)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
