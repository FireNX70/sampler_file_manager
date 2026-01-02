#include <cstdint>
#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <string>
#include <memory>
#include <cstring>

#include "Host_FS/host_drv.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"
#include "Roland/S7XX/S7XX_types.hpp"
#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "Utils/testing_helpers.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "min_vfs/min_vfs.hpp"
#include "helpers.hpp"
#include "test_data.hpp"
#include "E-MU/Tests/fs_test_data.hpp"
#include "Roland/S7XX/Tests/Test_data.hpp"

constexpr char BASE_TEST_FS_PATH[] = "base_test_fs.img";
constexpr char TEST_S7XX_FS_PATH[] = "test_S7XX_fs.img";
constexpr char EMPTY_FILE_PATH[] = "empty_file";
constexpr char TEST_HOST_DIR[] = "test_dir";
constexpr char TEST_HOST_SYMLINK_TO_DIR[] = "test_dir_symlink";
constexpr char TEST_FILE_IN_DIR_NAME[] = "test_file";
constexpr char EMU_FS_PATH[] = "emu3_test.img"; //Temp copies for tests
constexpr char S7XX_FS_PATH[] = "s7xx_test.img"; //Temp copies for tests
constexpr char S7XX_FS_SYMLINK_PATH[] = "s7xx_symlink"; //Temp symlink for tests
constexpr char TEST_FOPEN_CREATE[] = "fopen_create";
constexpr char MOVE_NESTED_TEST_DIR[] = "move_nested_test_dir";
constexpr char RENAME_TEST_FILENAME[] = "rename_test";
constexpr char RENAME_OPEN_TEST_FILENAME[] = "rename_open_test";
constexpr char RENAME_TEST_DIR[] = "rename_test_dir";

constexpr u16 BUFFER_SIZE = 512;

namespace EMU_TEST = EMU::FS::testing;
namespace S7XX_TEST = S7XX::FS::testing;

static int mount_tests()
{
	u16 err, expected_err;
	std::fstream fstr;
	min_vfs::mount_stats_t expected_mount;

	std::vector<min_vfs::mount_stats_t> mounts;
	std::vector<min_vfs::map_stats_t> map_stats;

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NONEXISTANT_DISK);
	err = min_vfs::mount("nx_file");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 1);
		return 1;
	}

	//Unsupported FS
	fstr.open(EMPTY_FILE_PATH, std::ios_base::binary | std::ios_base::out
		| std::ios_base::trunc);
	fstr.put(0);
	fstr.close();
	std::filesystem::resize_file(EMPTY_FILE_PATH, 0);


	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::WRONG_FS);
	err = min_vfs::mount(EMPTY_FILE_PATH);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 2);
		return 2;
	}

	//Actually try and mount something
	err = min_vfs::mount(BASE_TEST_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 3);
		return 3;
	}

	min_vfs::lsmount(mounts);

	if(mounts.size() != 1)
	{
		std::cerr << "Mount count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 4;
		return 4;
	}

	expected_mount =
	{
		.path = std::filesystem::absolute(BASE_TEST_FS_PATH),
		.type = "EMU3"
	};

	if(!std::filesystem::equivalent(mounts[0].path, expected_mount.path)
		|| mounts[0].type != expected_mount.type)
	{
		std::cerr << "FS stats mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\tpath: " << expected_mount.path << std::endl;
		std::cerr << "\ttype: " << expected_mount.type << std::endl;
		std::cerr << "Got:\n\tpath: " << mounts[0].path << std::endl;
		std::cerr << "\ttype: " << mounts[0].type << std::endl;
		std::cerr << "Exit: " << 5 << std::endl;
		return 5;
	}

	//And another one
	err = min_vfs::mount(TEST_S7XX_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 6);
		return 6;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);

	if(mounts.size() != 2)
	{
		std::cerr << "Mount count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 7;
		return 7;
	}

	expected_mount =
	{
		.path = std::filesystem::canonical(TEST_S7XX_FS_PATH),
		.type = "S7XX"
	};

	if(mounts[1].path != expected_mount.path
		|| mounts[1].type != expected_mount.type)
	{
		std::cerr << "FS stats mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\tpath: " << expected_mount.path << std::endl;
		std::cerr << "\ttype: " << expected_mount.type << std::endl;
		std::cerr << "Got:\n\tpath: " << mounts[1].path << std::endl;
		std::cerr << "\ttype: " << mounts[1].type << std::endl;
		std::cerr << "Exit: " << 8 << std::endl;
		return 8;
	}

	//Try to mount the same one again
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::ALREADY_OPEN);
	err = min_vfs::mount(TEST_S7XX_FS_PATH);
	if(err != expected_err)
	{
		std::cerr << "Looking for: " << TEST_S7XX_FS_PATH << std::endl;
		std::cerr << "Mounts:" << std::endl;

		min_vfs::lsmap(map_stats);

		for(const min_vfs::map_stats_t &mount: map_stats)
		{
			std::cerr << "\tKey: " << mount.key << std::endl;
			std::cerr << "\tPath: " << mount.mount.path << std::endl
				<< std::endl;
		}

		print_expected_err(expected_err, err, 88);
		return 88;
	}

	return 0;
}

