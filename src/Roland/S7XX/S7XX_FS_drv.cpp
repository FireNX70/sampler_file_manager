#include <cstdint>
#include <fstream>
#include <cstring>
#include <bit>
#include <vector>
#include <filesystem>
#include <map>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <regex>
#include <unordered_map>
#include <set>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "Utils/str_util.hpp"
#include "Utils/FAT_utils.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S7XX/host_FS_utils.hpp"

#include "S7XX_FS_types.hpp"
#include "S7XX_FS_drv.hpp"
#include "fs_drv_constants.hpp"
#include "fs_drv_helpers.hpp"

namespace S7XX::FS
{
	//we fake these because it makes sense to present the "FS" like this
	constexpr std::string_view DIR_NAMES[] =
	{
		"OS", //actually a file
		"Volumes",
		"Performances",
		"Patches",
		"Partials",
		"Samples"
	};

	//must start at 0, it's used as an index
	enum struct ftype_IDs: u8
	{
		OS,
		VOLS,
		PERFS,
		PATCHES,
		PARTIALS,
		SAMPLES
	};

	//no std::flat_map yet
	typedef std::map<std::string_view, const type_attrs_t &> dir_map_t;
	const dir_map_t DIR_NAME_TO_ATTRS =
	{
		{DIR_NAMES[0], TYPE_ATTRS[0]},
		{DIR_NAMES[1], TYPE_ATTRS[1]},
		{DIR_NAMES[2], TYPE_ATTRS[2]},
		{DIR_NAMES[3], TYPE_ATTRS[3]},
		{DIR_NAMES[4], TYPE_ATTRS[4]},
		{DIR_NAMES[5], TYPE_ATTRS[5]}
	};

	const min_vfs::dentry_t ROOT_DIR[] =
	{
		{
			.fname = std::string(DIR_NAMES[1]),
			.fsize = (uintmax_t)(TYPE_ATTRS[1].MAX_CNT * On_disk_sizes::LIST_ENTRY),
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::dir
		},
		{
			.fname = std::string(DIR_NAMES[2]),
			.fsize = (uintmax_t)(TYPE_ATTRS[2].MAX_CNT * On_disk_sizes::LIST_ENTRY),
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::dir
		},
		{
			.fname = std::string(DIR_NAMES[3]),
			.fsize = (uintmax_t)(TYPE_ATTRS[3].MAX_CNT * On_disk_sizes::LIST_ENTRY),
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::dir
		},
		{
			.fname = std::string(DIR_NAMES[4]),
			.fsize = (uintmax_t)(TYPE_ATTRS[4].MAX_CNT * On_disk_sizes::LIST_ENTRY),
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::dir
		},
		{
			.fname = std::string(DIR_NAMES[5]),
			.fsize = (uintmax_t)(TYPE_ATTRS[5].MAX_CNT * On_disk_sizes::LIST_ENTRY),
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::dir
		}
	};

	struct type_attrs_ext_t
	{
		const u8 ON_DISK_TOC_OFFSET;
		const u8 IDX_CNT;
		const u8 SIZE_BEFORE_INDICES;
		const char *const FILE_MAGIC;
		const std::string_view &DIR_NAME;
	};

	constexpr type_attrs_ext_t TYPE_ATTRS_EXT[] =
	{
		{0, 0, 0, nullptr, DIR_NAMES[0]}, //dummy
		{0x14, 112, 16, Host_FS::FILE_MAGICS::VOL, DIR_NAMES[1]},
		{0x16, 32, 240, Host_FS::FILE_MAGICS::PERF, DIR_NAMES[2]},
		{0x18, 128, 240, Host_FS::FILE_MAGICS::PATCH, DIR_NAMES[3]},
		{0x1A, 4, 0, Host_FS::FILE_MAGICS::PARTIAL, DIR_NAMES[4]}, //special case
		{0x1C, 0, 0, Host_FS::FILE_MAGICS::SAMPLE, DIR_NAMES[5]}
	};

	const std::regex IDX_STR("[0-9]{1,4}", std::regex::ECMAScript
		| std::regex::optimize);
	const std::regex NAME_WITH_IDX("([0-9]{1,4})-(.*)", std::regex::ECMAScript
		| std::regex::optimize);
	constexpr u16 DUMMY_IDX = 0xFFFF;

	/*Returning the index's string doesn't seem worth it because we're gonna try
	and get a valid index through other means if the filename does not contain
	one. I think it ends up being cleaner to just parse it and then convert the
	final index to a string if needed.*/
	static uint16_t idx_and_name_from_name(const std::string &filename,
										   std::string &cleaned_name)
	{
		u16 res;
		std::smatch rx_match;

		const bool has_idx = std::regex_match(filename, rx_match,
											  NAME_WITH_IDX);

		if(has_idx)
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
			for(u8 i = cleaned_name.size(); i < 16; i++)
				cleaned_name.append(" ");
		}

