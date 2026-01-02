#include <filesystem>
#include <regex>
#include <fstream>
#include <cstring>
#include <charconv>

#include "Utils/ints.hpp"
#include "Utils/utils.hpp"
#include "Utils/str_util.hpp"
#include "host_FS_utils.hpp"

namespace S7XX::Host_FS
{
	void sanitize_filename(std::string &filename)
	{
		size_t pos;

		/*separator after category. this is only a problem on Windows, but we're
		always gonna treat them the same way for the sake of some
		standardization*/
		pos = filename.find(':');

		if(pos != filename.npos)
			filename[pos] = '-';

		//used before 'L' or 'R' on samples
		pos = filename.find_last_of(0x7F);

		if(pos != filename.npos)
			filename[pos] = '-';

		//filenames can contain '/'... ·_·
		pos = filename.find('/');
		while(pos != filename.npos)
		{
			filename[pos] = '_';
			pos = filename.find('/', pos);
		}

		/*remove trailing whitespaces. Windows already does this so let's make
		sure our filenames are consistent on Linux/Unix/POSIX/etc.*/
		str_util::rtrim(filename);
	}

	uint16_t find_from_index(const std::filesystem::path &dir_path, const u16 idx, std::filesystem::path &file_path)
	{
		u16 f_idx;

		if(!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path))
			return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_PATH);

		for(const std::filesystem::directory_entry dentry: std::filesystem::directory_iterator(dir_path))
		{
			if(!dentry.is_regular_file()) continue;

			const std::string fname = dentry.path().filename().string();

			if(!std::regex_match(fname, S7XX_HOST_FNAME_REGEX))
				continue;

			f_idx = std::stoi(fname.substr(0, fname.find_first_of('-')));

			if(f_idx == idx)
			{
				file_path = dentry.path();
				return 0;
			}
		}

		return ret_val_setup(LIBRARY_ID, (u8)ERR::NOT_FOUND);
	}

	namespace FILE_SIZES
	{
		constexpr u16 VOL = 272;
		constexpr u16 PER = 528;
		constexpr u16 PAT = 528;
		constexpr u8 PAR = 144;
		constexpr u8 SAM = 64; //+ audio
	}
}
