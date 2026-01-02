#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <utility>
#include <memory>
#include <bit>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "Utils/testing_helpers.hpp"
#include "E-MU/EMU_FS_types.hpp"
#include "E-MU/EMU_FS_drv.hpp"
#include "fs_test_data.hpp"
#include "Helpers.hpp"
#include "min_vfs/min_vfs_base.hpp"

using namespace EMU::FS::testing;

static int list_tests(EMU::FS::filesystem_t &emu_fs)
{
	u16 err, expected_err;
	min_vfs::dentry_t expected_dentry;

	std::string path;
	std::vector<min_vfs::dentry_t> dirs, files;

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs.list("/nx_dir", files);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 4);
		return 4;
	}

	err = emu_fs.list((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str(), files);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 5);
		return 5;
	}
	/*------------------------End of common err testing-----------------------*/

	err = emu_fs.list("/", dirs);
	if(err)
	{
		print_unexpected_err(err, 6);
		return 6;
	}

	err = check_file_list(EXPECTED_ROOT_DIR, dirs, 0);
	if(err) return 6 + err;

	//List root dir's dentry
	dirs.clear();
	err = emu_fs.list("/", dirs, true);
	if(err)
	{
		print_unexpected_err(err, 698);
		return 698;
	}

	if(dirs.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dirs.size() << std::endl;
		std::cerr << "Exit: " << 699 << std::endl;
		return 699;
	}

	expected_dentry =
	{
		.fname = "/",
		.fsize = EXPECTED_HEADER.dir_list_blk_cnt * EMU::FS::BLK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	};

	if(dirs[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dirs[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 700 << std::endl;
		return 700;
	}

	err = emu_fs.list(EXPECTED_ROOT_DIR[0].fname.c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 9);
		return 9;
	}

	err = check_file_list(EXPECTED_DIR0, files, 0);
	if(err) return 9 + err;

	//List dir's dentry
	dirs.clear();
	err = emu_fs.list(EXPECTED_ROOT_DIR[0].fname.c_str(), dirs, true);
	if(err)
	{
		print_unexpected_err(err, 701);
		return 701;
	}

	if(dirs.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dirs.size() << std::endl;
		std::cerr << "Exit: " << 702 << std::endl;
		return 702;
	}

	expected_dentry = EXPECTED_ROOT_DIR[0];
	if(dirs[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dirs[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 703 << std::endl;
		return 703;
	}

	files.clear();
	err = emu_fs.list(EXPECTED_ROOT_DIR[1].fname.c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 12);
		return 12;
	}

	err = check_file_list(EXPECTED_DIR1, files, 0);
	if(err) return 12 + err;

	path = "/" + EXPECTED_ROOT_DIR[1].fname;

	files.clear();
	err = emu_fs.list((path + "/" + EXPECTED_DIR1[0].fname).c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 15);
		return 15;
	}

	//bank num only
	err = check_file_list({EXPECTED_DIR1[0]}, files, 0);
	if(err) return 15 + err;

	files.clear();
	err = emu_fs.list((path + "/16-").c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 18);
		return 18;
	}

	err = check_file_list({EXPECTED_DIR1[1]}, files, 0);
	if(err) return 18 + err;

	//filename only
	files.clear();
	err = emu_fs.list((path + "/lae dee em cee t").c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 21);
		return 21;
	}

	err = check_file_list({EXPECTED_DIR1[1]}, files, 0);
	if(err) return 21 + err;

	//bank num + invalid filename
	files.clear();
	err = emu_fs.list((path + "/16-irrelevant").c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 24);
		return 24;
	}

	err = check_file_list({EXPECTED_DIR1[1]}, files, 0);
	if(err) return 24 + err;

	//invalid bank num + filename
	files.clear();
	err = emu_fs.list((path + "/256-lae dee em cee t").c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 66);
		return 66;
	}

	if(files.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << files.size() << std::endl;
		std::cerr << "Exit: 67" << std::endl;
		return 67;
	}

	if(files[0] != EXPECTED_DIR1[1])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1[1].to_string(1) << std::endl;
		std::cerr << "Got: " << files[0].to_string(1) << std::endl;
		std::cerr << "Exit: 68" << std::endl;
		return 68;
	}

	//List dir with '/' in name
	files.clear();
	err = emu_fs.list(EXPECTED_ROOT_DIR[2].fname.c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 600);
		return 600;
	}

	err = check_file_list(EXPECTED_DIR2, files, 0);
	if(err) return 600 + err;

	//List file with '/' in name
	files.clear();
	err = emu_fs.list((EXPECTED_ROOT_DIR[0].fname + "/" + EXPECTED_DIR0[7].fname).c_str(), files);
	if(err)
	{
		print_unexpected_err(err, 603);
		return 603;
	}

	if(!files.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expexted: " << 1 << std::endl;
		std::cerr << "Got: " << files.size() << std::endl;
		std::cerr << "Exit: " << 604 << std::endl;
		return 604;
	}

	if(files[0] != EXPECTED_DIR0[7])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << EXPECTED_DIR0[7].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << files[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 605 << std::endl;
		return 605;
	}

	return 0;
}

