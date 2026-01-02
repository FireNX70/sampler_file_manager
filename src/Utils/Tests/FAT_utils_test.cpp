#include <bit>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <cstring>
#include <iostream>

#include "Utils/ints.hpp"
#include "Utils/FAT_utils.hpp"

constexpr FAT_utils::FAT_attrs_t<uint16_t> FAT_ATTRS(std::endian::little, 0, 1,
	0x7FFE, 0x7FFF, 0x8000);
constexpr FAT_utils::FAT_dyna_attrs_t<uint16_t> FAT_DYNA_ATTRS(1034, 0);
//1034 clusters because EMU_FS header doesn't count cluster 0

constexpr u16 EXPECTED_FREE_CLUSTER_COUNT = 31;
constexpr u16 get_nth_cluster_expected_chain[] = {0x177, 0x178, 0x179, 0x17A, 0x17B, 0x17C, 0x42, 0x43};
constexpr u16 follow_chain_tests_expected_chain[] = {1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
	29, 30, 98, 99};
constexpr u16 find_free_chain_tests_expected_chain[] = {55, 68, 219};
constexpr u16 grow_chain_test_expected_chain[] = {55, 68, 219, 220, 295, 298, 299, 300, 301};

template <typename T>
void print_vector(const std::vector<T> &vector, const u8 indent_level)
{
	const std::string prepend("\t", indent_level);

	if constexpr(sizeof(T) > sizeof(uintptr_t))
	{
		for(const T &elem: vector)
			std::cout << prepend << elem << std::endl;
	}
	else
	{
		for(const T elem: vector)
			std::cout << prepend << elem << std::endl;
	}
}

int follow_chain_disk_tests(std::fstream &fstr)
{
	std::vector<u16> chain;

	u16 expected_err, err;

	expected_err = (FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 4;
	}

	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)(FAT_ATTRS.DATA_MIN - 1), chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 5;
	}

	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)(FAT_ATTRS.DATA_MAX + 1), chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 6;
	}

	expected_err = 0;
	chain.clear();
	fstr.seekg(0);
	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)1, chain);

	if(err)
	{
		std::cerr << "Follow chain: Unexpected error!!!" << std::endl;
		std::cerr << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 7;
	}

	if(chain.size() != (sizeof(follow_chain_tests_expected_chain) / 2))
	{
		std::cerr << "Follow chain: Chain size mismatch!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", expected size: " << (sizeof(follow_chain_tests_expected_chain) / 2) << std::endl;
		return 8;
	}

	if(std::memcmp(follow_chain_tests_expected_chain, chain.data(), sizeof(follow_chain_tests_expected_chain)))
	{
		std::cerr << "Follow chain: Chain data mismatch!!!" << std::endl;
		
		std::cerr << "Expected chain:" << std::endl;
		for(const u16 cluster: follow_chain_tests_expected_chain)
			std::cerr << "\t" << cluster << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain: " << std::endl;
		print_vector(chain, 1);
		
		return 9;
	}

	/*Cluster val 0 should be treated as end-of-chain. Should we at least return
	some non-0 value?*/
	expected_err = 0;
	chain.clear();
	fstr.seekg(0);
	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)55, chain);

	if(err)
	{
		std::cerr << "Follow chain: Unexpected error!!!" << std::endl;
		std::cerr << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 10;
	}

	if(chain.size() != 1)
	{
		std::cerr << "Follow chain: Chain size mismatch!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", expected size: " << 1 << std::endl;
		print_vector(chain, 1);
		return 11;
	}

	if(chain[0] != 55)
	{
		std::cerr << "Follow chain: Chain data mismatch!!!" << std::endl;
		
		std::cerr << "Expected: " << (u16)55 << ", got: ";
		print_vector(chain, 1);

		return 12;
	}

	return 0;
}

int find_free_chain_disk_tests(std::fstream &fstr)
{
	u16 err, cluster;
	std::vector<u16> chain, chain_copy;

	fstr.seekg(0);
	cluster = FAT_utils::find_next_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS);

	if(cluster != 55)
	{
		std::cerr << "Find free chain: Next free cluster should be 55, got " << cluster << " instead!!!" << std::endl;
		return 13;
	}

	fstr.seekg(0);
	cluster = FAT_utils::find_next_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)55);

	if(cluster != 55)
	{
		std::cerr << "Find free chain: Offset should be included when searching for next free cluster!!!" << std::endl;
		return 14;
	}

	fstr.seekg(0);
	cluster = FAT_utils::find_next_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)56);

	if(cluster != 68)
	{
		std::cerr << "Find free chain: Next free cluster after 55 should be 68, got " << cluster << " instead!!!" << std::endl;
		return 15;
	}

	fstr.seekg(0);
	err = FAT_utils::find_free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)3, chain);

	if(err)
	{
		std::cerr << "Find free chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 16;
	}

	if(chain.size() != sizeof(find_free_chain_tests_expected_chain) / 2)
	{
		std::cerr << "Find free chain: Chain length mismatch!!!" << std::endl;
		std::cerr << "Expected " << (sizeof(find_free_chain_tests_expected_chain) / 2) << ", got " << chain.size() << std::endl;
		
		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		return 17;
	}

	if(std::memcmp(find_free_chain_tests_expected_chain, chain.data(), sizeof(find_free_chain_tests_expected_chain)))
	{
		std::cerr << "Find free chain: Chain data mismatch!!!" << std::endl;
		
		std::cerr << "Expected chain:" << std::endl;
		for(const u16 temp: find_free_chain_tests_expected_chain)
			std::cerr << "\"" << temp << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		return 18;
	}

	//mark found chain by hand
	for(u16 i = 0; i < chain.size() - 1; i++)
	{
		fstr.seekp(chain[i] * 2);
		fstr.write((char*)&chain[i + 1], 2);
	}

	fstr.seekp(chain.back() * 2);
	fstr.write((char*)&FAT_ATTRS.END_OF_CHAIN, 2);

	chain_copy = chain;

	//grow chain test
	fstr.seekg(0);
	err = FAT_utils::find_free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)9, chain);

	if(err)
	{
		std::cerr << "Grow chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		err = 55;
	}
	else if(chain.size() != sizeof(grow_chain_test_expected_chain) / 2)
	{
		std::cerr << "Grow chain: Chain length mismatch!!!" << std::endl;
		std::cerr << "Expected " << 9 << ", got " << chain.size() << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		err = 56;
	}
	else if(std::memcmp(chain.data(), grow_chain_test_expected_chain, chain.size()))
	{
		std::cerr << "Grow chain: Chain data mismatch!!!" << std::endl;

		std::cerr << "Expected chain:" << std::endl;
		for(const u16 temp: grow_chain_test_expected_chain)
			std::cerr << "\t" << temp << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		err = 59;
	}

	//free original found chain by hand
	for(const u16 cluster: chain_copy)
	{
		fstr.seekp(cluster * 2);
		fstr.write((char*)&FAT_ATTRS.FREE_CLUSTER, 2);
	}

	return err;
}

