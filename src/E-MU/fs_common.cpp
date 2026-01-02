#include <cstring>
#include <bit>
#include <algorithm>

#include "Utils/ints.hpp"
#include "EMU_FS_types.hpp"
#include "EMU_FS_drv.hpp"
#include "min_vfs/min_vfs_base.hpp"

namespace EMU::FS
{
	void load_header(const u8 src[BLK_SIZE], Header_t &dst)
	{
		u8 offset;

		offset = 4;

		std::memcpy(&dst.block_cnt, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.dir_list_blk_addr, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.dir_list_blk_cnt, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.file_list_blk_addr, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.file_list_blk_cnt, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.FAT_blk_addr, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.FAT_blk_cnt, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.data_sctn_blk_addr, src + offset, 4);
		offset += 4;

		std::memcpy(&dst.cluster_cnt, src + offset, 2);
		offset += 2 + 2;

		std::memcpy(&dst.cluster_shift, src + offset, 1);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			dst.block_cnt = std::byteswap(dst.block_cnt);
			dst.dir_list_blk_addr = std::byteswap(dst.dir_list_blk_addr);
			dst.dir_list_blk_cnt = std::byteswap(dst.dir_list_blk_cnt);
			dst.file_list_blk_addr = std::byteswap(dst.file_list_blk_addr);
			dst.file_list_blk_cnt = std::byteswap(dst.file_list_blk_cnt);
			dst.FAT_blk_addr = std::byteswap(dst.FAT_blk_addr);
			dst.FAT_blk_cnt = std::byteswap(dst.FAT_blk_cnt);
			dst.data_sctn_blk_addr = std::byteswap(dst.data_sctn_blk_addr);
			dst.cluster_cnt = std::byteswap(dst.cluster_cnt);
		}
	}

	void write_header(u8 dst[BLK_SIZE], Header_t src)
	{
		u8 offset;
		u16 sum, cur;

		if constexpr(std::endian::native != ENDIANNESS)
		{
			src.block_cnt = std::byteswap(src.block_cnt);
			src.dir_list_blk_addr = std::byteswap(src.dir_list_blk_addr);
			src.dir_list_blk_cnt = std::byteswap(src.dir_list_blk_cnt);
			src.file_list_blk_addr = std::byteswap(src.file_list_blk_addr);
			src.file_list_blk_cnt = std::byteswap(src.file_list_blk_cnt);
			src.FAT_blk_addr = std::byteswap(src.FAT_blk_addr);
			src.FAT_blk_cnt = std::byteswap(src.FAT_blk_cnt);
			src.data_sctn_blk_addr = std::byteswap(src.data_sctn_blk_addr);
			src.cluster_cnt = std::byteswap(src.cluster_cnt);
		}

		offset = 4;

		std::memcpy(dst + offset, &src.block_cnt, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.dir_list_blk_addr, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.dir_list_blk_cnt, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.file_list_blk_addr, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.file_list_blk_cnt, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.FAT_blk_addr, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.FAT_blk_cnt, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.data_sctn_blk_addr, 4);
		offset += 4;

		std::memcpy(dst + offset, &src.cluster_cnt, 2);
		offset += 2 + 2;

		std::memcpy(dst + offset, &src.cluster_shift, 1);
		offset += 1;

		dst[offset++] = 1;

		std::memcpy(dst + offset, &src.block_cnt, 4);
		offset += 4;

		std::memset(dst + offset, 0, 4);
		offset += 4;

		dst[offset++] = 1;
		dst[offset++] = 0xD;

		std::memset(dst + offset, 0, BLK_SIZE - offset - 2);
		
		sum = 0;

		for(u16 i = 0; i < BLK_SIZE - 2; i += 2)
		{
			std::memcpy(&cur, dst + i, 2);

			if constexpr(ENDIANNESS != std::endian::native)
				cur = std::byteswap(cur);

			sum += cur;
		}

		if constexpr(ENDIANNESS != std::endian::native)
			sum = std::byteswap(sum);

		std::memcpy(dst + 510, &sum, 2);
	}

	void write_dir(u8 *dst, Dir_t dir)
	{
		u8 offset;

		std::memcpy(dst, dir.name, 16);
		offset = 16;

		dst[16] = 0; //unused
		offset += 1;

		std::memcpy(dst + offset, &dir.type, 1);
		offset += 1;

		if constexpr(std::endian::native != ENDIANNESS)
			for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
				dir.blocks[i] = std::byteswap(dir.blocks[i]);

		std::memcpy(dst + offset, dir.blocks, 14);
	}

	void prepare_dir_name(const std::string &name, char dst[16])
	{
		constexpr u8 MAX_DIR_NAME_LEN = 16;
		const u8 dir_name_len = std::min(name.size(), (size_t)MAX_DIR_NAME_LEN);

		std::memcpy(dst, name.data(), dir_name_len);
		std::memset(dst + dir_name_len, 0, MAX_DIR_NAME_LEN - dir_name_len);
	}

	void write_file(u8 *dst, File_t file)
	{
		u8 offset;

		if constexpr(ENDIANNESS != std::endian::native)
		{
			file.start_cluster = std::byteswap(file.start_cluster);
			file.cluster_cnt = std::byteswap(file.cluster_cnt);
			file.block_cnt = std::byteswap(file.block_cnt);
			file.byte_cnt = std::byteswap(file.byte_cnt);
		}

		std::memcpy(dst, file.name, 16);
		offset = 16 + 1;

		dst[offset++] = file.bank_num;

		std::memcpy(dst + offset, &file.start_cluster, 2);
		offset += 2;

		std::memcpy(dst + offset, &file.cluster_cnt, 2);
		offset += 2;

		std::memcpy(dst + offset, &file.block_cnt, 2);
		offset += 2;

		std::memcpy(dst + offset, &file.byte_cnt, 2);
		offset += 2;

		dst[offset++] = (u8)file.type;

		std::memcpy(dst + offset, file.props, 5);
	}
}
