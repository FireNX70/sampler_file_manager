#include <filesystem>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S5XX/S5XX_FS_types.hpp"

namespace S5XX::FS
{
	/*TODO. This is just a minimal implementation to avoid returning
	 *UNSUPPORTED_OPERATION on non-S5XX filesystems.*/
	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status)
	{
		constexpr u8 MAGIC_SIZE = 16;

		char magic[MAGIC_SIZE];
		std::fstream fs_fstr;

		if(!std::filesystem::exists(fs_path)
			|| !std::filesystem::is_regular_file(fs_path))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);

		if(std::filesystem::file_size(fs_path) < BLOCK_SIZE)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		fs_fstr.open(fs_path, std::fstream::in | std::fstream::out
			| std::fstream::binary);

		if(!fs_fstr.is_open() || !fs_fstr.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		fs_fstr.seekg(0);
		fs_fstr.read(magic, MAGIC_SIZE);

		if(std::memcmp(magic, MAGIC, MAGIC_SIZE))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		return ret_val_setup(min_vfs::LIBRARY_ID,
							 (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}
}