static int list_tests()
{
	u16 err, expected_err;
	std::fstream fstr;

	std::vector<min_vfs::dentry_t> dentries, expected_dentries;

	//Data setup
	if(std::filesystem::exists(TEST_HOST_DIR))
		std::filesystem::remove_all(TEST_HOST_DIR);

	std::filesystem::create_directory(TEST_HOST_DIR);

	//Check host dir (just use current workdir)
	err = Host::FS::filesystem_t::list_static(
		std::filesystem::current_path().c_str(), expected_dentries, false);
	if(err)
	{
		print_unexpected_err(err, 9);
		return 9;
	}

	err = min_vfs::list(std::filesystem::current_path().c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 10);
		return 10;
	}

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Workdir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 11 << std::endl;
		return 11;
	}

	err = compare_dirs(dentries, expected_dentries);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 12 << std::endl;
		return 12;
	}

	//Check host file
	expected_dentries.clear();
	err = Host::FS::filesystem_t::list_static(EMPTY_FILE_PATH,
											  expected_dentries, false);
	if(err)
	{
		print_unexpected_err(err, 13);
		return 13;
	}

	dentries.clear();
	err = min_vfs::list(EMPTY_FILE_PATH, dentries);
	if(err)
	{
		print_unexpected_err(err, 14);
		return 14;
	}

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 15 << std::endl;
		return 15;
	}

	err = compare_dirs(dentries, expected_dentries);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 16 << std::endl;
		return 16;
	}

	expected_dentries.clear();

	//Check empty host dir
	if(std::filesystem::exists(TEST_HOST_DIR))
		std::filesystem::remove_all(TEST_HOST_DIR);

	std::filesystem::create_directory(TEST_HOST_DIR);

	dentries.clear();
	err = min_vfs::list(TEST_HOST_DIR, dentries);
	if(err)
	{
		print_unexpected_err(err, 17);
		return 17;
	}

	if(dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 18 << std::endl;
		return 18;
	}

	//Check host symlink pointing host dir
	if(std::filesystem::exists(TEST_HOST_SYMLINK_TO_DIR))
		std::filesystem::remove_all(TEST_HOST_SYMLINK_TO_DIR);

	std::filesystem::create_symlink(TEST_HOST_DIR, TEST_HOST_SYMLINK_TO_DIR);

	dentries.clear();
	err = min_vfs::list(TEST_HOST_SYMLINK_TO_DIR, dentries);
	if(err)
	{
		print_unexpected_err(err, 19);
		return 19;
	}

	/*Because we always follow symlinks, this should list the directory's
	content.*/
	if(dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 20 << std::endl;
		return 20;
	}

	//Check host file within dir pointed to by symlink
	const std::string test_file_path = std::string(TEST_HOST_DIR) + "/"
		+ TEST_FILE_IN_DIR_NAME;

	if(std::filesystem::exists(test_file_path))
		std::filesystem::remove_all(test_file_path);

	fstr.open(test_file_path, std::ios_base::binary | std::ios_base::out
		| std::ios_base::trunc);
	fstr.put(0);
	fstr.close();
	std::filesystem::resize_file(test_file_path, 0);

	err = Host::FS::filesystem_t::list_static(test_file_path, expected_dentries,
											  false);
	if(err)
	{
		print_unexpected_err(err, 21);
		return 21;
	}

	if(expected_dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 22 << std::endl;
		return 22;
	}

	dentries.clear();
	err = min_vfs::list(std::string(TEST_HOST_SYMLINK_TO_DIR) + "/"
		+ TEST_FILE_IN_DIR_NAME, dentries);
	if(err)
	{
		print_unexpected_err(err, 23);
		return 23;
	}

	/*Because we always follow symlinks, this should list the directory's
	 *content.*/
	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 24 << std::endl;
		return 24;
	}

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 25;
		return 25;
	}

	//Check mounted FS' root dir
	dentries.clear();
	err = min_vfs::list(BASE_TEST_FS_PATH, dentries);
	if(err)
	{
		print_unexpected_err(err, 26);
		return 26;
	}

	if(dentries.size() != EMU_TEST::EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "EMU test FS root dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EMU_TEST::EXPECTED_ROOT_DIR.size()
			<< std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 27 << std::endl;
		return 27;
	}

	err = compare_dirs(dentries, EMU_TEST::EXPECTED_ROOT_DIR);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 28 << std::endl;
		return 28;
	}

	//Check mounted FS' non-root dir
	dentries.clear();
	err = min_vfs::list((std::string(BASE_TEST_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname).c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 29);
		return 29;
	}

	if(dentries.size() != EMU_TEST::EXPECTED_DIR0.size())
	{
		std::cerr << "EMU test FS first dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << EMU_TEST::EXPECTED_ROOT_DIR.size()
			<< std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 30 << std::endl;
		return 30;
	}

	err = compare_dirs(dentries, EMU_TEST::EXPECTED_DIR0);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 31 << std::endl;
		return 31;
	}

	//Check other mounted FS' root dir
	dentries.clear();
	err = min_vfs::list(TEST_S7XX_FS_PATH, dentries);
	if(err)
	{
		print_unexpected_err(err, 32);
		return 32;
	}

	if(dentries.size() != S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_ROOT_DIR.size())
	{
		std::cerr << "S7XX test FS root dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: "
			<< S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_ROOT_DIR.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 33 << std::endl;
		return 33;
	}

	err = compare_dirs(dentries, S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_ROOT_DIR);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 34 << std::endl;
		return 34;
	}

	//Check other mounted FS' non-root dir
	dentries.clear();
	err = min_vfs::list((std::string(TEST_S7XX_FS_PATH) + "/"
		+ S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_ROOT_DIR[0].fname).c_str(),
						dentries);
	if(err)
	{
		print_unexpected_err(err, 35);
		return 35;
	}

	if(dentries.size() != S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_VOLUME_DIR.size())
	{
		std::cerr << "S7XX test FS first dir size mismatch!!!" << std::endl;
		std::cerr << "Expected: "
			<< S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_VOLUME_DIR.size()
			<< std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 36 << std::endl;
		return 36;
	}

	err = compare_dirs(dentries,
					   S7XX_TEST::NATIVE_FS_DIRS::EXPECTED_VOLUME_DIR);
	if(!err) //True if matched
	{
		std::cerr << "Exit: " << 37 << std::endl;
		return 37;
	}

	return 0;
}

int mkdir_tests()
{
	constexpr char MKDIR_TEST_DIRNAME[] = "mkdir_test";

	u16 err, expected_err;
	std::string path;

	std::vector<min_vfs::dentry_t> dentries, expected_dentries;

	//Data setup
	if(std::filesystem::exists(MKDIR_TEST_DIRNAME))
		std::filesystem::remove_all(MKDIR_TEST_DIRNAME);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//Host FS
	err = min_vfs::mkdir(MKDIR_TEST_DIRNAME);
	if(err)
	{
		print_unexpected_err(err, 38);
		return 38;
	}

	err = min_vfs::list(MKDIR_TEST_DIRNAME, dentries, true);
	if(err)
	{
		print_unexpected_err(err, 39);
		return 39;
	}

	expected_dentries.emplace_back(MKDIR_TEST_DIRNAME, 0, 0, 0, 0,
								   min_vfs::ftype_t::dir);

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 40 << std::endl;
		return 40;
	}

	expected_dentries[0].mtime = dentries[0].mtime;

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\t" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n\t" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 41 << std::endl;
		return 41;
	}

	//EMU FS
	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 42);
		return 42;
	}

	path = std::string(EMU_FS_PATH) + "/" + MKDIR_TEST_DIRNAME;
	err = min_vfs::mkdir(path.c_str());
	if(err)
	{
		print_unexpected_err(err, 43);
		return 43;
	}

	dentries.clear();
	err = min_vfs::list(path.c_str(), dentries, true);
	if(err)
	{
		print_unexpected_err(err, 44);
		return 44;
	}

	expected_dentries.clear();
	expected_dentries.emplace_back(MKDIR_TEST_DIRNAME, 0, 0, 0, 0,
								   min_vfs::ftype_t::dir);

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 45 << std::endl;
		return 45;
	}

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\t" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n\t" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 46 << std::endl;
		return 46;
	}

	//S7XX FS
	path = std::string(TEST_S7XX_FS_PATH) + "/" + MKDIR_TEST_DIRNAME;
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::UNSUPPORTED_OPERATION);
	err = min_vfs::mkdir(path.c_str());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 47);
		return 47;
	}

	return 0;
}

