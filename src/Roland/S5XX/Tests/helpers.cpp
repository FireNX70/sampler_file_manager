#include <cstring>
#include <string>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "Utils/ints.hpp"

bool file_entrycmp(const S5XX::FS::File_entry_t &a, const S5XX::FS::File_entry_t &b)
{
	return !std::strncmp(a.name, b.name, sizeof(S5XX::FS::File_entry_t::name))
		&& a.block_addr == b.block_addr && a.block_cnt == b.block_cnt
		&& a.type == b.type && a.ins_grp_idx == b.ins_grp_idx;
}

std::string file_entry_to_string(const S5XX::FS::File_entry_t &fentry, const u8 indent_level)
{
	const std::string prepend = std::string(indent_level, '\t');

	std::string res;

	res = prepend + fentry.name + "\n";
	res += prepend + "\tBlock address: " + std::to_string(fentry.block_addr) + "\n";
	res += prepend + "\tBlock count: " + std::to_string(fentry.block_cnt) + "\n";
	res += prepend + "\tType: " + std::to_string((u8)fentry.type) + "\n";
	res += prepend + "\tInstr. group index: " + std::to_string(fentry.ins_grp_idx) + "\n";

	return res;
}
