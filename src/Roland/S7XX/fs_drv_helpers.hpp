#ifndef S7XX_FS_DRIVER_HELPERS_INCLUDE_GUARD
#define S7XX_FS_DRIVER_HELPERS_INCLUDE_GUARD

#include <cstdint>
#include <fstream>

#include "Utils/ints.hpp"
#include "S7XX_FS_types.hpp"

namespace S7XX::FS
{
	constexpr bool is_HDD(const u8 media_type)
	{
		return media_type == (u8)Media_type_t::HDD
			|| media_type == (u8)Media_type_t::HDD_with_OS
			|| media_type == (u8)Media_type_t::HDD_with_OS_S760;
	}

	constexpr u16 block_cnt_to_cls_cnt(const u32 block_cnt)
	{
		return block_cnt / (AUDIO_SEGMENT_SIZE / BLK_SIZE);
	}

	uint16_t load_TOC(std::fstream &src, TOC_t &dst);
	uint16_t write_TOC(TOC_t src, std::fstream &dst);
}
#endif // !