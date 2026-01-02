#ifndef DIR_HISTORY_HEADER_INCLUDE_GUARD
#define DIR_HISTORY_HEADER_INCLUDE_GUARD

#include <memory>
#include <filesystem>

#include "Utils/ints.hpp"

class dir_history_t
{
public:
	dir_history_t(const std::filesystem::path &start_path);

	std::filesystem::path& back();
	std::filesystem::path& fwd();
	void fwd(const std::filesystem::path& path);

private:
	u8 cur_point, his_size, back_off;
	std::unique_ptr<std::filesystem::path[]> history;
};
#endif