int umount_tests()
{
	u16 err, expected_err;

	std::vector<min_vfs::mount_stats_t> mounts, expected_mounts;

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 48);
		return 48;
	}

	//Umount nonexistant
	min_vfs::lsmount(expected_mounts);
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NONEXISTANT_DISK);
	err = min_vfs::umount("nx_fs");
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 49);
		return 49;
	}

	min_vfs::lsmount(mounts);

	if(mounts.size() != expected_mounts.size())
	{
		std::cerr << "Mounted FS count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_mounts.size() << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 50 << std::endl;
		return 50;
	}

	//Umount mounted
	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 51);
		return 51;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);

	if(mounts.size() != expected_mounts.size() - 1)
	{
		std::cerr << "Mounted FS count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_mounts.size() - 1 << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 52 << std::endl;
		return 52;
	}

	//Umount through symlink
	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 53);
		return 53;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);

	if(mounts.size() != expected_mounts.size() - 2)
	{
		std::cerr << "Mounted FS count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_mounts.size() - 2 << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 54 << std::endl;
		return 54;
	}

	return 0;
}

int trunc_tests()
{
	constexpr char FILENAME[] = "test_file";

	u16 err, expected_err;
	std::string path;

	std::vector<min_vfs::dentry_t> dentries, expected_dentries;

	//Data setup
	if(std::filesystem::exists(FILENAME))
		std::filesystem::remove_all(FILENAME);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
			std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);

	//Host FS
	err = min_vfs::ftruncate(FILENAME, 0);
	if(err)
	{
		print_unexpected_err(err, 55);
		return 55;
	}

	err = min_vfs::list(FILENAME, dentries, true);
	if(err)
	{
		print_unexpected_err(err, 56);
		return 56;
	}

	expected_dentries.emplace_back(FILENAME, 0, 0, 0, 0,
								   min_vfs::ftype_t::file);

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 57 << std::endl;
		return 57;
	}

	expected_dentries[0].mtime = dentries[0].mtime;

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\t" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n\t" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 58 << std::endl;
		return 58;
	}

	//EMU FS
	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 59);
		return 59;
	}

	path = std::string(EMU_FS_PATH) + "/" + EMU_TEST::EXPECTED_ROOT_DIR[0].fname
		+ "/" + FILENAME;
	err = min_vfs::ftruncate(path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 60);
		return 60;
	}

	dentries.clear();
	err = min_vfs::list(path.c_str(), dentries, true);
	if(err)
	{
		print_unexpected_err(err, 61);
		return 61;
	}

	expected_dentries.clear();
	expected_dentries.emplace_back(std::string("0-") + FILENAME, 0, 0, 0, 0,
								   min_vfs::ftype_t::file);

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 62 << std::endl;
		return 62;
	}

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\t" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n\t" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 63 << std::endl;
		return 63;
	}

	//S7XX FS
	err = min_vfs::mount(S7XX_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 64);
		return 64;
	}

	path = std::string(S7XX_FS_SYMLINK_PATH) + "/Volumes/" + FILENAME;
	err = min_vfs::ftruncate(path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 65);
		return 65;
	}

	dentries.clear();
	err = min_vfs::list(path.c_str(), dentries, true);
	if(err)
	{
		print_unexpected_err(err, 66);
		return 66;
	}

	expected_dentries.clear();
	expected_dentries.emplace_back(std::string("1-") + FILENAME
		+ std::string("       "),
							S7XX::FS::On_disk_sizes::VOLUME_PARAMS_ENTRY,
							0, 0, 0, min_vfs::ftype_t::file);

	if(dentries.size() != expected_dentries.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentries.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 67 << std::endl;
		return 67;
	}

	if(dentries[0] != expected_dentries[0])
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n\t" << expected_dentries[0].to_string(1)
			<< std::endl;
		std::cerr << "Got:\n\t" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 68 << std::endl;
		return 68;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 69);
		return 69;
	}

	min_vfs::umount(S7XX_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 70);
		return 70;
	}

	return 0;
}

int remove_tests()
{
	constexpr char FILENAME[] = "test_file";

	u16 err, expected_err;
	std::string path;

	std::vector<min_vfs::dentry_t> dentries, expected_dentries;

	//Data setup
	if(std::filesystem::exists(FILENAME))
		std::filesystem::remove_all(FILENAME);

	if(std::filesystem::exists(TEST_HOST_DIR))
		std::filesystem::remove_all(TEST_HOST_DIR);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);

	//Host FS file
	err = min_vfs::ftruncate(FILENAME, 0);
	if(err)
	{
		print_unexpected_err(err, 71);
		return 71;
	}

	err = min_vfs::remove(FILENAME);
	if(err)
	{
		print_unexpected_err(err, 72);
		return 72;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(FILENAME, dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 73);
		return 73;
	}

	//Host FS dir
	err = min_vfs::mkdir(TEST_HOST_DIR);
	if(err)
	{
		print_unexpected_err(err, 74);
		return 74;
	}

	err = min_vfs::ftruncate(std::string(TEST_HOST_DIR) + "/" + FILENAME, 0);
	if(err)
	{
		print_unexpected_err(err, 75);
		return 75;
	}

	err = min_vfs::remove(TEST_HOST_DIR);
	if(err)
	{
		print_unexpected_err(err, 76);
		return 76;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(TEST_HOST_DIR, dentries, true);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 77);
		return 77;
	}

	//EMU FS
	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 78);
		return 78;
	}

	path = std::string(EMU_FS_PATH) + "/" + EMU_TEST::EXPECTED_ROOT_DIR[0].fname
		+ "/" + FILENAME;
	err = min_vfs::ftruncate(path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 79);
		return 79;
	}

	err = min_vfs::remove(path.c_str());
	if(err)
	{
		print_unexpected_err(err, 80);
		return 80;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 81);
		return 81;
	}

	//S7XX FS
	err = min_vfs::mount(S7XX_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 82);
		return 82;
	}

	path = std::string(S7XX_FS_SYMLINK_PATH) + "/Volumes/" + FILENAME;
	err = min_vfs::ftruncate(path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 83);
		return 83;
	}

	err = min_vfs::remove(path.c_str());
	if(err)
	{
		print_unexpected_err(err, 84);
		return 84;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 85);
		return 85;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 86);
		return 86;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 87);
		return 87;
	}

	return 0;
}

