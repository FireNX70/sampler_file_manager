#include <string_view>
#include <filesystem>
#include <iostream>
#include <cstring>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "Roland/S7XX/host_FS_utils.hpp"
#include "Host_FS_test_data.hpp"
#include "Helpers.hpp"

constexpr char OG_FILENAME[] = "TST:Test_01   \x7FL";

static int find_from_index_tests()
{
	u16 expected_err, err;
	std::string expected_file_path;
	std::filesystem::path dir_path, file_path;

	expected_err = ret_val_setup(S7XX::Host_FS::LIBRARY_ID, (u8)S7XX::Host_FS::ERR::INVALID_PATH);
	err = S7XX::Host_FS::find_from_index("nx_dir", 0, file_path);
	if(err != expected_err)
	{
		std::cerr << "Error mismatch!!!" << std::endl;
		std::cerr << "\tExpected 0x" << std::hex << expected_err << std::endl;
		std::cerr << "\tGot 0x" << err << std::dec << std::endl;
		return 2;
	}

	dir_path = HOST_FS_TEST_DATA::REF_HOST_FS_PATH;
	dir_path /= "Samples/";

	expected_err = ret_val_setup(S7XX::Host_FS::LIBRARY_ID, (u8)S7XX::Host_FS::ERR::NOT_FOUND);
	err = S7XX::Host_FS::find_from_index(dir_path, 456, file_path);
	if(err != expected_err)
	{
		std::cerr << "Error mismatch!!!" << std::endl;
		std::cerr << "\tExpected 0x" << std::hex << expected_err << std::endl;
		std::cerr << "\tGot 0x" << err << std::dec << std::endl;
		return 3;
	}

	expected_file_path = dir_path.string(); 
	expected_file_path += HOST_FS_TEST_DATA::EXPECTED_SAMPLE_NAMES[0];

	expected_err = 0;
	err = S7XX::Host_FS::find_from_index(dir_path, 0, file_path);
	if(err)
	{
		std::cerr << "Unexpected error 0x" << std::hex << err << std::dec << "!!!" << std::endl;
		return 4;
	}

	if(file_path.string() != expected_file_path)
	{
		std::cerr << "File path mismatch!!!" << std::endl;
		std::cerr << "\tExpected: " << expected_file_path << std::endl;
		std::cerr << "\tGot: " << file_path << std::endl;
		return 5;
	}

	dir_path = HOST_FS_TEST_DATA::REF_HOST_FS_PATH;
	dir_path /= "Patches/";

	expected_file_path = dir_path.string();
	expected_file_path += HOST_FS_TEST_DATA::EXPECTED_PATCH_NAMES[4];

	expected_err = 0;
	err = S7XX::Host_FS::find_from_index(dir_path, 4, file_path);
	if(err)
	{
		std::cerr << "Unexpected error 0x" << std::hex << err << std::dec << "!!!" << std::endl;
		return 6;
	}

	if(file_path.compare(expected_file_path))
	{
		std::cerr << "File path mismatch!!!" << std::endl;
		std::cerr << "\tExpected: " << expected_file_path << std::endl;
		std::cerr << "\tGot: " << file_path << std::endl;
		return 7;
	}

	return 0;
}

static int sanitize_filename_tests()
{
	std::string test_str = OG_FILENAME;
	S7XX::Host_FS::sanitize_filename(test_str);

	test_str = std::string("0-") + test_str;

	if(test_str != HOST_FS_TEST_DATA::EXPECTED_SAMPLE_NAMES[0])
	{
		std::cerr << "Sanitize filename mismatch!!!" << std::endl;
		std::cerr << "Expected: " << HOST_FS_TEST_DATA::EXPECTED_SAMPLE_NAMES[0] << std::endl;
		std::cerr << "Got: " << test_str << std::endl;
		return 9;
	}

	test_str = std::string("0-") + OG_FILENAME;
	S7XX::Host_FS::sanitize_filename(test_str);

	if(test_str != HOST_FS_TEST_DATA::EXPECTED_SAMPLE_NAMES[0])
	{
		std::cerr << "Sanitize filename mismatch!!!" << std::endl;
		std::cerr << "Expected: " << HOST_FS_TEST_DATA::EXPECTED_SAMPLE_NAMES[0] << std::endl;
		std::cerr << "Got: " << test_str << std::endl;
		return 10;
	}

	return 0;
}

int main()
{
	u16 err;

	if(!std::filesystem::exists(HOST_FS_TEST_DATA::REF_HOST_FS_PATH))
	{
		std::cerr << "Ref host FS is missing!!!" << std::endl;
		return 1;
	}

	std::cout << "Sanitized name regex test..." << std::endl;
	err = std::regex_match("1-ABC-TEST", S7XX::Host_FS::S7XX_HOST_FNAME_REGEX);
	if(!err)
	{
		std::cerr << "Sanitized fname regex match failed!!!" << std::endl;
		return 8;
	}
	std::cout << "Sanitized name regex OK!" << std::endl;

	std::cout << "Sanitize fname test..." << std::endl;
	err = sanitize_filename_tests();
	if(err)
	{
		std::cerr << "Sanitize fname tests failed!!!" << std::endl;
		std::cerr << "Err " << err << std::endl;
		return err;
	}
	std::cout << "Sanitize fname regex OK!" << std::endl;

	std::cout << "Find from index tests..." << std::endl;
	err = find_from_index_tests();
	if(err)
	{
		std::cerr << "Find from index tests failed!!!" << std::endl;
		std::cerr << "Err " << err << std::endl;
		return err;
	}
	std::cout << "Find from index tests OK!" << std::endl;

	return 0;
}
