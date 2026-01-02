#ifndef MIN_VFS_TESTING_HELPERS_HEADER_INCLUDE_GUARD
#define MIN_VFS_TESTING_HELPERS_HEADER_INCLUDE_GUARD

#include <vector>

#include "min_vfs/min_vfs_base.hpp"

bool compare_dirs(const std::vector<min_vfs::dentry_t> &expected,
				  const std::vector<min_vfs::dentry_t> &got);
bool compare_dentries_ignore_time(const min_vfs::dentry_t &a,
								  const min_vfs::dentry_t &b);
bool compare_dirs_ignore_times(const std::vector<min_vfs::dentry_t> &expected,
							   const std::vector<min_vfs::dentry_t> &got);

#endif