int fopen_tests()
{
	u16 err, expected_err;
	uintmax_t expected_open_file_count;
	std::string current_path;
	min_vfs::stream_t stream_a, stream_b;

	std::vector<min_vfs::mount_stats_t> mounts;
	std::vector<min_vfs::dentry_t> dentries;

	if(std::filesystem::exists(TEST_HOST_DIR))
		std::filesystem::remove_all(TEST_HOST_DIR);

	std::filesystem::create_directory(TEST_HOST_DIR);

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);

	min_vfs::lsmount(mounts);
	const size_t BASE_MOUNT = mounts.size();

	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 89);
		return 89;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 90);
		return 90;
	}

	//Open first
	err = min_vfs::fopen(EMPTY_FILE_PATH, stream_a);
	if(err)
	{
		print_unexpected_err(err, 91);
		return 91;
	}

	//Open second
	err = min_vfs::fopen(EMPTY_FILE_PATH, stream_b);
	if(err)
	{
		print_unexpected_err(err, 92);
		return 92;
	}

	//Close first
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 93);
		return 93;
	}

	//Try to fopen dir
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_A_FILE);
	err = min_vfs::fopen(TEST_HOST_DIR, stream_a);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 94);
		return 94;
	}

	//fopen create
	err = min_vfs::fopen(TEST_FOPEN_CREATE, stream_a);
	if(err)
	{
		print_unexpected_err(err, 95);
		return 95;
	}

	//Open in mounted FS over stream_b
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/" + TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 96);
		return 96;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);
	if(mounts.size() != BASE_MOUNT + 2)
	{
		std::cerr << "Mounted FS count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << BASE_MOUNT + 2 << std::endl;
		std::cerr << "Got: " << mounts.size() << std::endl;
		std::cerr << "Exit: " << 97 << std::endl;
		return 97;
	}

	//EMU FS counts dirs containing open files as open
	expected_open_file_count = 2;
	if(mounts[BASE_MOUNT].open_file_count != expected_open_file_count)
	{
		std::cerr << "EMU FS open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_open_file_count << std::endl;
		std::cerr << "Got: " << mounts[BASE_MOUNT].open_file_count << std::endl;
		std::cerr << "Exit: " << 98 << std::endl;
		return 98;
	}

	err = min_vfs::list(current_path, dentries);
	if(err)
	{
		print_unexpected_err(err, 99);
		return 99;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 100 << std::endl;
		return 100;
	}

	//Open in mounted FS over stream_a, through symlink
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Volumes/"
		+ TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 101);
		return 101;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);
	expected_open_file_count = 1;
	if(mounts[BASE_MOUNT + 1].open_file_count != expected_open_file_count)
	{
		std::cerr << "S7XX FS open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_open_file_count << std::endl;
		std::cerr << "Got: " << mounts[BASE_MOUNT + 1].open_file_count
			<< std::endl;
		std::cerr << "Exit: " << 102 << std::endl;
		return 102;
	}

	dentries.clear();
	err = min_vfs::list(current_path, dentries);
	if(err)
	{
		print_unexpected_err(err, 103);
		return 103;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 104 << std::endl;
		return 104;
	}

	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 105);
		return 105;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);
	expected_open_file_count = 0;
	if(mounts[BASE_MOUNT + 1].open_file_count != expected_open_file_count)
	{
		std::cerr << "S7XX FS open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_open_file_count << std::endl;
		std::cerr << "Got: " << mounts[BASE_MOUNT + 1].open_file_count
			<< std::endl;
		std::cerr << "Exit: " << 106 << std::endl;
		return 106;
	}

	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 107);
		return 107;
	}

	mounts.clear();
	min_vfs::lsmount(mounts);
	if(mounts[BASE_MOUNT].open_file_count != expected_open_file_count)
	{
		std::cerr << "EMU FS open file count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_open_file_count << std::endl;
		std::cerr << "Got: " << mounts[BASE_MOUNT].open_file_count << std::endl;
		std::cerr << "Exit: " << 108 << std::endl;
		return 108;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 109);
		return 109;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 110);
		return 110;
	}

	return 0;
}

