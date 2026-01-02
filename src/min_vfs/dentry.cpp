#include "min_vfs_base.hpp"

namespace min_vfs
{
	/*dentry_t::dentry_t(const std::string &fname, const uintmax_t fsize,
			 const std::time_t ctime, const std::time_t mtime,
		  const std::time_t atime, const ftype_t ftype):
		  fname(fname), fsize(fsize), ctime(ctime), mtime(mtime), atime(atime),
		  ftype(ftype)
	{
		//NOP
	}*/

	bool dentry_t::operator==(const dentry_t &other) const
	{
		return fname == other.fname && fsize == other.fsize
			&& ctime == other.ctime && mtime == other.mtime
			&& atime == other.atime && ftype == other.ftype;
	}

	std::string dentry_t::to_string(const u8 indent) const
	{
		//indend :DDDDDD
		const std::string indend_str(indent, '\t');

		std::string str;

		str = indend_str + "\"" + fname + "\"\n";
		str += indend_str + "\tftype: " + ftype_to_string(ftype) + "\n";
		str += indend_str + "\tctime: " + std::to_string(ctime) + "\n";
		str += indend_str + "\tmtime: " + std::to_string(mtime) + "\n";
		str += indend_str + "\tatime: " + std::to_string(atime) + "\n";
		str += indend_str + "\tfsize: " + std::to_string(fsize) + "\n";

		return str;
	}
}