int write_chain_disk_tests(std::fstream &fstr)
{
	u16 expected_err, err, alt_err;
	std::vector<u16> chain, read_chain;
	
	fstr.seekp(0);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::EMPTY_CHAIN;
	err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 19;
	}

	fstr.seekp(0);
	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 20;
	}

	fstr.seekp(0);
	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 21;
	}

	/*This assumes the find_free_chain_disk tests have already been performed
	and went fine.*/
	fstr.seekg(0);
	chain.resize(0);
	err = FAT_utils::find_free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)8, chain);

	if(err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 22;
	}

	fstr.seekp(0);
	err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	/*from here on out we avoid returning early to attempt to delete the written
	chain even if something has gone wrong*/

	if(err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
	}

	/*This assumes the follow_chain_disk tests have already been performed and
	went fine.*/
	alt_err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain[0], read_chain);

	if(alt_err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << alt_err << std::endl;
		std::cerr << std::dec;
	}

	if(chain.size() != read_chain.size())
	{
		std::cerr << "Write chain: Chain size mismatch when reading chain back!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", Read chain size: " << read_chain.size() << std::endl;
		alt_err = 23;
	}
	else if(std::memcmp(chain.data(), read_chain.data(), chain.size() * 2))
	{
		std::cerr << "Write chain: Chain data mismatch when reading chain back!!!" << std::endl;
		
		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);
		std::cerr << std::endl;

		std::cerr << "Read chain:" << std::endl;
		print_vector(read_chain, 1);
		std::cerr << std::endl;

		alt_err = 24;
	}

	const u16 free_cluster_val =
		FAT_ATTRS.ENDIANNESS == std::endian::native ? FAT_ATTRS.FREE_CLUSTER : std::byteswap(FAT_ATTRS.FREE_CLUSTER);

	//delete written chain by hand
	for(u16 cluster: chain)
	{
		fstr.seekp(cluster * 2);
		fstr.write((char*)&free_cluster_val, 2);
	}

	if(fstr.fail())
	{
		std::cerr << "Write chain: Something went wrong when deleting the chain!!!" << std::endl;
		alt_err = 25;
	}

	return err << 16 | alt_err;
}

int free_chain_disk_tests(std::fstream &fstr)
{
	u16 expected_err, err, alt_err;
	std::vector<u16> chain;

	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::EMPTY_CHAIN;
	err = FAT_utils::free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 26;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	err = FAT_utils::free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 27;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	err = FAT_utils::free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 28;
	}

	/*This assumes the follow_chain_disk tests have already been performed and
	went fine.*/
	chain.resize(0);
	err = FAT_utils::follow_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)1, chain);

	if(err)
	{
		std::cerr << "Free chain: Unexpected error following chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 29;
	}

	err = FAT_utils::free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	/*from here on out we avoid returning early to attempt to rewrite the
	deleted chain even if something has gone wrong*/

	if(err)
	{
		std::cerr << "Free chain: Unexpected error freeing chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
	}

	for(u16 cluster: chain)
	{
		fstr.seekg(cluster * 2);
		fstr.read((char*)&cluster, 2);

		if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
			cluster = std::byteswap(cluster);

		if(cluster != FAT_ATTRS.FREE_CLUSTER)
		{
			std::cerr << "Free chain: A cluster wasn't freed!!!" << std::endl;
			std::cerr << "Expected " << FAT_ATTRS.FREE_CLUSTER << ", got " << cluster << std::endl;
			break;
		}
	}

	if(fstr.fail())
	{
		std::cerr << "Free chain: IO error when checking chain!!!" << std::endl;
		fstr.clear();
	}

	/*This assumes the write_chain_disk tests have already been performed and
	went fine.*/
	fstr.seekp(0);
	alt_err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);

	if(alt_err)
	{
		std::cerr << "Free chain: Unexpected error rewriting chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << alt_err << std::endl;
		std::cerr << std::dec;
	}

	return err << 16 | alt_err;
}

