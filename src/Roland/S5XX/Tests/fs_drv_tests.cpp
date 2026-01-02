#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <memory>
#include <cstring>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "Roland/S5XX/S5XX_FS_drv.hpp"
#include "Utils/utils.hpp"
#include "fs_drv_test_data.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "helpers.hpp"

static int mount_tests()
{
	bool errored;
	u16 expected_err;
	std::unique_ptr<S5XX::FS::filesystem_t> fs;

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NONEXISTANT_DISK);
	errored = false;
	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>("nx_file.img");
	}
	catch(min_vfs::FS_err e)
	{
		if(e.err_code != expected_err)
		{
			print_expected_err(expected_err, e.err_code, 1);
			return 1;
		}

		errored = true;
	}

	if(!errored)
	{
		std::cerr << "THIS SHOULD BE UNREACHABLE!!!" << std::endl;
		std::cerr << "Exit: 2" << std::endl;
		return 2;
	}

	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>("S550_CD_SB.img");
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 3);
		return 3;
	}

	if(fs->FIRST_DIR_IDX != EXPECTED_CD_FIRST_DIR_IDX)
	{
		std::cerr << "Wrong FS variant!!!" << std::endl;
		std::cerr << "Expected FIRST_DIR_IDX: " <<
			EXPECTED_CD_FIRST_DIR_IDX << std::endl;
		std::cerr << "Got: " << fs->FIRST_DIR_IDX << std::endl;
		std::cerr << "Exit: 4" << std::endl;
		return 4;
	}

	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>("S550_HDD_SB.img");
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 5);
		return 5;
	}

	if(fs->FIRST_DIR_IDX != EXPECTED_HDD_FIRST_DIR_IDX)
	{
		std::cerr << "Wrong FS variant!!!" << std::endl;
		std::cerr << "Expected FIRST_DIR_IDX: " <<
			EXPECTED_HDD_FIRST_DIR_IDX << std::endl;
		std::cerr << "Got: " << fs->FIRST_DIR_IDX << std::endl;
		std::cerr << "Exit: 6" << std::endl;
		return 6;
	}

	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>(BASE_TEST_FS_FNAME);
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 7);
		return 7;
	}

	if(fs->FIRST_DIR_IDX != 7)
	{
		std::cerr << "Wrong FS variant!!!" << std::endl;
		std::cerr << "Expected FIRST_DIR_IDX: " << 8 << std::endl;
		std::cerr << "Got: " << fs->FIRST_DIR_IDX << std::endl;
		std::cerr << "Exit: 8" << std::endl;
		return 8;
	}

	return 0;
}

