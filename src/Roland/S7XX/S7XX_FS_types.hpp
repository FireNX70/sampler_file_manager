#ifndef S7XX_FS_HEADER
#define S7XX_FS_HEADER

#include <bit>
#include <cstdint>

#include "S7XX_types.hpp"
#include "Utils/FAT_utils.hpp"

namespace S7XX::FS
{
	constexpr char FS_NAME[] = "S7XX";
	constexpr std::endian ENDIANNESS = std::endian::little;
	
	constexpr uint8_t MAX_VOLUME_COUNT = 128;
	constexpr uint16_t MAX_PERF_COUNT = 512;
	constexpr uint16_t MAX_PATCH_COUNT = 1024;
	constexpr uint16_t MAX_PARTIAL_COUNT = 4096;
	constexpr uint16_t MAX_SAMPLE_COUNT = 8192;
	constexpr uint32_t MAX_SAMPLE_SIZE = 0xFFFFFF;
	
	constexpr uint16_t BLK_SIZE = 512;
	constexpr uint8_t S760_OS_CLUSTERS = 114;
	
	constexpr char MACHINE_NAME[] = "S770 MR25A";
	constexpr char TEXT_SYS772[] = "SYS-772";
	constexpr char TEXT_S760[] = "S-760";

	namespace On_disk_addrs
	{
		constexpr uint16_t TOC = 0x100;
		constexpr uint16_t OS = 0x800;
		constexpr uint32_t FAT = 0x80800;
		constexpr uint32_t VOLUME_LIST = 0xA0800;
		constexpr uint32_t PERF_LIST = 0xA1800;
		constexpr uint32_t PATCH_LIST = 0xA5800;
		constexpr uint32_t PARTIAL_LIST = 0xAD800;
		constexpr uint32_t SAMPLE_LIST = 0xCD800;
		constexpr uint32_t VOLUME_PARAMS = 0x10D800;
		constexpr uint32_t PERF_PARAMS = 0x115800;
		constexpr uint32_t PATCH_PARAMS = 0x155800;
		constexpr uint32_t PARTIAL_PARAMS = 0x1D5800;
		constexpr uint32_t SAMPLE_PARAMS = 0x255800;
		constexpr uint32_t AUDIO_SECTION = 0x2B5800;
	}

	namespace On_disk_sizes
	{
		constexpr uint16_t HEADER = 96;
		constexpr uint8_t TOC = 30;
		constexpr uint32_t OS = 0x80000;
		constexpr uint32_t FAT = 0x10000 * 2; //the whole thing's always there
		constexpr uint8_t LIST_ENTRY = 32;
		constexpr uint16_t VOLUME_PARAMS_ENTRY = 0x100;
		constexpr uint16_t PERF_PARAMS_ENTRY = 0x200;
		constexpr uint16_t PATCH_PARAMS_ENTRY = 0x200;
		constexpr uint16_t PARTIAL_PARAMS_ENTRY = 128;
		constexpr uint8_t SAMPLE_PARAMS_ENTRY = 48;
		constexpr uint32_t S760_EXT_OS = AUDIO_SEGMENT_SIZE * S760_OS_CLUSTERS;
	}

	constexpr FAT_utils::FAT_attrs_t<uint16_t> FAT_ATTRS(ENDIANNESS, 0, 2,
		0xFFF5, 0xFFF8, 0xFFFF);

	constexpr uint32_t MIN_DISK_SIZE = On_disk_addrs::AUDIO_SECTION + AUDIO_SEGMENT_SIZE;
	constexpr uint16_t MAX_FAT_LENGTH = FAT_ATTRS.DATA_MAX + 1;
	constexpr uint32_t MAX_AUDIO_CAPACITY = MAX_FAT_LENGTH * AUDIO_SEGMENT_SIZE;
	constexpr uint32_t MAX_BLK_CNT = On_disk_addrs::AUDIO_SECTION / BLK_SIZE + AUDIO_SEGMENT_SIZE / BLK_SIZE * MAX_FAT_LENGTH;

	enum struct Media_type_t: uint8_t
	{
		HDD, //also used for CDs
		HDD_with_OS = 0x20,
		DSDD_SYS_770 = 0x31,
		DSHD_diskette,
		DSDD_diskette,
		HDD_with_OS_S760 = 0x40,
		DSHD_SYS_760 = 0x50
	};

	struct TOC_t
	{
		char name[17];
		uint32_t block_cnt;
		uint16_t volume_cnt;
		uint16_t perf_cnt;
		uint16_t patch_cnt;
		uint16_t partial_cnt;
		uint16_t sample_cnt;
	};

	struct Header_t
	{
		char machine_name[11];
		Media_type_t media_type;
		char text[80];
		TOC_t TOC;
	};
	
	enum struct Element_type_t: uint8_t
	{
		volume = 0x40,
		performance,
		patch,
		partial,
		sample
	};
	
	struct List_entry_t
	{
		char name[17];
		Element_type_t type;
		uint16_t next_idx;
		uint16_t prev_idx;
		uint16_t cur_idx;
		uint8_t program_num;
		uint16_t start_segment;
		uint16_t segment_cnt;
	};
}
#endif
