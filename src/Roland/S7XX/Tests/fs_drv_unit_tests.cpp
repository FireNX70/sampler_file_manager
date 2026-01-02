#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Utils/ints.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"
#include "Roland/S7XX/S7XX_FS_drv.cpp"
#include "Roland/S7XX/mkfs.cpp"
#include "Roland/S7XX/fs_drv_helpers.cpp"
#include "Roland/S7XX/Tests/Test_data.hpp"
#include "Helpers.cpp"

using namespace S7XX::FS::testing;

static int idx_and_name_from_name_tests()
{
	u16 err;
	std::string test_fname, cleaned_fname;

	test_fname = "1-";
	err = S7XX::FS::idx_and_name_from_name(test_fname, cleaned_fname);
	if(err != 1 || cleaned_fname != "                ")
	{
		std::cerr << "idx: " << err << std::endl;
		std::cerr << "cleaned fname: " << cleaned_fname << std::endl;
		std::cerr << "Exit: 1" << std::endl;
		return 1;
	}

	test_fname = "1-TEST";
	err = S7XX::FS::idx_and_name_from_name(test_fname, cleaned_fname);
	if(err != 1 || cleaned_fname != "TEST            ")
	{
		std::cerr << "idx: " << err << std::endl;
		std::cerr << "cleaned fname: " << cleaned_fname << std::endl;
		std::cerr << "Exit: 2" << std::endl;
		return 2;
	}

	test_fname = "TEST";
	err = S7XX::FS::idx_and_name_from_name(test_fname, cleaned_fname);
	if(err != 0xFFFF || cleaned_fname != "TEST            ")
	{
		std::cerr << "idx: " << err << std::endl;
		std::cerr << "cleaned fname: " << cleaned_fname << std::endl;
		std::cerr << "Exit: 3" << std::endl;
		return 3;
	}

	return 0;
}

static int find_sample_from_name_tests(S7XX::FS::filesystem_t &fs)
{
	u16 err;

	const std::string test_name = NATIVE_FS_DIRS::EXPECTED_SAMPLE_DIR[2].fname.data();
	std::string cleaned_name;
	err = S7XX::FS::idx_and_name_from_name(test_name, cleaned_name);
	if(err == 0xFFFF)
	{
		std::cerr << "uhoh stinky" << std::endl;
		std::cerr << "Exit: 4" << std::endl;
		return 4;
	}

	err = S7XX::FS::find_sample_from_name(fs, cleaned_name.c_str());
	if(err == fs.header.TOC.sample_cnt || err == S7XX::FS::MAX_SAMPLE_COUNT)
	{
		std::cout << err << std::endl;
		std::cerr << "Exit: 5" << std::endl;
		return 5;
	}

	return 0;
}

