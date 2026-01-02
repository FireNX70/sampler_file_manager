#include <filesystem>
#include <cstring>
#include <bit>

#include "Utils/ints.hpp"
#include "EMU_FS_types.hpp"
#include "fs_common.hpp"
#include "EMU_FS_drv.hpp"

#include <iostream>

namespace EMU::FS
{
	template <typename T>
	requires std::integral<T>
	constexpr bool overlap_check(const T addr_a, const size_t count_a, const T addr_b, const size_t count_b)
	{
		const size_t final_addr_a = addr_a + count_a - (count_a != 0);
		const size_t final_addr_b = addr_b + count_b - (count_b != 0);

		return (addr_a >= addr_b && addr_a <= final_addr_b)
			|| (final_addr_a >= addr_b && final_addr_a <= final_addr_b)
			|| (addr_a <= addr_b && final_addr_a >= final_addr_b);
	}

	uint16_t fsck(const std::filesystem::path &fs_path, u16 &fsck_status)
	{
		std::unique_ptr<u8[]> data;
		std::unique_ptr<u16[]> FAT;

		bool bad_file_list_addr, should_write, should_write_alt, dupe;
		u8 digit_cnt;
		u16 err, next_file_list_blk, sum, cur, offset;
		u64 min_block_cnt, next_tgt;
		uintmax_t block_addr, FAT_len;

		std::fstream stream;
		std::vector<bool> map;
		std::vector<std::pair<u8, bool>> bank_num_map;
		std::string dedup_name;

		typedef std::unordered_map<std::string, std::pair<uintmax_t, bool>>
			name_map_t;

		name_map_t dir_name_map;
		name_map_t::iterator dir_it;

		Header_t header;
		Dir_t dir;
		File_t file;

		const uintmax_t blk_count = std::filesystem::file_size(fs_path)
			/ BLK_SIZE;

		//Initial size check
		if(!blk_count)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		stream.open(fs_path, std::ios_base::binary | std::ios_base::in
			| std::ios_base::out);

		if(!stream.is_open() || !stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		data = std::make_unique<u8[]>(BLK_SIZE);
		stream.read((char*)data.get(), BLK_SIZE);

		if(!stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		/*--------------------------Superblock checks-------------------------*/
		//Check magic
		if(std::memcmp(data.get(), MAGIC, 4))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);

		should_write = false;
		should_write_alt = false;
		sum = 0;

		for(u16 i = 0; i < BLK_SIZE - 2; i += 2)
		{
			std::memcpy(&cur, data.get() + i, 2);

			if constexpr(ENDIANNESS != std::endian::native)
				cur = std::byteswap(cur);

			sum += cur;
		}

		std::memcpy(&cur, data.get() + 510, 2);

		if constexpr(ENDIANNESS != std::endian::native)
			cur = std::byteswap(cur);

		//Checksum
		if(sum != cur)
		{
			fsck_status |= (u16)FSCK_STATUS::INVALID_CHECKSUM;

			//writing the superblock will recalculate the checksum
			should_write = true;
		}

		load_header(data.get(), header);

		if(header.cluster_shift + MIN_CLUSTER_SHIFT > MAX_CLUSTER_SHIFT)
		{
			fsck_status |= (u16)FSCK_STATUS::BAD_CLUSTER_SHIFT;
			header.cluster_shift = MAX_CLUSTER_SHIFT - MIN_CLUSTER_SHIFT;
			should_write = true;
		}

		const u32 CLUSTER_SIZE = calc_cluster_size(header.cluster_shift);
		const u16 BLOCKS_PER_CLUSTER = CLUSTER_SIZE / BLK_SIZE;

		if(!header.cluster_cnt)
		{
			fsck_status |= (u16)FSCK_STATUS::BAD_CLUSTER_CNT;
			/*TODO: By sorting the sections(root dir, file list, FAT and data),
			we might be able to figure out a reasonable cluster count.*/
		}
		else if(header.cluster_cnt > MAX_CLUSTER_CNT)
		{
			fsck_status |= (u16)FSCK_STATUS::BAD_CLUSTER_CNT;
			header.cluster_cnt = MAX_CLUSTER_CNT;
			should_write = true;
		}

		const u32 DATA_BLK_CNT = header.cluster_cnt * BLOCKS_PER_CLUSTER;
		const u16 EXPECTED_FAT_SIZE = (header.cluster_cnt + FAT_ATTRS.DATA_MIN) * 2;
		const u16 EXPECTED_FAT_BLOCKS = EXPECTED_FAT_SIZE / BLK_SIZE + ((EXPECTED_FAT_SIZE % BLK_SIZE) != 0);

		if(header.FAT_blk_cnt != EXPECTED_FAT_BLOCKS)
		{
			header.FAT_blk_cnt = EXPECTED_FAT_BLOCKS;
			fsck_status |= (u16)FSCK_STATUS::BAD_FAT_BLK_CNT;
			should_write = true;
		}

		//Kind of obsolete, but also kind of a failsafe?
		if(header.FAT_blk_cnt > MAX_FAT_BLOCKS)
		{
			header.FAT_blk_cnt = MAX_FAT_BLOCKS;
			fsck_status |= (u16)FSCK_STATUS::BAD_FAT_BLK_CNT;
			should_write = true;
		}

		/*We're prioritising cluster count/cluster size over block count. It
		might be better if we did it the other way around.*/
		min_block_cnt = FIRST_NON_RESERVED_BLK + header.dir_list_blk_cnt +
			header.file_list_blk_cnt + header.FAT_blk_cnt + DATA_BLK_CNT;

		if(header.block_cnt < min_block_cnt)
		{
			header.block_cnt = min_block_cnt;
			fsck_status |= (u16)FSCK_STATUS::BAD_BLOCK_CNT;
			should_write = true;
		}

		//Final size check
		if(blk_count < header.block_cnt)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::DISK_TOO_SMALL);

		if(header.dir_list_blk_addr < FIRST_NON_RESERVED_BLK)
		{
			header.dir_list_blk_addr = FIRST_NON_RESERVED_BLK;
			fsck_status |= (u16)FSCK_STATUS::BAD_ROOT_DIR;
			should_write = true;
		}

		if(header.file_list_blk_addr < FIRST_NON_RESERVED_BLK)
		{
			header.file_list_blk_addr = FIRST_NON_RESERVED_BLK;
			fsck_status |= (u16)FSCK_STATUS::BAD_FILE_LIST;
			bad_file_list_addr = true;
			should_write = true;
		}
		else if(header.file_list_blk_addr > 0xFFFF)
		{
			header.file_list_blk_addr = 0xFFFF;
			fsck_status |= (u16)FSCK_STATUS::BAD_FILE_LIST;
			bad_file_list_addr = true;
			should_write = true;
		}

		if(header.FAT_blk_addr < FIRST_NON_RESERVED_BLK)
		{
			header.FAT_blk_addr = FIRST_NON_RESERVED_BLK;
			fsck_status |= (u16)FSCK_STATUS::BAD_FAT_ADDR;
			should_write = true;
		}

		if(header.file_list_blk_addr + header.file_list_blk_cnt > 0x10000)
		{
			header.file_list_blk_cnt = 0x10000 - header.file_list_blk_addr;
			fsck_status |= (u16)FSCK_STATUS::BAD_FILE_LIST;
			should_write = true;
		}

		if(overlap_check(header.file_list_blk_addr, header.file_list_blk_cnt,
			header.dir_list_blk_addr, header.dir_list_blk_cnt))
		{
			fsck_status |= (u16)FSCK_STATUS::FILE_LIST_COLLISION;
			should_write_alt = true;
		}

		if(overlap_check(header.FAT_blk_addr, header.FAT_blk_cnt,
			header.dir_list_blk_addr, header.dir_list_blk_cnt)
			|| overlap_check(header.FAT_blk_addr, header.FAT_blk_cnt,
				header.file_list_blk_addr, header.file_list_blk_cnt))
		{
			fsck_status |= (u16)FSCK_STATUS::FAT_COLLISION;
			should_write_alt = true;
		}

		if(overlap_check(header.data_sctn_blk_addr, DATA_BLK_CNT,
			header.dir_list_blk_addr, header.dir_list_blk_cnt)
			|| overlap_check(header.data_sctn_blk_addr, DATA_BLK_CNT,
				header.file_list_blk_addr, header.file_list_blk_cnt)
			|| overlap_check(header.data_sctn_blk_addr, DATA_BLK_CNT,
				header.FAT_blk_addr, header.FAT_blk_cnt))
		{
			fsck_status |= (u16)FSCK_STATUS::DATA_COLLISION;
			should_write_alt = true;
		}
		/*----------------------End of superblock checks----------------------*/

		if(should_write && !should_write_alt)
		{
			write_header(data.get(), header);
			stream.seekp(0);
			stream.write((char*)data.get(), BLK_SIZE);
		}

		stream.seekg(BLK_SIZE);
		stream.read((char*)&next_file_list_blk, 2);
		if constexpr(ENDIANNESS != std::endian::native)
			next_file_list_blk = std::byteswap(next_file_list_blk);
		
		/*---------------------------Root dir checks--------------------------*/
		const u16 END_OF_FILE_BLKS = header.file_list_blk_addr + header.file_list_blk_cnt;
		map.resize(header.file_list_blk_cnt);
		dupe = false;

		for(u32 i = 0; i < header.dir_list_blk_cnt; i++)
		{
			block_addr = header.dir_list_blk_addr * BLK_SIZE + i * BLK_SIZE;
			stream.seekg(block_addr);
			stream.read((char*)data.get(), BLK_SIZE);
			offset = 0;

			should_write_alt = false;
			for(u8 j = 0; j < DIRS_PER_BLOCK; j++, offset += On_disk_sizes::DIR_ENTRY)
			{
				if(!is_valid_dir((Dir_type_e)data[offset + 0x11]))
					continue;

				dir.addr = block_addr + offset;
				load_dir<false>(data.get() + offset, dir);

				name_map_t::iterator dir_it = dir_name_map.find(dir.name);

				//Map only. Fix in a second pass.
				if(dir_it != dir_name_map.end())
				{
					dir_it->second.first++;
					fsck_status |= (u16)FSCK_STATUS::BAD_DIR;
					dupe = true;
				}
				else dir_name_map[dir.name] = {1, false};

				//Check for multiple references to the same block
				should_write = false;
				for(u8 k = 0; k < MAX_BLOCKS_PER_DIR; k++)
				{
					if(dir.blocks[k] >= header.file_list_blk_addr && dir.blocks[k] < END_OF_FILE_BLKS)
					{
						if(map[dir.blocks[k] - header.file_list_blk_addr])
						{
							dir.blocks[k] = 0xFFFF;
							fsck_status |= (u16)FSCK_STATUS::BAD_DIR;
							should_write = true;
							should_write_alt = true;
						}
						else map[dir.blocks[k] - header.file_list_blk_addr] = true;
					}					
				}

				if(should_write)
					write_dir(data.get() + offset, dir);
			}

			if(should_write_alt)
			{
				stream.seekp(block_addr);
				stream.write((char*)data.get(), BLK_SIZE);
			}
		}

		if(dupe)
		{
			//Second pass to fix duplicate names
			for(u32 i = 0; i < header.dir_list_blk_cnt; i++)
			{
				block_addr = header.dir_list_blk_addr * BLK_SIZE + i * BLK_SIZE;
				stream.seekg(block_addr);
				stream.read((char*)data.get(), BLK_SIZE);
				offset = 0;

				should_write_alt = false;
				for(u8 j = 0; j < DIRS_PER_BLOCK; j++, offset += On_disk_sizes::DIR_ENTRY)
				{
					if(!is_valid_dir((Dir_type_e)data[offset + 0x11]))
						continue;

					dir.addr = block_addr + offset;
					load_dir<false>(data.get() + offset, dir);

					dir_it = dir_name_map.find(dir.name);

					if(dir_it->second.second)
					{
						//Max 16 digits
						constexpr u64 MAX_VAL = 9'999'999'999'999'999;
						digit_cnt = 1;
						next_tgt = 10;

						for(u64 k = 2; k < MAX_VAL; k++)
						{
							if(k == next_tgt)
							{
								digit_cnt++;
								next_tgt *= 10;
							}

							const std::string num_str = std::to_string(k);

							if(digit_cnt > 14) dedup_name = num_str;
							else
							{
								dedup_name = dir.name;
								dedup_name.resize(std::min(dedup_name.size(), (size_t)(16 - digit_cnt - 1)));
								dedup_name += "_" + num_str;
							}

							if(!dir_name_map.contains(dedup_name))
							{
								prepare_dir_name(dedup_name, dir.name);
								dir.name[16] = 0;

								should_write = true;
								dir_name_map[dedup_name].second = true;
								break;
							}
						}
					}
					else dir_it->second.second = true;

					if(should_write)
					{
						write_dir(data.get() + offset, dir);
						should_write_alt = true;
					}
				}

				if(should_write_alt)
				{
					stream.seekp(block_addr);
					stream.write((char*)data.get(), BLK_SIZE);
				}
			}
		}

		//Check next dir content block
		should_write = false;
		for(s64 i = header.file_list_blk_cnt - 1; i >= 0; i--)
		{
			/*We want the first block in the last group of unused blocks. So the
			last free block before the first used block from the end.*/
			if(map[i])
			{
				if(next_file_list_blk != i + 1 + header.file_list_blk_addr)
				{
					next_file_list_blk = i + 1 + header.file_list_blk_addr;
					fsck_status |= (u16)FSCK_STATUS::BAD_NEXT_DIR_CONTENT_BLK;
					should_write = true;
				}

				break;
			}
		}

		if(should_write)
		{
			stream.seekp(BLK_SIZE);

			if constexpr(ENDIANNESS != std::endian::native)
				next_file_list_blk = std::byteswap(next_file_list_blk);

			stream.write((char*)&next_file_list_blk, 2);
		}

		//TODO: Make dir entries contiguous
		//TODO: Defragment dir contents
		//TODO: Maybe mark last
		/*-----------------------End of root dir checks-----------------------*/

		/*-------------------------Internal FAT checks------------------------*/
		//TODO: Map all chain starts
		//TODO: Check for chain integrity
		FAT_len = header.FAT_blk_cnt * (BLK_SIZE / 2);
		FAT = std::make_unique<u16[]>(FAT_len);
		stream.seekg(header.FAT_blk_addr * BLK_SIZE);
		stream.read((char*)FAT.get(), FAT_len * 2);

		if constexpr(ENDIANNESS != std::endian::native)
		{
			for(uintmax_t i = 0; i < FAT_len; i++)
				FAT[i] = std::byteswap(FAT[i]);
		}

		should_write = false;

		if(FAT[0] != FAT_ATTRS.RESERVED)
		{
			FAT[0] = FAT_ATTRS.RESERVED;
			fsck_status |= (u16)FSCK_STATUS::UNMARKED_RESERVED_CLUSTERS;
			should_write = true;
		}

		for(uintmax_t i = header.cluster_cnt + FAT_ATTRS.DATA_MIN; i < FAT_len; i++)
		{
			if(FAT[i] != FAT_ATTRS.RESERVED)
			{
				FAT[i] = FAT_ATTRS.RESERVED;
				fsck_status |= (u16)FSCK_STATUS::UNMARKED_RESERVED_CLUSTERS;
				should_write = true;
			}
		}

		if(should_write)
		{
			stream.seekg(header.FAT_blk_addr * BLK_SIZE);

			for(uintmax_t i = 0; i < FAT_len; i++)
			{
				cur = FAT[i];

				if constexpr(ENDIANNESS != std::endian::native)
					cur = std::byteswap(cur);

				stream.write((char*)&cur, 2);
			}
		}
		/*---------------------End of internal FAT checks---------------------*/

		/*--------------------------File list checks--------------------------*/
		data = std::make_unique<u8[]>(BLK_SIZE * 2);

		//TODO: Find and fix double chain references
		/*TODO: Find and fix chain collisions (or pointing to somewhere in
		another chain). Pointing somewhere mid-chain should be fairly easy to
		detect if we track chain starts. If we're pointing to a seemingly valid
		cluster which isn't a chain start, it's a bad start cluster.*/
		/*TODO: Remove unreferenced chains at the end of pass 1.*/
		for(u32 i = 0; i < header.dir_list_blk_cnt; i++)
		{
			block_addr = header.dir_list_blk_addr * BLK_SIZE + i * BLK_SIZE;
			stream.seekg(block_addr);
			stream.read((char*)data.get(), BLK_SIZE);
			offset = 0;

			for(u8 j = 0; j < DIRS_PER_BLOCK; j++, offset += On_disk_sizes::DIR_ENTRY)
			{
				if(!is_valid_dir((Dir_type_e)data[offset + 0x11]))
					continue;

				dir.addr = block_addr + offset;
				load_dir<false>(data.get() + offset, dir);
				
				bank_num_map.clear();
				bank_num_map.resize(0xFF);

				dupe = false;
				for(u8 k = 0; k < MAX_BLOCKS_PER_DIR; k++)
				{
					if(dir.blocks[k] < header.file_list_blk_addr || dir.blocks[k] >= END_OF_FILE_BLKS)
						continue;

					stream.seekg(dir.blocks[k] * BLK_SIZE);
					stream.read((char*)data.get() + BLK_SIZE, BLK_SIZE);
					cur = 0;

					should_write_alt = false;
					for(u8 l = 0; l < FILES_PER_BLOCK; l++, cur += On_disk_sizes::FILE_ENTRY)
					{
						if(!is_valid_file((File_type_e)data[BLK_SIZE + cur + 0x1A]))
							continue;

						should_write = false;
						load_file<false>(data.get() + BLK_SIZE + cur, file);

						//Only map. Fix in a second pass.
						if(bank_num_map[file.bank_num].first)
						{
							dupe = true;
							fsck_status |= (u16)FSCK_STATUS::BAD_FILE;
						}

						bank_num_map[file.bank_num].first++;

						//Check start cluster
						if((file.start_cluster >= FAT_ATTRS.DATA_MIN && file.start_cluster < header.cluster_cnt)
							&& (FAT[file.start_cluster] < FAT_ATTRS.DATA_MIN || FAT[file.start_cluster] >= header.cluster_cnt)
							&& FAT[file.start_cluster] != FAT_ATTRS.END_OF_CHAIN)
						{
							file.start_cluster = FAT_ATTRS.END_OF_CHAIN;
							fsck_status |= (u16)FSCK_STATUS::BAD_FILE;
							should_write = true;
						}

						//Check cluster count
						if(file.cluster_cnt > header.cluster_cnt)
						{
							file.cluster_cnt = header.cluster_cnt;
							fsck_status |= (u16)FSCK_STATUS::BAD_FILE;
							should_write = true;
						}

						//Check block count
						if(file.block_cnt > BLOCKS_PER_CLUSTER)
						{
							file.block_cnt = BLOCKS_PER_CLUSTER;
							fsck_status |= (u16)FSCK_STATUS::BAD_FILE;
							should_write = true;
						}

						//Check byte count
						if(file.byte_cnt > BLK_SIZE)
						{
							file.byte_cnt = BLK_SIZE;
							fsck_status |= (u16)FSCK_STATUS::BAD_FILE;
							should_write = true;
						}

						if(should_write)
						{
							write_file(data.get() + BLK_SIZE + cur, file);
							should_write_alt = true;
						}
					}

					if(should_write_alt)
					{
						stream.seekp(dir.blocks[k] * BLK_SIZE);
						stream.write((char*)data.get() + BLK_SIZE, BLK_SIZE);
					}
				}

				if(dupe)
				{
					for(std::pair<u8, bool> &entry : bank_num_map)
						entry.second = false;

					//Fix duplicate bank numbers
					for(u8 k = 0; k < MAX_BLOCKS_PER_DIR; k++)
					{
						if(dir.blocks[k] < header.file_list_blk_addr || dir.blocks[k] >= END_OF_FILE_BLKS)
							continue;

						stream.seekg(dir.blocks[k] * BLK_SIZE);
						stream.read((char*)data.get() + BLK_SIZE, BLK_SIZE);
						cur = 0;

						should_write_alt = false;
						for(u8 l = 0; l < FILES_PER_BLOCK; l++, cur += On_disk_sizes::FILE_ENTRY)
						{
							if(!is_valid_file((File_type_e)data[BLK_SIZE + cur + 0x1A]))
								continue;

							load_file<false>(data.get() + BLK_SIZE + cur, file);

							if(bank_num_map[file.bank_num].second)
							{
								for(u8 m = 0; m < 0xFF; m++)
								{
									if(bank_num_map[m].first) continue;

									file.bank_num = m;
									bank_num_map[m].first = 1;
									break;
								}

								write_file(data.get() + BLK_SIZE + cur, file);
								should_write_alt = true;
							}

							bank_num_map[file.bank_num].second = true;
						}

						if(should_write_alt)
						{
							stream.seekp(dir.blocks[k] * BLK_SIZE);
							stream.write((char*)data.get() + BLK_SIZE, BLK_SIZE);
						}
					}
				}
			}
		}
		/*-----------------------End of file list checks----------------------*/

		return 0;
	}
}
