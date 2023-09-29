#pragma once

#include "common/types.h"
#include "resourceinfo.h"

namespace rage
{
	/**
	 * \brief Represents mapping between resource (file) offset and game memory address.
	 */
	struct datResourceChunk
	{
		u64 SrcAddr;	// Address of chunk in file
		u64 DestAddr;	// Address of chunk allocated in heap
		u64 Size;		// Even though it's defined as 64 bits, maximum allowed size (100MB) won't exceed 32 bits.

		pVoid GetAllocatedAddress() const { return reinterpret_cast<pVoid>(DestAddr); }

		// Gets offset that maps resource address to corresponding address allocated in game heap.
		u64 GetFixup() const { return DestAddr - SrcAddr; }
		u8 GetSizeShift() const { return datResourceInfo::GetChunkSizeShift(static_cast<u32>(Size)); }
	};

	/**
	 * \brief Same as datResourceChunk but used in sorted list for quick binary search.
	 */
	struct datResourceSortedChunk
	{
		static constexpr u64 SORTED_CHUNK_SIZE_MASK = 0x00FF'FFFF'FFFF'FFFF;
		static constexpr u64 SORTED_CHUNK_INDEX_SHIFT = 56; // 8 highest bits
		static constexpr u64 SORTED_CHUNK_INDEX_SIZE = 0xFF;

		u64 Address = 0; // Start address of this chunk
		u64 IndexAndSize = 0;

		datResourceSortedChunk() = default;
		datResourceSortedChunk(u64 address, u64 size, u8 index)
		{
			Address = address;
			IndexAndSize = static_cast<u64>(index) << SORTED_CHUNK_INDEX_SHIFT | size;
		}

		// Corresponding chunk in datResourceMap.Chunks
		u8 GetChunkIndex() const { return static_cast<u8>(IndexAndSize >> SORTED_CHUNK_INDEX_SHIFT & SORTED_CHUNK_INDEX_SIZE); }
		u64 GetSize() const { return IndexAndSize & SORTED_CHUNK_SIZE_MASK; }
		u64 GetEndAddress() const { return Address + GetSize(); }
		// Gets whether given address is within address range of this chunk.
		u64 ContainsThisAddress(u64 addr) const
		{
			return addr >= Address && addr < GetEndAddress();
		}
	};
}