static int unzero_all_before()
{
	constexpr char TEST_FS_FNAME[] = "unzero_all_before.img";

	u16 err;

	std::fstream test_fs_str(TEST_FS_FNAME, std::fstream::trunc | std::fstream::binary | std::fstream::in | std::fstream::out);

	if(!test_fs_str.is_open() || !test_fs_str.good())
	{
		std::cerr << "Couldn't open/create " << TEST_FS_FNAME << std::endl;
		std::cerr << "Exit: 6" << std::endl;
		return 6;
	}
	test_fs_str.close();
	std::filesystem::resize_file(TEST_FS_FNAME, 80 * 1024 * 1024);

	err = S7XX::FS::mkfs(TEST_FS_FNAME, "UNZERO ALL TEST");
	if(err)
	{
		std::cerr << "mkfs err: " << std::hex << err << std::dec << std::endl;
		std::cerr << "Exit: 7" << std::endl;
		return 7;
	}

	S7XX::FS::filesystem_t s7xx_fs(TEST_FS_FNAME);
	constexpr S7XX::FS::type_attrs_t TYPE_ATTRS = S7XX::FS::TYPE_ATTRS[5];
	char name0;

	s7xx_fs.stream.seekg(TYPE_ATTRS.LIST_ADDR);
	for(u16 i = 0; i < TYPE_ATTRS.MAX_CNT; i++)
	{
		name0 = s7xx_fs.stream.peek();
		if(name0)
		{
			std::cerr << "Non-zero first char in name!!!" << std::endl;
			std::cerr << "idx: " << i << std::endl;
			std::cerr << "Exit: 8" << std::endl;
			return 8;
		}

		s7xx_fs.stream.seekg(S7XX::FS::On_disk_sizes::LIST_ENTRY, std::fstream::cur);
	}

	err = S7XX::FS::unzero_all_before<S7XX::FS::TYPE_ATTRS[5]>(s7xx_fs, 4096);
	if(err)
	{
		std::cerr << "unzero err: " << err << std::endl;
		std::cerr << "Exit: 9" << std::endl;
		return 9;
	}

	s7xx_fs.stream.seekg(TYPE_ATTRS.LIST_ADDR);
	for(u16 i = 0; i < 4096; i++)
	{
		name0 = s7xx_fs.stream.peek();
		if(name0)
		{
			std::cerr << "Non-zero first char in name!!!" << std::endl;
			std::cerr << "idx: " << i << std::endl;
			std::cerr << "Exit: 10" << std::endl;
			return 10;
		}

		s7xx_fs.stream.seekg(S7XX::FS::On_disk_sizes::LIST_ENTRY, std::fstream::cur);
	}

	for(u16 i = 4096; i < TYPE_ATTRS.MAX_CNT; i++)
	{
		name0 = s7xx_fs.stream.peek();
		if(name0)
		{
			std::cerr << "Non-zero first char in name!!!" << std::endl;
			std::cerr << "idx: " << i << std::endl;
			std::cerr << "Exit: 11" << std::endl;
			return 11;
		}

		s7xx_fs.stream.seekg(S7XX::FS::On_disk_sizes::LIST_ENTRY, std::fstream::cur);
	}

	return 0;
}

