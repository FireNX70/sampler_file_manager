#include "min_vfs_base.hpp"

namespace min_vfs
{
	std::string ftype_to_string(const min_vfs::ftype_t &ftype)
	{
		switch(ftype)
		{
			using enum min_vfs::ftype_t;

			case file: return "file";
			case dir: return "dir";
			case symlink: return "symlink";
			case blk_dev: return "block device";
			case char_dev: return "character device";
			case pipe: return "pipe";
			case socket: return "socket";
			case unknown:
			default:
				return "unknown";
		}
	}
}
