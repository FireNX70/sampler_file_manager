#include <filesystem>
#include <iostream>

#include "Host_FS/host_drv.cpp"
#include "min_vfs/min_vfs_base.hpp"

bool host_ftype_to_ftype_test(const std::filesystem::file_type native_ftype,
							 const min_vfs::ftype_t expected)
{
	min_vfs::ftype_t min_vfs_ftype;

	min_vfs_ftype = Host::FS::host_ftype_to_ftype(native_ftype);

	if(min_vfs_ftype != expected)
	{
		std::cerr << "File type mismatch!!!" << std::endl;
		std::cerr << "Expected " << min_vfs::ftype_to_string(expected)
			<< std::endl;
		std::cerr << "Got: " << min_vfs::ftype_to_string(min_vfs_ftype)
		<< std::endl;

		return true;
	}

	return false;
}

int host_ftype_to_ftype_tests()
{
	using native_ftype = std::filesystem::file_type;

	if(host_ftype_to_ftype_test(native_ftype::regular, min_vfs::ftype_t::file))
	{
		std::cerr << "Exit: " << 1 << std::endl;
		return 1;
	}

	if(host_ftype_to_ftype_test(native_ftype::directory, min_vfs::ftype_t::dir))
	{
		std::cerr << "Exit: " << 2 << std::endl;
		return 2;
	}

	if(host_ftype_to_ftype_test(native_ftype::symlink,
		min_vfs::ftype_t::symlink))
	{
		std::cerr << "Exit: " << 3 << std::endl;
		return 3;
	}

	if(host_ftype_to_ftype_test(native_ftype::block, min_vfs::ftype_t::blk_dev))
	{
		std::cerr << "Exit: " << 4 << std::endl;
		return 4;
	}

	if(host_ftype_to_ftype_test(native_ftype::character,
		min_vfs::ftype_t::char_dev))
	{
		std::cerr << "Exit: " << 5 << std::endl;
		return 5;
	}

	if(host_ftype_to_ftype_test(native_ftype::fifo, min_vfs::ftype_t::pipe))
	{
		std::cerr << "Exit: " << 6 << std::endl;
		return 6;
	}

	if(host_ftype_to_ftype_test(native_ftype::socket, min_vfs::ftype_t::socket))
	{
		std::cerr << "Exit: " << 7 << std::endl;
		return 7;
	}

	if(host_ftype_to_ftype_test(native_ftype::none, min_vfs::ftype_t::unknown))
	{
		std::cerr << "Exit: " << 8 << std::endl;
		return 8;
	}

	if(host_ftype_to_ftype_test(native_ftype::unknown,
		min_vfs::ftype_t::unknown))
	{
		std::cerr << "Exit: " << 9 << std::endl;
		return 9;
	}

	if(host_ftype_to_ftype_test(native_ftype::not_found,
		min_vfs::ftype_t::unknown))
	{
		std::cerr << "Exit: " << 10 << std::endl;
		return 10;
	}

	return 0;
}

int main(void)
{
	int err;

	std::cout << "Host ftype to min_vfs ftype tests..." << std::endl;
	err = host_ftype_to_ftype_tests();
	if(err) return err;
	std::cout << "Host ftype to min_vfs ftype OK!" << std::endl;

	return 0;
}
