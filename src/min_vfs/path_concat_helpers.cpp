#include <filesystem>

std::filesystem::path concat_path_func(const std::filesystem::path &A,
									   const std::filesystem::path &B)
{
	return A.string() + "/" + B.string();
}

std::filesystem::path& concat_assign_path_func(std::filesystem::path &A,
											   const std::filesystem::path &B)
{
	A = A.string() + "/" + B.string();
	return A;
}