#include <memory>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S5XX/S5XX_FS_drv.hpp"
#include "Roland/S5XX/Tests/fs_drv_test_data.hpp"

u16 read_separate_files()
{
	constexpr u8 STREAM_CNT = 8;
	constexpr u16 BLK_SIZE = 512;

	bool borked;
	u16 err, expected_err, rets[STREAM_CNT];
	u32 blk_cnts[STREAM_CNT];

	std::unique_ptr<u8[]> buffer;
	std::fstream host_streams[STREAM_CNT];
	std::thread threads[STREAM_CNT];
	std::mutex print_mtx;

	std::unique_ptr<S5XX::FS::filesystem_t> fs;
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t streams[STREAM_CNT];

	/*--------------------------------FS setup--------------------------------*/
	try
	{
		fs = std::make_unique<S5XX::FS::filesystem_t>(BASE_TEST_FS_FNAME);
	}
	catch(min_vfs::FS_err e)
	{
		print_unexpected_err(e.err_code, 1);
		return 1;
	}
	/*-----------------------------End of FS setup----------------------------*/

	buffer = std::make_unique<u8[]>(BLK_SIZE);

	//Stream and data setup
	for(u8 file_idx = 0; file_idx < STREAM_CNT; file_idx++)
	{
		host_streams[file_idx].open(EXPECTED_SOUND_DIR[file_idx].fname, std::ios_base::binary
			| std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
		if(!host_streams[file_idx].is_open() || !host_streams[file_idx].good())
		{
			std::cerr << "Failed to open host stream for \"";
			std::cerr << EXPECTED_SOUND_DIR[file_idx].fname + "\" (file " << (u16)file_idx;
			std::cerr << ")" << std::endl;
			std::cerr << "Exit: " << 2 << std::endl;
			return 2;
		}

		err = fs->fopen((EXPECTED_ROOT_DIR[1].fname + "/"
			+ EXPECTED_SOUND_DIR[file_idx].fname).c_str(), streams[file_idx]);
		if(err)
		{
			print_unexpected_err(err, 3);
			return 3;
		}

		dentries.clear();
		err = fs->list((EXPECTED_ROOT_DIR[1].fname + "/"
			+ EXPECTED_SOUND_DIR[file_idx].fname).c_str(), dentries);
		if(err)
		{
			print_unexpected_err(err, 4);
			return 4;
		}

		blk_cnts[file_idx] = dentries[0].fsize / BLK_SIZE;

		for(u32 j = 0; j < blk_cnts[file_idx]; j++)
		{
			err = streams[file_idx].read(buffer.get(), BLK_SIZE);
			if(err)
			{
				std::cerr << "Failed read for file ";
				std::cerr << EXPECTED_SOUND_DIR[file_idx].fname + " (file " << (u16)file_idx;
				std::cerr << ")" << std::endl;
				print_unexpected_err(err, 5);
				return 5;
			}

			host_streams[file_idx].write((char*)buffer.get(), BLK_SIZE);
			if(!host_streams[file_idx].good())
			{
				std::cerr << "Failed write for file ";
				std::cerr << EXPECTED_SOUND_DIR[file_idx].fname + " (file " << (u16)file_idx;
				std::cerr << ")" << std::endl;
				std::cerr << "Exit: " << 6 << std::endl;
				return 6;
			}
		}
	}

	//Read
	for(u8 file_idx = 0; file_idx < STREAM_CNT; file_idx++)
	{
		threads[file_idx] = std::thread
		(
			[file_idx, &host_streams, &streams, &blk_cnts, &print_mtx, &rets]()
			{
				u16 thread_err;

				std::unique_ptr<u8[]> local_expected_cluster, local_cluster;

				local_expected_cluster = std::make_unique<u8[]>(BLK_SIZE);
				local_cluster = std::make_unique<u8[]>(BLK_SIZE);

				host_streams[file_idx].seekg(0);
				streams[file_idx].seek(0);

				for(u32 j = 0; j < blk_cnts[file_idx]; j++)
				{
					host_streams[file_idx].read((char*)local_expected_cluster.get(),
										 BLK_SIZE);
					if(!host_streams[file_idx].good())
					{
						print_mtx.lock();
						std::cerr << "Failed read from file ";
						std::cerr << EXPECTED_SOUND_DIR[file_idx].fname + " (file ";
						std::cerr << (u16)file_idx << ")" << std::endl;
						std::cerr << "Exit: " << 7 << std::endl;
						print_mtx.unlock();
						rets[file_idx] = 7;
						return (u16)7;
					}

					thread_err = streams[file_idx].read(local_cluster.get(), BLK_SIZE);
					if(thread_err)
					{
						print_mtx.lock();
						print_unexpected_err(thread_err, 8);
						print_mtx.unlock();
						rets[file_idx] = 8;
						return (u16)8;
					}

					if(std::memcmp(local_cluster.get(),
						local_expected_cluster.get(), BLK_SIZE))
					{
						print_mtx.lock();
						std::cerr << "File data mismatch!!!" << std::endl;
						std::cerr << "Thread " << (u16)file_idx << std::endl;
						std::cerr << "Exit: " << 9 << std::endl;
						print_mtx.unlock();
						rets[file_idx] = 9;
						return (u16)9;
					}
				}

				rets[file_idx] = 0;
				return (u16)0;
			}
		);
	}

	borked = false;

	for(u8 i = 0; i < STREAM_CNT; i++)
	{
		threads[i].join();
		err = streams[i].close();
		host_streams[i].close();

		if(err)
		{
			print_unexpected_err(err, 10);
			borked = true;
		}
	}

	if(borked) return 10;

	for(u8 i = 0; i < STREAM_CNT; i++) if(rets[i]) return rets[i];

	return 0;
}

int main()
{
	u16 err;

	std::cout << "Read separate files..." << std::endl;
	err = read_separate_files();
	if(err) return err;
	std::cout << "Read separate files OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;
	return 0;
}
