#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>

#include "Utils/ints.hpp"
#include "E-MU/EMU_FS_types.hpp"
#include "fs_test_data.hpp"
#include "E-MU/fs_common.cpp"
#include "E-MU/EMU_FS_drv.cpp"
#include "Utils/testing_helpers.hpp"
#include "Helpers.hpp"

using namespace EMU::FS::testing;

template<typename... Types>
static std::string tuple_to_string(const std::tuple<Types...> tuple)
{
	/*Mostly lifted from cppreference's std::apply page. Lambdas are no problem,
	but a variadic call on a lambda within a variadic lambda within a variadic
	template...*/

	std::stringstream res;

	std::apply
	(
		[&res](const Types... element)
		{
			size_t n = 0;
			((res << element << (++n != sizeof...(Types) ? ", " : "")), ...);
		}, tuple
	);

	return res.str();
}

static int file_size_to_counts_tests()
{
	std::tuple<u16, u16, u16> counts, expected_counts;

	expected_counts = {0, 0, 0};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, 0);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 1" << std::endl;
		return 1;
	}

	expected_counts = {1, 1, 1};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, 1);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 2" << std::endl;
		return 2;
	}

	expected_counts = {1, 1, EMU::FS::BLK_SIZE};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, EMU::FS::BLK_SIZE);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 3" << std::endl;
		return 3;
	}

	expected_counts = {1, CLUSTER_SIZE / EMU::FS::BLK_SIZE, EMU::FS::BLK_SIZE};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, CLUSTER_SIZE);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 4" << std::endl;
		return 4;
	}

	expected_counts = {2, 1, 1};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, CLUSTER_SIZE + 1);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 5" << std::endl;
		return 5;
	}

	expected_counts = {2, 1, EMU::FS::BLK_SIZE};
	counts = EMU::FS::file_size_to_counts(CLUSTER_SIZE, CLUSTER_SIZE + EMU::FS::BLK_SIZE);

	if(counts != expected_counts)
	{
		std::cerr << "Counts mismatch!!!" << std::endl;
		std::cerr << "Expected: " << tuple_to_string(expected_counts) << std::endl;
		std::cerr << "Got: " << tuple_to_string(counts) << std::endl;
		std::cerr << "Exit: 6" << std::endl;
		return 6;
	}

	return 0;
}

static int calc_file_size()
{
	uintmax_t file_size, expected_size;

	expected_size = CLUSTER_SIZE + EMU::FS::BLK_SIZE;
	file_size = EMU::FS::calc_file_size({.cluster_cnt = 2, .block_cnt = 1, .byte_cnt = EMU::FS::BLK_SIZE}, CLUSTER_SIZE);
	if(file_size != expected_size)
	{
		std::cerr << "Size mismatch!!!" << std::endl;
		std::cerr << "Expected: " << expected_size << std::endl;
		std::cerr << "Got: " << file_size << std::endl;
		std::cerr << "Exit: 7" << std::endl;
		return 7;
	}

	return 0;
}

constexpr char TEST_DIR_FILENAME[] = "test_dir_entry.bin";
constexpr EMU::FS::Dir_t EXPECTED_DIR =
{
	.name = "Test dir\\1\0\0\0\0\0\0",
	.type = EMU::FS::Dir_type_e::NORMAL,
	.blocks = {1, 2, 3, 4, 5, 6, 7},
	.addr = 0,
};

constexpr char EXPECTED_DIR_DATA[EMU::FS::On_disk_sizes::DIR_ENTRY] =
{
	'T', 'e', 's', 't', ' ', 'd', 'i', 'r', '/', '1', 0, 0, 0, 0, 0, 0,
	0, (char)EXPECTED_DIR.type, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0
};

static int write_dir()
{
	char data[EMU::FS::On_disk_sizes::DIR_ENTRY], expected_data[EMU::FS::On_disk_sizes::DIR_ENTRY];
	std::fstream fstr;

	std::memcpy(expected_data, EXPECTED_DIR_DATA, sizeof(EXPECTED_DIR_DATA));
	expected_data[8] = '\\';

	std::memset(data, 0, EMU::FS::On_disk_sizes::DIR_ENTRY);
	write_dir((u8*)data, EXPECTED_DIR);

	if(std::memcmp(data, expected_data, EMU::FS::On_disk_sizes::DIR_ENTRY))
	{
		std::cerr << "Dir data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_buffer(expected_data, EMU::FS::On_disk_sizes::DIR_ENTRY);
		std::cerr << "Got: " << std::endl;
		print_buffer(data, EMU::FS::On_disk_sizes::DIR_ENTRY);
		std::cerr << "Exit: 8" << std::endl;
		return 8;
	}

	fstr.open(TEST_DIR_FILENAME, std::ios_base::binary | std::ios_base::trunc
	| std::ios_base::in | std::ios_base::out);
	write_dir(fstr, EXPECTED_DIR);
	fstr.read(data, EMU::FS::On_disk_sizes::DIR_ENTRY);

	if(std::memcmp(data, expected_data, EMU::FS::On_disk_sizes::DIR_ENTRY))
	{
		std::cerr << "Dir data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_buffer(expected_data, EMU::FS::On_disk_sizes::DIR_ENTRY);
		std::cerr << "Got: " << std::endl;
		print_buffer(data, EMU::FS::On_disk_sizes::DIR_ENTRY);
		std::cerr << "Exit: 9" << std::endl;
		return 9;
	}

	return 0;
}

