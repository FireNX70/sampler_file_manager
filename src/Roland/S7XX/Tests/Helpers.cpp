#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <string_view>
#include <map>

#include "Utils/ints.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "Roland/S7XX/S7XX_FS_types.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"

std::string_view media_type_to_str(const S7XX::FS::Media_type_t media_type)
{
	switch(media_type)
	{
		using enum S7XX::FS::Media_type_t;

		case HDD:
			return "HDD";

		case HDD_with_OS:
			return "HDD with S-770 OS";

		case DSDD_SYS_770:
			return "DSDD S-770 OS diskette";

		case DSHD_diskette:
			return "DSHD diskette";

		case DSDD_diskette:
			return "DSDD diskette";

		case HDD_with_OS_S760:
			return "HDD with S-760 OS";

		case DSHD_SYS_760:
			return "DSHD S-760 OS diskette";
	}

	return "Unknown media type!!!";
}

void print_TOC(const S7XX::FS::TOC_t &TOC)
{
	std::cout << TOC.name << std::endl;
	std::cout << "\tBlock cnt: " << TOC.block_cnt << std::endl;
	std::cout << "\tVol count: " << TOC.volume_cnt << std::endl;
	std::cout << "\tPerf cnt: " << TOC.perf_cnt << std::endl;
	std::cout << "\tPatch cnt: " << TOC.patch_cnt << std::endl;
	std::cout << "\tPartial cnt: " << TOC.partial_cnt << std::endl;
	std::cout << "\tSample cnt: " << TOC.sample_cnt << std::endl;
}

bool TOCcmp(const S7XX::FS::TOC_t &A, const S7XX::FS::TOC_t &B)
{
	return !std::strncmp(A.name, B.name, 16)
		&& A.block_cnt == B.block_cnt && A.volume_cnt == B.volume_cnt
		&& A.perf_cnt == B.perf_cnt && A.patch_cnt == B.patch_cnt
		&& A.partial_cnt == B.partial_cnt && A.sample_cnt == B.sample_cnt;
}

bool Headercmp(const S7XX::FS::Header_t &A, const S7XX::FS::Header_t &B)
{
	return !std::memcmp(A.machine_name, B.machine_name, 10)
		&& A.media_type == B.media_type
		&& !std::strncmp(A.text, B.text, sizeof(S7XX::FS::Header_t::text))
		&& TOCcmp(A.TOC, B.TOC);
}

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
			std::cerr << "\tExpected: " << (u8)expected_dentries[i].ftype << std::endl;
			std::cerr << "\tGot: " << (u8)dentries[j].ftype << std::endl;
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

bool filecmp(const std::filesystem::path &path_a, const std::filesystem::path &path_b, const uintmax_t start_offset, const uintmax_t size)
{
	const std::filesystem::path final_path_a = std::filesystem::is_symlink(path_a) ? std::filesystem::read_symlink(path_a) : path_a,
		final_path_b = std::filesystem::is_symlink(path_b) ? std::filesystem::read_symlink(path_b) : path_b;

	const uintmax_t fsize = std::filesystem::file_size(final_path_a);

	if(fsize != std::filesystem::file_size(final_path_b))
		return false;

	std::ifstream ifs_a(final_path_a, std::ifstream::binary), ifs_b(final_path_b, std::ifstream::binary);

	ifs_a.seekg(start_offset);
	ifs_b.seekg(start_offset);

	std::istreambuf_iterator isb_a(ifs_a), isb_b(ifs_b);

	const uintmax_t cmp_size = std::min(fsize - start_offset, size);

	for(uintmax_t i = 0; i < cmp_size; i++)
		if(*isb_a++ != *isb_b++) return false;

	return true;
}

bool filecmp(const std::filesystem::path &path_a, const std::filesystem::path &path_b)
{
	const std::filesystem::path final_path_a = std::filesystem::is_symlink(path_a) ? std::filesystem::read_symlink(path_a) : path_a,
		final_path_b = std::filesystem::is_symlink(path_b) ? std::filesystem::read_symlink(path_b) : path_b;

	const uintmax_t fsize = std::filesystem::file_size(final_path_a);

	if(fsize != std::filesystem::file_size(final_path_b))
		return false;

	std::ifstream ifs_a(final_path_a, std::ifstream::binary), ifs_b(final_path_b, std::ifstream::binary);
	std::istreambuf_iterator isb_a(ifs_a), isb_b(ifs_b);

	for(uintmax_t i = 0; i < fsize; i++)
		if(*isb_a++ != *isb_b++) return false;

	return true;
}

std::string compose_path(const std::initializer_list<std::string_view> elements)
{
	std::string result;

	for(size_t i = 0; i < elements.size() - 1; i++)
	{
		result += elements.begin()[i];
		result += "/";
	}

	result += *(elements.end() - 1);

	return result;
}

void print_fsck_status(const u16 fsck_status)
{
	std::cout << "fsck: ";

	if(fsck_status & S7XX::FS::FSCK_ERR::TOC_INCONSISTENCY)
		std::cout << "TOC inconsistency." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::TOO_LARGE)
		std::cout << "FS blk count too high." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::BAD_CLS0_VAL)
		std::cout << "Bad cluster 0 value." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::UNMARKED_RESVD_CLS)
		std::cout << "OOB clusters not reserved." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::FREE_CLS_CNT_MISMATCH)
		std::cout << "Wrong free cluster count." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::BAD_FENTRY)
		std::cout << "Bad file entry." << std::endl;

	if(fsck_status & S7XX::FS::FSCK_ERR::INACCESSIBLE_FENTRIES)
		std::cout << "Inaccessible file entries." << std::endl;
}

bool comp_FATs(S7XX::FS::filesystem_t &fs)
{
	u16 cls;

	fs.stream.seekg(fs.fat_attrs.BASE_ADDR);
	
	for(u16 i = 0; i < fs.fat_attrs.LENGTH; i++)
	{
		fs.stream.read((char*)&cls, 2);

		if constexpr(S7XX::FS::ENDIANNESS != std::endian::native)
			cls = std::byteswap(cls);

		if(cls != fs.FAT[i]) return false;
	}

	return true;
}