int shrink_chain_disk_tests(std::fstream &fstr)
{
	constexpr size_t expected_chain_len = sizeof(grow_chain_test_expected_chain) / sizeof(*grow_chain_test_expected_chain);

	u16 expected_err, err, alt_err, test_cls[2];
	std::vector<u16> chain;

	/*Shrink chain (disk) can only ever generate OOB errors and only if the
	last cluster that would still be used is OOB. It will, however, return any
	errors generated by free_chain. These are sanity checks to ensure those
	errors are being returned properly.*/

	//free_chain is called on the part of the chain that needs to be cut, we
	//need a huge chain with a target size of 0.
	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	fstr.seekp(0);
	err = FAT_utils::shrink_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain, (u16)0);

	if(err != expected_err)
	{
		std::cerr << "Shrink chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 61;
	}

	//test for the aforementioned OOB
	chain = std::vector<u16>(grow_chain_test_expected_chain, grow_chain_test_expected_chain + expected_chain_len);
	chain[chain.size() - 3] = FAT_DYNA_ATTRS.LENGTH;
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	fstr.seekp(0);
	err = FAT_utils::shrink_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain, (u16)(chain.size() - 2));

	if(err != expected_err)
	{
		std::cerr << "Shrink chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 62;
	}

	chain = std::vector<u16>(grow_chain_test_expected_chain, grow_chain_test_expected_chain + expected_chain_len);
	expected_err = 0;

	fstr.seekp(0);
	err = FAT_utils::write_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);
	if(err)
	{
		std::cerr << "Shrink chain: Write chain error!!!" << std::endl;
		std::cerr << "0x" << err << std::endl;
		std::cerr << std::dec;
		return 63;
	}

	fstr.seekp(0);
	err = FAT_utils::shrink_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain, (u16)(chain.size() - 2));
	if(err)
	{
		std::cerr << "Shrink chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << err << std::endl;
		std::cerr << std::dec;
		return 64;
	}

	fstr.seekg(chain[chain.size() - 2] * 2);
	fstr.read((char*)test_cls, 2);

	fstr.seekg(chain[chain.size() - 1] * 2);
	fstr.read((char*)(test_cls + 1), 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
	{
		for(u16 &cls: test_cls) cls = std::byteswap(cls);
	}

	if(test_cls[0] != FAT_ATTRS.FREE_CLUSTER
		|| test_cls[1] != FAT_ATTRS.FREE_CLUSTER)
	{
		std::cerr << "Shrink chain: Failed to free partial chain!!!" << std::endl;
		return 65;
	}

	fstr.seekg(chain[chain.size() - 3] * 2);
	fstr.read((char*)test_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		test_cls[0] = std::byteswap(test_cls[0]);

	if(test_cls[0] != FAT_ATTRS.END_OF_CHAIN)
	{
		std::cerr << "Shrink chain: Failed to write new end of chain!!!" << std::endl;
		return 66;
	}

	return FAT_utils::free_chain(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, chain);
}

int follow_chain_memory_tests(const u16 FAT[])
{
	std::vector<u16> chain;

	u16 expected_err, err;

	expected_err = (FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 30;
	}

	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)(FAT_ATTRS.DATA_MIN - 1), chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 31;
	}

	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)(FAT_ATTRS.DATA_MAX + 1), chain);

	if(err != expected_err)
	{
		std::cerr << "Follow chain: Unexpected error value!!!" << std::endl;
		std::cerr << "Expected bad start (0x" << std::hex << expected_err << "), got 0x" << err << "." << std::endl;
		std::cerr << std::dec;
		return 32;
	}

	expected_err = 0;
	chain.clear();
	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)1, chain);

	if(err)
	{
		std::cerr << "Follow chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 33;
	}

	if(chain.size() != (sizeof(follow_chain_tests_expected_chain) / 2))
	{
		std::cerr << "Follow chain: Chain size mismatch!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", expected size: " << (sizeof(follow_chain_tests_expected_chain) / 2) << std::endl;
		return 34;
	}

	if(std::memcmp(follow_chain_tests_expected_chain, chain.data(), sizeof(follow_chain_tests_expected_chain)))
	{
		std::cerr << "Follow chain: Chain data mismatch!!!" << std::endl;

		std::cerr << "Expected chain:" << std::endl;
		for(const u16 cluster: follow_chain_tests_expected_chain)
			std::cerr << "\t" << cluster << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain: " << std::endl;
		print_vector(chain, 1);

		return 35;
	}

	/*Cluster val 0 should be treated as end-of-chain. Should we at least return
	some non-0 value?*/
	expected_err = 0;
	chain.clear();
	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)55, chain);

	if(err)
	{
		std::cerr << "Follow chain: Unexpected error!!!" << std::endl;
		std::cerr << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 36;
	}

	if(chain.size() != 1)
	{
		std::cerr << "Follow chain: Chain size mismatch!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", expected size: " << 1 << std::endl;
		print_vector(chain, 1);
		return 37;
	}

	if(chain[0] != 55)
	{
		std::cerr << "Follow chain: Chain data mismatch!!!" << std::endl;

		std::cerr << "Expected: " << (u16)55 << ", got: ";
		print_vector(chain, 1);

		return 38;
	}

	return 0;
}

