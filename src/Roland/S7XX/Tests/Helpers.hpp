#ifndef S7XX_FS_TEST_HELPERS
#define S7XX_FS_TEST_HELPERS

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <initializer_list>

#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"

u8 check_file_list(const std::vector<min_vfs::dentry_t> &expected_dentries, const std::vector<min_vfs::dentry_t> &dentries, const ptrdiff_t offset);
bool filecmp(const std::filesystem::path &path_a, const std::filesystem::path &path_b);
bool filecmp(const std::filesystem::path &path_a, const std::filesystem::path &path_b, const uintmax_t start_offset, const uintmax_t size);
std::string compose_path(const std::initializer_list<std::string_view> elements);
void print_fsck_status(const u16 fsck_status);

//S7XX_FS
std::string_view media_type_to_str(const S7XX::FS::Media_type_t media_type);
void print_TOC(const S7XX::FS::TOC_t &TOC);
bool TOCcmp(const S7XX::FS::TOC_t &A, const S7XX::FS::TOC_t &B);
bool Headercmp(const S7XX::FS::Header_t &A, const S7XX::FS::Header_t &B);
bool comp_FATs(S7XX::FS::filesystem_t &fs);
#endif
