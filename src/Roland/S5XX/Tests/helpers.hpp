#ifndef S5XX_FS_TESTING_HELPERS_INCLUDE_GUARD
#define S5XX_FS_TESTING_HELPERS_INCLUDE_GUARD

#include <string>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "Utils/ints.hpp"

bool file_entrycmp(const S5XX::FS::File_entry_t &a, const S5XX::FS::File_entry_t &b);
std::string file_entry_to_string(const S5XX::FS::File_entry_t &fentry, const u8 indent_level);

#endif