static int mkdir_tests()
{
	constexpr char EMU_FS[] = "mkdir_fs.img";

	u16 err, expected_err, fsck_status;

	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 27" << std::endl;
		return 27;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->mkdir("/");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 28);
		return 28;
	}

	err = emu_fs->mkdir("/nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 29);
		return 29;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_EXISTS);
	err = emu_fs->mkdir(EXPECTED_ROOT_DIR[0].fname.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 30);
		return 30;
	}

	err = emu_fs->mkdir(EXPECTED_ROOT_DIR[1].fname.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 31);
		return 31;
	}

	err = emu_fs->mkdir(EXPECTED_ROOT_DIR[2].fname.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 606);
		return 606;
	}

	//NO_SPACE_LEFT has to be tested at the end, after checking mkdir works.
	/*------------------------End of common err testing-----------------------*/

	err = emu_fs->mkdir("/mkdir_test_1");
	if(err)
	{
		print_unexpected_err(err, 32);
		return 32;
	}

	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 33);
		return 33;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size() + 1)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 34" << std::endl;
		return 34;
	}

	expected_dentry =
	{
		.fname = "mkdir_test_1",
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	};

	if(dentries[1] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << dentries[1].to_string(1) << std::endl;
		std::cerr << "Exit: 35" << std::endl;
		return 35;
	}

	err = emu_fs->mkdir("/mkdir_test_2");
	if(err)
	{
		print_unexpected_err(err, 36);
		return 36;
	}

	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 37);
		return 37;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size() + 2)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() + 2 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 38" << std::endl;
		return 38;
	}

	expected_dentry.fname = "mkdir_test_2";

	if(dentries[4] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << dentries[4].to_string(1) << std::endl;
		std::cerr << "Exit: 39" << std::endl;
		return 39;
	}

	//Make dir with '/' in name
	err = emu_fs->mkdir("/mkdir\\test_3");
	if(err)
	{
		print_unexpected_err(err, 607);
		return 607;
	}

	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 608);
		return 608;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size() + 3)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() + 3 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 609" << std::endl;
		return 609;
	}

	expected_dentry.fname = "mkdir\\test_3";

	if(dentries[5] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[5].to_string(1) << std::endl;
		std::cerr << "Exit: 610" << std::endl;
		return 610;
	}

	constexpr u32 MAX_DIRS = EXPECTED_HEADER.dir_list_blk_cnt * EMU::FS::DIRS_PER_BLOCK;
	const u32 LEFTOVER_DIRS = MAX_DIRS - EXPECTED_ROOT_DIR.size() - 3;

	for(u32 i = 0; i < LEFTOVER_DIRS; i++)
	{
		err = emu_fs->mkdir((std::string("/mkdir_auto_") + std::to_string(i)).c_str());
		if(err)
		{
			print_unexpected_err(err, 40);
			return 40;
		}
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);
	err = emu_fs->mkdir("/mkdir_no_spc");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 41);
		return 41;
	}

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 549);
		return 549;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 550" << std::endl;
		return 550;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int trunc_tests()
{
	constexpr char EMU_FS[] = "trunc_fs.img";

	u16 err, expected_err, fsck_status, expected_dir_size, free_clusters,
		next_dir_content_block;
	u64 filled_dirs;
	uintmax_t old_free_space, new_free_space;

	std::string d_path, f_path;

	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries, og_dir_cont, expected_dir_cont;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 42" << std::endl;
		return 42;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	//Test file too large
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::FILE_TOO_LARGE);
	err = emu_fs->ftruncate("/nx_dir/irrelevant", emu_fs->header.cluster_cnt * CLUSTER_SIZE + 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 124);
		return 124;
	}

	//Test invalid path
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->ftruncate("", 0);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 125);
		return 125;
	}

	err = emu_fs->ftruncate("/nx_dir", 0);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 126);
		return 126;
	}

	err = emu_fs->ftruncate("/nx_dir/nx_file/invalid", 0);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 127);
		return 127;
	}

	/*NO_SPACE_LEFT has to be tested at the end, after checking creating files
	and expanding dirs actually work.*/
	/*------------------------End of common err testing-----------------------*/
	
	/*-------------------------------Create file------------------------------*/
	//create by bank_num
	d_path = "/" + EXPECTED_ROOT_DIR[0].fname;
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 43);
		return 43;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 44" << std::endl;
		return 44;
	}

	expected_dir_size = dentries[0].fsize;

	err = emu_fs->list(d_path.c_str(), og_dir_cont);
	if(err)
	{
		print_unexpected_err(err, 45);
		return 45;
	}

	expected_dentry =
	{
		.fname = "30-Trunc_test_1",
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	f_path = d_path + "/30-Trunc_test_1";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 46);
		return 46;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 47);
		return 47;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 48" << std::endl;
		return 48;
	}

	if(dentries[0].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 49" << std::endl;
		return 49;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 50);
		return 50;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 51" << std::endl;
		return 51;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 52" << std::endl;
		return 52;
	}

	expected_dir_cont = og_dir_cont;
	expected_dir_cont.insert(expected_dir_cont.cbegin() + 9, expected_dentry);

	dentries.clear();
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 53);
		return 53;
	}

	err = check_file_list(expected_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 53 + err << std::endl;
		return 53 + err;
	}

	//create by name
	expected_dentry.fname = "0-Trunc_test_2";

	f_path = d_path + "/Trunc_test_2";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 56);
		return 56;
	}

	dentries.clear();
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 57);
		return 57;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 58" << std::endl;
		return 58;
	}

	if(dentries[0].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 59" << std::endl;
		return 59;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 60);
		return 60;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 61" << std::endl;
		return 61;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 62" << std::endl;
		return 62;
	}

	expected_dir_cont.insert(expected_dir_cont.cbegin() + 10, expected_dentry);

	dentries.clear();
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 63);
		return 63;
	}

	err = check_file_list(expected_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 63 + err << std::endl;
		return 63 + err;
	}

	//Create by bank num with '/' in dirname and filename
	d_path = "/" + EXPECTED_ROOT_DIR[2].fname + "/";
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 611);
		return 611;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 612" << std::endl;
		return 612;
	}

	expected_dir_size = dentries[2].fsize;

	og_dir_cont.clear();
	err = emu_fs->list(d_path.c_str(), og_dir_cont);
	if(err)
	{
		print_unexpected_err(err, 613);
		return 613;
	}

	expected_dentry =
	{
		.fname = "3-Trunc_test\\6",
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	f_path = d_path + "3-Trunc_test\\6";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 614);
		return 614;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 615);
		return 615;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 616 << std::endl;
		return 616;
	}

	if(dentries[2].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[2].fsize << std::endl;
		std::cerr << "Exit: " << 617 << std::endl;
		return 617;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 618);
		return 618;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 619 << std::endl;
		return 619;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 620 << std::endl;
		return 620;
	}

	expected_dir_cont = og_dir_cont;
	expected_dir_cont.push_back(expected_dentry);

	dentries.clear();
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 621);
		return 621;
	}

	err = check_file_list(expected_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 621 + err << std::endl;
		return 621 + err;
	}

	//Create by name with '/' in dir name and filename
	expected_dentry.fname = "2-Trunc\\test\\8";

	f_path = d_path + "Trunc\\test\\8";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 624);
		return 624;
	}

	dentries.clear();
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 625);
		return 625;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 626 << std::endl;
		return 626;
	}

	if(dentries[2].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[2].fsize << std::endl;
		std::cerr << "Exit: " << 627 << std::endl;
		return 627;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 628);
		return 628;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 629 << std::endl;
		return 629;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 630 << std::endl;
		return 630;
	}

	expected_dir_cont.insert(expected_dir_cont.cbegin() + 3, expected_dentry);

	dentries.clear();
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 631);
		return 631;
	}

	err = check_file_list(expected_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 631 + err << std::endl;
		return 631 + err;
	}
	/*---------------------------End of create file---------------------------*/

	/*-------------------------------Resize file------------------------------*/
	//create by name
	d_path = "/" + EXPECTED_ROOT_DIR[0].fname;
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 69);
		return 69;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 70" << std::endl;
		return 70;
	}

	expected_dir_size = dentries[0].fsize;

	err = emu_fs->list(d_path.c_str(), og_dir_cont);
	if(err)
	{
		print_unexpected_err(err, 71);
		return 71;
	}

	expected_dentry =
	{
		.fname = "15-Trunc_test_3",
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	f_path = d_path + "/Trunc_test_3";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 72);
		return 72;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 73);
		return 73;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 74" << std::endl;
		return 74;
	}

	if(dentries[0].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 75" << std::endl;
		return 75;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 76);
		return 76;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 77" << std::endl;
		return 77;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 78" << std::endl;
		return 78;
	}

	og_dir_cont.insert(og_dir_cont.cbegin() + 14, expected_dentry);

	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 79);
		return 79;
	}

	err = check_file_list(og_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 79 + err << std::endl;
		return 79 + err;
	}

	//Grow by bank num
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fsize = CLUSTER_SIZE;
	f_path = d_path + "/15-";
	
	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 82);
		return 82;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 83);
		return 83;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 84" << std::endl;
		return 84;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 85" << std::endl;
		return 85;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space - CLUSTER_SIZE * 1)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space - CLUSTER_SIZE * 1 << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: 130" << std::endl;
		return 130;
	}

	//Grow by name
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fsize = CLUSTER_SIZE + EMU::FS::BLK_SIZE;
	f_path = d_path + "/Trunc_test_3";

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 86);
		return 86;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 87);
		return 87;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 88" << std::endl;
		return 88;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 89" << std::endl;
		return 89;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space - CLUSTER_SIZE * 1)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space - CLUSTER_SIZE * 1 << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: 131" << std::endl;
		return 131;
	}

	//grow by bank num + invalid name
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fsize = CLUSTER_SIZE + EMU::FS::BLK_SIZE + 1;
	f_path = d_path + "/15-irrelevant";

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 90);
		return 90;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 91);
		return 91;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 92" << std::endl;
		return 92;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 93" << std::endl;
		return 93;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: 132" << std::endl;
		return 132;
	}

	//grow by invalid bank num + name
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fsize = CLUSTER_SIZE * 2 + 1;
	f_path = d_path + "/256-Trunc_test_3";

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 94);
		return 94;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 95);
		return 95;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 96" << std::endl;
		return 96;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 97" << std::endl;
		return 97;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space - CLUSTER_SIZE * 1)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space - CLUSTER_SIZE * 1 << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: 133" << std::endl;
		return 133;
	}

	//shrink by invalid bank num + name
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fsize = EMU::FS::BLK_SIZE;
	f_path = d_path + "/256-Trunc_test_3";

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 99);
		return 99;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 100);
		return 100;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 101" << std::endl;
		return 101;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 102" << std::endl;
		return 102;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space + CLUSTER_SIZE * 2)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space + CLUSTER_SIZE * 2 << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: 134" << std::endl;
		return 134;
	}

	//Grow by name with '/' in dir name and filename
	d_path = "/" + EXPECTED_ROOT_DIR[2].fname + "/";
	old_free_space = emu_fs->get_free_space();
	expected_dentry.fname = "3-Trunc_test\\6";
	expected_dentry.fsize = CLUSTER_SIZE + EMU::FS::BLK_SIZE;
	f_path = d_path + "Trunc_test\\6";

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 634);
		return 634;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 635);
		return 635;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 636 << std::endl;
		return 636;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 637 << std::endl;
		return 637;
	}

	new_free_space = emu_fs->get_free_space();
	if(new_free_space != old_free_space - CLUSTER_SIZE * 2)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << old_free_space - CLUSTER_SIZE * 1 << std::endl;
		std::cerr << "Got: " << new_free_space << std::endl;
		std::cerr << "Exit: " << 638 << std::endl;
		return 638;
	}
	/*---------------------------End of resize file---------------------------*/

	/*-------------------------------Expand dir-------------------------------*/
	//create by name
	next_dir_content_block = emu_fs->next_file_list_blk;
	d_path = "/" + EXPECTED_ROOT_DIR[1].fname;
	err = emu_fs->list("/", dentries);
	if(err)
	{
		print_unexpected_err(err, 103);
		return 103;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 104" << std::endl;
		return 104;
	}

	expected_dir_size = dentries[1].fsize + EMU::FS::BLK_SIZE;

	err = emu_fs->list(d_path.c_str(), og_dir_cont);
	if(err)
	{
		print_unexpected_err(err, 105);
		return 105;
	}

	expected_dentry =
	{
		.fname = "1-Trunc_test_4",
		.fsize = CLUSTER_SIZE + EMU::FS::BLK_SIZE + 1,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	f_path = d_path + "/Trunc_test_4";
	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 106);
		return 106;
	}

	if(emu_fs->next_file_list_blk == next_dir_content_block)
	{
		std::cerr << "Failed to update dir content block!!!" << std::endl;
		std::cerr << "Expected: !=" << next_dir_content_block << std::endl;
		std::cerr << "Got: " << emu_fs->next_file_list_blk << std::endl;
		std::cerr << "Exit: 362" << std::endl;
		return 362;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 107);
		return 107;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 108" << std::endl;
		return 108;
	}

	if(dentries[1].fsize != expected_dir_size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir_size << std::endl;
		std::cerr << "Got: " << dentries[1].fsize << std::endl;
		std::cerr << "Exit: 109" << std::endl;
		return 109;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 110);
		return 110;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 111" << std::endl;
		return 111;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 112" << std::endl;
		return 112;
	}
 
	og_dir_cont.insert(og_dir_cont.cend(), expected_dentry);

	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 113);
		return 113;
	}

	err = check_file_list(og_dir_cont, dentries, 0);
	if(err)
	{
		std::cerr << "Exit: " << 113 + err << std::endl;
		return 113 + err;
	}

	//Test DIR_SIZE_MAXED
	expected_err = ret_val_setup(EMU::FS::LIBRARY_ID, (u8)EMU::FS::ERR::DIR_SIZE_MAXED);

	for(u8 i = dentries.size(); i < EMU::FS::MAX_FILES_PER_DIR; i++)
	{
		f_path = d_path + "/auto_file_" + std::to_string(i - dentries.size());
		err = emu_fs->ftruncate(f_path.c_str(), 0);
		if(err)
		{
			print_unexpected_err(err, 116);
			return 116;
		}
	}

	f_path = d_path + "/auto_file_255";
	err = emu_fs->ftruncate(f_path.c_str(), 0);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 117);
		return 117;
	}

	//Test no space left (no dir content blocks left)
	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 118);
		return 118;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);

	for(u64 i = dentries.size(); i < emu_fs->header.dir_list_blk_cnt * EMU::FS::DIRS_PER_BLOCK; i++)
	{
		d_path = "/auto_dir_" + std::to_string(i - dentries.size());
		err = emu_fs->mkdir(d_path.c_str());
		if(err)
		{
			print_unexpected_err(err, 119);
			return 119;
		}
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 120);
		return 120;
	}

	for(filled_dirs = 0; filled_dirs < emu_fs->header.file_list_blk_cnt / EMU::FS::MAX_BLOCKS_PER_DIR; filled_dirs++)
	{
		d_path = dentries[filled_dirs].fname;

		og_dir_cont.clear();
		err = emu_fs->list(d_path.c_str(), og_dir_cont);
		if(err)
		{
			print_unexpected_err(err, 121);
			return 121;
		}

		for(u8 i = og_dir_cont.size(); i < EMU::FS::MAX_FILES_PER_DIR; i++)
		{
			f_path = d_path + "/auto_file_" + std::to_string(i - og_dir_cont.size());
			err = emu_fs->ftruncate(f_path.c_str(), 0);
			if(err)
			{
				print_unexpected_err(err, 122);
				return 122;
			}
		}
	}

	d_path = dentries[filled_dirs].fname;

	for(u8 i = og_dir_cont.size(); i < EMU::FS::MAX_FILES_PER_DIR; i++)
	{
		f_path = d_path + "/auto_file_" + std::to_string(i - og_dir_cont.size());
		err = emu_fs->ftruncate(f_path.c_str(), 0);
		if(err) break;
	}

	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 123);
		return 123;
	}
	/*----------------------------End of expand dir---------------------------*/

	//Test no space left (FAT full)
	d_path = "/" + EXPECTED_ROOT_DIR[0].fname;
	f_path = d_path + "/Trunc_test_1";

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 135);
		return 135;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 136" << std::endl;
		return 136;
	}
	
	old_free_space = emu_fs->get_free_space();
	free_clusters = dentries[0].fsize / CLUSTER_SIZE + ((dentries[0].fsize % CLUSTER_SIZE) != 0);
	new_free_space = old_free_space + free_clusters * CLUSTER_SIZE;

	err = emu_fs->ftruncate(f_path.c_str(), new_free_space);
	if(err)
	{
		print_unexpected_err(err, 137);
		return 137;
	}

	if(emu_fs->get_free_space())
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << emu_fs->get_free_space() << std::endl;
		std::cerr << "Exit: 138" << std::endl;
		return 138;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_SPACE_LEFT);
	err = emu_fs->ftruncate(f_path.c_str(), new_free_space + 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 139);
		return 139;
	}

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 365);
		return 365;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 366" << std::endl;
		return 366;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int rename_tests()
{
	constexpr char EMU_FS[] = "rename_fs.img";

	u8 cnt_a, cnt_b;
	u16 err, expected_err, fsck_status;

	std::string path, dir_a_path, dir_b_path;

	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries, expected_dir;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 341" << std::endl;
		return 435;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	err = emu_fs->rename("", "");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 436);
		return 436;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->rename("/nx_dir/nx_dir/nx_file", "");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 437);
		return 437;
	}

	//Special case
	expected_err = 0;
	err = emu_fs->rename("/nx_dir", "");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 438);
		return 438;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	err = emu_fs->rename("/nx_dir", "/nx_dir/nx_dir");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 439);
		return 439;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->rename("/nx_dir", "/nx_dir/nx_dir/nx_dir");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 440);
		return 440;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	err = emu_fs->rename("/nx_dir/nx_file", "");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 441);
		return 441;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->rename("/nx_dir/nx_file", "/nx_dir/nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 442);
		return 442;
	}

	//Rename non-existant by name and name
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str(),
						 (EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 443);
		return 443;
	}

	//Rename non-existant by bank num and name
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/255-").c_str(),
						 (EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 444);
		return 444;
	}

	//Rename non-existant by name and bank num
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str(),
						 (EXPECTED_ROOT_DIR[0].fname + "/255-").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 445);
		return 445;
	}

	//Rename non-existant by bank num and bank num
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/254-").c_str(),
						 (EXPECTED_ROOT_DIR[0].fname + "/255-").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 446);
		return 446;
	}

	//Move non-existant by name and name
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str(),
						 (EXPECTED_ROOT_DIR[1].fname + "/nx_file").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 447);
		return 447;
	}

	//Move non-existant by bank num and name
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/255-").c_str(),
						 (EXPECTED_ROOT_DIR[1].fname + "/nx_file").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 448);
		return 448;
	}

	//Move non-existant by name and bank num
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str(),
						 (EXPECTED_ROOT_DIR[1].fname + "/255-").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 449);
		return 449;
	}

	//Move non-existant by bank num and bank num
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/254-").c_str(),
						 (EXPECTED_ROOT_DIR[1].fname + "/255-").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 450);
		return 450;
	}
	/*------------------------End of common err testing-----------------------*/

	/*-------------------------------Rename dir-------------------------------*/
	expected_dentry = EXPECTED_ROOT_DIR[0];
	expected_dentry.fname = "Rename test 1";
	err = emu_fs->rename(EXPECTED_ROOT_DIR[0].fname.c_str(), expected_dentry.fname.c_str());
	if(err)
	{
		print_unexpected_err(err, 451);
		return 451;
	}

	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 452);
		return 452;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 453" << std::endl;
		return 453;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << dentries[0].to_string(0) << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 454" << std::endl;
		return 454;
	}

	//Try replace non-empty
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_EXISTS);
	err = emu_fs->rename(EXPECTED_ROOT_DIR[0].fname.c_str(), EXPECTED_ROOT_DIR[1].fname.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 455);
		return 455;
	}

	//Replace by rename
	expected_dentry = EXPECTED_ROOT_DIR[1];
	expected_dentry.fname = "Test rename 2";
	err = emu_fs->mkdir("Test rename 2");
	if(err)
	{
		print_unexpected_err(err, 456);
		return 456;
	}

	err = emu_fs->rename(EXPECTED_ROOT_DIR[1].fname.c_str(), expected_dentry.fname.c_str());
	if(err)
	{
		print_unexpected_err(err, 457);
		return 457;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 458);
		return 458;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 459" << std::endl;
		return 459;
	}

	if(dentries[1] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Exit: 460" << std::endl;
		return 460;
	}
	/*----------------------------End of rename dir---------------------------*/

	/*-------------------------------Rename file------------------------------*/
	//Dir as dest, should be special-cased to just return 0
	path = expected_dentry.fname;
	err = emu_fs->rename((path + "/" + EXPECTED_DIR1[0].fname).c_str(),
						 path.c_str());
	if(err)
	{
		print_unexpected_err(err, 461);
		return 461;
	}

	//Change name only
	expected_dentry = EXPECTED_DIR1[0];
	expected_dentry.fname = "15-Test rename 1";

	err = emu_fs->rename((path + "/" + EXPECTED_DIR1[0].fname).c_str(),
						 (path + "/Test rename 1").c_str());
	if(err)
	{
		print_unexpected_err(err, 462);
		return 462;
	}

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 463);
		return 463;
	}

	if(dentries.size() != EXPECTED_DIR1.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 464" << std::endl;
		return 464;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 465" << std::endl;
		return 465;
	}

	//Change bank num only
	err = emu_fs->rename((path + "/" + expected_dentry.fname).c_str(),
						 (path + "/255-").c_str());
	if(err)
	{
		print_unexpected_err(err, 466);
		return 466;
	}

	expected_dentry.fname = "255-Test rename 1";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 467);
		return 467;
	}

	if(dentries.size() != EXPECTED_DIR1.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 468" << std::endl;
		return 468;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 469" << std::endl;
		return 469;
	}

	//Change to preexisting bank num
	err = emu_fs->rename((path + "/" + expected_dentry.fname).c_str(),
						 (path + "/16-").c_str());
	if(err)
	{
		print_unexpected_err(err, 470);
		return 470;
	}

	expected_dentry.fname = "16-Test rename 1";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 471);
		return 471;
	}

	if(dentries.size() != EXPECTED_DIR1.size() - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1.size() - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 472" << std::endl;
		return 472;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 473" << std::endl;
		return 473;
	}

	//Change both
	err = emu_fs->rename((path + "/" + expected_dentry.fname).c_str(),
						 (path + "/127-Test rename 3").c_str());
	if(err)
	{
		print_unexpected_err(err, 474);
		return 474;
	}

	expected_dentry.fname = "127-Test rename 3";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 475);
		return 475;
	}

	if(dentries.size() != EXPECTED_DIR1.size() - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1.size() - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 476" << std::endl;
		return 476;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 477" << std::endl;
		return 477;
	}

	//Change both with preexisting new bank num
	err = emu_fs->rename((path + "/" + expected_dentry.fname).c_str(),
						 (path + "/17-Test rename 4").c_str());
	if(err)
	{
		print_unexpected_err(err, 478);
		return 478;
	}

	expected_dentry.fname = "17-Test rename 4";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 479);
		return 479;
	}

	if(dentries.size() != EXPECTED_DIR1.size() - 2)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR1.size() - 2 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 480" << std::endl;
		return 480;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 481" << std::endl;
		return 481;
	}

	//Now do it all again, with name-only as source
	//Change name only
	expected_dentry = EXPECTED_DIR0[4];
	expected_dentry.fname = "3-Test rename 1";
	path = "Rename test 1";

	err = emu_fs->rename((path + "/Windows G ENABLE").c_str(),
						 (path + "/Test rename 1").c_str());
	if(err)
	{
		print_unexpected_err(err, 482);
		return 482;
	}

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 483);
		return 483;
	}

	if(dentries.size() != EXPECTED_DIR0.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 484" << std::endl;
		return 484;
	}

	if(dentries[4] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[4].to_string(1) << std::endl;
		std::cerr << "Exit: 485" << std::endl;
		return 485;
	}

	//Change bank num only
	err = emu_fs->rename((path + "/Test rename 1").c_str(),
						 (path + "/255-").c_str());
	if(err)
	{
		print_unexpected_err(err, 486);
		return 486;
	}

	expected_dentry.fname = "255-Test rename 1";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 487);
		return 487;
	}

	if(dentries.size() != EXPECTED_DIR0.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 488" << std::endl;
		return 488;
	}

	if(dentries[4] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[4].to_string(1) << std::endl;
		std::cerr << "Exit: 489" << std::endl;
		return 489;
	}

	//Change to preexisting bank num
	err = emu_fs->rename((path + "/Test rename 1").c_str(),
						 (path + "/1-").c_str());
	if(err)
	{
		print_unexpected_err(err, 490);
		return 490;
	}

	expected_dentry.fname = "1-Test rename 1";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 491);
		return 491;
	}

	if(dentries.size() != EXPECTED_DIR0.size() - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 492" << std::endl;
		return 492;
	}

	if(dentries[3] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[3].to_string(1) << std::endl;
		std::cerr << "Exit: 493" << std::endl;
		return 493;
	}

	//Change both
	err = emu_fs->rename((path + "/Test rename 1").c_str(),
						 (path + "/127-Test rename 3").c_str());
	if(err)
	{
		print_unexpected_err(err, 494);
		return 494;
	}

	expected_dentry.fname = "127-Test rename 3";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 495);
		return 495;
	}

	if(dentries.size() != EXPECTED_DIR0.size() - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 496" << std::endl;
		return 496;
	}

	if(dentries[3] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[3].to_string(1) << std::endl;
		std::cerr << "Exit: 497" << std::endl;
		return 497;
	}

	//Change both with preexisting new bank num
	err = emu_fs->rename((path + "/Test rename 3").c_str(),
						 (path + "/2-Test rename 4").c_str());
	if(err)
	{
		print_unexpected_err(err, 498);
		return 498;
	}

	expected_dentry.fname = "2-Test rename 4";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 499);
		return 499;
	}

	if(dentries.size() != EXPECTED_DIR0.size() - 2)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() - 2 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 500" << std::endl;
		return 500;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 501" << std::endl;
		return 501;
	}

	//Now do it all yet again to test with '/' in names
	//Set a couple of files up
	path = EXPECTED_ROOT_DIR[2].fname + "/";

	expected_dir = EXPECTED_DIR2;

	expected_dir.push_back
	(
		{
			.fname = "4-Rename test\\1",
			.fsize = 0,
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::file
		}
	);

	expected_dir.push_back
	(
		{
			.fname = "5-Rename\\test\\2",
			.fsize = 0,
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::file
		}
	);

	expected_dir.push_back
	(
		{
			.fname = "6-Rename test\\3",
			.fsize = 0,
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::file
		}
	);

	expected_dir.push_back
	(
		{
			.fname = "7-Rename\\test 4",
			.fsize = 0,
			.ctime = 0,
			.mtime = 0,
			.atime = 0,
			.ftype = min_vfs::ftype_t::file
		}
	);

	for(size_t i = EXPECTED_DIR2.size(); i < expected_dir.size(); i++)
	{
		err = emu_fs->ftruncate((path + expected_dir[i].fname).c_str(), 0);
		if(err)
		{
			std::cout << (path + expected_dir[i].fname).c_str() << std::endl;
			print_unexpected_err(err, 639);
			return 639;
		}
	}

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 640);
		return 640;
	}

	if(dentries.size() != expected_dir.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 641 << std::endl;
		return 641;
	}

	//Dir as dest, should be special-cased to just return 0
	err = emu_fs->rename((path + "Test\\file\\1").c_str(),
						 path.c_str());
	if(err)
	{
		print_unexpected_err(err, 642);
		return 642;
	}

	//Name-only as source
	//Change name only
	expected_dentry = expected_dir[3];
	expected_dentry.fname = "5-Test\\rename\\4";

	err = emu_fs->rename((path + "Rename\\test\\2").c_str(),
						 (path + "Test\\rename\\4").c_str());
	if(err)
	{
		print_unexpected_err(err, 643);
		return 643;
	}

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 644);
		return 644;
	}

	if(dentries.size() != expected_dir.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 645 << std::endl;
		return 645;
	}

	if(dentries[3] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[3].to_string(1) << std::endl;
		std::cerr << "Exit: " << 646 << std::endl;
		return 646;
	}

	//Change both
	err = emu_fs->rename((path + "Test\\rename\\4").c_str(),
						 (path + "127-Test\\rename 5").c_str());
	if(err)
	{
		print_unexpected_err(err, 647);
		return 647;
	}

	expected_dentry.fname = "127-Test\\rename 5";

	dentries.clear();
	err = emu_fs->list(path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 648);
		return 648;
	}

	if(dentries.size() != expected_dir.size())
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 649 << std::endl;
		return 649;
	}

	if(dentries[3] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[3].to_string(1) << std::endl;
		std::cerr << "Exit: " << 650 << std::endl;
		return 650;
	}
	/*---------------------------End of rename file---------------------------*/

	/*--------------------------------Move file-------------------------------*/
	dir_a_path = "Rename test 1";
	dir_b_path = "Test rename 2";

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 502);
		return 502;
	}

	cnt_a = dentries.size();
	expected_dentry = dentries[3];

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 503);
		return 503;
	}

	cnt_b = dentries.size();

	//Src as name
	//Dir as dest
	err = emu_fs->rename((dir_a_path + "/Booti PlS!!1!  ").c_str(), dir_b_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 504);
		return 504;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 505);
		return 505;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 506" << std::endl;
		return 506;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 507" << std::endl;
		return 507;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 508);
		return 508;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 509" << std::endl;
		return 509;
	}

	if(dentries[1] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[1].to_string(1) << std::endl;
		std::cerr << "Exit: 510" << std::endl;
		return 510;
	}

	//Bank num only as dest
	expected_dentry.fname = "111-Booti PlS!!1!  ";
	err = emu_fs->rename((dir_b_path + "/Booti PlS!!1!  ").c_str(),
						 (dir_a_path + "/111-").c_str());
	if(err)
	{
		print_unexpected_err(err, 511);
		return 511;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 512);
		return 512;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 513" << std::endl;
		return 513;
	}

	if(dentries[1] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 514" << std::endl;
		return 514;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 515);
		return 515;
	}

	if(dentries.size() != cnt_a)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 516" << std::endl;
		return 516;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 517" << std::endl;
		return 517;
	}

	//Name only as dest
	expected_dentry.fname = "111-Move test";
	err = emu_fs->rename((dir_a_path + "/Booti PlS!!1!  ").c_str(),
						 (dir_b_path + "/Move test").c_str());
	if(err)
	{
		print_unexpected_err(err, 518);
		return 518;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 519);
		return 519;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 520" << std::endl;
		return 520;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 521" << std::endl;
		return 521;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 522);
		return 522;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 523" << std::endl;
		return 523;
	}

	if(dentries[1] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[1].to_string(1) << std::endl;
		std::cerr << "Exit: 524" << std::endl;
		return 524;
	}

	//Both as dest
	expected_dentry.fname = "192-Move test 2";
	err = emu_fs->rename((dir_b_path + "/Move test").c_str(),
						 (dir_a_path + "/192-Move test 2").c_str());
	if(err)
	{
		print_unexpected_err(err, 525);
		return 525;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 526);
		return 526;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 527" << std::endl;
		return 527;
	}

	if(dentries[1] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 528" << std::endl;
		return 528;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 529);
		return 529;
	}

	if(dentries.size() != cnt_a)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 530" << std::endl;
		return 530;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 531" << std::endl;
		return 531;
	}

	//Existing bank num only as dest
	expected_dentry.fname = "17-Move test 2";
	err = emu_fs->rename((dir_a_path + "/Move test 2").c_str(),
						 (dir_b_path + "/17-").c_str());
	if(err)
	{
		print_unexpected_err(err, 532);
		return 532;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 533);
		return 533;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 534" << std::endl;
		return 534;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 535" << std::endl;
		return 535;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 536);
		return 536;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 537" << std::endl;
		return 537;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 538" << std::endl;
		return 538;
	}

	//Both as dest with existing bank num
	expected_dentry.fname = "2-Move test 3";
	err = emu_fs->rename((dir_b_path + "/Move test 2").c_str(),
						 (dir_a_path + "/2-Move test 3").c_str());
	if(err)
	{
		print_unexpected_err(err, 539);
		return 539;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 540);
		return 540;
	}

	if(dentries.size() != cnt_b - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 541" << std::endl;
		return 541;
	}

	if(dentries[1] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 542" << std::endl;
		return 542;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 543);
		return 543;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 544" << std::endl;
		return 544;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 545" << std::endl;
		return 545;
	}

	//Now do it all over again but with the bank number as source, just in case
	dir_a_path = "Rename test 1";
	dir_b_path = "Test rename 2";

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 556);
		return 556;
	}

	cnt_a = dentries.size();
	expected_dentry = dentries[2];

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 557);
		return 557;
	}

	cnt_b = dentries.size();

	//Src as name
	//Dir as dest
	err = emu_fs->rename((dir_a_path + "/2-").c_str(), dir_b_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 558);
		return 558;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 559);
		return 559;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 560" << std::endl;
		return 560;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 561" << std::endl;
		return 561;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 562);
		return 562;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 563" << std::endl;
		return 563;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 564" << std::endl;
		return 564;
	}

	//Bank num only as dest
	expected_dentry.fname = "111-Move test 3";
	err = emu_fs->rename((dir_b_path + "/2-").c_str(),
						 (dir_a_path + "/111-").c_str());
	if(err)
	{
		print_unexpected_err(err, 565);
		return 565;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 566);
		return 566;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 567" << std::endl;
		return 567;
	}

	if(dentries[0] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 568" << std::endl;
		return 568;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 569);
		return 569;
	}

	if(dentries.size() != cnt_a)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 570" << std::endl;
		return 570;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 571" << std::endl;
		return 571;
	}

	//Name only as dest
	expected_dentry.fname = "111-Move test 4";
	err = emu_fs->rename((dir_a_path + "/111-").c_str(),
						 (dir_b_path + "/Move test 4").c_str());
	if(err)
	{
		print_unexpected_err(err, 572);
		return 572;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 573);
		return 573;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 574" << std::endl;
		return 574;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 575" << std::endl;
		return 575;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 576);
		return 576;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 577" << std::endl;
		return 577;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 578" << std::endl;
		return 578;
	}

	//Both as dest
	expected_dentry.fname = "192-Move test 5";
	err = emu_fs->rename((dir_b_path + "/111-").c_str(),
						 (dir_a_path + "/192-Move test 5").c_str());
	if(err)
	{
		print_unexpected_err(err, 579);
		return 579;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 580);
		return 580;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 581" << std::endl;
		return 581;
	}

	if(dentries[0] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 582" << std::endl;
		return 582;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 583);
		return 583;
	}

	if(dentries.size() != cnt_a)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 584" << std::endl;
		return 584;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 585" << std::endl;
		return 585;
	}

	//Existing bank num only as dest
	expected_dentry.fname = "18-Move test 5";
	err = emu_fs->rename((dir_a_path + "/192-").c_str(),
						 (dir_b_path + "/18-").c_str());
	if(err)
	{
		print_unexpected_err(err, 586);
		return 586;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 587);
		return 587;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 588" << std::endl;
		return 588;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 589" << std::endl;
		return 589;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 590);
		return 590;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 591" << std::endl;
		return 591;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 592" << std::endl;
		return 592;
	}

	//Both as dest with existing bank num
	expected_dentry.fname = "5-Move test 6";
	err = emu_fs->rename((dir_b_path + "/18-").c_str(),
						 (dir_a_path + "/5-Move test 6").c_str());
	if(err)
	{
		print_unexpected_err(err, 593);
		return 593;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 594);
		return 594;
	}

	if(dentries.size() != cnt_b - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 595" << std::endl;
		return 595;
	}

	if(dentries[1] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: 596" << std::endl;
		return 596;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 597);
		return 597;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 598" << std::endl;
		return 598;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: 599" << std::endl;
		return 599;
	}

	//Do it all yet again testing with '/' in names
	dir_a_path = "Test\\dir\\2/";
	dir_b_path = "Test\\dir 3/";
	err = emu_fs->mkdir(dir_b_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 652);
		return 652;
	}

	expected_dir.clear();
	err = emu_fs->list("", expected_dir);
	if(err)
	{
		print_unexpected_err(err, 653);
		return 653;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 654);
		return 654;
	}

	cnt_a = dentries.size();
	expected_dentry = dentries[0];

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 655);
		return 655;
	}

	cnt_b = dentries.size();

	//Name as src
	//Dir as dest, also grow dst dir
	expected_dentry.fname = "4-Rename test\\1";
	err = emu_fs->rename((dir_a_path + "Rename test\\1").c_str(), dir_b_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 656);
		return 656;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 657);
		return 657;
	}

	//Check dir grew
	if(dentries[1].fsize != expected_dir[1].fsize + EMU::FS::BLK_SIZE)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dir[1].fsize + EMU::FS::BLK_SIZE << std::endl;
		std::cerr << "Got: " << dentries[1].fsize << std::endl;
		std::cerr << "Exit: " << 658 << std::endl;
		return 658;
	}

	//Check growing dir didn't mess anything up
	expected_dir[1].fsize = EMU::FS::BLK_SIZE;
	if(dentries[1] != expected_dir[1])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dir[1].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[1].to_string(1) << std::endl;
		std::cerr << "Exit: " << 659 << std::endl;
		return 659;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 660);
		return 660;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 661 << std::endl;
		return 661;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: " << 662 << std::endl;
		return 662;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 663);
		return 663;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 664 << std::endl;
		return 664;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 665 << std::endl;
		return 665;
	}

	//Name only as dest
	expected_dentry.fname = "4-Move\\test";
	err = emu_fs->rename((dir_b_path + "Rename test\\1").c_str(),
						 (dir_a_path + "Move\\test").c_str());
	if(err)
	{
		print_unexpected_err(err, 666);
		return 666;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 667);
		return 667;
	}

	if(dentries.size() != cnt_b)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 668 << std::endl;
		return 668;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 669);
		return 669;
	}

	if(dentries.size() != cnt_a)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 670 << std::endl;
		return 670;
	}

	if(dentries[2] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[2].to_string(1) << std::endl;
		std::cerr << "Exit: " << 671 << std::endl;
		return 671;
	}

	//Both as dest
	expected_dentry.fname = "192-Move test\\2";
	err = emu_fs->rename((dir_a_path + "/Move\\test").c_str(),
						 (dir_b_path + "/192-Move test\\2").c_str());
	if(err)
	{
		print_unexpected_err(err, 672);
		return 672;
	}

	dentries.clear();
	err = emu_fs->list(dir_a_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 673);
		return 673;
	}

	if(dentries.size() != cnt_a - 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_a - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 674 << std::endl;
		return 674;
	}

	if(dentries[2] == expected_dentry)
	{
		std::cerr << "File wasn't correctly removed from source directory!!!" << std::endl;
		std::cerr << expected_dentry.to_string(0) << std::endl;
		std::cerr << "Exit: " << 675 << std::endl;
		return 675;
	}

	dentries.clear();
	err = emu_fs->list(dir_b_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 676);
		return 676;
	}

	if(dentries.size() != cnt_b + 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << cnt_b + 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 677 << std::endl;
		return 677;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 678 << std::endl;
		return 678;
	}
	/*----------------------------End of move file----------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 546);
		return 546;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 547" << std::endl;
		return 547;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int remove_tests()
{
	constexpr char EMU_FS[] = "remove_fs.img";

	u16 err, expected_err, cluster_cnt, fsck_status;
	uintmax_t free_space, expected_free_space, fsize;

	std::string d_path, f_path;

	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 140" << std::endl;
		return 140;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs->remove("/nx_dir");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 141);
		return 141;
	}

	err = emu_fs->remove((EXPECTED_ROOT_DIR[0].fname + "/nx_file").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 142);
		return 142;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->remove("/nx_dir/nx_dir/nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 143);
		return 143;
	}
	/*------------------------End of common err testing-----------------------*/

	/*-------------------------------Remove file------------------------------*/
	d_path = EXPECTED_ROOT_DIR[0].fname;
	f_path = d_path + "/" + EXPECTED_DIR0[4].fname;
	free_space = emu_fs->get_free_space();

	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 144);
		return 144;
	}

	fsize = dentries[0].fsize;

	err = emu_fs->remove(f_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 145);
		return 145;
	}

	dentries.clear();
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 146);
		return 146;
	}

	if(dentries.size() != EXPECTED_DIR0.size() - 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_DIR0.size() - 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 147" << std::endl;
		return 147;
	}

	expected_free_space = free_space + CLUSTER_SIZE * (fsize / CLUSTER_SIZE + (fsize % CLUSTER_SIZE != 0));

	if(emu_fs->get_free_space() != expected_free_space)
	{
		std::cerr << "Expected free space mismatch!!!" << std::endl;
		std::cerr << "Initial: " << free_space << std::endl;
		std::cerr << "Expected: " << expected_free_space << std::endl;
		std::cerr << "Got: " << emu_fs->get_free_space() << std::endl;
		std::cerr << "Exit: 148" << std::endl;
		return 148;
	}

	cluster_cnt = FAT_utils::count_free_clusters(emu_fs->FAT.get(), EMU::FS::FAT_ATTRS, emu_fs->FAT_attrs.LENGTH);
	if(cluster_cnt != expected_free_space / CLUSTER_SIZE)
	{
		std::cerr << "Free cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_free_space / CLUSTER_SIZE << std::endl;
		std::cerr << "Got: " << cluster_cnt << std::endl;
		std::cerr << "Exit: 149" << std::endl;
		return 149;
	}

	//Make sure it won't try to delete a deleted file
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs->remove(f_path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 150);
		return 150;
	}

	//Now test it by name
	err = emu_fs->remove((d_path + "/Windows G ENABLE").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 151);
		return 151;
	}
	/*---------------------------End of remove file---------------------------*/

	/*-------------------------------Remove dir-------------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	free_space = emu_fs->get_free_space();

	/*Make sure there's at least one deleted file in the directory to make sure
	it doesn't try to delete it again.*/
	err = emu_fs->remove((d_path + "/" + EXPECTED_DIR1[5].fname).c_str());
	if(err)
	{
		print_unexpected_err(err, 152);
		return 152;
	}
	
	err = emu_fs->remove(d_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 153);
		return 153;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs->list(d_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 154);
		return 154;
	}

	expected_free_space = free_space;

	for(const min_vfs::dentry_t &dentry : EXPECTED_DIR1)
		expected_free_space += CLUSTER_SIZE * (dentry.fsize / CLUSTER_SIZE + (dentry.fsize % CLUSTER_SIZE != 0));

	if(emu_fs->get_free_space() != expected_free_space)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_free_space << std::endl;
		std::cerr << "Got: " << emu_fs->get_free_space() << std::endl;
		std::cerr << "Exit: 155" << std::endl;
		return 155;
	}
	/*----------------------------End of remove dir---------------------------*/

	/*-------------------------------Remove root------------------------------*/
	//Make sure there's at least 2 directories.
	err = emu_fs->mkdir("remove_test");
	if(err)
	{
		print_unexpected_err(err, 156);
		return 156;
	}

	err = emu_fs->ftruncate("remove_test/temp_file", 0);
	if(err)
	{
		print_unexpected_err(err, 157);
		return 157;
	}
	
	err = emu_fs->remove("");
	if(err)
	{
		print_unexpected_err(err, 158);
		return 158;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_expected_err(expected_err, err, 159);
		return 159;
	}

	if(dentries.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 160" << std::endl;
		return 160;
	}

	free_space = CLUSTER_SIZE * EXPECTED_HEADER.cluster_cnt;
	if(emu_fs->get_free_space() != free_space)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << free_space << std::endl;
		std::cerr << "Got: " << emu_fs->get_free_space() << std::endl;
		std::cerr << "Exit: 161" << std::endl;
		return 161;
	}

	for(const bool block: emu_fs->dir_content_block_map)
	{
		if(block)
		{
			std::cerr << "Failed to free dir content blocks!!!" << std::endl;
			std::cerr << "Exit: 162" << std::endl;
			return 162;
		}
	}
	/*---------------------------End of remove root---------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 367);
		return 367;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 368" << std::endl;
		return 368;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int fopen_tests()
{
	constexpr char EMU_FS[] = "fopen_fs.img";

	u16 err, expected_err, cluster_cnt, fsck_status;
	uintmax_t refcount, size;

	std::string d_path, f_path, bank_num_f_path, other_fpath,
		other_bank_num_f_path, other_d_path;

	min_vfs::dentry_t expected_dentry;
	min_vfs::stream_t stream_a, stream_b;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 163" << std::endl;
		return 163;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = emu_fs->fopen("", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 164);
		return 164;
	}

	err = emu_fs->fopen("/nx_dir", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 165);
		return 165;
	}

	err = emu_fs->fopen("/nx_dir/nx_dir/nx_file", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 166);
		return 166;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND);
	err = emu_fs->fopen("nx_dir/nx_file", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 167);
		return 167;
	}
	/*------------------------End of common err testing-----------------------*/
	
	/*-------------------------fopen preexisting file-------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	f_path = d_path + "/" + EXPECTED_DIR1[0].fname;
	bank_num_f_path = d_path + "/15";

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 168);
		return 168;
	}

	//Opening a file should open the parent directory, at least for now.
	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 169" << std::endl;
		return 169;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 170" << std::endl;
		return 170;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 171" << std::endl;
		return 171;
	}

	//Open same stream again (by name)
	err = emu_fs->fopen((d_path + "/Melopuerto pls p").c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 172);
		return 172;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 173" << std::endl;
		return 173;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 174" << std::endl;
		return 174;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 175" << std::endl;
		return 175;
	}

	//Open second stream
	err = emu_fs->fopen(f_path.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 176);
		return 176;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 177" << std::endl;
		return 177;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 178" << std::endl;
		return 178;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 2)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 179" << std::endl;
		return 179;
	}

	//Close second stream
	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 180);
		return 180;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 181" << std::endl;
		return 181;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 182" << std::endl;
		return 182;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 183" << std::endl;
		return 183;
	}

	//Open other file
	other_fpath = d_path + "/" + EXPECTED_DIR1[3].fname;
	other_bank_num_f_path = d_path + "/18";

	err = emu_fs->fopen(other_fpath.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 184);
		return 184;
	}

	if(emu_fs->open_files.size() != 3)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 3" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 185" << std::endl;
		return 185;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 2)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 186" << std::endl;
		return 186;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 187" << std::endl;
		return 187;
	}

	refcount = emu_fs->open_files.at(other_bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 188" << std::endl;
		return 188;
	}

	//Close second file
	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 189);
		return 189;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 190" << std::endl;
		return 190;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 191" << std::endl;
		return 191;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 192" << std::endl;
		return 192;
	}

	//Open file in other dir
	other_d_path = EXPECTED_ROOT_DIR[0].fname;
	other_fpath = other_d_path + "/" + EXPECTED_DIR0[1].fname;
	other_bank_num_f_path = other_d_path + "/107";

	err = emu_fs->fopen(other_fpath.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 193);
		return 193;
	}

	if(emu_fs->open_files.size() != 4)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 4" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 194" << std::endl;
		return 194;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 195" << std::endl;
		return 195;
	}

	refcount = emu_fs->open_files.at(other_d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 196" << std::endl;
		return 196;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 197" << std::endl;
		return 197;
	}

	refcount = emu_fs->open_files.at(other_bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 198" << std::endl;
		return 198;
	}

	//Close file in other dir
	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 199);
		return 199;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 200" << std::endl;
		return 200;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 201" << std::endl;
		return 201;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 202" << std::endl;
		return 202;
	}

	//Close first file
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 203);
		return 203;
	}

	if(emu_fs->open_files.size())
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 0" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 204" << std::endl;
		return 204;
	}

	//Test with '/' in names
	d_path = EXPECTED_ROOT_DIR[2].fname;

	//Open first file
	f_path = d_path + "/Test\\file\\1";
	bank_num_f_path = d_path + "/0";
	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 679);
		return 679;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 680 << std::endl;
		return 680;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: " << 681 << std::endl;
		return 681;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 682" << std::endl;
		return 682;
	}

	//Open second file
	f_path = d_path + "/fopen\\create";
	bank_num_f_path = d_path + "/2";

	err = emu_fs->fopen(f_path.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 683);
		return 683;
	}

	if(emu_fs->open_files.size() != 3)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 3" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 684 << std::endl;
		return 684;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 2)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: " << 685 << std::endl;
		return 685;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 686" << std::endl;
		return 686;
	}

	//Close them both
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 687);
		return 687;
	}

	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 688);
		return 688;
	}
	/*----------------------End of fopen preexisting file---------------------*/

	/*-----------------------------Create by fopen----------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	f_path = d_path + "/fopen_create";
	bank_num_f_path = d_path + "/1";
	expected_dentry =
	{
		.fname = "1-fopen_create",
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	//Create by name
	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 205);
		return 205;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 206" << std::endl;
		return 206;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 207" << std::endl;
		return 207;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 208" << std::endl;
		return 208;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 209);
		return 209;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 210" << std::endl;
		return 210;
	}

	size = EXPECTED_ROOT_DIR[1].fsize + EMU::FS::BLK_SIZE;
	if(dentries[1].fsize != size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << size << std::endl;
		std::cerr << "Got: " << dentries[1].fsize << std::endl;
		std::cerr << "Exit: 211" << std::endl;
		return 211;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 212);
		return 212;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 213" << std::endl;
		return 213;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 214" << std::endl;
		return 214;
	}

	//Create by bank num and name
	expected_dentry.fname = "70-fopen_create2";
	f_path = d_path + "/" + expected_dentry.fname;
	bank_num_f_path = d_path + "/70";

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 215);
		return 215;
	}

	if(emu_fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 2" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 216" << std::endl;
		return 216;
	}

	refcount = emu_fs->open_files.at(d_path).first;
	if(refcount != 1)
	{
		std::cerr << "Dir refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 217" << std::endl;
		return 217;
	}

	refcount = emu_fs->open_files.at(bank_num_f_path).first;
	if(refcount != 1)
	{
		std::cerr << "File refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << refcount << std::endl;
		std::cerr << "Exit: 218" << std::endl;
		return 218;
	}

	dentries.clear();
	err = emu_fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 219);
		return 219;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 220" << std::endl;
		return 220;
	}

	size = EXPECTED_ROOT_DIR[1].fsize + EMU::FS::BLK_SIZE;
	if(dentries[1].fsize != size)
	{
		std::cerr << "Dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << size << std::endl;
		std::cerr << "Got: " << dentries[1].fsize << std::endl;
		std::cerr << "Exit: 221" << std::endl;
		return 221;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 222);
		return 222;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 223" << std::endl;
		return 223;
	}

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: 224" << std::endl;
		return 224;
	}
	/*-------------------------End of create by fopen-------------------------*/

	/*-------------------------------Remove open------------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	f_path = d_path + "/" + EXPECTED_DIR1[0].fname;

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 225);
		return 225;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = emu_fs->remove(f_path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 226);
		return 226;
	}

	err = emu_fs->remove(d_path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 227);
		return 227;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 363);
		return 363;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: 1" << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 364" << std::endl;
		return 364;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 548);
		return 548;
	}
	/*---------------------------End of remove open---------------------------*/
	
	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 369);
		return 369;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 370" << std::endl;
		return 370;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int read_tests()
{
	constexpr char EMU_FS[] = "read_fs.img";

	u16 err, expected_err, cluster_cnt, fsck_status;
	uintmax_t size;

	std::string d_path, f_path;
	std::unique_ptr<u8[]> buffer, buffer2;
	std::ifstream host_fs_file;

	min_vfs::dentry_t expected_dentry;
	min_vfs::stream_t stream_a, stream_b;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 234" << std::endl;
		return 234;
	}
	/*----------------------------End of data setup---------------------------*/

	/*--------------------------------Read file-------------------------------*/
	d_path = EXPECTED_ROOT_DIR[0].fname;
	expected_dentry = EXPECTED_DIR0[11];
	f_path = d_path + "/" + expected_dentry.fname;

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 235);
		return 235;
	}

	buffer = std::make_unique<u8[]>(CLUSTER_SIZE);
	buffer2 = std::make_unique<u8[]>(CLUSTER_SIZE);
	cluster_cnt = expected_dentry.fsize / CLUSTER_SIZE;
	host_fs_file.open("test_file", std::ios_base::binary);

	//Partial read
	err = stream_a.read(buffer.get(), EMU::FS::BLK_SIZE);
	if(err)
	{
		print_unexpected_err(err, 236);
		return 236;
	}

	host_fs_file.read((char*)buffer2.get(), EMU::FS::BLK_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), EMU::FS::BLK_SIZE))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), EMU::FS::BLK_SIZE);
		std::cerr << "Exit: 237" << std::endl;
		return 237;
	}

	//Read whole file
	err = stream_a.seek(0);
	if(err)
	{
		print_unexpected_err(err, 238);
		return 238;
	}

	host_fs_file.seekg(0);

	for(uintmax_t i = 0; i < cluster_cnt; i++)
	{
		err = stream_a.read(buffer.get(), CLUSTER_SIZE);
		if(err)
		{
			print_unexpected_err(err, 239);
			return 239;
		}

		host_fs_file.read((char*)buffer2.get(), CLUSTER_SIZE);

		if(std::memcmp(buffer.get(), buffer2.get(), CLUSTER_SIZE))
		{
			std::cerr << "Diff in data!!!" << std::endl;
			print_buffers(buffer.get(), buffer2.get(), CLUSTER_SIZE);
			std::cerr << "Exit: 240" << std::endl;
			return 240;
		}
	}

	size = expected_dentry.fsize - cluster_cnt * CLUSTER_SIZE;

	err = stream_a.read(buffer.get(), size);
	if(err)
	{
		print_unexpected_err(err, 241);
		return 241;
	}

	host_fs_file.read((char*)buffer2.get(), size);

	if(std::memcmp(buffer.get(), buffer2.get(), size))
	{
		std::cerr << "Diff in audio data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), size);
		std::cerr << "Exit: 242" << std::endl;
		return 242;
	}

	//mid-cluster read
	err = stream_a.seek(CLUSTER_SIZE / 2);
	if(err)
	{
		print_unexpected_err(err, 243);
		return 243;
	}

	host_fs_file.seekg(CLUSTER_SIZE / 2);

	err = stream_a.read(buffer.get(), CLUSTER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 244);
		return 244;
	}

	host_fs_file.read((char*)buffer2.get(), CLUSTER_SIZE);

	if(std::memcmp(buffer.get(), buffer2.get(), CLUSTER_SIZE))
	{
		std::cerr << "Diff in audio data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), CLUSTER_SIZE);
		std::cerr << "Exit: 245" << std::endl;
		return 245;
	}

	//Whole cluster tests
	buffer = std::make_unique<u8[]>(CLUSTER_SIZE * 2);
	buffer2 = std::make_unique<u8[]>(CLUSTER_SIZE * 2);

	//aligned
	err = stream_a.seek(0);
	if(err)
	{
		print_unexpected_err(err, 246);
		return 246;
	}

	host_fs_file.seekg(0);

	err = stream_a.read(buffer.get(), CLUSTER_SIZE * 2);
	if(err)
	{
		print_unexpected_err(err, 247);
		return 247;
	}

	host_fs_file.read((char*)buffer2.get(), CLUSTER_SIZE * 2);

	if(std::memcmp(buffer.get(), buffer2.get(), CLUSTER_SIZE * 2))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), CLUSTER_SIZE * 2);
		std::cerr << "Exit: 248" << std::endl;
		return 248;
	}

	//unaligned
	err = stream_a.seek(CLUSTER_SIZE / 2);
	if(err)
	{
		print_unexpected_err(err, 249);
		return 249;
	}

	host_fs_file.seekg(CLUSTER_SIZE / 2);

	err = stream_a.read(buffer.get(), CLUSTER_SIZE * 2);
	if(err)
	{
		print_unexpected_err(err, 250);
		return 250;
	}

	host_fs_file.read((char*)buffer2.get(), CLUSTER_SIZE * 2);

	if(std::memcmp(buffer.get(), buffer2.get(), CLUSTER_SIZE * 2))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), CLUSTER_SIZE * 2);
		std::cerr << "Exit: 251" << std::endl;
		return 251;
	}

	buffer = std::make_unique<u8[]>(CLUSTER_SIZE);
	buffer2 = std::make_unique<u8[]>(CLUSTER_SIZE);

	host_fs_file.close();

	//EOF tests
	err = stream_a.seek(expected_dentry.fsize);
	if(err)
	{
		print_unexpected_err(err, 252);
		return 252;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	err = stream_a.read(buffer.get(), 1);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 253);
		return 253;
	}

	size = stream_a.get_pos();
	err = stream_a.seek(-1, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 254);
		return 254;
	}

	if(stream_a.get_pos() != size - 1)
	{
		std::cerr << "Unexpected pos!!!" << std::endl;
		std::cerr << "Expected: " << size - 1 << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 255" << std::endl;
		return 255;
	}

	err = stream_a.read(buffer.get(), 2);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 256);
		return 256;
	}

	stream_a.close();
	/*----------------------------End of read file----------------------------*/

	return 0;
}

