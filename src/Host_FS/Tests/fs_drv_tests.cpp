#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/testing_helpers.hpp"
#include "Host_FS/host_drv.hpp"
#include "Utils/utils.hpp"
#include "min_vfs/min_vfs_base.hpp"

int fopen_fclose_tests(void)
{
	constexpr u8 STRM_CNT = 7;
	constexpr char TEST_FNAME_1[] = "test_file_1",
		TEST_FNAME_2[] = "test_file_2";

	const std::filesystem::path TEST_ABS_FPATH_1 =
		std::filesystem::absolute(TEST_FNAME_1), TEST_ABS_FPATH_2 =
		std::filesystem::absolute(TEST_FNAME_2);

	u16 err;

	std::unique_ptr<Host::FS::filesystem_t> fs =
		std::make_unique<Host::FS::filesystem_t>();
	Host::FS::filesystem_t::file_map_iterator_t fmap_it;

	std::vector<min_vfs::stream_t> streams(STRM_CNT);

	if(std::filesystem::exists(TEST_FNAME_1))
		std::filesystem::remove(TEST_FNAME_1);

	if(std::filesystem::exists(TEST_FNAME_2))
		std::filesystem::remove(TEST_FNAME_2);

	//Open first file
	err = fs->fopen(TEST_FNAME_1, streams[0]);
	if(err)
	{
		print_unexpected_err(err, 1);
		return 1;
	}

	if(fs->file_map.size() != 1)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 2 << std::endl;
		return 2;
	}

	fmap_it = fs->file_map.find(TEST_ABS_FPATH_1.string());

	if(fmap_it == fs->file_map.end())
	{
		std::vector<Host::FS::filesystem_t::file_map_t::key_type> keys;
		keys.reserve(fs->file_map.size());

		for(Host::FS::filesystem_t::file_map_t::value_type &kv: fs->file_map)
			keys.push_back(kv.first);

		std::cerr << "File map does not contain entry with key ";
		std::cerr << TEST_ABS_FPATH_1 << std::endl;
		std::cerr << "Current key is: " << keys[0] << std::endl;
		std::cerr << "Exit: " << 3 << std::endl;
		return 3;
	}

	if(fmap_it->second)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 4 << std::endl;
		return 4;
	}

	if(fs->streams.size() != 1)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 5 << std::endl;
		return 5;
	}

	//Open more streams to same file
	for(u8 i = 0; i < STRM_CNT - 1; i++)
	{
		err = fs->fopen(TEST_FNAME_1, streams[i + 1]);
		if(err)
		{
			print_unexpected_err(err, 6);
			return 6;
		}

		if(fs->file_map.size() != 1)
		{
			std::vector<Host::FS::filesystem_t::file_map_t::key_type> keys;
			keys.reserve(fs->file_map.size());

			std::cerr << "File map size mismatch!!!" << std::endl;
			std::cerr << "Expected: " << 2 + i << std::endl;
			std::cerr << "Got: " << fs->file_map.size() << std::endl;
			std::cerr << "Keys:" << std::endl;

			for(Host::FS::filesystem_t::file_map_t::value_type &kv: fs->file_map)
				std::cerr << "\t" << kv.first << std::endl;

			std::cerr << "Exit: " << 7 << std::endl;
			return 7;
		}

		if(fmap_it->second != 1 + i)
		{
			std::cerr << "Bad refcount!!!" << std::endl;
			std::cerr << "Expected: " << 1 + i << std::endl;
			std::cerr << "Got: " << fmap_it->second << std::endl;
			std::cerr << "Exit: " << 8 << std::endl;
			return 8;
		}

		if(fs->streams.size() != 2 + i)
		{
			std::cerr << "Bad stream count!!!" << std::endl;
			std::cerr << "Expected: " << 2 + i << std::endl;
			std::cerr << "Got: " << fs->streams.size() << std::endl;
			std::cerr << "Exit: " << 9 << std::endl;
			return 9;
		}
	}

	//Close the first stream
	err = streams[0].close();
	if(err)
	{
		print_unexpected_err(err, 10);
		return 10;
	}

	if(fs->file_map.size() != 1)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 11 << std::endl;
		return 11;
	}

	if(fmap_it->second != STRM_CNT - 2)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 2 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 12 << std::endl;
		return 12;
	}

	if(fs->streams.size() != STRM_CNT - 1)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 1 << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 13 << std::endl;
		return 13;
	}

	//Close a second stream (the last one)
	err = streams[STRM_CNT - 1].close();
	if(err)
	{
		print_unexpected_err(err, 14);
		return 14;
	}

	if(fs->file_map.size() != 1)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 15 << std::endl;
		return 15;
	}

	if(fmap_it->second != STRM_CNT - 3)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 3 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 16 << std::endl;
		return 16;
	}

	if(fs->streams.size() != STRM_CNT - 2)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 2 << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 17 << std::endl;
		return 17;
	}

	//Open the second file
	err = fs->fopen(TEST_FNAME_2, streams[0]);
	if(err)
	{
		print_unexpected_err(err, 18);
		return 18;
	}

	if(fs->file_map.size() != 2)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 19 << std::endl;
		return 19;
	}

	fmap_it = fs->file_map.find(TEST_ABS_FPATH_2.string());

	if(fmap_it == fs->file_map.end())
	{
		std::vector<Host::FS::filesystem_t::file_map_t::key_type> keys;
		keys.reserve(fs->file_map.size());

		for(Host::FS::filesystem_t::file_map_t::value_type &kv: fs->file_map)
			keys.push_back(kv.first);

		std::cerr << "File map does not contain entry with key ";
		std::cerr << TEST_ABS_FPATH_2 << std::endl;
		std::cerr << "Current key is: " << keys[0] << std::endl;
		std::cerr << "Exit: " << 20 << std::endl;
		return 20;
	}

	if(fmap_it->second)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 21 << std::endl;
		return 21;
	}

	if(fs->streams.size() != STRM_CNT - 1)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 1 << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 22 << std::endl;
		return 22;
	}

	//Open a second stream to the second file
	err = fs->fopen(TEST_FNAME_2, streams[STRM_CNT - 1]);
	if(err)
	{
		print_unexpected_err(err, 23);
		return 23;
	}

	if(fs->file_map.size() != 2)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 24 << std::endl;
		return 24;
	}

	fmap_it = fs->file_map.find(TEST_ABS_FPATH_2.string());

	if(fmap_it == fs->file_map.end())
	{
		std::vector<Host::FS::filesystem_t::file_map_t::key_type> keys;
		keys.reserve(fs->file_map.size());

		for(Host::FS::filesystem_t::file_map_t::value_type &kv: fs->file_map)
			keys.push_back(kv.first);

		std::cerr << "File map does not contain entry with key ";
		std::cerr << TEST_ABS_FPATH_2 << std::endl;
		std::cerr << "Current key is: " << keys[0] << std::endl;
		std::cerr << "Exit: " << 25 << std::endl;
		return 25;
	}

	if(fmap_it->second != 1)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << 1 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 26 << std::endl;
		return 26;
	}

	if(fs->streams.size() != STRM_CNT)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 27 << std::endl;
		return 27;
	}

	//Close the first stream again
	err = streams[0].close();
	if(err)
	{
		print_unexpected_err(err, 28);
		return 28;
	}

	if(fs->file_map.size() != 2)
	{
		std::cerr << "File map size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << 2 << std::endl;
		std::cerr << "Got: " << fs->file_map.size() << std::endl;
		std::cerr << "Exit: " << 29 << std::endl;
		return 29;
	}

	if(fmap_it->second)
	{
		std::cerr << "Bad refcount!!!" << std::endl;
		std::cerr << "Expected: " << 0 << std::endl;
		std::cerr << "Got: " << fmap_it->second << std::endl;
		std::cerr << "Exit: " << 30 << std::endl;
		return 30;
	}

	if(fs->streams.size() != STRM_CNT - 1)
	{
		std::cerr << "Bad stream count!!!" << std::endl;
		std::cerr << "Expected: " << STRM_CNT - 1 << std::endl;
		std::cerr << "Got: " << fs->streams.size() << std::endl;
		std::cerr << "Exit: " << 31 << std::endl;
		return 31;
	}

	constexpr u16 EXPECTED_ERR = ret_val_setup(min_vfs::LIBRARY_ID,
											   (u8)min_vfs::ERR::INVALID_STATE);

	for(u8 i = 0; i < STRM_CNT; i++)
	{
		err = streams[i].close();
		if(i && err)
		{
			print_unexpected_err(err, 32);
			return 32;
		}
		else if(!i && err != EXPECTED_ERR)
		{
			print_expected_err(EXPECTED_ERR, err, 33);
			return 33;
		}
	}

	return 0;
}

