#ifndef S7XX_FS_TEST_DATA_INCLUDE_GUARD
#define S7XX_FS_TEST_DATA_INCLUDE_GUARD

#include <string_view>
#include <vector>

#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S7XX/fs_drv_constants.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"

namespace S7XX::FS::testing
{
	constexpr char TEST_FS_PATH[] = "test_fs.img";
	constexpr char EXT_FS_PATH[] = "test_ext_fs";
	constexpr char INS_FS_PATH[] = "test_ins_fs.img";

	constexpr std::string_view DIR_NAMES[] =
	{
		"OS", //actually a file
		"Volumes",
		"Performances",
		"Patches",
		"Partials",
		"Samples"
	};

	constexpr S7XX::FS::Media_type_t EXPECTED_MEDIA_TYPE = S7XX::FS::Media_type_t::HDD;
	constexpr S7XX::FS::TOC_t EXPECTED_TOC =
	{
		"Test S7XX FS    ",
		0x28000,
		1,
		1,
		5,
		5,
		5
	};

	constexpr S7XX::FS::Header_t EXPECTED_HEADER =
	{
		"S770 MR25A",
		EXPECTED_MEDIA_TYPE,
		"S-7XX FS, formatted by sample_thing",
		EXPECTED_TOC
	};

	constexpr S7XX::FS::Media_type_t EXPECTED_INS_MEDIA_TYPE = S7XX::FS::Media_type_t::HDD;
	constexpr S7XX::FS::TOC_t EXPECTED_INS_TOC =
	{
		"Test ins S7XX FS",
		0x28000,
		0,
		0,
		0,
		0,
		0
	};

	constexpr S7XX::FS::Header_t EXPECTED_INS_HEADER =
	{
		"S770 MR25A",
		EXPECTED_INS_MEDIA_TYPE,
		"S-7XX FS, formatted by sample_thing",
		EXPECTED_INS_TOC
	};

	constexpr u8 EXPECTED_DIR_CNT = sizeof(DIR_NAMES) / sizeof(DIR_NAMES[0]);

	//Native FS names
	namespace NATIVE_FS_DIRS
	{
		const std::vector<min_vfs::dentry_t> EXPECTED_ROOT_DIR =
		{
			{
				.fname = std::string("Volumes"),
				.fsize = S7XX::FS::MAX_VOLUME_COUNT
					* S7XX::FS::On_disk_sizes::LIST_ENTRY,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::dir
			},
			{
				.fname = std::string("Performances"),
				.fsize = S7XX::FS::MAX_PERF_COUNT
					* S7XX::FS::On_disk_sizes::LIST_ENTRY,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::dir
			},
			{
				.fname = std::string("Patches"),
				.fsize = S7XX::FS::MAX_PATCH_COUNT
					* S7XX::FS::On_disk_sizes::LIST_ENTRY,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::dir
			},
			{
				.fname = std::string("Partials"),
				.fsize = S7XX::FS::MAX_PARTIAL_COUNT
					* S7XX::FS::On_disk_sizes::LIST_ENTRY,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::dir
			},
			{
				.fname = std::string("Samples"),
				.fsize = S7XX::FS::MAX_SAMPLE_COUNT
					* S7XX::FS::On_disk_sizes::LIST_ENTRY,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::dir
			}
		};

		const std::vector<min_vfs::dentry_t> EXPECTED_SAMPLE_DIR =
		{
			{
				.fname = std::string("0-TST:Test_01   \x7FL"),
				.fsize = S7XX::FS::TYPE_ATTRS[5].PARAMS_ENTRY_SIZE + 0x5E
					* S7XX::AUDIO_SEGMENT_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("1-TST:Test_01   \x7FR"),
				.fsize = S7XX::FS::TYPE_ATTRS[5].PARAMS_ENTRY_SIZE + 0x5E
					* S7XX::AUDIO_SEGMENT_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("2-TST:Test_02     "),
				.fsize = S7XX::FS::TYPE_ATTRS[5].PARAMS_ENTRY_SIZE + 0x26
					* S7XX::AUDIO_SEGMENT_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("3-TST:Test_03     "),
				.fsize = S7XX::FS::TYPE_ATTRS[5].PARAMS_ENTRY_SIZE + 0x08
					* S7XX::AUDIO_SEGMENT_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("4-TST:Test_04     "),
				.fsize = S7XX::FS::TYPE_ATTRS[5].PARAMS_ENTRY_SIZE + 0x84
					* S7XX::AUDIO_SEGMENT_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			}
		};

		const std::vector<min_vfs::dentry_t> EXPECTED_PARTIAL_DIR =
		{
			{
				.fname = std::string("0-TST:Test_01     "),
				.fsize = S7XX::FS::TYPE_ATTRS[4].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("1-TST:Test_02     "),
				.fsize = S7XX::FS::TYPE_ATTRS[4].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("2-TST:Test_03     "),
				.fsize = S7XX::FS::TYPE_ATTRS[4].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("3-TST:Test_04     "),
				.fsize = S7XX::FS::TYPE_ATTRS[4].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("4-TST:Test_05     "),
				.fsize = S7XX::FS::TYPE_ATTRS[4].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			}
		};

		const std::vector<min_vfs::dentry_t> EXPECTED_PATCH_DIR =
		{
			{
				.fname = std::string("0-TST:Test_01     "),
				.fsize = S7XX::FS::TYPE_ATTRS[3].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("1-TST:Test_02     "),
				.fsize = S7XX::FS::TYPE_ATTRS[3].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("2-TST:Test_03     "),
				.fsize = S7XX::FS::TYPE_ATTRS[3].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("3-TST:Test_04     "),
				.fsize = S7XX::FS::TYPE_ATTRS[3].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			},
			{
				.fname = std::string("4-TST:Test_05     "),
				.fsize = S7XX::FS::TYPE_ATTRS[3].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			}
		};

		const std::vector<min_vfs::dentry_t> EXPECTED_PERF_DIR =
		{
			{
				.fname = std::string("0-TST:Test_01     "),
				.fsize = S7XX::FS::TYPE_ATTRS[2].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			}
		};

		const std::vector<min_vfs::dentry_t> EXPECTED_VOLUME_DIR =
		{
			{
				.fname = std::string("0-TST:Test_01     "),
				.fsize = S7XX::FS::TYPE_ATTRS[1].PARAMS_ENTRY_SIZE,
				.ctime = 0,
				.mtime = 0,
				.atime = 0,
				.ftype = min_vfs::ftype_t::file
			}
		};
	}
}
#endif // !S7XX_FS_TEST_DATA_INCLUDE_GUARD
