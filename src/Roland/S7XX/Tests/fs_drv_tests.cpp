#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <string_view>
#include <iostream>
#include <algorithm>
#include <utility>

#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"
#include "Roland/S7XX/host_FS_utils.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"
#include "Roland/S7XX/fs_drv_constants.hpp"
#include "Helpers.hpp"
#include "Test_data.hpp"
#include "Host_FS_test_data.hpp"
#include "Utils/utils.hpp"
#include "min_vfs/min_vfs_base.hpp"

using namespace S7XX::FS::testing;

static std::vector<min_vfs::dentry_t> ROOT_DIR =
{
	{
		.fname = std::string(DIR_NAMES[0]),
		.fsize = 0, // only variable
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	//everything else could be const
	{
		.fname = std::string(DIR_NAMES[1]),
		.fsize = (uintmax_t)(S7XX::FS::TYPE_ATTRS[1].MAX_CNT * S7XX::FS::On_disk_sizes::LIST_ENTRY),
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	},
	{
		.fname = std::string(DIR_NAMES[2]),
		.fsize = (uintmax_t)(S7XX::FS::TYPE_ATTRS[2].MAX_CNT * S7XX::FS::On_disk_sizes::LIST_ENTRY),
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	},
	{
		.fname = std::string(DIR_NAMES[3]),
		.fsize = (uintmax_t)(S7XX::FS::TYPE_ATTRS[3].MAX_CNT * S7XX::FS::On_disk_sizes::LIST_ENTRY),
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	},
	{
		.fname = std::string(DIR_NAMES[4]),
		.fsize = (uintmax_t)(S7XX::FS::TYPE_ATTRS[4].MAX_CNT * S7XX::FS::On_disk_sizes::LIST_ENTRY),
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	},
	{
		.fname = std::string(DIR_NAMES[5]),
		.fsize = (uintmax_t)(S7XX::FS::TYPE_ATTRS[5].MAX_CNT * S7XX::FS::On_disk_sizes::LIST_ENTRY),
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	}
};

int fsck_tests()
{
	constexpr char test_file_name[] = "test_file.bin";

	u16 err, expected_err, fsck_status;

	std::ofstream test_ofs;
	std::ostreambuf_iterator obi = test_ofs;

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = S7XX::FS::fsck("nx_file", fsck_status);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 47);
		return 47;
	}

	test_ofs.open(test_file_name);
	if(!test_ofs.is_open() || !test_ofs.good())
	{
		std::cerr << "IO error!!!" << std::endl;
		std::cerr << "Exit: 48" << std::endl;
		return 48;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);
	err = S7XX::FS::fsck(test_file_name, fsck_status);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 49);
		return 49;
	}

	for(u32 i = 0; i < S7XX::FS::MIN_DISK_SIZE; i++) *obi++ = 0xAA;
	test_ofs.close();

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);
	err = S7XX::FS::fsck(test_file_name, fsck_status);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 51);
		return 51;
	}

	//TODO: FS with magic but too small.

	std::filesystem::remove(test_file_name);

	//Test known good FS. Should have no errors.
	fsck_status = 0;
	err = S7XX::FS::fsck(TEST_FS_PATH, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 50);
		return 50;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors in known good FS!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 52" << std::endl;
		return 52;
	}

	return 0;
}

//Expects that mounting will already be tested. This is already done by main.
//Expects that fsck will already be tested.
int mkfs_tests(std::unique_ptr<S7XX::FS::filesystem_t> &dst_fs)
{
	constexpr uintmax_t ins_fs_tgt_size = 80 * 1024 * 1024;

	u16 err, expected_err, fsck_status;

	expected_err = ret_val_setup(S7XX::FS::LIBRARY_ID, (u8)S7XX::FS::ERR::INVALID_PATH);
	err = S7XX::FS::mkfs("nx_file", "SHOULD NOT EXIST");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 37);
		return 37;
	}

	if(std::filesystem::exists(INS_FS_PATH))
	{
		if(std::filesystem::is_regular_file(INS_FS_PATH))
			std::filesystem::remove(INS_FS_PATH);
		else
		{
			std::cerr << "Something exists at " << INS_FS_PATH << ", but it shouldn't!!!" << std::endl;
			std::cerr << "Exit: 38" << std::endl;
			return 38;
		}
	}
		
	std::ofstream temp(INS_FS_PATH);

	if(!temp.is_open() || !temp.good())
	{
		std::cerr << "Failed to create file!!!" << std::endl;
		std::cerr << "Exit: 39" << std::endl;
		return 39;
	}
	temp.close();

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::DISK_TOO_SMALL);
	err = S7XX::FS::mkfs(INS_FS_PATH, "SHOULD NOT EXIST");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 40);
		return 40;
	}
	
	if(std::filesystem::file_size(INS_FS_PATH) < ins_fs_tgt_size)
		std::filesystem::resize_file(INS_FS_PATH, ins_fs_tgt_size);
	
	expected_err = 0;
	err = S7XX::FS::mkfs(INS_FS_PATH, "Test ins S7XX FS");
	if(err)
	{
		print_unexpected_err(err, 41);
		return 41;
	}

	try
	{
		dst_fs = std::make_unique<S7XX::FS::filesystem_t>(INS_FS_PATH);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 42" << std::endl;
		return 42;
	}

	if(dst_fs->header.media_type != EXPECTED_INS_MEDIA_TYPE)
	{
		std::cerr << "Unexpected media type!!!" << std::endl;
		std::cerr << "Expected: 0x" << std::hex << (u16)EXPECTED_INS_MEDIA_TYPE;
		std::cerr << ", got 0x" << (u16)dst_fs->header.media_type << std::dec << std::endl;
		std::cerr << "Exit: 43" << std::endl;
		return 43;
	}

	if(!TOCcmp(dst_fs->header.TOC, EXPECTED_INS_TOC))
	{
		std::cerr << "TOC mismatch!!!" << std::endl;
		std::cerr << "Expected TOC: ";
		print_TOC(dst_fs->header.TOC);
		std::cerr << "Got: ";
		print_TOC(EXPECTED_TOC);
		std::cerr << "Exit: 44" << std::endl;
		return 44;
	}

	fsck_status = 0;
	err = S7XX::FS::fsck(INS_FS_PATH, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 45);
		return 45;
	}

	if(fsck_status)
	{
		std::cerr << "New FS failed fsck!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 46" << std::endl;
		return 46;
	}

	return 0;
}

int mount_tests(std::unique_ptr<S7XX::FS::filesystem_t> &test_fs)
{
	u16 err, expected_err;

	std::string path_str;

	expected_err = 0;

	try
	{
		test_fs = std::make_unique<S7XX::FS::filesystem_t>("nx_disk");
	}
	catch(min_vfs::FS_err e)
	{
		expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NONEXISTANT_DISK);

		if(e.err_code != expected_err)
		{
			print_expected_err(expected_err, e.err_code, 53);
			return 53;
		}
	}

	if(!expected_err)
	{
		std::cerr << "Mount should have thrown an exception but didn't!!!" << std::endl;
		std::cerr << "Exit: 54" << std::endl;
		return 54;
	}

	path_str = "test_dir";
	std::filesystem::create_directory(path_str);

	try
	{
		test_fs = std::make_unique<S7XX::FS::filesystem_t>(path_str.c_str());
	}
	catch(min_vfs::FS_err e)
	{
		expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_A_FILE);
		std::filesystem::remove(path_str);

		if(e.err_code != expected_err)
		{
			print_expected_err(expected_err, e.err_code, 55);
			return 55;
		}
	}

	if(!expected_err)
	{
		std::filesystem::remove(path_str);
		std::cerr << "Mount should have thrown an exception but didn't!!!" << std::endl;
		std::cerr << "Exit: 56" << std::endl;
		return 56;
	}

	try
	{
		test_fs = std::make_unique<S7XX::FS::filesystem_t>(TEST_FS_PATH);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 1" << std::endl;
		return 1;
	}

	if(test_fs->header.media_type != EXPECTED_MEDIA_TYPE)
	{
		std::cerr << "Unexpected media type!!!" << std::endl;
		std::cerr << "Expected: 0x" << std::hex << (u16)EXPECTED_MEDIA_TYPE;
		std::cerr << ", got 0x" << (u16)test_fs->header.media_type
			<< std::dec << std::endl;
		std::cerr << "Exit: 2" << std::endl;
		return 2;
	}

	if(!TOCcmp(test_fs->header.TOC, EXPECTED_TOC))
	{
		std::cerr << "TOC mismatch!!!" << std::endl;
		std::cerr << "Expected TOC: ";
		print_TOC(test_fs->header.TOC);
		std::cerr << "Got: ";
		print_TOC(EXPECTED_TOC);
		std::cerr << "Exit: 3" << std::endl;
		return 3;
	}

	return 0;
}

