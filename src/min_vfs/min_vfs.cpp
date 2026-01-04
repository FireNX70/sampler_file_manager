#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <list>
#include <utility>
#include <shared_mutex>
#include <ctime>
#include <vector>
#include <stack>

#include "Utils/str_util.hpp"
#include "Utils/utils.hpp"
#include "min_vfs_base.hpp"
#include "path_concat_helpers.hpp"
#include "min_vfs.hpp"
#include "Host_FS/host_drv.hpp"
#include "E-MU/EMU_FS_drv.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"
#include "Roland/S5XX/S5XX_FS_drv.hpp"

namespace min_vfs
{
	/*Thread safety: The mount map is guarded via an std::shared_mutex, since it
	 *should be read-mostly.
	 *As for filesystems themselves, right now we use a single mutex per
	 *filesystem instance. Drivers themselves only handle thread safety for
	 *reading and writing file data. The VFS handles thread safety for all other
	 *operations.*/

	/*Mount map: I have an idea for using multiple maps (one per layer/
	 *directory). I started implementing that, but it was a mess so I chose to
	 *go with this much simpler implementation. Consider taking a look at that
	 *and trying again later. Or perhaps there's a better way to map this out.*/

	/*Paths: One problem I have to sort out is the current mess of mixing
	 *std::filesystem::path, std::string and char*. This leads to possible
	 *needless copying, and to problems where std::filesystem::path can
	 *implicitly convert to std::string on Linux, but not on Windows (we're
	 *using std::string for keys in a bunch of maps).*/

	typedef std::list<std::unique_ptr<filesystem_t>> fs_list_t;
	typedef std::unordered_map<std::filesystem::path, fs_list_t::iterator>
		fs_map_t;

	std::shared_mutex mounts_mtx;

	Host::FS::filesystem_t host_fs;
	fs_list_t fs_list;
	fs_map_t fs_map;


	void lsmount(std::vector<mount_stats_t> &mounts)
	{
		for(const std::unique_ptr<filesystem_t> &fs: fs_list)
			mounts.emplace_back(fs->path, fs->get_type_name(),
								fs->get_open_file_count());
	}

	void lsmap(std::vector<map_stats_t> &map_stats)
	{
		for(const fs_map_t::value_type &entry: fs_map)
			map_stats.emplace_back(entry.first, mount_stats_t
			(
				entry.second->get()->path,
				entry.second->get()->get_type_name(),
				entry.second->get()->get_open_file_count()
			));
	}

	/*We can't use std::filesystem::canonical because it fails on paths pointing
	 *inside any filesystems we've mounted (because it treats a non-directory)
	 *as a directory. std::filesystem::absolute does not fail on this (not with
	 *GCC, at least). Hopefully, this should work with symlinks.*/
	fs_map_t::iterator find_fs(std::filesystem::path path,
							   std::filesystem::path &remainder)
	{
		constexpr std::filesystem::path::value_type SEPARATOR_AS_STRING[] =
			{'/', '\0'}; //Can't use a string literal because Windows

		size_t og_path_off;

		fs_map_t::iterator fs_map_it;
		std::filesystem::path path_copy;

		path = std::filesystem::absolute(path);

		const std::vector<std::filesystem::path::string_type> split_path =
			str_util::split<std::filesystem::path::value_type, false>(
				path.native(), SEPARATOR_AS_STRING);

		if(!split_path.size()) return fs_map.end();

		/*This entire initialization clusterfuck exists to satisfy Windows.*/
		path_copy = split_path[0] + SEPARATOR_AS_STRING;

		//-1 to compensate for loop's unconditional +1.
		og_path_off = path_copy.native().length() - 1;

		for(size_t i = 1; i < split_path.size(); i++)
		{
			CONCAT_ASSIGN_PATH(path_copy, split_path[i]);
			og_path_off += 1 + split_path[i].length();

			if(std::filesystem::exists(path_copy)
				&& std::filesystem::is_symlink(path_copy))
				path_copy = std::filesystem::canonical(
					std::filesystem::read_symlink(path_copy));

			fs_map_it = fs_map.find(path_copy);

			if(fs_map_it != fs_map.end())
			{
				remainder = path.generic_string().substr(og_path_off);
				return fs_map_it;
			}
		}

		remainder = path;
		return fs_map.end();
	}

	typedef uint16_t (*fsck_f)(const std::filesystem::path &fs_path,
							   u16 &fsck_status);

