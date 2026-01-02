#include <iostream>

#include "ints.hpp"

void print_unexpected_err(const u16 err, const uintmax_t ret_code)
{
	std::cerr << "Unexpected error!!!" << std::endl;
	std::cerr << "0x" << std::hex << err << std::dec << std::endl;
	std::cerr << "Exit: " << ret_code << std::endl;
}

void print_expected_err(const u16 expected_err, const u16 err, const uintmax_t ret_code)
{
	std::cerr << "Error mismatch!!!" << std::endl;
	std::cerr << "Expected 0x" << std::hex << expected_err;
	std::cerr << ", got 0x" << err << std::dec << std::endl;
	std::cerr << "Exit: " << ret_code << std::endl;
}

void print_buffer(const void *buffer, const size_t size)
{
	constexpr size_t INNER_ITERS = 16;
	const size_t outer_iters = size / INNER_ITERS;
	const size_t leftover = size % INNER_ITERS;

	for(size_t i = 0; i < outer_iters; i++)
	{
		std::cerr << "\t";

		for(size_t j = 0; j < INNER_ITERS; j++)
			std::cerr << (u16)((unsigned char *)buffer)[i * INNER_ITERS + j] << " ";

		std::cerr << std::endl;
	}

	if(leftover)
	{
		std::cerr << "\t";

		for(size_t i = leftover; i > 0; i--)
			std::cerr << (u16)((unsigned char *)buffer)[size - leftover] << " ";

		std::cerr << std::endl;
	}
}

void print_buffers(const void *buffer_a, const void *buffer_b, const size_t size)
{
	std::cerr << "Buffer A:" << std::endl;
	print_buffer(buffer_a, size);

	std::cerr << std::endl;

	std::cerr << "Buffer B:" << std::endl;
	print_buffer(buffer_b, size);
}