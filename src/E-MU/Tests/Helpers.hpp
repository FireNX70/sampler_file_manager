#ifndef EMU_FS_TESTING_HELPERS_HEADER_INCLUDE_GUARD
#define EMU_FS_TESTING_HELPERS_HEADER_INCLUDE_GUARD

#include <vector>
#include <string>

#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "E-MU/EMU_FS_types.hpp"

u8 check_file_list(const std::vector<min_vfs::dentry_t> &expected_dentries, const std::vector<min_vfs::dentry_t> &dentries, const ptrdiff_t offset);

std::string header_to_string(const EMU::FS::Header_t &header, const u8 indent_level);
bool headercmp(const EMU::FS::Header_t &a, const EMU::FS::Header_t &b);

bool dircmp(const EMU::FS::Dir_t &a, const EMU::FS::Dir_t &b);
std::string dir_to_string(const EMU::FS::Dir_t &dir, const u8 indent_level);

bool filecmp(const EMU::FS::File_t &a, const EMU::FS::File_t &b);
std::string file_to_string(const EMU::FS::File_t &file, const u8 indent_level);

void print_fsck_status(const u16 fsck_status);
#endif
