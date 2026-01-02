#ifndef S7XX_FS_UTILS_HEADER_GUARD
#define S7XX_FS_UTILS_HEADER_GUARD

#include <cstdint>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "min_vfs/min_vfs_base.hpp"
#include "library_IDs.hpp"
#include "S7XX_FS_types.hpp"

namespace S7XX::FS
{
	constexpr uint8_t LIBRARY_ID = (uint8_t)Library_IDs::S7XX_FS;
	
	namespace FSCK_ERR
	{
		constexpr u8 TOC_INCONSISTENCY = 1 << 0;
		constexpr u8 TOO_LARGE = 1 << 1;
		constexpr u8 BAD_CLS0_VAL = 1 << 2;
		constexpr u8 UNMARKED_RESVD_CLS = 1 << 3;
		constexpr u8 FREE_CLS_CNT_MISMATCH = 1 << 4;
		constexpr u8 BAD_FENTRY = 1 << 5;
		constexpr u8 INACCESSIBLE_FENTRIES = 1 << 6;
	}

	enum struct ERR: uint8_t
	{
		NOT_A_FILE = 1,
		MACHINE_NAME_MISMATCH,
		MEDIA_TYPE_NOT_HDD,
		UNKNOWN_TEXT_MACHINE_TYPE,
		MEDIA_TYPE_MISMATCH,
		UNKNOWN_ELEMENT_TYPE,
		EMPTY_ENTRY,
		ELEMENT_TYPE_MISMATCH,
		BAD_COUNT,
		MISSING_ENTRIES,
		INVALID_PATH,
		WTF,
		FAILED_TO_CREATE_DST_FS,
		CHAIN_SIZE_MISMATCH,
		TOO_SMALL,
		NO_FREE_SLOTS,
		NOT_FOUND,
		OS_SIZE_MISMATCH,
		IO_ERROR,
		NOT_S7XX_FS,
		INVALID_FILE,
		IDX_OUT_OF_RANGE
	};

	uint16_t mkfs(const std::filesystem::path &fs_path, const std::string &label);
	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status);

	struct internal_file_t;

	struct filesystem_t: min_vfs::filesystem_t
	{
		Header_t header;
		FAT_utils::FAT_dyna_attrs_t<uint16_t> fat_attrs;

		std::unique_ptr<u16[]> FAT;

		/*TODO: Use u16s as keys. Lower 13 bits for the actual file idx, upper
		3 for the type_idx.*/
		typedef std::unordered_map<std::string, std::pair<uintmax_t, internal_file_t>> file_map_t;
		file_map_t open_files;
		using file_map_iterator_t = file_map_t::iterator;

		filesystem_t() = default;
		filesystem_t(const char *path);

		min_vfs::filesystem_t& operator=(min_vfs::filesystem_t &&other) noexcept override;
		filesystem_t& operator=(filesystem_t &&other) noexcept;

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
		u8 type_idx;
		List_entry_t list_entry;
	};
}
#endif