static int list_tests(S7XX::FS::filesystem_t &fs)
{
	u16 err, expected_err;
	min_vfs::dentry_t expected_dentry;

	std::vector<min_vfs::dentry_t> dentries;

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(S7XX::FS::LIBRARY_ID, (u8)S7XX::FS::ERR::INVALID_PATH);
	err = fs.list("1/2/3/4", dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 33);
		return 33;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = fs.list("DIR", dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 34);
		return 34;
	}

	dentries.clear();
	err = fs.list("/Samples/8192-", dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 132);
		return 132;
	}

	if(dentries.size() != 0)
	{
		std::cerr << "Bad dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 133" << std::endl;
		return 133;
	}
	/*------------------------End of common err testing-----------------------*/

	/*-----------------------------Root list tests----------------------------*/
	expected_err = 0;
	dentries.clear();
	err = fs.list("/", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 4);
		return 4;
	}

	err = check_file_list(ROOT_DIR, dentries, 1);
	if(err)
	{
		std::cerr << "Exit: " << err + 4 << std::endl;
		return err + 4;
	}

	fs.header.media_type = S7XX::FS::Media_type_t::HDD_with_OS;
	ROOT_DIR[0].fsize = S7XX::FS::On_disk_sizes::OS;
	dentries.clear();
	err = fs.list("", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 7);
		return 7;
	}

	err = check_file_list(ROOT_DIR, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 7 << std::endl;
		return err + 7;
	}

	//List root dir's dentry
	dentries.clear();
	err = fs.list("", dentries, true);
	if(err)
	{
		print_unexpected_err(err, 515);
		return 515;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 516 << std::endl;
		return 516;
	}

	expected_dentry =
	{
		.fname = "/",
		.fsize = S7XX::FS::BLK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	};

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 517 << std::endl;
		return 517;
	}
	/*-------------------------End of root list tests-------------------------*/

	/*------------------------------OS list tests-----------------------------*/
	dentries.clear();
	err = fs.list("/OS", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 10);
		return 10;
	}

	err = check_file_list({ROOT_DIR[0]}, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 10 << std::endl;
		return err + 10;
	}

	dentries.clear();
	err = fs.list("OS", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 13);
		return 13;
	}

	err = check_file_list({ROOT_DIR[0]}, dentries, 0);
	if(err) return err + 13;

	fs.header.media_type = S7XX::FS::Media_type_t::HDD;
	ROOT_DIR[0].fsize = 0;
	dentries.clear();
	err = fs.list("OS", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 16);
		return 16;
	}

	if(dentries.size())
	{
		std::cerr << "Filename list should be empty but isn't!!!" << std::endl;
		std::cerr << "Filenames:" << std::endl;
		for(const min_vfs::dentry_t &dentry: dentries)
			std::cerr << dentry.fname << std::endl;

		std::cerr << "Exit: 17" << std::endl;
		return 17;
	}
	/*--------------------------End of OS list tests--------------------------*/

	/*--------------------------------List dirs-------------------------------*/
	dentries.clear();
	err = fs.list("/Samples", dentries, true);
	if(err)
	{
		print_unexpected_err(err, 518);
		return 518;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 519 << std::endl;
		return 519;
	}

	expected_dentry =
	{
		.fname = "Samples",
		.fsize = S7XX::FS::On_disk_sizes::LIST_ENTRY
			* S7XX::FS::MAX_SAMPLE_COUNT,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	};

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 520 << std::endl;
		return 520;
	}

	dentries.clear();
	err = fs.list("/Volumes", dentries, true);
	if(err)
	{
		print_unexpected_err(err, 521);
		return 521;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 522 << std::endl;
		return 522;
	}

	expected_dentry =
	{
		.fname = "Volumes",
		.fsize = S7XX::FS::On_disk_sizes::LIST_ENTRY
			* S7XX::FS::MAX_VOLUME_COUNT,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	};

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 523 << std::endl;
		return 523;
	}
	/*----------------------------End of list dirs----------------------------*/

	/*----------------------------List dir contents---------------------------*/
	dentries.clear();
	err = fs.list("/Samples", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 18);
		return 18;
	}

	err = check_file_list(NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR, dentries , 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 18 << std::endl;
		return err + 18;
	}

	dentries.clear();
	err = fs.list("/Partials", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 21);
		return 21;
	}

	err = check_file_list(NATIVE_FS_DIRS::EXPECTED_PARTIAL_DIR, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 21 << std::endl;
		return err + 21;
	}

	dentries.clear();
	err = fs.list("/Patches", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 24);
		return 24;
	}

	err = check_file_list(NATIVE_FS_DIRS::EXPECTED_PATCH_DIR, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 24 << std::endl;
		return err + 24;
	}

	dentries.clear();
	err = fs.list("/Performances", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 27);
		return 27;
	}

	err = check_file_list(NATIVE_FS_DIRS::EXPECTED_PERF_DIR, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 27 << std::endl;
		return err + 27;
	}

	dentries.clear();
	err = fs.list("/Volumes", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 30);
		return 30;
	}

	err = check_file_list(NATIVE_FS_DIRS::EXPECTED_VOLUME_DIR, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << err + 30 << std::endl;
		return err + 30;
	}
	/*------------------------End of list dir contents------------------------*/

	/*--------------------------------List file-------------------------------*/
	dentries.clear();
	err = fs.list("/Samples/0-TST:Test_01   \x7FL", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 105);
		return 105;
	}

	err = check_file_list({NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0]}, dentries,
						  0);
	if(err)
	{
		std::cerr << "Exit: " << err + 105 << std::endl;
		return err + 105;
	}

	dentries.clear();
	err = fs.list("/Samples/0-", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 108);
		return 108;
	}

	err = check_file_list({NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0]}, dentries, 0);
	if(err) return err + 108;

	dentries.clear();
	err = fs.list("/Samples/TST:Test_01   \x7FL", dentries);
	if(err != expected_err)
	{
		print_unexpected_err(err, 111);
		return 111;
	}

	err = check_file_list({NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0]}, dentries, 0);
	if(err) return err + 111;
	/*----------------------------End of list file----------------------------*/

	return 0;
}

