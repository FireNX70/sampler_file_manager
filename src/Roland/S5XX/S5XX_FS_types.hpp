#ifndef S5XX_FS_TYPES_INCLUDE_GUARD
#define S5XX_FS_TYPES_INCLUDE_GUARD

#include <bit>

#include "Utils/ints.hpp"

namespace S5XX::FS
{
	constexpr char FS_NAME[] = "S5XX";
	constexpr char MAGIC[] = "* ROLAND S-550 *";
	constexpr std::endian ENDIANNESS = std::endian::big;
	constexpr u16 BLOCK_SIZE = 512;

	namespace On_disk_sizes
	{
		constexpr u8 DIR_ENTRY = 32;
		constexpr u8 FILE_ENTRY = 64;
		constexpr u16 AREA_BLKS = 0x6D2; //typical
		constexpr u32 AREA_BYTES = AREA_BLKS * BLOCK_SIZE; //typical
		constexpr u8 INSTR_GRP = 32;
	}

	constexpr u8 MAX_ROOT_DENTRIES = (BLOCK_SIZE / On_disk_sizes::DIR_ENTRY) - 1;
	constexpr u8 FIRST_CD_DIR = 7;
	constexpr u8 FILE_TYPE_OFFSET = 0x18;
	constexpr u8 FILES_PER_SECTOR = BLOCK_SIZE / On_disk_sizes::FILE_ENTRY;

	enum struct File_type_e: u8
	{
		system_directory = 32,
		system_file,
		sound_directory = 64,
		area,
		instrument_group = 72, //CD only, kind of a dir
		map = 74 //CD only, kind of a dir
	};

	struct Dir_entry_t
	{
		char name[17];
		u32 block_addr;
		u32 block_cnt;
		File_type_e type;
		u8 file_cnt; //CD only
	};

	struct File_entry_t
	{
		char name[49];
		u32 block_addr;
		u32 block_cnt;
		File_type_e type;
		u8 ins_grp_idx; //CD only
	};
}

#endif // !S5XX_FS_TYPES_INCLUDE_GUARD