int read_tests()
{
	u16 err, expected_err;
	std::fstream fstr;
	min_vfs::stream_t mstr;
	std::string current_path;

	std::unique_ptr<u8[]> buffer_a, buffer_b;

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 111);
		return 111;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 112);
		return 112;
	}

	//Host FS
	err = min_vfs::fopen(TEST_FOPEN_CREATE, mstr);
	if(err)
	{
		print_unexpected_err(err, 113);
		return 113;
	}

	fstr.open(TEST_FOPEN_CREATE);

	buffer_a = std::make_unique<u8[]>(BUFFER_SIZE);
	buffer_b = std::make_unique<u8[]>(BUFFER_SIZE);

	std::memset(buffer_a.get(), 0x55, BUFFER_SIZE);

	fstr.write((char*)buffer_a.get(), BUFFER_SIZE);
	fstr.close();

	err = mstr.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 114);
		return 114;
	}

	if(std::memcmp(buffer_a.get(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Read data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(buffer_a.get(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 115 << std::endl;
		return 115;
	}

	//EMU FS
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/"
		+ EMU_TEST::EXPECTED_DIR0[0].fname;
	err = min_vfs::fopen(current_path.c_str(), mstr);
	if(err)
	{
		print_unexpected_err(err, 116);
		return 116;
	}

	err = mstr.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 117);
		return 117;
	}

	if(std::memcmp(EMU_FS_EXPECTED_READ.data(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Read data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(EMU_FS_EXPECTED_READ.data(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 118 << std::endl;
		return 118;
	}

	//S7XX FS
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Samples/0-";
	err = min_vfs::fopen(current_path.c_str(), mstr);
	if(err)
	{
		print_unexpected_err(err, 119);
		return 119;
	}

	err = mstr.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 120);
		return 120;
	}

	if(std::memcmp(S7XX_FS_EXPECTED_READ.data(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Read data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(S7XX_FS_EXPECTED_READ.data(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 121 << std::endl;
		return 121;
	}

	//Cleanup
	err = mstr.close();
	if(err)
	{
		print_unexpected_err(err, 122);
		return 122;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 123);
		return 123;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 124);
		return 124;
	}

	return 0;
}

int write_tests()
{
	u16 err, expected_err;
	min_vfs::stream_t stream_a, stream_b;
	std::string current_path;

	std::unique_ptr<u8[]> buffer_a, buffer_b;

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 125);
		return 125;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 126);
		return 126;
	}

	buffer_a = std::make_unique<u8[]>(BUFFER_SIZE);
	buffer_b = std::make_unique<u8[]>(BUFFER_SIZE);

	std::memset(buffer_a.get(), 0xAA, BUFFER_SIZE);

	//Host FS
	err = min_vfs::fopen(TEST_FOPEN_CREATE, stream_a);
	if(err)
	{
		print_unexpected_err(err, 127);
		return 127;
	}

	err = min_vfs::fopen(TEST_FOPEN_CREATE, stream_b);
	if(err)
	{
		print_unexpected_err(err, 128);
		return 128;
	}

	err = stream_a.write(buffer_a.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 129);
		return 129;
	}

	err = stream_a.flush();
	if(err)
	{
		print_unexpected_err(err, 130);
		return 130;
	}

	err = stream_b.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 131);
		return 131;
	}

	err = stream_a.seek(0);
	if(err)
	{
		print_unexpected_err(err, 132);
		return 132;
	}

	if(std::memcmp(buffer_a.get(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(buffer_a.get(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 133 << std::endl;
		return 133;
	}

	//EMU FS
	std::memset(buffer_a.get(), 0x55, BUFFER_SIZE);
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/" + TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 134);
		return 134;
	}

	err = min_vfs::fopen(current_path.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 135);
		return 135;
	}

	err = stream_a.write(buffer_a.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 136);
		return 136;
	}

	err = stream_a.flush();
	if(err)
	{
		print_unexpected_err(err, 137);
		return 137;
	}

	err = stream_b.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 138);
		return 138;
	}

	if(std::memcmp(buffer_a.get(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(buffer_a.get(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 139 << std::endl;
		return 139;
	}

	//S7XX FS
	std::memset(buffer_a.get(), 0xAA, BUFFER_SIZE);
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Samples/"
		+ TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream_a);
	if(err)
	{
		print_unexpected_err(err, 140);
		return 140;
	}

	err = min_vfs::fopen(current_path.c_str(), stream_b);
	if(err)
	{
		print_unexpected_err(err, 141);
		return 141;
	}

	err = stream_a.write(buffer_a.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 142);
		return 142;
	}

	err = stream_a.flush();
	if(err)
	{
		print_unexpected_err(err, 143);
		return 143;
	}

	err = stream_b.read(buffer_b.get(), BUFFER_SIZE);
	if(err)
	{
		print_unexpected_err(err, 144);
		return 144;
	}

	if(std::memcmp(buffer_a.get(), buffer_b.get(), BUFFER_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		std::cerr << "Expected:\n";
		print_buffer(buffer_a.get(), BUFFER_SIZE);
		std::cerr << "Got:\n";
		print_buffer(buffer_b.get(), BUFFER_SIZE);
		std::cerr << "Exit: " << 145 << std::endl;
		return 145;
	}

	//Cleanup
	err = stream_a.close();
	if(err)
	{
		print_unexpected_err(err, 146);
		return 146;
	}

	err = stream_b.close();
	if(err)
	{
		print_unexpected_err(err, 147);
		return 147;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 148);
		return 148;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 149);
		return 149;
	}

	return 0;
}

int trunc_open_tests()
{
	u16 err, expected_err;
	uintmax_t target_fsize;
	min_vfs::stream_t stream;
	std::string current_path;

	std::vector<min_vfs::dentry_t> dentries;

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 150);
		return 150;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 151);
		return 151;
	}

	//Host FS
	err = min_vfs::fopen(TEST_FOPEN_CREATE, stream);
	if(err)
	{
		print_unexpected_err(err, 152);
		return 152;
	}

	target_fsize = 256;
	err = min_vfs::ftruncate(TEST_FOPEN_CREATE, target_fsize);
	if(err)
	{
		print_unexpected_err(err, 153);
		return 153;
	}

	err = min_vfs::list(TEST_FOPEN_CREATE, dentries);
	if(err)
	{
		print_unexpected_err(err, 154);
		return 154;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 155 << std::endl;
		return 155;
	}

	if(dentries[0].fsize != target_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << target_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: " << 156 << std::endl;
		return 156;
	}

	//EMU FS
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/" + TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 157);
		return 157;
	}

	target_fsize = 512 + 64;
	err = min_vfs::ftruncate(current_path.c_str(), target_fsize);
	if(err)
	{
		print_unexpected_err(err, 158);
		return 158;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 159);
		return 159;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 160 << std::endl;
		return 160;
	}

	if(dentries[0].fsize != target_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << target_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: " << 161 << std::endl;
		return 161;
	}

	//S7XX FS
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Samples/"
		+ TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 162);
		return 162;
	}

	target_fsize = 512 + 64;
	err = min_vfs::ftruncate(current_path.c_str(), target_fsize);
	if(err)
	{
		print_unexpected_err(err, 163);
		return 163;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 164);
		return 164;
	}

	if(dentries.size() != 1)
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 165 << std::endl;
		return 165;
	}

	//Actual expected size
	target_fsize = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
		+ S7XX::AUDIO_SEGMENT_SIZE;

	if(dentries[0].fsize != target_fsize)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << target_fsize << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: " << 166 << std::endl;
		return 166;
	}

	//Cleanup
	err = stream.close();
	if(err)
	{
		print_unexpected_err(err, 167);
		return 167;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 168);
		return 168;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 169);
		return 169;
	}

	return 0;
}

int umount_open_tests()
{
	u16 err, expected_err;
	min_vfs::stream_t stream;
	std::string current_path;

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 170);
		return 170;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 171);
		return 171;
	}

	//EMU FS
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/" + TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 172);
		return 172;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FS_BUSY);
	err = min_vfs::umount(EMU_FS_PATH);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 173);
		return 173;
	}

	//S7XX FS
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Samples/"
		+ TEST_FOPEN_CREATE;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 174);
		return 174;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::FS_BUSY);
	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 175);
		return 175;
	}

	//Cleanup
	err = stream.close();
	if(err)
	{
		print_unexpected_err(err, 176);
		return 176;
	}

	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 177);
		return 177;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 178);
		return 178;
	}

	return 0;
}

