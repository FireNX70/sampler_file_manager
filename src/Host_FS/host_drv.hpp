#ifndef HOST_FS_DRV_HEADER_INCLUDE_GUARD
#define HOST_FS_DRV_HEADER_INCLUDE_GUARD

#include <cstdint>
#include <unordered_map>
#include <string>
#include <list>
#include <fstream>
#include <mutex>

#include "min_vfs/min_vfs.hpp"
#include "min_vfs/min_vfs_base.hpp"

namespace Host::FS
{
	struct map_entry_t;
	struct internal_file_t;

	struct filesystem_t: min_vfs::filesystem_t
	{
		typedef std::unordered_map<std::string, uintmax_t> file_map_t;
		file_map_t file_map;
		using file_map_iterator_t = file_map_t::iterator;

		typedef std::list<internal_file_t> stream_list_t;
		stream_list_t streams;
		using stream_list_iterator_t = stream_list_t::iterator;

		filesystem_t() = default;
		constexpr filesystem_t(const char *path);

		min_vfs::filesystem_t& operator=(min_vfs::filesystem_t &&other) noexcept
			override;
		filesystem_t& operator=(filesystem_t &&other) noexcept;

		std::string get_type_name() override;
		uintmax_t get_open_file_count() override;
		constexpr bool can_unmount() override;

		static uint16_t list_static(std::filesystem::path path,
									std::vector<min_vfs::dentry_t> &dentries,
							  const bool get_dir);
		static uint16_t mkdir_static(const char *dir_path);
		static uint16_t ftruncate_static(const char *path,
										 const uintmax_t new_size);
		static uint16_t remove_static(const char *path);
		static uint16_t rename_static(const char *cur_path,
									  const char *new_path);

		using min_vfs::filesystem_t::list;
		uint16_t list(const char *path,
					  std::vector<min_vfs::dentry_t> &dentries,
				const bool get_dir) override;
		uint16_t mkdir(const char *dir_path) override;
		uint16_t ftruncate(const char *path, const uintmax_t new_size) override;
		uint16_t rename(const char *cur_path, const char *new_path) override;
		uint16_t remove(const char *path) override;

		uint16_t fclose(void *internal_file) override;
		uint16_t read(void *internal_file, uintmax_t &pos, uintmax_t len,
					  void *dst) override;
		uint16_t write(void *internal_file, uintmax_t &pos, uintmax_t len,
					   void *src) override;
		uint16_t flush(void *internal_file) override;

	private:
		static uint16_t list_internal(std::filesystem::path path,
									  std::vector<min_vfs::dentry_t> &dentries,
								const bool get_dir);
		static uint16_t mkdir_internal(const char *dir_path);
		static uint16_t ftruncate_internal(const char *path,
										   const uintmax_t new_size);
		uint16_t fopen_internal_but_actually(const char *path,
											 void **internal_file);

		uint16_t fopen_internal(const char *path, void **internal_file)
			override;
	};

	struct internal_file_t
	{
		filesystem_t::stream_list_iterator_t self;
		filesystem_t::file_map_t::value_type *map_entry;
		std::fstream fstr;
	};
}

#endif
