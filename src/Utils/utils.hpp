#ifndef UTILS_HEADER_GUARD
#define UTILS_HEADER_GUARD

#include <cstdint>
#include <concepts>

#include "ints.hpp"

//maybe wrap this in a namespace

template <typename T>
requires std::integral<T>
constexpr T div_int_round_to_pos_inf(const T a, const T b)
{
	return a / b + ((a % b) != 0);
}

constexpr u16 ret_val_setup(const u8 library_ID, const u8 err)
{
	return (library_ID << 8) | err;
}

#endif