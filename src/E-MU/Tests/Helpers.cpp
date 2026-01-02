#include <iostream>
#include <string>
#include <cstring>

#include "min_vfs/min_vfs_base.hpp"
#include "E-MU/EMU_FS_types.hpp"
#include "E-MU/EMU_FS_drv.hpp"

u8 check_file_list(const std::vector<min_vfs::dentry_t> &expected_dentries, const std::vector<min_vfs::dentry_t> &dentries, const ptrdiff_t offset)
{
	u8 err;
	u16 i, end;

	err = 0;

	if(offset < 0)
	{
		i = 0;
		end = expected_dentries.size() + offset;
	}
	else
	{
		i = offset;
		end = expected_dentries.size();
	}

	if(dentries.size() != (end - i))
	{
		std::cerr << "Mismatched entry cnt!!!" << std::endl;
		std::cerr << "Expected cnt: " << (end - i);
		std::cerr << ", got: " << dentries.size() << std::endl;
		return 1;
	}

	for(u16 j = 0; i < end; i++, j++)
	{
		if(expected_dentries[i].fname != dentries[j].fname)
		{
			err = 2;
			std::cerr << "Filename mismatched!!!" << std::endl;
			std::cerr << "Expected: " << expected_dentries[i].fname << std::endl;
			std::cerr << "Got: " << dentries[j].fname << std::endl;
		}

		if(expected_dentries[i].ftype != dentries[j].ftype)
		{
			err = 2;
			std::cerr << dentries[j].fname << std::endl;
			std::cerr << "\tFile type mismatched!!!" << std::endl;
			std::cerr << "\tExpected: " << (u16)expected_dentries[i].ftype << std::endl;
			std::cerr << "\tGot: " << (u16)dentries[j].ftype << std::endl;
		}

		if(expected_dentries[i].fsize != dentries[j].fsize)
		{
			err = 2;
			std::cerr << dentries[j].fname << std::endl;
			std::cerr << "\tFile size mismatched!!!" << std::endl;
			std::cerr << "\tExpected: " << expected_dentries[i].fsize << std::endl;
			std::cerr << "\tGot: " << dentries[j].fsize << std::endl;
		}
	}

	return err;
}

bool headercmp(const EMU::FS::Header_t &a, const EMU::FS::Header_t &b)
{
	return a.block_cnt == b.block_cnt
		&& a.dir_list_blk_addr == b.dir_list_blk_addr
		&& a.dir_list_blk_cnt == b.dir_list_blk_cnt
		&& a.file_list_blk_addr == b.file_list_blk_addr
		&& a.file_list_blk_cnt == b.file_list_blk_cnt
		&& a.FAT_blk_addr == b.FAT_blk_addr && a.FAT_blk_cnt == b.FAT_blk_cnt
		&& a.data_sctn_blk_addr == b.data_sctn_blk_addr
		&& a.cluster_cnt == b.cluster_cnt && a.cluster_shift == b.cluster_shift;
}

std::string header_to_string(const EMU::FS::Header_t &header, const u8 indent_level)
{
	const std::string prepend("\t", indent_level);

	std::string res;

	res = prepend + "Header:\n";
	res += prepend + "\tBlock count: " + std::to_string(header.block_cnt) + "\n";
	res += prepend + "\tDir list block addr: " + std::to_string(header.dir_list_blk_addr) + "\n";
	res += prepend + "\tDir list block count: " + std::to_string(header.dir_list_blk_cnt) + "\n";
	res += prepend + "\tFile list block addr: " + std::to_string(header.file_list_blk_addr) + "\n";
	res += prepend + "\tFile list block count: " + std::to_string(header.file_list_blk_cnt) + "\n";
	res += prepend + "\tFAT block addr: " + std::to_string(header.FAT_blk_addr) + "\n";
	res += prepend + "\tFAT block count: " + std::to_string(header.FAT_blk_cnt) + "\n";
	res += prepend + "\tData section block addr: " + std::to_string(header.data_sctn_blk_addr) + "\n";
	res += prepend + "\tCluster count: " + std::to_string(header.cluster_cnt) + "\n";
	res += prepend + "\tCluster shift: " + std::to_string((u16)header.cluster_shift) + "\n";

	return res;
}

