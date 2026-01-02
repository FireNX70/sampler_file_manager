#ifndef FAT_UTILS_HEADER_GUARD
#define FAT_UTILS_HEADER_GUARD

#include <bit>
#include <concepts>
#include <cstdint>
#include <vector>
#include <fstream>

#include "utils.hpp"
#include "library_IDs.hpp"

namespace FAT_utils
{
	constexpr uint8_t LIBRARY_ID = (u8)Library_IDs::FAT_utils;
	
	enum struct ERR: uint8_t
	{
		BAD_START = 1,
		CHAIN_OOB,
		NO_FREE_CLUSTERS,
		EMPTY_CHAIN,
		CHAIN_TOO_LARGE,
		IO_ERROR,
		END_OF_CHAIN,
		ALLOC, //not actually an error
		BAD_NEXT_CLS
	};
	
	//maybe add more stuff to FAT_attrs_t
	template <typename index_type>
	requires std::integral<index_type>
	struct FAT_attrs_t
	{
		const std::endian ENDIANNESS;
		const index_type FREE_CLUSTER;
		const index_type DATA_MIN;
		const index_type DATA_MAX;
		const index_type END_OF_CHAIN;
		const index_type RESERVED;
		
		constexpr FAT_attrs_t(const std::endian endianness,
			const index_type free_cluster, const index_type data_min,
			const index_type data_max, const index_type end_of_chain,
			const index_type reserved):
			ENDIANNESS(endianness), FREE_CLUSTER(free_cluster),
			DATA_MIN(data_min), DATA_MAX(data_max), END_OF_CHAIN(end_of_chain),
			RESERVED(reserved)
		{
			//NOP
		}
	};

	template <typename index_type>
	requires std::integral<index_type>
	struct FAT_dyna_attrs_t
	{
		index_type LENGTH;
		uintmax_t BASE_ADDR;

		constexpr FAT_dyna_attrs_t(const index_type length,
			const uintmax_t base_addr):
			LENGTH(length), BASE_ADDR(base_addr)
		{
			//NOP
		}

		constexpr FAT_dyna_attrs_t(): FAT_dyna_attrs_t(0, 0)
		{
			//NOP
		}
	};

	//Might wanna create something like FAT_dyna_attrs_t to hold address and
	//size (and maybe more?). This would be particularly useful to explicitly
	//pass the FAT's address for the disk versions instead of implicitly getting
	//it from the fstream's get pointer.
	
	template <typename index_type>
	requires std::integral<index_type>
	index_type count_free_clusters(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len)
	{
		index_type count;
		
		count = 0;
		
		for(index_type i = FAT_attrs.DATA_MIN; i < FAT_len; i++)
			if(FAT[i] == FAT_attrs.FREE_CLUSTER) count++;
	
		return count;
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type count_free_clusters(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs)
	{
		index_type count, cls;

		count = 0;

		fstream.seekg(FAT_dyna_attrs.BASE_ADDR + FAT_attrs.DATA_MIN * sizeof(index_type));
		fstream.read((char*)&cls, sizeof(index_type));

		if(FAT_attrs.ENDIANNESS != std::endian::native)
			cls = std::byteswap(cls);

		if(cls == FAT_attrs.FREE_CLUSTER) count++;

		for(index_type i = FAT_attrs.DATA_MIN + 1; i < FAT_dyna_attrs.LENGTH; i++)
		{
			fstream.read((char*)&cls, sizeof(index_type));

			if(FAT_attrs.ENDIANNESS != std::endian::native)
				cls = std::byteswap(cls);

			if(cls == FAT_attrs.FREE_CLUSTER) count++;
		}

		return count;
	}
	
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t get_nth_cluster(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		index_type &start, index_type idx)
	{
		if(start < FAT_attrs.DATA_MIN || start > FAT_attrs.DATA_MAX
			|| start >= FAT_len)
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::BAD_START);

		while(FAT[start] >= FAT_attrs.DATA_MIN
			&& FAT[start] <= FAT_attrs.DATA_MAX && idx)
		{
			if(FAT[start] >= FAT_len)
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_OOB);

			start = FAT[start];
			idx--;
		}
		
