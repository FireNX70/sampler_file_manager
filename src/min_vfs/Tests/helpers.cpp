#include <vector>
#include <iostream>

#include "min_vfs/min_vfs_base.hpp"

bool compare_dirs(const std::vector<min_vfs::dentry_t> &expected,
				  const std::vector<min_vfs::dentry_t> &got)
{
	bool ret;

	ret = true;
	for(size_t i = 0; i < expected.size(); i++)
	{
		if(got[i] != expected[i])
		{
			std::cerr << "Mismatched dentry!!!" << std::endl;
			std::cerr << "Expected:\n" << expected[i].to_string(1) << std::endl;
			std::cerr << "Got:\n" << got[i].to_string(1) << std::endl;
			ret = false;
		}
	}

	return ret;
}

bool compare_dentries_ignore_time(const min_vfs::dentry_t &a,
								  const min_vfs::dentry_t &b)
{
	return a.fname == b.fname && a.fsize == b.fsize && a.ftype == b.ftype;
}

bool compare_dirs_ignore_times(const std::vector<min_vfs::dentry_t> &expected,
							   const std::vector<min_vfs::dentry_t> &got)
{
	bool ret;

	ret = true;
	for(size_t i = 0; i < expected.size(); i++)
	{
		if(!compare_dentries_ignore_time(got[i], expected[i]))
		{
			std::cerr << "Mismatched dentry!!!" << std::endl;
			std::cerr << "Expected:\n" << expected[i].to_string(1) << std::endl;
			std::cerr << "Got:\n" << got[i].to_string(1) << std::endl;
			ret = false;
		}
	}

	return ret;
}