static int del_tests()
{
	constexpr char S7XX_FS[] = "del_fs.img";

	u16 err, expected_err, fsck_status;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 57" << std::endl;
		return 57;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = s7xx_fs->remove("nx_dir/nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 58);
		return 58;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = s7xx_fs->remove("nx_dir");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 59);
		return 59;
	}

	err = s7xx_fs->remove("nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 60);
		return 60;
	}

	err = s7xx_fs->remove("Samples/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 61);
		return 61;
	}

	err = s7xx_fs->remove("/Samples/8192-");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 134);
		return 134;
	}

	expected_err = ret_val_setup(S7XX::FS::LIBRARY_ID, (u8)S7XX::FS::ERR::EMPTY_ENTRY);
	err = s7xx_fs->remove("Samples/8191-");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 62);
		return 62;
	}
	/*------------------------End of common err testing-----------------------*/

	/*------------------------------Delete sample-----------------------------*/
	fsck_status = s7xx_fs->header.TOC.sample_cnt;
	err = s7xx_fs->remove("Samples/0-");
	if(err)
	{
		print_unexpected_err(err, 63);
		return 63;
	}

	if(s7xx_fs->header.TOC.sample_cnt != fsck_status - 1)
	{
		std::cerr << "Failed to update TOC!!!" << std::endl;
		std::cerr << "Expected: " << fsck_status - 1 << std::endl;
		std::cerr << "Actual: " << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 64" << std::endl;
		return 64;
	}
	/*--------------------------End of delete sample--------------------------*/

	/*--------------------------------Delete OS-------------------------------*/
	err = s7xx_fs->remove("OS");
	if(err)
	{
		print_unexpected_err(err, 65);
		return 65;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD)
	{
		std::cerr << "Failed to update media type!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD) << std::endl;
		std::cerr << "Actual: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 66" << std::endl;
		return 66;
	}
	/*----------------------------End of delete OS----------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 67);
		return 67;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 68" << std::endl;
		return 68;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int trunc_tests()
{
	constexpr char S7XX_FS[] = "trunc_fs.img";
	min_vfs::dentry_t expected_dentry;
	S7XX::FS::Media_type_t media_type;

	u16 err, expected_err, fsck_status, count, cls;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::vector<min_vfs::dentry_t> dentries;
	std::vector<u16> chain;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 69" << std::endl;
		return 69;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = s7xx_fs->ftruncate("nx_dir/nx_dir/nx_file", 69);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 70);
		return 70;
	}

	err = s7xx_fs->ftruncate("nx_dir", 69);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 71);
		return 71;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = s7xx_fs->ftruncate("nx_dir/nx_file", 69);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 72);
		return 72;
	}
	/*------------------------End of common err testing-----------------------*/

	/*-----------------------------Truncate sample----------------------------*/
	count = s7xx_fs->header.TOC.sample_cnt + 1;
	expected_dentry =
	{
		.fname = "8191-                ",
		.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	err = s7xx_fs->ftruncate("Samples/8191-", 0);
	if(err)
	{
		print_unexpected_err(err, 75);
		return 75;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 76" << std::endl;
		return 76;
	}

	err = s7xx_fs->list("/Samples/", dentries);
	if(err)
	{
		print_unexpected_err(err, 77);
		return 77;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		
		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 78" << std::endl;
		return 78;
	}

	expected_dentry.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
		+ S7XX::AUDIO_SEGMENT_SIZE;

	err = s7xx_fs->ftruncate("Samples/8191-",
							 S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + 1);
	if(err)
	{
		print_unexpected_err(err, 79);
		return 79;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 80" << std::endl;
		return 80;
	}

	err = s7xx_fs->list("/Samples/", dentries);
	if(err)
	{
		print_unexpected_err(err, 81);
		return 81;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 82" << std::endl;
		return 82;
	}

	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::SAMPLE_LIST + 8191
		* S7XX::FS::On_disk_sizes::LIST_ENTRY + 0x1C);
	s7xx_fs->stream.read((char*)&cls, 2);

	if constexpr(S7XX::FS::ENDIANNESS != std::endian::native)
		cls = std::byteswap(cls);

	err = FAT_utils::follow_chain(s7xx_fs->FAT.get(), S7XX::FS::FAT_ATTRS,
								  s7xx_fs->fat_attrs.LENGTH, cls, chain);
	if(err)
	{
		print_unexpected_err(err, 83);
		return 83;
	}

	if(chain.size() != 1)
	{
		std::cerr << "Chain size mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << chain.size() << std::endl;
		std::cerr << "Exit: 84" << std::endl;
		return 84;
	}

	expected_dentry.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
		+ S7XX::AUDIO_SEGMENT_SIZE * 6;

	err = s7xx_fs->ftruncate("Samples/8191-", expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 85);
		return 85;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 86" << std::endl;
		return 86;
	}

	err = s7xx_fs->list("/Samples/", dentries);
	if(err)
	{
		print_unexpected_err(err, 87);
		return 87;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 88" << std::endl;
		return 88;
	}

	chain.resize(1);
	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::SAMPLE_LIST + 8191
		* S7XX::FS::On_disk_sizes::LIST_ENTRY + 0x1C);
	s7xx_fs->stream.read((char*)&chain[0], 2);

	if constexpr(S7XX::FS::ENDIANNESS != std::endian::native)
		chain[0] = std::byteswap(chain[0]);

	if(chain[0] != cls)
	{
		std::cerr << "Chain mismatch after truncate!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << chain[0] << std::endl;
		std::cerr << "Exit: 89" << std::endl;
		return 89;
	}

	chain.clear();
	err = FAT_utils::follow_chain(s7xx_fs->FAT.get(), S7XX::FS::FAT_ATTRS,
								  s7xx_fs->fat_attrs.LENGTH, cls, chain);
	if(err)
	{
		print_unexpected_err(err, 90);
		return 90;
	}

	if(chain.size() != 6)
	{
		std::cerr << "Chain size mismatch!!!" << std::endl;
		std::cerr << "Expected: 6" << std::endl;
		std::cerr << "Got: " << chain.size() << std::endl;
		std::cerr << "Exit: 91" << std::endl;
		return 91;
	}

	expected_dentry.fname = "5-NAME_TRUNC_TEST ";
	expected_dentry.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY;

	count = s7xx_fs->header.TOC.sample_cnt;
	err = s7xx_fs->ftruncate("/Samples/NAME_TRUNC_TEST", 0);
	if(err)
	{
		print_unexpected_err(err, 73);
		return 73;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count + 1)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 74" << std::endl;
		return 74;
	}

	dentries.clear();
	err = s7xx_fs->list("Samples/NAME_TRUNC_TEST", dentries);
	if(err)
	{
		print_unexpected_err(err, 137);
		return 137;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 138" << std::endl;
		return 138;
	}

	expected_dentry.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + 19
		* S7XX::AUDIO_SEGMENT_SIZE;
	cls = s7xx_fs->FAT[1] - 19;

	count = s7xx_fs->header.TOC.sample_cnt;
	err = s7xx_fs->ftruncate("/Samples/NAME_TRUNC_TEST",
							 S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + 19
							 * S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 139);
		return 139;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 140" << std::endl;
		return 140;
	}

	if(s7xx_fs->FAT[1] != cls)
	{
		std::cerr << "Free cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 305" << std::endl;
		return 305;
	}

	dentries.clear();
	err = s7xx_fs->list("Samples/NAME_TRUNC_TEST", dentries);
	if(err)
	{
		print_unexpected_err(err, 141);
		return 141;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 142" << std::endl;
		return 142;
	}

	expected_dentry.fsize -= S7XX::AUDIO_SEGMENT_SIZE;
	cls = s7xx_fs->FAT[1] + 1;

	count = s7xx_fs->header.TOC.sample_cnt;
	err = s7xx_fs->ftruncate("/Samples/NAME_TRUNC_TEST", dentries[0].fsize - S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 300);
		return 300;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 301" << std::endl;
		return 301;
	}

	if(s7xx_fs->FAT[1] != cls)
	{
		std::cerr << "Free cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 302" << std::endl;
		return 302;
	}

	dentries.clear();
	err = s7xx_fs->list("Samples/NAME_TRUNC_TEST", dentries);
	if(err)
	{
		print_unexpected_err(err, 303);
		return 303;
	}

	if(dentries.back() != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 304" << std::endl;
		return 304;
	}
	/*-------------------------End of truncate sample-------------------------*/

	/*-------------------------------Truncate OS------------------------------*/
	media_type = s7xx_fs->header.media_type;
	cls = s7xx_fs->FAT[1];
	
	err = s7xx_fs->ftruncate("/OS", 0);
	if(err)
	{
		print_unexpected_err(err, 114);
		return 114;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Media type pre-truncate: " << media_type_to_str(media_type) << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 115" << std::endl;
		return 115;
	}

	if(media_type != S7XX::FS::Media_type_t::HDD_with_OS_S760 && s7xx_fs->FAT[1] != cls)
	{
		std::cerr << "Bad free cluster value!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 116" << std::endl;
		return 116;
	}
	else if(media_type == S7XX::FS::Media_type_t::HDD_with_OS_S760 && s7xx_fs->FAT[1] != cls + S7XX::FS::S760_OS_CLUSTERS)
	{
		std::cerr << "Bad free cluster value!!!" << std::endl;
		std::cerr << "Expected: " << cls + S7XX::FS::S760_OS_CLUSTERS << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 117" << std::endl;
		return 117;
	}

	err = s7xx_fs->ftruncate("/OS", 0x10 + 1);
	if(err)
	{
		print_unexpected_err(err, 118);
		return 118;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Media type pre-truncate: " << media_type_to_str(media_type) << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 119" << std::endl;
		return 119;
	}

	cls = s7xx_fs->FAT[1];

	err = s7xx_fs->ftruncate("/OS", 0x10 + S7XX::FS::On_disk_sizes::OS + 1);
	if(err)
	{
		print_unexpected_err(err, 120);
		return 120;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS_S760)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Media type pre-truncate: " << media_type_to_str(media_type) << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS_S760) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 121" << std::endl;
		return 121;
	}

	if(s7xx_fs->FAT[1] != cls - S7XX::FS::S760_OS_CLUSTERS)
	{
		std::cerr << "Bad free cluster value!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 122" << std::endl;
		return 122;
	}
	/*---------------------------End of truncate OS---------------------------*/

	/*-----------------------------Truncate other-----------------------------*/
	expected_dentry =
	{
		.fname = "5-                ",
		.fsize = S7XX::FS::On_disk_sizes::VOLUME_PARAMS_ENTRY,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	count = s7xx_fs->header.TOC.volume_cnt;

	err = s7xx_fs->ftruncate("/Volumes/5-", 0);
	if(err)
	{
		print_unexpected_err(err, 123);
		return 123;
	}

	if(s7xx_fs->header.TOC.volume_cnt != count + 1)
	{
		std::cerr << "Mismatched TOC count!!!" << std::endl;
		std::cerr << "Expected: " << count + 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.volume_cnt << std::endl;
		std::cerr << "Exit: 124" << std::endl;
		return 124;
	}

	dentries.clear();
	err = s7xx_fs->list("/Volumes/5-", dentries);
	if(err)
	{
		print_unexpected_err(err, 125);
		return 125;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 126" << std::endl;
		return 126;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 127" << std::endl;
		return 127;
	}

	expected_dentry.fname = "1-NAME_TRUNC_TEST ";
	count = s7xx_fs->header.TOC.volume_cnt;

	err = s7xx_fs->ftruncate("/Volumes/NAME_TRUNC_TEST", 0);
	if(err)
	{
		print_unexpected_err(err, 143);
		return 143;
	}

	if(s7xx_fs->header.TOC.volume_cnt != count + 1)
	{
		std::cerr << "Mismatched TOC count!!!" << std::endl;
		std::cerr << "Expected: " << count + 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.volume_cnt << std::endl;
		std::cerr << "Exit: 144" << std::endl;
		return 144;
	}

	dentries.clear();
	err = s7xx_fs->list("Volumes/NAME_TRUNC_TEST", dentries);
	if(err)
	{
		print_unexpected_err(err, 145);
		return 145;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 146" << std::endl;
		return 146;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 147" << std::endl;
		return 147;
	}
	/*--------------------------End of truncate other-------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 92);
		return 92;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 93" << std::endl;
		return 93;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int rename_tests()
{
	constexpr char S7XX_FS[] = "rename_fs.img";
	min_vfs::dentry_t expected_dentry;

	u16 err, expected_err, fsck_status;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::vector<min_vfs::dentry_t> dentries;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 94" << std::endl;
		return 94;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	err = s7xx_fs->rename("nx_dir/nx_dir/nx_file", "nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 95);
		return 95;
	}

	err = s7xx_fs->rename("nx_dir/nx_dir/nx_file", "nx_dir/nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 96);
		return 96;
	}

	err = s7xx_fs->rename("nx_dir/nx_file", "nx_dir2/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 97);
		return 97;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = s7xx_fs->rename("nx_dir/nx_file", "nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 98);
		return 98;
	}

	err = s7xx_fs->rename("/Samples/8192-", "/Samples/8192-");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 99);
		return 99;
	}
	/*------------------------End of common err testing-----------------------*/

	expected_dentry = NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0];
	expected_dentry.fname = "0-RENAME_TEST     ";

	err = s7xx_fs->rename("/Samples/0-", "/Samples/0-RENAME_TEST");
	if(err)
	{
		print_unexpected_err(err, 100);
		return 100;
	}

	err = s7xx_fs->list("/Samples/0-", dentries);
	if(err)
	{
		print_unexpected_err(err, 101);
		return 101;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;

		std::cerr << "Exit: 102" << std::endl;
		return 102;
	}
	
	err = s7xx_fs->rename("/Samples/TST:Test_01   \x7FR", "/Samples/RENAME_TEST_2");
	if(err)
	{
		print_unexpected_err(err, 131);
		return 131;
	}

	expected_dentry = NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[1];
	expected_dentry.fname = "1-RENAME_TEST_2   ";

	dentries.clear();
	err = s7xx_fs->list("/Samples/1-", dentries);
	if(err)
	{
		print_unexpected_err(err, 135);
		return 135;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;

		std::cerr << "Exit: 136" << std::endl;
		return 136;
	}

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 103);
		return 103;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 104" << std::endl;
		return 104;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

