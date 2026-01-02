#ifndef LIBRARY_IDS_HEADER
#define LIBRARY_IDS_HEADER

#include "Utils/ints.hpp"

enum class Library_IDs: u8
{
	//0 is reserved for main
	EMU_FS = 1,
	S7XX_FS,
	FAT_utils,
	MIN_VFS,
	S7XX_HOST_FS
};

#endif // !LIBRARY_IDS_HEADER