int read_tests(void)
{
	constexpr u16 BUFF_SIZE = 512;
	constexpr char TEST_FNAME_1[] = "read_test_file_1";

	u16 err;

	std::unique_ptr<u8[]> host_buf, drv_buf;

	std::fstream fstr;

	std::unique_ptr<Host::FS::filesystem_t> fs =
		std::make_unique<Host::FS::filesystem_t>();

	min_vfs::stream_t stream;
	Host::FS::filesystem_t::file_map_iterator_t fmap_it;

	const uintmax_t fsize = std::filesystem::file_size(TEST_FNAME_1);

	fstr.open(TEST_FNAME_1, std::ios_base::binary | std::ios_base::in |
		std::ios_base::out);

	if(!fstr.is_open() || !fstr.good())
	{
		std::cerr << "Failed to open native stream!!!" << std::endl;
		std::cerr << "Exit: " << 34 << std::endl;
		return 34;
	}

	err = fs->fopen(TEST_FNAME_1, stream);
	if(err)
	{
		print_unexpected_err(err, 35);
		return 35;
	}

	drv_buf = std::make_unique<u8[]>(BUFF_SIZE);
	host_buf = std::make_unique<u8[]>(BUFF_SIZE);

	err = stream.read(drv_buf.get(), BUFF_SIZE);
	if(err)
	{
		print_unexpected_err(err, 36);
		return 36;
	}

	if(stream.get_pos() != BUFF_SIZE)
	{
		std::cerr << "Bad stream pos!!!" << std::endl;
		std::cerr << "Expected: " << BUFF_SIZE << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: " << 37 << std::endl;
		return 37;
	}

	fstr.read((char*)host_buf.get(), BUFF_SIZE);
	if(!fstr.good())
	{
		std::cerr << "Native stream read error!!!" << std::endl;
		std::cerr << "Exit: " << 38 << std::endl;
		return 38;
	}

	if(std::memcmp(drv_buf.get(), host_buf.get(), BUFF_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		print_buffers(host_buf.get(), drv_buf.get(), BUFF_SIZE);
		std::cerr << "Exit: " << 39 << std::endl;
		return 39;
	}

	const u16 off = 345;

	err = stream.seek(fsize - off);
	if(err)
	{
		print_unexpected_err(err, 40);
		return 40;
	}

	fstr.seekg(fsize - off);

	const u16 expected_err = ret_val_setup(min_vfs::LIBRARY_ID,
										   (u8)min_vfs::ERR::END_OF_FILE);
	err = stream.read(drv_buf.get(), BUFF_SIZE);
	if(err != expected_err)
	{
		print_expected_err(expected_err, err, 41);
		return 41;
	}

	if(stream.get_pos() != fsize)
	{
		std::cerr << "Bad stream pos!!!" << std::endl;
		std::cerr << "Expected: " << fsize << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: " << 42 << std::endl;
		return 42;
	}

	fstr.read((char*)host_buf.get(), BUFF_SIZE);
	if(!fstr.eof())
	{
		std::cerr << "Unexpected native stream state!!!" << std::endl;
		std::cerr << "Expected: EOF" << std::endl;
		std::cerr << "Got: EOF=" << fstr.eof() << ", badbit=" << fstr.bad();
		std::cerr << ", fail=" << fstr.fail() << std::endl;
		std::cerr << "Exit: " << 43 << std::endl;
		return 44;
	}

	if(std::memcmp(drv_buf.get(), host_buf.get(), BUFF_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		print_buffers(host_buf.get(), drv_buf.get(), BUFF_SIZE);
		std::cerr << "Exit: " << 44 << std::endl;
		return 44;
	}

	return 0;
}

int write_tests(void)
{
	constexpr u16 BUFF_SIZE = 512;
	constexpr char TEST_FNAME_1[] = "write_test_file_1";

	u16 err;

	std::unique_ptr<u8[]> host_buf, drv_buf;

	std::fstream fstr;

	std::unique_ptr<Host::FS::filesystem_t> fs =
		std::make_unique<Host::FS::filesystem_t>();

	min_vfs::stream_t stream;
	Host::FS::filesystem_t::file_map_iterator_t fmap_it;

	fstr.open(TEST_FNAME_1, std::ios_base::binary | std::ios_base::in |
		std::ios_base::out | std::ios_base::trunc);

	if(!fstr.is_open() || !fstr.good())
	{
		std::cerr << "Failed to open native stream!!!" << std::endl;
		std::cerr << "Exit: " << 45 << std::endl;
		return 45;
	}

	err = fs->fopen(TEST_FNAME_1, stream);
	if(err)
	{
		print_unexpected_err(err, 46);
		return 46;
	}

	drv_buf = std::make_unique<u8[]>(BUFF_SIZE);
	host_buf = std::make_unique<u8[]>(BUFF_SIZE);

	std::memset(drv_buf.get(), 0x5A, BUFF_SIZE);

	err = stream.write(drv_buf.get(), BUFF_SIZE);
	if(err)
	{
		print_unexpected_err(err, 47);
		return 47;
	}

	if(stream.get_pos() != BUFF_SIZE)
	{
		std::cerr << "Bad stream pos!!!" << std::endl;
		std::cerr << "Expected: " << BUFF_SIZE << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: " << 48 << std::endl;
		return 48;
	}

	err = stream.flush();
	if(err)
	{
		print_unexpected_err(err, 49);
		return 49;
	}

	fstr.read((char*)host_buf.get(), BUFF_SIZE);
	if(!fstr.good())
	{
		std::cerr << "Native stream read error!!!" << std::endl;
		std::cerr << "Exit: " << 50 << std::endl;
		return 50;
	}

	if(std::memcmp(drv_buf.get(), host_buf.get(), BUFF_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		print_buffers(host_buf.get(), drv_buf.get(), BUFF_SIZE);
		std::cerr << "Exit: " << 51 << std::endl;
		return 51;
	}

	stream.seek(256);
	std::memset(drv_buf.get(), 0xA5, BUFF_SIZE);

	err = stream.write(drv_buf.get(), BUFF_SIZE);
	if(err)
	{
		print_unexpected_err(err, 52);
		return 52;
	}

	if(stream.get_pos() != BUFF_SIZE + 256)
	{
		std::cerr << "Bad stream pos!!!" << std::endl;
		std::cerr << "Expected: " << BUFF_SIZE << std::endl;
		std::cerr << "Got: " << stream.get_pos() << std::endl;
		std::cerr << "Exit: " << 53 << std::endl;
		return 53;
	}

	err = stream.flush();
	if(err)
	{
		print_unexpected_err(err, 54);
		return 54;
	}

	fstr.seekg(0);
	fstr.read((char*)host_buf.get(), BUFF_SIZE);
	if(!fstr.good())
	{
		std::cerr << "Native stream read error!!!" << std::endl;
		std::cerr << "Exit: " << 55 << std::endl;
		return 55;
	}

	std::memset(drv_buf.get(), 0x5A, 256);

	if(std::memcmp(drv_buf.get(), host_buf.get(), BUFF_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		print_buffers(host_buf.get(), drv_buf.get(), BUFF_SIZE);
		std::cerr << "Exit: " << 56 << std::endl;
		return 56;
	}

	fstr.seekg(BUFF_SIZE - 256);
	fstr.read((char*)host_buf.get(), BUFF_SIZE);
	if(!fstr.good())
	{
		std::cerr << "Native stream read error!!!" << std::endl;
		std::cerr << "Exit: " << 57 << std::endl;
		return 57;
	}

	std::memset(drv_buf.get(), 0xA5, 256);

	if(std::memcmp(drv_buf.get(), host_buf.get(), BUFF_SIZE))
	{
		std::cerr << "Data mismatch!!!" << std::endl;
		print_buffers(host_buf.get(), drv_buf.get(), BUFF_SIZE);
		std::cerr << "Exit: " << 58 << std::endl;
		return 58;
	}

	return 0;
}

int main(void)
{
	u16 err;

	std::cout << "fopen/fclose tests..." << std::endl;
	err = fopen_fclose_tests();
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

	std::cout << "ALL TESTS OK!" << std::endl;

	return 0;
}