int rename_tests()
{
	constexpr char MOVE_AND_RENAME_FNAME[] = "mv_rename_file";
	constexpr char MOVE_AND_RENAME_DIRNAME[] = "mv_rename_dir";

	u16 err, expected_err;
	std::string current_path, new_path;
	min_vfs::dentry_t expected_dentry;

	std::vector<min_vfs::dentry_t> dentries, dentries_old;

	if(std::filesystem::exists(MOVE_NESTED_TEST_DIR))
		std::filesystem::remove_all(MOVE_NESTED_TEST_DIR);

	std::filesystem::create_directory(MOVE_NESTED_TEST_DIR);

	if(std::filesystem::exists(EMU_TEST::EXPECTED_DIR0[0].fname))
		std::filesystem::remove_all(EMU_TEST::EXPECTED_DIR0[0].fname);

	if(std::filesystem::exists(EMU_TEST::EXPECTED_ROOT_DIR[1].fname))
		std::filesystem::remove_all(EMU_TEST::EXPECTED_ROOT_DIR[1].fname);

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	if(std::filesystem::exists(EMPTY_FILE_PATH))
		std::filesystem::remove_all(EMPTY_FILE_PATH);

	if(std::filesystem::exists(RENAME_TEST_FILENAME))
		std::filesystem::remove_all(RENAME_TEST_FILENAME);

	if(std::filesystem::exists(MOVE_AND_RENAME_FNAME))
		std::filesystem::remove_all(MOVE_AND_RENAME_FNAME);

	if(std::filesystem::exists(MOVE_AND_RENAME_DIRNAME))
		std::filesystem::remove_all(MOVE_AND_RENAME_DIRNAME);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 179);
		return 179;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 180);
		return 180;
	}

	//Host rename
	err = min_vfs::ftruncate(EMPTY_FILE_PATH, 0);
	if(err)
	{
		print_unexpected_err(err, 181);
		return 181;
	}

	err = min_vfs::rename(EMPTY_FILE_PATH, RENAME_TEST_FILENAME);
	if(err)
	{
		print_unexpected_err(err, 182);
		return 182;
	}

	err = min_vfs::list(RENAME_TEST_FILENAME, dentries);
	if(err)
	{
		print_unexpected_err(err, 183);
		return 183;
	}

	if(dentries[0].fname != RENAME_TEST_FILENAME
		|| dentries[0].ftype != min_vfs::ftype_t::file
		|| dentries[0].fsize != 0)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected filename: " << RENAME_TEST_FILENAME << std::endl;
		std::cerr << "Actual filename: " << dentries[0].fname << std::endl;
		std::cerr << "Expected ftype: "
			<< min_vfs::ftype_to_string(min_vfs::ftype_t::file) << std::endl;
		std::cerr << "Actual ftype: "
			<< min_vfs::ftype_to_string(dentries[0].ftype) << std::endl;
		std::cerr << "Expected fsize: " << 0 << std::endl;
		std::cerr << "Actual fsize: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: " << 184 << std::endl;
		return 184;
	}

	//EMU rename
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/";
	new_path = current_path;
	current_path += EMPTY_FILE_PATH;
	new_path += RENAME_TEST_FILENAME;

	err = min_vfs::ftruncate(current_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 185);
		return 185;
	}

	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 186);
		return 186;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 187);
		return 187;
	}

	expected_dentry =
	{
		.fname = std::string("0-") + RENAME_TEST_FILENAME,
		.fsize = 0,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	};

	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 188 << std::endl;
		return 188;
	}

	//S7XX rename
	current_path = std::string(S7XX_FS_SYMLINK_PATH) + "/Volumes/";
	new_path = current_path;
	current_path += EMPTY_FILE_PATH;
	new_path += RENAME_TEST_FILENAME;

	err = min_vfs::ftruncate(current_path.c_str(), 0);
	if(err)
	{
		print_unexpected_err(err, 189);
		return 189;
	}

	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 190);
		return 190;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 191);
		return 191;
	}

	expected_dentry.fname = std::string("1-") + RENAME_TEST_FILENAME + "     ";
	expected_dentry.fsize = S7XX::FS::On_disk_sizes::VOLUME_PARAMS_ENTRY;
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 192 << std::endl;
		return 192;
	}

	//EMU->Host file move
	current_path = std::string(EMU_FS_PATH) + "/"
	+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/"
	+ EMU_TEST::EXPECTED_DIR0[0].fname;
	err = min_vfs::rename(current_path.c_str(),
						  std::filesystem::current_path().c_str());
	if(err)
	{
		print_unexpected_err(err, 193);
		return 193;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 194);
		return 194;
	}

	dentries.clear();
	err = min_vfs::list(EMU_TEST::EXPECTED_DIR0[0].fname, dentries);
	if(err)
	{
		print_unexpected_err(err, 195);
		return 195;
	}

	expected_dentry = EMU_TEST::EXPECTED_DIR0[0];
	expected_dentry.mtime = dentries[0].mtime;
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 196 << std::endl;
		return 196;
	}

	//Host->EMU file move
	err = min_vfs::rename(EMU_TEST::EXPECTED_DIR0[0].fname,
						  current_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 197);
		return 197;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(EMU_TEST::EXPECTED_DIR0[0].fname, dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 198);
		return 198;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 199);
		return 199;
	}

	expected_dentry.mtime = 0;
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 200 << std::endl;
		return 200;
	}

	//EMU->Host dir move
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[1].fname;
	err = min_vfs::rename(current_path.c_str(),
						  std::filesystem::current_path().c_str());
	if(err)
	{
		print_unexpected_err(err, 201);
		return 201;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries, true);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 202);
		return 202;
	}

	dentries.clear();
	err = min_vfs::list(EMU_TEST::EXPECTED_ROOT_DIR[1].fname, dentries);
	if(err)
	{
		print_unexpected_err(err, 203);
		return 203;
	}

	if(!compare_dirs_ignore_times(EMU_TEST::EXPECTED_DIR1, dentries))
	{
		std::cerr << "Dir content mismatch!!!" << std::endl;
		std::cerr << "Exit: " << 204 << std::endl;
		return 204;
	}

	//Host->EMU dir move
	err = min_vfs::rename(EMU_TEST::EXPECTED_ROOT_DIR[1].fname,
						  EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 205);
		return 205;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(EMU_TEST::EXPECTED_ROOT_DIR[1].fname, dentries, true);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 206);
		return 206;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 207);
		return 207;
	}

	if(!compare_dirs(EMU_TEST::EXPECTED_DIR1, dentries))
	{
		std::cerr << "Dir content mismatch!!!" << std::endl;
		std::cerr << "Exit: " << 208 << std::endl;
		return 208;
	}

	//EMU->Host file move + rename
	current_path = std::string(EMU_FS_PATH) + "/"
	+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/"
	+ EMU_TEST::EXPECTED_DIR0[1].fname;
	new_path = (std::filesystem::current_path() / MOVE_AND_RENAME_FNAME)
		.string();

	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 241);
		return 241;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 242);
		return 242;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 243);
		return 243;
	}

	expected_dentry = EMU_TEST::EXPECTED_DIR0[1];
	expected_dentry.mtime = dentries[0].mtime;
	expected_dentry.fname = MOVE_AND_RENAME_FNAME;
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 244 << std::endl;
		return 244;
	}

	//EMU->Host dir move + rename
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[1].fname;
	new_path = (std::filesystem::current_path() / MOVE_AND_RENAME_DIRNAME)
		.string();

	err = min_vfs::list(current_path, dentries_old);
	if(err)
	{
		print_unexpected_err(err, 245);
		return 245;
	}

	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 246);
		return 246;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 247);
		return 247;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries, true);
	if(err)
	{
		print_unexpected_err(err, 248);
		return 248;
	}

	expected_dentry = EMU_TEST::EXPECTED_ROOT_DIR[1];
	expected_dentry.mtime = dentries[0].mtime;
	expected_dentry.fname = MOVE_AND_RENAME_DIRNAME;
	expected_dentry.fsize = 0; //Host dir
	if(dentries[0] != expected_dentry)
	{
		std::cerr << "Dentry mismatch!!!" << std::endl;
		std::cerr << "Expected:\n" << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got:\n" << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 249 << std::endl;
		return 249;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 250);
		return 250;
	}

	if(dentries.size() != dentries_old.size())
	{
		std::cerr << "Dentry count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << dentries_old.size() << std::endl;
		std::cerr << "Got: " << dentries.size() << std::endl;
		std::cerr << "Exit: " << 250 << std::endl;
		return 250;
	}

	if(!compare_dirs_ignore_times(dentries_old, dentries))
	{
		std::cerr << "Exit: " << 251 << std::endl;
		return 251;
	}

	/*TODO Add test case for nested directories. The only way we would have of
	testing this currently is moving a filesystem's root. The problem is, moving
	mount points is currently not allowed; and, if it is ever allowed, it should
	just move the image file itself and swap out the stream the appropriate
	driver instance is using (if we even need to, since I think POSIX specifies
	open files should be perfectly fine to rename).*/

	//Cleanup
	err = min_vfs::umount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 209);
		return 209;
	}

	err = min_vfs::umount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 210);
		return 210;
	}

	return 0;
}

