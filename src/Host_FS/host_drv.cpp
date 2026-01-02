#include <filesystem>
#include <chrono>
#include <utility>
#include <fstream>
#include <system_error>

#include "Utils/utils.hpp"
#include "Host_FS/host_drv.hpp"
#include "min_vfs/min_vfs_base.hpp"

namespace Host::FS
{
	static std::time_t file_time_to_time_t(const std::filesystem::file_time_type
		time)
	{
		return std::chrono::system_clock::to_time_t(
			std::chrono::clock_cast<std::chrono::system_clock>(time));
	}

	static min_vfs::ftype_t host_ftype_to_ftype(const std::filesystem::file_type
		host_ftype)
	{
		switch(host_ftype)
		{
			using enum std::filesystem::file_type;
			using ftype_t = enum min_vfs::ftype_t;

			case regular: return ftype_t::file;
			case directory: return ftype_t::dir;
			case symlink: return ftype_t::symlink;
			case block: return ftype_t::blk_dev;
			case character: return ftype_t::char_dev;
			case fifo: return ftype_t::pipe;
			case socket: return ftype_t::socket;
			case none:
			case unknown:
			case not_found:
			default:
				return ftype_t::unknown;
		}
	}

	min_vfs::dentry_t host_dentry_to_min_vfs_dentry(
		const std::filesystem::directory_entry &dentry)
	{
		using native_ftype =  std::filesystem::file_type;

		return
		{
			.fname = dentry.path().filename().string(),
			.fsize = dentry.is_regular_file() || (dentry.is_symlink()
				&& dentry.status().type() == native_ftype::regular) ?
				dentry.file_size() : 0,

			.ctime = 0, //not provided by std::filesystem

			.mtime = dentry.symlink_status().type() != native_ftype::not_found
			&& dentry.status().type() != native_ftype::not_found ?
			file_time_to_time_t(dentry.last_write_time()) : 0,

			.atime = 0, //not provided by std::filesystem either
			.ftype = host_ftype_to_ftype(dentry.status().type())
		};
	}

	template <bool write>
	static uint16_t read_write_data(internal_file_t *internal_file,
									uintmax_t &pos, const uintmax_t len,
									void *dst)
	{
		internal_file->fstr.seekg(pos);

		if constexpr(write) internal_file->fstr.write((char*)dst, len);
		else internal_file->fstr.read((char*)dst, len);

		if constexpr(!write)
		{
			if(internal_file->fstr.eof())
			{
				pos += internal_file->fstr.gcount();
				internal_file->fstr.clear();
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::END_OF_FILE);
			}
		}

		pos = internal_file->fstr.tellg();