int find_free_chain_memory_tests(u16 FAT[])
{
	u16 err, cluster;
	std::vector<u16> chain, chain_copy;

	cluster = FAT_utils::find_next_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH);

	if(cluster != 55)
	{
		std::cerr << "Find free chain: Next free cluster should be 55, got " << cluster << " instead!!!" << std::endl;
		return 39;
	}

	cluster = FAT_utils::find_next_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)55);

	if(cluster != 55)
	{
		std::cerr << "Find free chain: Offset should be included when searching for next free cluster!!!" << std::endl;
		return 40;
	}

	cluster = FAT_utils::find_next_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)56);

	if(cluster != 68)
	{
		std::cerr << "Find free chain: Next free cluster after 55 should be 68, got " << cluster << " instead!!!" << std::endl;
		return 41;
	}

	err = FAT_utils::find_free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)3, chain);

	if(err)
	{
		std::cerr << "Find free chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 42;
	}

	if(chain.size() != sizeof(find_free_chain_tests_expected_chain) / 2)
	{
		std::cerr << "Find free chain: Chain length mismatch!!!" << std::endl;
		std::cerr << "Expected " << (sizeof(find_free_chain_tests_expected_chain) / 2) << ", got " << chain.size() << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		return 43;
	}

	if(std::memcmp(find_free_chain_tests_expected_chain, chain.data(), sizeof(find_free_chain_tests_expected_chain)))
	{
		std::cerr << "Find free chain: Chain data mismatch!!!" << std::endl;

		std::cerr << "Expected chain:" << std::endl;
		for(const u16 temp: find_free_chain_tests_expected_chain)
			std::cerr << "\t" << temp << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		return 44;
	}

	//mark found chain by hand
	for(u16 i = 0; i < chain.size() - 1; i++)
		FAT[chain[i]] = chain[i + 1];

	FAT[chain.back()] = FAT_ATTRS.END_OF_CHAIN;
	chain_copy = chain;

	//grow chain test
	err = FAT_utils::find_free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)9, chain);

	if(err)
	{
		std::cerr << "Find free chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		err = 57;
	}
	else if(chain.size() != sizeof(grow_chain_test_expected_chain) / 2)
	{
		std::cerr << "Find free chain: Chain length mismatch!!!" << std::endl;
		std::cerr << "Expected " << 9 << ", got " << chain.size() << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		err = 58;
	}
	else if(std::memcmp(chain.data(), grow_chain_test_expected_chain, chain.size()))
	{
		std::cerr << "Grow chain: Chain data mismatch!!!" << std::endl;

		std::cerr << "Expected chain:" << std::endl;
		for(const u16 temp: grow_chain_test_expected_chain)
			std::cerr << "\t" << temp << std::endl;
		std::cerr << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);

		err =  60;
	}

	//free original found chain by hand
	for(const u16 cluster: chain_copy)
		FAT[cluster] = FAT_ATTRS.FREE_CLUSTER;

	return err;
}

int write_chain_memory_tests(u16 FAT[])
{
	u16 expected_err, err, alt_err;
	std::vector<u16> chain, read_chain;

	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::EMPTY_CHAIN;
	err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 45;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 46;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Write chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 47;
	}

	/*This assumes the find_free_chain_disk tests have already been performed
	and went fine.*/
	chain.resize(0);
	err = FAT_utils::find_free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)8, chain);

	if(err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 48;
	}

	err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	/*from here on out we avoid returning early to attempt to delete the written
	chain even if something has gone wrong*/

	if(err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
	}

	/*This assumes the follow_chain_disk tests have already been performed and
	went fine.*/
	alt_err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain[0], read_chain);

	if(alt_err)
	{
		std::cerr << "Write chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << alt_err << std::endl;
		std::cerr << std::dec;
	}

	if(chain.size() != read_chain.size())
	{
		std::cerr << "Write chain: Chain size mismatch when reading chain back!!!" << std::endl;
		std::cerr << "Chain size: " << chain.size() << ", Read chain size: " << read_chain.size() << std::endl;
		alt_err = 49;
	}
	else if(std::memcmp(chain.data(), read_chain.data(), chain.size() * 2))
	{
		std::cerr << "Write chain: Chain data mismatch when reading chain back!!!" << std::endl;

		std::cerr << "Chain:" << std::endl;
		print_vector(chain, 1);
		std::cerr << std::endl;

		std::cerr << "Read chain:" << std::endl;
		print_vector(read_chain, 1);
		std::cerr << std::endl;

		alt_err = 50;
	}

	const u16 free_cluster_val =
		FAT_ATTRS.ENDIANNESS == std::endian::native ? FAT_ATTRS.FREE_CLUSTER : std::byteswap(FAT_ATTRS.FREE_CLUSTER);

	//delete written chain by hand
	for(const u16 cluster: chain)
		FAT[cluster] = free_cluster_val;

	return err << 16 | alt_err;
}

