#include "dir_history.hpp"

dir_history_t::dir_history_t(const std::filesystem::path &start_path)
{
	cur_point = 0;
	his_size = 0;
	back_off = 0;

	history = std::make_unique<std::filesystem::path[]>(256);
	history[0] = start_path;
}

std::filesystem::path& dir_history_t::back()
{
	if(back_off < his_size) back_off++;

	return history[(cur_point - back_off) & 0xFF];
}

std::filesystem::path& dir_history_t::fwd()
{
	if(back_off) back_off--;

	return history[(cur_point - back_off) & 0xFF];
}

void dir_history_t::fwd(const std::filesystem::path &path)
{
	his_size -= back_off;
	cur_point -= back_off;
	back_off = 0;

	history[++cur_point] = path;

	if(his_size < 0xFF) his_size++;
}
