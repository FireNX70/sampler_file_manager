#include <vector>
#include <string>
#include <iostream>

#include "Utils/ints.hpp"
#include "Utils/str_util.hpp"

int main()
{
	u8 err;
	size_t str_cnt;

	err = 0;

	std::vector<std::string> strings = str_util::split((std::string)"/dir/file", (std::string)"/");
	
	str_cnt = strings.size();

	std::cout << "string count: " << str_cnt << std::endl;

	for(std::string str: strings)
		std::cout << str << std::endl;
	
	if(str_cnt != 2) err |= 1;

	strings = str_util::split((std::string)"////////delim/////////test", (std::string)"/");

	str_cnt = strings.size();

	std::cout << "string count: " << str_cnt << std::endl;

	for(std::string str : strings)
		std::cout << str << std::endl;

	if(str_cnt != 2) err |= 2;
	
	return err;
}