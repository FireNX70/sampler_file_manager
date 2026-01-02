#ifndef HOST_FS_TEST_DATA_INCLUDE_GUARD
#define HOST_FS_TEST_DATA_INCLUDE_GUARD

#include <string_view>

#include "Roland/S7XX/S7XX_types.hpp"

namespace HOST_FS_TEST_DATA
{
	constexpr char REF_HOST_FS_PATH[] = "ref_host_fs";

	constexpr std::string_view EXPECTED_SAMPLE_NAMES[] =
	{
		"0-TST-Test_01   -L",
		"1-TST-Test_01   -R",
		"2-TST-Test_02",
		"3-TST-Test_03",
		"4-TST-Test_04"
	};

	constexpr std::string_view EXPECTED_PARTIAL_NAMES[] =
	{
		"0-TST-Test_01",
		"1-TST-Test_02",
		"2-TST-Test_03",
		"3-TST-Test_04",
		"4-TST-Test_05"
	};

	constexpr std::string_view EXPECTED_PATCH_NAMES[] =
	{
		"0-TST-Test_01",
		"1-TST-Test_02",
		"2-TST-Test_03",
		"3-TST-Test_04",
		"4-TST-Test_05"
	};

	constexpr std::string_view EXPECTED_PERF_NAMES[] =
	{
		"0-TST-Test_01"
	};

	constexpr std::string_view EXPECTED_VOLUME_NAMES[] =
	{
		"0-TST-Test_01"
	};
}
#endif // !HOST_FS_TEST_DATA_INCLUDE_GUARD
