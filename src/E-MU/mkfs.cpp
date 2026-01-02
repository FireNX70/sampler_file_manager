#include <filesystem>
#include <string>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "min_vfs/min_vfs_base.hpp"

namespace EMU::FS
{
	uint16_t mkfs(const std::filesystem::path &fs_path, const std::string &label)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID,
							 (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}
}