	constexpr fsck_f fsck_funcs[] =
	{
		EMU::FS::fsck,
		S7XX::FS::fsck,
		S5XX::FS::fsck
	};

	uint16_t fsck(std::filesystem::path path)
	{
		constexpr u16 WRONG_FS_CODE = ret_val_setup(LIBRARY_ID,
													(u16)ERR::WRONG_FS);

		u16 fsck_status;

		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		if(std::filesystem::is_symlink(path))
			path = std::filesystem::read_symlink(path);

		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		path = std::filesystem::absolute(path);

		for(const fsck_f fsck_func: fsck_funcs)
		{
			const u16 err = fsck_func(path, fsck_status);

			if(!err || err != WRONG_FS_CODE) return err;
			if(fsck_status) return ret_val_setup(LIBRARY_ID,
				(u16)ERR::INVALID_STATE);

			/*TODO: Figure out standard fsck status codes or some form of
			logging for fsck.*/
		}

		return WRONG_FS_CODE;
	}

	template<typename T>
	requires(std::is_base_of_v<filesystem_t, T>)
	filesystem_t* mount_fs(const char *path)
	{
		return new T(path);
	}

	typedef filesystem_t* (*mount_fs_f)(const char *path);

	constexpr mount_fs_f mount_funcs[] =
	{
		mount_fs<EMU::FS::filesystem_t>,
		mount_fs<S7XX::FS::filesystem_t>,
		mount_fs<S5XX::FS::filesystem_t>
	};

	uint16_t mount_internal(std::filesystem::path &path)
	{
		bool succ;
		size_t i;
		std::filesystem::path remainder;

		fs_map_t::iterator fs_map_it;

		if(find_fs(path, remainder) != fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::ALREADY_OPEN);

		succ = false;
		for(mount_fs_f mount_func: mount_funcs)
		{
			try
			{
				fs_list.emplace_back(mount_func(path.string().c_str()));
				succ = true;
				break;
			}
			catch(FS_err e)
			{
				if(e.err_code != ret_val_setup(LIBRARY_ID,
					(u8)ERR::DISK_TOO_SMALL) && e.err_code
					!= ret_val_setup(LIBRARY_ID, (u8)ERR::WRONG_FS))
					return e.err_code;
			}
		}

		//No appropriate driver. Do we want it to have its own error code?
		if(!succ)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::WRONG_FS);

		fs_map[path] = --fs_list.end();

