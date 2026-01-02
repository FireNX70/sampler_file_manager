#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <bit>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <regex>
#include <tuple>

#include "Utils/ints.hpp"
#include "Utils/str_util.hpp"
#include "Utils/utils.hpp"
#include "EMU_FS_types.hpp"
#include "fs_common.hpp"
#include "EMU_FS_drv.hpp"
#include "Utils/FAT_utils.hpp"
#include "min_vfs/min_vfs_base.hpp"

/*Mount can't be made const, even if we wanted it to be read only, because it
contains the fstream. Kind of a PITA.*/

/*TODO: Because of the way we deal with '/' in filenames; if two directories
exist with almost identical names with the exception of '/' and '\', and the one
with '/' is the first one; extending the directory, causing it to have its
dentry updated (overwritten), will cause it to become a duplicate of the other
directory; because its '/'s will have been replaced with '\'s. This can be fixed
by running fsck, but it's not great. I should try to figure out some way of
dealing with this.*/

namespace EMU::FS
{
	enum class comp_e: u8
	{
		NONE,
		NAME,
		BANK
	};

	union comp_to_t
	{
		const char *name;
		u8 bank_num;
	};

	static constexpr u32 calc_file_size(const File_t &file, const u32 cluster_size)
	{
		return (file.cluster_cnt - (file.cluster_cnt != 0)) * cluster_size
			+ (file.block_cnt - (file.block_cnt != 0)) * BLK_SIZE
			+ file.byte_cnt;
	}

	static constexpr u16 get_dir_size(const u32 file_list_blk_addr, const u32 file_list_blk_cnt, const Dir_t &dir)
	{
		const u16 END_OF_FILE_LIST = file_list_blk_addr + file_list_blk_cnt;

		u16 size = 0;

		for(const u16 sector: dir.blocks)
		{
			if(sector >= file_list_blk_addr && sector < END_OF_FILE_LIST)
				size += BLK_SIZE;
		}

		return size;
	}

	static constexpr u64 cluster_to_abs_addr(const Header_t &header, const u16 cluster_addr)
	{
		return header.data_sctn_blk_addr * BLK_SIZE + (cluster_addr - FAT_ATTRS.DATA_MIN) *
			calc_cluster_size(header.cluster_shift);
	}

	static constexpr void dir_to_dentry(const u32 file_list_blk_addr, const u32 file_list_blk_cnt, const Dir_t &src, min_vfs::dentry_t &dst)
	{
		dst.fname = src.name;
		dst.ftype = min_vfs::ftype_t::dir;
		dst.ctime = 0;
		dst.mtime = 0;
		dst.atime = 0;
		dst.fsize = get_dir_size(file_list_blk_addr, file_list_blk_cnt, src);
	}

	static constexpr void file_to_dentry(const u32 cluster_size, const File_t &src, min_vfs::dentry_t &dst)
	{
		dst.fname = std::to_string(src.bank_num) + "-" + src.name;
		dst.ftype = min_vfs::ftype_t::file;
		dst.ctime = 0;
		dst.mtime = 0;
		dst.atime = 0;
		dst.fsize = calc_file_size(src, cluster_size);
	}

	const std::regex NAME_WITH_IDX("([0-9]{1,3})-(.*)", std::regex::ECMAScript | std::regex::optimize);
	constexpr u16 DUMMY_IDX = 0xFFFF;

	static uint16_t bank_num_and_name_from_name(const std::string &filename, std::string &cleaned_name)
	{
		u16 res;
		std::smatch rx_match;

		const bool has_bank_num = std::regex_match(filename, rx_match, NAME_WITH_IDX);

		if(has_bank_num)
		{
			try
			{
				res = std::stoi(rx_match[1]);
			}
			catch(const std::invalid_argument &e)
			{
				res = DUMMY_IDX;
			}
			catch(const std::out_of_range &e)
			{
				res = DUMMY_IDX;
			}

			cleaned_name = rx_match[2];
		}
		else
		{
			res = DUMMY_IDX;
			cleaned_name = filename;
		}

		if(cleaned_name.size() > 16) cleaned_name.resize(16);
		else if(cleaned_name.size() < 16)
		{
			cleaned_name.append(16 - cleaned_name.size(), 0);
			/*for(u8 i = cleaned_name.size(); i < 16; i++)
				cleaned_name.append(0);*/
		}

		return res;
	}

	u16 load_header(std::fstream &disk, Header_t &header)
	{
		char magic[4];
		disk.read(magic, 4);

		if(std::strncmp(magic, MAGIC, 4))
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::WRONG_FS);