static int write_tests()
{
	constexpr char EMU_FS[] = "write_fs.img";

	u16 err, expected_err, cluster_cnt, fsck_status;
	size_t buf_size;
	uintmax_t size, pos;

	std::string d_path, f_path;
	std::unique_ptr<u8[]> buffer, buffer2;
	std::ifstream host_fs_file;

	min_vfs::dentry_t expected_dentry;
	min_vfs::stream_t stream_a, stream_b;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 257" << std::endl;
		return 257;
	}
	/*----------------------------End of data setup---------------------------*/

	/*-------------------------------Write file-------------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	expected_dentry = EXPECTED_DIR1[4];
	f_path = d_path + "/" + expected_dentry.fname;

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 258);
		return 258;
	}

	buf_size = CLUSTER_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 259);
		return 259;
	}

	if(stream_a.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 260" << std::endl;
		return 260;
	}

	err = stream_a.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 261);
		return 261;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 262);
		return 262;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 263" << std::endl;
		return 263;
	}

	//Whole cls test
	buf_size = CLUSTER_SIZE * 2;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	pos = 0;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 264);
		return 264;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 265);
		return 265;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 266" << std::endl;
		return 266;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 267);
		return 267;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 268);
		return 268;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 269" << std::endl;
		return 269;
	}

	//Unaligned cls test
	buf_size = CLUSTER_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xF0, buf_size);

	pos = CLUSTER_SIZE / 2;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 270);
		return 270;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 271);
		return 271;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 272" << std::endl;
		return 272;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 273);
		return 273;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 274);
		return 274;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 275" << std::endl;
		return 275;
	}

	//Extend by write (within final block)
	pos = expected_dentry.fsize;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 276);
		return 276;
	}

	err = stream_a.write(buffer.get(), 1);
	if(err)
	{
		print_unexpected_err(err, 277);
		return 277;
	}

	if(stream_a.get_pos() != pos + 1)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + 1 << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 278" << std::endl;
		return 278;
	}

	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 279);
		return 279;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 280" << std::endl;
		return 280;
	}

	if(dentries[0].fsize != pos + 1)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + 1 << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 281" << std::endl;
		return 281;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 282);
		return 282;
	}

	err = stream_a.read(buffer2.get(), 1);
	if(err)
	{
		print_unexpected_err(err, 283);
		return 283;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), 1))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), 1);
		std::cerr << "Exit: 284" << std::endl;
		return 284;
	}
	
	//Extend by write (within final cluster)
	size = EMU::FS::BLK_SIZE + 1;
	pos = dentries[0].fsize;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 285);
		return 285;
	}

	err = stream_a.write(buffer.get(), size);
	if(err)
	{
		print_unexpected_err(err, 286);
		return 286;
	}

	if(stream_a.get_pos() != pos + size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 287" << std::endl;
		return 287;
	}

	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 288);
		return 288;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 289" << std::endl;
		return 289;
	}

	if(dentries[0].fsize != pos + size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 290" << std::endl;
		return 290;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 291);
		return 291;
	}

	err = stream_a.read(buffer2.get(), size);
	if(err)
	{
		print_unexpected_err(err, 292);
		return 292;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), size);
		std::cerr << "Exit: 293" << std::endl;
		return 293;
	}

	//Extend by write tests (a.k.a. write past EOF)
	pos = dentries[0].fsize;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 294);
		return 294;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 295);
		return 295;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 296" << std::endl;
		return 296;
	}

	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 297);
		return 297;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 298" << std::endl;
		return 298;
	}

	if(dentries[0].fsize != pos + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 299" << std::endl;
		return 299;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 300);
		return 300;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 301);
		return 301;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 302" << std::endl;
		return 302;
	}

	//Extend unaligned
	size = dentries[0].fsize;
	pos = size - CLUSTER_SIZE / 2;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 303);
		return 303;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 304);
		return 304;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 305" << std::endl;
		return 305;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 306);
		return 306;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 307" << std::endl;
		return 307;
	}

	if(dentries[0].fsize != pos + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 308" << std::endl;
		return 308;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 309);
		return 309;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 310);
		return 310;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 311" << std::endl;
		return 311;
	}

	//Extend (whole cls, buf_size = CLUSTER_SIZE * 2)
	buf_size = CLUSTER_SIZE * 2;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	size = dentries[0].fsize;
	pos = size;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 312);
		return 312;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 313);
		return 313;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 314" << std::endl;
		return 314;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 315);
		return 315;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 316" << std::endl;
		return 316;
	}

	if(dentries[0].fsize != size + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << size + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 317" << std::endl;
		return 317;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 318);
		return 318;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 319);
		return 319;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 320" << std::endl;
		return 320;
	}

	//Extend unaligned (whole cls, buf_size = CLUSTER_SIZE * 2)
	size = dentries[0].fsize;
	pos = size - CLUSTER_SIZE / 2;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 321);
		return 321;
	}

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 322);
		return 322;
	}

	if(stream_a.get_pos() != pos + buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 323" << std::endl;
		return 323;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 324);
		return 324;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "File count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 325" << std::endl;
		return 325;
	}

	if(dentries[0].fsize != pos + buf_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + buf_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 326" << std::endl;
		return 326;
	}

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 327);
		return 327;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 328);
		return 328;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: 329" << std::endl;
		return 329;
	}

	//Make sure cluster aligned writes with size 0 at the end of file do NOT extend
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 330);
		return 330;
	}

	err = emu_fs->ftruncate(f_path.c_str(), CLUSTER_SIZE * 3);
	if(err)
	{
		print_unexpected_err(err, 331);
		return 331;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 332);
		return 332;
	}

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 333);
		return 333;
	}

	buf_size = CLUSTER_SIZE * 2;

	buffer = std::make_unique<u8[]>(buf_size);
	size = dentries[0].fsize;
	pos = size;

	err = stream_a.seek(pos);
	if(err)
	{
		print_unexpected_err(err, 334);
		return 334;
	}

	err = stream_a.write(buffer.get(), 0);
	if(err)
	{
		print_unexpected_err(err, 335);
		return 335;
	}

	if(stream_a.get_pos() != pos + 0)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << pos + 0 << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: 336" << std::endl;
		return 336;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
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

	if(dentries[0].fsize != size + 0)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << size + 0 << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 339" << std::endl;
		return 339;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 340);
		return 340;
	}

	//Test with '/' in names
	d_path = EXPECTED_ROOT_DIR[2].fname;
	expected_dentry = EXPECTED_DIR2[0];
	f_path = d_path + "/Test\\file\\1";

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 689);
		return 689;
	}

	buf_size = CLUSTER_SIZE;

	buffer = std::make_unique<u8[]>(buf_size);
	buffer2 = std::make_unique<u8[]>(buf_size);
	std::memset(buffer.get(), 0xAA, buf_size);

	err = stream_a.write(buffer.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 690);
		return 690;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 691);
		return 691;
	}

	expected_dentry.fsize = buf_size;

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 692 << std::endl;
		return 692;
	}

	if(stream_a.get_pos() != buf_size)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << buf_size << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: " << 693 << std::endl;
		return 693;
	}

	err = stream_a.seek(-buf_size, std::ios_base::cur);
	if(err)
	{
		print_unexpected_err(err, 694);
		return 694;
	}

	err = stream_a.read(buffer2.get(), buf_size);
	if(err)
	{
		print_unexpected_err(err, 695);
		return 695;
	}

	if(std::memcmp(buffer.get(), buffer2.get(), buf_size))
	{
		std::cerr << "Diff in data!!!" << std::endl;
		print_buffers(buffer.get(), buffer2.get(), buf_size);
		std::cerr << "Exit: " << 696 << std::endl;
		return 696;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 697);
		return 697;
	}
	/*----------------------------End of write file---------------------------*/

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 371);
		return 371;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 372" << std::endl;
		return 372;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int trunc_open_tests()
{
	constexpr char EMU_FS[] = "trunc_open.img";

	u16 err, expected_err, fsck_status;

	std::string d_path, f_path;
	std::unique_ptr<u8[]> buffer;

	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 341" << std::endl;
		return 341;
	}
	/*----------------------------End of data setup---------------------------*/
	d_path = EXPECTED_ROOT_DIR[0].fname;
	expected_dentry = EXPECTED_DIR0[0];
	f_path = d_path + "/" + expected_dentry.fname;

	err = emu_fs->fopen(f_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 342);
		return 342;
	}

	buffer = std::make_unique<u8[]>(CLUSTER_SIZE);
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);

	err = stream.seek(expected_dentry.fsize - 8);
	if(err)
	{
		print_unexpected_err(err, 343);
		return 343;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 344);
		return 344;
	}

	if(stream.get_pos() != expected_dentry.fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 345" << std::endl;
		return 345;
	}

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize + 1);
	if(err)
	{
		print_unexpected_err(err, 346);
		return 346;
	}

	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 347);
		return 347;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 348" << std::endl;
		return 348;
	}

	if(dentries[0].fsize != expected_dentry.fsize + 1)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.fsize + 1 << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 349" << std::endl;
		return 349;
	}

	err = stream.seek(expected_dentry.fsize + 1 - 8);
	if(err)
	{
		print_unexpected_err(err, 350);
		return 350;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 351);
		return 351;
	}

	if(stream.get_pos() != expected_dentry.fsize + 1)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.fsize + 1 << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 352" << std::endl;
		return 352;
	}

	err = emu_fs->ftruncate(f_path.c_str(), expected_dentry.fsize - EMU::FS::BLK_SIZE);
	if(err)
	{
		print_unexpected_err(err, 353);
		return 353;
	}

	dentries.clear();
	err = emu_fs->list(f_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 354);
		return 354;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: 355" << std::endl;
		return 355;
	}

	if(dentries[0].fsize != expected_dentry.fsize - EMU::FS::BLK_SIZE)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.fsize - EMU::FS::BLK_SIZE << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 356" << std::endl;
		return 356;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 357);
		return 357;
	}

	if(stream.get_pos() < dentries[0].fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: >=" << dentries[0].fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 358" << std::endl;
		return 358;
	}

	err = stream.seek(dentries[0].fsize - 8);
	if(err)
	{
		print_unexpected_err(err, 359);
		return 359;
	}

	err = stream.read(buffer.get(), 0x10);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 360);
		return 360;
	}

	if(stream.get_pos() != dentries[0].fsize)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << dentries[0].fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: 361" << std::endl;
		return 361;
	}

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 373);
		return 373;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 374" << std::endl;
		return 374;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int fsck_tests()
{
	constexpr char EMU_FS[] = "fsck_fs.img";

	u8 cluster_shift;
	u16 err, expected_err, fsck_status, expected_checksum, sum, cur;
	u32 FAT_blk_cnt;
	
	std::unique_ptr<u8[]> data;
	std::fstream stream;

	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 375" << std::endl;
		return 375;
	}
	/*----------------------------End of data setup---------------------------*/

	/*------------------------------Known good FS-----------------------------*/
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 376);
		return 376;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 377" << std::endl;
		return 377;
	}
	/*--------------------------End of known good FS--------------------------*/

	/*----------------------------Superblock checks---------------------------*/
	//Checksum
	emu_fs->stream.seekg(EMU::FS::BLK_SIZE - 2);
	emu_fs->stream.read((char*)&expected_checksum, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		expected_checksum = std::byteswap(expected_checksum);

	fsck_status = ~expected_checksum;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		fsck_status = std::byteswap(fsck_status);

	emu_fs->stream.seekp(EMU::FS::BLK_SIZE - 2);
	emu_fs->stream.write((char*)&fsck_status, 2);
	emu_fs->stream.flush();

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM;
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 378);
		return 378;
	}

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 379" << std::endl;
		return 379;
	}

	emu_fs->stream.seekg(0);
	data = std::make_unique<u8[]>(EMU::FS::BLK_SIZE);
	emu_fs->stream.read((char*)data.get(), EMU::FS::BLK_SIZE - 2);
	emu_fs->stream.read((char*)&expected_checksum, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		expected_checksum = std::byteswap(expected_checksum);

	//Recalculate checksum
	sum = 0;

	for(u16 i = 0; i < EMU::FS::BLK_SIZE - 2; i += 2)
	{
		std::memcpy(&cur, data.get() + i, 2);

		if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
			cur = std::byteswap(cur);

		sum += cur;
	}

	if(sum != expected_checksum)
	{
		std::cerr << "Checksum mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_checksum << std::endl;
		std::cerr << "Got: " << sum << std::endl;
		std::cerr << "Exit: 380" << std::endl;
		return 380;
	}

	//Cluster shift
	emu_fs->stream.seekp(0x28);
	emu_fs->stream.put(0xFF);
	emu_fs->stream.flush();

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::DISK_TOO_SMALL);
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 381);
		return 381;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_CLUSTER_SHIFT
		| (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_BLOCK_CNT;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 382" << std::endl;
		return 382;
	}

	emu_fs.reset();

	//Cluster count
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	stream.open(EMU_FS, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	std::memset(data.get(), 0xFF, 4);
	stream.seekp(0x24);
	stream.write((char*)data.get(), 4);
	stream.flush();

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::DISK_TOO_SMALL);
	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 383);
		return 383;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_CLUSTER_CNT
		| (u16)EMU::FS::FSCK_STATUS::BAD_FAT_BLK_CNT
		| (u16)EMU::FS::FSCK_STATUS::BAD_BLOCK_CNT;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 384" << std::endl;
		return 384;
	}

	//FAT block count
	stream.close();
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	stream.open(EMU_FS, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	stream.seekp(0x1C);
	stream.write((char*)data.get(), 4);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 385);
		return 385;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_FAT_BLK_CNT;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 386" << std::endl;
		return 386;
	}

	stream.seekg(0x1C);
	stream.read((char*)&FAT_blk_cnt, 4);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		FAT_blk_cnt = std::byteswap(FAT_blk_cnt);

	if(FAT_blk_cnt != EXPECTED_HEADER.FAT_blk_cnt)
	{
		std::cerr << "FAT block count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_HEADER.FAT_blk_cnt << std::endl;
		std::cerr << "Got: " << FAT_blk_cnt << std::endl;
		std::cerr << "Exit: 387" << std::endl;
		return 387;
	}

	//FAT block address
	std::memset(data.get(), 0, 4);
	stream.seekp(0x18);
	stream.write((char*)data.get(), 4);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 388);
		return 388;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_FAT_ADDR;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 389" << std::endl;
		return 389;
	}

	stream.seekg(0x18);
	stream.read((char*)&FAT_blk_cnt, 4);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		FAT_blk_cnt = std::byteswap(FAT_blk_cnt);

	if(FAT_blk_cnt != EXPECTED_HEADER.FAT_blk_addr)
	{
		std::cerr << "FAT block count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_HEADER.FAT_blk_addr << std::endl;
		std::cerr << "Got: " << FAT_blk_cnt << std::endl;
		std::cerr << "Exit: 390" << std::endl;
		return 390;
	}

	//Root dir address
	std::memset(data.get(), 0, 4);
	stream.seekp(0x08);
	stream.write((char*)data.get(), 4);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 391);
		return 391;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_ROOT_DIR
		| (u16)EMU::FS::FSCK_STATUS::FAT_COLLISION;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 392" << std::endl;
		return 392;
	}

	//File list address
	std::memset(data.get(), 0, 4);
	stream.seekp(0x10);
	stream.write((char*)data.get(), 4);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 393);
		return 393;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_ROOT_DIR
		| (u16)EMU::FS::FSCK_STATUS::BAD_FILE_LIST
		| (u16)EMU::FS::FSCK_STATUS::FAT_COLLISION
		| (u16)EMU::FS::FSCK_STATUS::FILE_LIST_COLLISION;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 394" << std::endl;
		return 394;
	}

	//Last addressable file list block
	stream.close();
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	stream.open(EMU_FS, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	stream.seekg(0x10);
	stream.read((char*)&FAT_blk_cnt, 4);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		FAT_blk_cnt = std::byteswap(FAT_blk_cnt);

	FAT_blk_cnt = 0x10001 - FAT_blk_cnt;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		FAT_blk_cnt = std::byteswap(FAT_blk_cnt);

	stream.seekp(0x10);
	stream.write((char*)&FAT_blk_cnt, 4);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 395);
		return 395;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM
		| (u16)EMU::FS::FSCK_STATUS::BAD_FILE_LIST
		| (u16)EMU::FS::FSCK_STATUS::DATA_COLLISION;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 396" << std::endl;
		return 396;
	}
	/*------------------------End of superblock checks------------------------*/

	/*-----------------------------Root dir checks----------------------------*/
	stream.close();
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	stream.open(EMU_FS, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
	
	//Dir content block double ref
	cur = 0xB;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0xE54);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 397);
		return 397;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_DIR;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 398" << std::endl;
		return 398;
	}

	stream.seekg(0xE54);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != 0xFFFF)
	{
		std::cerr << "Block address mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0xFFFF << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 399";
		return 399;
	}

	//Dupe dir names
	stream.seekg(0xE00);
	stream.read((char*)data.get(), 16);

	stream.seekp(0xE40);
	stream.write((char*)data.get(), 16);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 400);
		return 400;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_DIR;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 401" << std::endl;
		return 401;
	}

	stream.seekg(0xE40);
	stream.read((char*)data.get(), 16);
	data[16] = 0;

	if(std::strncmp((char*)data.get(), "Test dir 1    _2", 16))
	{
		std::cerr << "Dedup filename mismatch!!!" << std::endl;
		std::cerr << "Expected: Test dir 1    _2" << std::endl;
		std::cerr << "Got: " << (char*)data.get() << std::endl;
		std::cerr << "Exit: 402" << std::endl;
		return 402;
	}

	//Double dupe
	stream.seekg(0xE00);
	stream.read((char*)data.get(), 32);
	std::memset(data.get() + 0x12, 0xFF, 14);

	stream.seekp(0xE40);
	stream.write((char*)data.get(), 32);

	const std::streampos dupe_file_addr = 0xE80;

	stream.seekp(dupe_file_addr);
	stream.write((char*)data.get(), 32);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 403);
		return 403;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_DIR;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 404" << std::endl;
		return 404;
	}

	stream.seekg(0xE40);
	stream.read((char*)data.get(), 16);
	data[16] = 0;

	if(std::strncmp((char*)data.get(), "Test dir 1    _2", 16))
	{
		std::cerr << "Dedup filename mismatch!!!" << std::endl;
		std::cerr << "Expected: Test dir 1    _2" << std::endl;
		std::cerr << "Got: " << (char*)data.get() << std::endl;
		std::cerr << "Exit: 405" << std::endl;
		return 405;
	}

	stream.seekg(dupe_file_addr);
	stream.read((char*)data.get(), 16);
	data[16] = 0;

	if(std::strncmp((char*)data.get(), "Test dir 1    _3", 16))
	{
		std::cerr << "Dedup filename mismatch!!!" << std::endl;
		std::cerr << "Expected: Test dir 1    _3" << std::endl;
		std::cerr << "Got: " << (char*)data.get() << std::endl;
		std::cerr << "Exit: 406" << std::endl;
		return 406;
	}

	//Wrong next dir content block
	cur = EXPECTED_NEXT_DIR_CONTENT_BLOCK;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0xE54);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 407);
		return 407;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_NEXT_DIR_CONTENT_BLK;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 408" << std::endl;
		return 408;
	}

	stream.seekg(EMU::FS::BLK_SIZE);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EXPECTED_NEXT_DIR_CONTENT_BLOCK + 1)
	{
		std::cerr << "Next directory content block mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_NEXT_DIR_CONTENT_BLOCK + 1 << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 409" << std::endl;
		return 409;
	}
	/*-------------------------End of root dir checks-------------------------*/

	/*-------------------------------FAT checks-------------------------------*/
	//Bad cluster 0
	cur = 0xFFFF;

	stream.seekp(EXPECTED_HEADER.FAT_blk_addr * EMU::FS::BLK_SIZE);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 410);
		return 410;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::UNMARKED_RESERVED_CLUSTERS;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 411" << std::endl;
		return 411;
	}

	stream.seekg(EXPECTED_HEADER.FAT_blk_addr * EMU::FS::BLK_SIZE);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EMU::FS::FAT_ATTRS.RESERVED)
	{
		std::cerr << "Cluster 0 mismatch" << std::endl;
		std::cerr << "Expected: " << EMU::FS::FAT_ATTRS.RESERVED << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 412" << std::endl;
		return 412;
	}

	//Bad padding
	cur = 0xFFFF;

	stream.seekp(EXPECTED_HEADER.FAT_blk_addr * EMU::FS::BLK_SIZE
		+ (EMU::FS::FAT_ATTRS.DATA_MIN + EXPECTED_HEADER.cluster_cnt) * 2);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 413);
		return 413;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::UNMARKED_RESERVED_CLUSTERS;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 414" << std::endl;
		return 414;
	}

	stream.seekg(EXPECTED_HEADER.FAT_blk_addr * EMU::FS::BLK_SIZE
		+ (EMU::FS::FAT_ATTRS.DATA_MIN + EXPECTED_HEADER.cluster_cnt) * 2);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EMU::FS::FAT_ATTRS.RESERVED)
	{
		std::cerr << "Padding cluster mismatch" << std::endl;
		std::cerr << "Expected: " << EMU::FS::FAT_ATTRS.RESERVED << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 415" << std::endl;
		return 415;
	}
	/*-------------------------------FAT checks-------------------------------*/

	/*-------------------------------File checks------------------------------*/
	stream.close();
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	stream.open(EMU_FS, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	//Bad start cluster
	cur = 412;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0x1212);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 416);
		return 416;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 417" << std::endl;
		return 417;
	}

	stream.seekg(0x1212);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EMU::FS::FAT_ATTRS.END_OF_CHAIN)
	{
		std::cerr << "Start cluster mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EMU::FS::FAT_ATTRS.END_OF_CHAIN << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 418" << std::endl;
		return 418;
	}

	//Bad cluster count
	cur = EXPECTED_HEADER.cluster_cnt + 1;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0x1214);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 419);
		return 419;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 420" << std::endl;
		return 420;
	}

	stream.seekg(0x1214);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EXPECTED_HEADER.cluster_cnt)
	{
		std::cerr << "Cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_HEADER.cluster_cnt << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 421" << std::endl;
		return 421;
	}

	//Bad block count
	cur = CLUSTER_SIZE / EMU::FS::BLK_SIZE + 1;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0x1216);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 422);
		return 422;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 423" << std::endl;
		return 423;
	}

	stream.seekg(0x1216);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != CLUSTER_SIZE / EMU::FS::BLK_SIZE)
	{
		std::cerr << "Block count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << CLUSTER_SIZE / EMU::FS::BLK_SIZE << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 424" << std::endl;
		return 424;
	}

	//Bad byte count
	cur = EMU::FS::BLK_SIZE + 1;

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	stream.seekp(0x1218);
	stream.write((char*)&cur, 2);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 425);
		return 425;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 426" << std::endl;
		return 426;
	}

	stream.seekg(0x1218);
	stream.read((char*)&cur, 2);

	if constexpr(EMU::FS::ENDIANNESS != std::endian::native)
		cur = std::byteswap(cur);

	if(cur != EMU::FS::BLK_SIZE)
	{
		std::cerr << "Byte count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EMU::FS::BLK_SIZE << std::endl;
		std::cerr << "Got: " << cur << std::endl;
		std::cerr << "Exit: 427" << std::endl;
		return 427;
	}

	//Dupe bank number
	stream.seekp(0x1211);
	stream.put(1);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 428);
		return 428;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 429" << std::endl;
		return 429;
	}

	stream.seekg(0x1251);
	cluster_shift = stream.get();

	if(cluster_shift != 0)
	{
		std::cerr << "Bank number mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << (u16)cluster_shift << std::endl;
		std::cerr << "Exit: 430" << std::endl;
		return 430;
	}

	//Double dupe
	stream.seekp(0x1211);
	stream.put(1);

	stream.seekp(0x1231);
	stream.put(1);

	stream.seekp(0x1251);
	stream.put(1);
	stream.flush();

	fsck_status = 0;
	err = EMU::FS::fsck(EMU_FS, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 431);
		return 431;
	}

	expected_err = (u16)EMU::FS::FSCK_STATUS::BAD_FILE;

	if(fsck_status != expected_err)
	{
		std::cerr << "fsck status mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_fsck_status(expected_err);
		std::cerr << "Got: " << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 432" << std::endl;
		return 432;
	}

	stream.seekg(0x1231);
	cluster_shift = stream.get();

	if(cluster_shift != 0)
	{
		std::cerr << "Bank number mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << (u16)cluster_shift << std::endl;
		std::cerr << "Exit: 433" << std::endl;
		return 433;
	}

	stream.seekg(0x1251);
	cluster_shift = stream.get();

	if(cluster_shift != 15)
	{
		std::cerr << "Bank number mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 15 << std::endl;
		std::cerr << "Got: " << (u16)cluster_shift << std::endl;
		std::cerr << "Exit: 434" << std::endl;
		return 434;
	}
	/*---------------------------End of file checks---------------------------*/

	return 0;
}

