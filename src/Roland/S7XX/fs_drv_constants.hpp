#ifndef S7XX_FS_DRIVER_CONSTANTS_INCLUDE_GUARD
#define S7XX_FS_DRIVER_CONSTANTS_INCLUDE_GUARD

#include "S7XX_FS_types.hpp"

//For constants internal to mkfs/fsck/the driver only
namespace S7XX::FS
{
	struct type_attrs_t
	{
		uint16_t TOC_t:: *const TOC_PTR;
		const uint16_t MAX_CNT;
		const uint32_t LIST_ADDR;
		const uint32_t PARAMS_ADDR;
		const uint16_t PARAMS_ENTRY_SIZE;
		const Element_type_t ELEMENT_TYPE;
		const u8 TYPE_IDX; //for templated funcs
	};

	constexpr type_attrs_t TYPE_ATTRS[] =
	{
		{nullptr, 1, 0, 0, 0, (Element_type_t)0, 0}, //OS is a special case
		{&TOC_t::volume_cnt, MAX_VOLUME_COUNT, On_disk_addrs::VOLUME_LIST, On_disk_addrs::VOLUME_PARAMS, On_disk_sizes::VOLUME_PARAMS_ENTRY, Element_type_t::volume, 1},
		{&TOC_t::perf_cnt, MAX_PERF_COUNT, On_disk_addrs::PERF_LIST, On_disk_addrs::PERF_PARAMS, On_disk_sizes::PERF_PARAMS_ENTRY, Element_type_t::performance, 2},
		{&TOC_t::patch_cnt, MAX_PATCH_COUNT, On_disk_addrs::PATCH_LIST, On_disk_addrs::PATCH_PARAMS, On_disk_sizes::PATCH_PARAMS_ENTRY, Element_type_t::patch, 3},
		{&TOC_t::partial_cnt, MAX_PARTIAL_COUNT, On_disk_addrs::PARTIAL_LIST, On_disk_addrs::PARTIAL_PARAMS, On_disk_sizes::PARTIAL_PARAMS_ENTRY, Element_type_t::partial, 4},
		{&TOC_t::sample_cnt, MAX_SAMPLE_COUNT, On_disk_addrs::SAMPLE_LIST, On_disk_addrs::SAMPLE_PARAMS, On_disk_sizes::SAMPLE_PARAMS_ENTRY, Element_type_t::sample, 5}
	};
}
#endif