/*We're testing this driver's fopen and fclose, and also min_vfs' stream_t;
since we need at least one driver to test with.*/
static int fopen_tests()
{
	constexpr char S7XX_FS[] = "fopen_fs.img";

	u16 err, expected_err, count, fsck_status;
	size_t expected_fcount;
	uintmax_t expected_refcount;

	std::string fpath;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream_a, stream_b;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 128" << std::endl;
		return 128;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = s7xx_fs->fopen("", stream_b);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 129);
		return 129;
	}

	err = s7xx_fs->fopen("/Samples/nx_dir/nx_file", stream_b);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 130);
		return 130;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = s7xx_fs->fopen("/nx_dir/0-", stream_b);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 148);
		return 148;
	}
	/*------------------------End of common err testing-----------------------*/

	/*------------------------------Open OS tests-----------------------------*/
	err = s7xx_fs->ftruncate("/OS", 0);
	if(err)
	{
		print_unexpected_err(err, 149);
		return 149;
	}
	
	err = s7xx_fs->fopen("/OS", stream_a);
	if(err)
	{
		print_unexpected_err(err, 150);
		return 150;
	}

	if(s7xx_fs->open_files.size() != 1)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 151" << std::endl;
		return 151;
	}

	err = s7xx_fs->fopen("/OS", stream_b);
	if(err)
	{
		print_unexpected_err(err, 152);
		return 152;
	}

	if(s7xx_fs->open_files.size() != 1)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 153" << std::endl;
		return 153;
	}

	if(s7xx_fs->open_files.at("/OS").first != 2)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 154" << std::endl;
		return 154;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 155);
		return 155;
	}

	if(s7xx_fs->open_files.size() != 1)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 156" << std::endl;
		return 156;
	}

	if(s7xx_fs->open_files.at("/OS").first != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 157" << std::endl;
		return 157;
	}

	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 158);
		return 158;
	}

	if(s7xx_fs->open_files.size() != 0)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 159" << std::endl;
		return 159;
	}

	err = s7xx_fs->ftruncate("/OS", 0x10 + 1);
	if(err)
	{
		print_unexpected_err(err, 160);
		return 160;
	}

	err = s7xx_fs->fopen("/OS", stream_a);
	if(err)
	{
		print_unexpected_err(err, 161);
		return 161;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 162);
		return 162;
	}

	err = s7xx_fs->ftruncate("/OS", 0x10 + S7XX::FS::On_disk_sizes::OS + 1);
	if(err)
	{
		print_unexpected_err(err, 163);
		return 163;
	}

	err = s7xx_fs->fopen("/OS", stream_a);
	if(err)
	{
		print_unexpected_err(err, 164);
		return 164;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 165);
		return 165;
	}
	/*--------------------------End of open OS tests--------------------------*/

	/*--------------------------Move assignment tests-------------------------*/
	err = s7xx_fs->fopen("/OS", stream_a);
	if(err)
	{
		print_unexpected_err(err, 166);
		return 166;
	}

	stream_b = std::move(stream_a);
	expected_fcount = 1;
	expected_refcount = 1;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 167" << std::endl;
		return 167;
	}

	if(s7xx_fs->open_files.at("/OS").first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 168" << std::endl;
		return 168;
	}

	err = s7xx_fs->fopen("/OS", stream_a);
	if(err)
	{
		print_unexpected_err(err, 169);
		return 169;
	}

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 170" << std::endl;
		return 170;
	}

	expected_refcount = 2;

	if(s7xx_fs->open_files.at("/OS").first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 171" << std::endl;
		return 171;
	}

	stream_a = std::move(stream_b);

	if(s7xx_fs->open_files.size() != 1)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 172" << std::endl;
		return 172;
	}

	expected_refcount = 1;

	if(s7xx_fs->open_files.at("/OS").first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 173" << std::endl;
		return 173;
	}

	stream_a = std::move(stream_a);

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 174" << std::endl;
		return 174;
	}

	if(s7xx_fs->open_files.at("/OS").first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 175" << std::endl;
		return 175;
	}

	stream_a = std::move(stream_b);

	expected_fcount = 0;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 176" << std::endl;
		return 176;
	}
	/*----------------------End of move assignment tests----------------------*/


	/*----------------------------Open other tests----------------------------*/
	fpath = "/Samples/0";

	err = s7xx_fs->fopen((fpath + "-").c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 177);
		return 177;
	}

	expected_fcount = 1;
	expected_refcount = 1;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 178" << std::endl;
		return 178;
	}

	if(s7xx_fs->open_files.at(fpath.c_str()).first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 179" << std::endl;
		return 179;
	}

	err = s7xx_fs->fopen("/Samples/TST:Test_01   L", stream_b);
	if(err)
	{
		print_unexpected_err(err, 180);
		return 180;
	}

	expected_refcount = 2;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 181" << std::endl;
		return 181;
	}

	if(s7xx_fs->open_files.at(fpath.c_str()).first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 182" << std::endl;
		return 182;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 183);
		return 183;
	}

	expected_refcount = 1;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 184" << std::endl;
		return 184;
	}

	if(s7xx_fs->open_files.at(fpath.c_str()).first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 185" << std::endl;
		return 185;
	}

	stream_b = std::move(stream_a);
	expected_fcount = 0;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 186" << std::endl;
		return 186;
	}
	/*-------------------------End of open other tests------------------------*/

	/*-----------------------------Create by fopen----------------------------*/
	fpath = "/Samples/5";
	count = s7xx_fs->header.TOC.sample_cnt;

	err = s7xx_fs->fopen((fpath + "-fopen_new_test").c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 187);
		return 187;
	}

	expected_fcount = 1;
	expected_refcount = 1;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 188" << std::endl;
		return 188;
	}

	if(s7xx_fs->open_files.at(fpath.c_str()).first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 189" << std::endl;
		return 189;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count + 1)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 190" << std::endl;
		return 190;
	}

	expected_dentry =
	{
		.fname = "5-fopen_new_test  ",
		.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	dentries.clear();
	err = s7xx_fs->list((fpath + "-").c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 191);
		return 191;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 192" << std::endl;
		return 192;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 193" << std::endl;
		return 193;
	}

	fpath = "/Samples/6";
	count = s7xx_fs->header.TOC.sample_cnt;

	//By name. This also tests fopen on an already open stream.
	err = s7xx_fs->fopen((fpath + "-fopen_new_test2").c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 194);
		return 194;
	}

	expected_fcount = 1;
	expected_refcount = 1;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 195" << std::endl;
		return 195;
	}

	if(s7xx_fs->open_files.at(fpath.c_str()).first != expected_refcount)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_refcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 196" << std::endl;
		return 196;
	}

	if(s7xx_fs->header.TOC.sample_cnt != count + 1)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << count;
		std::cerr << ", got:" << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 197" << std::endl;
		return 197;
	}

	expected_dentry =
	{
		.fname = "6-fopen_new_test2 ",
		.fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	dentries.clear();
	err = s7xx_fs->list((fpath + "-").c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 198);
		return 198;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 199" << std::endl;
		return 199;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;

		std::cerr << "Expected dentry:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;

		std::cerr << "Got:" << std::endl;
		std::cerr << dentries.back().to_string(1) << std::endl;

		std::cerr << "Exit: 200" << std::endl;
		return 200;
	}
	/*-------------------------End of create by fopen-------------------------*/

	/*--------------------------fclose via destructor-------------------------*/
	fpath = "/Samples/0";

	err = s7xx_fs->fopen((fpath + "-").c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 201);
		return 201;
	}

	stream_a.~stream_t();
	expected_fcount = 0;

	if(s7xx_fs->open_files.size() != expected_fcount)
	{
		std::cerr << "Open file map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_fcount << std::endl;
		std::cerr << "Got: " << s7xx_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 202" << std::endl;
		return 202;
	}
	/*----------------------End of fclose via destructor----------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 203);
		return 203;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 204" << std::endl;
		return 204;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int read_tests()
{
	constexpr char S7XX_FS[] = "read_tests.img";

	u16 err, expected_err, cls_cnt, leftover;
	uintmax_t pos;

	std::string fpath;
	std::unique_ptr<u8[]> buffer, buffer2;
	std::ifstream host_fs_file;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	min_vfs::stream_t stream;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 35" << std::endl;
		return 35;
	}
	/*----------------------------End of data setup---------------------------*/

	/*-------------------------------Read sample------------------------------*/
	fpath = "/Samples/0";

	err = s7xx_fs->fopen((fpath + "-").c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 36);
		return 36;
	}

	buffer = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	buffer2 = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	cls_cnt = (NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize - 0x10 - S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY) / S7XX::AUDIO_SEGMENT_SIZE;
	host_fs_file.open("ref_host_fs/Samples/0-TST-Test_01   -L");

	err = stream.read(buffer.get(), 0x10);
	if(err)
	{
		print_unexpected_err(err, 205);
		return 205;
	}

	host_fs_file.read((char*)buffer2.get(), 0x10);

	if(std::memcmp(buffer.get(), buffer2.get(), 0x10))
	{
		std::cerr << "Diff in header!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), 0x10);
		std::cerr << "Exit: 206" << std::endl;
		return 206;
	}

	err = stream.read(buffer.get(), S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY);
	if(err)
	{
		print_unexpected_err(err, 207);
		return 207;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY))
	{
		std::cerr << "Diff in params!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY);
		std::cerr << "Exit: 208" << std::endl;
		return 208;
	}

	for(uintmax_t i = 0; i < cls_cnt; i++)
	{
		err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
		if(err)
		{
			print_unexpected_err(err, 209);
			return 209;
		}

		host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

		if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
		{
			std::cerr << "Diff in audio data!!!" << std::endl;
			print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
			std::cerr << "Exit: 210" << std::endl;
			return 210;
		}
	}

	//mid-cluster read
	err = stream.seek(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE / 2);
	if(err)
	{
		print_unexpected_err(err, 211);
		return 211;
	}

	host_fs_file.seekg(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE / 2);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 212);
		return 212;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in audio data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 213" << std::endl;
		return 213;
	}

	//header + params + first cls (partial)
	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 214);
		return 214;
	}

	host_fs_file.seekg(0);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 215);
		return 215;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 216" << std::endl;
		return 216;
	}

	//mid-header
	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 217);
		return 217;
	}

	host_fs_file.seekg(8);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 218);
		return 218;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 219" << std::endl;
		return 219;
	}

	//mid-params
	err = stream.seek(0x20);
	if(err)
	{
		print_unexpected_err(err, 220);
		return 220;
	}

	host_fs_file.seekg(0x20);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 221);
		return 221;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 222" << std::endl;
		return 222;
	}

	//Whole cluster tests
	buffer = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE * 2);
	buffer2 = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE * 2);
	
	//aligned
	err = stream.seek(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY);
	if(err)
	{
		print_unexpected_err(err, 223);
		return 223;
	}

	host_fs_file.seekg(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);
	if(err)
	{
		print_unexpected_err(err, 224);
		return 224;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);
		std::cerr << "Exit: 225" << std::endl;
		return 225;
	}

	//unaligned
	err = stream.seek(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE / 2);
	if(err)
	{
		print_unexpected_err(err, 226);
		return 226;
	}

	host_fs_file.seekg(0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE / 2);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);
	if(err)
	{
		print_unexpected_err(err, 227);
		return 227;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE * 2);
		std::cerr << "Exit: 228" << std::endl;
		return 228;
	}

	buffer = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	buffer2 = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

	host_fs_file.close();

	//EOF tests
	err = stream.seek(NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize);
	if(err)
	{
		print_unexpected_err(err, 229);
		return 229;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.read(buffer.get(), 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 230);
		return 230;
	}

	pos = stream.get_pos();
	err = stream.seek(-1, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 231);
		return 231;
	}

	if(stream.get_pos() != pos - 1)
	{
		std::cerr << "Unexpected pos!!!" << std::endl;
		std::cerr << "Expected: " << pos - 1 << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 232" << std::endl;
		return 232;
	}

	err = stream.read(buffer.get(), 2);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 233);
		return 233;
	}
	/*---------------------------End of read sample---------------------------*/

	/*-------------------------------Read other-------------------------------*/
	fpath = "/Patches/0";

	err = s7xx_fs->fopen((fpath + "-").c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 234);
		return 234;
	}

	host_fs_file.open("ref_host_fs/Patches/0-TST-Test_01");

	err = stream.read(buffer.get(), 0x10);
	if(err)
	{
		print_unexpected_err(err, 235);
		return 235;
	}

	host_fs_file.read((char*)buffer2.get(), 0x10);

	if(std::memcmp(buffer.get(), buffer2.get(), 0x10))
	{
		std::cerr << "Diff in header!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), 0x10);
		std::cerr << "Exit: 236" << std::endl;
		return 236;
	}

	stream.seek(0);
	err = stream.read(buffer.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
	if(err)
	{
		print_unexpected_err(err, 237);
		std::cerr << "stream pos: " << stream.get_pos() << std::endl;
		return 237;
	}

	host_fs_file.seekg(0);
	host_fs_file.read((char*)buffer2.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
	{
		std::cerr << "Diff in params!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
		std::cerr << "Exit: 238" << std::endl;
		return 238;
	}

	//mid-header (doesn't exist anymore)
	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 239);
		return 239;
	}

	host_fs_file.seekg(8);

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.read(buffer.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY
		+ 8);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 240);
		return 240;
	}

	host_fs_file.read((char*)buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY + 8);

	if(std::memcmp(buffer.get(), buffer2.get(),
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY + 8))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY + 8);
		std::cerr << "Exit: 241" << std::endl;
		return 241;
	}

	//mid-params
	err = stream.seek(0x20);
	if(err)
	{
		print_unexpected_err(err, 242);
		return 242;
	}

	host_fs_file.clear();
	host_fs_file.seekg(0x20);

	err = stream.read(buffer.get(), S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY
		- 0x10);
	if(err != expected_err)
	{
		print_unexpected_err(err, 243);
		return 243;
	}

	host_fs_file.read((char*)buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY - 0x10);

	if(std::memcmp(buffer.get(), buffer2.get(),
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY - 0x10))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY - 0x10);
		std::cerr << "Exit: 244" << std::endl;
		return 244;
	}
	host_fs_file.close();

	//EOF tests
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.read(buffer.get(), 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 245);
		return 245;
	}

	err = stream.seek(-1, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 246);
		return 246;
	}

	err = stream.read(buffer.get(), 2);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 247);
		return 247;
	}
	/*----------------------------End of read other---------------------------*/

	/*---------------------------------Read OS--------------------------------*/
	fpath = "/OS";

	err = s7xx_fs->ftruncate(fpath.c_str(), S7XX::FS::On_disk_sizes::OS + 1);
	if(err)
	{
		print_unexpected_err(err, 248);
		return 248;
	}

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 249);
		return 249;
	}

	host_fs_file.open("ref_host_fs/OS");

	cls_cnt = S7XX::FS::On_disk_sizes::OS / S7XX::AUDIO_SEGMENT_SIZE;
	leftover = S7XX::FS::On_disk_sizes::OS - S7XX::AUDIO_SEGMENT_SIZE * cls_cnt;

	for(u8 i = 0; i < cls_cnt; i++)
	{
		err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
		if(err)
		{
			print_unexpected_err(err, 250);
			return 250;
		}

		host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

		if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
		{
			std::cerr << "Diff in S-7XX OS data!!!" << std::endl;
			print_buffers(buffer.get(), buffer2.get(),
						  S7XX::AUDIO_SEGMENT_SIZE);
			std::cerr << "Exit: 251" << std::endl;
			return 251;
		}
	}

	if(leftover)
	{
		err = stream.read(buffer.get(), leftover);
		if(err)
		{
			print_unexpected_err(err, 252);
			std::cerr << "leftover: " << leftover << std::endl;
			return 252;
		}

		host_fs_file.read((char*)buffer2.get(), leftover);

		if(std::memcmp(buffer.get(), buffer2.get(), leftover))
		{
			std::cerr << "Diff in S-7XX OS data!!!" << std::endl;
			std::cerr << "leftover: " << leftover << std::endl;
			print_buffers(buffer.get(), buffer2.get(), leftover);
			std::cerr << "Exit: 253" << std::endl;
			return 253;
		}
	}

	for(uintmax_t i = 0; i < S7XX::FS::S760_OS_CLUSTERS; i++)
	{
		err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
		if(err)
		{
			print_unexpected_err(err, 254);
			return 254;
		}

		host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

		if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
		{
			std::cerr << "Diff in S-760 OS data!!! (cluster " << i << ")"
				<< std::endl;
			print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
			std::cerr << "Exit: 255" << std::endl;
			return 255;
		}
	}

	//mid-header
	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 256);
		return 256;
	}

	host_fs_file.seekg(8);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 257);
		return 257;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 258" << std::endl;
		return 258;
	}

	//mid-params
	err = stream.seek(0x20);
	if(err)
	{
		print_unexpected_err(err, 259);
		return 259;
	}

	host_fs_file.seekg(0x20);

	err = stream.read(buffer.get(), S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 260);
		return 260;
	}

	host_fs_file.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);
		std::cerr << "Exit: 261" << std::endl;
		return 261;
	}
	host_fs_file.close();

	//EOF tests
	err = stream.seek(0x10 + S7XX::FS::On_disk_sizes::OS
		+ S7XX::AUDIO_SEGMENT_SIZE * S7XX::FS::S760_OS_CLUSTERS);
	if(err)
	{
		print_unexpected_err(err, 262);
		return 262;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.read(buffer.get(), 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 263);
		return 263;
	}

	err = stream.seek(-1, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 264);
		return 264;
	}

	err = stream.read(buffer.get(), 2);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 265);
		return 265;
	}

	err = s7xx_fs->ftruncate(fpath.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 266);
		return 266;
	}

	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 267);
		return 267;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 268);
		return 268;
	}

	if(stream.get_pos() != 8)
	{
		std::cerr << "Unexpected stream pos!!!" << std::endl;
		std::cerr << "Expected: " << 8 << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 269" << std::endl;
		return 269;
	}

	err = s7xx_fs->ftruncate(fpath.c_str(), 0x10 + 1);
	if(err)
	{
		print_unexpected_err(err, 270);
		return 270;
	}

	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 271);
		return 271;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err)
	{
		print_unexpected_err(err, 272);
		return 272;
	}

	if(stream.get_pos() != 0x18)
	{
		std::cerr << "Unexpected stream pos!!!" << std::endl;
		std::cerr << "Expected: " << 0x18 << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 273" << std::endl;
		return 273;
	}

	err = stream.seek(S7XX::FS::On_disk_sizes::OS - 8);
	if(err)
	{
		print_unexpected_err(err, 274);
		return 274;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 275);
		return 275;
	}

	if(stream.get_pos() != S7XX::FS::On_disk_sizes::OS)
	{
		std::cerr << "Unexpected stream pos!!!" << std::endl;
		std::cerr << "Expected: " << S7XX::FS::On_disk_sizes::OS << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 276" << std::endl;
		return 276;
	}
	/*-----------------------------End of read OS-----------------------------*/

	return 0;
}