int rename_open_tests()
{
	u16 err, expected_err;
	std::string current_path, new_path;
	min_vfs::dentry_t expected_dentry;
	min_vfs::stream_t stream;

	std::vector<min_vfs::dentry_t> dentries;

	if(std::filesystem::exists(MOVE_NESTED_TEST_DIR))
		std::filesystem::remove_all(MOVE_NESTED_TEST_DIR);

	std::filesystem::create_directory(MOVE_NESTED_TEST_DIR);

	if(std::filesystem::exists(EMU_TEST::EXPECTED_DIR0[0].fname))
		std::filesystem::remove_all(EMU_TEST::EXPECTED_DIR0[0].fname);

	if(std::filesystem::exists(EMU_TEST::EXPECTED_ROOT_DIR[1].fname))
		std::filesystem::remove_all(EMU_TEST::EXPECTED_ROOT_DIR[1].fname);

	if(std::filesystem::exists(TEST_FOPEN_CREATE))
		std::filesystem::remove_all(TEST_FOPEN_CREATE);

	if(std::filesystem::exists(EMPTY_FILE_PATH))
		std::filesystem::remove_all(EMPTY_FILE_PATH);

	if(std::filesystem::exists(RENAME_TEST_FILENAME))
		std::filesystem::remove_all(RENAME_TEST_FILENAME);

	//EMU FS setup
	if(std::filesystem::exists(EMU_FS_PATH))
		std::filesystem::remove_all(EMU_FS_PATH);

	std::filesystem::copy_file(BASE_TEST_FS_PATH, EMU_FS_PATH);

	//S7XX FS setup
	if(std::filesystem::exists(S7XX_FS_PATH))
		std::filesystem::remove_all(S7XX_FS_PATH);

	std::filesystem::copy_file(std::filesystem::read_symlink(TEST_S7XX_FS_PATH),
							   S7XX_FS_PATH);

	if(std::filesystem::exists(S7XX_FS_SYMLINK_PATH))
		std::filesystem::remove_all(S7XX_FS_SYMLINK_PATH);

	std::filesystem::create_symlink(S7XX_FS_PATH, S7XX_FS_SYMLINK_PATH);


	err = min_vfs::mount(EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 211);
		return 211;
	}

	err = min_vfs::mount(S7XX_FS_SYMLINK_PATH);
	if(err)
	{
		print_unexpected_err(err, 212);
		return 212;
	}

	//TODO Check whether this fails on WinNT if we don't force it
	//Try rename mount point (should fail)
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::CANT_MOVE);
	err = min_vfs::rename(EMU_FS_PATH, RENAME_OPEN_TEST_FILENAME);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 213);
		return 213;
	}

	err = min_vfs::list(EMU_FS_PATH, dentries);
	if(err)
	{
		print_unexpected_err(err, 214);
		return 214;
	}

	//TODO Check whether this fails on WinNT
	//Host rename (should succeed on POSIX)
	err = min_vfs::fopen(EMPTY_FILE_PATH, stream);
	if(err)
	{
		print_unexpected_err(err, 215);
		return 215;
	}

	err = min_vfs::rename(EMPTY_FILE_PATH, RENAME_OPEN_TEST_FILENAME);
	if(err)
	{
		print_unexpected_err(err, 216);
		return 216;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(EMPTY_FILE_PATH, dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 217);
		return 217;
	}

	err = min_vfs::list(RENAME_OPEN_TEST_FILENAME, dentries);
	if(err)
	{
		print_unexpected_err(err, 218);
		return 218;
	}

	//EMU FS rename (should fail)
	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname;
	new_path = current_path;
	current_path += "/" + EMU_TEST::EXPECTED_DIR0[0].fname;
	new_path += std::string("/") + RENAME_OPEN_TEST_FILENAME;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 219);
		return 219;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::ALREADY_OPEN);
	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err != expected_err)
	{
		print_unexpected_err(err, 220);
		return 220;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 221);
		return 221;
	}

	//EMU -> Host file move (should copy then fail to remove)
	std::filesystem::remove_all(RENAME_OPEN_TEST_FILENAME);

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::ALREADY_OPEN);
	err = min_vfs::rename(current_path.c_str(), RENAME_OPEN_TEST_FILENAME);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 222);
		return 222;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 223);
		return 223;
	}

	dentries.clear();
	err = min_vfs::list(RENAME_OPEN_TEST_FILENAME, dentries);
	if(err)
	{
		print_unexpected_err(err, 224);
		return 224;
	}

	expected_dentry = EMU_TEST::EXPECTED_DIR0[0];
	expected_dentry.fname = RENAME_OPEN_TEST_FILENAME;
	if(!compare_dentries_ignore_time(dentries[0], expected_dentry))
	{
		std::cerr << "Dentry mismatch (ignore time)!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 225 << std::endl;
		return 225;
	}

	//TODO Test that WinNT fails to remove
	/*Host -> EMU file move. Should copy then fail to remove (WinNT) or remove
	successfully (POSIX).*/
	current_path = RENAME_OPEN_TEST_FILENAME;
	new_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[0].fname + "/" + RENAME_OPEN_TEST_FILENAME;
	err = min_vfs::fopen(current_path.c_str(), stream);
	if(err)
	{
		print_unexpected_err(err, 226);
		return 226;
	}

	err = min_vfs::rename(current_path.c_str(), new_path.c_str());
	if(err)
	{
		print_unexpected_err(err, 227);
		return 227;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 228);
		return 228;
	}

	dentries.clear();
	err = min_vfs::list(new_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 229);
		return 229;
	}

	expected_dentry = EMU_TEST::EXPECTED_DIR0[0];
	expected_dentry.fname = std::string("0-") + RENAME_OPEN_TEST_FILENAME;
	if(!compare_dentries_ignore_time(dentries[0], expected_dentry))
	{
		std::cerr << "Dentry mismatch (ignore time)!!!" << std::endl;
		std::cerr << "Expected: " << expected_dentry.to_string(1) << std::endl;
		std::cerr << "Got: " << dentries[0].to_string(1) << std::endl;
		std::cerr << "Exit: " << 230 << std::endl;
		return 230;
	}

	/*EMU -> Host dir move (should copy everything, remove most files, then fail
	to remove the open file and dir).*/
	if(std::filesystem::exists(EMU_TEST::EXPECTED_ROOT_DIR[1].fname))
		std::filesystem::remove_all(EMU_TEST::EXPECTED_ROOT_DIR[1].fname);

	current_path = std::string(EMU_FS_PATH) + "/"
		+ EMU_TEST::EXPECTED_ROOT_DIR[1].fname;
	err = min_vfs::fopen(current_path + "/" + EMU_TEST::EXPECTED_DIR1[0].fname,
						 stream);
	if(err)
	{
		print_unexpected_err(err, 231);
		return 231;
	}

	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::ALREADY_OPEN);
	err = min_vfs::rename(current_path.c_str(),
						  std::filesystem::current_path());
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 232);
		return 232;
	}

	dentries.clear();
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 233);
		return 233;
	}

	new_path = (std::filesystem::current_path()
		/ EMU_TEST::EXPECTED_ROOT_DIR[1].fname).string();

	dentries.clear();
	err = min_vfs::list(new_path, dentries);
	if(err)
	{
		print_unexpected_err(err, 234);
		return 234;
	}

	if(!compare_dirs_ignore_times(EMU_TEST::EXPECTED_DIR1, dentries))
	{
		std::cerr << "Dentry mismatch (ignore time)!!!" << std::endl;
		std::cerr << "Exit: " << 235 << std::endl;
		return 235;
	}

	//TODO Check Windows behavior
	/*Host -> EMU dir move. Should copy everything, remove most files, then fail
	 *to remove (WinNT) or remove successfully (POSIX).*/
	if(std::filesystem::exists(RENAME_TEST_DIR))
		std::filesystem::remove_all(RENAME_TEST_DIR);

	std::filesystem::rename(EMU_TEST::EXPECTED_ROOT_DIR[1].fname,
							RENAME_TEST_DIR);

	current_path = std::string(RENAME_TEST_DIR);
	err = min_vfs::fopen(current_path + "/" + EMU_TEST::EXPECTED_DIR1[0].fname,
						 stream);
	if(err)
	{
		print_unexpected_err(err, 236);
		return 236;
	}

	err = min_vfs::rename(current_path.c_str(),
						  EMU_FS_PATH);
	if(err)
	{
		print_unexpected_err(err, 237);
		return 237;
	}

	dentries.clear();
	expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
								 (u8)min_vfs::ERR::NOT_FOUND);
	err = min_vfs::list(current_path.c_str(), dentries);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 238);
		return 238;
	}

	new_path = std::string(EMU_FS_PATH) + "/" + RENAME_TEST_DIR;

	dentries.clear();
	err = min_vfs::list(new_path, dentries);
	if(err)
	{
		print_unexpected_err(err, 239);
		return 239;
	}

	if(!compare_dirs_ignore_times(EMU_TEST::EXPECTED_DIR1, dentries))
	{
		std::cerr << "Dentry mismatch (ignore time)!!!" << std::endl;
		std::cerr << "Exit: " << 240 << std::endl;
		return 240;
	}

	return 0;
}

int main()
{
	int err;

	std::cout << "Mount tests..." << std::endl;
	err = mount_tests();
	if(err) return err;
	std::cout << "Mount tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "List tests..." << std::endl;
	err = list_tests();
	if(err) return err;
	std::cout << "List tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "mkdir tests..." << std::endl;
	err = mkdir_tests();
	if(err) return err;
	std::cout << "mkdir tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Umount tests..." << std::endl;
	err = umount_tests();
	if(err) return err;
	std::cout << "Umount tests OK!" << std::endl;
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

	std::cout << "Umount open tests..." << std::endl;
	err = umount_open_tests();
	if(err) return err;
	std::cout << "Umount open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Rename tests..." << std::endl;
	err = rename_tests();
	if(err) return err;
	std::cout << "Rename tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Rename open tests..." << std::endl;
	err = rename_open_tests();
	if(err) return err;
	std::cout << "Rename open tests OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!!!" << std::endl;

	return 0;
}
