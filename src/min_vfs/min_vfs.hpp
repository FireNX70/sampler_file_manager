#ifndef MIN_VFS_HEADER_INCLUDE_GUARD
#define MIN_VFS_HEADER_INCLUDE_GUARD

#include <cstdint>
#include <vector>
#include <string>

#include "min_vfs_base.hpp"

namespace min_vfs
{
	struct mount_stats_t
	{
		std::filesystem::path path;
		std::string type;
		uintmax_t open_file_count;
	};

	struct map_stats_t
	{
		std::filesystem::path key;
		mount_stats_t mount;
	};

	void lsmount(std::vector<mount_stats_t> &mounts);
	void lsmap(std::vector<map_stats_t> &map_stats);
	uint16_t fsck(std::filesystem::path path);
	uint16_t mount(std::filesystem::path path);
	uint16_t umount(std::filesystem::path path);

	uint16_t list(std::filesystem::path path, std::vector<dentry_t> &dentries);
	uint16_t list(std::filesystem::path path, std::vector<dentry_t> &dentries,
				  const bool get_dir);
	uint16_t mkdir(std::filesystem::path dir_path);
	uint16_t ftruncate(std::filesystem::path path, const uintmax_t new_size);
	uint16_t copy(std::filesystem::path cur_path,
				  std::filesystem::path new_path);
	uint16_t rename(std::filesystem::path cur_path,
					std::filesystem::path new_path);
	uint16_t remove(std::filesystem::path path);

	uint16_t fopen(std::filesystem::path path, stream_t &stream); //may throw
	//fclose is handled by the stream
}
#endif