		if(idx)
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::END_OF_CHAIN);

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t get_nth_cluster(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs, index_type &start,
		index_type idx)
	{
		index_type cls;

		if(start < FAT_attrs.DATA_MIN || start > FAT_attrs.DATA_MAX
			|| start >= FAT_dyna_attrs.LENGTH)
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::BAD_START);
		
		while(idx)
		{
			fstream.seekg(FAT_dyna_attrs.BASE_ADDR + (std::streampos)(start * sizeof(index_type)));
			fstream.read((char*)&cls, sizeof(index_type));

			if(FAT_attrs.ENDIANNESS != std::endian::native)
				cls = std::byteswap(cls);

			if(cls < FAT_attrs.DATA_MIN || cls > FAT_attrs.DATA_MAX) break;

			if(cls >= FAT_dyna_attrs.LENGTH)
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_OOB);

			start = cls;
			idx--;
		}

		if(idx)
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::END_OF_CHAIN);

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t follow_chain(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		const index_type start, std::vector<index_type> &chain)
	{
		if(start < FAT_attrs.DATA_MIN || start > FAT_attrs.DATA_MAX
			|| start >= FAT_len)
			return LIBRARY_ID << 8 | (uint8_t)ERR::BAD_START;
		
		chain.push_back(start);
		
		//will not include end-of-chain marker
		while(FAT[chain.back()] >= FAT_attrs.DATA_MIN
			&& FAT[chain.back()] <= FAT_attrs.DATA_MAX)
		{
			//chain points OOB, likely corrupt
			if(FAT[chain.back()] >= FAT_len)
				return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;
			
			chain.push_back(FAT[chain.back()]);
		}
		
		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t follow_chain(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const index_type start, std::vector<index_type> &chain)
	{
		index_type cluster_addr;

		if(start < FAT_attrs.DATA_MIN || start > FAT_attrs.DATA_MAX
			|| start >= FAT_dyna_attrs.LENGTH)
			return LIBRARY_ID << 8 | (uint8_t)ERR::BAD_START;

		cluster_addr = start;

		//will not include end-of-chain marker
		do
		{
			//chain points OOB, likely corrupt
			if(cluster_addr >= FAT_dyna_attrs.LENGTH)
				return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;

			chain.push_back(cluster_addr);

			fstream.seekg(FAT_dyna_attrs.BASE_ADDR + cluster_addr * sizeof(index_type));
			fstream.read((char*)&cluster_addr, sizeof(index_type));

			if(FAT_attrs.ENDIANNESS != std::endian::native)
				cluster_addr = std::byteswap(cluster_addr);

			if(fstream.fail())
				return (LIBRARY_ID << 8) | (uint8_t)ERR::IO_ERROR;
		} while(cluster_addr >= FAT_attrs.DATA_MIN
			&& cluster_addr <= FAT_attrs.DATA_MAX);

		return 0;
	}
	
	template <typename index_type>
	requires std::integral<index_type>
	index_type find_next_free_cluster(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		const index_type offset)
	{
		if(offset < FAT_attrs.DATA_MIN || offset > FAT_attrs.DATA_MAX)
			return FAT_attrs.END_OF_CHAIN;
			
		for(index_type i = offset; i < FAT_len; i++)
			if(FAT[i] == FAT_attrs.FREE_CLUSTER) return i;
	
		return FAT_attrs.END_OF_CHAIN;
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type find_next_free_cluster(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const index_type offset)
	{
		index_type temp;

		if(offset < FAT_attrs.DATA_MIN || offset > FAT_attrs.DATA_MAX)
			return FAT_attrs.END_OF_CHAIN;

		fstream.seekg(FAT_dyna_attrs.BASE_ADDR + (std::streamoff)(offset * sizeof(index_type)));

		for(index_type i = offset; i < FAT_dyna_attrs.LENGTH; i++)
		{
			fstream.read((char*)&temp, sizeof(temp));
			if(FAT_attrs.ENDIANNESS != std::endian::native)
				temp = std::byteswap(temp);
			if(temp == FAT_attrs.FREE_CLUSTER) return i;
		}

		return FAT_attrs.END_OF_CHAIN;
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type find_next_free_cluster(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len)
	{
		return find_next_free_cluster(FAT, FAT_attrs, FAT_len,
									  FAT_attrs.DATA_MIN);
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type find_next_free_cluster(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs)
	{
		return find_next_free_cluster(fstream, FAT_attrs,
			FAT_dyna_attrs, FAT_attrs.DATA_MIN);
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type get_next_or_free_cluster(const index_type FAT[], const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len, index_type cur_cls, index_type &dst, const index_type offset)
	{
		if(cur_cls < FAT_attrs.DATA_MIN || cur_cls > FAT_attrs.DATA_MAX || cur_cls >= FAT_len)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_START);

		if(FAT[cur_cls] >= FAT_attrs.DATA_MIN && FAT[cur_cls] <= FAT_attrs.DATA_MAX)
		{
			if(FAT[cur_cls] >= FAT_len)
				return ret_val_setup(LIBRARY_ID, (u8)ERR::CHAIN_OOB);

			dst = FAT[cur_cls];
		}
		else
		{
			dst = find_next_free_cluster(FAT, FAT_attrs, FAT_len, offset);

			return ret_val_setup(LIBRARY_ID, (u8)ERR::ALLOC);
		}

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type get_next_or_free_cluster(const index_type FAT[], const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len, index_type cur_cls, index_type &dst)
	{
		return get_next_or_free_cluster(FAT, FAT_attrs, FAT_len, cur_cls, dst,
			FAT_attrs.DATA_MIN);
	}
	
	template <typename index_type>
	requires std::integral<index_type>
	index_type get_next_or_free_cluster(std::fstream &stream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs, index_type cur_cls,
		index_type &dst, const index_type offset)
	{
		index_type cls;

		if(cur_cls < FAT_attrs.DATA_MIN || cur_cls > FAT_attrs.DATA_MAX || cur_cls >= FAT_dyna_attrs.LENGTH)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_START);

		stream.seekg(FAT_dyna_attrs.BASE_ADDR + cur_cls * sizeof(index_type));
		stream.read((char*)&cls, sizeof(index_type));

		if(FAT_attrs.ENDIANNESS != std::endian::native)
			cls = std::byteswap(cls);

		if(cls >= FAT_attrs.DATA_MIN && cls <= FAT_attrs.DATA_MAX)
		{
			if(cls >= FAT_dyna_attrs.LENGTH)
				return ret_val_setup(LIBRARY_ID, (u8)ERR::CHAIN_OOB);

			dst = cls;
		}
		else
		{
			dst = find_next_free_cluster(stream, FAT_attrs, FAT_dyna_attrs, offset);

			return ret_val_setup(LIBRARY_ID, (u8)ERR::ALLOC);
		}

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	index_type get_next_or_free_cluster(std::fstream &stream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs, index_type cur_cls,
		index_type &dst)
	{
		return get_next_or_free_cluster(stream, FAT_attrs, FAT_dyna_attrs, cur_cls, dst,
			FAT_attrs.DATA_MIN);
	}

	//maybe add FAT_len to FAT_attrs
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t find_free_chain(const index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		index_type cluster_cnt, std::vector<index_type> &chain)
	{
		index_type last_cluster;
		
		if(cluster_cnt < chain.size()) return 0; //nothing to do, not an error

		cluster_cnt -= chain.size();
		last_cluster = FAT_attrs.DATA_MIN;
		
		for(uintmax_t i = 0; i < cluster_cnt; i++)
		{
			last_cluster = find_next_free_cluster(FAT, FAT_attrs, FAT_len,
											  last_cluster);
			
			if(last_cluster == FAT_attrs.END_OF_CHAIN)
				return LIBRARY_ID << 8 | (uint8_t)ERR::NO_FREE_CLUSTERS;
			
			chain.push_back(last_cluster++);
		}
		
		return 0;
	}

	//maybe add FAT_len to FAT_attrs
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t find_free_chain(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		index_type cluster_cnt, std::vector<index_type> &chain)
	{
		index_type last_cluster;

		if(cluster_cnt < chain.size()) return 0; //nothing to do, not an error

		cluster_cnt -= chain.size();
		last_cluster = FAT_attrs.DATA_MIN;

		for(uintmax_t i = 0; i < cluster_cnt; i++)
		{
			last_cluster = find_next_free_cluster(fstream, FAT_attrs,
				FAT_dyna_attrs, last_cluster);

			if(fstream.fail())
				return LIBRARY_ID << 8 | (uint8_t)ERR::IO_ERROR;

			if(last_cluster == FAT_attrs.END_OF_CHAIN)
				return LIBRARY_ID << 8 | (uint8_t)ERR::NO_FREE_CLUSTERS;

			chain.push_back(last_cluster++);
		}

		return 0;
	}
	
	//maybe add FAT_addr to FAT_attrs
	//Note: FAT-based FSes don't usually support linking
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t free_chain(index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		const std::vector<index_type> &chain)
	{
		index_type i;
		
		if(!chain.size()) return LIBRARY_ID << 8 | (uint8_t)ERR::EMPTY_CHAIN;
		if(chain.size() > (FAT_len - FAT_attrs.DATA_MIN))
			return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_TOO_LARGE;
		
		/*Delete from end to minimize the chance that a dangling chain could be
		 *created (by losing the start of the chain). This would only really
		 *matter if we were working directly with the disk; which ideally we
		 *would, but memory-mapped IO isn't the most portable thing ever
		 *(although I do think something like it could be implemented by
		 *"trapping" accesses to an std::vector)*/
		
		for(i = 0; i < chain.size(); i++)
		{
			if(chain[chain.size() - (i + 1)] < FAT_attrs.DATA_MIN ||
				chain[chain.size() - (i + 1)] > FAT_attrs.DATA_MAX ||
				chain[chain.size() - (i + 1)] >= FAT_len)
				return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;

			FAT[chain[chain.size() - (i + 1)]] = FAT_attrs.FREE_CLUSTER;
		}
		
		return 0;
	}

	//maybe add FAT_addr to FAT_attrs
	//Note: FAT-based FSes don't usually support linking
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t free_chain(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const std::vector<index_type> &chain)
	{
		index_type i;

		if(!chain.size()) return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::EMPTY_CHAIN);
		if(chain.size() > (FAT_dyna_attrs.LENGTH - FAT_attrs.DATA_MIN))
			return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_TOO_LARGE);

		const index_type free_cluster_val =
			FAT_attrs.ENDIANNESS == std::endian::native ? FAT_attrs.FREE_CLUSTER : std::byteswap(FAT_attrs.FREE_CLUSTER);

		/*Delete from end to minimize the chance that a dangling chain could be
		 *created (by losing the start of the chain). This would only really
		 *matter if we were working directly with the disk; which ideally we
		 *would, but memory-mapped IO isn't the most portable thing ever
		 *(although I do think something like it could be implemented by
		 *"trapping" accesses to an std::vector)*/

		for(i = 0; i < chain.size(); i++)
		{
			if(chain[chain.size() - (i + 1)] < FAT_attrs.DATA_MIN ||
				chain[chain.size() - (i + 1)] > FAT_attrs.DATA_MAX ||
				chain[chain.size() - (i + 1)] >= FAT_dyna_attrs.LENGTH)
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_OOB);

			fstream.seekp(FAT_dyna_attrs.BASE_ADDR + (std::streamoff)chain[chain.size() - (i + 1)] * sizeof(index_type));
			fstream.write((char*)&free_cluster_val, sizeof(index_type));
		}

		;
		return fstream.fail() ? ret_val_setup(LIBRARY_ID, (uint8_t)ERR::IO_ERROR) : 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t shrink_chain(index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		const std::vector<index_type> &chain, const index_type tgt_size)
	{
		uint16_t err;

		if(!chain.size() || tgt_size >= chain.size()) return 0;

		const std::vector<index_type> to_free(chain.begin() + tgt_size, chain.end());

		err = free_chain(FAT, FAT_attrs, FAT_len, to_free);
		if(err) return err;

		if(tgt_size)
		{
			if(chain[tgt_size - 1] < FAT_attrs.DATA_MIN || chain[tgt_size - 1] > FAT_attrs.DATA_MAX ||
				chain[tgt_size - 1] >= FAT_len)
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_OOB);

			FAT[chain[tgt_size - 1]] = FAT_attrs.END_OF_CHAIN;
		}

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t shrink_chain(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const std::vector<index_type> &chain, const index_type tgt_size)
	{
		uint16_t err;

		if(!chain.size() || tgt_size >= chain.size()) return 0;

		const index_type end_of_chain_val =
			FAT_attrs.ENDIANNESS == std::endian::native ? FAT_attrs.END_OF_CHAIN : std::byteswap(FAT_attrs.END_OF_CHAIN);
		const std::vector<index_type> to_free(chain.begin() + tgt_size, chain.end());

		err = free_chain(fstream, FAT_attrs, FAT_dyna_attrs, to_free);
		if(err) return err;

		if(tgt_size)
		{
			if(chain[tgt_size - 1] < FAT_attrs.DATA_MIN || chain[tgt_size - 1] > FAT_attrs.DATA_MAX ||
				chain[tgt_size - 1] >= FAT_dyna_attrs.LENGTH)
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::CHAIN_OOB);

			fstream.seekp(FAT_dyna_attrs.BASE_ADDR + chain[tgt_size - 1] * 2);
			fstream.write((char*)&end_of_chain_val, 2);

			if(!fstream.good())
				return ret_val_setup(LIBRARY_ID, (uint8_t)ERR::IO_ERROR);
		}

		return 0;
	}

	//maybe add FAT_addr to FAT_attrs
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t write_chain(index_type FAT[],
		const FAT_attrs_t<index_type> FAT_attrs, const index_type FAT_len,
		const std::vector<index_type> &chain)
	{
		index_type i;
		
		if(!chain.size()) return LIBRARY_ID << 8 | (uint8_t)ERR::EMPTY_CHAIN;
		if(chain.size() > (FAT_len - FAT_attrs.DATA_MIN))
			return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_TOO_LARGE;
		
		if(chain[0] < FAT_attrs.DATA_MIN || chain[0] > FAT_attrs.DATA_MAX ||
			chain[0] >= FAT_len)
			return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;

		for(i = 0; i < chain.size() - 1; i++)
		{
			if(chain[i + 1] < FAT_attrs.DATA_MIN || chain[i + 1] > FAT_attrs.DATA_MAX ||
				chain[i + 1] >= FAT_len)
				return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;
			
			FAT[chain[i]] = chain[i + 1];
		}
		
		FAT[chain[i]] = FAT_attrs.END_OF_CHAIN;
		
		return 0;
	}

	//maybe add FAT_addr to FAT_attrs
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t write_chain(std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const std::vector<index_type> &chain)
	{
		index_type i, next_cluster;

		if(!chain.size()) return LIBRARY_ID << 8 | (uint8_t)ERR::EMPTY_CHAIN;
		if(chain.size() > (FAT_dyna_attrs.LENGTH - FAT_attrs.DATA_MIN))
			return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_TOO_LARGE;

		if(chain[0] < FAT_attrs.DATA_MIN || chain[0] > FAT_attrs.DATA_MAX ||
			chain[0] >= FAT_dyna_attrs.LENGTH)
			return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;

		for(i = 0; i < chain.size() - 1; i++)
		{
			//should be within DATA_MIN and DATA_MAX, our chains don't contain
			//the end-of-chain marker (we add it by hand)
			if(chain[i + 1] < FAT_attrs.DATA_MIN ||
				chain[i + 1] > FAT_attrs.DATA_MAX || chain[i + 1] >= FAT_dyna_attrs.LENGTH)
				return LIBRARY_ID << 8 | (uint8_t)ERR::CHAIN_OOB;

			if(FAT_attrs.ENDIANNESS != std::endian::native)
				next_cluster = std::byteswap(chain[i + 1]);
			else next_cluster = chain[i + 1];

			fstream.seekp(FAT_dyna_attrs.BASE_ADDR + (std::streamoff)chain[i] * sizeof(index_type));
			fstream.write((char*)&next_cluster, sizeof(index_type));
		}

		if(FAT_attrs.ENDIANNESS != std::endian::native)
			next_cluster = std::byteswap(FAT_attrs.END_OF_CHAIN);
		else next_cluster = FAT_attrs.END_OF_CHAIN;

		fstream.seekp(FAT_dyna_attrs.BASE_ADDR + (std::streamoff)chain[i] * sizeof(index_type));
		fstream.write((char*)&next_cluster, sizeof(index_type));

		return fstream.fail() ? LIBRARY_ID << 8 | (uint8_t)ERR::IO_ERROR : 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t write_cluster(index_type FAT[], std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const index_type cur_cls, const index_type next_cls)
	{
		u16 n_endian_cls;

		n_endian_cls = next_cls;

		if(FAT_attrs.ENDIANNESS != std::endian::native)
			n_endian_cls = std::byteswap(n_endian_cls);

		fstream.seekp(FAT_dyna_attrs.BASE_ADDR + cur_cls * sizeof(index_type));
		fstream.write((char*)&n_endian_cls, 2);

		if(!fstream.good())
			return ret_val_setup(LIBRARY_ID, (u8)ERR::IO_ERROR);

		FAT[cur_cls] = next_cls;

		return 0;
	}

	template <typename index_type>
	requires std::integral<index_type>
	uint16_t extend_chain(index_type FAT[], std::fstream &fstream,
		const FAT_attrs_t<index_type> FAT_attrs,
		const FAT_dyna_attrs_t<index_type> &FAT_dyna_attrs,
		const index_type cur_cls, const index_type next_cls)
	{
		u16 err;

		if(cur_cls < FAT_attrs.DATA_MIN || cur_cls > FAT_attrs.DATA_MAX || cur_cls >= FAT_dyna_attrs.LENGTH)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_START);

		if(next_cls < FAT_attrs.DATA_MIN || next_cls > FAT_attrs.DATA_MAX || next_cls >= FAT_dyna_attrs.LENGTH)
			return ret_val_setup(LIBRARY_ID, (u8)ERR::BAD_NEXT_CLS);

		err = 0;
		err |= write_cluster(FAT, fstream, FAT_attrs, FAT_dyna_attrs, cur_cls, next_cls);
		err |= write_cluster(FAT, fstream, FAT_attrs, FAT_dyna_attrs, next_cls, FAT_attrs.END_OF_CHAIN);

		return err;
	}

	//maybe add cluster_size and start_of_data to some sort of FAT_dyna_attrs_t
	template <typename index_type>
	requires std::integral<index_type>
	uint16_t write_data(std::vector<index_type> &chain,
		const uintmax_t cluster_size, const uintmax_t start_of_data,
		std::fstream &src, std::fstream &dst)
	{
		index_type last_index;
		char *buffer;

		if(!chain.size()) return LIBRARY_ID << 8 | (uint8_t)ERR::EMPTY_CHAIN;

		buffer = new char[cluster_size];

		for(index_type index: chain)
		{
			dst.seekp(start_of_data + index * cluster_size);

			src.read(buffer, cluster_size);
			dst.write(buffer, cluster_size);

			if(!src.good()) break;
		}

		delete[] buffer;

		return 0;
	}
}
#endif