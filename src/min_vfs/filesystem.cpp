#include <filesystem>
#include <fstream>

#include "Utils/utils.hpp"
#include "min_vfs_base.hpp"

namespace min_vfs
{
	filesystem_t::filesystem_t(const char *path)
	{
		this->path = std::filesystem::is_symlink(path) ?
			std::filesystem::read_symlink(path) : path;

		if(!std::filesystem::exists(this->path))
			throw min_vfs::FS_err(ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK));

		//in the future, I'd like to get it working for device files too
		if(!std::filesystem::is_regular_file(this->path))
			throw min_vfs::FS_err(ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_A_FILE));

		/*The image/partition/device size check will be done by the individual
		drivers because it depends on the FS*/

		stream.open(this->path, std::fstream::binary | std::fstream::in
			| std::fstream::out);

		if(!stream.is_open() || !stream.good())
			throw min_vfs::FS_err(ret_val_setup(LIBRARY_ID, (u8)min_vfs::ERR::CANT_OPEN_DISK));
	}

	uint16_t filesystem_t::list(const char *path, std::vector<dentry_t> &dentries)
	{
		return list(path, dentries, false);
	}

	uint16_t filesystem_t::fopen(const char *path, stream_t &stream)
	{
		void *internal_file;

		if(stream.fs) stream.close();

		const u16 err = fopen_internal(path, &internal_file);
		if(err) return err;

		stream.fs = this;
		stream.internal_file = internal_file;
		stream.pos = 0;

		return 0;
	}
}