static int trunc_open_tests()
{
	constexpr char S7XX_FS[] = "trunc_open.img";

	u16 err, expected_err;

	std::string fpath;
	std::unique_ptr<u8[]> buffer;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 279" << std::endl;
		return 279;
	}
	/*----------------------------End of data setup---------------------------*/

	fpath = "/Samples/0-";

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 280);
		return 280;
	}

	buffer = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	
	err = stream.seek(NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize - 8);
	if(err)
	{
		print_unexpected_err(err, 281);
		return 281;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 282);
		return 282;
	}

	if(stream.get_pos() != NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 283" << std::endl;
		return 283;
	}

	err = s7xx_fs->ftruncate(fpath.c_str(), NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + 1);
	if(err)
	{
		print_unexpected_err(err, 284);
		return 284;
	}

	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 285);
		return 285;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 286" << std::endl;
		return 286;
	}

	if(dentries[0].fsize != NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + S7XX::AUDIO_SEGMENT_SIZE)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + S7XX::AUDIO_SEGMENT_SIZE << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 287" << std::endl;
		return 287;
	}

	err = stream.seek(NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + S7XX::AUDIO_SEGMENT_SIZE - 8);
	if(err)
	{
		print_unexpected_err(err, 288);
		return 288;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 289);
		return 289;
	}

	if(stream.get_pos() != NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + S7XX::AUDIO_SEGMENT_SIZE)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize + S7XX::AUDIO_SEGMENT_SIZE << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 290" << std::endl;
		return 290;
	}

	err = s7xx_fs->ftruncate(fpath.c_str(), NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize - S7XX::AUDIO_SEGMENT_SIZE);
	if(err)
	{
		print_unexpected_err(err, 291);
		return 291;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 292);
		return 292;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 293" << std::endl;
		return 293;
	}

	if(dentries[0].fsize != NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize - S7XX::AUDIO_SEGMENT_SIZE)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize - S7XX::AUDIO_SEGMENT_SIZE << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 294" << std::endl;
		return 294;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 295);
		return 295;
	}

	if(stream.get_pos() < dentries[0].fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: >=" << dentries[0].fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 296" << std::endl;
		return 296;
	}

	err = stream.seek(dentries[0].fsize - 8);
	if(err)
	{
		print_unexpected_err(err, 297);
		return 297;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 298);
		return 298;
	}

	if(stream.get_pos() != dentries[0].fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << dentries[0].fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 299" << std::endl;
		return 299;
	}

	return 0;
}

