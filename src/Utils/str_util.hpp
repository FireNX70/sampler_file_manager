#ifndef STR_UTIL_HEADER_INCLUDE_GUARD
#define STR_UTIL_HEADER_INCLUDE_GUARD

#include <vector>
#include <string>
#include <sstream>
#include <limits>
#include <string_view>

#include "ints.hpp"

namespace str_util
{
	template <typename T>
	void rtrim(std::basic_string<T> &str)
	{
		using str_size_type = std::basic_string<T>::size_type;

		str_size_type pos = str.size() - 1;

		while(pos != std::numeric_limits<str_size_type>::max()
			&& (!str[pos] || str[pos] == ' ')) pos--;

		str.resize(pos + 1);
	}

	template <typename T, const bool skip_empty = true>
	std::vector<std::basic_string<T>> split(std::basic_string<T> str,
											std::basic_string<T> delimiter)
	{
		size_t off, nxt_delim;
		std::basic_string<T> temp;
		std::vector<std::basic_string<T>> strings;

		off = 0;

		do
		{
			nxt_delim = str.find(delimiter, off);
			temp = str.substr(off, nxt_delim - off);
			off = nxt_delim + 1;

			if constexpr(skip_empty)
			{
				if(temp.length() > 0) strings.push_back(temp);
			}
			else strings.push_back(temp);
		}
		while(nxt_delim != std::string::npos);

		return strings;
	}

	template <typename T>
	std::vector<std::basic_string<T>> split_command(std::basic_string<T> command)
	{
		u8 ignore_spaces, escape;
		T last_quote;
		size_t off;
		std::basic_stringstream<T> sstream;
		std::vector<std::basic_string<T>> strings;

		ignore_spaces = 0;
		escape = 0;
		off = command.find(" ");
		strings.push_back(command.substr(0, off));

		for(size_t i = off; i < command.length(); i++)
		{
			if(!escape)
			{
				if(command[i] == "\\")
				{
					escape = 1;
					continue;
				}
				else if(command[i] == '"' || command[i] == '\'')
				{
					ignore_spaces = !ignore_spaces;
					last_quote = command[i];
					continue;
				}
				else if(!ignore_spaces && command[i] == " ")
				{
					strings.push_back(sstream.str());
					sstream.str("");
					continue;
				}
			}
			else escape = 0;

			sstream << command[i];
		}

		if(!sstream.empty()) strings.push_back(sstream.str());

		return strings;
	}

	constexpr std::string_view SIZE_SUFFIXES[] =
	{
		"B",
		"KB",
		"MB",
		"GB",
		"TB",
		"PB" //We shouldn't even really need this, or even TB
	};

	constexpr size_t SUFFIX_CNT = sizeof(SIZE_SUFFIXES)
		/ sizeof(std::string_view);
	constexpr size_t MAX_SUFFIX_IDX = SUFFIX_CNT - 1;

	template <typename T>
	std::basic_string<T> size_to_str(uintmax_t size)
	{
		u8 suffix_idx;

		suffix_idx = 0;
		while(size >= 1024 && suffix_idx < MAX_SUFFIX_IDX)
		{
			size /= 1024;
			suffix_idx++;
		}

		return std::to_string(size) + " "
			+ std::basic_string<T>(SIZE_SUFFIXES[suffix_idx]);
	}
}

#endif //!STR_UTIL_HEADER_INCLUDE_GUARD