		disk.read((char*)&header.block_cnt, 4);
		disk.read((char*)&header.dir_list_blk_addr, 4);
		disk.read((char*)&header.dir_list_blk_cnt, 4);
		disk.read((char*)&header.file_list_blk_addr, 4);
		disk.read((char*)&header.file_list_blk_cnt, 4);
		disk.read((char*)&header.FAT_blk_addr, 4);
		disk.read((char*)&header.FAT_blk_cnt, 4);
		disk.read((char*)&header.data_sctn_blk_addr, 4);
		disk.read((char*)&header.cluster_cnt, 2);
		disk.seekg(disk.tellg() + (std::streamoff)2);
		disk.read((char*)&header.cluster_shift, 1);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			header.block_cnt = std::byteswap(header.block_cnt);
			header.dir_list_blk_addr = std::byteswap(header.dir_list_blk_addr);
			header.dir_list_blk_cnt = std::byteswap(header.dir_list_blk_cnt);
			header.file_list_blk_addr = std::byteswap(header.file_list_blk_addr);
			header.file_list_blk_cnt = std::byteswap(header.file_list_blk_cnt);
			header.FAT_blk_addr = std::byteswap(header.FAT_blk_addr);
			header.FAT_blk_cnt = std::byteswap(header.FAT_blk_cnt);
			header.data_sctn_blk_addr = std::byteswap(header.data_sctn_blk_addr);
			header.cluster_cnt = std::byteswap(header.cluster_cnt);
		}

		if(disk.fail())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		if(!header.cluster_cnt || header.cluster_cnt + 1 > FAT_ATTRS.DATA_MAX)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_CLUSTER_CNT);

		if(!header.FAT_blk_cnt || header.FAT_blk_cnt * BLK_SIZE / 2 > 0x8000)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_FAT_BLK_CNT);
		
		return 0;
	}

	static u16 check_header(const Header_t &header)
	{
		if(!header.cluster_cnt || header.cluster_cnt > FAT_ATTRS.DATA_MAX)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_CLUSTER_CNT);

		if(!header.FAT_blk_cnt || (header.FAT_blk_cnt * BLK_SIZE / 2) > 0x8000)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_FAT_BLK_CNT);

		if(header.file_list_blk_addr + header.file_list_blk_cnt > 0x10000)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_FILE_LIST_ADDR_OR_CNT);

		return 0;
	}

	u16 load_dir(std::fstream &disk, Dir_t &dir)
	{
		dir.addr = disk.tellg();
		disk.read(dir.name, 16);
		dir.name[16] = 0;
		disk.seekg(disk.tellg() + (std::streampos)1);
		disk.read((char*)&dir.type, 1);
		disk.read((char*)dir.blocks, 14);

		if constexpr(std::endian::native != ENDIANNESS)
			for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
				dir.blocks[i] = std::byteswap(dir.blocks[i]);

		if(disk.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
		else return 0;
	}

	static u16 write_dir(std::fstream &fstr, Dir_t dir)
	{
		fstr.seekp(dir.addr);

		fstr.write(dir.name, 16);
		fstr.seekg(1, std::ios_base::cur);
		fstr.write((char*)&dir.type, 1);

		if constexpr(std::endian::native != ENDIANNESS)
			for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
				dir.blocks[i] = std::byteswap(dir.blocks[i]);

		fstr.write((char*)dir.blocks, 14);

		if(!fstr.is_open() || !fstr.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
		
		return 0;
	}

	static void map_dir_blocks(filesystem_t &fs, std::vector<bool> &map)
	{
		const u16 END_OF_FILE_BLKS = fs.header.file_list_blk_addr + map.size();

		u8 d_type;
		u16 blks[MAX_BLOCKS_PER_DIR];

		for(u32 i = 0; i < fs.header.dir_list_blk_cnt; i++)
		{
			fs.stream.seekg(fs.header.dir_list_blk_addr * BLK_SIZE + i * BLK_SIZE + 0x11);

			for(u8 j = 0; j < DIRS_PER_BLOCK; j++)
			{
				d_type = fs.stream.get();

				if(!is_valid_dir((Dir_type_e)d_type))
				{
					fs.stream.seekg(14 + 0x11, std::ios_base::cur);
					continue;
				}

				fs.stream.read((char*)blks, 14);
				
				for(u8 k = 0; k < MAX_BLOCKS_PER_DIR; k++)
				{
					if constexpr(ENDIANNESS != std::endian::native)
						blks[k] = std::byteswap(blks[k]);

					if(blks[k] >= fs.header.file_list_blk_addr && blks[k] < END_OF_FILE_BLKS)
						map[blks[k] - fs.header.file_list_blk_addr] = true;
				}

				fs.stream.seekg(0x11, std::ios_base::cur);
			}
		}
	}

	u16 load_file(std::fstream &disk, File_t &file)
	{
		file.addr = disk.tellg();
		disk.read(file.name, 16);
		file.name[16] = 0;
		disk.seekg(disk.tellg() + (std::streampos)1);
		disk.read((char*)&file.bank_num, 1);
		disk.read((char*)&file.start_cluster, 2);
		disk.read((char*)&file.cluster_cnt, 2);
		disk.read((char*)&file.block_cnt, 2);
		disk.read((char*)&file.byte_cnt, 2);
		disk.read((char*)&file.type, 1);
		disk.read((char*)file.props, 5);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			file.start_cluster = std::byteswap(file.start_cluster);
			file.cluster_cnt = std::byteswap(file.cluster_cnt);
			file.block_cnt = std::byteswap(file.block_cnt);
			file.byte_cnt = std::byteswap(file.byte_cnt);
		}

		if(disk.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
		else return 0;
	}

	static u16 write_file(std::fstream &disk, File_t file)
	{
		disk.seekp(file.addr);

		disk.write(file.name, 16);
		disk.seekp(1, std::ios_base::cur);
		disk.write((char*)&file.bank_num, 1);
		
		if constexpr(ENDIANNESS != std::endian::native)
		{
			file.start_cluster = std::byteswap(file.start_cluster);
			file.cluster_cnt = std::byteswap(file.cluster_cnt);
			file.block_cnt = std::byteswap(file.block_cnt);
			file.byte_cnt = std::byteswap(file.byte_cnt);
		}

		disk.write((char*)&file.start_cluster, 2);
		disk.write((char*)&file.cluster_cnt, 2);
		disk.write((char*)&file.block_cnt, 2);
		disk.write((char*)&file.byte_cnt, 2);
		disk.write((char*)&file.type, 1);
		disk.write((char*)file.props, 5);

		if(disk.fail()) return ret_val_setup(LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
		else return 0;
	}

	template<bool check_name>
	u16 load_dir_from_name(filesystem_t &mount, const char *dirname, std::vector<Dir_t> &dst)
	{
		u8 temp[BLK_SIZE];
		u16 offset;
		uintmax_t block_abs_addr;

		Dir_t dir;

		block_abs_addr = mount.header.dir_list_blk_addr * BLK_SIZE;
		for(u32 block = 0; block < mount.header.dir_list_blk_cnt; block++, block_abs_addr += BLK_SIZE)
		{
			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < DIRS_PER_BLOCK; i++, offset += On_disk_sizes::DIR_ENTRY)
			{
				if(!is_valid_dir((Dir_type_e)temp[offset + 0x11])) continue;

				if constexpr(check_name)
				{
					std::replace(temp + offset, temp + offset + 16, '/', '\\');

					if(std::strncmp((char*)(temp + offset), dirname, 16))
						continue;
				}

				dir.addr = block_abs_addr + offset;
				load_dir<true>(temp + offset, dir);

				dst.push_back(dir);
				if constexpr(check_name) return 0;
			}
		}

		if constexpr(check_name)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
		else return 0;
	}

	template<const comp_e COMP>
	u16 load_file_in_dir(filesystem_t &mount, const Dir_t &dir,
						 const comp_to_t comp_to, std::vector<File_t> &dst)
	{
		u8 temp[BLK_SIZE];
		u16 offset;
		uintmax_t block_abs_addr;
		
		File_t file;

		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] < mount.header.file_list_blk_addr ||
				dir.blocks[i] >= mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt)
				continue;

			block_abs_addr = dir.blocks[i] * BLK_SIZE;

			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < FILES_PER_BLOCK; i++, offset += On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((File_type_e)temp[offset + 0x1A])) continue;

				if constexpr(COMP == comp_e::NAME)
				{
					std::replace(temp + offset, temp + offset + 16, '/', '\\');

					if(std::strncmp((char*)(temp + offset), comp_to.name, 16))
						continue;
				}
				else if constexpr(COMP == comp_e::BANK)
				{
					if(temp[offset + 0x11] != comp_to.bank_num) continue;
				}

				file.addr = block_abs_addr + offset;
				load_file<true>(temp + offset, file);

				dst.push_back(file);
				if constexpr(COMP != comp_e::NONE) return 0;
			}
		}

		if constexpr(COMP == comp_e::NONE) return 0;
		else
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	}

	static u16 update_next_file_list_block(filesystem_t &mount, const u16 used)
	{
		if(used >= mount.next_file_list_blk)
		{
			u16 i, final_block_addr;

			for(i = used - mount.header.file_list_blk_addr; i < mount.header.file_list_blk_cnt; i++)
			{
				if(!mount.dir_content_block_map[i])
				{
					mount.next_file_list_blk = i + mount.header.file_list_blk_addr;
					break;
				}
			}

			if(i >= mount.header.file_list_blk_cnt && mount.next_file_list_blk != i + mount.header.file_list_blk_addr)
				mount.next_file_list_blk = mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt;

			final_block_addr = mount.next_file_list_blk;

			if constexpr(ENDIANNESS != std::endian::native)
				final_block_addr = std::byteswap(final_block_addr);

			mount.stream.seekp(BLK_SIZE);
			mount.stream.write((char*)&final_block_addr, 2);
			if(!mount.stream.is_open() || !mount.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
		}

		return 0;
	}

	static u16 extend_dir(filesystem_t &mount, Dir_t &dir, u16 &new_block)
	{
		const u16 END_OF_FILE_LIST = mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt;

		u16 err;

		for(u16 &block_addr: dir.blocks)
		{
			if(block_addr < mount.header.file_list_blk_addr || block_addr >= END_OF_FILE_LIST)
			{
				for(u16 i = 0; i < mount.header.file_list_blk_cnt; i++)
				{
					if(!mount.dir_content_block_map[i])
					{
						//Consider 0ing out the new sector
						block_addr = i + mount.header.file_list_blk_addr;
						err = write_dir(mount.stream, dir);
						if(err) return err;

						mount.dir_content_block_map[i] = true;
						new_block = block_addr;

						return update_next_file_list_block(mount, block_addr);
					}
				}

				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);
			}
		}

		return ret_val_setup(LIBRARY_ID, (u8)ERR::DIR_SIZE_MAXED);
	}

	/*I could unify both of these into a single template, but I'm not sure it's
	worth it.*/
	static u16 find_file_or_free(filesystem_t &mount, const Dir_t &dir, const u8 bank_num, File_t &dst)
	{
		bool found_free;
		u8 temp[BLK_SIZE];
		u16 offset;
		uintmax_t block_abs_addr, free_abs_addr;

		found_free = false;
		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] < mount.header.file_list_blk_addr ||
				dir.blocks[i] >= mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt)
				continue;

			block_abs_addr = dir.blocks[i] * BLK_SIZE;

			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < FILES_PER_BLOCK; i++, offset += On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((EMU::FS::File_type_e)temp[offset + 0x1A]))
				{
					if(!found_free)
					{
						free_abs_addr = block_abs_addr + offset;
						found_free = true;
					}

					continue;
				}

				if(temp[offset + 0x11] != bank_num) continue;

				dst.addr = block_abs_addr + offset;
				load_file<true>(temp + offset, dst);
				return 0;
			}
		}

		dst.start_cluster = FAT_ATTRS.END_OF_CHAIN;
		dst.cluster_cnt = 0;
		dst.block_cnt = 0;
		dst.byte_cnt = 0;
		dst.bank_num = bank_num;
		dst.type = File_type_e::DEL;
		std::memset(dst.props, 0, sizeof(dst.props));

		if(found_free) dst.addr = free_abs_addr;
		else return ret_val_setup(LIBRARY_ID, (u8)ERR::TRY_GROW_DIR);

		return 0;
	}

	static u16 find_file_or_free(filesystem_t &mount, const Dir_t &dir,
								 const char *fname, File_t &dst)
	{
		bool found_free, bank_nums[255];
		u8 temp[BLK_SIZE], i;
		u16 offset;
		uintmax_t block_abs_addr, free_abs_addr;

		std::memset(bank_nums, 0, sizeof(bank_nums));
		found_free = false;
		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] < mount.header.file_list_blk_addr ||
				dir.blocks[i] >= mount.header.file_list_blk_addr
					+ mount.header.file_list_blk_cnt)
				continue;

			block_abs_addr = dir.blocks[i] * BLK_SIZE;

			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID,
				(u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < FILES_PER_BLOCK; i++, offset += On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((EMU::FS::File_type_e)temp[offset + 0x1A]))
				{
					if(!found_free)
					{
						free_abs_addr = block_abs_addr + offset;
						found_free = true;
					}

					continue;
				}

				bank_nums[temp[offset + 0x11]] = true;

				std::replace(temp + offset, temp + offset + 16, '/', '\\');

				if(std::strncmp((char*)(temp + offset), fname, 16))
					continue;

				dst.addr = block_abs_addr + offset;
				load_file<true>(temp + offset, dst);
				return 0;
			}
		}

		for(i = 0; i < 0x80; i++)
		{
			if(!bank_nums[i])
			{
				dst.bank_num = i;
				break;
			}
		}

		if(i > 0x7F) //This should be impossible
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NO_SPACE_LEFT);

		dst.name[16] = 0;
		dst.start_cluster = FAT_ATTRS.END_OF_CHAIN;
		dst.cluster_cnt = 0;
		dst.block_cnt = 0;
		dst.byte_cnt = 0;
		dst.type = File_type_e::DEL;
		std::memset(dst.props, 0, sizeof(dst.props));

		/*The only reason I even bothered guarding the assignment is MSVC
		asserts on reading uninitialized variables. The only part that really
		needs to be conditional is the return.*/
		if(found_free) dst.addr = free_abs_addr;
		else return ret_val_setup(LIBRARY_ID, (u8)ERR::TRY_GROW_DIR);

		return 0;
	}

	static std::tuple<u16, u16, u16> file_size_to_counts(const u32 cls_size, const uintmax_t tgt_size)
	{
		u16 cls_cnt, block_cnt, byte_cnt;

		byte_cnt = tgt_size % BLK_SIZE;
		block_cnt = (tgt_size % cls_size) / BLK_SIZE + (byte_cnt != 0);
		cls_cnt = tgt_size / cls_size + (block_cnt != 0);
		block_cnt += cls_size / BLK_SIZE * (block_cnt == 0) * (tgt_size != 0);
		byte_cnt += BLK_SIZE * (byte_cnt == 0) * (tgt_size != 0);

		return {cls_cnt, block_cnt, byte_cnt};
	}

	static u16 resize_file(filesystem_t &mount, File_t &file, const uintmax_t new_size)
	{
		const std::tuple<u16, u16, u16> counts = file_size_to_counts(calc_cluster_size(mount.header.cluster_shift), new_size);

		const bool grow = std::get<0>(counts) > file.cluster_cnt;
		const bool shrink = std::get<0>(counts) < file.cluster_cnt;

		u16 err, old_cls_cnt;
		std::vector<u16> chain;

		if(grow)
		{
			err = FAT_utils::follow_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, file.start_cluster, chain);
			if(err && err != ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::BAD_START))
				return err;

			old_cls_cnt = chain.size();

			err = FAT_utils::find_free_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, std::get<0>(counts), chain);
			if(err)
			{
				if(err == ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::NO_FREE_CLUSTERS))
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);

				return err;
			}

			file.start_cluster = chain[0];
		}
		else if(shrink)
		{
			err = FAT_utils::follow_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, file.start_cluster, chain);
			if(err) return err;

			old_cls_cnt = chain.size();

			err = FAT_utils::shrink_chain(mount.stream, FAT_ATTRS, mount.FAT_attrs, chain, std::get<0>(counts));
			if(err) return err;

			err = FAT_utils::shrink_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, chain, std::get<0>(counts));
			if(err) return err;

			file.start_cluster = chain.size() ? chain[0] : FAT_ATTRS.END_OF_CHAIN;
			mount.free_clusters += old_cls_cnt - std::get<0>(counts);
		}

		file.cluster_cnt = std::get<0>(counts);
		file.block_cnt = std::get<1>(counts);
		file.byte_cnt = std::get<2>(counts);

		err = write_file(mount.stream, file);
		if(err) return err;

		if(grow)
		{
			err = FAT_utils::write_chain(mount.stream, FAT_ATTRS, mount.FAT_attrs, chain);
			if(err) return err;

			err = FAT_utils::write_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, chain);
			if(err) return err;

			mount.free_clusters -= std::get<0>(counts) - old_cls_cnt;
		}

		return 0;
	}

	static u16 remove_file(filesystem_t &mount, File_t &file)
	{
		u16 err;
		std::vector<u16> chain;

		if(file.cluster_cnt)
		{
			err = FAT_utils::follow_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, file.start_cluster, chain);
			if(err) return err;

			err = FAT_utils::free_chain(mount.stream, FAT_ATTRS, mount.FAT_attrs, chain);
			if(err) return err;

			err = FAT_utils::free_chain(mount.FAT.get(), FAT_ATTRS, mount.FAT_attrs.LENGTH, chain);
			if(err) return err;
		}

		mount.free_clusters += chain.size();

		mount.stream.seekp(file.addr + 0x1A);
		mount.stream.put(0);
		return 0;
	}

	template<bool recurse>
	static u16 remove_dir(filesystem_t &mount, const Dir_t &dir)
	{
		const u16 END_OF_FILE_LIST = mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt;

		bool del_failed;
		u8 temp[BLK_SIZE];
		u16 offset, err;
		uintmax_t block_abs_addr;

		filesystem_t::file_map_iterator_t fmap_it;
		File_t file;

		if constexpr(recurse)
		{
			del_failed = false;
			err = 0;
		}

		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] < mount.header.file_list_blk_addr ||
				dir.blocks[i] >= mount.header.file_list_blk_addr + mount.header.file_list_blk_cnt)
				continue;

			block_abs_addr = dir.blocks[i] * BLK_SIZE;

			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < FILES_PER_BLOCK; i++, offset += On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((File_type_e)temp[offset + 0x1A]))
					continue;

				if constexpr(recurse)
				{
					fmap_it = mount.open_files.find(std::string(dir.name) + "/" + std::to_string(temp[offset + 0x11]));

					if(fmap_it != mount.open_files.end())
					{
						del_failed = true;
						continue;
					}

					file.addr = block_abs_addr + offset;
					load_file<true>(temp + offset, file);

					err |= remove_file(mount, file);
				}
				else return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_EMPTY);
			}
		}

		if constexpr(recurse)
		{
			if(del_failed)
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

			if(err) return err;
		}

		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] >= mount.header.file_list_blk_addr && dir.blocks[i] < END_OF_FILE_LIST)
				mount.dir_content_block_map[dir.blocks[i] - mount.header.file_list_blk_addr] = false;
		}

		mount.stream.seekp(dir.addr + 0x11);
		mount.stream.put((char)Dir_type_e::DEL);

		if(!mount.stream.is_open() || !mount.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	static u16 rename_dir(filesystem_t &mount, const char *src_name, const char *dst_name)
	{
		/*Run a single pass trying to find and load old dir by name and trying
		to find if dir with dst_name exists. If nonexistant, load cur dir and
		rename.*/

		bool found_src, found_dst;
		u8 temp[BLK_SIZE];
		u16 offset;
		uintmax_t block_abs_addr;

		Dir_t dir, dst_dir;

		found_src = false;
		found_dst = false;
		block_abs_addr = mount.header.dir_list_blk_addr * BLK_SIZE;
		for(u32 block = 0; block < mount.header.dir_list_blk_cnt; block++, block_abs_addr += BLK_SIZE)
		{
			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < DIRS_PER_BLOCK; i++, offset += On_disk_sizes::DIR_ENTRY)
			{
				if(!is_valid_dir((Dir_type_e)temp[offset + 0x11]))
					continue;

				if(!found_src && !std::strncmp((char*)(temp + offset), src_name, 16))
				{
					dir.addr = block_abs_addr + offset;
					load_dir<true>(temp + offset, dir);
					found_src = true;
				}
				else if(!std::strncmp((char*)(temp + offset), dst_name, 16))
				{
					dst_dir.addr = block_abs_addr + offset;
					load_dir<true>(temp + offset, dst_dir);
					
					if(remove_dir<false>(mount, dst_dir))
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_EXISTS);

					found_dst = true;
				}
			}

			if(found_src && found_dst) break;
		}

		if(!found_src)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

		prepare_dir_name(dst_name, dir.name);
		write_dir(temp, dir);
		mount.stream.seekp(dir.addr);
		mount.stream.write((char*)temp, On_disk_sizes::DIR_ENTRY);

		if(!mount.stream.is_open() || !mount.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	static u16 move_file(filesystem_t &mount, const File_t &src_file, Dir_t &dst_dir, const std::string &dst_fname)
	{
		u16 dst_bank_num, new_block, err;
		std::string cleaned_name;
		File_t dst_file;

		dst_bank_num = bank_num_and_name_from_name(dst_fname, cleaned_name);
		if(dst_bank_num >= 0x100) dst_bank_num = src_file.bank_num;

		if(mount.open_files.contains(std::string(dst_dir.name) + "/" + std::to_string(dst_bank_num)))
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

		err = find_file_or_free(mount, dst_dir, dst_bank_num, dst_file);
		if(err)
		{
			if(err != ret_val_setup(LIBRARY_ID, (u8)ERR::TRY_GROW_DIR))
				return err;

			err = extend_dir(mount, dst_dir, new_block);
			if(err) return err;

			dst_file.addr = new_block * BLK_SIZE;
		}
		else if(dst_file.type != File_type_e::DEL)
		{
			err = remove_file(mount, dst_file);
			if(err) return err;
		}

		dst_file.bank_num = dst_bank_num;

		if(cleaned_name.size() && cleaned_name[0])
		{
			std::memcpy(dst_file.name, cleaned_name.data(), 16);
			dst_file.name[16] = 0;
		}
		else std::memcpy(dst_file.name, src_file.name, 17);

		dst_file.start_cluster = src_file.start_cluster;
		dst_file.cluster_cnt = src_file.cluster_cnt;
		dst_file.block_cnt = src_file.block_cnt;
		dst_file.byte_cnt = src_file.byte_cnt;
		dst_file.type = src_file.type;
		std::memcpy(dst_file.props, src_file.props, sizeof(File_t::props));

		err = write_file(mount.stream, dst_file);
		if(err) return err;

		mount.stream.seekp(src_file.addr + 0x1A);
		mount.stream.put((u8)File_type_e::DEL);

		return 0;
	}

	//Controls only src comp. Dst always by block num.
	template <const comp_e COMP>
	static u16 load_src_and_dst(filesystem_t &mount, const Dir_t &dir,
								File_t &src_file, File_t &dst_file)
	{
		bool found_src, found_dst;
		u8 temp[BLK_SIZE];
		u16 offset, err;
		uintmax_t block_abs_addr;

		filesystem_t::file_map_iterator_t fmap_it;

		found_src = false;
		found_dst = false;
		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++)
		{
			if(dir.blocks[i] < mount.header.file_list_blk_addr ||
				dir.blocks[i] >= mount.header.file_list_blk_addr
					+ mount.header.file_list_blk_cnt)
				continue;

			block_abs_addr = dir.blocks[i] * BLK_SIZE;

			mount.stream.seekg(block_abs_addr);
			mount.stream.read((char*)temp, BLK_SIZE);
			if(mount.stream.fail()) return ret_val_setup(min_vfs::LIBRARY_ID,
				(u8)min_vfs::ERR::IO_ERROR);

			offset = 0;
			for(u8 i = 0; i < FILES_PER_BLOCK; i++, offset
				+= On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((File_type_e)temp[offset + 0x1A]))
					continue;

				if constexpr(COMP == comp_e::NAME)
				{
					std::replace(temp + offset, temp + offset + 16, '/', '\\');

					if(!found_src && !std::strncmp((char*)(temp + offset),
						src_file.name, 16))
					{
						src_file.addr = block_abs_addr + offset;
						load_file<true>(temp + offset, src_file);
						found_src = true;
					}
				}
				else if constexpr(COMP == comp_e::BANK)
				{
					if(!found_dst && temp[offset + 0x11] == src_file.bank_num)
					{
						src_file.addr = block_abs_addr + offset;
						load_file<true>(temp + offset, src_file);
						found_src = true;
					}
				}

				if(!found_dst && temp[offset + 0x11] == dst_file.bank_num)
				{
					dst_file.addr = block_abs_addr + offset;
					load_file<true>(temp + offset, dst_file);
					found_dst = true;
				}
			}

			if(found_src && found_dst) break;
		}

		err = 0;

		if(!found_src)
		{
			src_file.type = File_type_e::DEL;
			err = ret_val_setup(min_vfs::LIBRARY_ID,
								(u8)min_vfs::ERR::NOT_FOUND);
		}

		if(!found_dst)
		{
			dst_file.type = File_type_e::DEL;
			err = ret_val_setup(min_vfs::LIBRARY_ID,
								(u8)min_vfs::ERR::NOT_FOUND);
		}

		return err;
	}

	static u16 rename_file(filesystem_t &mount, const Dir_t &dir,
						   const std::string &src_fname,
						   const std::string &dst_fname)
	{
		u16 src_bank_num, dst_bank_num, err;
		std::string src_fname_cln, dst_fname_cln;

		filesystem_t::file_map_iterator_t fmap_it;
		File_t *fptr;
		std::vector<File_t> files;

		src_bank_num = bank_num_and_name_from_name(src_fname, src_fname_cln);
		dst_bank_num = bank_num_and_name_from_name(dst_fname, dst_fname_cln);

		/*We only need worry about the dst if dst_bank_num is in range and !=
		 *src_bank_num, or if we have a valid dst_bank_num and no valid
		 *src_bank_num. Otherwise, just rename.*/
		if(dst_bank_num < 0x100 && src_bank_num != dst_bank_num)
		{
			fmap_it = mount.open_files.find(std::string(dir.name) + "/"
				+ std::to_string(dst_bank_num));

			if(fmap_it != mount.open_files.end())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::ALREADY_OPEN);

			files.resize(2);

			files[1].bank_num = dst_bank_num;

			if(src_bank_num < 0x100)
			{
				fmap_it = mount.open_files.find(std::string(dir.name) + "/"
					+ std::to_string(src_bank_num));

				if(fmap_it != mount.open_files.end())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::ALREADY_OPEN);

				files[0].bank_num = src_bank_num;
				files[1].bank_num = dst_bank_num;
				err = load_src_and_dst<comp_e::BANK>(mount, dir, files[0],
													 files[1]);

				if(files[0].type == File_type_e::DEL)
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NOT_FOUND);
			}
			else
			{
				std::memcpy(files[0].name, src_fname_cln.data(), 16);
				files[0].name[16] = 0;
				files[1].bank_num = dst_bank_num;
				err = load_src_and_dst<comp_e::NAME>(mount, dir, files[0],
													 files[1]);

				if(files[0].type == File_type_e::DEL)
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NOT_FOUND);

				fmap_it = mount.open_files.find(std::string(dir.name) + "/"
					+ std::to_string(files[0].bank_num));

				if(fmap_it != mount.open_files.end())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::ALREADY_OPEN);
			}

			if(files[1].type != File_type_e::DEL && files[0].bank_num
				!= dst_bank_num)
			{
				if(remove_file(mount, files[1]))
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::ALREADY_EXISTS);
			}

			files[0].bank_num = dst_bank_num;
		}
		else
		{
			if(src_bank_num < 0x100)
			{
				fmap_it = mount.open_files.find(std::string(dir.name) + "/"
					+ std::to_string(src_bank_num));

				if(fmap_it != mount.open_files.end())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::ALREADY_OPEN);

				err = load_file_in_dir<comp_e::BANK>(mount, dir,
										{.bank_num = (u8)src_bank_num}, files);
				if(err) return err;
			}
			else
			{
				err = load_file_in_dir<comp_e::NAME>(mount, dir,
										{.name = src_fname_cln.c_str()}, files);
				if(err) return err;

				fmap_it = mount.open_files.find(std::string(dir.name) + "/"
					+ std::to_string(files[0].bank_num));

				if(fmap_it != mount.open_files.end())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::ALREADY_OPEN);
			}
		}

		if(dst_fname_cln.size() && dst_fname_cln[0])
			std::memcpy(files[0].name, dst_fname_cln.data(), 16);

		write_file(mount.stream, files[0]);

		return 0;
	}

	static uint16_t trunc_open_common(filesystem_t &mount, Dir_t &dir,
									  const u16 bank_num, File_t &file,
									filesystem_t::file_map_iterator_t &fmap_it)
	{
		u16 err, new_block;

		if(bank_num < 0x100)
		{
			fmap_it = mount.open_files.find(std::string(dir.name) + "/"
				+ std::to_string(bank_num));

			if(fmap_it != mount.open_files.end())
				return ret_val_setup(LIBRARY_ID, (u8)ERR::FOUND_IN_MAP);

			err = find_file_or_free(mount, dir, bank_num, file);
		}
		else
		{
			err = find_file_or_free(mount, dir, file.name, file);

			if(file.type != File_type_e::DEL)
			{
				fmap_it = mount.open_files.find(std::string(dir.name) + "/"
					+ std::to_string(file.bank_num));

				if(fmap_it != mount.open_files.end())
					return ret_val_setup(LIBRARY_ID, (u8)ERR::FOUND_IN_MAP);
			}
		}

		if(err)
		{
			if(err != ret_val_setup(LIBRARY_ID, (u8)ERR::TRY_GROW_DIR))
				return err;

			err = extend_dir(mount, dir, new_block);
			if(err) return err;

			file.addr = new_block * BLK_SIZE;
		}

		return 0;
	}

	static uint16_t get_or_alloc_next_cls(filesystem_t &mount,
										  const u16 cur_cls, File_t &file)
	{
		u16 next_cls, err;

		err = FAT_utils::get_next_or_free_cluster(mount.FAT.get(), FAT_ATTRS,
												  mount.FAT_attrs.LENGTH,
											cur_cls, next_cls);
		if(err)
		{
			if(err == ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::ALLOC))
			{
				if(next_cls != FAT_ATTRS.END_OF_CHAIN)
				{
					if(!file.cluster_cnt)
						file.start_cluster = next_cls;

					file.cluster_cnt++;
					file.block_cnt = 0;
					file.byte_cnt = 0;

					write_file(mount.stream, file);
					if(!mount.stream.good())
						throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

					err = FAT_utils::extend_chain(mount.FAT.get(), mount.stream,
												  FAT_ATTRS, mount.FAT_attrs,
												  cur_cls, next_cls);
					if(err)
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::IO_ERROR);

					mount.free_clusters--;
				}
			}
			else throw min_vfs::FS_err(err);
		}

		return next_cls;
	}

	static uint16_t get_or_alloc_nth_cls(filesystem_t &fs, u16 &cls,
										 const u16 start_cls_idx,
										 const uintmax_t len,
										 internal_file_t &internal_file,
										 const uintmax_t pos)
	{
		u16 err;

		err = FAT_utils::get_nth_cluster(fs.FAT.get(), FAT_ATTRS,
										 fs.FAT_attrs.LENGTH, cls,
										 start_cls_idx);

		if(err)
		{
			/*This avoids erroniously growing the file on 0-length writes.
			 *Should this maybe be added to min_vfs' stream_t's write instead?
			 *Would the extra overhead from the branch matter at all? Would the
			 *potential saved runtime on 0-length writes matter? I guess the
			 *best reason to add it there is to make it so drivers don't have to
			 *worry about 0-length writes.*/
			if(!len) return 0;

			if(err == ret_val_setup(FAT_utils::LIBRARY_ID,
									(u8)FAT_utils::ERR::CHAIN_OOB))
				return err;
			else
			{
				err = resize_file(fs, internal_file.file_entry, pos + 1);
				if(err) return err;

				cls = internal_file.file_entry.start_cluster;
				err = FAT_utils::get_nth_cluster(fs.FAT.get(), FAT_ATTRS,
												 fs.FAT_attrs.LENGTH, cls,
												 start_cls_idx);
				if(err) return err;
			}
		}

		return 0;
	}

	template <const bool write>
	static uint16_t read_write_file(filesystem_t &mount,
									internal_file_t &internal_file,
									uintmax_t &pos, uintmax_t len, void *dst)
	{
		uintmax_t dst_off;

		const u32 cluster_size = calc_cluster_size(mount.header.cluster_shift);
		const uintmax_t max_file_size = mount.header.cluster_cnt * cluster_size;

		if(pos < max_file_size)
		{
			const u32 file_size = calc_file_size(internal_file.file_entry,
												 cluster_size);
			const u32 remaining = write ? (max_file_size > pos ? max_file_size
				- pos : 0) : (file_size > pos ? file_size - pos : 0);
			const u32 local_len = std::min((uintmax_t)remaining, len);
			const u16 start_cls_idx = pos / cluster_size;
			const u32 pos_in_first_cls = pos - start_cls_idx * cluster_size;
			const u32 first_cls_len = std::min((u32)(cluster_size
				- pos_in_first_cls), local_len);
			const u16 whole_cls = (local_len - first_cls_len) / cluster_size;

			u16 err, cls = internal_file.file_entry.start_cluster;

			if constexpr(write)
			{
				mount.mtx.lock();
				err = get_or_alloc_nth_cls(mount, cls, start_cls_idx, len,
										   internal_file, pos);
				mount.mtx.unlock();

				if(err) return err;
			}
			else
			{
				err = FAT_utils::get_nth_cluster(mount.FAT.get(), FAT_ATTRS,
												 mount.FAT_attrs.LENGTH, cls,
												 start_cls_idx);

				if(err)
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
			}

			const uintmax_t DATA_ADDR = mount.header.data_sctn_blk_addr * BLK_SIZE;

			dst_off = 0;

			mount.mtx.lock();
			mount.stream.seekg(DATA_ADDR + cluster_size * (cls
				- FAT_ATTRS.DATA_MIN) + pos_in_first_cls);

			if constexpr(write) mount.stream.write((char*)dst + dst_off,
				first_cls_len);
			else mount.stream.read((char*)dst + dst_off, first_cls_len);
			mount.mtx.unlock();

			if(!mount.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);

			len -= first_cls_len;
			pos += first_cls_len;
			dst_off += first_cls_len;

			for(u16 i = 0; i < whole_cls; i++)
			{
				if constexpr(write)
				{
					try
					{
						mount.mtx.lock();
						cls = get_or_alloc_next_cls(mount, cls,
													internal_file.file_entry);
						mount.mtx.unlock();
					}
					catch(min_vfs::FS_err e)
					{
						mount.mtx.unlock();
						return e.err_code;
					}
				}
				else cls = mount.FAT[cls];

				if(cls < FAT_ATTRS.DATA_MIN || cls > FAT_ATTRS.DATA_MAX)
				{
					if constexpr(write)
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::NO_SPACE_LEFT);
					else
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::END_OF_FILE);
				}

				mount.mtx.lock();
				mount.stream.seekg(DATA_ADDR + cluster_size * (cls
					- FAT_ATTRS.DATA_MIN));

				if constexpr(write) mount.stream.write((char*)dst + dst_off,
					cluster_size);
				else mount.stream.read((char*)dst + dst_off, cluster_size);
				mount.mtx.unlock();

				if(!mount.stream.good())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::IO_ERROR);

				len -= cluster_size;
				pos += cluster_size;
				dst_off += cluster_size;
			}

			if(len)
			{
				if constexpr(write)
				{
					try
					{
						mount.mtx.lock();
						cls = get_or_alloc_next_cls(mount, cls,
													internal_file.file_entry);
						mount.mtx.unlock();
					}
					catch(min_vfs::FS_err e)
					{
						mount.mtx.unlock();
						return e.err_code;
					}
				}
				else cls = mount.FAT[cls];

				if(cls < FAT_ATTRS.DATA_MIN || cls > FAT_ATTRS.DATA_MAX)
				{
					if constexpr(write)
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::NO_SPACE_LEFT);
					else
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::END_OF_FILE);
				}

				mount.mtx.lock();
				mount.stream.seekg(DATA_ADDR + cluster_size * (cls
					- FAT_ATTRS.DATA_MIN));

				if constexpr(write) mount.stream.write((char*)dst + dst_off,
					len);
				else mount.stream.read((char*)dst + dst_off, len);
				mount.mtx.unlock();

				if(!mount.stream.good())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::IO_ERROR);

				pos += len;
				len = 0;
			}

			/*Might wanna move this to the outer write function (filesystem_t's
			write function) so that we always update the file size, regardless
			of how we return from this one.*/
			if constexpr(write)
			{
				const uintmax_t current_file_size =
					calc_file_size(internal_file.file_entry, cluster_size);

				if(pos > current_file_size)
				{
					const std::tuple<u16, u16, u16> counts =
						file_size_to_counts(cluster_size, pos);

					internal_file.file_entry.cluster_cnt = std::get<0>(counts);
					internal_file.file_entry.block_cnt = std::get<1>(counts);
					internal_file.file_entry.byte_cnt = std::get<2>(counts);

					mount.mtx.lock();
					write_file(mount.stream, internal_file.file_entry);
					mount.mtx.unlock();

					if(!mount.stream.good())
						return ret_val_setup(min_vfs::LIBRARY_ID,
											 (u8)min_vfs::ERR::IO_ERROR);
				}
			}
			
			if(!len) return 0;
		}

		if constexpr(write)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FILE_TOO_LARGE);
		else
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::END_OF_FILE);
	}

	filesystem_t::filesystem_t(const char *path): min_vfs::filesystem_t(path)
	{
		std::unique_ptr<u8[]> data;
		u16 err, sum, cur;
		size_t dir_cnt_dir_blcks;
		
		const uintmax_t blk_count = std::filesystem::file_size(path) / BLK_SIZE;

		//Initial size check
		if(!blk_count)
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
											(u8)min_vfs::ERR::DISK_TOO_SMALL));

		data = std::make_unique<u8[]>(BLK_SIZE);
		stream.read((char*)data.get(), BLK_SIZE);

		if(!stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

		//Magic check
		if(std::memcmp(data.get(), MAGIC, 4))
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::WRONG_FS));

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
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
											(u8)min_vfs::ERR::INVALID_STATE));

		load_header(data.get(), header);
		data.reset();

		err = check_header(header);
		if(err) throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::INVALID_STATE));

		//Final size check
		if(blk_count < header.block_cnt)
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
											(u8)min_vfs::ERR::DISK_TOO_SMALL));

		stream.seekg(BLK_SIZE);
		stream.read((char*)&next_file_list_blk, 2);
		if constexpr(ENDIANNESS != std::endian::native)
			next_file_list_blk = std::byteswap(next_file_list_blk);

		dir_cnt_dir_blcks = header.dir_list_blk_cnt * DIRS_PER_BLOCK
			* MAX_BLOCKS_PER_DIR;
		dir_content_block_map.resize(std::min((size_t)header.file_list_blk_cnt,
											  dir_cnt_dir_blcks));
		map_dir_blocks(*this, dir_content_block_map);

		FAT_attrs.BASE_ADDR = header.FAT_blk_addr * BLK_SIZE;
		FAT_attrs.LENGTH = header.cluster_cnt + 1;

		stream.seekg(FAT_attrs.BASE_ADDR);
		FAT = std::make_unique<u16[]>(FAT_attrs.LENGTH);
		stream.read((char*)FAT.get(), FAT_attrs.LENGTH * 2);

		if constexpr(ENDIANNESS != std::endian::native)
		{
			for(u32 i = 0; i < header.cluster_cnt + 1; i++)
				FAT[i] = std::byteswap(FAT[i]);
		}

		free_clusters = FAT_utils::count_free_clusters(FAT.get(), FAT_ATTRS,
													   FAT_attrs.LENGTH);

		if(!stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));
	}

	filesystem_t &filesystem_t::operator=(filesystem_t &&other) noexcept
	{
		if(this == &other) return *this;

		path = other.path;
		stream.swap(other.stream);

		header = other.header;
		next_file_list_blk = other.next_file_list_blk;
		dir_content_block_map = other.dir_content_block_map;
		FAT_attrs = other.FAT_attrs;
		FAT = std::move(other.FAT);
		free_clusters = other.free_clusters;

		/*See comment in S7XX driver's implementation of this.*/
		open_files = other.open_files;

		return *this;
	}

	min_vfs::filesystem_t &filesystem_t::operator=(min_vfs::filesystem_t &&other) noexcept
	{
		filesystem_t *other_ptr;

		other_ptr = dynamic_cast<filesystem_t*>(&other);

		if(!other_ptr) return *this;

		return *this = std::move(*other_ptr);
	}

	uintmax_t filesystem_t::get_free_space()
	{
		return free_clusters * calc_cluster_size(header.cluster_shift);
	}

	std::string filesystem_t::get_type_name()
	{
		return FS_NAME;
	}

	uintmax_t filesystem_t::get_open_file_count()
	{
		return open_files.size();
	}

	bool filesystem_t::can_unmount()
	{
		return open_files.empty();
	}

	u16 filesystem_t::list(const char *file_path,
						   std::vector<min_vfs::dentry_t> &dentries,
						   const bool get_dir)
	{
		u16 err;

		std::vector<std::string> split_path;
		std::vector<Dir_t> dirs;
		std::vector<File_t> files;

		split_path = str_util::split(std::string(file_path), std::string("/"));

		switch(split_path.size())
		{
			case 0:
				if(get_dir)
				{
					dentries.emplace_back
					(
						"/",
						header.dir_list_blk_cnt * BLK_SIZE,
						0, //ctime
						0, //mtime
						0, //atime
						min_vfs::ftype_t::dir
					);
					return 0;
				}

				err = load_dir_from_name<false>(*this, "", dirs);
				if(err) return err;

				dentries.resize(dirs.size());

				for(u32 i = 0; i < dentries.size(); i++)
					dir_to_dentry(header.file_list_blk_addr,
								  header.file_list_blk_cnt, dirs[i],
								  dentries[i]);

				return 0;

			case 1:
				err = load_dir_from_name<true>(*this, split_path[0].c_str(),
											   dirs);
				if(err) return err;

				if(get_dir)
				{
					dentries.resize(1);
					dir_to_dentry(header.file_list_blk_addr,
								  header.file_list_blk_cnt, dirs[0],
								  dentries[0]);
					return 0;
				}

				err = load_file_in_dir<comp_e::NONE>(*this, dirs[0], {0},
													 files);
				if(err) return err;

				dentries.resize(files.size());
				
				{
					const u32 cluster_size =
						calc_cluster_size(header.cluster_shift);

					for(u8 i = 0; i < files.size(); i++)
						file_to_dentry(cluster_size, files[i], dentries[i]);
				}

				return 0;

			case 2:
				err = load_dir_from_name<true>(*this, split_path[0].c_str(),
											   dirs);
				if(err) return err;

				{
					std::string fname;
					u16 bank_num;

					bank_num = bank_num_and_name_from_name(split_path[1],
														   fname);

					if(bank_num < 0x100)
						err = load_file_in_dir<comp_e::BANK>(*this, dirs[0],
											{.bank_num = (u8)bank_num}, files);
					else
						err = load_file_in_dir<comp_e::NAME>(*this, dirs[0],
												{.name = fname.c_str()}, files);
				}
				if(err) return err;

				dentries.resize(1);
				
				file_to_dentry(calc_cluster_size(header.cluster_shift),
							   files[0], dentries[0]);

				return 0;

			default:
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::INVALID_PATH);
		}

		return 0;
	}

	uint16_t filesystem_t::mkdir(const char *dir_path)
	{
		bool found_free, found_dir;
		u16 offset;
		uintmax_t block_abs_addr, free_dir_slot_addr;

		std::unique_ptr<u8[]> buffer;
		std::vector<std::string> split_path;

		Dir_t dir;

		split_path = str_util::split(std::string(dir_path), std::string("/"));

		if(split_path.size() != 1)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

		const char *search_name = split_path[0].c_str();

		found_dir = false;
		found_free = false;
		buffer = std::make_unique<u8[]>(BLK_SIZE);
		for(block_abs_addr = header.dir_list_blk_addr * BLK_SIZE;
			block_abs_addr < header.dir_list_blk_addr * BLK_SIZE
				+ header.dir_list_blk_cnt * BLK_SIZE;
			block_abs_addr += BLK_SIZE)
		{
			stream.seekg(block_abs_addr);
			stream.read((char*)buffer.get(), BLK_SIZE);

			offset = 0;
			for(u8 i = 0; i < DIRS_PER_BLOCK; i++, offset
				+= On_disk_sizes::DIR_ENTRY)
			{
				if(!is_valid_dir((Dir_type_e)buffer[offset + 0x11]))
				{
					if(!found_free)
					{
						free_dir_slot_addr = block_abs_addr + offset;
						found_free = true;
					}

					continue;
				}

				std::replace(buffer.get() + offset, buffer.get() + offset + 16,
							 '/', '\\');

				if(!std::strncmp((char*)(buffer.get() + offset), search_name,
					16))
				{
					dir.addr = block_abs_addr + offset;
					load_dir<true>(buffer.get() + offset, dir);
					found_dir = true;
					break;
				}
			}
		}

		if(found_dir)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_EXISTS);

		if(!found_free)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);

		prepare_dir_name(split_path[0], dir.name);
		dir.type = Dir_type_e::NORMAL;
		for(u8 i = 0; i < MAX_BLOCKS_PER_DIR; i++) dir.blocks[i] = 0xFFFF;

		write_dir(buffer.get(), dir);
		stream.seekp(free_dir_slot_addr);
		stream.write((char*)buffer.get(), On_disk_sizes::DIR_ENTRY);
		stream.flush();
		if(!stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	uint16_t filesystem_t::ftruncate(const char *path, const uintmax_t new_size)
	{
		u16 err, bank_num;

		std::string cleaned_name;
		std::vector<std::string> split_path;

		File_t file, *fptr;
		std::vector<Dir_t> dirs;
		file_map_iterator_t fmap_it;

		if(new_size > header.cluster_cnt * calc_cluster_size(header.cluster_shift))
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::FILE_TOO_LARGE);

		split_path = str_util::split(std::string(path), std::string("/"));

		if(split_path.size() != 2)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

		err = load_dir_from_name<true>(*this, split_path[0].c_str(), dirs);
		if(err) return err;

		bank_num = bank_num_and_name_from_name(split_path[1], cleaned_name);

		std::memcpy(file.name, cleaned_name.data(), 16);
		file.name[16] = 0;

		err = trunc_open_common(*this, dirs[0], bank_num, file, fmap_it);
		if(err)
		{
			if(err != ret_val_setup(LIBRARY_ID, (u8)ERR::FOUND_IN_MAP))
				return err;

			fptr = &fmap_it->second.second.file_entry;
		}
		else fptr = &file;

		fptr->type = File_type_e::STD;

		err = resize_file(*this, *fptr, new_size);
		if(err) return err;

		stream.flush();

		if(!stream.is_open() || !stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	/*Both POSIX's rename() and Linux's VFS' rename implement the move behavior.
	According to POSIX, rename() should be able to rename/move open files. This
	would complicate things somewhat and we do not support it for now.
	
	I think it should be feasible, but I tried making the open files map store
	std::unique_ptrs and MSVC wasn't having it (didn't test g++). I don't wanna
	resort to std::shared_ptrs for this; I want the map to be the sole owner of
	the data, I just need to be able to move the map's contents around (to
	replace the key when the path changes) while keeping the actual data in the
	same place so pointers to it remain valid.
	
	I don't know if an std::flat_map would really be an option (whenever that
	gets support). I guess I could store the actual data in a list and just
	store pointers to it in the map. Unsure if I'd wanna keep the refcount in
	the map or the list.*/
	uint16_t filesystem_t::rename(const char *cur_path, const char *new_path)
	{
		u16 err, src_bank_num, dst_bank_num, new_block;

		std::string src_cleaned_name, dst_cleaned_name;
		std::vector<std::string> split_src_path, split_dst_path;

		File_t *fptr;
		std::vector<Dir_t> dirs;
		std::vector<File_t> files;
		file_map_iterator_t fmap_it;

		split_src_path = str_util::split(std::string(cur_path), std::string("/"));

		switch(split_src_path.size())
		{
			case 0:
				//Should never happen. min_vfs should rename mount dir/img file.
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);

			case 1:
				//Source is dir. Only rename is possible.

				split_dst_path = str_util::split(std::string(new_path), std::string("/"));

				//Move dir to root (pointless).
				switch(split_dst_path.size())
				{
					case 0:
						return 0;

					case 1:
						break;

					case 2:
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);

					default:
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
				}

				fmap_it = open_files.find(split_src_path[0]);
				if(fmap_it != open_files.end())
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

				err = rename_dir(*this, split_src_path[0].c_str(), split_dst_path[0].c_str());
				stream.flush();

				return err;

			case 2:
				/*Source is file. Both rename and move are possible. This is a
				 *mess because we have to determine both whether we're moving or
				 *renaming and whether we're gonna be using filenames or bank
				 *numbers.*/

				split_dst_path = str_util::split(std::string(new_path), std::string("/"));

				if(!split_dst_path.size())
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
				else if(split_dst_path.size() > 2)
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

				//Rename
				if(split_dst_path[0] == split_src_path[0])
				{
					//Rename without new name, so just return.
					if(split_dst_path.size() != 2) return 0;

					err = load_dir_from_name<true>(*this, split_src_path[0].c_str(), dirs);
					if(err) return err;

					return rename_file(*this, dirs[0], split_src_path[1], split_dst_path[1]);
				}

				/*TODO: Consider modifying load_dir_from_name to take an array
				 *of strings for dirnames so we can load both in a single pass.
				 *I'd probably add another template flag to control whether name
				 *checking is done in a loop, although that might be excessive
				 *since IO should be the bottleneck anyway. That flag could also
				 *just be an enum instead of using 2 flags.*/
				err = load_dir_from_name<true>(*this, split_src_path[0].c_str(), dirs);
				if(err) return err;

				err = load_dir_from_name<true>(*this, split_dst_path[0].c_str(), dirs);
				if(err) return err;

				src_bank_num = bank_num_and_name_from_name(split_src_path[1], src_cleaned_name);

				if(src_bank_num < 0x100)
				{
					fmap_it = open_files.find(split_src_path[0] + "/" + std::to_string(src_bank_num));

					if(fmap_it != open_files.end())
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

					err = load_file_in_dir<comp_e::BANK>(*this, dirs[0], {.bank_num = (u8)src_bank_num}, files);
					if(err) return err;
				}
				else
				{
					err = load_file_in_dir<comp_e::NAME>(*this, dirs[0], {.name = src_cleaned_name.c_str()}, files);
					if(err) return err;

					fmap_it = open_files.find(split_src_path[0] + "/" + std::to_string(files[0].bank_num));

					if(fmap_it != open_files.end())
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
				}

				if(split_dst_path.size() < 2)
					err = move_file(*this, files[0], dirs[1], "");
				else
					err = move_file(*this, files[0], dirs[1], split_dst_path[1]);

				stream.flush();
				return err;

			default:
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
		}

		return 0;
	}

	u16 filesystem_t::remove(const char *file_path)
	{
		u16 err, bank_num;

		std::string cleaned_name;
		std::vector<std::string> split_path;

		std::vector<Dir_t> dirs;
		std::vector<File_t> files;
		file_map_iterator_t fmap_it;

		split_path = str_util::split(std::string(file_path), std::string("/"));

		switch(split_path.size())
		{
			case 0:
				err = load_dir_from_name<false>(*this, "", dirs);
				if(err) return err;

				for(Dir_t &dir: dirs)
					err |= remove_dir<true>(*this, dir);
				break;

			case 1:
				err = load_dir_from_name<true>(*this, split_path[0].c_str(), dirs);
				if(err) return err;

				err = remove_dir<true>(*this, dirs[0]);
				break;

			case 2:
				err = load_dir_from_name<true>(*this, split_path[0].c_str(), dirs);
				if(err) return err;

				bank_num = bank_num_and_name_from_name(split_path[1], cleaned_name);

				if(bank_num < 0x100)
				{
					fmap_it = open_files.find(std::string(dirs[0].name) + "/" + std::to_string(bank_num));

					if(fmap_it != open_files.end())
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

					err = load_file_in_dir<comp_e::BANK>(*this, dirs[0], {.bank_num = (u8)bank_num}, files);
					if(err) return err;
				}
				else
				{
					err = load_file_in_dir<comp_e::NAME>(*this, dirs[0], {.name = cleaned_name.c_str()}, files);
					if(err) return err;

					fmap_it = open_files.find(std::string(dirs[0].name) + "/" + std::to_string(files[0].bank_num));

					if(fmap_it != open_files.end())
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
				}

				err = remove_file(*this, files[0]);
				break;

			default:
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
		}

		stream.flush();
		if(err) return err;

		if(!stream.is_open() || !stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	uint16_t filesystem_t::fopen_internal(const char *path, void **internal_file)
	{
		u16 err, bank_num;
		std::string fname;

		file_map_iterator_t fmap_it;
		File_t file;
		std::vector<File_t> files;
		std::vector<Dir_t> dirs;

		std::pair<uintmax_t, internal_file_t> file_entry;
		std::pair<file_map_iterator_t, bool> map_entry;
		
		const std::vector<std::string> split_path = str_util::split(std::string(path), std::string("/"));

		if(split_path.size() != 2)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

		err = load_dir_from_name<true>(*this, split_path[0].c_str(), dirs);
		if(err) return err;

		bank_num = bank_num_and_name_from_name(split_path[1], fname);

		std::memcpy(file.name, fname.data(), 16);
		file.name[16] = 0;
		err = trunc_open_common(*this, dirs[0], bank_num, file, fmap_it);
		if(err)
		{
			if(err != ret_val_setup(LIBRARY_ID, (u8)ERR::FOUND_IN_MAP))
				return err;

			fmap_it->second.first++;
			*internal_file = &fmap_it->second.second;
			return 0;
		}

		file.type = File_type_e::STD;
		write_file(stream, file);
		stream.flush();

		if(!stream.is_open() || !stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		fmap_it = open_files.find(split_path[0]);

		if(fmap_it != open_files.end()) fmap_it->second.first++;
		else
		{
			file_entry.first = 1;
			file_entry.second.ftype = min_vfs::ftype_t::dir;
			file_entry.second.file_entry.addr = dirs[0].addr;

			map_entry = open_files.emplace(dirs[0].name, file_entry);

			if(!map_entry.second)
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);

			fmap_it = map_entry.first;
			fmap_it->second.second.map_entry = &(*map_entry.first);
		}

		file_entry.first = 1;
		file_entry.second.ftype = min_vfs::ftype_t::file;
		file_entry.second.file_entry = file;
		file_entry.second.dir_map_entry = &(*fmap_it);

		map_entry = open_files.emplace(std::string(dirs[0].name) + "/" + std::to_string(file.bank_num), file_entry);

		if(!map_entry.second)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);

		map_entry.first->second.second.map_entry = &(*map_entry.first);

		*internal_file = &map_entry.first->second.second;

		return 0;
	}

	uint16_t filesystem_t::fclose(void *internal_file)
	{
		internal_file_t *const cast_ptr = (internal_file_t*)internal_file;

		if(cast_ptr->map_entry->second.first == 1)
		{
			if(cast_ptr->ftype == min_vfs::ftype_t::file)
			{
				if(cast_ptr->dir_map_entry->second.first == 1)
					open_files.erase(cast_ptr->dir_map_entry->first);
				else cast_ptr->dir_map_entry->second.first--;
			}

			open_files.erase(cast_ptr->map_entry->first);
		}
		else cast_ptr->map_entry->second.first--;

		return 0;
	}

	uint16_t filesystem_t::read(void *internal_file, uintmax_t &pos, uintmax_t len, void *dst)
	{
		return read_write_file<false>(*this, *((internal_file_t*)internal_file), pos, len, dst);
	}

	uint16_t filesystem_t::write(void *internal_file, uintmax_t &pos, uintmax_t len, void *src)
	{
		return read_write_file<true>(*this, *((internal_file_t*)internal_file), pos, len, src);
	}

	uint16_t filesystem_t::flush(void *internal_file)
	{
		mtx.lock();
		stream.flush();
		const bool str_status = stream.good();
		mtx.unlock();

		if(str_status) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::IO_ERROR);
	}
}