static int reloc_cluster()
{
	constexpr char test_img_name[] = "reloc_cls_test.img";
	constexpr u16 EXPECTED_NEW_CLS_1 = 368, EXPECTED_NEW_CLS_2 = 369;

	u16 err, cls;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;
	std::unique_ptr<u8[]> buffer1, buffer2;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(test_img_name))
		std::filesystem::remove_all(test_img_name);

	std::filesystem::copy_file(TEST_FS_PATH, test_img_name);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(test_img_name);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 12" << std::endl;
		return 12;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 13" << std::endl;
		return 13;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	/*Can only return min_vfs::ERR::NO_SPACE_LEFT and min_vfs::ERR::IO_ERROR.
	Testing for those would be kind of a PITA and it's not urgent.*/
	/*------------------------End of common err testing-----------------------*/

	/*----------------------Reloc first cluster in chain----------------------*/
	cls = s7xx_fs->FAT[S7XX::FS::FAT_ATTRS.DATA_MIN];

	err = S7XX::FS::reloc_cluster(*s7xx_fs, S7XX::FS::FAT_ATTRS.DATA_MIN, S7XX::FS::S760_OS_CLUSTERS);
	if(err)
	{
		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		std::cerr << "Exit: 14" << std::endl;
		return 14;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 15" << std::endl;
		return 15;
	}

	if(s7xx_fs->FAT[S7XX::FS::FAT_ATTRS.DATA_MIN] != S7XX::FS::FAT_ATTRS.FREE_CLUSTER)
	{
		std::cerr << "Bad cluster value (at old)!!!" << std::endl;
		std::cerr << "Expected free cluster value (";
		std::cerr << S7XX::FS::FAT_ATTRS.FREE_CLUSTER << ")" << std::endl;
		std::cerr << "Got " << s7xx_fs->FAT[S7XX::FS::FAT_ATTRS.DATA_MIN] << std::endl;
		std::cerr << "Exit: 16" << std::endl;
		return 16;
	}

	if(s7xx_fs->FAT[EXPECTED_NEW_CLS_1] != cls)
	{
		std::cerr << "Bad cluster value (at new)!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[EXPECTED_NEW_CLS_1] << std::endl;
		std::cerr << "Exit: 17" << std::endl;
		return 17;
	}

	s7xx_fs->stream.seekg(0xCD81C);
	s7xx_fs->stream.read((char*)&cls, 2);

	if constexpr(S7XX::FS::ENDIANNESS != std::endian::native)
		cls = std::byteswap(cls);

	if(cls != EXPECTED_NEW_CLS_1)
	{
		std::cerr << "Failed to update file entry!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_NEW_CLS_1 << std::endl;
		std::cerr << "Got: " << cls << std::endl;
		std::cerr << "Exit: 18" << std::endl;
		return 18;
	}

	buffer1 = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);
	buffer2 = std::make_unique<u8[]>(S7XX::AUDIO_SEGMENT_SIZE);

	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::AUDIO_SECTION);
	s7xx_fs->stream.read((char*)buffer1.get(), S7XX::AUDIO_SEGMENT_SIZE);

	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::AUDIO_SECTION + (EXPECTED_NEW_CLS_1 - S7XX::FS::FAT_ATTRS.DATA_MIN) * S7XX::AUDIO_SEGMENT_SIZE);
	s7xx_fs->stream.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	for(u16 i = 0; i < S7XX::AUDIO_SEGMENT_SIZE; i++)
	{
		if(buffer1[i] != buffer2[i])
		{
			std::cerr << "Data mismatch at " << i << "!!!" << std::endl;
			std::cerr << "Expected: " << (u16)buffer1[i] << std::endl;
			std::cerr << "Got: " << (u16)buffer2[i] << std::endl;
			std::cerr << "Exit: 19" << std::endl;
			return 19;
		}
	}
	/*-------------------End of reloc first cluster in chain------------------*/

	/*----------------------Reloc other cluster in chain----------------------*/
	constexpr u16 OTHER_CLS_ADDR = S7XX::FS::FAT_ATTRS.DATA_MIN + 1;
	cls = s7xx_fs->FAT[OTHER_CLS_ADDR];

	err = S7XX::FS::reloc_cluster(*s7xx_fs, OTHER_CLS_ADDR, S7XX::FS::S760_OS_CLUSTERS);
	if(err)
	{
		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		std::cerr << "Exit: 20" << std::endl;
		return 20;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 21" << std::endl;
		return 21;
	}

	if(s7xx_fs->FAT[OTHER_CLS_ADDR] != S7XX::FS::FAT_ATTRS.FREE_CLUSTER)
	{
		std::cerr << "Bad cluster value (at old)!!!" << std::endl;
		std::cerr << "Expected free cluster value (";
		std::cerr << S7XX::FS::FAT_ATTRS.FREE_CLUSTER << ")" << std::endl;
		std::cerr << "Got " << s7xx_fs->FAT[OTHER_CLS_ADDR] << std::endl;
		std::cerr << "Exit: 22" << std::endl;
		return 22;
	}

	if(s7xx_fs->FAT[EXPECTED_NEW_CLS_2] != cls)
	{
		std::cerr << "Bad cluster value (at new)!!!" << std::endl;
		std::cerr << "Expected: " << cls << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[EXPECTED_NEW_CLS_2] << std::endl;
		std::cerr << "Exit: 23" << std::endl;
		return 23;
	}

	if(s7xx_fs->FAT[EXPECTED_NEW_CLS_1] != EXPECTED_NEW_CLS_2)
	{
		std::cerr << "Failed to update previous cluster!!!" << std::endl;
		std::cerr << "Expected: " << EXPECTED_NEW_CLS_2 << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[EXPECTED_NEW_CLS_1] << std::endl;
		std::cerr << "Exit: 24" << std::endl;
		return 24;
	}

	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::AUDIO_SECTION + (OTHER_CLS_ADDR - S7XX::FS::FAT_ATTRS.DATA_MIN) * S7XX::AUDIO_SEGMENT_SIZE);
	s7xx_fs->stream.read((char*)buffer1.get(), S7XX::AUDIO_SEGMENT_SIZE);

	s7xx_fs->stream.seekg(S7XX::FS::On_disk_addrs::AUDIO_SECTION + (EXPECTED_NEW_CLS_2 - S7XX::FS::FAT_ATTRS.DATA_MIN) * S7XX::AUDIO_SEGMENT_SIZE);
	s7xx_fs->stream.read((char*)buffer2.get(), S7XX::AUDIO_SEGMENT_SIZE);

	for(u16 i = 0; i < S7XX::AUDIO_SEGMENT_SIZE; i++)
	{
		if(buffer1[i] != buffer2[i])
		{
			std::cerr << "Data mismatch at " << i << "!!!" << std::endl;
			std::cerr << "Expected: " << (u16)buffer1[i] << std::endl;
			std::cerr << "Got: " << (u16)buffer2[i] << std::endl;
			std::cerr << "Exit: 25" << std::endl;
			return 25;
		}
	}

	buffer1.reset();
	buffer2.reset();
	/*-------------------End of reloc other cluster in chain------------------*/

	/*TODO: Consider moving fsck test to its own file and running a safety fsck
	here.*/
	return 0;
}