		return res;
	}

	static uint16_t load_header(std::fstream &src, Header_t &dst)
	{
		std::streampos header_base;

		header_base = src.tellg();

		src.seekg(header_base + (std::streamoff)4);
		src.read(dst.machine_name, 10);
		dst.machine_name[10] = 0;

		src.seekg(header_base + (std::streamoff)15);
		src.read((char*)&dst.media_type, 1);
		src.read(dst.text, 80);

		return 0;
	}

	//includes first two clusters, which are always reserved
	static uint16_t FAT_find_length(std::fstream &fstr)
	{
		uint16_t i, cur_val;

		fstr.seekg(On_disk_addrs::FAT + FAT_ATTRS.DATA_MIN * 2);
		i = FAT_ATTRS.DATA_MIN;

		do
		{
			fstr.read((char*)&cur_val, 2);
		} while(cur_val != 0xFFFF && ++i < MAX_FAT_LENGTH);

		return i;
	}

	typedef uint16_t (*load_list_entry_f)(std::fstream &src, const uint16_t slot,
		List_entry_t &dst);

	template <type_attrs_t MAPPED_TYPE_ATTRS>
	static uint16_t load_list_entry(std::fstream &src, const uint16_t slot,
							 List_entry_t &dst)
	{
		if constexpr(!MAPPED_TYPE_ATTRS.TYPE_IDX) return 0;

		const std::streampos entry_base = MAPPED_TYPE_ATTRS.LIST_ADDR + slot
			* On_disk_sizes::LIST_ENTRY;

		src.seekg(entry_base);

		src.read(dst.name, 16);
		dst.name[16] = 0;

		if(!dst.name[0] || dst.name[0] == (char)0xFE)
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::EMPTY_ENTRY);

		src.read((char*)&dst.type, 1);

		if(dst.type != MAPPED_TYPE_ATTRS.ELEMENT_TYPE)
			return ret_val_setup(LIBRARY_ID,
								 (uint8_t)ERR::ELEMENT_TYPE_MISMATCH);

		std::replace(dst.name, dst.name + sizeof(List_entry_t::name), '/', '\\');

		src.seekg(entry_base + (std::streamoff)0x12);

		src.read((char*)&dst.next_idx, 2);
		src.read((char*)&dst.prev_idx, 2);
		src.read((char*)&dst.cur_idx, 2);

		src.seekg(entry_base + (std::streamoff)0x1B);

		src.read((char*)&dst.program_num, 1);
		src.read((char*)&dst.start_segment, 2);
		src.read((char*)&dst.segment_cnt, 2);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			dst.next_idx = std::byteswap(dst.next_idx);
			dst.prev_idx = std::byteswap(dst.prev_idx);
			dst.cur_idx = std::byteswap(dst.cur_idx);
			dst.start_segment = std::byteswap(dst.start_segment);
			dst.segment_cnt = std::byteswap(dst.segment_cnt);
		}

		return 0;
	}

	constexpr load_list_entry_f LOAD_LIST_ENTRY_FUNCS[] =
	{
		load_list_entry<TYPE_ATTRS[0]>, load_list_entry<TYPE_ATTRS[1]>,
		load_list_entry<TYPE_ATTRS[2]>, load_list_entry<TYPE_ATTRS[3]>,
		load_list_entry<TYPE_ATTRS[4]>, load_list_entry<TYPE_ATTRS[5]>
	};

	typedef uint16_t (*find_free_slot_f)(std::fstream &fstr);

	template <const type_attrs_t MAPPED_TYPE_ATTRS>
	static uint16_t find_free_slot(std::fstream &fstr)
	{
		char name0;
		uint16_t i;

		for(i = 0; i < MAPPED_TYPE_ATTRS.MAX_CNT; i++)
		{
			fstr.seekg(MAPPED_TYPE_ATTRS.LIST_ADDR + i * On_disk_sizes::LIST_ENTRY);
			name0 = fstr.get();

			if(!name0 || name0 == (char)0xFE) break;
		}

		return i;
	}

	constexpr find_free_slot_f FIND_FREE_SLOT_FUNCS[] =
	{
		nullptr, find_free_slot<TYPE_ATTRS[1]>, find_free_slot<TYPE_ATTRS[2]>,
		find_free_slot<TYPE_ATTRS[3]>, find_free_slot<TYPE_ATTRS[4]>,
		find_free_slot<TYPE_ATTRS[5]>
	};

	typedef void (*write_list_entry_f)(List_entry_t src, const uint16_t slot,
		std::fstream &dst);

	template <type_attrs_t TYPE_ATTRS>
	static void write_list_entry(List_entry_t src, const uint16_t slot,
							  std::fstream &dst)
	{
		if constexpr((u8)TYPE_ATTRS.ELEMENT_TYPE == 0)
			throw min_vfs::FS_err(ret_val_setup(LIBRARY_ID, (u8)ERR::WTF));

		const std::streampos entry_base = TYPE_ATTRS.LIST_ADDR + slot * On_disk_sizes::LIST_ENTRY;

		if constexpr(std::endian::native != ENDIANNESS)
		{
			src.next_idx = std::byteswap(src.next_idx);
			src.prev_idx = std::byteswap(src.prev_idx);
			src.cur_idx = std::byteswap(src.cur_idx);
			src.start_segment = std::byteswap(src.start_segment);
			src.segment_cnt = std::byteswap(src.segment_cnt);
		}

		dst.seekp(entry_base);

		dst.write(src.name, 16);
		dst.write((char*)&src.type, 1);

		dst.seekp(entry_base + (std::streamoff)0x12);

		dst.write((char*)&src.next_idx, 2);
		dst.write((char*)&src.prev_idx, 2);
		dst.write((char*)&src.cur_idx, 2);

		/*Valgrind complains about an uninitialized value here, but I can't
		 *figure it out.*/
		dst.seekp(entry_base + (std::streamoff)0x1B);

		dst.write((char*)&src.program_num, 1);
		dst.write((char*)&src.start_segment, 2);
		dst.write((char*)&src.segment_cnt, 2);
	}

	constexpr write_list_entry_f WRITE_LIST_ENTRY_FUNCS[6] =
	{
		nullptr, write_list_entry<TYPE_ATTRS[1]>,
		write_list_entry<TYPE_ATTRS[2]>, write_list_entry<TYPE_ATTRS[3]>,
		write_list_entry<TYPE_ATTRS[4]>, write_list_entry<TYPE_ATTRS[5]>
	};

	static constexpr u32 OS_size_from_media_type(const Media_type_t media_type)
	{
		switch(media_type)
		{
			using enum Media_type_t;

			case HDD_with_OS:
				return On_disk_sizes::OS;

			case HDD_with_OS_S760:
				return On_disk_sizes::OS + On_disk_sizes::S760_EXT_OS;

			default:
				return 0;
		}
	}

	typedef u16(*list_fentry_f)(filesystem_t &fs, const u16 idx,
								min_vfs::dentry_t &dst);

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static u16 list_fentry(filesystem_t &fs, const u16 idx,
						   min_vfs::dentry_t &dst)
	{
		if constexpr(!(u8)TYPE_ATTRS_ENTRY.ELEMENT_TYPE)
		{
			if(fs.header.media_type == Media_type_t::HDD_with_OS
				|| fs.header.media_type == Media_type_t::HDD_with_OS_S760)
				dst =
				{
					.fname = "OS",
					.fsize = OS_size_from_media_type(fs.header.media_type),
					.ctime = 0,
					.mtime = 0,
					.atime = 0,
					.ftype = min_vfs::ftype_t::file
				};

			return 0;
		}
		else
		{
			u16 cls_cnt;

			fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + idx
				* On_disk_sizes::LIST_ENTRY);
			dst =
			{
				.fname = std::string(16, '\0'),
				//pretend they have the header
				.fsize = (uintmax_t)(TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE),
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			};

			fs.stream.read(dst.fname.data(), 16);
			dst.fname = std::to_string(idx) + "-" + dst.fname;

			if(TYPE_ATTRS_ENTRY.ELEMENT_TYPE == Element_type_t::sample)
			{
				fs.stream.seekp(14, std::fstream::cur);
				fs.stream.read((char*)&cls_cnt, 2);

				if constexpr(ENDIANNESS != std::endian::native)
					cls_cnt = std::byteswap(cls_cnt);

				dst.fsize += cls_cnt * AUDIO_SEGMENT_SIZE;
			}

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);
			else return 0;
		}
	}

	constexpr list_fentry_f LIST_FUNCS[] =
	{
		list_fentry<TYPE_ATTRS[0]>, list_fentry<TYPE_ATTRS[1]>,
		list_fentry<TYPE_ATTRS[2]>, list_fentry<TYPE_ATTRS[3]>,
		list_fentry<TYPE_ATTRS[4]>, list_fentry<TYPE_ATTRS[5]>
	};

	static u16 list_dir_contents(filesystem_t &fs,
								 const std::vector<std::string> &split_path,
							  std::vector<min_vfs::dentry_t> &dentries,
								 const bool get_dir)
	{
		char name[17];
		u16 cls_cnt;

		const dir_map_t::const_iterator dir_it =
			DIR_NAME_TO_ATTRS.find(split_path[0]);

		if(dir_it == DIR_NAME_TO_ATTRS.end())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);

		const type_attrs_t &mapped_type_attrs = dir_it->second;

		if(!(u8)mapped_type_attrs.ELEMENT_TYPE)
		{
			if(fs.header.media_type == Media_type_t::HDD_with_OS
				|| fs.header.media_type == Media_type_t::HDD_with_OS_S760)
				dentries.push_back
				(
					{
						.fname = "OS",
						.fsize = OS_size_from_media_type(fs.header.media_type),
						.ctime = 0,
						.mtime = 0,
						.atime = 0,
						.ftype = min_vfs::ftype_t::file
					}
				);

			return 0;
		}

		if(get_dir)
		{
			dentries.push_back(ROOT_DIR[mapped_type_attrs.TYPE_IDX - 1]);
			return 0;
		}

		name[16] = 0;
		for(u16 i = 0, j = 0; i < mapped_type_attrs.MAX_CNT &&
			j < fs.header.TOC.*mapped_type_attrs.TOC_PTR; i++)
		{
			fs.stream.seekg(mapped_type_attrs.LIST_ADDR + i
				* On_disk_sizes::LIST_ENTRY);
			fs.stream.read(name, 16);

			if(name[0] && name[0] != (char)0xFE)
			{
				if(split_path.size() > 1 && split_path[1] != name) continue;

				dentries.push_back
				(
					{
						.fname = std::string(16, '\0'),
						//pretend they have the header
						.fsize =
							(uintmax_t)(mapped_type_attrs.PARAMS_ENTRY_SIZE),
						.ctime = 0,
						.mtime = 0,
						.atime = 0,
						.ftype = min_vfs::ftype_t::file
					}
				);

				std::memcpy(dentries.back().fname.data(), name, 16);
				dentries.back().fname = std::to_string(i) + "-"
					+ dentries.back().fname;

				if(mapped_type_attrs.ELEMENT_TYPE == Element_type_t::sample)
				{
					fs.stream.seekp(14, std::fstream::cur);
					fs.stream.read((char*)&cls_cnt, 2);

					if constexpr(ENDIANNESS != std::endian::native)
						cls_cnt = std::byteswap(cls_cnt);

					dentries.back().fsize += cls_cnt * AUDIO_SEGMENT_SIZE;
				}

				if(split_path.size() > 1) return 0;

				j++;
			}
		}

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		if(split_path.size() > 1)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);

		return 0;
	}

	typedef uint16_t (*find_slot_from_name_f)(filesystem_t &fs, const char name[]);

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static uint16_t find_slot_from_name(filesystem_t &fs, const char name[16])
	{
		u16 abs_idx, j;
		char tmp[16];

		for(abs_idx = 0, j = 0; abs_idx < TYPE_ATTRS_ENTRY.MAX_CNT && j < fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR; abs_idx++)
		{
			fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + abs_idx * On_disk_sizes::LIST_ENTRY);
			fs.stream.read(tmp, 16);

			if(tmp[0] && tmp[0] != (char)0xFE)
			{
				std::replace(tmp, tmp + 16, '/', '\\');
				if(!std::strncmp(tmp, name, 16)) break;

				j++;
			}
		}

		if(j == fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR) abs_idx = TYPE_ATTRS_ENTRY.MAX_CNT;

		return abs_idx;
	}

	constexpr find_slot_from_name_f find_volume_from_name = find_slot_from_name<TYPE_ATTRS[1]>;
	constexpr find_slot_from_name_f find_perf_from_name = find_slot_from_name<TYPE_ATTRS[2]>;
	constexpr find_slot_from_name_f find_patch_from_name = find_slot_from_name<TYPE_ATTRS[3]>;
	constexpr find_slot_from_name_f find_partial_from_name = find_slot_from_name<TYPE_ATTRS[4]>;
	constexpr find_slot_from_name_f find_sample_from_name = find_slot_from_name<TYPE_ATTRS[5]>;

	constexpr find_slot_from_name_f FIND_FROM_NAME_FUNCS[] =
	{
		nullptr, find_volume_from_name, find_perf_from_name,
		find_patch_from_name, find_partial_from_name, find_sample_from_name
	};

	static uint16_t reloc_cluster(filesystem_t &fs, const uint16_t cluster, const uint16_t offset)
	{
		//copy -> repoint -> free

		const u16 new_cluster = FAT_utils::find_next_free_cluster(fs.FAT.get(),
										FAT_ATTRS, fs.fat_attrs.LENGTH, offset);

		if(new_cluster == FAT_ATTRS.END_OF_CHAIN)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NO_SPACE_LEFT);

		char name0;
		u16 tgt_end_n_cls_val, tgt_end_o_cls_val, tgt_end_o_cls, rd_strt_cls;

		std::unique_ptr<char[]> cur_clus;

		cur_clus = std::make_unique<char[]>(AUDIO_SEGMENT_SIZE);

		fs.stream.seekg(On_disk_addrs::AUDIO_SECTION
			+ (cluster - FAT_ATTRS.DATA_MIN) * AUDIO_SEGMENT_SIZE);
		fs.stream.read(cur_clus.get(), AUDIO_SEGMENT_SIZE);

		fs.stream.seekp(On_disk_addrs::AUDIO_SECTION
			+ (new_cluster - FAT_ATTRS.DATA_MIN) * AUDIO_SEGMENT_SIZE);
		fs.stream.write(cur_clus.get(), AUDIO_SEGMENT_SIZE);

		cur_clus.reset();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		//mark new cluster
		tgt_end_o_cls_val = fs.FAT[cluster];

		if constexpr(ENDIANNESS != std::endian::native)
			tgt_end_o_cls_val = std::byteswap(tgt_end_o_cls_val);

		fs.stream.seekp(On_disk_addrs::FAT + 2 * new_cluster);
		fs.stream.write((char*)&tgt_end_o_cls_val, 2);

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		fs.FAT[new_cluster] = fs.FAT[cluster];

		if(cluster == FAT_ATTRS.END_OF_CHAIN) return 0;

		//update FAT
		tgt_end_n_cls_val = new_cluster;

		if constexpr(ENDIANNESS != std::endian::native)
			tgt_end_n_cls_val = std::byteswap(tgt_end_n_cls_val);

		for(u16 i = FAT_ATTRS.DATA_MIN; i < fs.fat_attrs.LENGTH; i++)
		{
			if(fs.FAT[i] == cluster)
			{
				fs.stream.seekp(On_disk_addrs::FAT + 2 * i);
				fs.stream.write((char*)&tgt_end_n_cls_val, 2);

				fs.FAT[i] = new_cluster;
			}
		}

		//update sample list
		tgt_end_o_cls = cluster;

		if constexpr(ENDIANNESS != std::endian::native)
			tgt_end_o_cls = std::byteswap(tgt_end_o_cls);

		for(u16 i = 0, j = 0; i < MAX_SAMPLE_COUNT && j < fs.header.TOC.sample_cnt; i++)
		{
			fs.stream.seekg(On_disk_addrs::SAMPLE_LIST + i
				* On_disk_sizes::LIST_ENTRY);
			name0 = fs.stream.get();

			if(name0 && name0 != (char)0xFE)
			{
				fs.stream.seekg(On_disk_addrs::SAMPLE_LIST + i
					* On_disk_sizes::LIST_ENTRY + 0x1C);
				fs.stream.read((char*)&rd_strt_cls, 2);

				if(rd_strt_cls == tgt_end_o_cls)
				{
					fs.stream.seekp(On_disk_addrs::SAMPLE_LIST + i
						* On_disk_sizes::LIST_ENTRY + 0x1C);
					fs.stream.write((char*)&tgt_end_n_cls_val, 2);
				}

				j++;
			}
		}

		//free
		tgt_end_n_cls_val = FAT_ATTRS.FREE_CLUSTER;

		if constexpr(ENDIANNESS != std::endian::native)
			tgt_end_n_cls_val = std::byteswap(tgt_end_n_cls_val);

		fs.stream.seekp(On_disk_addrs::FAT + 2 * cluster);
		fs.stream.write((char*)&tgt_end_n_cls_val, 2);
		fs.FAT[cluster] = FAT_ATTRS.FREE_CLUSTER;

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	static uint16_t free_OS_cls(filesystem_t &fs)
	{
		constexpr u16 FREE_CLS = ENDIANNESS != std::endian::native ?
			std::byteswap(FAT_ATTRS.FREE_CLUSTER) : FAT_ATTRS.FREE_CLUSTER;

		u16 err;

		fs.stream.seekp(On_disk_addrs::FAT + FAT_ATTRS.DATA_MIN * 2);

		for(u8 i = 0; i < S760_OS_CLUSTERS; i++)
		{
			fs.stream.write((char*)&FREE_CLS, 2);
			fs.FAT[i + FAT_ATTRS.DATA_MIN] = FAT_ATTRS.FREE_CLUSTER;
		}

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream, FAT_ATTRS,
									   fs.fat_attrs, (u16)1,
									   (u16)(fs.FAT[1] + S760_OS_CLUSTERS));
		if(err) return err;

		return 0;
	}

	typedef uint16_t(*count_clusters_f)(filesystem_t &fs, const uint16_t idx,
									std::unordered_map<u16, u16> &sample_map);

	static uint16_t count_clusters_partial(filesystem_t &fs, const uint16_t idx,
									std::unordered_map<u16, u16> &sample_map)
	{
		const std::streamoff list_entry_base = On_disk_addrs::PARTIAL_LIST + idx
			* On_disk_sizes::LIST_ENTRY;
		const std::streamoff params_entry_base = On_disk_addrs::PARTIAL_PARAMS
			+ idx * On_disk_sizes::PARTIAL_PARAMS_ENTRY;

		u16 cluster_cnt, temp;
		std::set<u16> local_s_set;

		for(u8 i = 0; i < 4; i++)
		{
			fs.stream.seekg(params_entry_base + 0x10 * (i + 1));
			fs.stream.read((char*)&temp, 2);

			if constexpr(ENDIANNESS != std::endian::native)
				temp = std::byteswap(temp);

			local_s_set.insert(temp);
		}

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		//it's probably better to split this into 2 steps because the first loop
		//would most likely read a single block and cache it. otherwise, we'd
		//keep seeking to fairly distant parts of the FS and reading stuff

		cluster_cnt = 0;

		for(const u16 index: local_s_set)
		{
			fs.stream.seekg(On_disk_addrs::SAMPLE_LIST + index * On_disk_sizes::LIST_ENTRY + 0x1E);
			fs.stream.read((char*)&temp, 2);

			if constexpr(ENDIANNESS != std::endian::native)
				temp = std::byteswap(temp);

			cluster_cnt += temp;
			sample_map[index] = temp;
		}

		if constexpr(ENDIANNESS != std::endian::native)
			cluster_cnt = std::byteswap(cluster_cnt);

		fs.stream.seekp(list_entry_base + 0x1E);
		fs.stream.write((char*)&cluster_cnt, 2);

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	template <const type_attrs_t TYPE_ATTRS_ENTRY,
		const type_attrs_ext_t TYPE_ATTRS_EXT_ENTRY, count_clusters_f NEXT_FUNC>
	static uint16_t count_clusters(filesystem_t &fs, const uint16_t idx,
								   std::unordered_map<u16, u16> &sample_map)
	{
		const std::streamoff list_entry_base = TYPE_ATTRS_ENTRY.LIST_ADDR + idx
			* On_disk_sizes::LIST_ENTRY;
		const std::streamoff params_entry_base = TYPE_ATTRS_ENTRY.PARAMS_ADDR
			+ idx * TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE;

		//might wanna move to the heap
		u16 indices[TYPE_ATTRS_EXT_ENTRY.IDX_CNT];
		u16 err, cluster_cnt;

		std::unordered_map<u16, u16> local_s_map;

		fs.stream.seekg(params_entry_base + 0x10
			+ TYPE_ATTRS_EXT_ENTRY.SIZE_BEFORE_INDICES);
		fs.stream.read((char*)indices, TYPE_ATTRS_EXT_ENTRY.IDX_CNT * 2);

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		for(u16 index: indices)
		{
			if constexpr(ENDIANNESS != std::endian::native)
				index = std::byteswap(index);

			err = NEXT_FUNC(fs, index, local_s_map);
			if(err) return err;
		}

		cluster_cnt = 0;

		for(const std::pair<u16, u16> entry: local_s_map)
		{
			cluster_cnt += entry.second;
			sample_map.insert(entry);
		}

		if constexpr(ENDIANNESS != std::endian::native)
			cluster_cnt = std::byteswap(cluster_cnt);

		fs.stream.seekp(list_entry_base + 0x1E);
		fs.stream.write((char*)&cluster_cnt, 2);

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	constexpr count_clusters_f count_patch_clusters = count_clusters<
		TYPE_ATTRS[(u8)ftype_IDs::PATCHES],
		TYPE_ATTRS_EXT[(u8)ftype_IDs::PATCHES], count_clusters_partial>;
	constexpr count_clusters_f count_perf_clusters = count_clusters<
		TYPE_ATTRS[(u8)ftype_IDs::PERFS], TYPE_ATTRS_EXT[(u8)ftype_IDs::PERFS],
		count_patch_clusters>;
	constexpr count_clusters_f count_volume_clusters = count_clusters<
		TYPE_ATTRS[(u8)ftype_IDs::VOLS], TYPE_ATTRS_EXT[(u8)ftype_IDs::VOLS],
		count_perf_clusters>;

	constexpr u16 DUMMY_CLS_CNT = 0xFFFF;

	static constexpr uint16_t size_to_clusters(const uintmax_t size)
	{
		if(size > MAX_SAMPLE_SIZE) return DUMMY_CLS_CNT;

		return size / AUDIO_SEGMENT_SIZE + ((size % AUDIO_SEGMENT_SIZE) != 0);
	}

	static uint16_t write_chain(filesystem_t &fs, const std::vector<u16> &chain)
	{
		u16 err;

		err = FAT_utils::write_chain(fs.stream, FAT_ATTRS, fs.fat_attrs, chain);
		if(err) return err;

		err = FAT_utils::write_chain(fs.FAT.get(), FAT_ATTRS,
									 fs.fat_attrs.LENGTH, chain);
		if(err) return err;

		return 0;
	}

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static bool file_exists(filesystem_t &fs, const u16 idx)
	{
		fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + idx
			* On_disk_sizes::LIST_ENTRY);
		const u8 name0 = fs.stream.get();

		return name0 && name0 != 0xFE;
	}

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static uint16_t unzero_all_before(filesystem_t &fs, const u16 idx)
	{
		for(u16 i = 0, j = 0; i < idx && i < TYPE_ATTRS_ENTRY.MAX_CNT && j <
			fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR; i++)
		{
			fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + i
				* On_disk_sizes::LIST_ENTRY);

			if(!fs.stream.peek()) fs.stream.put(0xFE);
		}

		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR)
			* !fs.stream.good();
	}

	typedef uint16_t(*truncate_file_f)(filesystem_t &fs,
									   List_entry_t &list_entry, uintmax_t size,
									const bool is_new);

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static uint16_t alloc_file(filesystem_t &fs, List_entry_t &list_entry,
							   const uintmax_t size, const bool is_new)
	{
		/*size is ignored here and only exists to meet the truncate_file_f
		interface*/
		u16 temp;

		if(!is_new) return 0;

		list_entry.type = TYPE_ATTRS_ENTRY.ELEMENT_TYPE;
		list_entry.program_num = 0;
		list_entry.segment_cnt = 0;
		list_entry.start_segment = 0;

		write_list_entry<TYPE_ATTRS_ENTRY>(list_entry, list_entry.cur_idx, fs.stream);
		fs.stream.flush();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		temp = unzero_all_before<TYPE_ATTRS_ENTRY>(fs, list_entry.cur_idx);
		if(temp) return temp;

		fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR += 1;
		temp = fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR;

		if constexpr(ENDIANNESS != std::endian::native)
			temp = std::byteswap(temp);

		fs.stream.seekp(On_disk_addrs::TOC
			+ TYPE_ATTRS_EXT[TYPE_ATTRS_ENTRY.TYPE_IDX].ON_DISK_TOC_OFFSET);
		fs.stream.write((char*)&temp, 2);
		fs.stream.flush();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	static constexpr Media_type_t media_type_from_OS_size(const uint32_t size)
	{
		if(!size) return Media_type_t::HDD;

		return size > On_disk_sizes::OS ?
			Media_type_t::HDD_with_OS_S760 : Media_type_t::HDD_with_OS;
	}

	static uint16_t reloc_OS_clusters(filesystem_t &fs)
	{
		u16 err, sp_os_cls_val, temp;

		//Might wanna check FAT[1], too.
		if(fs.fat_attrs.LENGTH - FAT_ATTRS.DATA_MIN < S760_OS_CLUSTERS)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NO_SPACE_LEFT);

		sp_os_cls_val = 0xFFFE;
		temp = 0xFFFE;

		if constexpr(ENDIANNESS != std::endian::native)
			temp = std::byteswap(temp);

		err = 0;
		for(u8 i = FAT_ATTRS.DATA_MIN; i < S760_OS_CLUSTERS
			+ FAT_ATTRS.DATA_MIN; i++)
		{
			if((fs.FAT[i] >= FAT_ATTRS.DATA_MIN && fs.FAT[i]
				<= FAT_ATTRS.DATA_MAX) || fs.FAT[i] == FAT_ATTRS.END_OF_CHAIN)
			{
				err = reloc_cluster(fs, i, S760_OS_CLUSTERS
					+ FAT_ATTRS.DATA_MIN);

				if(err) return err;
			}

			if(i == 57 + FAT_ATTRS.DATA_MIN)
			{
				sp_os_cls_val = 0xFFFD;
				temp = 0xFFFD;

				if constexpr(ENDIANNESS != std::endian::native)
					temp = std::byteswap(temp);
			}

			fs.stream.seekp(On_disk_addrs::FAT + 2 * i);
			fs.stream.write((char*)&temp, 2);

			fs.FAT[i] = sp_os_cls_val;
		}

		/*NOTE: Right now, we assume all clusters will be either free or in use.
		In reality, they may also be reserved (anything outside of the DATA_MIN
		- DATA_MAX range other than FREE_CLS and END_OF_CHAIN). The loop already
		checks to relocate only clusters in range or marked as EOC, this is
		about the fixed subtraction from FAT[1].
		I suppose the right thing to do would be to add an else to the loop's
		range/EOC check and subtract 1 from a count initialized to
		S760_OS_CLUSTERS if FAT[i] != FREE_CLS. Then we would subtract that
		count from FAT[i].*/

		//update FAT's free cls cnt
		err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream, FAT_ATTRS,
									   fs.fat_attrs, (u16)1,
									   (u16)(fs.FAT[1] - S760_OS_CLUSTERS));
		return err;
	}

	static uint16_t truncate_OS(filesystem_t &fs, List_entry_t &list_entry,
								const uintmax_t size, const bool is_new)
	{
		/*is_new is ignored here and only exists to meet the truncate_file_f
		interface*/

		if(size > On_disk_sizes::OS + On_disk_sizes::S760_EXT_OS)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FILE_TOO_LARGE);

		const Media_type_t new_media_type = media_type_from_OS_size(size);
		u16 err;

		if(new_media_type != fs.header.media_type)
		{
			if(new_media_type == Media_type_t::HDD_with_OS_S760)
			{
				err = reloc_OS_clusters(fs);
				if(err) return err;
			}
			else if(fs.header.media_type == Media_type_t::HDD_with_OS_S760)
			{
				err = free_OS_cls(fs);
				if(err) return err;
			}
		}

		fs.stream.seekp(0x10);
		fs.stream.put((u8)new_media_type);
		fs.stream.flush();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		fs.header.media_type = new_media_type;

		return 0;
	}

	static uint16_t shrink_chain(filesystem_t &fs,
								 const std::vector<u16> &chain,
								 const u16 cls_cnt)
	{
		u16 err;

		err = FAT_utils::shrink_chain(fs.stream, FAT_ATTRS, fs.fat_attrs, chain,
									  cls_cnt);
		if(err) return err;

		err = FAT_utils::shrink_chain(fs.FAT.get(), FAT_ATTRS,
									  fs.fat_attrs.LENGTH, chain, cls_cnt);
		if(err) return err;

		const s32 diff = cls_cnt - chain.size();

		err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream, FAT_ATTRS,
									   fs.fat_attrs, (u16)1,
									   (u16)(fs.FAT[1] - diff));
		if(err) return err;

		return 0;
	}

	static uint16_t truncate_sample(filesystem_t &fs, List_entry_t &list_entry,
									uintmax_t size, const bool is_new)
	{
		constexpr u8 MIN_SIZE = On_disk_sizes::SAMPLE_PARAMS_ENTRY;
		u16 err;
		std::vector<u16> chain;

		if(size < MIN_SIZE) size = 0;
		else size -= MIN_SIZE;

		const u16 tgt_cls_cnt = size_to_clusters(size);
		const bool should_grow = tgt_cls_cnt > list_entry.segment_cnt;

		if(tgt_cls_cnt == DUMMY_CLS_CNT)
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FILE_TOO_LARGE);

		if(!is_new)
		{
			err = FAT_utils::follow_chain(fs.FAT.get(), FAT_ATTRS,
										  fs.fat_attrs.LENGTH,
								 list_entry.start_segment, chain);

			if(err && err != ret_val_setup(FAT_utils::LIBRARY_ID,
				(u8)FAT_utils::ERR::BAD_START)) return err;
		}

		list_entry.type = Element_type_t::sample;

		if(is_new || should_grow)
		{
			const s32 diff = tgt_cls_cnt - chain.size();

			if(should_grow)
			{
				if(tgt_cls_cnt > fs.FAT[1])
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NO_SPACE_LEFT);

				err = FAT_utils::find_free_chain(fs.stream, FAT_ATTRS,
												 fs.fat_attrs, tgt_cls_cnt,
									 chain);
				if(err) return err;
			}

			list_entry.segment_cnt = is_new ? 0 : chain.size();
			list_entry.start_segment = list_entry.segment_cnt ? chain[0]
				: FAT_ATTRS.END_OF_CHAIN;

			write_list_entry<TYPE_ATTRS[5]>(list_entry, list_entry.cur_idx,
											fs.stream);
			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);

			err = unzero_all_before<TYPE_ATTRS[5]>(fs, list_entry.cur_idx);
			if(err) return err;

			if(list_entry.segment_cnt)
			{
				err = write_chain(fs, chain);
				if(err) return err;

				err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream,
											   FAT_ATTRS, fs.fat_attrs, (u16)1,
											   (u16)(fs.FAT[1] - diff));
				if(err) return err;
			}

			if(is_new)
			{
				fs.header.TOC.sample_cnt++;
				err = fs.header.TOC.sample_cnt;

				if constexpr(ENDIANNESS != std::endian::little)
					err = std::byteswap(err);

				fs.stream.seekp(On_disk_addrs::TOC
					+ TYPE_ATTRS_EXT[5].ON_DISK_TOC_OFFSET);
				fs.stream.write((char*)&err, 2);
			}

			fs.stream.flush();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);
		}
		else if(tgt_cls_cnt < list_entry.segment_cnt)
		{
			list_entry.segment_cnt = tgt_cls_cnt;
			list_entry.start_segment = list_entry.segment_cnt && chain.size()
				? chain[0] : FAT_ATTRS.END_OF_CHAIN;

			write_list_entry<TYPE_ATTRS[5]>(list_entry, list_entry.cur_idx,
											fs.stream);
			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);

			err = shrink_chain(fs, chain, tgt_cls_cnt);
			if(err) return err;

			fs.stream.flush();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::IO_ERROR);
		}

		return 0;
	}

	constexpr truncate_file_f TRUNC_FUNCS[6] =
	{
		truncate_OS, alloc_file<TYPE_ATTRS[1]>, alloc_file<TYPE_ATTRS[2]>,
		alloc_file<TYPE_ATTRS[3]>, alloc_file<TYPE_ATTRS[4]>,
		truncate_sample
	};

	typedef uint16_t(*delete_file_f)(filesystem_t &fs, const uint16_t idx);

	static uint16_t delete_OS(filesystem_t &fs)
	{
		u16 err;

		if(fs.header.media_type != Media_type_t::HDD_with_OS
			&& fs.header.media_type != Media_type_t::HDD_with_OS_S760) return 0;

		if(fs.header.media_type == Media_type_t::HDD_with_OS_S760)
		{
			err = free_OS_cls(fs);
			if(err) return err;
		}

		fs.stream.seekp(15);
		fs.stream.put((u8)Media_type_t::HDD);
		fs.stream.flush();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::IO_ERROR);

		fs.header.media_type = Media_type_t::HDD;
		return 0;
	}

	template <const type_attrs_t &TYPE_ATTRS_ENTRY>
	static uint16_t delete_file(filesystem_t &fs, const uint16_t idx)
	{
		char name0;
		u16 err;
		std::vector<u16> chain;
		std::ostreambuf_iterator<std::fstream::char_type,
			std::fstream::traits_type> obi(fs.stream);

		fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + idx
			* On_disk_sizes::LIST_ENTRY);
		name0 = fs.stream.get();

		if(!name0 || name0 == (char)0xFE)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::EMPTY_ENTRY);

		//HW does it, but do we wanna bother? It should be pointless.
		obi = fs.stream.seekp(TYPE_ATTRS_ENTRY.PARAMS_ADDR + idx
			* TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE);
		for(u16 i = 0; i < TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE; i++)
			*obi++ = 0xFF;

		if constexpr(TYPE_ATTRS_ENTRY.ELEMENT_TYPE == Element_type_t::sample)
		{
			u16 start_cluster, cluster_cnt;

			//chain setup
			fs.stream.seekg(TYPE_ATTRS_ENTRY.LIST_ADDR + idx
				* On_disk_sizes::LIST_ENTRY + 0x1C);
			fs.stream.read((char*)&start_cluster, 2);
			fs.stream.read((char*)&cluster_cnt, 2);

			if constexpr(ENDIANNESS != std::endian::native)
			{
				start_cluster = std::byteswap(start_cluster);
				cluster_cnt = std::byteswap(cluster_cnt);
			}

			err = FAT_utils::follow_chain(fs.FAT.get(), FAT_ATTRS,
										  fs.fat_attrs.LENGTH, start_cluster,
								 chain);
			if(err) return err;

			if(chain.size() != cluster_cnt)
				return ret_val_setup(LIBRARY_ID, (u8)ERR::CHAIN_SIZE_MISMATCH);

			//free chain
			err = FAT_utils::free_chain(fs.stream, FAT_ATTRS, fs.fat_attrs,
										chain);
			if(err) return err;

			err = FAT_utils::free_chain(fs.FAT.get(), FAT_ATTRS,
										fs.fat_attrs.LENGTH, chain);
			if(err) return err;

			err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream, FAT_ATTRS,
										   fs.fat_attrs, (u16)1,
										   (u16)(fs.FAT[1] + chain.size()));
			if(err) return err;
		}

		obi = fs.stream.seekp(TYPE_ATTRS_ENTRY.LIST_ADDR + idx
			* On_disk_sizes::LIST_ENTRY);

		*obi++ = 0xFE;
		for(u8 i = 0; i < On_disk_sizes::LIST_ENTRY - 1; i++) *obi++ = 0;

		fs.header.TOC.*TYPE_ATTRS_ENTRY.TOC_PTR -= 1;
		err = write_TOC(fs.header.TOC, fs.stream);

		fs.stream.flush();

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)ERR::IO_ERROR);

		return 0;
	}

	constexpr delete_file_f DELETE_FUNCS[] =
	{
		nullptr,
		delete_file<TYPE_ATTRS[1]>,
		delete_file<TYPE_ATTRS[2]>,
		delete_file<TYPE_ATTRS[3]>,
		delete_file<TYPE_ATTRS[4]>,
		delete_file<TYPE_ATTRS[5]>
	};

	static uint16_t delete_dir(filesystem_t &fs,
							   const type_attrs_t &mapped_type_attrs)
	{
		const std::string DIR_PATH = "/"
			+ std::string(DIR_NAMES[mapped_type_attrs.TYPE_IDX]) + "/";
		const u16 INITIAL_CNT = fs.header.TOC.*mapped_type_attrs.TOC_PTR;

		u16 err, local_err;

		err = 0;

		for(u16 i = 0, j = 0; i < mapped_type_attrs.MAX_CNT && j < INITIAL_CNT;
			i++)
		{
			if(fs.open_files.contains(DIR_PATH + std::to_string(i)))
			{
				err |= ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::ALREADY_OPEN);
				j++;
				continue;
			}

			local_err = DELETE_FUNCS[(u8)mapped_type_attrs.TYPE_IDX](fs, i);

			if(!local_err) j++;
			else
			{
				if(local_err != ret_val_setup(LIBRARY_ID, (u8)ERR::EMPTY_ENTRY))
					return local_err;
			}
		}

		return err;
	}

	typedef uint16_t (*read_file_f)(filesystem_t &fs,
									internal_file_t &internal_file,
									uintmax_t &pos, uintmax_t len, void *dst);

	static void check_and_extend_os_S770(filesystem_t &fs)
	{
		if(fs.header.media_type != Media_type_t::HDD_with_OS
			&& fs.header.media_type != Media_type_t::HDD_with_OS_S760)
		{
			fs.stream.seekp(16);
			fs.stream.put((char)Media_type_t::HDD_with_OS);

			fs.header.media_type = Media_type_t::HDD_with_OS;
		}
	}

	static uint16_t check_and_extend_os_S760(filesystem_t &fs)
	{
		if(fs.header.media_type != Media_type_t::HDD_with_OS_S760)
		{
			const u16 err = reloc_OS_clusters(fs);
			if(err) return err;

			fs.stream.seekp(16);
			fs.stream.put((char)Media_type_t::HDD_with_OS_S760);

			fs.header.media_type = Media_type_t::HDD_with_OS_S760;
		}

		return 0;
	}

	template <const bool write>
	static uint16_t read_write_OS(filesystem_t &fs,
								  internal_file_t &internal_file,
								  uintmax_t &pos, uintmax_t len, void *dst)
	{
		u32 dst_off = 0;

		if constexpr(write)
		{
			fs.mtx.lock();
			check_and_extend_os_S770(fs);
			fs.mtx.unlock();
		}
		else
		{
			if(fs.header.media_type != Media_type_t::HDD_with_OS
				&& fs.header.media_type != Media_type_t::HDD_with_OS_S760)
			{
				return ret_val_setup(min_vfs::LIBRARY_ID,
									 (u8)min_vfs::ERR::END_OF_FILE);
			}
		}

		if(pos < On_disk_sizes::OS)
		{
			constexpr u8 base_pos = 0;
			const u32 local_pos = pos - base_pos;
			const u32 remaining = On_disk_sizes::OS - local_pos;
			const u32 local_len = std::min((uintmax_t)remaining, len);

			fs.mtx.lock();
			fs.stream.seekg(On_disk_addrs::OS + local_pos);
			if constexpr(write) fs.stream.write((char*)dst + dst_off, local_len);
			else fs.stream.read((char*)dst + dst_off, local_len);
			fs.mtx.unlock();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			len -= local_len;
			pos += local_len;
			if(!len) return 0;

			dst_off += local_len;
		}

		if constexpr(write)
		{
			fs.mtx.lock();
			const u16 err = check_and_extend_os_S760(fs);
			fs.mtx.unlock();

			if(err) return err;
		}
		else
		{
			if(fs.header.media_type != Media_type_t::HDD_with_OS_S760)
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
		}

		if(pos < On_disk_sizes::S760_EXT_OS + On_disk_sizes::OS)
		{
			constexpr u32 base_pos = On_disk_sizes::OS;
			const u32 local_pos = pos - base_pos;
			const u32 remaining = On_disk_sizes::S760_EXT_OS - local_pos;
			const u32 local_len = std::min((uintmax_t)remaining, len);

			fs.mtx.lock();
			fs.stream.seekg(On_disk_addrs::AUDIO_SECTION + local_pos);
			if constexpr(write) fs.stream.write((char*)dst + dst_off, local_len);
			else fs.stream.read((char*)dst + dst_off, local_len);
			fs.mtx.unlock();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			len -= local_len;
			pos += local_len;
			if(!len) return 0;
		}

		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	}

	static uint16_t get_or_alloc_next_cls(filesystem_t &fs, const u16 cur_cls, List_entry_t &list_entry)
	{
		u16 next_cls, err;

		err = FAT_utils::get_next_or_free_cluster(fs.FAT.get(), FAT_ATTRS,
												  fs.fat_attrs.LENGTH, cur_cls,
											next_cls);
		if(err)
		{
			if(err == ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::ALLOC))
			{
				if(next_cls != FAT_ATTRS.END_OF_CHAIN)
				{
					if(!list_entry.segment_cnt)
						list_entry.start_segment = next_cls;

					list_entry.segment_cnt++;

					WRITE_LIST_ENTRY_FUNCS[1 + ((u8)list_entry.type - (u8)Element_type_t::volume)](list_entry, list_entry.cur_idx, fs.stream);
					if(!fs.stream.good())
						throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR));

					err = FAT_utils::extend_chain(fs.FAT.get(), fs.stream, FAT_ATTRS, fs.fat_attrs, cur_cls, next_cls);
					if(err)
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

					err = FAT_utils::write_cluster(fs.FAT.get(), fs.stream, FAT_ATTRS, fs.fat_attrs, (u16)1, (u16)(fs.FAT[1] - 1));
					if(err)
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);
				}
			}
			else throw min_vfs::FS_err(err);
		}

		return next_cls;
	}

	static uint16_t get_or_alloc_nth_cls(filesystem_t &fs, u16 &cls,
										 const u16 start_cls_idx,
										 internal_file_t &internal_file,
										 const uintmax_t pos)
	{
		u16 err;
		err = FAT_utils::get_nth_cluster(fs.FAT.get(), FAT_ATTRS,
										 fs.fat_attrs.LENGTH, cls,
								   start_cls_idx);

		if(err)
		{
			if(err == ret_val_setup(FAT_utils::LIBRARY_ID, (uint8_t)FAT_utils::ERR::CHAIN_OOB))
				return err;
			else
			{
				//TODO: Check that this does not grow files if size == 0.
				err = TRUNC_FUNCS[5](fs, internal_file.list_entry, pos + 1, false);
				if(err) return err;

				cls = internal_file.list_entry.start_segment;
				err = FAT_utils::get_nth_cluster(fs.FAT.get(), FAT_ATTRS, fs.fat_attrs.LENGTH, cls, start_cls_idx);
				if(err) return err;
			}
		}

		return 0;
	}

	//TODO: Clean this shit up.
	template <const bool write, const bool first_cls>
	static u16 get_cls_then_read_write(filesystem_t &fs,
									   internal_file_t &internal_file, u16 &cls,
									   void *const dst, const u16 len,
									   const u16 start_cls_idx,
									   const uintmax_t pos,
									   const u16 cls_pos_off)
	{
		if constexpr(first_cls)
		{
			if constexpr(write)
			{
				const u16 err = get_or_alloc_nth_cls(fs, cls, start_cls_idx,
													 internal_file, pos);

				if(err) return err;
			}
			else
			{
				const u16 err = FAT_utils::get_nth_cluster(fs.FAT.get(),
														FAT_ATTRS,
														fs.fat_attrs.LENGTH,
														cls, start_cls_idx);

				if(err)
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
			}
		}
		else
		{
			if constexpr(write)
			{
				try
				{
					cls = get_or_alloc_next_cls(fs, cls, internal_file.list_entry);
				}
				catch(min_vfs::FS_err e)
				{
					return e.err_code;
				}
			}
			else cls = fs.FAT[cls];
		}

		if(cls < FAT_ATTRS.DATA_MIN || cls > FAT_ATTRS.DATA_MAX)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);

		fs.stream.seekg(On_disk_addrs::AUDIO_SECTION + AUDIO_SEGMENT_SIZE *
			(cls - FAT_ATTRS.DATA_MIN) + cls_pos_off);

		if constexpr(write) fs.stream.write((char*)dst, len);
		else fs.stream.read((char*)dst, len);

		if(!fs.stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	template <const bool write>
	static uint16_t read_write_sample(filesystem_t &fs,
									  internal_file_t &internal_file,
									  uintmax_t &pos, uintmax_t len, void *dst)
	{
		u32 dst_off = 0;

		if(pos < On_disk_sizes::SAMPLE_PARAMS_ENTRY)
		{
			constexpr u8 base_pos = 0;
			const u32 local_pos = pos - base_pos;
			const u32 remaining = On_disk_sizes::SAMPLE_PARAMS_ENTRY - local_pos;
			const u8 local_len = std::min((uintmax_t)remaining, len);

			fs.mtx.lock();
			fs.stream.seekg(On_disk_addrs::SAMPLE_PARAMS
				+ On_disk_sizes::SAMPLE_PARAMS_ENTRY
				* internal_file.list_entry.cur_idx + local_pos);

			if constexpr(write) fs.stream.write((char*)dst + dst_off, local_len);
			else fs.stream.read((char*)dst + dst_off, local_len);
			fs.mtx.unlock();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			len -= local_len;
			pos += local_len;
			if(!len) return 0;

			dst_off += local_len;
		}

		if(pos < MAX_SAMPLE_SIZE + On_disk_sizes::SAMPLE_PARAMS_ENTRY)
		{
			constexpr u8 base_pos = On_disk_sizes::SAMPLE_PARAMS_ENTRY;
			const u32 local_pos = pos - base_pos;
			const u32 file_size = internal_file.list_entry.segment_cnt
				* AUDIO_SEGMENT_SIZE;
			const u32 remaining = write
				? (MAX_SAMPLE_SIZE > local_pos ? MAX_SAMPLE_SIZE - local_pos
					: 0)
				: (file_size > local_pos ? file_size - local_pos : 0);
			const u32 local_len = std::min((uintmax_t)remaining, len);
			const u16 start_cls_idx = local_pos / AUDIO_SEGMENT_SIZE;
			const u16 pos_in_first_cls = local_pos - start_cls_idx
				* AUDIO_SEGMENT_SIZE;
			const u16 first_cls_len = std::min((u32)(AUDIO_SEGMENT_SIZE
				- pos_in_first_cls), local_len);
			const u16 whole_cls = (local_len - first_cls_len)
				/ AUDIO_SEGMENT_SIZE;

			u16 err, cls = internal_file.list_entry.start_segment;

			fs.mtx.lock();
			err = get_cls_then_read_write<write, true>(fs, internal_file,
													cls, (char*)dst + dst_off,
													first_cls_len,
													start_cls_idx, pos,
													pos_in_first_cls);
			fs.mtx.unlock();

			if(err) return err;

			len -= first_cls_len;
			pos += first_cls_len;
			dst_off += first_cls_len;

			for(u16 i = 0; i < whole_cls; i++)
			{
				/*We must lock before getting the next cluster and only unlock
				 *after reading/writing its data; otherwise, writing/truncating
				 *the OS may relocate a cluster after we get its address but
				 *before we read/write its data, which would lead to accessing
				 *data at the old cluster address.*/
				fs.mtx.lock();
				err = get_cls_then_read_write<write, false>(fs, internal_file,
													cls, (char*)dst + dst_off,
													AUDIO_SEGMENT_SIZE, 0, 0,
													0);
				fs.mtx.unlock();

				if(err) return err;

				len -= AUDIO_SEGMENT_SIZE;
				pos += AUDIO_SEGMENT_SIZE;
				dst_off += AUDIO_SEGMENT_SIZE;
			}

			if(len)
			{
				fs.mtx.lock();
				err = get_cls_then_read_write<write, false>(fs, internal_file,
													cls, (char*)dst + dst_off,
													len, 0, 0, 0);
				fs.mtx.unlock();

				if(err) return err;

				pos += len;
				len = 0;
			}

			if(!len) return 0;
		}

		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	}

	template <const type_attrs_t TYPE_ATTRS_ENTRY, const bool write>
	static uint16_t read_write_file(filesystem_t &fs,
									internal_file_t &internal_file,
									uintmax_t &pos, uintmax_t len, void *dst)
	{
		u32 dst_off = 0;

		if(pos < TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE)
		{
			constexpr u8 base_pos = 0;
			const u32 local_pos = pos - base_pos;
			const u32 remaining = TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE - local_pos;
			const u16 local_len = std::min((uintmax_t)remaining, len);

			fs.mtx.lock();
			fs.stream.seekg(TYPE_ATTRS_ENTRY.PARAMS_ADDR
				+ TYPE_ATTRS_ENTRY.PARAMS_ENTRY_SIZE
				* internal_file.list_entry.cur_idx + local_pos);

			if constexpr(write) fs.stream.write((char*)dst + dst_off, local_len);
			else fs.stream.read((char*)dst + dst_off, local_len);
			fs.mtx.unlock();

			if(!fs.stream.good())
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

			len -= local_len;
			pos += local_len;
			if(!len) return 0;
		}

		return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	}

	constexpr read_file_f READ_FUNCS[6] =
	{
		read_write_OS<false>, read_write_file<TYPE_ATTRS[1], false>,
		read_write_file<TYPE_ATTRS[2], false>, read_write_file<TYPE_ATTRS[3], false>,
		read_write_file<TYPE_ATTRS[4], false>, read_write_sample<false>
	};

	constexpr read_file_f WRITE_FUNCS[6] =
	{
		read_write_OS<true>, read_write_file<TYPE_ATTRS[1], true>,
		read_write_file<TYPE_ATTRS[2], true>, read_write_file<TYPE_ATTRS[3], true>,
		read_write_file<TYPE_ATTRS[4], true>, read_write_sample<true>
	};

	static u16 find_from_name_or_free(filesystem_t &fs, const u8 type_id, const char fname[16])
	{
		u16 idx;

		idx = FIND_FROM_NAME_FUNCS[type_id](fs, fname);

		if(idx >= TYPE_ATTRS[type_id].MAX_CNT)
			idx = FIND_FREE_SLOT_FUNCS[type_id](fs.stream);

		return idx;
	}

	//A.K.A. mount
	filesystem_t::filesystem_t(const char *path): min_vfs::filesystem_t(path)
	{
		u16 err;
		const uintmax_t disk_size = std::filesystem::file_size(this->path);

		//base constructor already does basic checks

		if(disk_size < On_disk_sizes::HEADER)
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::WRONG_FS));

		err = load_header(stream, header);
		if(err) throw min_vfs::FS_err(err);

		if(!stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

		if(std::memcmp(header.machine_name, MACHINE_NAME, 10))
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::WRONG_FS));

		if(!is_HDD((u8)header.media_type))
			throw min_vfs::FS_err(ret_val_setup(LIBRARY_ID,
												(u8)ERR::MEDIA_TYPE_NOT_HDD));

		if(disk_size < MIN_DISK_SIZE)
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
											(u8)min_vfs::ERR::DISK_TOO_SMALL));

		err = load_TOC(stream, header.TOC);
		if(err) throw min_vfs::FS_err(err);

		if(!this->stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

		if(disk_size / BLK_SIZE < header.TOC.block_cnt)
			throw(min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
										(u8)min_vfs::ERR::FS_SIZE_MISMATCH)));

		fat_attrs = FAT_utils::FAT_dyna_attrs_t(FAT_find_length(stream),
												On_disk_addrs::FAT);

		if(!this->stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

		//might wanna make it a !=
		if(header.TOC.block_cnt < On_disk_addrs::AUDIO_SECTION / BLK_SIZE
			+ (fat_attrs.LENGTH - FAT_ATTRS.DATA_MIN)
			* (AUDIO_SEGMENT_SIZE / BLK_SIZE))
			throw(min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
										(u8)min_vfs::ERR::FS_SIZE_MISMATCH)));

		stream.seekg(On_disk_addrs::FAT);
		FAT = std::make_unique<u16[]>(fat_attrs.LENGTH);
		stream.read((char*)FAT.get(), fat_attrs.LENGTH * 2);

		if(!stream.good())
			throw min_vfs::FS_err(ret_val_setup(min_vfs::LIBRARY_ID,
												(u8)min_vfs::ERR::IO_ERROR));

		if constexpr(ENDIANNESS != std::endian::native)
		{
			for(u16 i = 1; i < fat_attrs.LENGTH; i++)
				FAT[i] = std::byteswap(FAT[i]);
		}
	}

	filesystem_t& filesystem_t::operator=(filesystem_t &&other) noexcept
	{
		if(this == &other) return *this;

		this->path = other.path;
		this->stream.swap(other.stream);

		this->header = other.header;
		this->fat_attrs = other.fat_attrs;
		this->FAT = std::move(other.FAT);

		/*This shouldn't work, especially since we're copying. I'm not sure
		 *moving a map would leave its nodes intact anyway. It seems like a bad
		 *idea to allow moving filesystems. Or maybe limit it to filesystems
		 *with no open files, but then that means move-assignments can fail.
		 *The base class' move-assignments can already fail if the actual types
		 *of both filesystems don't match, so that might be a reasonable
		 *compromise.*/
		this->open_files = other.open_files;

		return *this;
	}

	min_vfs::filesystem_t& filesystem_t::operator=(min_vfs::filesystem_t &&other) noexcept
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

	uint16_t filesystem_t::list(const char *file_path,
								std::vector<min_vfs::dentry_t> &dentries,
								const bool get_dir)
	{
		std::vector<std::string> split_path;

		split_path = str_util::split(std::string(file_path), std::string("/"));

		switch(split_path.size())
		{
			case 0:
				{
					if(get_dir)
					{
						dentries.push_back
						(
							{
								.fname = "/",
								.fsize = BLK_SIZE, //Bold-faced lie
								.ctime = 0,
								.mtime = 0,
								.atime = 0,
								.ftype = min_vfs::ftype_t::dir
							}
						);
						return 0;
					}

					if(this->header.media_type == Media_type_t::HDD_with_OS
						|| this->header.media_type == Media_type_t::HDD_with_OS_S760)
					{
						dentries.push_back
						(
							{
								.fname = std::string(DIR_NAMES[0]),
								.fsize = OS_size_from_media_type(
									this->header.media_type),
								.ctime = 0,
								.mtime = 0,
								.atime = 0,
								.ftype = min_vfs::ftype_t::file
							}
						);
					}

					for(u8 i = 0; i < 5; i++)
						dentries.push_back(ROOT_DIR[i]);
				}
				return 0;

			case 1:
				return list_dir_contents(*this, split_path, dentries, get_dir);

			case 2:
			{
				std::string final_name;
				const dir_map_t::const_iterator dir_it =
					DIR_NAME_TO_ATTRS.find(split_path[0]);

				if(dir_it == DIR_NAME_TO_ATTRS.end())
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NOT_FOUND);

				const type_attrs_t &mapped_type_attrs = dir_it->second;
				const u16 idx = mapped_type_attrs.TYPE_IDX
					? idx_and_name_from_name(split_path[1], final_name) : 0;

				if(idx >= mapped_type_attrs.MAX_CNT)
				{
					split_path[1] = final_name;
					return list_dir_contents(*this, split_path, dentries,
											 false);
				}
				else
					return LIST_FUNCS[mapped_type_attrs.TYPE_IDX](*this, idx,
													dentries.emplace_back());
			}

			default:
				return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_PATH);
		}

		return 0;
	}

	uint16_t filesystem_t::mkdir(const char *dir_path)
	{
		return ret_val_setup(min_vfs::LIBRARY_ID,
							 (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	}

	uint16_t filesystem_t::ftruncate(const char *path, const uintmax_t new_size)
	{
		bool is_new, valid_idx;
		u16 idx;
		std::string final_name, abs_path;

		std::vector<u16> chain;
		std::vector<std::string> split_path;
		List_entry_t list_entry;

		split_path = str_util::split(std::string(path), std::string("/"));

		if(split_path.size() != 2 && !(split_path.size() == 1
			&& split_path[0] == DIR_NAMES[0]))
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::INVALID_PATH);

		const dir_map_t::const_iterator dir_it =
			DIR_NAME_TO_ATTRS.find(split_path[0]);

		if(dir_it == DIR_NAME_TO_ATTRS.end())
			return ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);

		const type_attrs_t &mapped_type_attrs = dir_it->second;
		idx = mapped_type_attrs.TYPE_IDX ? idx_and_name_from_name(split_path[1],
																  final_name)
			: 0;

		/*I hate this. I've spent a while rewriting it to be less shitty and
		it's still disgusting. fopen's version of this is even worse; so fuck
		it, this will have to do.*/

		is_new = false;

		if(mapped_type_attrs.TYPE_IDX)
		{
			if(idx >= mapped_type_attrs.MAX_CNT)
				idx = FIND_FROM_NAME_FUNCS[mapped_type_attrs.TYPE_IDX](*this,
															final_name.data());

			if(idx >= mapped_type_attrs.MAX_CNT)
			{
				idx = FIND_FREE_SLOT_FUNCS[mapped_type_attrs.TYPE_IDX](
					this->stream);

				if(idx >= mapped_type_attrs.MAX_CNT)
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::NO_SPACE_LEFT);

				is_new = true;
			}
			else
				abs_path = "/" + split_path[0] + "/" + std::to_string(idx);
		}
		else abs_path = "/OS";

		if(!is_new)
		{
			const file_map_iterator_t fmap_it = open_files.find(abs_path);

			if(fmap_it != open_files.end())
			{
				std::pair<uintmax_t, internal_file_t> &file = fmap_it->second;
				return TRUNC_FUNCS[mapped_type_attrs.TYPE_IDX](*this,
														file.second.list_entry,
														new_size, false);
			}
			else
			{
				const u16 err =
					LOAD_LIST_ENTRY_FUNCS[mapped_type_attrs.TYPE_IDX](stream,
															idx, list_entry);
				is_new = err == ret_val_setup(LIBRARY_ID,
											  (uint8_t)ERR::EMPTY_ENTRY);

				if(!is_new && err)
					return ret_val_setup(min_vfs::LIBRARY_ID,
										 (u8)min_vfs::ERR::FAILED_TO_OPEN_FILE);
			}
		}

		if(is_new)
		{
			std::memcpy(list_entry.name, final_name.data(), 16);
			list_entry.name[16] = 0;
			list_entry.cur_idx = idx;
			list_entry.segment_cnt = 0;
		}

		return TRUNC_FUNCS[mapped_type_attrs.TYPE_IDX](*this, list_entry,
													   new_size, is_new);
	}

	uint16_t filesystem_t::rename(const char *cur_path, const char *new_path)
	{
		/*NOTES:
			1. Maybe it would be better to just take the references to two
			std::vectors of std::strings, since the shell/file manager is
			already going to have to split the paths to figure out that we're
			renaming and not moving.

			2. Internal moves are not supported, because file data (not just the
			directory entry) is tied to a specific slot for a specific
			directory.

			3. I might wanna check that the new filename's index matches the
			current one, but that could be needlessly restrictive (we can
			trivially ignore the new filename's index). The reason we don't
			allow changing indices through rename, like we do bank nums for
			EMU's FS, is the same reason we don't support internal moves.
		*/
		u16 idx;

		std::string final_name;
		std::vector<std::string> split_cur_path, split_new_path;

		split_cur_path = str_util::split(std::string(cur_path), std::string("/"));
		split_new_path = str_util::split(std::string(new_path), std::string("/"));

		if(split_new_path.size() != 2
			|| split_new_path.size() != split_cur_path.size()
			|| split_new_path[0] != split_cur_path[0])
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);

		const dir_map_t::const_iterator dir_it = DIR_NAME_TO_ATTRS.find(split_cur_path[0]);

		if(dir_it == DIR_NAME_TO_ATTRS.end())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

		const type_attrs_t &mapped_type_attrs = dir_it->second;
		idx = idx_and_name_from_name(split_cur_path[1], final_name);

		if(idx >= mapped_type_attrs.MAX_CNT)
			idx = FIND_FROM_NAME_FUNCS[mapped_type_attrs.TYPE_IDX](*this, final_name.data());

		if(idx >= mapped_type_attrs.MAX_CNT)
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

		//disallow renaming open files
		if(open_files.contains(split_cur_path[0] + "/" + std::to_string(idx)))
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

		//here we only want the name
		idx_and_name_from_name(split_new_path[1], final_name);
		stream.seekp(mapped_type_attrs.LIST_ADDR + idx * On_disk_sizes::LIST_ENTRY);
		stream.write(final_name.data(), 16);
		stream.flush();

		if(!stream.good())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR);

		return 0;
	}

	uint16_t filesystem_t::remove(const char *path)
	{
		bool was_idx;
		u16 err, idx;

		std::vector<std::string> split_path;

		split_path = str_util::split(std::string(path), std::string("/"));

		/*TODO: Clean up the open_files mess.*/

		switch(split_path.size())
		{
			case 0:
				if(open_files.contains("/OS"))
					err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
				else
					err = delete_OS(*this);

				for(u8 i = 1; i < 6; i++)
				{
					err |= delete_dir(*this, TYPE_ATTRS[i]);
				}

				return err;

			case 1:
			{
				const dir_map_t::const_iterator dir_it = DIR_NAME_TO_ATTRS.find(split_path[0]);

				if(dir_it == DIR_NAME_TO_ATTRS.end())
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

				const type_attrs_t &mapped_type_attrs = dir_it->second;

				if(!(u8)mapped_type_attrs.ELEMENT_TYPE)
				{
					if(open_files.contains("/OS"))
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

					return delete_OS(*this);
				}

				return delete_dir(*this, mapped_type_attrs);
			}

			case 2:
			{
				const dir_map_t::const_iterator dir_it = DIR_NAME_TO_ATTRS.find(split_path[0]);
				std::string cleaned_fname;

				if(dir_it == DIR_NAME_TO_ATTRS.end())
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

				const type_attrs_t &mapped_type_attrs = dir_it->second;
				idx = idx_and_name_from_name(split_path[1], cleaned_fname);

				if(idx >= mapped_type_attrs.MAX_CNT)
					idx = FIND_FROM_NAME_FUNCS[mapped_type_attrs.TYPE_IDX](*this, cleaned_fname.data());

				if(idx >= mapped_type_attrs.MAX_CNT)
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

				if(open_files.contains("/" + split_path[0] + "/" + std::to_string(idx)))
					return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);

				return DELETE_FUNCS[mapped_type_attrs.TYPE_IDX](*this, idx);
			}

			default:
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
		}

		return 0;
	}

	uint16_t filesystem_t::fopen_internal(const char *path, void **internal_file)
	{
		u16 err, idx;
		List_entry_t list_entry;
		std::string fname;

		const std::vector<std::string> split_path = str_util::split(std::string(path), std::string("/"));

		if(!split_path.size())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

		const dir_map_t::const_iterator dir_it = DIR_NAME_TO_ATTRS.find(split_path[0]);

		if(dir_it == DIR_NAME_TO_ATTRS.end())
			return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);

		const type_attrs_t &mapped_type_attrs = dir_it->second;

		if(!(u8)mapped_type_attrs.ELEMENT_TYPE)
		{
			if(split_path.size() != 1)
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

			const std::string OS_ABS_PATH = "/OS";
			const file_map_iterator_t fmap_it = open_files.find(OS_ABS_PATH);

			if(fmap_it != open_files.end())
			{
				std::pair<uintmax_t, internal_file_t> &file = fmap_it->second;
				*internal_file = &file.second;
				file.first++;
				return 0;
			}
			else
			{
				fname = OS_ABS_PATH;
				list_entry.type = (Element_type_t)0;
				idx = 0;
			}
		}
		else
		{
			if(split_path.size() != 2)
				return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);

			idx = idx_and_name_from_name(split_path[1], fname);

			if(idx >= mapped_type_attrs.MAX_CNT)
				idx = FIND_FROM_NAME_FUNCS[mapped_type_attrs.TYPE_IDX](*this, fname.data());

			const std::string ABS_DIR_PATH = "/" + split_path[0] + "/";
			const file_map_iterator_t fmap_it = open_files.find(ABS_DIR_PATH + std::to_string(idx));

			if(fmap_it != open_files.end())
			{
				std::pair<uintmax_t, internal_file_t> &file = fmap_it->second;
				*internal_file = &file.second;
				file.first++;
				return 0;
			}
			else
			{
				if(idx < mapped_type_attrs.MAX_CNT)
					err = LOAD_LIST_ENTRY_FUNCS[mapped_type_attrs.TYPE_IDX](this->stream, idx, list_entry);
				else
				{
					idx = FIND_FREE_SLOT_FUNCS[mapped_type_attrs.TYPE_IDX](this->stream);

					if(idx >= mapped_type_attrs.MAX_CNT)
						return ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);

					err = ret_val_setup(LIBRARY_ID, (uint8_t)ERR::EMPTY_ENTRY);
				}

				//new file
				if(err == ret_val_setup(LIBRARY_ID, (uint8_t)ERR::EMPTY_ENTRY))
				{
					std::memcpy(list_entry.name, fname.data(), 16);
					list_entry.name[16] = 0;
					list_entry.cur_idx = idx;
					list_entry.segment_cnt = 0;

					err = TRUNC_FUNCS[mapped_type_attrs.TYPE_IDX](*this,
																  list_entry, 0,
																  true);
				}
			}

			if(err) return err;

			fname = ABS_DIR_PATH + std::to_string(idx);
		}

		list_entry.cur_idx = idx;

		std::pair<uintmax_t, internal_file_t> file_entry;

		file_entry.first = 1;
		file_entry.second.type_idx = mapped_type_attrs.TYPE_IDX;
		file_entry.second.list_entry = list_entry;

		std::pair<file_map_iterator_t, bool> map_entry = open_files.emplace(fname, file_entry);

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
			open_files.erase(cast_ptr->map_entry->first);
		else cast_ptr->map_entry->second.first--;

		return 0;
	}

	uint16_t filesystem_t::read(void *internal_file, uintmax_t &pos, uintmax_t len, void *dst)
	{
		/*I guess I could make the internal file itself hold a function pointer
		to its read and write functions.*/
		return READ_FUNCS[((internal_file_t*)internal_file)->type_idx](*this, *((internal_file_t*)internal_file), pos, len, dst);
	}

	uint16_t filesystem_t::write(void *internal_file, uintmax_t &pos, uintmax_t len, void *src)
	{
		return WRITE_FUNCS[((internal_file_t*)internal_file)->type_idx](*this, *((internal_file_t*)internal_file), pos, len, src);
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
