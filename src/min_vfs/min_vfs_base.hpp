#ifndef MIN_VFS_BASE_HEADER_INCLUDE_GUARD
#define MIN_VFS_BASE_HEADER_INCLUDE_GUARD

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <vector>
#include <string>
#include <ctime>
#include <mutex>

#include "library_IDs.hpp"

namespace min_vfs
{
	constexpr uint8_t LIBRARY_ID = (uint8_t)Library_IDs::MIN_VFS;

	enum struct ERR: uint8_t
	{
		NONEXISTANT_DISK = 1,
		DISK_TOO_SMALL,
		CANT_OPEN_DISK,
		IO_ERROR,
		WRONG_FS,
		FS_SIZE_MISMATCH,
		NOT_FOUND,
		INVALID_PATH,
		NOT_A_FILE, //we only accept disk images for now
		NOT_A_DIR,
		FILE_TOO_LARGE,
		NO_SPACE_LEFT,
		UNSUPPORTED_OPERATION,
		ALREADY_OPEN, //also used when trying to remove open files
		CANT_OPEN_FILE,
		FAILED_TO_OPEN_FILE,
		ALREADY_EXISTS,
		END_OF_FILE,
		INVALID_STATE, //for streams, on nullptrs
		NOT_EMPTY,
		CANT_REMOVE, //for mount points
		CANT_MOVE, //for trying to move mount points into non-host fss
		FS_BUSY,
		UNKNOWN_ERROR, //Should only be used by host FS pseudo-driver
		NO_PERM
	};

	//Used by constructors, which take care of mounting.
	//Might wanna rename to VFS_err
	struct FS_err: std::runtime_error
	{
		const uint16_t err_code;

		FS_err(const uint16_t err_code): std::runtime_error("FS error"), err_code(err_code)
		{
			//NOP
		}
	};

	enum struct ftype_t: uint8_t
	{
		file,
		dir,
		symlink,
		blk_dev,
		char_dev,
		pipe,
		socket,
		unknown
	};

	std::string ftype_to_string(const min_vfs::ftype_t &ftype);

	struct dentry_t
	{
		//might wanna have the dir's path, too
		std::string fname;
		uintmax_t fsize;
		std::time_t ctime;
		std::time_t mtime;
		std::time_t atime;
		ftype_t ftype;

		//dentry_t() = default;
		/*dentry_t(const std::string &fname, const uintmax_t fsize,
				 const std::time_t ctime, const std::time_t mtime,
				 const std::time_t atime, const ftype_t ftype);
		*/
		bool operator==(const dentry_t &other) const;

		std::string to_string(const u8 indent) const;
	};

	/*TODO:
		1. Should maybe consider mount flags (like read-only).

		2. I gotta figure out some sort of fs stats system (for fs size, free
		space...).

		3. Consider adding a recursion flag. None of the sampler filesystems
		we're gonna be working with support nested directories as far as I know;
		but it seems like a reasonable way to control whether
		extracting/inserting files should cascade on an S7XX fs, for example.
		Could be useful for other stuff in the future. I think both the force
		and recursion flags are something the file manager/shell should handle,
		not the driver.
	*/

	/*The functions in filesystem_t will be called through functions in
	 min_vfs which will work with a global filesystem map. Filesystems will
	 receive absolute paths relative to their respective roots (as opposed to
	 the global filesystem).*/

	class stream_t;

	//Any path a filesystem_t gets should be an absolute path (relative to its
	//root, not the global root).
	class filesystem_t
	{
	public:
		std::filesystem::path path;
		std::fstream stream;
		std::mutex mtx;

		//constructor will be our mount function
		filesystem_t() = default; //only for host FS
		filesystem_t(const char *path);
		virtual ~filesystem_t() = default;
		virtual filesystem_t& operator=(filesystem_t &&other) noexcept = 0;

		/*Maybe std::string it would be better to use string_view?*/
		virtual std::string get_type_name() = 0;
		virtual uintmax_t get_open_file_count() = 0;
		virtual bool can_unmount() = 0;

		uint16_t list(const char *path, std::vector<dentry_t> &dentries);
		virtual uint16_t list(const char *path, std::vector<dentry_t> &dentries,
							  const bool get_dir) = 0;
		virtual uint16_t mkdir(const char *dir_path) = 0;
		//consider using a flag to choose between filling or leaving uninitialized
		virtual uint16_t ftruncate(const char *path, const uintmax_t new_size) = 0;
		virtual uint16_t rename(const char *cur_path, const char *new_path) = 0;
		virtual uint16_t remove(const char *path) = 0;

		uint16_t fopen(const char *path, stream_t &stream);
		virtual uint16_t fclose(void *internal_file) = 0;
		virtual uint16_t read(void *internal_file, uintmax_t &pos, uintmax_t len, void *dst) = 0;
		virtual uint16_t write(void *internal_file, uintmax_t &pos, uintmax_t len, void *src) = 0;
		virtual uint16_t flush(void *internal_file) = 0;

	private:
		virtual uint16_t fopen_internal(const char *path, void **internal_file) = 0;
	};

	class stream_t
	{
	private:
		filesystem_t *fs;
		void *internal_file;
		uintmax_t pos;

		friend filesystem_t;

	public:
		stream_t();
		stream_t(filesystem_t *fs, void *internal_file);
		~stream_t();
		
		uint16_t read(void *dst, uintmax_t len);
		uint16_t write(void *src, uintmax_t len);

		uint16_t seek(const uintmax_t pos);
		uint16_t seek(const intmax_t off, const std::ios_base::seekdir dir);
		uintmax_t get_pos();
		
		uint16_t flush();
		uint16_t close();

		stream_t& operator=(stream_t &other) = delete;
		stream_t& operator=(stream_t &&other);
	};
}
#endif
