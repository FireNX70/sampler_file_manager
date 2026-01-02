#include <cstdint>
#include <fstream>

#include "S7XX_FS_types.hpp"

namespace S7XX::FS
{
	uint16_t load_TOC(std::fstream &src, TOC_t &dst)
	{
		src.seekg(On_disk_addrs::TOC);

		src.read(dst.name, 16);
		dst.name[16] = 0;
		src.read((char*)&dst.block_cnt, 4);
		src.read((char*)&dst.volume_cnt, 2);
		src.read((char*)&dst.perf_cnt, 2);
		src.read((char*)&dst.patch_cnt, 2);
		src.read((char*)&dst.partial_cnt, 2);
		src.read((char*)&dst.sample_cnt, 2);

		if constexpr(std::endian::native != ENDIANNESS)
		{
			dst.block_cnt = std::byteswap(dst.block_cnt);
			dst.volume_cnt = std::byteswap(dst.volume_cnt);
			dst.perf_cnt = std::byteswap(dst.perf_cnt);
			dst.patch_cnt = std::byteswap(dst.patch_cnt);
			dst.partial_cnt = std::byteswap(dst.partial_cnt);
			dst.sample_cnt = std::byteswap(dst.sample_cnt);
		}

		return 0;
	}

	uint16_t write_TOC(TOC_t src, std::fstream &dst)
	{
		if constexpr(std::endian::native != ENDIANNESS)
		{
			src.block_cnt = std::byteswap(src.block_cnt);
			src.volume_cnt = std::byteswap(src.volume_cnt);
			src.perf_cnt = std::byteswap(src.perf_cnt);
			src.patch_cnt = std::byteswap(src.patch_cnt);
			src.partial_cnt = std::byteswap(src.partial_cnt);
			src.sample_cnt = std::byteswap(src.sample_cnt);
		}

		dst.seekp(On_disk_addrs::TOC);

		dst.write(src.name, 16);
		dst.write((char*)&src.block_cnt, 4);
		dst.write((char*)&src.volume_cnt, 2);
		dst.write((char*)&src.perf_cnt, 2);
		dst.write((char*)&src.patch_cnt, 2);
		dst.write((char*)&src.partial_cnt, 2);
		dst.write((char*)&src.sample_cnt, 2);

		return 0;
	}
}