static int list_tests()
{
	u16 err, expected_err;
	min_vfs::dentry_t expected_dentry;

	std::unique_ptr<S5XX::FS::filesystem_t> fs;
	std::vector<min_vfs::dentry_t> dentries;

	/*--------------------------------FS setup--------------------------------*/
	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>(BASE_TEST_FS_FNAME);
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 9);
		return 9;
	}
	/*-----------------------------End of FS setup----------------------------*/

	/*----------------------------Common err tests----------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = fs->list("nx_dir/nx_dir/nx_file", dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 10);
		return 10;
	}
	/*-------------------------End of common err tests------------------------*/

	//List root contents
	err = fs->list("", dentries);
	if(err)
	{
		print_unexpected_err(err, 11);
		return 11;
	}

	if(dentries.size() != EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 12 << std::endl;
		return 12;
	}

	err = 0;
	for(size_t i = 0; i < dentries.size(); i++)
	{
		if(dentries[i] != EXPECTED_ROOT_DIR[i])
		{
			std::cerr << "Dentry mismatch!!!" << std::endl;
			std::cerr << "Expected:" << std::endl;
			std::cerr << EXPECTED_ROOT_DIR[i].to_string(1) << std::endl;
			std::cerr << "Got:" << std::endl;
			std::cerr << dentries[i].to_string(1) << std::endl;
			err = 1;
		}
	}

	if(err)
	{
		std::cerr << "Exit: " << 13 << std::endl;
		return 13;
	}

	//List root dir's dentry
	dentries.clear();
	err = fs->list("/", dentries, true);
	if(err)
	{
		print_unexpected_err(err, 80);
		return 80;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 81 << std::endl;
		return 81;
	}

	expected_dentry =
	{
		.fname = "/",
		.fsize = (S5XX::FS::MAX_ROOT_DENTRIES - EXPECTED_FIRST_DIR_IDX)
			* S5XX::FS::On_disk_sizes::DIR_ENTRY,
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
		std::cerr << "Exit: " << 82 << std::endl;
		return 82;
	}

	//List in root
	dentries.clear();
	err = fs->list(EXPECTED_ROOT_DIR[0].fname.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 14);
		return 14;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 15 << std::endl;
		return 15;
	}

	if(dentries[0] != EXPECTED_ROOT_DIR[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << EXPECTED_ROOT_DIR[0].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 16 << std::endl;
		return 16;
	}

	//List dir
	dentries.clear();
	err = fs->list(EXPECTED_ROOT_DIR[1].fname.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 17);
		return 17;
	}

	if(dentries.size() != EXPECTED_SOUND_DIR.size())
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_SOUND_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 18 << std::endl;
		return 18;
	}

	err = 0;
	for(size_t i = 0; i < dentries.size(); i++)
	{
		if(dentries[i] != EXPECTED_SOUND_DIR[i])
		{
			std::cerr << "Dentry mismatch!!!" << std::endl;
			std::cerr << "Expected:" << std::endl;
			std::cerr << EXPECTED_SOUND_DIR[i].to_string(1) << std::endl;
			std::cerr << "Got:" << std::endl;
			std::cerr << dentries[i].to_string(1) << std::endl;
			err = 1;
		}
	}

	if(err)
	{
		std::cerr << "Exit: " << 19 << std::endl;
		return 19;
	}

	//List dir's dentry
	dentries.clear();
	err = fs->list(EXPECTED_ROOT_DIR[1].fname.c_str(), dentries, true);
	if(err)
	{
		print_unexpected_err(err, 83);
		return 83;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Mismatched dentry count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 84 << std::endl;
		return 84;
	}

	expected_dentry = EXPECTED_ROOT_DIR[1];
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 85 << std::endl;
		return 85;
	}

	//List file
	dentries.clear();
	err = fs->list((EXPECTED_ROOT_DIR[1].fname + "/" + EXPECTED_SOUND_DIR[17].fname).c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 20);
		return 20;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 21 << std::endl;
		return 21;
	}

	if(dentries[0] != EXPECTED_SOUND_DIR[17])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << EXPECTED_SOUND_DIR[17].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 22 << std::endl;
		return 22;
	}

	//List file with '/' in name
	dentries.clear();
	err = fs->list((EXPECTED_ROOT_DIR[1].fname + "/" + EXPECTED_SOUND_DIR[7].fname).c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 23);
		return 23;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dir count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 24 << std::endl;
		return 24;
	}

	if(dentries[0] != EXPECTED_SOUND_DIR[7])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << EXPECTED_SOUND_DIR[7].to_string(1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 25 << std::endl;
		return 25;
	}

	return 0;
}