int free_chain_memory_tests(u16 FAT[])
{
	u16 expected_err, err, alt_err;
	std::vector<u16> chain;

	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::EMPTY_CHAIN;
	err = FAT_utils::free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 51;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	err = FAT_utils::free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 52;
	}

	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	err = FAT_utils::free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(err != expected_err)
	{
		std::cerr << "Free chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 53;
	}

	/*This assumes the follow_chain_disk tests have already been performed and
	went fine.*/
	chain.resize(0);
	err = FAT_utils::follow_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)1, chain);

	if(err)
	{
		std::cerr << "Free chain: Unexpected error following chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 54;
	}

	err = FAT_utils::free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	/*from here on out we avoid returning early to attempt to rewrite the
	deleted chain even if something has gone wrong*/

	if(err)
	{
		std::cerr << "Free chain: Unexpected error freeing chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
	}

	for(const u16 cluster: chain)
	{
		if(FAT[cluster] != FAT_ATTRS.FREE_CLUSTER)
		{
			std::cerr << "Free chain: A cluster wasn't freed!!!" << std::endl;
			std::cerr << "Expected " << FAT_ATTRS.FREE_CLUSTER << ", got " << cluster << std::endl;
			break;
		}
	}

	/*This assumes the write_chain_disk tests have already been performed and
	went fine.*/
	alt_err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);

	if(alt_err)
	{
		std::cerr << "Free chain: Unexpected error rewriting chain!!!" << std::endl;
		std::cerr << "0x" << std::hex << alt_err << std::endl;
		std::cerr << std::dec;
	}

	return err << 16 | alt_err;
}

//shrink_chain itself uses free_chain, so free_chain is required. The test also
//expects that write_chain will already have been tested.
int shrink_chain_memory_tests(u16 FAT[])
{
	constexpr size_t expected_chain_len = sizeof(grow_chain_test_expected_chain) / sizeof(*grow_chain_test_expected_chain);

	u16 expected_err, err, alt_err;
	std::vector<u16> chain;

	/*Shrink chain (memory) can only ever generate OOB errors and only if the
	last cluster that would still be used is OOB. It will, however, return any
	errors generated by free_chain. These are sanity checks to ensure those
	errors are being returned properly.*/

	//free_chain is called on the part of the chain that needs to be cut, we
	//need a huge chain with a target size of 0.
	chain.resize(FAT_DYNA_ATTRS.LENGTH - FAT_ATTRS.DATA_MIN + 1);
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_TOO_LARGE;
	err = FAT_utils::shrink_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain, (u16)0);

	if(err != expected_err)
	{
		std::cerr << "Shrink chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 55;
	}

	//test for the aforementioned OOB
	chain = std::vector<u16>(grow_chain_test_expected_chain, grow_chain_test_expected_chain + expected_chain_len);
	chain[chain.size() - 3] = FAT_DYNA_ATTRS.LENGTH;
	expected_err = FAT_utils::LIBRARY_ID << 8 | (u8)FAT_utils::ERR::CHAIN_OOB;
	err = FAT_utils::shrink_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain, (u16)(chain.size() - 2));

	if(err != expected_err)
	{
		std::cerr << "Shrink chain: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 56;
	}

	chain = std::vector<u16>(grow_chain_test_expected_chain, grow_chain_test_expected_chain + expected_chain_len);
	expected_err = 0;
	
	err = FAT_utils::write_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);
	if(err)
	{
		std::cerr << "Shrink chain: Write chain error!!!" << std::endl;
		std::cerr << "0x" << err << std::endl;
		std::cerr << std::dec;
		return 57;
	}

	err = FAT_utils::shrink_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain, (u16)(chain.size() - 2));
	if(err)
	{
		std::cerr << "Shrink chain: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << err << std::endl;
		std::cerr << std::dec;
		return 58;
	}

	if(FAT[chain[chain.size() - 1]] != FAT_ATTRS.FREE_CLUSTER
		|| FAT[chain[chain.size() - 2]] != FAT_ATTRS.FREE_CLUSTER)
	{
		std::cerr << "Shrink chain: Failed to free partial chain!!!" << std::endl;
		return 59;
	}

	if(FAT[chain[chain.size() - 3]] != FAT_ATTRS.END_OF_CHAIN)
	{
		std::cerr << "Shrink chain: Failed to write new end of chain!!!" << std::endl;
		return 60;
	}

	return FAT_utils::free_chain(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, chain);
}

static int get_nth_cluster_memory_tests(u16 FAT[])
{
	u16 cls, expected_cls, err, expected_err;

	cls = FAT_ATTRS.END_OF_CHAIN;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)1);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 67;
	}

	cls = 0x43;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::END_OF_CHAIN);
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)1);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 68;
	}

	cls = 0x43;
	expected_cls = 0x43;
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)0);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 69;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 70;
	}

	cls = 0x42;
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)1);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 71;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 72;
	}

	cls = get_nth_cluster_expected_chain[0];
	expected_cls = get_nth_cluster_expected_chain[7];
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)7);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 73;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 74;
	}

	cls = get_nth_cluster_expected_chain[0];
	expected_cls = get_nth_cluster_expected_chain[7];
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::END_OF_CHAIN);
	err = FAT_utils::get_nth_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, cls, (u16)8);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 75;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 76;
	}

	return 0;
}

static int get_nth_cluster_disk_tests(std::fstream &fstr)
{
	u16 cls, expected_cls, err, expected_err;

	cls = FAT_ATTRS.END_OF_CHAIN;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)1);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 77;
	}

	cls = 0x43;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::END_OF_CHAIN);
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)1);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 78;
	}

	cls = 0x43;
	expected_cls = 0x43;
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)0);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 79;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 80;
	}

	cls = 0x42;
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)1);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 81;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 82;
	}

	cls = get_nth_cluster_expected_chain[0];
	expected_cls = get_nth_cluster_expected_chain[7];
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)7);
	if(err)
	{
		std::cerr << "Get nth cluster: Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::endl;
		std::cerr << std::dec;
		return 83;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 84;
	}

	cls = get_nth_cluster_expected_chain[0];
	expected_cls = get_nth_cluster_expected_chain[7];
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::END_OF_CHAIN);
	err = FAT_utils::get_nth_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, cls, (u16)8);
	if(err != expected_err)
	{
		std::cerr << "Get nth cluster: Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 85;
	}

	if(cls != expected_cls)
	{
		std::cerr << "Get nth cluster: Cluster address mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_cls << ", got 0x" << cls << std::endl;
		std::cerr << std::dec;
		return 86;
	}

	return 0;
}

