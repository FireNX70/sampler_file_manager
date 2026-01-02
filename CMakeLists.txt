cmake_minimum_required(VERSION 3.20)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

project(sampler_file_manager)

option(ENABLE_TESTS "" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #For clangd

if(ENABLE_TESTS)
	include(CTest)
endif()

include_directories(src)
add_subdirectory(src)