void print_fsck_status(const u16 fsck_status)
{
	std::cout << "fsck (" << fsck_status << "): " << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::INVALID_CHECKSUM)
		std::cout << "\tInvalid checksum." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_CLUSTER_SHIFT)
		std::cout << "\tBad cluster shift." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_BLOCK_CNT)
		std::cout << "\tBad block count." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_CLUSTER_CNT)
		std::cout << "\tBad cluster count." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_ROOT_DIR)
		std::cout << "\tBad root directory." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_FILE_LIST)
		std::cout << "\tBad file list." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_FAT_ADDR)
		std::cout << "\tBad FAT address." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_FAT_BLK_CNT)
		std::cout << "\tBad FAT block count." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::FILE_LIST_COLLISION)
		std::cout << "\tFile list collision." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::FAT_COLLISION)
		std::cout << "\tFAT collision." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::DATA_COLLISION)
		std::cout << "\tData collision." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_DIR)
		std::cout << "\tBad directory." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_NEXT_DIR_CONTENT_BLK)
		std::cout << "\tBad next directory content block." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::UNMARKED_RESERVED_CLUSTERS)
		std::cout << "\tUnmarked reserved clusters." << std::endl;

	if(fsck_status & (u16)EMU::FS::FSCK_STATUS::BAD_FILE)
		std::cout << "\tBad file." << std::endl;
}

bool dircmp(const EMU::FS::Dir_t &a, const EMU::FS::Dir_t &b)
{
	return !std::strncmp(a.name, b.name, sizeof(EMU::FS::Dir_t::name))
		&& a.type == b.type
		&& std::memcmp(a.blocks, b.blocks, sizeof(EMU::FS::Dir_t::blocks))
		&& a.addr == b.addr;
}

std::string dir_to_string(const EMU::FS::Dir_t &dir, const u8 indent_level)
{
	const std::string prepend("\t", indent_level);

	std::string res;

	res = prepend + dir.name + "\n";
	res += prepend + "\tType: " + std::to_string((u8)dir.type) + "\n";
	res += prepend + "\tBlocks:\n";
	res += prepend + "\t";

	for(const u16 block: dir.blocks)
		res += std::to_string(block) + " ";

	res += "\n";
	res += prepend + "\tAddress: " + std::to_string(dir.addr) + "\n";

	return res;
}

bool filecmp(const EMU::FS::File_t &a, const EMU::FS::File_t &b)
{
	return !std::strncmp(a.name, b.name, sizeof(EMU::FS::File_t::name))
	&& a.bank_num == b.bank_num && a.start_cluster == b.start_cluster
	&& a.cluster_cnt == b.cluster_cnt && a.block_cnt == b.block_cnt
	&& a.byte_cnt == b.byte_cnt && a.type == b.type
	&& std::memcmp(a.props, b.props, sizeof(EMU::FS::File_t::props))
	&& a.addr == b.addr;
}

std::string file_to_string(const EMU::FS::File_t &file, const u8 indent_level)
{
	const std::string prepend("\t", indent_level);

	std::string res;

	res = prepend + file.name + "\n";
	res += prepend + "\tBank number: " + std::to_string(file.bank_num) + "\n";
	res += prepend + "\tStart cluster: " + std::to_string(file.start_cluster) + "\n";
	res += prepend + "\tCluster count: " + std::to_string(file.cluster_cnt) + "\n";
	res += prepend + "\tBlock count: " + std::to_string(file.block_cnt) + "\n";
	res += prepend + "\tByte count: " + std::to_string(file.cluster_cnt) + "\n";
	res += prepend + "\tType: " + std::to_string((u8)file.type) + "\n";
	res += prepend + "\tProps:\n";
	res += prepend + "\t";

	for(const u8 prop: file.props)
		res += std::to_string(prop) + " ";

	res += "\n";
	res += prepend + "\tAddress: " + std::to_string(file.addr) + "\n";

	return res;
}
