#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <bit>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "S7XX_FS_types.hpp"
#include "S7XX_FS_drv.hpp"
#include "fs_drv_constants.hpp"
#include "fs_drv_helpers.hpp"

namespace S7XX::FS
{
	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status)
	{
		u8 media_type, element_type;
		char magic[10], name0;
		u16 err, entry_cnt, first_nul_entry, last_used_entry, expected_cls_cnt, cls_val, FAT_free_cls_cnt, FAT_len, free_cls_cnt;

		TOC_t TOC;

		std::fstream fs_fstr;

		if(!std::filesystem::exists(fs_path)
			|| !std::filesystem::is_regular_file(fs_path))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);

		/*We haven't checked the magic yet, so we can't really know the disk is
		 *too small because we don't know that we've even got something that
		 *should be an S7XX FS. That's why we return WRONG_FS instead of
		 *DISK_TOO_SMALL.*/
		if(std::filesystem::file_size(fs_path) < MIN_DISK_SIZE)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		fs_fstr.open(fs_path, std::fstream::in | std::fstream::out
			| std::fstream::binary);

		if(!fs_fstr.is_open() || !fs_fstr.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		fs_fstr.seekg(4);
		fs_fstr.read(magic, sizeof(magic));

		if(std::memcmp(magic, MACHINE_NAME, sizeof(magic)))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		fs_fstr.seekg(15);
		media_type = fs_fstr.get();

		if(!is_HDD(media_type))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::MEDIA_TYPE_NOT_HDD);

		//check size
		err = load_TOC(fs_fstr, TOC);
		if(err) return err;

		if(TOC.block_cnt > MAX_BLK_CNT)
		{
			fsck_status |= FSCK_ERR::TOO_LARGE;
			TOC.block_cnt = MAX_BLK_CNT;
		}

		if(TOC.block_cnt * BLK_SIZE > std::filesystem::file_size(fs_path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::TOO_SMALL);

		//check lists and list + TOC consistency
		for(u8 i = 1; i < 6; i++)
		{
			entry_cnt = 0;
			first_nul_entry = 0xFFFF;
			last_used_entry = 0;

			for(u16 j = 0; j < TYPE_ATTRS[i].MAX_CNT; j++)
			{
				fs_fstr.seekg(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY);
				name0 = fs_fstr.get();

				if(name0)
				{
					if(name0 != (char)0xFE)
					{
						last_used_entry = j;
						entry_cnt++;

						fs_fstr.seekg(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY + 0x10);
						element_type = fs_fstr.get();

						if(element_type != (u8)TYPE_ATTRS[i].ELEMENT_TYPE)
						{
							fsck_status |= FSCK_ERR::BAD_FENTRY;

							fs_fstr.seekp(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY + 0x10);
							fs_fstr.put((char)TYPE_ATTRS[i].ELEMENT_TYPE);
						}
					}
				}
				else if(first_nul_entry == 0xFFFF) first_nul_entry = j;
			}

			if(entry_cnt != TOC.*TYPE_ATTRS[i].TOC_PTR)
			{
				fsck_status |= FSCK_ERR::TOC_INCONSISTENCY;
				TOC.*TYPE_ATTRS[i].TOC_PTR = entry_cnt;
			}

			if(last_used_entry > first_nul_entry)
			{
				fsck_status |= FSCK_ERR::INACCESSIBLE_FENTRIES;

				for(u16 j = first_nul_entry; j < last_used_entry; j++)
				{
					fs_fstr.seekg(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY);
					name0 = fs_fstr.get();

					if(!name0)
					{
						fs_fstr.seekp(TYPE_ATTRS[i].LIST_ADDR + j * On_disk_sizes::LIST_ENTRY);
						fs_fstr.put(0xFE);
					}
				}
			}
		}

		err = write_TOC(TOC, fs_fstr);
		if(err) return err;

		//TODO: check FS-type vs first 114 clusters
		//check FAT
		expected_cls_cnt = block_cnt_to_cls_cnt(TOC.block_cnt - (On_disk_addrs::AUDIO_SECTION / BLK_SIZE));

		fs_fstr.seekg(On_disk_addrs::FAT);
		fs_fstr.read((char*)&cls_val, 2);

		if constexpr(ENDIANNESS != std::endian::native)
			cls_val = std::byteswap(cls_val);

		if(cls_val != 0xFFFA)
		{
			fsck_status |= FSCK_ERR::BAD_CLS0_VAL;
			cls_val = 0xFFFA;

			if constexpr(ENDIANNESS != std::endian::native)
				cls_val = std::byteswap(cls_val);

			fs_fstr.seekp(On_disk_addrs::FAT);
			fs_fstr.write((char *)&cls_val, 2);
		}

		fs_fstr.read((char *)&FAT_free_cls_cnt, 2);

		if constexpr(ENDIANNESS != std::endian::native)
			FAT_free_cls_cnt = std::byteswap(FAT_free_cls_cnt);

		free_cls_cnt = 0;

		for(u32 i = FAT_ATTRS.DATA_MIN; i < 0x10000; i++)
		{
			fs_fstr.read((char *)&cls_val, 2);

			if constexpr(ENDIANNESS != std::endian::native)
				cls_val = std::byteswap(cls_val);

			if((i >= expected_cls_cnt + FAT_ATTRS.DATA_MIN) && (cls_val < FAT_ATTRS.END_OF_CHAIN + 1))
			{
				fsck_status |= FSCK_ERR::UNMARKED_RESVD_CLS;
				cls_val = FAT_ATTRS.RESERVED;

				if constexpr(ENDIANNESS != std::endian::native)
					cls_val = std::byteswap(cls_val);

				fs_fstr.seekp(On_disk_addrs::FAT + i * 2);
				fs_fstr.write((char *)&cls_val, 2);

				if constexpr(ENDIANNESS != std::endian::native)
					cls_val = FAT_ATTRS.RESERVED;
			}

			if(cls_val == FAT_ATTRS.FREE_CLUSTER) free_cls_cnt++;
		}

		if(free_cls_cnt != FAT_free_cls_cnt)
		{
			fsck_status |= FSCK_ERR::FREE_CLS_CNT_MISMATCH;

			if constexpr(ENDIANNESS != std::endian::native)
				free_cls_cnt = std::byteswap(free_cls_cnt);

			fs_fstr.seekp(On_disk_addrs::FAT + 2);
			fs_fstr.write((char *)&free_cls_cnt, 2);
		}

		if(!fs_fstr.is_open() || !fs_fstr.good())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::IO_ERROR);

		//TODO: Check sample list + FAT consistency (and maybe chains)

		return 0;
	}
}
