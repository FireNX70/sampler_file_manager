#ifndef TESTING_HELPERS_HEADER_INCLUDE_GUARD
#define TESTING_HELPERS_HEADER_INCLUDE_GUARD

#include "ints.hpp"

void print_unexpected_err(const u16 err, const uintmax_t ret_code);
void print_expected_err(const u16 expected_err, const u16 err, const uintmax_t ret_code);
void print_buffer(const void *buffer, const size_t size);
void print_buffers(const void *buffer_a, const void *buffer_b, const size_t size);

#endif // !TESTING_HELPERS_HEADER_INCLUDE_GUARD