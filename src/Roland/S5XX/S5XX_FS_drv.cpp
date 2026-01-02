#include <cstdint>
#include <filesystem>
#include <cstring>
#include <string>
#include <vector>
#include <bit>
#include <algorithm>

#include "Utils/ints.hpp"
#include "Utils/str_util.hpp"
#include "Utils/utils.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "S5XX_FS_types.hpp"
#include "S5XX_FS_drv.hpp"

namespace S5XX::FS
{
	uint16_t mkfs(const std::filesystem::path &fs_path, const std::string &label)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID,
							 (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	static constexpr bool is_valid_dir(const File_type_e dir_type)
	{
		return dir_type == File_type_e::system_directory
			|| dir_type == File_type_e::sound_directory;
	}

	static constexpr bool is_valid_file(const File_type_e file_type)
	{
		return file_type == File_type_e::system_file
			|| file_type == File_type_e::area
			|| file_type == File_type_e::instrument_group
			|| file_type == File_type_e::map;
	}

	static void load_dir(const bool is_CD, const void *data, Dir_entry_t &dst)
	{
		std::memcpy(dst.name, data, 16);
		dst.name[16] = 0;
		std::memcpy(&dst.block_addr, (char*)data + 16, 4);
		std::memcpy(&dst.block_cnt, (char*)data + 20, 4);

		dst.type = (File_type_e)((char*)data)[24];

		if(is_CD)
			dst.file_cnt = ((char*)data)[26];

		if constexpr(ENDIANNESS != std::endian::native)
		{
			dst.block_addr = std::byteswap(dst.block_addr);
			dst.block_cnt = std::byteswap(dst.block_cnt);
			dst.file_cnt = std::byteswap(dst.file_cnt);
		}
	}

	static void load_file(const bool is_CD, const void *data, File_entry_t &dst)
	{
		std::memcpy(dst.name, data, 48);
		dst.name[48] = 0;

		for(u8 i = 0; i < 48; i++)
			if(dst.name[i] == '/') dst.name[i] = '\\';

		std::memcpy(&dst.block_addr, (char*)data + 48, 4);
		std::memcpy(&dst.block_cnt, (char*)data + 52, 4);

		dst.type = (File_type_e)((char*)data)[56];

		if(is_CD)
			dst.ins_grp_idx = ((char*)data)[57];

		if constexpr(ENDIANNESS != std::endian::native)
		{
			dst.block_addr = std::byteswap(dst.block_addr);
			dst.block_cnt = std::byteswap(dst.block_cnt);
		}
	}

	template <const bool all>
	static uint16_t load_from_root(filesystem_t &fs, const char *dir_name,
								   std::vector<Dir_entry_t> &dst)
	{
		const u8 dir_cnt = MAX_ROOT_DENTRIES - fs.FIRST_DIR_IDX;
		const u16 data_size = dir_cnt * On_disk_sizes::DIR_ENTRY;

		u16 dir_base;
		std::unique_ptr<u8[]> data;

		data = std::make_unique<u8[]>(data_size);
		dir_base = fs.FIRST_DIR_IDX * On_disk_sizes::DIR_ENTRY;

		fs.stream.seekg(32 + dir_base);
		fs.stream.read((char*)data.get(), data_size);
		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		dir_base = 0;
		for(u8 i = fs.FIRST_DIR_IDX; i < MAX_ROOT_DENTRIES; i++, dir_base += On_disk_sizes::DIR_ENTRY)
		{
			if(!is_valid_dir((File_type_e)data[dir_base + FILE_TYPE_OFFSET])
				&& !is_valid_file((File_type_e)data[dir_base + FILE_TYPE_OFFSET]))
				continue;

			if constexpr(!all)
			{
				if(std::strncmp(dir_name, (char*)&data[dir_base], 16))
					continue;
			}

			dst.emplace_back();
			load_dir(fs.FIRST_DIR_IDX == FIRST_CD_DIR, &data[dir_base], dst.back());

			if constexpr(!all) return 0;
		}

		if constexpr(all) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::NOT_FOUND);
	}

