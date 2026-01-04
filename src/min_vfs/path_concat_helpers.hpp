#ifndef MIN_VFS_PATH_CONCAT_HELPERS_HEADER_INCLUDE_GUARD
#define MIN_VFS_PATH_CONCAT_HELPERS_HEADER_INCLUDE_GUARD

#if _WIN32
	#include <filesystem>

	std::filesystem::path concat_path_func(const std::filesystem::path &A,
										   const std::filesystem::path &B);

	#define CONCAT_PATHS(A, B) concat_path_func(A, B)
#else
	#define CONCAT_PATHS(A, B) A / B
#endif

#if _WIN32
	std::filesystem::path& concat_assign_path_func(std::filesystem::path &A,
												const std::filesystem::path &B);

	#define CONCAT_ASSIGN_PATH(A, B) concat_assign_path_func(A, B)
#else
	#define CONCAT_ASSIGN_PATH(A, B) A /= B
#endif

#endif