		return 0;
	}

	uint16_t mount(std::filesystem::path path)
	{
		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		if(std::filesystem::is_symlink(path))
			path = std::filesystem::read_symlink(path);

		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		path = std::filesystem::absolute(path);

		mounts_mtx.lock();
		const u16 err = mount_internal(path);
		mounts_mtx.unlock();

		return err;
	}

	uint16_t umount_internal(std::filesystem::path &path)
	{
		u16 err;
		std::filesystem::path remainder;

		fs_map_t::iterator fs_map_it;

		fs_map_it = find_fs(path, remainder);
		if(fs_map_it == fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);

		filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const bool succ = fs->can_unmount();

		if(succ)
		{
			fs_list.erase(fs_map_it->second);
			fs_map.erase(fs_map_it);
			err = 0;
		}
		else
		{
			//Unlock the filesystem's mutex only if we didn't unmount.
			fs->mtx.unlock();
			err = ret_val_setup(LIBRARY_ID, (u8)ERR::FS_BUSY);
		}

		return err;
	}

	uint16_t umount(std::filesystem::path path)
	{
		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		if(std::filesystem::is_symlink(path))
			path = std::filesystem::read_symlink(path);

		if(!std::filesystem::exists(path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NONEXISTANT_DISK);

		path = std::filesystem::absolute(path);

		mounts_mtx.lock();
		const u16 err = umount_internal(path);
		mounts_mtx.unlock();

		return err;
	}

	static uint16_t list_internal(std::filesystem::path path,
								  std::vector<dentry_t> &dentries,
							   const bool get_dir)
	{
		std::filesystem::path remainder;
		fs_map_t::iterator fs_map_it;

		path = std::filesystem::absolute(path);

		if(std::filesystem::exists(path))
		{
			/*I initially came up with a small "optimization" (for the crappy
			 *way we're doing things right now anyway) where we'd check if the
			 *path pointed to an existing file, and if it did we would just
			 *operate on that. The reason that is a problem is if the file is an
			 *image we've mounted, we're just gonna return the image's dentry
			 *instead of listing its root. That's what this first check here
			 *"fixes".*/

			fs_map_it = find_fs(path, remainder);
			if(fs_map_it != fs_map.end())
			{
				min_vfs::filesystem_t *const fs = fs_map_it->second->get();

				fs->mtx.lock();
				const u16 err = fs->list("", dentries, get_dir);
				fs->mtx.unlock();

				return err;
			}

			return Host::FS::filesystem_t::list_static(path.string().c_str(),
													   dentries, get_dir);
		}

		fs_map_it = find_fs(path, remainder);
		if(fs_map_it == fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);

		min_vfs::filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const u16 err = fs->list(remainder.string().c_str(), dentries, get_dir);
		fs->mtx.unlock();

		return err;
	}

	uint16_t list(std::filesystem::path path, std::vector<dentry_t> &dentries,
				  const bool get_dir)
	{
		mounts_mtx.lock_shared();
		const u16 err = list_internal(path, dentries, get_dir);
		mounts_mtx.unlock_shared();

		return err;
	}

	uint16_t list(std::filesystem::path path, std::vector<dentry_t> &dentries)
	{
		return list(path, dentries, false);
	}

	static uint16_t mkdir_internal(std::filesystem::path &dir_path)
	{
		std::filesystem::path remainder;
		fs_map_t::iterator fs_map_it;

		if(std::filesystem::exists(dir_path.parent_path())
			&& std::filesystem::is_directory(dir_path.parent_path()))
		{
			fs_map_it = find_fs(dir_path.parent_path(), remainder);
			if(fs_map_it != fs_map.end())
			{
				min_vfs::filesystem_t *const fs = fs_map_it->second->get();

				fs->mtx.lock();
				const u16 err = fs->mkdir(remainder.string().c_str());
				fs->mtx.unlock();

				return err;
			}

			return Host::FS::filesystem_t::mkdir_static(
				dir_path.string().c_str());
		}

		fs_map_it = find_fs(dir_path, remainder);
		if(fs_map_it == fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);

		filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const u16 err = fs->mkdir(remainder.string().c_str());
		fs->mtx.unlock();

		return err;
	}

	uint16_t mkdir(std::filesystem::path dir_path)
	{
		dir_path = std::filesystem::absolute(dir_path);

		mounts_mtx.lock_shared();
		const u16 err = mkdir_internal(dir_path);
		mounts_mtx.unlock_shared();

		return err;
	}

	static uint16_t ftruncate_internal(std::filesystem::path path,
									   const uintmax_t new_size)
	{
		std::filesystem::path remainder;
		fs_map_t::iterator fs_map_it;

		if(std::filesystem::exists(path.parent_path()))
		{
			fs_map_it = find_fs(path.parent_path(), remainder);
			if(fs_map_it != fs_map.end())
			{
				min_vfs::filesystem_t *const fs = fs_map_it->second->get();

				fs->mtx.lock();
				const u16 err = fs->ftruncate(remainder.string().c_str(),
											  new_size);
				fs->mtx.unlock();

				return err;
			}

			return Host::FS::filesystem_t::ftruncate_static(
				path.string().c_str(), new_size);
		}

		fs_map_it = find_fs(path, remainder);
		if(fs_map_it == fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);

		filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const u16 err = fs->ftruncate(remainder.string().c_str(), new_size);
		fs->mtx.unlock();

		return err;
	}

	uint16_t ftruncate(std::filesystem::path path, const uintmax_t new_size)
	{
		path = std::filesystem::absolute(path);

		mounts_mtx.lock_shared();
		const u16 err = ftruncate_internal(path, new_size);
		mounts_mtx.unlock_shared();

		return err;
	};

	/*We need to implement this for the copy case of rename. I guess we might as
	well expose a copy operation in the VFS?*/
	static uint16_t copy_file_inner(filesystem_t *const src_fs,
									filesystem_t *const dst_fs,
								 const char *src_path,
								 const char *dst_path)
	{
		constexpr size_t BUFFER_SIZE = 512;

		u16 err, dst_err;
		std::unique_ptr<u8[]> buffer;

		stream_t src_str, dst_str;
		std::vector<dentry_t> dentries;

		src_fs->mtx.lock();
		err = src_fs->fopen(src_path, src_str);
		src_fs->mtx.unlock();
		if(err) return err;

		dst_fs->mtx.lock();
		err = dst_fs->fopen(dst_path, dst_str);
		dst_fs->mtx.unlock();
		if(err) return err;

		src_fs->mtx.lock();
		err = src_fs->list(src_path, dentries, false);
		src_fs->mtx.unlock();
		if(err) return err;

		const uintmax_t full_buffers = dentries[0].fsize / BUFFER_SIZE;

		/*I kinda wanna benchmark whether mod or multiply and subtract is
		faster, but the difference that's gonna make is negligible and might
		change significatively with different uarchs.

		While x86 doesn't have dedicated modulo/remainder instructions, DIV
		actually also gives you the remainder. The hope is that by using modulo
		right after the division, GCC might recognize that and store both the
		quotient AND the remainder.
		RISC-V does implement dedicated modulo instructions (or remainder,
		rather, since it's named REM).

		According to Godbolt (https://godbolt.org/z/9YKEevnM4); GCC (15.2) will
		just use a shift for the division and a bitwise and (with 511 as the
		mask) for the mod, thanks to the fact that our buffer size is a power of
		2.
		With a non-power-of-2 divisor (https://godbolt.org/z/bxE5GEPa7); it
		avoids running divs by running a shift then mul then shift for BOTH,
		then tacking on an imul and a sub for the mod. So it's choosing the
		multiply then subtract approach, but it fails to realize it only needs
		to run the division once. Not sure that this is really faster than a
		single div and just storing both results. Maybe I read the documentation
		for div wrong, or there's some reason to avoid doing that that just
		escapes me.*/
		const uintmax_t remainder = dentries[0].fsize % BUFFER_SIZE;

		buffer = std::make_unique<u8[]>(BUFFER_SIZE);

		for(uintmax_t i = 0; i < full_buffers; i++)
		{
			err = src_str.read(buffer.get(), BUFFER_SIZE);
			if(err) return err;

			dst_err = dst_str.write(buffer.get(), BUFFER_SIZE);
			if(dst_err) return dst_err;
		}

		if(remainder)
		{
			err = src_str.read(buffer.get(), remainder);
			if(err) return err;

			dst_err = dst_str.write(buffer.get(), remainder);
			if(dst_err) return dst_err;
		}

		return 0;
	}

	struct dir_stack_entry_t
	{
		std::string comp;
		size_t level;
	};

	typedef std::stack<dir_stack_entry_t> dir_stack_t;

	/*The paths should be the directories paths relative to their respective
	 *filesystems.*/
	static uint16_t copy_dir_inner(filesystem_t *const src_fs,
								   filesystem_t *const dst_fs,
								const std::filesystem::path &src_path,
								std::filesystem::path dst_path,
								dir_stack_t &dir_stack, const size_t level,
								const bool renamed)
	{
		u16 err;
		std::vector<min_vfs::dentry_t> dentries;

		if(!renamed)
			CONCAT_ASSIGN_PATH(dst_path, src_path.filename());

		dst_fs->mtx.lock();
		err = dst_fs->list(dst_path.string().c_str(), dentries, true);
		dst_fs->mtx.unlock();
		if(err)
		{
			if(err == ret_val_setup(LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND))
			{
				dst_fs->mtx.lock();
				err = dst_fs->mkdir(dst_path.string().c_str());
				dst_fs->mtx.unlock();
				if(err) return err;
			}
			else return err;
		}

		dentries.clear();
		src_fs->mtx.lock();
		err = src_fs->list(src_path.string().c_str(), dentries);
		src_fs->mtx.unlock();
		if(err) return err;

		for(size_t i = 0; i < dentries.size(); i++)
		{
			if(dentries[i].ftype == ftype_t::dir)
			{
				dir_stack.emplace(dentries[i].fname, level + 1);
				continue;
			}

			err = copy_file_inner(src_fs, dst_fs,
				(CONCAT_PATHS(src_path, dentries[i].fname)).string().c_str(),
				(CONCAT_PATHS(dst_path, dentries[i].fname)).string().c_str());
			if(err) return err;

			dentries.erase(dentries.begin() + i);
			i--; //Gotta recheck the same index afterwards
		}

		return 0;
	}

	static uint16_t copy_dir_hrchy(filesystem_t *const src_fs,
								   filesystem_t *const dst_fs,
								const char *const src_path_og,
								const char *const dst_path_og,
								bool top_dir_renamed)
	{
		u16 err;
		size_t level;
		std::filesystem::path src_path, dst_path;

		dir_stack_t dir_stack;
		dir_stack_entry_t dir_stack_entry;

		src_path = src_path_og;
		dst_path = dst_path_og;

		level = 0;
		dir_stack.emplace(src_path.filename().string(), 0);
		src_path = src_path.parent_path();

		/*The current approach only works because we only remove the source
		 *hierarchy after copying the whole thing, instead of removing each
		 *directory as soon as we're done copying its contents.*/

		do
		{
			dir_stack_entry = dir_stack.top();
			dir_stack.pop();

			while(level > dir_stack_entry.level)
			{
				src_path = src_path.parent_path();
				dst_path = dst_path.parent_path();
				level--;
			}

			level = dir_stack_entry.level;
			CONCAT_ASSIGN_PATH(src_path, dir_stack_entry.comp);

			err = copy_dir_inner(src_fs, dst_fs, src_path.string().c_str(),
								 dst_path.string().c_str(), dir_stack, level,
								 top_dir_renamed);
			top_dir_renamed = false;
			if(err) return err;

			if(dir_stack.size())
			{
				if(dir_stack.top().level > level)
					CONCAT_ASSIGN_PATH(dst_path, dir_stack_entry.comp);
				else src_path = src_path.parent_path();
			}
			else break;
		}
		while(true);

		return 0;
	}

	static uint16_t copy_inner(filesystem_t *const src_fs,
							   filesystem_t *const dst_fs,
							const char *src_path,
							const char *dst_path)
	{
		constexpr u16 NOT_FOUND_ERR = ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::NOT_FOUND);
		u16 err;
		std::filesystem::path final_dst_path;

		std::vector<dentry_t> src_dentries, dst_dentries;

		src_fs->mtx.lock();
		err = src_fs->list(src_path, src_dentries, true);
		src_fs->mtx.unlock();
		if(err) return err;

		dst_fs->mtx.lock();
		err = dst_fs->list(dst_path, dst_dentries, true);
		dst_fs->mtx.unlock();
		if(err && err != NOT_FOUND_ERR) return err;

		final_dst_path = dst_path;
		if(src_dentries[0].ftype == ftype_t::dir)
		{
			const bool top_dir_renamed = err == NOT_FOUND_ERR;
			if(top_dir_renamed)
			{
				const std::string new_dir_name = final_dst_path.filename()
					.string();
				final_dst_path = final_dst_path.parent_path();
				dst_fs->mtx.lock();
				err = dst_fs->list(final_dst_path.string().c_str(),
								   dst_dentries, true);
				dst_fs->mtx.unlock();
				if(err) return err;

				if(dst_dentries[0].ftype != ftype_t::dir)
					return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_A_DIR);

				dst_fs->mtx.lock();
				err = dst_fs->mkdir(dst_path);
				dst_fs->mtx.unlock();
				if(err) return err;

				final_dst_path = dst_path;
			}
			else
			{
				if(dst_dentries[0].ftype != ftype_t::dir)
					return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_A_DIR);
			}

			return copy_dir_hrchy(src_fs, dst_fs, src_path,
								  final_dst_path.string().c_str(),
								  top_dir_renamed);
		}
		else
		{
			if(!dst_dentries.size())
			{
				dst_dentries.clear();
				dst_fs->mtx.lock();
				err = dst_fs->list(
					final_dst_path.parent_path().string().c_str(), dst_dentries,
								   true);
				dst_fs->mtx.unlock();
				if(err) return err;

				if(dst_dentries[0].ftype != min_vfs::ftype_t::dir)
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::INVALID_PATH);
			}
			else if(dst_dentries[0].ftype == min_vfs::ftype_t::dir)
				CONCAT_ASSIGN_PATH(final_dst_path, src_dentries[0].fname);

			return copy_file_inner(src_fs, dst_fs, src_path,
								   final_dst_path.string().c_str());
		}
	}

	static uint16_t copy_internal(std::filesystem::path cur_path,
								  std::filesystem::path new_path)
	{
		u16 err;
		std::filesystem::path cur_remainder, new_remainder;

		if(cur_path == new_path) return 0;

		const fs_map_t::iterator src_fs_it = find_fs(cur_path, cur_remainder);
		const fs_map_t::iterator dst_fs_it = find_fs(new_path, new_remainder);

		filesystem_t *const src_fs = src_fs_it == fs_map.end() ? &host_fs
			: src_fs_it->second->get();

		filesystem_t *const dst_fs = dst_fs_it == fs_map.end() ? &host_fs
			: dst_fs_it->second->get();

		return copy_inner(src_fs, dst_fs, cur_remainder.string().c_str(),
						 new_remainder.string().c_str());
	}

	uint16_t copy(std::filesystem::path cur_path,
				  std::filesystem::path new_path)
	{
		mounts_mtx.lock_shared();
		const u16 err = copy_internal(cur_path, new_path);
		mounts_mtx.unlock_shared();

		return err;
	}

	static std::string get_fpath_from_path_and_fs_it(const fs_map_t::iterator
		fs_it, const std::filesystem::path &fpath)
	{
		return std::string(fpath.string().substr(fs_it->first.string().size()));
	}

	static uint16_t rename_internal(std::filesystem::path cur_path,
							 std::filesystem::path new_path)
	{
		u16 err;
		std::filesystem::path cur_remainder, new_remainder;

		if(cur_path == new_path) return 0;

		const fs_map_t::iterator src_fs_it = find_fs(cur_path, cur_remainder);
		const fs_map_t::iterator dst_fs_it = find_fs(new_path, new_remainder);

		filesystem_t *const src_fs = src_fs_it == fs_map.end() ? &host_fs
			: src_fs_it->second->get();

		filesystem_t *const dst_fs = dst_fs_it == fs_map.end() ? &host_fs
			: dst_fs_it->second->get();

		/*Src is a filesystem's root/mount point, disallow moving. Consider
		treating as host rename in the future.*/
		if(cur_remainder == "")
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::CANT_MOVE);

		if(src_fs == dst_fs)
		{
			if(src_fs == &host_fs)
				return Host::FS::filesystem_t::rename_static(
					cur_path.string().c_str(), new_path.string().c_str());

			src_fs->mtx.lock();
			err = src_fs->rename(cur_remainder.string().c_str(),
								 new_remainder.string().c_str());
			src_fs->mtx.unlock();

			return err;
		}
		else
		{
			//No need to lock the mount mtx, already done by outer rename
			err = copy_inner(src_fs, dst_fs, cur_remainder.string().c_str(),
							 new_remainder.string().c_str());
			if(err) return err;

			src_fs->mtx.lock();
			err = src_fs->remove(cur_remainder.string().c_str());
			src_fs->mtx.unlock();
			return err;
		}
	}

	uint16_t rename(std::filesystem::path cur_path,
					std::filesystem::path new_path)
	{
		mounts_mtx.lock_shared();
		const u16 err = rename_internal(cur_path, new_path);
		mounts_mtx.unlock_shared();

		return err;
	}


	static uint16_t remove_internal(std::filesystem::path path)
	{
		std::filesystem::path remainder;
		fs_map_t::iterator fs_map_it;

		if(std::filesystem::exists(path))
		{
			if(fs_map.contains(path))
				return ret_val_setup(LIBRARY_ID, (u8)ERR::CANT_REMOVE);

			return Host::FS::filesystem_t::remove_static(path.string().c_str());
		}

		fs_map_it = find_fs(path, remainder);
		if(fs_map_it == fs_map.end())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);

		filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const u16 err = fs->remove(remainder.string().c_str());
		fs->mtx.unlock();

		return err;
	}

	uint16_t remove(std::filesystem::path path)
	{
		mounts_mtx.lock_shared();
		const u16 err = remove_internal(path);
		mounts_mtx.unlock_shared();

		return err;
	}

	uint16_t fopen_internal(std::filesystem::path &path, stream_t &stream)
	{
		std::filesystem::path remainder;
		fs_map_t::iterator fs_map_it;

		fs_map_it = find_fs(path, remainder);
		if(fs_map_it == fs_map.end())
		{
			/*This is the only place here where we actually want to lock the
			host fs' mutex. Other than that it's just streams that are gonna use
			it.*/
			host_fs.mtx.lock();
			const u16 err = host_fs.fopen(path.string().c_str(), stream);
			host_fs.mtx.unlock();
			return err;
		}

		filesystem_t *const fs = fs_map_it->second->get();

		fs->mtx.lock();
		const u16 err = fs->fopen(remainder.string().c_str(), stream);
		fs->mtx.unlock();

		return err;
	}

	uint16_t fopen(std::filesystem::path path, stream_t &stream) //may throw
	{
		path = std::filesystem::absolute(path);

		mounts_mtx.lock_shared();
		const u16 err = fopen_internal(path, stream);
		mounts_mtx.unlock_shared();

		return err;
	}
}