		if(internal_file->fstr.good()) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::IO_ERROR);
	}

	static uint16_t system_error_to_min_vfs_error(const std::error_code &ec)
	{
		switch(ec.value())
		{
			using enum std::errc;

			case (int)no_such_file_or_directory:
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::NOT_FOUND);

			case (int)io_error:
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);

			case (int)permission_denied:
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::NO_PERM);

			default:
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::UNKNOWN_ERROR);
		}
	}

	constexpr filesystem_t::filesystem_t(const char *path):
		min_vfs::filesystem_t()
	{
		//NOP
	}

	filesystem_t& filesystem_t::operator=(filesystem_t &&other) noexcept
	{
		if(this == &other) return *this;

		//We don't really care about the base class' path and stream

		/*See comment in S7XX driver's implementation of this.*/
		this->file_map = other.file_map;

		return *this;
	}

	min_vfs::filesystem_t& filesystem_t::operator=(
		min_vfs::filesystem_t &&other) noexcept
	{
		filesystem_t *other_ptr;

		other_ptr = dynamic_cast<filesystem_t*>(&other);

		if(!other_ptr) return *this;

		return *this = std::move(*other_ptr);
	}

	std::string filesystem_t::get_type_name()
	{
		return "Host";
	}

	uintmax_t filesystem_t::get_open_file_count()
	{
		return file_map.size();
	}

	constexpr bool filesystem_t::can_unmount()
	{
		return false;
	}

	uint16_t filesystem_t::list_internal(std::filesystem::path path,
									   std::vector<min_vfs::dentry_t> &dentries,
									   const bool get_dir)
	{
		/*if(std::filesystem::is_symlink(path))
			path = std::filesystem::read_symlink(path);*/

		if(!get_dir && std::filesystem::is_directory(path))
		{
			const std::filesystem::directory_iterator end;
			std::filesystem::directory_iterator dir_it(path);

			/*According to https://en.cppreference.com/w/cpp/filesystem/
			 *file_size.html: "The result of attempting to determine the size of
			 *a directory (as well as any other file that is not a regular file
			 *or a symlink) is implementation-defined."*/

			while(dir_it != end)
			{
				dentries.push_back(host_dentry_to_min_vfs_dentry(*dir_it));
				dir_it++;
			}
		}
		else
		{
			const std::filesystem::directory_entry host_dentry(path);
			if(!host_dentry.exists())
				return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NOT_FOUND);

			dentries.push_back(host_dentry_to_min_vfs_dentry(host_dentry));
		}

		return 0;
	}

	uint16_t filesystem_t::list_static(std::filesystem::path path,
										 std::vector<min_vfs::dentry_t> &dentries,
									  const bool get_dir)
	{
		try
		{
			return list_internal(path, dentries, get_dir);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}
	}

	uint16_t filesystem_t::list(const char *path,
								std::vector<min_vfs::dentry_t> &dentries,
								const bool get_dir)
	{
		return filesystem_t::list_static(path, dentries, get_dir);
	}

	uint16_t filesystem_t::mkdir_internal(const char *dir_path)
	{
		if(std::filesystem::exists(dir_path))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::ALREADY_EXISTS);

		std::filesystem::create_directory(dir_path);
		return 0;
	}

	uint16_t filesystem_t::mkdir_static(const char *dir_path)
	{
		try
		{
			return mkdir_internal(dir_path);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}
	}

	uint16_t filesystem_t::mkdir(const char *dir_path)
	{
		return mkdir_static(dir_path);
	}

	uint16_t filesystem_t::ftruncate_internal(const char *path,
											  const uintmax_t new_size)
	{
		if(!std::filesystem::exists(path))
		{
			std::ofstream ofstr(path);
			ofstr.close();
		}
		else if(!std::filesystem::is_regular_file(path))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_A_FILE);

		std::filesystem::resize_file(path, new_size);
		return 0;
	}

	uint16_t filesystem_t::ftruncate_static(const char *path,
											const uintmax_t new_size)
	{
		try
		{
			return ftruncate_internal(path, new_size);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}
	}

	uint16_t filesystem_t::ftruncate(const char *path, const uintmax_t new_size)
	{
		return ftruncate_static(path, new_size);
	}

	uint16_t filesystem_t::rename_static(const char *cur_path,
										 const char *new_path)
	{
		try
		{
			std::filesystem::rename(cur_path, new_path);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}

		return 0;
	}

	uint16_t filesystem_t::rename(const char *cur_path, const char *new_path)
	{
		return rename_static(cur_path, new_path);
	}

	uint16_t filesystem_t::remove_static(const char *path)
	{
		try
		{
			if(std::filesystem::is_directory(path))
				std::filesystem::remove_all(path);

			std::filesystem::remove(path);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}

		return 0;
	}

	uint16_t filesystem_t::remove(const char *path)
	{
		return remove_static(path);
	}

	uint16_t filesystem_t::fopen_internal_but_actually(const char *path,
													   void **internal_file)
	{
		const std::filesystem::path abs_path = std::filesystem::absolute(path);
		const filesystem_t::file_map_iterator_t fmap_it =
			file_map.find(abs_path.string());

		std::ios_base::openmode openmode = std::ios_base::binary |
			std::ios_base::in | std::ios_base::out;

		if(fmap_it == file_map.end())
		{
			if(!std::filesystem::exists(path)) openmode |= std::ios_base::trunc;
			else if(std::filesystem::is_directory(path))
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::NOT_A_FILE);
		}

		streams.emplace_back();
		const filesystem_t::stream_list_iterator_t slist_it = --streams.end();

		slist_it->fstr.open(path, openmode);

		if(!slist_it->fstr.is_open() || !slist_it->fstr.good())
		{
			streams.erase(slist_it);
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);
		}

		if(fmap_it == file_map.end())
		{
			const std::pair<filesystem_t::file_map_iterator_t, bool> fmap_res =
				file_map.emplace(abs_path.string(), 0);

			if(!fmap_res.second)
			{
				streams.erase(slist_it);
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);
			}

			slist_it->map_entry = &(*fmap_res.first);
		}
		else
		{
			++fmap_it->second;
			slist_it->map_entry = &(*fmap_it);
		}

		slist_it->self = slist_it;
		*internal_file = &(*slist_it);
		return 0;
	}

	uint16_t filesystem_t::fopen_internal(const char *path,
										  void **internal_file)
	{
		try
		{
			return fopen_internal_but_actually(path, internal_file);
		}
		catch(std::filesystem::filesystem_error e)
		{
			return system_error_to_min_vfs_error(e.code());
		}
	}

	uint16_t filesystem_t::fclose(void *internal_file)
	{
		internal_file_t *const fptr = (internal_file_t*)internal_file;

		if(fptr->map_entry->second)
		{
			--fptr->map_entry->second;
			streams.erase(fptr->self);
		}
		else
		{
			const filesystem_t::file_map_t::value_type *const fmap_entry =
				fptr->map_entry;

			streams.erase(fptr->self);
			file_map.erase(fmap_entry->first);
		}

		return 0;
	}

	uint16_t filesystem_t::read(void *internal_file, uintmax_t &pos,
								uintmax_t len, void *dst)
	{
		const u16 ret = read_write_data<false>((internal_file_t*)internal_file,
											   pos, len, dst);
		return ret;
	}

	uint16_t filesystem_t::write(void *internal_file, uintmax_t &pos,
								 uintmax_t len, void *src)
	{
		const u16 ret = read_write_data<true>((internal_file_t*)internal_file,
											   pos, len, src);
		return ret;
	}

	uint16_t filesystem_t::flush(void *internal_file)
	{
		((internal_file_t*)internal_file)->fstr.flush();

		if(((internal_file_t*)internal_file)->fstr.good()) return 0;
		else return ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::IO_ERROR);
	}
}