static int get_next_or_free_cluster_memory_tests(u16 FAT[])
{
	u16 err, expected_err, next_cls, temp;

	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::get_next_or_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)(FAT_ATTRS.DATA_MAX + 1), next_cls);
	if(err != expected_err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 87;
	}

	temp = FAT[FAT_ATTRS.DATA_MIN];
	FAT[FAT_ATTRS.DATA_MIN] = FAT_DYNA_ATTRS.LENGTH;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::CHAIN_OOB);
	err = FAT_utils::get_next_or_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, FAT_ATTRS.DATA_MIN, next_cls);
	if(err != expected_err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 88;
	}

	FAT[FAT_ATTRS.DATA_MIN] = temp;

	temp = 0x126;
	err = FAT_utils::get_next_or_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)0x65, next_cls);
	if(err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 89;
	}

	if(next_cls != temp)
	{
		std::cerr << "Cluster value mismatch!!!" << std::endl;
		std::cerr << "Expected " << temp << ", got " << next_cls << std::endl;
		return 90;
	}

	temp = 0x37;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::ALLOC);
	err = FAT_utils::get_next_or_free_cluster(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH, (u16)0x32, next_cls);
	if(err != expected_err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 91;
	}

	if(next_cls != temp)
	{
		std::cerr << "Cluster value mismatch!!!" << std::endl;
		std::cerr << "Expected " << temp << ", got " << next_cls << std::endl;
		return 92;
	}

	return 0;
}

static int get_next_or_free_cluster_disk_tests(std::fstream &fstr)
{
	u16 err, expected_err, next_cls, temp;

	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::BAD_START);
	err = FAT_utils::get_next_or_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)(FAT_ATTRS.DATA_MAX + 1), next_cls);
	if(err != expected_err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 93;
	}

	fstr.seekg(FAT_ATTRS.DATA_MIN * sizeof(FAT_ATTRS.DATA_MIN));
	fstr.read((char*)&temp, sizeof(FAT_ATTRS.DATA_MIN));

	next_cls = FAT_DYNA_ATTRS.LENGTH;
	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		next_cls = std::byteswap(next_cls);

	fstr.seekp(FAT_ATTRS.DATA_MIN * sizeof(FAT_ATTRS.DATA_MIN));
	fstr.write((char*)&next_cls, sizeof(FAT_ATTRS.DATA_MIN));

	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::CHAIN_OOB);
	err = FAT_utils::get_next_or_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, FAT_ATTRS.DATA_MIN, next_cls);
	if(err != expected_err)
	{
		//one last desperate attempt to clean up
		fstr.seekp(FAT_ATTRS.DATA_MIN * sizeof(FAT_ATTRS.DATA_MIN));
		fstr.write((char*)&temp, sizeof(FAT_ATTRS.DATA_MIN));

		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 94;
	}

	fstr.seekp(FAT_ATTRS.DATA_MIN * sizeof(FAT_ATTRS.DATA_MIN));
	fstr.write((char*)&temp, sizeof(FAT_ATTRS.DATA_MIN));

	temp = 0x126;
	err = FAT_utils::get_next_or_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)0x65, next_cls);
	if(err)
	{
		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		return 95;
	}

	if(next_cls != temp)
	{
		std::cerr << "Cluster value mismatch!!!" << std::endl;
		std::cerr << "Expected " << temp << ", got " << next_cls << std::endl;
		return 96;
	}

	temp = 0x37;
	expected_err = ret_val_setup(FAT_utils::LIBRARY_ID, (u8)FAT_utils::ERR::ALLOC);
	err = FAT_utils::get_next_or_free_cluster(fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)0x32, next_cls);
	if(err != expected_err)
	{
		std::cerr << "Error value mismatch!!!" << std::endl;
		std::cerr << "Expected 0x" << std::hex << expected_err << ", got 0x" << err << std::endl;
		std::cerr << std::dec;
		return 97;
	}

	if(next_cls != temp)
	{
		std::cerr << "Cluster value mismatch!!!" << std::endl;
		std::cerr << "Expected " << temp << ", got " << next_cls << std::endl;
		return 98;
	}

	return 0;
}

static int count_free_clusters_memory_tests(u16 FAT[])
{
	u16 free_cluster_count;

	free_cluster_count = FAT_utils::count_free_clusters(FAT, FAT_ATTRS, FAT_DYNA_ATTRS.LENGTH);
	if(free_cluster_count != EXPECTED_FREE_CLUSTER_COUNT)
	{
		std::cerr << "Mismatched free cluster count!!!" << std::endl;
		std::cerr << "Expected " << EXPECTED_FREE_CLUSTER_COUNT << ", got ";
		std::cerr << free_cluster_count << std::endl;
		return 99;
	}

	return 0;
}