static int fopen_tests()
{
	constexpr char TEST_FS_NAME[] = "fopen_tests.img";
	u16 err, expected_err;

	std::string fpath;

	min_vfs::stream_t stream_a, stream_b;
	std::unique_ptr<S5XX::FS::filesystem_t> fs;
	S5XX::FS::filesystem_t::file_map_iterator_t fmap_it;
	S5XX::FS::File_entry_t fentry;

	/*--------------------------------FS setup--------------------------------*/
	if(std::filesystem::exists(TEST_FS_NAME))
		std::filesystem::remove_all(TEST_FS_NAME);

	std::filesystem::copy_file(BASE_TEST_FS_FNAME, TEST_FS_NAME);

	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>(TEST_FS_NAME);
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 26);
		return 26;
	}
	/*-----------------------------End of FS setup----------------------------*/

	/*----------------------------Common err tests----------------------------*/
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH);
	err = fs->fopen("", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 27);
		return 27;
	}

	err = fs->fopen("nx_dir/nx_dir/nx_file", stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 28);
		return 28;
	}
	/*-------------------------End of common err tests------------------------*/

	/*------------------------------fopen in root-----------------------------*/
	fpath = "InstrumentGroup";
	err = fs->fopen(fpath.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 29);
		return 29;
	}

	if(fs->open_files.size() != 1)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 30 << std::endl;
		return 30;
	}

	fmap_it = fs->open_files.begin();

	if(fmap_it->first != fpath)
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Expected: " << fpath << std::endl;
		std::cerr << "Got: " << fmap_it->first << std::endl;
		std::cerr << "Exit: " << 31 << std::endl;
		return 31;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 32 << std::endl;
		return 32;
	}

	fentry =
	{
		.name = "InstrumentGroup",
		.block_addr = 240,
		.block_cnt = 15,
		.type = S5XX::FS::File_type_e::instrument_group,
		.ins_grp_idx = 1
	};

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr <<file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 33 << std::endl;
		return 33;
	}

	//Same file again but with absolute path
	err = fs->fopen((std::string("/") + fpath).c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 34);
		return 34;
	}

	if(fs->open_files.size() != 1)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 35 << std::endl;
		return 35;
	}

	fmap_it = fs->open_files.begin();

	if(fmap_it->first != fpath)
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Expected: " << fpath << std::endl;
		std::cerr << "Got: " << fmap_it->first << std::endl;
		std::cerr << "Exit: " << 36 << std::endl;
		return 36;
	}

	if(fmap_it->second.first != 2)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 37 << std::endl;
		return 37;
	}

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr <<file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 38 << std::endl;
		return 38;
	}
	/*--------------------------End of fopen in root--------------------------*/

	/*------------------------------fopen in dir------------------------------*/
	fpath = "SoundDirectory /HAMMOND                                         ";
	err = fs->fopen(fpath.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 39);
		return 39;
	}

	if(fs->open_files.size() != 3)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 3 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 40 << std::endl;
		return 40;
	}

	fmap_it = fs->open_files.find("SoundDirectory ");

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open dir path!!!" << std::endl;
		std::cerr << "Exit: " << 41 << std::endl;
		return 41;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Bad dir refcount!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 42 << std::endl;
		return 42;
	}

	fmap_it = fs->open_files.find(fpath);

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Exit: " << 43 << std::endl;
		return 43;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 44 << std::endl;
		return 44;
	}

	fentry =
	{
		.name = "HAMMOND                                         ",
		.block_addr = 0x37A0,
		.block_cnt = 0x6D2,
		.type = S5XX::FS::File_type_e::area,
		.ins_grp_idx = 0
	};

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr <<file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 45 << std::endl;
		return 45;
	}

	//Same file again but with absolute path
	err = fs->fopen((std::string("/") + fpath).c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 46);
		return 46;
	}

	if(fs->open_files.size() != 2)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 47 << std::endl;
		return 47;
	}

	fmap_it = fs->open_files.find("SoundDirectory ");

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open dir path!!!" << std::endl;
		std::cerr << "Exit: " << 48 << std::endl;
		return 48;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Bad dir refcount!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 49 << std::endl;
		return 49;
	}

	fmap_it = fs->open_files.find(fpath);

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Exit: " << 50 << std::endl;
		return 50;
	}

	if(fmap_it->second.first != 2)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 51 << std::endl;
		return 51;
	}

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr <<file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 52 << std::endl;
		return 52;
	}

	//open second file in dir
	fpath = "SoundDirectory /CHOIR 1                                         ";
	err = fs->fopen((std::string("/") + fpath).c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 53);
		return 53;
	}

	if(fs->open_files.size() != 3)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 3 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 54 << std::endl;
		return 54;
	}

	fmap_it = fs->open_files.find("SoundDirectory ");

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open dir path!!!" << std::endl;
		std::cerr << "Exit: " << 55 << std::endl;
		return 55;
	}

	if(fmap_it->second.first != 2)
	{
		std::cerr << "Bad dir refcount!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 56 << std::endl;
		return 56;
	}

	fmap_it = fs->open_files.find(fpath);

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Exit: " << 57 << std::endl;
		return 57;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 58 << std::endl;
		return 58;
	}

	fentry =
	{
		.name = "CHOIR 1                                         ",
		.block_addr = 0x4548,
		.block_cnt = 0x6D2,
		.type = S5XX::FS::File_type_e::area,
		.ins_grp_idx = 0
	};

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr <<file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 59 << std::endl;
		return 59;
	}

	//Open file with '/' in name
	fpath = EXPECTED_ROOT_DIR[1].fname + "/" + EXPECTED_SOUND_DIR[12].fname;
	err = fs->fopen((std::string("/") + fpath).c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 60);
		return 60;
	}

	if(fs->open_files.size() != 3)
	{
		std::cerr << "Open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 3 << std::endl;
		std::cerr << "Got: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 61 << std::endl;
		return 61;
	}

	fmap_it = fs->open_files.find("SoundDirectory ");

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open dir path!!!" << std::endl;
		std::cerr << "Exit: " << 62 << std::endl;
		return 62;
	}

	if(fmap_it->second.first != 2)
	{
		std::cerr << "Bad dir refcount!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 63 << std::endl;
		return 63;
	}

	fmap_it = fs->open_files.find(fpath);

	if(fmap_it == fs->open_files.end())
	{
		std::cerr << "Bad open file path!!!" << std::endl;
		std::cerr << "Exit: " << 64 << std::endl;
		return 64;
	}

	if(fmap_it->second.first != 1)
	{
		std::cerr << "Refcount mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second.first << std::endl;
		std::cerr << "Exit: " << 65 << std::endl;
		return 65;
	}

	fentry =
	{
		.name = "Club E.PianoSplits W\\Bs & WaveSynth #1          ",
		.block_addr = 0x52F0,
		.block_cnt = 0x6D2,
		.type = S5XX::FS::File_type_e::area,
		.ins_grp_idx = 0
	};

	if(!file_entrycmp(fmap_it->second.second.file_entry, fentry))
	{
		std::cerr << "Internal file metadata mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		std::cerr << file_entry_to_string(fentry, 1) << std::endl;
		std::cerr << "Got:" << std::endl;
		std::cerr << file_entry_to_string(fmap_it->second.second.file_entry, 1) << std::endl;
		std::cerr << "Exit: " << 66 << std::endl;
		return 66;
	}
	/*---------------------------End of fopen in dir--------------------------*/

	stream_a.close();
	stream_b.close();

	if(fs->open_files.size())
	{
		std::cerr << "Failed to close all files!!!" << std::endl;
		std::cerr << "Open file count: " << fs->open_files.size() << std::endl;
		std::cerr << "Exit: " << 67 << std::endl;
		return 67;
	}

	return 0;
}