static int load_dir()
{
	EMU::FS::Dir_t test_dir;
	std::fstream fstr;

	fstr.open(TEST_DIR_FILENAME, std::ios_base::binary | std::ios_base::out);
	fstr.write(EXPECTED_DIR_DATA, sizeof(EXPECTED_DIR_DATA));
	fstr.close();

	load_dir<true>((u8*)EXPECTED_DIR_DATA, test_dir);

	if(dircmp(test_dir, EXPECTED_DIR))
	{
		std::cerr << "Dir data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << dir_to_string(EXPECTED_DIR, 1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dir_to_string(test_dir, 1) << std::endl;
		std::cerr << "Exit: 10" << std::endl;
		return 10;
	}

	fstr.open(TEST_DIR_FILENAME, std::ios_base::binary | std::ios_base::in);
	load_dir(fstr, test_dir);

	if(dircmp(test_dir, EXPECTED_DIR))
	{
		std::cerr << "Dir data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << dir_to_string(EXPECTED_DIR, 1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << dir_to_string(test_dir, 1) << std::endl;
		std::cerr << "Exit: 11" << std::endl;
		return 11;
	}

	return 0;
}

constexpr char TEST_FILE_FILENAME[] = "test_file_entry.bin";
constexpr EMU::FS::File_t EXPECTED_FILE =
{
	.name = "Test file\\1\0\0\0\0\0",
	.bank_num = 127,
	.start_cluster = 64,
	.cluster_cnt = 12,
	.block_cnt = 7,
	.byte_cnt = 5,
	.type = EMU::FS::File_type_e::STD,
	.props = {0, 'E', '4', 'B', '0'},
	.addr = 0,
};

constexpr char EXPECTED_FILE_DATA[EMU::FS::On_disk_sizes::FILE_ENTRY] =
{
	'T', 'e', 's', 't', ' ', 'f', 'i', 'l', 'e', '/', '1', 0, 0, 0, 0, 0,
	0, EXPECTED_FILE.bank_num, 64, 0, 12, 0, 7, 0,
	5, 0, (char)EXPECTED_FILE.type, 0, 'E', '4', 'B', '0'
};

static int write_file()
{
	char data[EMU::FS::On_disk_sizes::FILE_ENTRY], expected_data[EMU::FS::On_disk_sizes::FILE_ENTRY];
	std::fstream fstr;

	std::memcpy(expected_data, EXPECTED_FILE_DATA, sizeof(EXPECTED_FILE_DATA));
	expected_data[9] = '\\';

	std::memset(data, 0, EMU::FS::On_disk_sizes::FILE_ENTRY);
	write_file((u8*)data, EXPECTED_FILE);

	if(std::memcmp(data, expected_data, EMU::FS::On_disk_sizes::FILE_ENTRY))
	{
		std::cerr << "File data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_buffer(expected_data, EMU::FS::On_disk_sizes::FILE_ENTRY);
		std::cerr << "Got: " << std::endl;
		print_buffer(data, EMU::FS::On_disk_sizes::FILE_ENTRY);
		std::cerr << "Exit: 12" << std::endl;
		return 12;
	}

	fstr.open(TEST_FILE_FILENAME, std::ios_base::binary | std::ios_base::trunc
	| std::ios_base::in | std::ios_base::out);
	write_file(fstr, EXPECTED_FILE);
	fstr.read(data, EMU::FS::On_disk_sizes::FILE_ENTRY);

	if(std::memcmp(data, expected_data, EMU::FS::On_disk_sizes::FILE_ENTRY))
	{
		std::cerr << "File data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		print_buffer(expected_data, EMU::FS::On_disk_sizes::FILE_ENTRY);
		std::cerr << "Got: " << std::endl;
		print_buffer(data, EMU::FS::On_disk_sizes::FILE_ENTRY);
		std::cerr << "Exit: 13" << std::endl;
		return 13;
	}

	return 0;
}

static int load_file()
{
	EMU::FS::File_t test_file;
	std::fstream fstr;

	fstr.open(TEST_FILE_FILENAME, std::ios_base::binary | std::ios_base::out);
	fstr.write(EXPECTED_FILE_DATA, sizeof(EXPECTED_FILE_DATA));
	fstr.close();

	load_file<true>((u8*)EXPECTED_FILE_DATA, test_file);

	if(filecmp(test_file, EXPECTED_FILE))
	{
		std::cerr << "File data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << file_to_string(EXPECTED_FILE, 1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << file_to_string(test_file, 1) << std::endl;
		std::cerr << "Exit: 14" << std::endl;
		return 14;
	}

	fstr.open(TEST_FILE_FILENAME, std::ios_base::binary | std::ios_base::in);
	load_file(fstr, test_file);

	if(filecmp(test_file, EXPECTED_FILE))
	{
		std::cerr << "File data mismatch!!!" << std::endl;
		std::cerr << "Expected: " << std::endl;
		std::cerr << file_to_string(EXPECTED_FILE, 1) << std::endl;
		std::cerr << "Got: " << std::endl;
		std::cerr << file_to_string(test_file, 1) << std::endl;
		std::cerr << "Exit: 15" << std::endl;
		return 15;
	}

	return 0;
}

int main()
{
	int err;

	std::cout << "File size to counts tests..." << std::endl;
	err = file_size_to_counts_tests();
	if(err) return err;
	std::cout << "File size to counts OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Calc file size tests..." << std::endl;
	err = calc_file_size();
	if(err) return err;
	std::cout << "Calc file size OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Load dir tests..." << std::endl;
	err = load_dir();
	if(err) return err;
	std::cout << "Load dir OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write dir tests..." << std::endl;
	err = write_dir();
	if(err) return err;
	std::cout << "Write dir OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Load file tests..." << std::endl;
	err = load_file();
	if(err) return err;
	std::cout << "Load file OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Write file tests..." << std::endl;
	err = write_file();
	if(err) return err;
	std::cout << "Write file OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!!!" << std::endl;

	return 0;
}