static int rename_open_tests()
{
	constexpr char EMU_FS[] = "rename_open.img";

	u16 err, expected_err, fsck_status;

	std::string d_path, f_path;
	std::unique_ptr<u8[]> buffer;

	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	min_vfs::dentry_t expected_dentry;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream_a;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(EMU_FS))
		std::filesystem::remove_all(EMU_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, EMU_FS);

	try
	{
		emu_fs = std::make_unique<EMU::FS::filesystem_t>(EMU_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 551" << std::endl;
		return 551;
	}
	/*----------------------------End of data setup---------------------------*/

	/*-------------------------------Source open------------------------------*/
	d_path = EXPECTED_ROOT_DIR[1].fname;
	f_path = d_path + "/" + EXPECTED_DIR1[0].fname;

	err = emu_fs->fopen(f_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 228);
		return 228;
	}

	//rename file
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = emu_fs->rename(f_path.c_str(), (d_path + "/should_fail").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 229);
		return 229;
	}

	//move file
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = emu_fs->rename(f_path.c_str(), (EXPECTED_ROOT_DIR[0].fname + "/should_fail2").c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 230);
		return 230;
	}

	/*---------------------------End of source open---------------------------*/

	/*--------------------------------Dest open-------------------------------*/
	//rename file
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = emu_fs->rename((d_path + "/" + EXPECTED_DIR1[1].fname).c_str(),
						 f_path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 552);
		return 552;
	}

	//move file
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::ALREADY_OPEN);
	err = emu_fs->rename((EXPECTED_ROOT_DIR[0].fname + "/" + EXPECTED_DIR0[0].fname).c_str(),
						 f_path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 553);
		return 553;
	}
	/*----------------------------End of dest open----------------------------*/

	/*-------------------------------Rename dir-------------------------------*/
	err = emu_fs->rename(d_path.c_str(), "new_name");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 231);
		return 231;
	}
	/*----------------------------End of rename dir---------------------------*/

	/*Close by hand because relying on the destructor getting called by return
	 *causes a use-after-free because the filesystem gets destroyed first.*/
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 232);
		return 232;
	}

	if(emu_fs->open_files.size())
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: 0" << std::endl;
		std::cerr << "Got: " << emu_fs->open_files.size() << std::endl;
		std::cerr << "Exit: 233" << std::endl;
		return 233;
	}

	/*-------------------------------Safety fsck------------------------------*/
	emu_fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(emu_fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 554);
		return 554;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 555" << std::endl;
		return 555;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

