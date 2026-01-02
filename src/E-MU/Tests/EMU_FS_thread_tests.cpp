#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "E-MU/EMU_FS_drv.hpp"
#include "fs_test_data.hpp"
#include "Helpers.hpp"

using namespace EMU::FS::testing;

static int separate_files()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u8 BASE_CLS_CNT = 20;
	constexpr char S7XX_FS[] = "write_mt_separate_tests.img";

	const std::string BASE_PATH = "/" + EXPECTED_ROOT_DIR[2].fname + "/mt_test_";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];
	std::mutex print_mtx;

	std::unique_ptr<EMU::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, S7XX_FS);

	try
	{
		fs = std::make_unique<EMU::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 1" << std::endl;
		return 1;
	}

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		err = fs->fopen((BASE_PATH + std::to_string(i)).c_str(), streams[i]);
		if(err)
		{
			print_unexpected_err(err, 2);
			return 2;
		}
	}
	/*----------------------------End of data setup---------------------------*/

	//Write
	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i] = std::thread
		(
			[i, &streams, &print_mtx]()
			{
				const u8 VAL = i + 1;
				u16 thread_err;

				std::unique_ptr<u8[]> LOCAL_CLUSTER;

				LOCAL_CLUSTER = std::make_unique<u8[]>(CLUSTER_SIZE);
				std::memset(LOCAL_CLUSTER.get(), VAL, CLUSTER_SIZE);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].write(LOCAL_CLUSTER.get(),
												  CLUSTER_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 14);
						print_mtx.unlock();
						return (u16)14;
					}
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++) threads[i].join();

	cluster = std::make_unique<u8[]>(CLUSTER_SIZE);
	expected_cluster = std::make_unique<u8[]>(CLUSTER_SIZE);

	//Check
	for (u8 i = 0; i < STREAM_CNT; i++)
	{
		dentries.clear();
		err = fs->list((BASE_PATH + std::to_string(i)).c_str(), dentries);
		if(err)
		{
			print_unexpected_err(err, 3);
			return 3;
		}

		const u32 expected_file_size = CLUSTER_SIZE * (i + 1 + BASE_CLS_CNT);

		if(dentries[0].fsize != expected_file_size)
		{
			std::cerr << "File size mismatch!!!" << std::endl;
			std::cerr << "Expected: " << expected_file_size << std::endl;
			std::cerr << "Got: " << dentries[0].fsize << std::endl;
			std::cerr << "Exit: 4" << std::endl;
			return 4;
		}

		std::memset(expected_cluster.get(), i + 1, CLUSTER_SIZE);

		for(u8 j = 0; j < i + 1 + BASE_CLS_CNT; j++)
		{
			streams[i].seek(16 + j * CLUSTER_SIZE);
			streams[i].read(cluster.get(), CLUSTER_SIZE);

			if(std::memcmp(cluster.get(), expected_cluster.get(),
				CLUSTER_SIZE))
			{
				std::cerr << "File data mismatch!!!" << std::endl;
				print_buffers(expected_cluster.get(), cluster.get(),
							  CLUSTER_SIZE);
				std::cerr << "Exit: 5" << std::endl;
				return 5;
			}
		}
	}

	//Read
	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i] = std::thread
		(
			[i, &streams, &print_mtx]()
			{
				const u8 VAL = i + 1;
				u16 thread_err;

				std::unique_ptr<u8[]> LOCAL_EXPECTED_CLUSTER, local_cluster;

				LOCAL_EXPECTED_CLUSTER = std::make_unique<u8[]>(CLUSTER_SIZE);
				std::memset(LOCAL_EXPECTED_CLUSTER.get(), VAL, CLUSTER_SIZE);

				local_cluster = std::make_unique<u8[]>(CLUSTER_SIZE);

				streams[i].seek(0);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].read(local_cluster.get(),
												  CLUSTER_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 15);
						print_mtx.unlock();
						return (u16)15;
					}

					if(std::memcmp(local_cluster.get(),
						LOCAL_EXPECTED_CLUSTER.get(), CLUSTER_SIZE))
					{
						print_mtx.lock();
						std::cerr << "File data mismatch!!!" << std::endl;
						std::cerr << "Thread " << (u16)i << std::endl;
						std::cerr << "Exit: 16" << std::endl;
						print_mtx.unlock();
						return (u16)16;
					}
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i].join();
		streams[i].close();
	}

	/*-------------------------------Safety fsck------------------------------*/
	fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 6);
		return 6;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 7" << std::endl;
		return 7;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int same_file()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u8 CLUSTER_CNT = 16;
	constexpr char S7XX_FS[] = "write_mt_same_tests.img";

	const std::string TEST_PATH = "/" + EXPECTED_ROOT_DIR[2].fname
		+ "/mt_test_1";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];

	std::unique_ptr<EMU::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_IMG_NAME, S7XX_FS);

	try
	{
		fs = std::make_unique<EMU::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 8" << std::endl;
		return 8;
	}

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		err = fs->fopen(TEST_PATH.c_str(), streams[i]);
		if(err)
		{
			print_unexpected_err(err, 9);
			return 9;
		}
	}
	/*----------------------------End of data setup---------------------------*/

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i] = std::thread
		(
			[i, &streams]()
			{
				const u8 VAL = i + 1;
				u16 thread_err;

				std::unique_ptr<u8[]> LOCAL_CLUSTER;

				LOCAL_CLUSTER = std::make_unique<u8[]>(CLUSTER_SIZE);
				std::memset(LOCAL_CLUSTER.get(), VAL, CLUSTER_SIZE);

				for(u8 j = 0; j < CLUSTER_CNT; j++)
				{
					thread_err = streams[i].write(LOCAL_CLUSTER.get(),
												  CLUSTER_SIZE);
					if(thread_err)
					{
						print_unexpected_err(thread_err, 17);
						return (u16)17;
					}
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i].join();
		streams[i].close();
	}

	dentries.clear();
	err = fs->list(TEST_PATH.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 10);
		return 10;
	}

	const u32 expected_file_size = CLUSTER_SIZE * CLUSTER_CNT;

	if(dentries[0].fsize != expected_file_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_file_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 11" << std::endl;
		return 11;
	}

	//We don't care about file contents.

	/*-------------------------------Safety fsck------------------------------*/
	fs->stream.flush();
	fsck_status = 0;
	err = EMU::FS::fsck(fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 12);
		return 12;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 13" << std::endl;
		return 13;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

int main()
{
	u16 err;

	std::cout << "Write/read separate files..." << std::endl;
	err = separate_files();
	if(err) return err;
	std::cout << "Write/read separate files OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write same file..." << std::endl;
	err = same_file();
	if(err) return err;
	std::cout << "Write same file OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;
	return 0;
}
