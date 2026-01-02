#ifndef S5XX_FS_DRV_INCLUDE_GUARD
#define S5XX_FS_DRV_INCLUDE_GUARD

#include <unordered_map>
#include <string>
#include <utility>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"

namespace S5XX::FS
{
	enum struct ERR: u8
	{
	};

	uint16_t mkfs(const std::filesystem::path &fs_path, const std::string &label);
	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status);

	struct internal_file_t;

	struct filesystem_t: min_vfs::filesystem_t
	{
		u8 FIRST_DIR_IDX; //CD if not 0
		//might wanna cache dir entries, since max count is very low
		typedef std::unordered_map<std::string, std::pair<uintmax_t, internal_file_t>> file_map_t;
		file_map_t open_files;
		using file_map_iterator_t = file_map_t::iterator;

		filesystem_t() = default;
		filesystem_t(const char *path);

		min_vfs::filesystem_t &operator=(min_vfs::filesystem_t &&other) noexcept override;
		filesystem_t &operator=(filesystem_t &&other) noexcept;

		std::string get_type_name();
		uintmax_t get_open_file_count();
		bool can_unmount();

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
		File_entry_t file_entry;
	};
}

#endif // !S5XX_FS_DRV_INCLUDE_GUARD