static int reloc_OS_clusters()
{
	constexpr char test_img_name[] = "reloc_OS_cls_test.img";

	u16 err, cls;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(test_img_name))
		std::filesystem::remove_all(test_img_name);

	std::filesystem::copy_file(TEST_FS_PATH, test_img_name);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(test_img_name);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 26" << std::endl;
		return 26;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 27" << std::endl;
		return 27;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	/*Again, this one only returns min_vfs::ERR::NO_SPACE_LEFT. Testing for that
	one is not urgent.*/
	/*------------------------End of common err testing-----------------------*/

	cls = s7xx_fs->FAT[1];

	err = S7XX::FS::reloc_OS_clusters(*s7xx_fs);
	if(err)
	{
		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		std::cerr << "Exit: 28" << std::endl;
		return 28;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 29" << std::endl;
		return 29;
	}

	if(s7xx_fs->FAT[1] != (cls - S7XX::FS::S760_OS_CLUSTERS))
	{
		std::cerr << "Cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << (cls - S7XX::FS::S760_OS_CLUSTERS) << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 30" << std::endl;
		return 30;
	}

	cls = 0xFFFE;

	for(u8 i = S7XX::FS::FAT_ATTRS.DATA_MIN; i < S7XX::FS::S760_OS_CLUSTERS + S7XX::FS::FAT_ATTRS.DATA_MIN; i++)
	{
		if(s7xx_fs->FAT[i] != cls)
		{
			std::cerr << "Cluster val mismatch!!!" << std::endl;
			std::cerr << "Expected: " << cls << std::endl;
			std::cerr << "Got: " << s7xx_fs->FAT[i] << std::endl;
			std::cerr << "Exit: 31" << std::endl;
			return 31;
		}

		if(i == S7XX::FS::FAT_ATTRS.DATA_MIN + 56) cls = 0xFFFD;
	}

	return 0;
}

static int free_OS_clusters()
{
	constexpr char test_img_name[] = "free_OS_cls_test.img";

	u16 err, cls;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(test_img_name))
		std::filesystem::remove_all(test_img_name);

	std::filesystem::copy_file(TEST_FS_PATH, test_img_name);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(test_img_name);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 32" << std::endl;
		return 32;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 33" << std::endl;
		return 33;
	}
	/*----------------------------End of data setup---------------------------*/

	/*---------------------------Common err testing---------------------------*/
	/*Can't error out (except for stream IO errors).*/
	/*------------------------End of common err testing-----------------------*/

	cls = s7xx_fs->FAT[1];

	err = S7XX::FS::free_OS_cls(*s7xx_fs);
	if(err)
	{
		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		std::cerr << "Exit: 34" << std::endl;
		return 34;
	}

	if(!comp_FATs(*s7xx_fs))
	{
		std::cerr << "FAT data mismatched!!!" << std::endl;
		std::cerr << "Exit: 35" << std::endl;
		return 35;
	}

	if(s7xx_fs->FAT[1] != (cls + S7XX::FS::S760_OS_CLUSTERS))
	{
		std::cerr << "Cluster count mismatch!!!" << std::endl;
		std::cerr << "Expected: " << (cls + S7XX::FS::S760_OS_CLUSTERS) << std::endl;
		std::cerr << "Got: " << s7xx_fs->FAT[1] << std::endl;
		std::cerr << "Exit: 36" << std::endl;
		return 36;
	}

	for(u8 i = S7XX::FS::FAT_ATTRS.DATA_MIN; i < S7XX::FS::S760_OS_CLUSTERS + S7XX::FS::FAT_ATTRS.DATA_MIN; i++)
	{
		if(s7xx_fs->FAT[i] != S7XX::FS::FAT_ATTRS.FREE_CLUSTER)
		{
			std::cerr << "Cluster val mismatch!!!" << std::endl;
			std::cerr << "Expected: " << S7XX::FS::FAT_ATTRS.FREE_CLUSTER << std::endl;
			std::cerr << "Got: " << s7xx_fs->FAT[i] << std::endl;
			std::cerr << "Exit: 37" << std::endl;
			return 37;
		}
	}

	return 0;
}