	template <const bool all>
	static uint16_t load_from_dir(filesystem_t &fs, const Dir_entry_t &dir,
								  std::string filename,
								  std::vector<File_entry_t> &dst)
	{
		u16 entry_base;
		uintmax_t base_addr;
		std::unique_ptr<u8[]> data;

		data = std::make_unique<u8[]>(BLOCK_SIZE);
		base_addr = dir.block_addr * BLOCK_SIZE;

		if constexpr(!all)
			std::replace(filename.begin(), filename.end(), '\\', '/');

		const char *on_disk_fname = filename.c_str();

		for(u32 i = 0; i < dir.block_cnt; i++, base_addr += BLOCK_SIZE)
		{
			fs.stream.seekg(base_addr);
			fs.stream.read((char*)data.get(), BLOCK_SIZE);

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);

			entry_base = 0;
			for(u8 j = 0; j < FILES_PER_SECTOR; j++, entry_base += On_disk_sizes::FILE_ENTRY)
			{
				if(!is_valid_file((File_type_e)data[entry_base + FILE_TYPE_OFFSET + 32]))
					continue;

				if constexpr(!all)
				{
					if(std::strncmp(on_disk_fname, (char*)&data[entry_base], 48))
						continue;
				}

				dst.emplace_back();
				load_file(fs.FIRST_DIR_IDX == FIRST_CD_DIR, &data[entry_base],
						  dst.back());

				if constexpr(!all) return 0;
			}
		}

