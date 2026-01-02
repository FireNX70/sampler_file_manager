#include <cstdint>
#include <filesystem>
#include <string>
#include <fstream>
#include <algorithm>
#include <bit>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "S7XX_FS_drv.hpp"
#include "fs_drv_constants.hpp"

namespace S7XX::FS
{
	constexpr char SB_TXT[] = "S-7XX FS, formatted by sample_thing";

	uint16_t mkfs(const std::filesystem::path &fs_path, const std::string &label)
	{
		u8 temp_u8;
		u16 cluster_cnt, temp_u16;
		u32 blk_cnt, temp_u32;
		uintmax_t disk_size;
		std::ofstream dst_ofs;

		if(!std::filesystem::exists(fs_path) || !std::filesystem::is_regular_file(fs_path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_PATH);

		disk_size = std::filesystem::file_size(fs_path);

		if(disk_size < MIN_DISK_SIZE)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::DISK_TOO_SMALL);

		//out mode truncates, so use in | out mode
		dst_ofs.open(fs_path, std::ofstream::in | std::ofstream::binary);

		if(!dst_ofs.is_open() || !dst_ofs.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		//header
		for(u8 i = 0; i < 4; i++) dst_ofs.put(0); //first 4 bytes must be 0
		dst_ofs.write(MACHINE_NAME, sizeof(MACHINE_NAME)); //magic
		dst_ofs.put((u8)Media_type_t::HDD);
		dst_ofs.write(SB_TXT, sizeof(SB_TXT));

		for(u8 i = sizeof(SB_TXT); i < On_disk_sizes::HEADER - 16; i++)
			dst_ofs.put(0);

		//TOC
		dst_ofs.seekp(On_disk_addrs::TOC);

		temp_u8 = std::min((size_t)16, label.size());

		dst_ofs.write(label.c_str(), temp_u8);

		for(u8 i = 0; i < 16 - temp_u8; i++)
			dst_ofs.put(' ');

		blk_cnt = std::min(disk_size / BLK_SIZE, (uintmax_t)MAX_BLK_CNT);
		temp_u32 = blk_cnt;

		if constexpr(ENDIANNESS != std::endian::native)
			temp_u32 = std::byteswap(temp_u32);

		dst_ofs.write((char *)&temp_u32, 4);

		for(u8 i = 0; i < 10; i++) dst_ofs.put(0);
		for(u8 i = 0; i < 2; i++) dst_ofs.put(0xFF);

		//FAT
		dst_ofs.seekp(On_disk_addrs::FAT);
		dst_ofs.put(0xFA);
		dst_ofs.put(0xFF);

		cluster_cnt = (blk_cnt - On_disk_addrs::AUDIO_SECTION / BLK_SIZE) / (AUDIO_SEGMENT_SIZE / BLK_SIZE);
		temp_u16 = cluster_cnt;

		if constexpr(ENDIANNESS != std::endian::native)
			temp_u16 = std::byteswap(temp_u16);

		dst_ofs.write((char *)&temp_u16, 2);

		for(u16 i = 0; i < cluster_cnt; i++)
			dst_ofs.write((char *)&FAT_ATTRS.FREE_CLUSTER, 2);

		for(u32 i = cluster_cnt + FAT_ATTRS.DATA_MIN; i < 0x10000; i++)
		{
			dst_ofs.put(0xFF);
			dst_ofs.put(0xFF);
		}

		//Lists
		for(u8 i = 1; i < sizeof(TYPE_ATTRS) / sizeof(TYPE_ATTRS[0]); i++)
		{
			for(u16 j = 0; j < TYPE_ATTRS[i].MAX_CNT; j++)
			{
				dst_ofs.seekp(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY);
				dst_ofs.put(0); //first char of name
				//e_type, doesn't seem to actually do anything
			}
		}

		if(!dst_ofs.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		dst_ofs.close();

		return 0;
	}
}