static int file_exists()
{
	constexpr char test_img_name[] = "file_exists_test.img";

	bool res;

	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;

	/*-------------------------------Data setup-------------------------------*/
	if(std::filesystem::exists(test_img_name))
		std::filesystem::remove_all(test_img_name);

	std::filesystem::copy_file(TEST_FS_PATH, test_img_name);

	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(test_img_name);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.err_code << std::endl;
		std::cerr << "Exit: 38" << std::endl;
		return 38;
	}
	/*----------------------------End of data setup---------------------------*/

	res = S7XX::FS::file_exists<S7XX::FS::TYPE_ATTRS[1]>(*s7xx_fs, 0);
	if(!res)
	{
		std::cerr << "Exit: 39" << std::endl;
		return 39;
	}

	res = S7XX::FS::file_exists<S7XX::FS::TYPE_ATTRS[1]>(*s7xx_fs, 1);
	if(res)
	{
		std::cerr << "Exit: 40" << std::endl;
		return 40;
	}

	s7xx_fs->stream.seekp(S7XX::FS::On_disk_addrs::VOLUME_LIST + 2 * S7XX::FS::On_disk_sizes::LIST_ENTRY);
	s7xx_fs->stream.put(0);

	res = S7XX::FS::file_exists<S7XX::FS::TYPE_ATTRS[1]>(*s7xx_fs, 2);
	if(res)
	{
		std::cerr << "Exit: 41" << std::endl;
		return 41;
	}

	return 0;
}

int main()
{
	constexpr char test_img_name[] = "test_fs.img";

	u16 err;
	std::unique_ptr<S7XX::FS::filesystem_t> s7xx_fs;

	std::cout << "Mounting test fs..." << std::endl;
	try
	{
		s7xx_fs = std::make_unique<S7XX::FS::filesystem_t>(test_img_name);
	}
	catch(min_vfs::FS_err e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << std::hex << e.err_code << std::dec << std::endl;
	}
	std::cout << "Mount OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Find sample from name test..." << std::endl;
	err = find_sample_from_name_tests(*s7xx_fs);
	if(err) return err;
	std::cout << "Find sample from name OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Index and name from name test..." << std::endl;
	err = idx_and_name_from_name_tests();
	if(err) return err;
	std::cout << "Index and name from name OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Unzero all before test..." << std::endl;
	err = unzero_all_before();
	if(err) return err;
	std::cout << "Unzero all before OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Reloc cluster test..." << std::endl;
	err = reloc_cluster();
	if(err) return err;
	std::cout << "Reloc cluster OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Reloc OS clusters test..." << std::endl;
	err = reloc_OS_clusters();
	if(err) return err;
	std::cout << "Reloc OS clusters OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "Free OS clusters test..." << std::endl;
	err = free_OS_clusters();
	if(err) return err;
	std::cout << "Free OS clusters OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "File exists test..." << std::endl;
	err = file_exists();
	if(err) return err;
	std::cout << "File exists OK!" << std::endl;
	std::cout << std::endl;

	std::cout << "ALL TESTS OK!" << std::endl;

	return 0;
}
