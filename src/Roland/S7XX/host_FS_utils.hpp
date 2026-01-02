#ifndef S7XX_HOST_FS_UTILS_INCLUDE_GUARD
#define S7XX_HOST_FS_UTILS_INCLUDE_GUARD

#include <filesystem>
#include <regex>
#include <bit>

#include "library_IDs.hpp"
#include "Utils/ints.hpp"
#include "S7XX_types.hpp"

namespace S7XX::Host_FS
{
	constexpr u8 LIBRARY_ID = (u8)Library_IDs::S7XX_HOST_FS;

	enum struct ERR: u8
	{
		INVALID_PATH = 1,
		NOT_FOUND,
		INVALID_FILE,
		IO_ERROR,
		BAD_FILE_SIZE
	};

	const std::regex S7XX_HOST_FNAME_REGEX("^[0-9]{1,4}-.{3}-.{0,12}$", std::regex::ECMAScript | std::regex::optimize);

	namespace FILE_MAGICS
	{
		constexpr char OS[] = "S7XX_OS\0";
		constexpr char VOL[] = "S7XX_VOL";
		constexpr char PERF[] = "S7XX_PER";
		constexpr char PATCH[] = "S7XX_PAT";
		constexpr char PARTIAL[] = "S7XX_PAR";
		constexpr char SAMPLE[] = "S7XX_SAM";
	}

	//The native FS driver does not touch the endianness
	constexpr std::endian ENDIANNESS = std::endian::little;

	void sanitize_filename(std::string &filename);
	uint16_t find_from_index(const std::filesystem::path &dir_path, const u16 idx, std::filesystem::path &file_path);
}
#endif // !S7XX_HOST_FS_UTILS_INCLUDE_GUARD
