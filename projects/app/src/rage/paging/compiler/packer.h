//
// File: packer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/array.h"
#include "rage/paging/paging.h"

namespace rage
{
	class pgSnapshotAllocator;

	struct datPackedChunks
	{
		u8 SizeShift;

		atArray<atArray<u16>>* Buckets;			// Pointer to packed chunks in pgRscPacker
		u8 BucketCounts[PG_MAX_BUCKETS]{};		// Number of chunks in every size bucket, for encoding
		u8 ChunkCount;

		bool IsEmpty; // Whether there's any packed block
	};

	/**
	 * \brief Packs memory blocks from snapshot allocator into chunks.
	 */
	class pgRscPacker
	{
		struct SortedBlock
		{
			struct PredicateDescending
			{
				bool operator()(SortedBlock const& left, SortedBlock const& right) const
				{
					return left.Size > right.Size;
				}
			};

			u32 Size;
			u16 Index;
		};

		static constexpr u8 sm_BucketAndSizeShift[] =
		{
			// Index is the most significant bit starting from 0x2000, see ::GetInitialBucketAndSizeShift
			// Bucket is stored in 4 high bits and shift is in the 4 low bits
			// Size shift begins with 4 (DAT_MIN_SIZE_SHIFT), dev who did this was clearly drunk.

			4 << 4 | 4,		// 0x2000
			3 << 4 | 4,		// 0x4000
			2 << 4 | 4,		// 0x8000
			1 << 4 | 4,		// 0x10000
			0 << 4 | 4,		// 0x20000
			0 << 4 | 5,		// 0x40000
			0 << 4 | 6,		// 0x80000
			0 << 4 | 7,		// 0x100000
			0 << 4 | 8,		// 0x200000
			0 << 4 | 9,		// 0x400000
			0 << 4 | 10,	// 0x800000
			0 << 4 | 11,	// 0x1000000
			0 << 4 | 12,	// 0x2000000
			0 << 4 | 13,	// 0x4000000

			// Shifts below exceed maximum allowed chunk size of 100MB

			0 << 4 | 14,	// 0x8000000
			0 << 4 | 15,	// 0x10000000
			0 << 4 | 16,	// 0x20000000
			0 << 4 | 17,	// 0x40000000
			0 << 4 | 18,	// 0x80000000
			0 << 4 | 19,	// 0x100000000 
		};
		static constexpr u32 BUCKET_SIZE = 0xF;
		static constexpr u32 BUCKET_SHIFT = 4;
		static constexpr u32 SIZE_SHIFT_MASK = 0xF;

		// Gets optimal starting bucket / shift to start packing with
		static void GetInitialBucketAndSizeShift(u32 largestBlock, u8& bucket, u8& shift);

		// Since we have max limit of 128 chunks for both virtual and physical segments,
		// we have to take into account number of chunks packed by previous packer (well it's always the virtual one)
		u32	m_ReservedChunks;

		u32						m_LargestBlock;
		atArray<SortedBlock>	m_SortedBlocks;
		atArray<atArray<u16>>	m_Buckets[PG_MAX_BUCKETS] = // 1, 3, 15, 63, 127, 1, 1, 1, 1
		{
			// (one or more) Bucket -> (one or more) Chunks -> (one or more) Blocks
			// This may look complex, it's recommended to understand how chunks are packed in the first place (datResourceInfo.h)

			// Can anyone suggest better way to represent buckets?

			atArray<atArray<u16>>(1),
			atArray<atArray<u16>>(3),
			atArray<atArray<u16>>(15),
			atArray<atArray<u16>>(63),
			atArray<atArray<u16>>(127),
			atArray<atArray<u16>>(1),
			atArray<atArray<u16>>(1),
			atArray<atArray<u16>>(1),
			atArray<atArray<u16>>(1),
		};
		bool m_BucketsDirty = false; // To clear all indices before re-packing

		void ResetBuckets();
		void CopyAndSortBlocks(const pgSnapshotAllocator& allocator);
		void PrintPackedInfo(const datPackedChunks& chunks) const;

		u32	GetTotalBlockSize(u8 bucket, u16 startIndex) const;

		// Gets number of total currently packed chunks.
		u32 GetTotalChunkCount() const;

		// Updates datPackedChunks::BucketCounts (chunk count per bucket) and datPackedChunks::ChunkCount (total chunk count)
		void CalculateChunkCountInBuckets(datPackedChunks& pack) const;

		// Tries to pack blocks with given min chunk size, if not successful, retries with larger min chunk size.
		// Returns packed size shift, or 255 if wasn't able to pack.
		u8 TryPack(u8 startBucket, u8 sizeShift);
	public:
		pgRscPacker(const pgSnapshotAllocator& allocator, u32 reservedChunks);

		bool Pack(datPackedChunks& outPack);
	};
}