static int read_tests()
{
	constexpr u16 BUF_SIZE = S5XX::FS::BLOCK_SIZE * 2;

	u16 err, expected_err;

	std::unique_ptr<u8[]> s5xx_buf, host_buf;
	std::fstream host_stream;
	std::streampos base_pos;

	min_vfs::stream_t stream_a;
	std::unique_ptr<S5XX::FS::filesystem_t> fs;

	/*--------------------------------FS setup--------------------------------*/
	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>(BASE_TEST_FS_FNAME);
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 68);
		return 68;
	}

	host_stream.open("test_file.s5xx", std::ios_base::binary | std::ios_base::in);
	if(!host_stream.is_open() || !host_stream.good())
	{
		std::cerr << "Failed to open host file!!!" << std::endl;
		std::cerr << "Exit: " << 69 << std::endl;
	}
	/*-----------------------------End of FS setup----------------------------*/

	err = fs->fopen((EXPECTED_ROOT_DIR[1].fname + "/" + EXPECTED_SOUND_DIR[18].fname).c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 70);
		return 70;
	}

	s5xx_buf = std::make_unique<u8[]>(BUF_SIZE);
	host_buf = std::make_unique<u8[]>(BUF_SIZE);

	err = stream_a.read(s5xx_buf.get(), BUF_SIZE);
	if(err)
	{
		print_unexpected_err(err, 71);
		return 71;
	}

	host_stream.read((char*)host_buf.get(), BUF_SIZE);

	if(stream_a.get_pos() != BUF_SIZE)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << BUF_SIZE << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: " << 72 << std::endl;
		return 72;
	}

	if(std::memcmp(s5xx_buf.get(), host_buf.get(), BUF_SIZE))
	{
		std::cerr << "File read data mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		print_buffer(host_buf.get(), BUF_SIZE);
		std::cerr << "Got:" << std::endl;
		print_buffer(s5xx_buf.get(), BUF_SIZE);
		std::cerr << "Exit: " << 73 << std::endl;
		return 73;
	}

	//Read from 0x10 before the audio section (because header)
	base_pos = 0x2400;

	host_stream.seekg(base_pos);
	stream_a.seek(base_pos);

	err = stream_a.read(s5xx_buf.get(), BUF_SIZE);
	if(err)
	{
		print_unexpected_err(err, 74);
		return 74;
	}

	host_stream.read((char*)host_buf.get(), BUF_SIZE);

	if(stream_a.get_pos() != base_pos + (std::streampos)BUF_SIZE)
	{
		std::cerr << "Stream pos mismatch!!!" << std::endl;
		std::cerr << "Expected: " << BUF_SIZE << std::endl;
		std::cerr << "Got: " << stream_a.get_pos() << std::endl;
		std::cerr << "Exit: " << 75 << std::endl;
		return 75;
	}

	if(std::memcmp(s5xx_buf.get(), host_buf.get(), BUF_SIZE))
	{
		std::cerr << "File read data mismatch!!!" << std::endl;
		std::cerr << "Expected:" << std::endl;
		print_buffer(host_buf.get(), BUF_SIZE);
		std::cerr << "Got:" << std::endl;
		print_buffer(s5xx_buf.get(), BUF_SIZE);
		std::cerr << "Exit: " << 76 << std::endl;
		return 76;
	}

	//Check EOF works correctly
	err = stream_a.seek(EXPECTED_SOUND_DIR[18].fsize);
	if(err)
	{
		print_unexpected_err(err, 77);
		return 77;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::END_OF_FILE);
	err = stream_a.read(s5xx_buf.get(), BUF_SIZE);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 78);
		return 78;
	}

	//Manually close stream
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 79);
		return 79;
	}

	return 0;
}

int main(void)
{
	u32 err;

	std::cout << "Mount tests..." << std::endl;
	err = mount_tests();
	if(err) return err;
	std::cout << "Mount OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "List tests..." << std::endl;
	err = list_tests();
	if(err) return err;
	std::cout << "List OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "fopen/fclose tests..." << std::endl;
	err = fopen_tests();
	if(err) return err;
	std::cout << "fopen/fclose OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Read tests..." << std::endl;
	err = read_tests();
	if(err) return err;
	std::cout << "Read OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;

	return 0;
}
