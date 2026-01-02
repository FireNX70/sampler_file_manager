#ifndef EMU_FS_UTILS
#define EMU_FS_UTILS

#include <unordered_map>

#include "library_IDs.hpp"
#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "EMU_FS_types.hpp"

namespace EMU::FS
{
	constexpr u8 LIBRARY_ID = (u8)Library_IDs::EMU_FS;

	enum class ERR: u8
	{
		BAD_CLUSTER_CNT = 1,
		BAD_FAT_BLK_CNT,
		BAD_FILE_LIST_ADDR_OR_CNT,
		TRY_GROW_DIR,
		DIR_SIZE_MAXED,
		FAILED_TO_UPDATE_OPEN_FILE_MAP,
		FOUND_IN_MAP
	};

	enum class FSCK_STATUS: u16
	{
		INVALID_CHECKSUM = 1 << 0,
		BAD_CLUSTER_SHIFT = 1 << 1,
		BAD_BLOCK_CNT = 1 << 2,
		BAD_CLUSTER_CNT = 1 << 3,
		BAD_ROOT_DIR = 1 << 4,
		BAD_FILE_LIST = 1 << 5,
		BAD_FAT_ADDR = 1 << 6,
		BAD_FAT_BLK_CNT = 1 << 7,
		FILE_LIST_COLLISION = 1 << 8,
		FAT_COLLISION = 1 << 9,
		DATA_COLLISION = 1 << 10,
		BAD_DIR = 1 << 11,
		BAD_NEXT_DIR_CONTENT_BLK = 1 << 12,
		UNMARKED_RESERVED_CLUSTERS = 1 << 13,
		BAD_FILE = 1 << 14
	};

	uint16_t mkfs(const std::filesystem::path &fs_path,
				  const std::string &label);
	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status);

	struct internal_file_t;

	struct filesystem_t: min_vfs::filesystem_t
	{
		Header_t header;
		u16 next_file_list_blk, free_clusters;
		std::vector<bool> dir_content_block_map;
		FAT_utils::FAT_dyna_attrs_t<u16> FAT_attrs;
		std::unique_ptr<u16[]> FAT;

		typedef std::unordered_map<std::string, std::pair<uintmax_t, internal_file_t>> file_map_t;
		file_map_t open_files;
		using file_map_iterator_t = file_map_t::iterator;

		//yes, this does not respect RAII
		filesystem_t() = default;
		filesystem_t(const char *path);

		min_vfs::filesystem_t& operator=(min_vfs::filesystem_t &&other) noexcept override;
		filesystem_t& operator=(filesystem_t &&other) noexcept;

		std::string get_type_name();
		uintmax_t get_open_file_count();
		bool can_unmount();

		uintmax_t get_free_space();

		using min_vfs::filesystem_t::list;
		uint16_t list(const char *path,
					  std::vector<min_vfs::dentry_t> &dentries,
				const bool get_dir);
		uint16_t mkdir(const char *dir_path);
		uint16_t ftruncate(const char *path, const uintmax_t new_size);
		uint16_t rename(const char *cur_path, const char *new_path);
		uint16_t remove(const char *path);

		uint16_t fclose(void *internal_file);
		uint16_t read(void *internal_file, uintmax_t &pos, uintmax_t len, void *dst);
		uint16_t write(void *internal_file, uintmax_t &pos, uintmax_t len, void *src);
		uint16_t flush(void *internal_file);

	private:
		uint16_t fopen_internal(const char *path, void **internal_file);
	};

	struct internal_file_t
	{
		filesystem_t::file_map_t::value_type *map_entry;
		filesystem_t::file_map_t::value_type *dir_map_entry;
		min_vfs::ftype_t ftype;
		File_t file_entry;
	};
}

#endif // !EMU_FS
