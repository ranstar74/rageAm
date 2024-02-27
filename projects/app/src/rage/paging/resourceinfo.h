#pragma once

#include "paging.h"
#include "common/types.h"
#include "helpers/align.h"

namespace rage
{
	struct datResourceMap;
	struct datPackedChunks;

	/**
	 * \brief Encoded virtual and physical chunks.
	 */
	struct datResourceInfo
	{
		// datResourceInfo::Data bit layout
		// 
		//  BYTE 4                 BYTE 3       BYTE 2       BYTE 1
		//  │                      │            │            │
		//  ▼                      ▼            ▼            ▼
		// ┌────┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌────────┐ ┌────────┐ ┌─────┐ ┌──┐ ┌─┐ ┌────┐
		// │0000│ │0│ │0│ │0│ │0│ │0000 000│ │0 0000 0│ │000 0│ │00│ │0│ │0000│
		// └┬───┘ └┬┘ └┬┘ └┬┘ └┬┘ └┬───────┘ └┬───────┘ └┬────┘ └┬─┘ └┬┘ └┬───┘
		//  │      │   │   │   │   │          │          │       │    │   │
		//  Ver    8   7   6   5   4          3          2       1    0   Size shift
		// 
		// This gives us total 24 bits reserved for chunks sizes.
		// 
		// Example of decoded chunk count array, assuming size shift is 15:
		// 
		// Index  Size        Max Chunk Count
		// 0      0x10000000  1
		// 1      0x8000000   3
		// 2      0x4000000   15
		// 3      0x2000000   63
		// 4      0x1000000   127
		// 5      0x800000    1
		// 6      0x400000    1
		// 7      0x200000    1
		// 8      0x100000    1

		// TODO: Replace this with bit field
		static constexpr u32 VERSION_SHIFT = 28;
		static constexpr u32 VERSION_MASK = 0xF << VERSION_SHIFT;

		u32 VirtualData;
		u32 PhysicalData;

		static u8 GetMaxChunkCountInBucket(u32 bucket)
		{
			static constexpr u8 chunkMaxCounts[] = { 1, 3, 15, 63, 127, 1, 1, 1, 1 };
			return chunkMaxCounts[bucket];
		}

		static u8 GetSizeShift(u32 data);
		static u8 GetChunkCount(u32 data);

		u8 GetVirtualChunkCount() const { return GetChunkCount(VirtualData); }
		u8 GetPhysicalChunkCount() const { return GetChunkCount(PhysicalData); }

		void SetVersion(int version)
		{
			VirtualData &= ~VERSION_MASK;
			PhysicalData &= ~VERSION_MASK;
			// Bits 4-8 in virtual
			VirtualData |= (version >> 4 & 0xF) << VERSION_SHIFT;
			// Bits 0-4 in physical
			PhysicalData |= (version & 0xF) << VERSION_SHIFT;
		}
		int GetVersion() const
		{
			return (int)(((VirtualData >> VERSION_SHIFT) & 0xF) << 4) | ((PhysicalData >> VERSION_SHIFT) & 0xF);
		}

		static u32 GetChunkSize(u32 blockSize)
		{
			if (blockSize == 0)
				return 0;

			if (blockSize < PG_MIN_CHUNK_SIZE)
				return PG_MIN_CHUNK_SIZE;

			return ALIGN_POWER_OF_TWO_64(blockSize);
		}

		static u8 GetChunkSizeShift(u32 chunkSize)
		{
			return BitScanR64(chunkSize) - BitScanR64(PG_MIN_CHUNK_SIZE);
		}

		/**
		 * \brief Encodes packed chunks and size shift in 32 bit.
		 */
		static u32 EncodeChunks(const datPackedChunks& pack);

		/**
		 * \brief Decodes chunk into resource map from encoded data.
		 *
		 * \param data			Encoded data (virtual or physical).
		 * \param baseSize		Min chunk size, see datResourceDef.h;
		 * \param map			Destination map where chunks will be written to.
		 * \param startChunk	Index of first chunk.
		 * \param baseAddr		Base file offset, see datResourceDef.h;
		 *
		 * \return Number of decoded chunks.
		 */
		static u8 GenerateChunks(u32 data, u64 baseSize, datResourceMap& map, u8 startChunk, u64 baseAddr);

		/**
		 * \brief Decodes virtual and physical chunks into given resource map.
		 */
		void GenerateMap(datResourceMap& map) const;

		u32 ComputeSize(u32 data) const;
		u32 ComputeVirtualSize() const { return ComputeSize(VirtualData); }
		u32 ComputePhysicalSize() const { return ComputeSize(PhysicalData); }
	};
}
