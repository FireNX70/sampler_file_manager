set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(
	sampler_file_manager
	main.cpp
)

target_link_libraries(
	sampler_file_manager

	PRIVATE
		Qt6::Widgets

	PUBLIC
		min_vfs
		qt_misc
)

if(WIN32)
	set_target_properties(sampler_file_manager PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_subdirectory(Utils)
add_subdirectory(min_vfs)
add_subdirectory(Host_FS)
add_subdirectory(E-MU)
add_subdirectory(Roland)
add_subdirectory(GUI)

if(ENABLE_TESTS)
	add_subdirectory(Tests)
endif()
