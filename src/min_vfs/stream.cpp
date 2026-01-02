#include <ios>
#include <algorithm>
#include <limits>

#include "min_vfs_base.hpp"
#include "Utils/utils.hpp"

namespace min_vfs
{
	stream_t::stream_t()
	{
		fs = 0;
		internal_file = 0;
		pos = 0;
	}

	stream_t::stream_t(filesystem_t *fs, void *internal_file): fs(fs),
		internal_file(internal_file)
	{
		pos = 0;
	}

	stream_t::~stream_t()
	{
		close();
	}

	stream_t &stream_t::operator=(stream_t &&other)
	{
		if(&other == this) return *this;

		close();
		
		fs = other.fs;
		internal_file = other.internal_file;
		pos = other.pos;

		other.fs = nullptr;
		other.internal_file = nullptr;

		return *this;
	}

	uint16_t stream_t::read(void *dst, uintmax_t len)
	{
		if(!fs) return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_STATE);

		return fs->read(internal_file, pos, len, dst);
	}

	uint16_t stream_t::write(void *src, uintmax_t len)
	{
		if(!fs) return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_STATE);

		return fs->write(internal_file, pos, len, src);
	}

	uint16_t stream_t::seek(const intmax_t off, const std::ios_base::seekdir dir)
	{
		switch(dir)
		{
			case std::ios_base::beg:
				pos = std::max((intmax_t)0, off);
				return 0;

			case std::ios_base::cur:
				pos += off < 0 ? -std::min(pos, (uintmax_t)-off) :
					std::min(std::numeric_limits<uintmax_t>::max() - pos, (uintmax_t)off);
				return 0;

			case std::ios_base::end:
			default:
				return ret_val_setup(LIBRARY_ID, (u8)ERR::UNSUPPORTED_OPERATION);
		}
	}

	uint16_t stream_t::seek(const uintmax_t pos)
	{
		this->pos = pos;
		return 0;
	}

	uintmax_t stream_t::get_pos()
	{
		return pos;
	}

	uint16_t stream_t::flush()
	{
		if(!fs) return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_STATE);

		return fs->flush(internal_file);
	}

	uint16_t stream_t::close()
	{
		if(!fs) return ret_val_setup(LIBRARY_ID, (u8)ERR::INVALID_STATE);

		fs->mtx.lock();
		const u16 err = fs->fclose(internal_file);
		fs->mtx.unlock();
		if(err) return err;
		
		fs = nullptr;
		internal_file = nullptr;
		return 0;
	}
}