		if constexpr(all) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::NOT_FOUND);
	}

	static min_vfs::dentry_t dir_entry_to_dentry(const bool is_CD,
												 const Dir_entry_t &dir_entry)
	{
		min_vfs::dentry_t dentry;

		dentry.fname = dir_entry.name;
		dentry.ctime = 0;
		dentry.mtime = 0;
		dentry.atime = 0;

		switch(dir_entry.type)
		{
			using enum File_type_e;

			case instrument_group:
				if(is_CD)
					dentry.fsize = dir_entry.file_cnt * On_disk_sizes::INSTR_GRP;
				else
					dentry.fsize = dir_entry.block_cnt * BLOCK_SIZE;

				dentry.ftype = min_vfs::ftype_t::file;
				break;

			case map:
				if(is_CD) dentry.fsize = dir_entry.file_cnt * 2;
				else dentry.fsize = dir_entry.block_cnt * BLOCK_SIZE;

				dentry.ftype = min_vfs::ftype_t::file;
				break;

			case sound_directory:
			case system_directory:
				dentry.fsize = dir_entry.block_cnt * BLOCK_SIZE;
				dentry.ftype = min_vfs::ftype_t::dir;
				break;

			default:
				dentry.fsize = dir_entry.block_cnt * BLOCK_SIZE;
				dentry.ftype = min_vfs::ftype_t::file;
				break;
		}

		return dentry;
	}

	static min_vfs::dentry_t file_entry_to_dentry(const File_entry_t &file_entry)
	{
		min_vfs::dentry_t dentry;

		dentry.fname = file_entry.name;
		dentry.fsize = file_entry.block_cnt * BLOCK_SIZE;
		dentry.ctime = 0;
		dentry.mtime = 0;
		dentry.atime = 0;
		dentry.ftype = min_vfs::ftype_t::file;

		return dentry;
	}

	constexpr char FILE_MAGIC[] = {'S', '5', 'X', 'X'};

	static uint16_t read_file(filesystem_t &fs, internal_file_t &internal_file,
							  uintmax_t &pos, uintmax_t len, void *dst)
	{
		u8 buff_off = 0;

		if(pos < 16)
		{
			const u8 remaining = 16 - pos;
			const u8 read_len = std::min((uintmax_t)remaining, len);

			const char HEADER[16] =
			{
				'S', '5', 'X', 'X', 0, 0, 0, 0,
				(char)internal_file.file_entry.type, 0, 0, 0, 0, 0, 0, 0
			};

			std::memcpy(dst, HEADER + pos, read_len);

			pos += read_len;
			len -= read_len;
			buff_off = read_len;
		}

		const uintmax_t size = internal_file.file_entry.block_cnt * BLOCK_SIZE;
		const uintmax_t local_pos = pos - 16;

		if(local_pos < size)
		{
			const uintmax_t remaining = size - local_pos;
			const uintmax_t read_len = std::min(remaining, len);

			//Not great that we're locking for the entire read operation
			fs.mtx.lock();
			fs.stream.seekg(internal_file.file_entry.block_addr * BLOCK_SIZE
				+ local_pos);
			fs.stream.read((char*)dst + buff_off, read_len);
			fs.mtx.unlock();

			pos += read_len;
			len -= read_len;
		}

		if(!len) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::END_OF_FILE);
	}

	filesystem_t::filesystem_t(const char *path): min_vfs::filesystem_t(path)
	{
		char magic[16];

		if(std::filesystem::file_size(this->path) < BLOCK_SIZE)
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
											(u8)min_vfs::ERR::WRONG_FS));

		stream.seekg(0);
		stream.read(magic, 16);

		if(std::memcmp(magic, MAGIC, 16))
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::WRONG_FS));

		//--------------------------FS type heuristic--------------------------
		constexpr u8 EXPECTED_HEURISTIC_DATA[5] =
		{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		this->FIRST_DIR_IDX = 0;

		for(u8 i = 0; i < FIRST_CD_DIR; i++)
		{
			stream.seekg(0x20 + On_disk_sizes::DIR_ENTRY * i + 0x1B);
			stream.read(magic, 5);

			if(std::memcmp(magic, EXPECTED_HEURISTIC_DATA, 5))
			{
				this->FIRST_DIR_IDX = FIRST_CD_DIR;
				break;
			}
		}
		//---------------------------End of heuristic--------------------------
	}

	filesystem_t &filesystem_t::operator=(filesystem_t &&other) noexcept
	{
		if(this == &other) return *this;

		path = other.path;
		stream.swap(other.stream);

		FIRST_DIR_IDX = other.FIRST_DIR_IDX;

		/*See comment in S7XX driver's implementation of this.*/

		return *this;
	}

	min_vfs::filesystem_t &filesystem_t::operator=(min_vfs::filesystem_t &&other) noexcept
	{
		filesystem_t *other_ptr;

		other_ptr = dynamic_cast<filesystem_t*>(&other);

		if(!other_ptr) return *this;

		return *this = std::move(*other_ptr);
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

	uint16_t filesystem_t::list(const char *path,
								std::vector<min_vfs::dentry_t> &dentries,
							 const bool get_dir)
	{
		u16 err;
		std::vector<std::string> split_path;
		std::vector<Dir_entry_t> dirs;
		std::vector<File_entry_t> files;

		split_path = str_util::split(std::string(path), std::string("/"));

		switch(split_path.size())
		{
			case 0:
				if(get_dir)
				{
					dentries.push_back
					(
						{
							.fname = "/",
							.fsize = (uintmax_t)((MAX_ROOT_DENTRIES
								- FIRST_DIR_IDX) * On_disk_sizes::DIR_ENTRY),
							.ctime = 0,
							.mtime = 0,
							.atime = 0,
							.ftype = min_vfs::ftype_t::dir
						}
					);
					return 0;
				}

				err = load_from_root<true>(*this, 0, dirs);
				if(err) return err;

				for(const Dir_entry_t &dir_entry: dirs)
					dentries.push_back(dir_entry_to_dentry(FIRST_DIR_IDX == FIRST_CD_DIR, dir_entry));

				return 0;

			case 1:
				err = load_from_root<false>(*this, split_path[0].c_str(), dirs);
				if(err) return err;

				if(get_dir || !is_valid_dir(dirs[0].type))
				{
					dentries.push_back(dir_entry_to_dentry(FIRST_DIR_IDX == FIRST_CD_DIR, dirs[0]));
					return 0;
				}

				err = load_from_dir<true>(*this, dirs[0], "", files);
				if(err) return err;

				for(const File_entry_t &file_entry: files)
					dentries.push_back(file_entry_to_dentry(file_entry));

				return 0;

			case 2:
				err = load_from_root<false>(*this, split_path[0].c_str(), dirs);
				if(err) return err;

				if(!is_valid_dir(dirs[0].type))
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_A_DIR);

				err = load_from_dir<false>(*this, dirs[0], split_path[1].c_str(), files);
				if(err) return err;

				dentries.push_back(file_entry_to_dentry(files.back()));

				return 0;
		}

		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	}

	uint16_t filesystem_t::mkdir(const char *dir_path)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	uint16_t filesystem_t::ftruncate(const char *path, const uintmax_t new_size)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	uint16_t filesystem_t::rename(const char *cur_path, const char *new_path)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	uint16_t filesystem_t::remove(const char *path)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	uint16_t filesystem_t::fopen_internal(const char *path, void **internal_file)
	{
		u16 err, bank_num;
		std::string fname;

		file_map_iterator_t fmap_it;
		std::vector<File_entry_t> files;
		std::vector<Dir_entry_t> dirs;

		std::pair<uintmax_t, internal_file_t> file_entry;
		std::pair<file_map_iterator_t, bool> map_entry;

		const std::vector<std::string> split_path = str_util::split(std::string(path), std::string("/"));

		if(split_path.size() < 1 || split_path.size() > 2)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

		fname = split_path[0];

		if(split_path.size() == 2)
		{
			fname += "/";
			fname += split_path[1];
		}

		fmap_it = open_files.find(fname);

		if(fmap_it != open_files.end())
		{
			fmap_it->second.first++;
			*internal_file = &fmap_it->second.second;
			return 0;
		}

		err = load_from_root<false>(*this, split_path[0].c_str(), dirs);
		if(err) return err;

		switch(split_path.size())
		{
			case 1:
				if(!is_valid_file(dirs[0].type))
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_A_FILE);

				files.emplace_back();
				std::memcpy(files[0].name, dirs[0].name, 17);
				files[0].block_addr = dirs[0].block_addr;
				files[0].block_cnt = dirs[0].block_cnt;
				files[0].type = dirs[0].type;
				files[0].ins_grp_idx = dirs[0].file_cnt;

				fmap_it = open_files.end();
				file_entry.second.dir_map_entry = 0;

				break;

			case 2:
				if(!is_valid_dir(dirs[0].type))
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_A_DIR);

				err = load_from_dir<false>(*this, dirs[0], split_path[1].c_str(), files);
				if(err) return err;

				fmap_it = open_files.find(split_path[0]);

				//Open parent dir
				if(fmap_it != open_files.end()) fmap_it->second.first++;
				else
				{
					file_entry.first = 1;
					file_entry.second.ftype = min_vfs::ftype_t::dir;

					map_entry = open_files.emplace(dirs[0].name, file_entry);

					if(!map_entry.second)
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);

					fmap_it = map_entry.first;
					fmap_it->second.second.map_entry = &(*map_entry.first);
				}

				file_entry.second.dir_map_entry = &(*fmap_it);
				break;
		}

		file_entry.first = 1;
		file_entry.second.ftype = min_vfs::ftype_t::file;
		file_entry.second.file_entry = files[0];

		map_entry = open_files.emplace(fname, file_entry);

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
			if(cast_ptr->ftype == min_vfs::ftype_t::file
				&& cast_ptr->dir_map_entry)
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
		return read_file(*this, *((internal_file_t*)internal_file), pos, len, dst);
	}

	uint16_t filesystem_t::write(void *internal_file, uintmax_t &pos, uintmax_t len, void *src)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
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
