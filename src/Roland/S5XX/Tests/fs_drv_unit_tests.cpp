#include <vector>
#include <random>
#include <memory>
#include <chrono>
#include <iostream>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "Utils/ints.hpp"
#include "Roland/S5XX/S5XX_FS_drv.cpp"
#include "fs_drv_test_data.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Utils/testing_helpers.hpp"

void fill_buffer(u64 *buffer, const size_t buff_size)
{
	//mt19937 should already be uniform
	static std::mt19937_64 rnd_eng(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

	for(size_t i = 0; i < buff_size; i++) buffer[i] = rnd_eng();
}

int rando_audio(void)
{
	/*I'm temporarily hijacking this to randomize the audio data to avoid
	 *potential copyright problems.*/
	u16 err;

	std::string sdir_path = "SoundDirectory /";
	std::unique_ptr<u64[]> buffer;

	const u8 buf_len = 512 / 8;

	S5XX::FS::filesystem_t fs = S5XX::FS::filesystem_t(BASE_TEST_FS_FNAME);
	std::vector<min_vfs::dentry_t> dentries;
	min_vfs::stream_t stream;
	S5XX::FS::File_entry_t *fentry;

	err = fs.list(sdir_path.c_str(), dentries);
	if(err)
	{
		print_unexpected_err(err, 1);
		return 1;
	}

	buffer = std::make_unique<u64[]>(buf_len);

	constexpr u8 block_off = 0x2400 / 0x200;

	for(const min_vfs::dentry_t &dentry: dentries)
	{
		err = fs.fopen((sdir_path + dentry.fname).c_str(), stream);
		if(err)
		{
			std::cerr << "fpath: " << sdir_path + dentry.fname << std::endl;
			print_unexpected_err(err, 2);
			return 2;
		}

		fentry = &(*fs.open_files.begin()).second.second.file_entry;

		fs.stream.seekp((fentry->block_addr + block_off) * S5XX::FS::BLOCK_SIZE);
		for(u32 j = block_off; j < fentry->block_cnt; j++)
		{
			fill_buffer(buffer.get(), buf_len);
			fs.stream.write((char*)buffer.get(), 512);
		}
	}

	stream.close();

	return 0;
}

int main(void)
{
	std::cerr << "No unit tests!!!" << std::endl;
	return 1;
}