static int count_free_clusters_disk_tests(std::fstream &fstr)
{
	u16 free_cluster_count;

	free_cluster_count = FAT_utils::count_free_clusters(fstr, FAT_ATTRS, FAT_DYNA_ATTRS);
	if(free_cluster_count != EXPECTED_FREE_CLUSTER_COUNT)
	{
		std::cerr << "Mismatched free cluster count!!!" << std::endl;
		std::cerr << "Expected " << EXPECTED_FREE_CLUSTER_COUNT << ", got ";
		std::cerr << free_cluster_count << std::endl;
		return 100;
	}

	return 0;
}

static void unfuck_FAT(u16 FAT[], std::fstream &fstr, const u16 TEST_IDX, const u16 prev_val)
{
	u16 n_endian_cls;

	FAT[TEST_IDX] = prev_val;
	n_endian_cls = prev_val;

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	fstr.seekp(TEST_IDX * 2);
	fstr.write((char*)&n_endian_cls, 2);
}

static int write_cluster_writethrough_tests(u16 FAT[], std::fstream &fstr)
{
	constexpr u8 TEST_IDX = 1;
	constexpr u16 TEST_VAL = 369;

	const u16 prev_val = FAT[TEST_IDX];

	u16 n_endian_cls;

	fstr.seekg(TEST_IDX * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(prev_val != n_endian_cls)
	{
		std::cerr << "On-disk and in-memory val mismatch!!!" << std::endl;
		std::cerr << "On-disk: 0x" << std::hex << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val << std::dec << std::endl;
		return 101;
	}

	const u16 err = FAT_utils::write_cluster(FAT, fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)TEST_IDX, (u16)TEST_VAL);

	if(err)
	{
		unfuck_FAT(FAT, fstr, TEST_IDX, prev_val);

		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		return 102;
	}

	fstr.seekg(TEST_IDX * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(FAT[TEST_IDX] != TEST_VAL || n_endian_cls != TEST_VAL)
	{
		unfuck_FAT(FAT, fstr, TEST_IDX, prev_val);

		std::cerr << "Mismatched cluster value!!!" << std::endl;
		std::cerr << "Expected: 0x" << std::hex << TEST_VAL << std::endl;
		std::cerr << "On-disk: 0x" << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val << std::dec << std::endl;
		return 103;
	}

	unfuck_FAT(FAT, fstr, TEST_IDX, prev_val);

	if(!fstr.good())
	{
		std::cerr << "Error on restoring FAT state!!!" << std::endl;
		return 104;
	}

	return 0;
}

static int extend_chain_writethrough_tests(u16 FAT[], std::fstream &fstr)
{
	constexpr u8 TEST_IDX = 1;
	constexpr u16 TEST_VAL = 369;

	const u16 prev_val[2] = {FAT[TEST_IDX], FAT[TEST_VAL]};

	u16 n_endian_cls;

	fstr.seekg(TEST_IDX * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(prev_val[0] != n_endian_cls)
	{
		std::cerr << "On-disk and in-memory val mismatch!!!" << std::endl;
		std::cerr << "On-disk: 0x" << std::hex << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val[0] << std::dec << std::endl;
		return 105;
	}

	fstr.seekg(TEST_VAL * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(prev_val[1] != n_endian_cls)
	{
		std::cerr << "On-disk and in-memory val mismatch!!!" << std::endl;
		std::cerr << "On-disk: 0x" << std::hex << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val[1] << std::dec << std::endl;
		return 106;
	}

	const u16 err = FAT_utils::extend_chain(FAT, fstr, FAT_ATTRS, FAT_DYNA_ATTRS, (u16)TEST_IDX, (u16)TEST_VAL);

	if(err)
	{
		unfuck_FAT(FAT, fstr, TEST_IDX, prev_val[0]);
		unfuck_FAT(FAT, fstr, TEST_VAL, prev_val[1]);

		std::cerr << "Unexpected error!!!" << std::endl;
		std::cerr << "0x" << std::hex << err << std::dec << std::endl;
		return 107;
	}

	fstr.seekg(TEST_IDX * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(FAT[TEST_IDX] != TEST_VAL || n_endian_cls != TEST_VAL)
	{
		unfuck_FAT(FAT, fstr, TEST_IDX, prev_val[0]);
		unfuck_FAT(FAT, fstr, TEST_VAL, prev_val[1]);

		std::cerr << "Mismatched cluster value!!!" << std::endl;
		std::cerr << "Expected: 0x" << std::hex << TEST_VAL << std::endl;
		std::cerr << "On-disk: 0x" << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val[0] << std::dec << std::endl;
		return 108;
	}

	fstr.seekg(TEST_VAL * 2);
	fstr.read((char*)&n_endian_cls, 2);

	if constexpr(FAT_ATTRS.ENDIANNESS != std::endian::native)
		n_endian_cls = std::byteswap(n_endian_cls);

	if(FAT[TEST_VAL] != FAT_ATTRS.END_OF_CHAIN || n_endian_cls != FAT_ATTRS.END_OF_CHAIN)
	{
		unfuck_FAT(FAT, fstr, TEST_IDX, prev_val[0]);
		unfuck_FAT(FAT, fstr, TEST_VAL, prev_val[1]);

		std::cerr << "Mismatched cluster value!!!" << std::endl;
		std::cerr << "Expected: 0x" << std::hex << TEST_VAL << std::endl;
		std::cerr << "On-disk: 0x" << n_endian_cls << std::endl;
		std::cerr << "In-memory: 0x" << prev_val[1] << std::dec << std::endl;
		return 109;
	}

	unfuck_FAT(FAT, fstr, TEST_IDX, prev_val[0]);
	unfuck_FAT(FAT, fstr, TEST_VAL, prev_val[1]);

	if(!fstr.good())
	{
		std::cerr << "Error on restoring FAT state!!!" << std::endl;
		return 110;
	}

	return 0;
}

/*IMPORTANT: All tests clean up after themselves and all tests expect a clean
FAT state (THIS DOES NOT MEAN EMPTY). Anyone adding new tests should ensure
these also clean up after themselves*/
int main()
{
	constexpr char TEST_DATA_PATH[] = "FAT_test_data.bin";

	std::fstream fstr;
	std::unique_ptr<u16[]> FAT;
	int err;

	if(!std::filesystem::exists(TEST_DATA_PATH))
	{
		std::cerr << "Test data file \"" << TEST_DATA_PATH << "\" does not exist!!!" << std::endl;
		return 1;
	}

	if(!std::filesystem::is_regular_file(TEST_DATA_PATH))
	{
		std::cerr << "\"" << TEST_DATA_PATH << "\" is not a file!!!" << std::endl;
		return 2;
	}

	fstr.open(TEST_DATA_PATH, std::fstream::binary | std::fstream::in | std::fstream::out);

	if(!fstr.is_open() || !fstr.good())
	{
		std::cerr << "Could not open test data file!!!" << std::endl;
		return 3;
	}

	FAT = std::make_unique<u16[]>(FAT_DYNA_ATTRS.LENGTH);

	fstr.seekg(0);
	fstr.read((char*)FAT.get(), FAT_DYNA_ATTRS.LENGTH * 2);

	std::cout << "Count free clusters (memory) tests..." << std::endl;
	err = count_free_clusters_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Count free clusters (memory) OK!" << std::endl;

	std::cout << "Get nth cluster (memory) tests..." << std::endl;
	err = get_nth_cluster_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Get nth cluster (memory) OK!" << std::endl;

	std::cout << "Get next or free cluster (memory) tests..." << std::endl;
	err = get_next_or_free_cluster_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Get next or free cluster (memory) OK!" << std::endl;

	std::cout << "Follow chain (memory) tests..." << std::endl;
	err = follow_chain_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Follow chain (memory) OK!" << std::endl;

	std::cout << "Find free chain (memory) tests..." << std::endl;
	err = find_free_chain_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Find free chain (memory) OK!" << std::endl;

	std::cout << "Write chain (memory) tests..." << std::endl;
	err = write_chain_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Write chain (memory) OK!" << std::endl;

	std::cout << "Free chain (memory) tests..." << std::endl;
	err = free_chain_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Free chain (memory) OK!" << std::endl;

	std::cout << "Shrink chain (memory) tests..." << std::endl;
	err = shrink_chain_memory_tests(FAT.get());
	if(err) return err;
	std::cout << "Shrink chain (memory) OK!" << std::endl;

	/*---------------------------End of memory tests--------------------------*/
	std::cout << std::endl;

	std::cout << "Count free clusters (disk) tests..." << std::endl;
	err = count_free_clusters_disk_tests(fstr);
	if(err) return err;
	std::cout << "Count free clusters (disk) OK!" << std::endl;

	std::cout << "Get nth cluster (disk) tests..." << std::endl;
	err = get_nth_cluster_disk_tests(fstr);
	if(err) return err;
	std::cout << "Get nth cluster (disk) OK!" << std::endl;

	std::cout << "Get next or free cluster (disk) tests..." << std::endl;
	err = get_next_or_free_cluster_disk_tests(fstr);
	if(err) return err;
	std::cout << "Get next or free cluster (disk) OK!" << std::endl;

	std::cout << "Follow chain (disk) tests..." << std::endl;
	err = follow_chain_disk_tests(fstr);
	if(err) return err;
	std::cout << "Follow chain (disk) OK!" << std::endl;

	std::cout << "Find free chain (disk) tests..." << std::endl;
	err = find_free_chain_disk_tests(fstr);
	if(err) return err;
	std::cout << "Find free chain (disk) OK!" << std::endl;

	std::cout << "Write chain (disk) tests..." << std::endl;
	err = write_chain_disk_tests(fstr);
	if(err) return err;
	std::cout << "Write chain (disk) OK!" << std::endl;

	std::cout << "Free chain (disk) tests..." << std::endl;
	err = free_chain_disk_tests(fstr);
	if(err) return err;
	std::cout << "Free chain (disk) OK!" << std::endl;

	std::cout << "Shrink chain (disk) tests..." << std::endl;
	err = shrink_chain_disk_tests(fstr);
	if(err) return err;
	std::cout << "Shrink chain (disk) OK!" << std::endl;

	/*----------------------------End of disk tests---------------------------*/
	std::cout << std::endl;

	std::cout << "Write cluster (writethrough) tests..." << std::endl;
	err = write_cluster_writethrough_tests(FAT.get(), fstr);
	if(err) return err;
	std::cout << "Write cluster (writethrough) OK!" << std::endl;

	std::cout << "Extend chain (writethrough) tests..." << std::endl;
	err = extend_chain_writethrough_tests(FAT.get(), fstr);
	if(err) return err;
	std::cout << "Extend chain (writethrough) OK!" << std::endl;

	/*------------------------End of writethrough tests-----------------------*/
	std::cout << std::endl;
	std::cout << "ALL TESTS OK!!!" << std::endl;
	return 0;
}
