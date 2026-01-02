#ifndef EMU_FS_TYPES
#define EMU_FS_TYPES

#include <bit>

#include "Utils/ints.hpp"
#include "Utils/FAT_utils.hpp"

namespace EMU::FS
{
	constexpr char MAGIC[] = "EMU3";
	constexpr char FS_NAME[] = "EMU3";
	constexpr std::endian ENDIANNESS = std::endian::little;
	constexpr u16 BLK_SIZE = 512;
	constexpr u8 MIN_CLUSTER_SHIFT = 15;
	constexpr u8 MAX_CLUSTER_SHIFT = 24;
	constexpr u8 FIRST_NON_RESERVED_BLK = 2;

	namespace On_disk_sizes
	{
		constexpr u8 HEADER = 41;
		constexpr u8 DIR_ENTRY = 32;
		constexpr u8 FILE_ENTRY = 32;
	}

	constexpr u8 DIRS_PER_BLOCK = BLK_SIZE / On_disk_sizes::DIR_ENTRY;
	constexpr u8 FILES_PER_BLOCK = BLK_SIZE / On_disk_sizes::FILE_ENTRY;
	constexpr u8 MAX_BLOCKS_PER_DIR = 7;
	constexpr u8 MAX_FILES_PER_DIR = FILES_PER_BLOCK * MAX_BLOCKS_PER_DIR;
	constexpr u8 MAX_BANK_IDX = 99;

	constexpr FAT_utils::FAT_attrs_t<uint16_t> FAT_ATTRS(ENDIANNESS, 0, 1,
		0x7FFE, 0x7FFF, 0x8000);

	constexpr u16 MAX_CLUSTER_CNT = FAT_ATTRS.DATA_MAX;
	constexpr u16 MAX_FAT_LEN = FAT_ATTRS.DATA_MAX;
	constexpr u16 MAX_FAT_BLOCKS = MAX_FAT_LEN * 2 / BLK_SIZE + ((MAX_FAT_LEN * 2 % BLK_SIZE) != 0);

	struct Header_t
	{
		u32 block_cnt;
		u32 dir_list_blk_addr;
		u32 dir_list_blk_cnt;
		u32 file_list_blk_addr;
		u32 file_list_blk_cnt;
		u32	FAT_blk_addr;
		u32 FAT_blk_cnt;
		u32 data_sctn_blk_addr;
		u16 cluster_cnt;
		u8 cluster_shift;
	};

	enum class File_type_e: u8
	{
		DEL,
		PADDING = 0x42,
		SYS = 0x80,
		STD,
		UPD = 0x83
	};

	struct File_t
	{
		char name[17]; //doesn't actually need to be unique in dir
		u8 bank_num; //must be unique in dir
		u16 start_cluster;
		u16 cluster_cnt;
		u16 block_cnt; //blocks in last cluster
		u16 byte_cnt; //bytes in last block
		File_type_e type;
		u8 props[5];
		uintmax_t addr; //absolute addr within FS
	};

	enum class Dir_type_e: u8
	{
		DEL,
		LAST = 0x40, //last marker?
		NORMAL = 0x80
	};

	struct Dir_t
	{
		char name[17];
		Dir_type_e type;
		u16 blocks[MAX_BLOCKS_PER_DIR]; //for file list
		uintmax_t addr; //absolute addr within FS
	};
}

#endif // !EMU_FS_TYPES