static int write_tests()
{
	constexpr char S7XX_FS[] = "write_tests.img";

	u16 err, expected_err, fsck_status, cls_cnt, leftover;
	size_t buf_size;
	uintmax_t pos, old_fsize;

	std::string fpath;
	std::unique_ptr<u8[]> buffer, buffer2;
	std::ifstream host_fs_file;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 306" << std::endl;
		return 306;
	}
	/*----------------------------End of data setup---------------------------*/

	/*------------------------------Write sample------------------------------*/
	fpath = "/Samples/0-";

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 307);
		return 307;
	}

	buf_size = S7XX::AUDIO_SEGMENT_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 308);
		return 308;
	}

	if(stream.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 309" << std::endl;
		return 309;
	}

	err = stream.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 310);
		return 310;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 311);
		return 311;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 312" << std::endl;
		return 312;
	}

	//Whole cls test
	buf_size = S7XX::AUDIO_SEGMENT_SIZE * 2;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);
	
	pos = 0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 313);
		return 313;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 314);
		return 314;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 315" << std::endl;
		return 315;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 316);
		return 316;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 317);
		return 317;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 318" << std::endl;
		return 318;
	}

	//Unaligned cls test
	buf_size = S7XX::AUDIO_SEGMENT_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xF0, buf_size);

	pos = 0x10 + S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY + S7XX::AUDIO_SEGMENT_SIZE / 2;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 319);
		return 319;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 320);
		return 320;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 321" << std::endl;
		return 321;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 322);
		return 322;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 323);
		return 323;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 324" << std::endl;
		return 324;
	}

	//Extend by write tests (a.k.a. write past EOF)
	pos = NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[0].fsize;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 325);
		return 325;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 326);
		return 326;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 327" << std::endl;
		return 327;
	}

	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 328);
		return 328;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 329" << std::endl;
		return 329;
	}

	if(dentries[0].fsize != pos + S7XX::AUDIO_SEGMENT_SIZE)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + S7XX::AUDIO_SEGMENT_SIZE << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 330" << std::endl;
		return 330;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 331);
		return 331;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 332);
		return 332;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 333" << std::endl;
		return 333;
	}

	//Extend unaligned
	old_fsize = dentries[0].fsize;
	pos = old_fsize - S7XX::AUDIO_SEGMENT_SIZE / 2;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 334);
		return 334;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 335);
		return 335;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 336" << std::endl;
		return 336;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 337);
		return 337;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 338" << std::endl;
		return 338;
	}

	if(dentries[0].fsize != old_fsize + S7XX::AUDIO_SEGMENT_SIZE)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize + S7XX::AUDIO_SEGMENT_SIZE << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 339" << std::endl;
		return 339;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 340);
		return 340;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 341);
		return 341;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 342" << std::endl;
		return 342;
	}
	
	//Extend (whole cls, buf_size = S7XX::AUDIO_SEGMENT_SIZE * 2)
	buf_size = S7XX::AUDIO_SEGMENT_SIZE * 2;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	old_fsize = dentries[0].fsize;
	pos = old_fsize;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 343);
		return 343;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 344);
		return 344;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 345" << std::endl;
		return 345;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 346);
		return 346;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 347" << std::endl;
		return 347;
	}

	if(dentries[0].fsize != old_fsize + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 348" << std::endl;
		return 348;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 349);
		return 349;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 350);
		return 350;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 351" << std::endl;
		return 351;
	}

	//Extend unaligned (whole cls, buf_size = S7XX::AUDIO_SEGMENT_SIZE * 2)
	old_fsize = dentries[0].fsize;
	pos = old_fsize - S7XX::AUDIO_SEGMENT_SIZE / 2;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 352);
		return 352;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 353);
		return 353;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 354" << std::endl;
		return 354;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 355);
		return 355;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 356" << std::endl;
		return 356;
	}

	if(dentries[0].fsize != old_fsize + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 357" << std::endl;
		return 357;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 358);
		return 358;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 359);
		return 359;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 360" << std::endl;
		return 360;
	}
	/*---------------------------End of write sample--------------------------*/

	/*-------------------------------Write other------------------------------*/
	fpath = "/Patches/0-";

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 361);
		return 361;
	}

	buf_size = S7XX::AUDIO_SEGMENT_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.write(buffer.get(), buf_size);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 362);
		return 362;
	}

	if(stream.get_pos() != S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " <<
			S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 363" << std::endl;
		return 363;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 364);
		return 364;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 365);
		return 365;
	}

	if(std::memcmp(buffer.get(), buffer2.get(),
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
		std::cerr << "Exit: 366" << std::endl;
		return 366;
	}

	//Write exact
	buf_size = S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY;
	std::memset(buffer.get(), 0x55, buf_size);

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 367);
		return 367;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 368);
		return 368;
	}

	if(stream.get_pos() != S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " <<
			S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 369" << std::endl;
		return 369;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 370);
		return 370;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 371);
		return 371;
	}

	if(std::memcmp(buffer.get(), buffer2.get(),
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
		std::cerr << "Exit: 372" << std::endl;
		return 372;
	}

	//Write unaligned from header (doesn't exist anymore)
	buf_size = S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY;
	std::memset(buffer.get(), 0xF0, buf_size);

	err = stream.seek(8);
	if(err)
	{
		print_unexpected_err(err, 373);
		return 373;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err != expected_err)
	{
		print_unexpected_err(err, 374);
		return 374;
	}

	if(stream.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 375" << std::endl;
		return 375;
	}

	std::memcpy(buffer.get(), buffer2.get(), 8);

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 376);
		return 376;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 377);
		return 377;
	}

	if(std::memcmp(buffer.get(), buffer2.get(),
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(),
					  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
		std::cerr << "Exit: 378" << std::endl;
		return 378;
	}
	/*---------------------------End of write other---------------------------*/

	/*--------------------------------Write OS--------------------------------*/
	fpath = "/OS";

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 379);
		return 379;
	}

	buf_size = S7XX::AUDIO_SEGMENT_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	//Write from start
	err = s7xx_fs->ftruncate(fpath.c_str(), S7XX::FS::On_disk_sizes::OS
		+ S7XX::AUDIO_SEGMENT_SIZE * S7XX::FS::S760_OS_CLUSTERS);
	if(err)
	{
		print_unexpected_err(err, 380);
		return 380;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 381);
		return 381;
	}

	if(stream.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 382" << std::endl;
		return 382;
	}

	err = stream.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 383);
		return 383;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 384);
		return 384;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 385" << std::endl;
		return 385;
	}

	//Write from header (which doesn't exist anymore) unaligned
	pos = 8;
	std::memset(buffer.get(), 0x55, buf_size);

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 386);
		return 386;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 387);
		return 387;
	}

	std::memcpy(buffer.get(), buffer2.get(), pos);

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 388" << std::endl;
		return 388;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 389);
		return 389;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 390);
		return 390;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 391" << std::endl;
		return 391;
	}

	//Write to S7XX OS data (unaligned)
	pos = 0x18;
	std::memset(buffer.get(), 0xAA, buf_size);

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 392);
		return 392;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 393);
		return 393;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 394" << std::endl;
		return 394;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 395);
		return 395;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 396);
		return 396;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 397" << std::endl;
		return 397;
	}

	//Write to end of S7XX OS data (unaligned)
	pos = 0x10 + S7XX::FS::On_disk_sizes::OS - 8;
	std::memset(buffer.get(), 0x55, buf_size);

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 398);
		return 398;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 399);
		return 399;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 400" << std::endl;
		return 400;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 401);
		return 401;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 402);
		return 402;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 403" << std::endl;
		return 403;
	}

	//Write to end of S760 OS data (unaligned)
	pos = S7XX::FS::On_disk_sizes::OS + S7XX::AUDIO_SEGMENT_SIZE
		* S7XX::FS::S760_OS_CLUSTERS - 8;
	std::memset(buffer.get(), 0xAA, buf_size);

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 404);
		return 404;
	}
	
	err = stream.write(buffer.get(), buf_size);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 405);
		return 405;
	}

	if(stream.get_pos() != pos + 8)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 406" << std::endl;
		return 406;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 407);
		return 407;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 408);
		return 408;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), 8))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), 8);
		std::cerr << "Exit: 409" << std::endl;
		return 409;
	}

	//Write from header extend
	err = s7xx_fs->ftruncate(fpath.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 410);
		return 410;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 411);
		return 411;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 412);
		return 412;
	}

	if(stream.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 413" << std::endl;
		return 413;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 414" << std::endl;
		return 414;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 415);
		return 415;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 416" << std::endl;
		return 416;
	}

	old_fsize = S7XX::FS::On_disk_sizes::OS;

	if(dentries[0].fsize != old_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 417" << std::endl;
		return 417;
	}

	err = stream.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 418);
		return 418;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 419);
		return 419;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 420" << std::endl;
		return 420;
	}

	//Write from header extend unaligned
	pos = 8;

	err = s7xx_fs->ftruncate(fpath.c_str(), pos);
	if(err)
	{
		print_unexpected_err(err, 421);
		return 421;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 422);
		return 422;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 423);
		return 423;
	}

	if(stream.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 424" << std::endl;
		return 424;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 425" << std::endl;
		return 425;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 426);
		return 426;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 427" << std::endl;
		return 427;
	}

	old_fsize = S7XX::FS::On_disk_sizes::OS;

	if(dentries[0].fsize != old_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 428" << std::endl;
		return 428;
	}

	err = stream.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 429);
		return 429;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 430);
		return 430;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 431" << std::endl;
		return 431;
	}

	//Write from S7XX OS extend
	pos = 0x10 + S7XX::FS::On_disk_sizes::OS;

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 432);
		return 432;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 433);
		return 433;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 434" << std::endl;
		return 434;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS_S760)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 435" << std::endl;
		return 435;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 436);
		return 436;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 437" << std::endl;
		return 437;
	}

	old_fsize = S7XX::FS::On_disk_sizes::OS
		+ S7XX::FS::On_disk_sizes::S760_EXT_OS;

	if(dentries[0].fsize != old_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 438" << std::endl;
		return 438;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 439);
		return 439;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 440);
		return 440;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 441" << std::endl;
		return 441;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 442);
		return 442;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 443);
		return 443;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 444" << std::endl;
		return 444;
	}

	//Write from S7XX OS extend unaligned
	pos = 0x10 + S7XX::FS::On_disk_sizes::OS - 8;

	err = s7xx_fs->ftruncate(fpath.c_str(), 0x10 + S7XX::FS::On_disk_sizes::OS);
	if(err)
	{
		print_unexpected_err(err, 445);
		return 445;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 446);
		return 446;
	}

	err = stream.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 447);
		return 447;
	}

	if(stream.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 448" << std::endl;
		return 448;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS_S760)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 449" << std::endl;
		return 449;
	}

	dentries.clear();
	err = s7xx_fs->list(fpath.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 450);
		return 450;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 451" << std::endl;
		return 451;
	}

	old_fsize = S7XX::FS::On_disk_sizes::OS
		+ S7XX::FS::On_disk_sizes::S760_EXT_OS;

	if(dentries[0].fsize != old_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 452" << std::endl;
		return 452;
	}

	err = stream.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 453);
		return 453;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 454);
		return 454;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 455" << std::endl;
		return 455;
	}

	err = stream.seek(0);
	if(err)
	{
		print_unexpected_err(err, 456);
		return 456;
	}

	err = stream.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 457);
		return 457;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 458" << std::endl;
		return 458;
	}
	/*-----------------------------End of write OS----------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 459);
		return 459;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 460" << std::endl;
		return 460;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int remove_open_tests()
{
	constexpr char S7XX_FS[] = "rm_open_tests.img";

	u16 err, expected_err, fsck_status, cnt;

	std::string fpath;
	std::ifstream host_fs_file;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 461" << std::endl;
		return 461;
	}
	/*----------------------------End of data setup---------------------------*/

	/*-------------------------------Remove file------------------------------*/
	//Sample
	fpath = "/Samples/0-";
	cnt = s7xx_fs->header.TOC.sample_cnt;

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 462);
		return 462;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = s7xx_fs->remove(fpath.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 463);
		return 463;
	}

	if(s7xx_fs->header.TOC.sample_cnt != cnt)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 464" << std::endl;
		return 464;
	}

	err = s7xx_fs->list("/Samples", dentries);
	if(err)
	{
		print_unexpected_err(err, 465);
		return 465;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 466" << std::endl;
		return 466;
	}

	//Other
	fpath = "/Patches/0-";
	cnt = s7xx_fs->header.TOC.patch_cnt;

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 467);
		return 467;
	}

	err = s7xx_fs->remove(fpath.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 468);
		return 468;
	}

	if(s7xx_fs->header.TOC.patch_cnt != cnt)
	{
		std::cerr << "Other count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.patch_cnt << std::endl;
		std::cerr << "Exit: 469" << std::endl;
		return 469;
	}

	dentries.clear();
	err = s7xx_fs->list("/Patches", dentries);
	if(err)
	{
		print_unexpected_err(err, 470);
		return 470;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 471" << std::endl;
		return 471;
	}

	//OS
	fpath = "/OS";

	err = s7xx_fs->ftruncate(fpath.c_str(), 0x11);
	if(err)
	{
		print_unexpected_err(err, 472);
		return 472;
	}

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 473);
		return 473;
	}

	err = s7xx_fs->remove(fpath.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 474);
		return 474;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 475" << std::endl;
		return 475;
	}
	/*---------------------------End of remove file---------------------------*/

	/*-------------------------------Remove dir-------------------------------*/
	//Sample
	fpath = "/Samples/0-";
	cnt = 1;

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 476);
		return 476;
	}

	err = s7xx_fs->remove("/Samples");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 477);
		return 477;
	}

	if(s7xx_fs->header.TOC.sample_cnt != cnt)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 478" << std::endl;
		return 478;
	}

	dentries.clear();
	err = s7xx_fs->list("/Samples", dentries);
	if(err)
	{
		print_unexpected_err(err, 479);
		return 479;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 480" << std::endl;
		return 480;
	}

	//Other
	fpath = "/Patches/0-";

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 481);
		return 481;
	}

	err = s7xx_fs->remove("/Patches");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 482);
		return 482;
	}

	if(s7xx_fs->header.TOC.patch_cnt != cnt)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.patch_cnt << std::endl;
		std::cerr << "Exit: 483" << std::endl;
		return 483;
	}

	dentries.clear();
	err = s7xx_fs->list("/Patches", dentries);
	if(err)
	{
		print_unexpected_err(err, 484);
		return 484;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 485" << std::endl;
		return 485;
	}
	/*----------------------------End of remove dir---------------------------*/

	/*-------------------------------Remove root------------------------------*/
	//Sample
	fpath = "/Samples/0-";

	/*TODO: Consider adding more samples to check all non-open samples do get
	deleted (via ftruncate).*/
	cnt = 1;

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 486);
		return 486;
	}

	err = s7xx_fs->remove("/");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 487);
		return 487;
	}

	if(s7xx_fs->header.TOC.sample_cnt != cnt)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.sample_cnt << std::endl;
		std::cerr << "Exit: 488" << std::endl;
		return 488;
	}

	err = s7xx_fs->header.TOC.partial_cnt;
	if(err)
	{
		std::cerr << "Partial count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 489" << std::endl;
		return 489;
	}

	err = s7xx_fs->header.TOC.patch_cnt;
	if(err)
	{
		std::cerr << "Patch count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 490" << std::endl;
		return 490;
	}

	err = s7xx_fs->header.TOC.perf_cnt;
	if(err)
	{
		std::cerr << "Performance count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 491" << std::endl;
		return 491;
	}

	err = s7xx_fs->header.TOC.volume_cnt;
	if(err)
	{
		std::cerr << "Volume count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 492" << std::endl;
		return 492;
	}

	dentries.clear();
	err = s7xx_fs->list("/Samples", dentries);
	if(err)
	{
		print_unexpected_err(err, 493);
		return 493;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 494" << std::endl;
		return 494;
	}

	//Other
	fpath = "/Patches/0-";

	/*Keep in mind the previous test already deleted everything but a sample by
	deleting root. The only reason we are going to have a patch is that fopen
	creates.*/
	cnt = 1;

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 495);
		return 495;
	}

	err = s7xx_fs->remove("");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 496);
		return 496;
	}

	if(s7xx_fs->header.TOC.patch_cnt != cnt)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << s7xx_fs->header.TOC.patch_cnt << std::endl;
		std::cerr << "Exit: 497" << std::endl;
		return 497;
	}

	err = s7xx_fs->header.TOC.sample_cnt;
	if(err)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 498" << std::endl;
		return 498;
	}

	err = s7xx_fs->header.TOC.partial_cnt;
	if(err)
	{
		std::cerr << "Partial count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 499" << std::endl;
		return 499;
	}

	err = s7xx_fs->header.TOC.perf_cnt;
	if(err)
	{
		std::cerr << "Performance count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 500" << std::endl;
		return 500;
	}

	err = s7xx_fs->header.TOC.volume_cnt;
	if(err)
	{
		std::cerr << "Volume count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 501" << std::endl;
		return 501;
	}

	dentries.clear();
	err = s7xx_fs->list("/Patches", dentries);
	if(err)
	{
		print_unexpected_err(err, 502);
		return 502;
	}

	if(dentries.size() != cnt)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 503" << std::endl;
		return 503;
	}

	//OS
	fpath = "/OS";

	err = s7xx_fs->ftruncate(fpath.c_str(), 0x11);
	if(err)
	{
		print_unexpected_err(err, 504);
		return 504;
	}

	err = s7xx_fs->fopen(fpath.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 505);
		return 505;
	}

	err = s7xx_fs->remove("/");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 506);
		return 506;
	}

	if(s7xx_fs->header.media_type != S7XX::FS::Media_type_t::HDD_with_OS)
	{
		std::cerr << "Media type mismatch!!!" << std::endl;
		std::cerr << "Expected: " << media_type_to_str(S7XX::FS::Media_type_t::HDD_with_OS) << std::endl;
		std::cerr << "Got: " << media_type_to_str(s7xx_fs->header.media_type) << std::endl;
		std::cerr << "Exit: 507" << std::endl;
		return 507;
	}

	err = s7xx_fs->header.TOC.sample_cnt;
	if(err)
	{
		std::cerr << "Sample count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 508" << std::endl;
		return 508;
	}

	err = s7xx_fs->header.TOC.partial_cnt;
	if(err)
	{
		std::cerr << "Partial count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 509" << std::endl;
		return 509;
	}

	err = s7xx_fs->header.TOC.patch_cnt;
	if(err)
	{
		std::cerr << "Patch count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 510" << std::endl;
		return 510;
	}

	err = s7xx_fs->header.TOC.perf_cnt;
	if(err)
	{
		std::cerr << "Performance count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 511" << std::endl;
		return 511;
	}

	err = s7xx_fs->header.TOC.volume_cnt;
	if(err)
	{
		std::cerr << "Volume count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << err << std::endl;
		std::cerr << "Exit: 512" << std::endl;
		return 512;
	}
	/*---------------------------End of remove root---------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	s7xx_fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(s7xx_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 513);
		return 513;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 514" << std::endl;
		return 514;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

//Host FS utils are a dependency of the driver, so its tests should be run
//first.
int main()
{
	u16 err;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs, dst_fs;

	//test standalone functions first
	std::cout << "fsck tests..." << std::endl;
	err = fsck_tests();
	if(err) return err;
	std::cout << "fsck tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "mkfs tests..." << std::endl;
	err = mkfs_tests(dst_fs);
	if(err) return err;
	std::cout << "mkfs tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Mount tests..." << std::endl;
	err = mount_tests(s7xx_fs);
	if(err) return err;
	std::cout << "Mount OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "List tests..." << std::endl;
	err = list_tests(*s7xx_fs);
	if(err) return err;
	std::cout << "List tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Delete tests..." << std::endl;
	err = del_tests();
	if(err) return err;
	std::cout << "Delete tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Truncate tests..." << std::endl;
	err = trunc_tests();
	if(err) return err;
	std::cout << "Truncate tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Rename tests..." << std::endl;
	err = rename_tests();
	if(err) return err;
	std::cout << "Rename tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "fopen/fclose tests..." << std::endl;
	err = fopen_tests();
	if(err) return err;
	std::cout << "fopen/fclose tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Read tests..." << std::endl;
	err = read_tests();
	if(err) return err;
	std::cout << "Read tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Truncate while open tests..." << std::endl;
	err = trunc_open_tests();
	if(err) return err;
	std::cout << "Truncate while open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write tests..." << std::endl;
	err = write_tests();
	if(err) return err;
	std::cout << "Write tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Remove while open tests..." << std::endl;
	err = remove_open_tests();
	if(err) return err;
	std::cout << "Remove while open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;

	return err;
}
