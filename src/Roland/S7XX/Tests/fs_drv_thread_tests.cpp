#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <filesystem>
#include <iostream>
#include <string>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S7XX/S7XX_types.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"
#include "Helpers.hpp"
#include "Test_data.hpp"

using namespace S7XX::FS::testing;

static int write_separate_samples()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u8 BASE_CLS_CNT = 20;
	constexpr char S7XX_FS[] = "write_mt_separate_tests.img";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];
	std::mutex print_mtx;

	std::unique_ptr<S7XX::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
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
		err = fs->fopen((std::string("/Samples/mt_test_") + std::to_string(i)).c_str(), streams[i]);
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

				LOCAL_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
				std::memset(LOCAL_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

				streams[i].seek(16);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].write(LOCAL_CLUSTER.get(),
												  S7XX::AUDIO_SEGMENT_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 33);
						print_mtx.unlock();
						return (u16)33;
					}
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++) threads[i].join();

	cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	expected_cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

	//Check
	for (u8 i = 0; i < STREAM_CNT; i++)
	{
		dentries.clear();
		err = fs->list((std::string("/Samples/mt_test_") +
			std::to_string(i)).c_str(), dentries);
		if(err)
		{
			print_unexpected_err(err, 3);
			return 3;
		}

		const u32 expected_file_size =
			S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
			+ S7XX::AUDIO_SEGMENT_SIZE * (i + 1 + BASE_CLS_CNT);

		if(dentries[0].fsize != expected_file_size)
		{
			std::cerr << "File size mismatch!!!" << std::endl;
			std::cerr << "Expected: " << expected_file_size << std::endl;
			std::cerr << "Got: " << dentries[0].fsize << std::endl;
			std::cerr << "Exit: 4" << std::endl;
			return 4;
		}

		std::memset(expected_cluster.get(), i + 1, S7XX::AUDIO_SEGMENT_SIZE);

		for(u8 j = 0; j < i + 1 + BASE_CLS_CNT; j++)
		{
			streams[i].seek(16 + j * S7XX::AUDIO_SEGMENT_SIZE);
			streams[i].read(cluster.get(), S7XX::AUDIO_SEGMENT_SIZE);

			if(std::memcmp(cluster.get(), expected_cluster.get(),
				S7XX::AUDIO_SEGMENT_SIZE))
			{
				std::cerr << "File data mismatch!!!" << std::endl;
				print_buffers(expected_cluster.get(), cluster.get(),
							  S7XX::AUDIO_SEGMENT_SIZE);
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

				LOCAL_EXPECTED_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
				std::memset(LOCAL_EXPECTED_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

				local_cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

				streams[i].seek(16);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].read(local_cluster.get(),
												 S7XX::AUDIO_SEGMENT_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 34);
						print_mtx.unlock();
						return (u16)34;
					}

					if(std::memcmp(local_cluster.get(),
						LOCAL_EXPECTED_CLUSTER.get(), S7XX::AUDIO_SEGMENT_SIZE))
					{
						print_mtx.lock();
						std::cerr << "File data mismatch!!!" << std::endl;
						std::cerr << "Thread " << (u16)i << std::endl;
						std::cerr << "Exit: 35" << std::endl;
						print_mtx.unlock();
						return (u16)35;
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
	err = S7XX::FS::fsck(fs->path, fsck_status);
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

static int same_sample()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u8 CLUSTER_CNT = 16;
	constexpr char S7XX_FS[] = "write_mt_same_tests.img";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];

	std::unique_ptr<S7XX::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
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
		err = fs->fopen("/Samples/mt_test_1", streams[i]);
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

				LOCAL_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
				std::memset(LOCAL_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

				streams[i].seek(16);

				for(u8 j = 0; j < CLUSTER_CNT; j++)
				{
					thread_err = streams[i].write(LOCAL_CLUSTER.get(),
												  S7XX::AUDIO_SEGMENT_SIZE);
					if(thread_err) return thread_err;
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
	err = fs->list("/Samples/mt_test_1", dentries);
	if(err)
	{
		print_unexpected_err(err, 10);
		return 10;
	}

	const u32 expected_file_size = S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
	+ S7XX::AUDIO_SEGMENT_SIZE * CLUSTER_CNT;

	if(dentries[0].fsize != expected_file_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_file_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 11" << std::endl;
		return 11;
	}

	//We don't care about file contents

	/*-------------------------------Safety fsck------------------------------*/
	fs->stream.flush();
	fsck_status = 0;
	err = S7XX::FS::fsck(fs->path, fsck_status);
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

static int write_separate_patches()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr char S7XX_FS[] = "write_mt_separate_patch_tests.img";
	constexpr char BASE_FNAME[] = "/Patches/mt_test_";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];
	std::mutex print_mtx;

	std::unique_ptr<S7XX::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 14" << std::endl;
		return 14;
	}

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		err = fs->fopen((std::string(BASE_FNAME) + std::to_string(i)).c_str(), streams[i]);
		if(err)
		{
			print_unexpected_err(err, 15);
			return 15;
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

				std::unique_ptr<u8[]> LOCAL_DATA;

				LOCAL_DATA = std::make_unique<u8[]>(S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
				std::memset(LOCAL_DATA.get(), VAL, S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

				streams[i].seek(0);

				thread_err = streams[i].write(LOCAL_DATA.get(),
											  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 36);
					print_mtx.unlock();
					return (u16)36;
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++) threads[i].join();

	cluster = std::make_unique<u8[]>(
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
	expected_cluster = std::make_unique<u8[]>(
		S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

	//Check
	for (u8 i = 0; i < STREAM_CNT; i++)
	{
		dentries.clear();
		err = fs->list((std::string(BASE_FNAME) +
			std::to_string(i)).c_str(), dentries);
		if(err)
		{
			print_unexpected_err(err, 16);
			return 16;
		}

		const u32 expected_file_size =
			S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY;

		if(dentries[0].fsize != expected_file_size)
		{
			std::cerr << "File size mismatch!!!" << std::endl;
			std::cerr << "Expected: " << expected_file_size << std::endl;
			std::cerr << "Got: " << dentries[0].fsize << std::endl;
			std::cerr << "Exit: 17" << std::endl;
			return 17;
		}

		std::memset(expected_cluster.get(), i + 1,
					S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

		streams[i].seek(0);
		streams[i].read(cluster.get(),
						S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

		if(std::memcmp(cluster.get(), expected_cluster.get(),
			S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
		{
			std::cerr << "File data mismatch!!!" << std::endl;
			print_buffers(expected_cluster.get(), cluster.get(),
						  S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
			std::cerr << "Exit: 18" << std::endl;
			return 18;
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

				LOCAL_EXPECTED_CLUSTER = std::make_unique<u8[]>(
						S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
				std::memset(LOCAL_EXPECTED_CLUSTER.get(), VAL,
							S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

				local_cluster = std::make_unique<u8[]>(S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);

				streams[i].seek(0);

				thread_err = streams[i].read(local_cluster.get(),
								S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 37);
					print_mtx.unlock();
					return (u16)37;
				}

				if(std::memcmp(local_cluster.get(),
					LOCAL_EXPECTED_CLUSTER.get(),
							   S7XX::FS::On_disk_sizes::PATCH_PARAMS_ENTRY))
				{
					print_mtx.lock();
					std::cerr << "File data mismatch!!!" << std::endl;
					std::cerr << "Thread " << (u16)i << std::endl;
					std::cerr << "Exit: 38" << std::endl;
					print_mtx.unlock();
					return (u16)38;
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
	err = S7XX::FS::fsck(fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 19);
		return 19;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 20" << std::endl;
		return 20;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

static int write_OS_and_samples()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u8 BASE_CLS_CNT = 20;
	constexpr u16 S770_OS_BLKS = S7XX::FS::On_disk_sizes::OS
		/ S7XX::FS::BLK_SIZE;
	constexpr char S7XX_FS[] = "write_mt_OS_and_samples.img";
	constexpr char BASE_FNAME[] = "/Samples/mt_test_";

	u16 err, fsck_status;

	std::unique_ptr<u8[]> cluster, expected_cluster;
	std::thread threads[STREAM_CNT];
	std::mutex print_mtx;

	std::unique_ptr<S7XX::FS::filesystem_t> fs;
	min_vfs::stream_t streams[STREAM_CNT];
	std::vector<min_vfs::dentry_t> dentries;


	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(S7XX_FS))
		std::filesystem::remove_all(S7XX_FS);

	std::filesystem::copy_file(TEST_FS_PATH, S7XX_FS);

	try
	{
		fs = std::make_unique<S7XX::FS::filesystem_t>(S7XX_FS);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 21" << std::endl;
		return 21;
	}

	for(u8 i = 0; i < STREAM_CNT - 1; i++)
	{
		err = fs->fopen((std::string(BASE_FNAME) + std::to_string(i)).c_str(), streams[i]);
		if(err)
		{
			print_unexpected_err(err, 22);
			return 22;
		}
	}

	err = fs->fopen("/OS", streams[STREAM_CNT - 1]);
	if(err)
	{
		print_unexpected_err(err, 23);
		return 23;
	}

	/*----------------------------End of data setup---------------------------*/

	//Write OS
	threads[STREAM_CNT - 1] = std::thread
	(
		[STREAM_CNT, &streams, &print_mtx]()
		{
			const u8 VAL = STREAM_CNT;
			u16 thread_err;

			std::unique_ptr<u8[]> LOCAL_CLUSTER;

			LOCAL_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
			std::memset(LOCAL_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

			streams[STREAM_CNT - 1].seek(0);

			for(u16 i = 0; i < S770_OS_BLKS; i++)
			{
				thread_err = streams[STREAM_CNT - 1].write(LOCAL_CLUSTER.get(),
														   S7XX::FS::BLK_SIZE);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 39);
					print_mtx.unlock();
					return (u16)39;
				}
			}

			for(u8 i = 0; i < S7XX::FS::S760_OS_CLUSTERS; i++)
			{
				thread_err = streams[STREAM_CNT - 1].write(LOCAL_CLUSTER.get(),
														   S7XX::AUDIO_SEGMENT_SIZE);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 40);
					print_mtx.unlock();
					return (u16)40;
				}
			}

			return (u16)0;
		}
	);

	//Write samples
	for(u8 i = 0; i < STREAM_CNT - 1; i++)
	{
		threads[i] = std::thread
		(
			[i, &streams, &print_mtx]()
			{
				const u8 VAL = i + 1;
				u16 thread_err;

				std::unique_ptr<u8[]> LOCAL_CLUSTER;

				LOCAL_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
				std::memset(LOCAL_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

				streams[i].seek(16);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].write(LOCAL_CLUSTER.get(),
												  S7XX::AUDIO_SEGMENT_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 41);
						print_mtx.unlock();
						return (u16)41;
					}
				}

				return (u16)0;
			}
		);
	}

	for(u8 i = 0; i < STREAM_CNT; i++) threads[i].join();

	cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	expected_cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

	//Check samples
	for (u8 i = 0; i < STREAM_CNT - 1; i++)
	{
		dentries.clear();
		err = fs->list((std::string(BASE_FNAME) +
			std::to_string(i)).c_str(), dentries);
		if(err)
		{
			print_unexpected_err(err, 24);
			return 24;
		}

		const u32 expected_file_size =
		S7XX::FS::On_disk_sizes::SAMPLE_PARAMS_ENTRY
		+ S7XX::AUDIO_SEGMENT_SIZE * (i + 1 + BASE_CLS_CNT);

		if(dentries[0].fsize != expected_file_size)
		{
			std::cerr << "File size mismatch!!!" << std::endl;
			std::cerr << "Expected: " << expected_file_size << std::endl;
			std::cerr << "Got: " << dentries[0].fsize << std::endl;
			std::cerr << "Exit: 25" << std::endl;
			return 25;
		}

		std::memset(expected_cluster.get(), i + 1, S7XX::AUDIO_SEGMENT_SIZE);

		for(u8 j = 0; j < i + 1 + BASE_CLS_CNT; j++)
		{
			streams[i].seek(16 + j * S7XX::AUDIO_SEGMENT_SIZE);
			streams[i].read(cluster.get(), S7XX::AUDIO_SEGMENT_SIZE);

			if(std::memcmp(cluster.get(), expected_cluster.get(),
				S7XX::AUDIO_SEGMENT_SIZE))
			{
				std::cerr << "File data mismatch!!!" << std::endl;
				print_buffers(expected_cluster.get(), cluster.get(),
							  S7XX::AUDIO_SEGMENT_SIZE);
				std::cerr << "Exit: 26" << std::endl;
				return 26;
			}
		}
	}

	//Check OS
	dentries.clear();
	err = fs->list("/OS", dentries);
	if(err)
	{
		print_unexpected_err(err, 27);
		return 27;
	}

	const u32 expected_file_size = S7XX::FS::On_disk_sizes::OS
		+ S7XX::FS::On_disk_sizes::S760_EXT_OS;

	if(dentries[0].fsize != expected_file_size)
	{
		std::cerr << "File size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_file_size << std::endl;
		std::cerr << "Got: " << dentries[0].fsize << std::endl;
		std::cerr << "Exit: 28" << std::endl;
		return 28;
	}

	std::memset(expected_cluster.get(), STREAM_CNT, S7XX::AUDIO_SEGMENT_SIZE);
	streams[STREAM_CNT - 1].seek(16);

	for(u16 i = 0; i < S770_OS_BLKS; i++)
	{
		streams[STREAM_CNT - 1].read(cluster.get(), S7XX::FS::BLK_SIZE);

		if(std::memcmp(cluster.get(), expected_cluster.get(),
			S7XX::FS::BLK_SIZE))
		{
			std::cerr << "File data mismatch!!!" << std::endl;
			print_buffers(expected_cluster.get(), cluster.get(),
						  S7XX::FS::BLK_SIZE);
			std::cerr << "Exit: 29" << std::endl;
			return 29;
		}
	}

	for(u8 i = 0; i < S7XX::FS::S760_OS_CLUSTERS; i++)
	{
		streams[STREAM_CNT - 1].read(cluster.get(), S7XX::AUDIO_SEGMENT_SIZE);

		if(std::memcmp(cluster.get(), expected_cluster.get(),
			S7XX::AUDIO_SEGMENT_SIZE))
		{
			std::cerr << "File data mismatch!!!" << std::endl;
			print_buffers(expected_cluster.get(), cluster.get(),
						  S7XX::AUDIO_SEGMENT_SIZE);
			std::cerr << "Exit: 30" << std::endl;
			return 30;
		}
	}

	//Read OS
	threads[STREAM_CNT - 1] = std::thread
	(
		[STREAM_CNT, &streams, &print_mtx]()
		{
			const u8 VAL = STREAM_CNT;
			u16 thread_err;

			std::unique_ptr<u8[]> LOCAL_EXPECTED_CLUSTER, local_cluster;

			LOCAL_EXPECTED_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
			std::memset(LOCAL_EXPECTED_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

			local_cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

			streams[STREAM_CNT - 1].seek(0);

			for(u16 i = 0; i < S770_OS_BLKS; i++)
			{
				thread_err = streams[STREAM_CNT - 1].read(local_cluster.get(),
														   S7XX::FS::BLK_SIZE);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 42);
					print_mtx.unlock();
					return (u16)42;
				}

				if(std::memcmp(local_cluster.get(),
					LOCAL_EXPECTED_CLUSTER.get(), S7XX::FS::BLK_SIZE))
				{
					print_mtx.lock();
					std::cerr << "OS data mismatch!!!" << std::endl;
					std::cerr << "Exit: " << (u16)43 << std::endl;
					print_mtx.unlock();
					return (u16)43;
				}
			}

			for(u8 i = 0; i < S7XX::FS::S760_OS_CLUSTERS; i++)
			{
				thread_err = streams[STREAM_CNT - 1].read(local_cluster.get(),
														   S7XX::AUDIO_SEGMENT_SIZE);
				if(thread_err)
				{
					print_mtx.lock();
					print_unexpected_err(thread_err, 44);
					print_mtx.unlock();
					return (u16)44;
				}

				if(std::memcmp(local_cluster.get(),
					LOCAL_EXPECTED_CLUSTER.get(), S7XX::AUDIO_SEGMENT_SIZE))
				{
					print_mtx.lock();
					std::cerr << "S-760 OS data mismatch!!!" << std::endl;
					std::cerr << "Exit: " << (u16)45 << std::endl;
					print_mtx.unlock();
					return (u16)45;
				}
			}

			return (u16)0;
		}
	);

	//Read samples
	for(u8 i = 0; i < STREAM_CNT - 1; i++)
	{
		threads[i] = std::thread
		(
			[i, &streams, &print_mtx]()
			{
				const u8 VAL = i + 1;
				u16 thread_err;

				std::unique_ptr<u8[]> LOCAL_EXPECTED_CLUSTER, local_cluster;

				LOCAL_EXPECTED_CLUSTER = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
				std::memset(LOCAL_EXPECTED_CLUSTER.get(), VAL, S7XX::AUDIO_SEGMENT_SIZE);

				local_cluster = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

				streams[i].seek(16);

				for(u8 j = 0; j < VAL + BASE_CLS_CNT; j++)
				{
					thread_err = streams[i].read(local_cluster.get(),
												  S7XX::AUDIO_SEGMENT_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 46);
						print_mtx.unlock();
						return (u16)46;
					}

					if(std::memcmp(local_cluster.get(),
						LOCAL_EXPECTED_CLUSTER.get(), S7XX::AUDIO_SEGMENT_SIZE))
					{
						print_mtx.lock();
						std::cerr << "File data mismatch!!!" << std::endl;
						std::cerr << "Thread " << (u16)i << std::endl;
						std::cerr << "Exit: 47" << std::endl;
						print_mtx.unlock();
						return (u16)47;
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
	err = S7XX::FS::fsck(fs->path, fsck_status);
	if(err)
	{
		print_unexpected_err(err, 31);
		return 31;
	}

	if(fsck_status)
	{
		std::cerr << "fsck found errors!!!" << std::endl;
		print_fsck_status(fsck_status);
		std::cerr << "Exit: 32" << std::endl;
		return 32;
	}
	/*---------------------------End of safety fsck---------------------------*/

	return 0;
}

int main()
{
	u16 err;

	std::cout << "Write/read separate samples..." << std::endl;
	err = write_separate_samples();
	if(err) return err;
	std::cout << "Write/read separate samples OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write same sample..." << std::endl;
	err = same_sample();
	if(err) return err;
	std::cout << "Write same sample OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write/read separate patches..." << std::endl;
	err = write_separate_patches();
	if(err) return err;
	std::cout << "Write/read separate patches OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write/read OS and samples..." << std::endl;
	err = write_OS_and_samples();
	if(err) return err;
	std::cout << "Write/read OS and samples OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;
	return 0;
}
