add_library(
	qt_misc
	main_window.cpp
	dir_list_item.cpp
	min_vfs_model.cpp
	min_vfs_selection_model.cpp
	dir_history.cpp
	filename_validator.cpp
	task_msg_proxy.cpp
	mkfs_dialog.cpp
	fs_list_model.cpp
)

target_link_libraries(
	qt_misc

	PRIVATE
		Qt6::Widgets

	PUBLIC
		min_vfs

		#for mkfs
		EMU_FS_drv
		S5XX_FS_drv
		S7XX_FS_drv
)