int main()
{
	int err;
	uintmax_t free_space;

	std::unique_ptr<min_vfs::filesystem_t> base_fs_ptr;
	std::unique_ptr<EMU::FS::filesystem_t> emu_fs;
	
	std::cout << "fsck tests..." << std::endl;
	err = fsck_tests();
	if(err) return err;
	std::cout << "fsck tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Mount..." << std::endl;
	try
	{
		base_fs_ptr = std::make_unique<EMU::FS::filesystem_t>(TEST_IMG_NAME);
		(*base_fs_ptr) = EMU::FS::filesystem_t(TEST_IMG_NAME);
		emu_fs = std::move((std::unique_ptr<EMU::FS::filesystem_t>&)base_fs_ptr);
	}
	catch(min_vfs::FS_err e)
	{
		switch(e.err_code)
		{
			using enum min_vfs::ERR;

			case 0:
				break;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)NONEXISTANT_DISK):
				std::cerr << "Test file does not exist!?" << std::endl;
				return e.err_code;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)NOT_A_FILE):
				std::cerr << "Test file is not a file!!!" << std::endl;
				return e.err_code;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)DISK_TOO_SMALL):
				std::cerr << "Test file is too small!!!" << std::endl;
				return e.err_code;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)CANT_OPEN_DISK):
				std::cerr << "Can't open test file!!!" << std::endl;
				return e.err_code;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)IO_ERROR):
				std::cerr << "IO error during mount!!!" << std::endl;
				return e.err_code;

			case ret_val_setup(min_vfs::LIBRARY_ID, (u8)WRONG_FS):
				std::cerr << "Test file is somehow not an EMU FS!?" << std::endl;
				return e.err_code;

			default:
				std::cerr << "Unknown error (" << e.err_code << ")!!!" << std::endl;
				return e.err_code;
		}
	}

	if(!headercmp(emu_fs->header, EXPECTED_HEADER))
	{
		std::cerr << "Loaded header does not match expected data!!!" << std::endl;
		std::cerr << "Expected: " << header_to_string(EXPECTED_HEADER, 0) << std::endl;
		std::cerr << "Got: " << header_to_string(emu_fs->header, 0) << std::endl;
		std::cerr << "Exit: 1" << std::endl;
		return 1;
	}

	if(emu_fs->dir_content_block_map.size() != emu_fs->header.file_list_blk_cnt)
	{
		std::cerr << "Dir content block map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << emu_fs->header.file_list_blk_cnt << std::endl;
		std::cerr << "Got: " << emu_fs->dir_content_block_map.size() << std::endl;
		std::cerr << "Exit: 2" << std::endl;
		return 2;
	}

	err = 0;
	for(u32 i = 0; i < emu_fs->header.file_list_blk_cnt; i++)
	{
		if(emu_fs->dir_content_block_map[i] != EXPECTED_DIR_CONTENT_BLOCK_MAP[i])
		{
			std::cerr << "Dir content block map mismatch!!!" << std::endl;
			std::cerr << "idx: " << i << std::endl;
			std::cerr << "Expected: " << EXPECTED_DIR_CONTENT_BLOCK_MAP[i] << std::endl;
			std::cerr << "Got: " << emu_fs->dir_content_block_map[i] << std::endl;
			err = 1;
		}
	}

	if(err)
	{
		std::cerr << "Exit: 3" << std::endl;
		return 3;
	}

	err = 0;
	for(u32 i = 0; i < emu_fs->header.cluster_cnt + 1; i++)
	{
		if(emu_fs->FAT[i] != EXPECTED_FAT[i])
		{
			std::cerr << "FAT mismatch!!!" << std::endl;
			std::cerr << "idx: " << i << std::endl;
			std::cerr << "Expected: " << EXPECTED_FAT[i] << std::endl;
			std::cerr << "Got: " << emu_fs->FAT[i] << std::endl;
			err = 1;
		}
	}

	if(err)
	{
		print_buffer(emu_fs->FAT.get(), sizeof(u16) * emu_fs->header.cluster_cnt + 1);

		std::cerr << "Exit: 98" << std::endl;
		return 98;
	}

	if(emu_fs->free_clusters != EXPECTED_FREE_CLUSTER_COUNT)
	{
		std::cerr << "Free cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_FREE_CLUSTER_COUNT << std::endl;
		std::cerr << "Got: " << emu_fs->free_clusters << std::endl;
		std::cerr << "Exit: 128" << std::endl;
		return 128;
	}

	free_space = emu_fs->get_free_space();
	if(free_space != EXPECTED_FREE_SPACE)
	{
		std::cerr << "Free space mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_FREE_SPACE << std::endl;
		std::cerr << "Got: " << free_space << std::endl;
		std::cerr << "Exit: 129" << std::endl;
		return 129;
	}

	if(emu_fs->next_file_list_blk != EXPECTED_NEXT_DIR_CONTENT_BLOCK)
	{
		std::cerr << "Next dir content block mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_NEXT_DIR_CONTENT_BLOCK << std::endl;
		std::cerr << "Got: " << emu_fs->next_file_list_blk << std::endl;
		std::cerr << "Exit: " << 651 << std::endl;
		return 651;
	}
	std::cout << "Mount OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "List tests..." << std::endl;
	err = list_tests((*emu_fs));
	if(err) return err;
	std::cout << "List tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "mkdir tests..." << std::endl;
	err = mkdir_tests();
	if(err) return err;
	std::cout << "mkdir tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Truncate tests..." << std::endl;
	err = trunc_tests();
	if(err) return err;
	std::cout << "Truncate tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Remove tests..." << std::endl;
	err = remove_tests();
	if(err) return err;
	std::cout << "Remove tests OK!" << std::endl;
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

	std::cout << "Write tests..." << std::endl;
	err = write_tests();
	if(err) return err;
	std::cout << "Write tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Truncate open tests..." << std::endl;
	err = trunc_open_tests();
	if(err) return err;
	std::cout << "Truncate open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Rename open tests..." << std::endl;
	err = rename_open_tests();
	if(err) return err;
	std::cout << "Rename open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!!!" << std::endl;

	return 0;
}
