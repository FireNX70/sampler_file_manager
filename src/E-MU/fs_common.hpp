#ifndef EMU_FS_COMMON_HEADER_INCLUDE_GUARD
#define EMU_FS_COMMON_HEADER_INCLUDE_GUARD

#include <cstring>
#include <algorithm>
#include <bit>

#include "Utils/ints.hpp"
#include "EMU_FS_types.hpp"

namespace EMU::FS
{
	constexpr u32 calc_cluster_size(const u8 cluster_shift)
	{
		return 1 << (MIN_CLUSTER_SHIFT + cluster_shift);
	}

	constexpr bool is_valid_dir(const Dir_type_e dtype)
	{
		using enum Dir_type_e;

		return dtype == NORMAL || dtype == LAST;
	}

	constexpr bool is_valid_file(const File_type_e ftype)
	{
		using enum File_type_e;

		return ftype == STD || ftype == UPD || ftype == SYS;
	}

	void load_header(const u8 src[BLK_SIZE], Header_t &dst);
	void write_header(u8 dst[BLK_SIZE], Header_t src);

	template<const bool REMAP_SLASHES>
	void load_dir(const u8 *src, Dir_t &dir)
	{
		u8 offset;

		std::memcpy(dir.name, src, 16);
		dir.name[16] = 0;

		if constexpr(REMAP_SLASHES)
			std::replace(dir.name, dir.name + sizeof(Dir_t::name), '/', '\\');

		offset = 16 + 1;

		std::memcpy(&dir.type, src + offset, 1);
		offset += 1;

		std::memcpy(dir.blocks, src + offset, 14);

		if constexpr(std::endian::native != ENDIANNESS)
			for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
				dir.blocks[i] = std::byteswap(dir.blocks[i]);
	}

	void write_dir(u8 *dst, Dir_t dir);
	void prepare_dir_name(const std::string &name, char dst[16]);

	template<const bool REMAP_SLASHES>
	u16 load_file(const u8 *src, File_t &dst)
	{
		u8 offset;

		std::memcpy(dst.name, src, 16);
		dst.name[16] = 0;

		if constexpr(REMAP_SLASHES)
			std::replace(dst.name, dst.name + sizeof(File_t::name), '/', '\\');

		offset = 16 + 1;

		std::memcpy(&dst.bank_num, src + offset, 1);
		offset += 1;

		std::memcpy(&dst.start_cluster, src + offset, 2);
		offset += 2;

		std::memcpy(&dst.cluster_cnt, src + offset, 2);
		offset += 2;

		std::memcpy(&dst.block_cnt, src + offset, 2);
		offset += 2;

		std::memcpy(&dst.byte_cnt, src + offset, 2);
		offset += 2;

		std::memcpy(&dst.type, src + offset, 1);
		offset += 1;

		std::memcpy(dst.props, src + offset, 5);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			dst.start_cluster = std::byteswap(dst.start_cluster);
			dst.cluster_cnt = std::byteswap(dst.cluster_cnt);
			dst.block_cnt = std::byteswap(dst.block_cnt);
			dst.byte_cnt = std::byteswap(dst.byte_cnt);
		}

		return 0;
	}

	void write_file(u8 *dst, File_t file);
}

#endif // !EMU_FS_COMMON_HEADER_INCLUDE_GUARD
