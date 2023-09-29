#include "packer.h"

#include "snapshotallocator.h"
#include "rage/math/math.h"
#include "rage/paging/resourceinfo.h"

void rage::pgRscPacker::GetInitialBucketAndSizeShift(u32 largestBlock, u8& bucket, u8& shift)
{
	u32 chunkSize = datResourceInfo::GetChunkSize(largestBlock);
	u32 index = BitScanR32(chunkSize) - PG_MIN_CHUNK_SIZE_SHIFT;

	u8 data = sm_BucketAndSizeShift[index];
	bucket = data >> BUCKET_SHIFT & BUCKET_SIZE;
	shift = data & SIZE_SHIFT_MASK;
}

void rage::pgRscPacker::ResetBuckets()
{
	if (!m_BucketsDirty)
		return;

	for (auto& chunkBucket : m_Buckets)
	{
		for (auto& blockIndices : chunkBucket)
		{
			blockIndices.Clear();
		}
	}
}

void rage::pgRscPacker::CopyAndSortBlocks(const pgSnapshotAllocator& allocator)
{
	u16 blockCount = allocator.GetBlockCount();
	if (blockCount == 0)
	{
		AM_DEBUGF("pgRscPacker(%s) -> No blocks to pack", allocator.GetDebugName());
		return;
	}

	atArray<u32> blocks;
	blocks.Reserve(blockCount);
	allocator.GetBlockSizes(blocks);

	m_SortedBlocks.Resize(blocks.GetSize());
	for (u16 i = 0; i < blocks.GetSize(); i++)
		m_SortedBlocks[i] = SortedBlock(blocks[i], i);

	if (allocator.IsVirtual())
	{
		// With virtual chunks we have one more rule - main chunk always has to be the first one
		std::sort(
			m_SortedBlocks.begin() + 1 /* Pin first block */,
			m_SortedBlocks.end(),
			SortedBlock::PredicateDescending());

		// If we've got only one block, its always the largest one. Otherwise we have to pick largest
		// between first and second one (because they're sorted in descending order)
		if (blockCount == 1)
			m_LargestBlock = m_SortedBlocks[0].Size;
		else
			m_LargestBlock = Math::Max(m_SortedBlocks[0].Size, m_SortedBlocks[1].Size);
	}
	else
	{
		std::ranges::sort(m_SortedBlocks, SortedBlock::PredicateDescending());

		m_LargestBlock = m_SortedBlocks[0].Size;
	}

	AM_DEBUGF("pgRscPacker(%s) -> %u blocks to pack, largest block: %#x", allocator.GetDebugName(), m_SortedBlocks.GetSize(), m_LargestBlock);
}

void rage::pgRscPacker::PrintPackedInfo(const datPackedChunks& chunks) const
{
#ifdef _DEBUG
	AM_DEBUG("pgRscPacker::PrintPackedInfo() -> Buckets:");
	AM_DEBUG("(Index, Block Count, Packed Chunk Count, Total Chunk Count, Used / Total, Chunk Size)");

	u32 chunkSize = PG_MIN_CHUNK_SIZE << chunks.SizeShift;
	for (u8 i = 0; i < PG_MAX_BUCKETS; i++)
	{
		u16 usedChunks = 0; // Number of chunks with at least one packed block
		u16 bucketBlockCount = 0; // Total number of blocks in this bucket
		u32 bucketBlocksSize = 0; // Total size of blocks in this bucket
		for (auto& bucketChunks : m_Buckets[i])
		{
			u16 blockCount = bucketChunks.GetSize();
			bucketBlockCount += blockCount;
			for (u16 index : bucketChunks)
				bucketBlocksSize += m_SortedBlocks[index].Size;

			// For average we need to account only chunks where at least one block present
			if (blockCount != 0)
				usedChunks++;
		}

		// This shows us how much memory in chunk is actually used, and how much is wasted (internal fragmentation)
		// This value is average per all chunks in bucket
		double totalChunks = (double)chunkSize * (double)usedChunks;
		double useRatio = 0;
		if (totalChunks > 0) useRatio = (double)bucketBlocksSize / totalChunks * 100;

		u8 totalChunkCount = datResourceInfo::GetMaxChunkCountInBucket(i);
		AM_DEBUGF("%i, %u, %u, %u,  \t%.0f%%,  \t%#x",
			i,						// Bucket Index
			bucketBlockCount,		// Block Count 
			chunks.BucketCounts[i], // Packed Chunk Count
			totalChunkCount,		// Total Chunk Count
			useRatio,				// Used / Total
			chunkSize);				// Chunk Size

		chunkSize /= 2;
	}
#endif
}

u32 rage::pgRscPacker::GetTotalBlockSize(u8 bucket, u16 startIndex) const
{
	u32 size = 0;
	for (u16 i = startIndex; i < m_SortedBlocks.GetSize(); i++)
		size += m_SortedBlocks[i].Size;
	return size;
}

u32 rage::pgRscPacker::GetTotalChunkCount() const
{
	u8 chunkCount = 0;
	for (const auto& m_Bucket : m_Buckets)
	{
		u8 bucketChunkCount = 0;
		for (auto& chunk : m_Bucket)
		{
			// We need at least one block in chunk to consider it as used
			if (chunk.Any()) bucketChunkCount++;
			else break; // If we don't have any block in this chunk, there won't be in next ones either
		}
		chunkCount += bucketChunkCount;
	}
	return chunkCount + m_ReservedChunks;
}

void rage::pgRscPacker::CalculateChunkCountInBuckets(datPackedChunks& pack) const
{
	pack.ChunkCount = 0;
	for (u8 i = 0; i < PG_MAX_BUCKETS; i++)
	{
		u8 bucketChunkCount = 0;
		for (auto& chunk : m_Buckets[i])
		{
			// We need at least one block in chunk to consider it as used
			if (chunk.Any()) bucketChunkCount++;
			else break; // If we don't have any block in this chunk, there won't be in next ones either
		}

		pack.BucketCounts[i] = bucketChunkCount;
		pack.ChunkCount += bucketChunkCount;
	}
}

u8 rage::pgRscPacker::TryPack(u8 startBucket, u8 sizeShift)
{
	ResetBuckets();

	u8 bucket = startBucket;

	u32 chunkSize = PG_MIN_CHUNK_SIZE << sizeShift >> bucket; // Initial chunk size
	u32 packedSize = 0; // Sum of all block sizes packed in current bucket
	u16 chunkIndex = 0; // Amount of packed chunks in current bucket

	for (u16 i = 0; i < m_SortedBlocks.GetSize(); i++)
	{
		const SortedBlock& block = m_SortedBlocks[i];

		// Not enough free space in chunk, we have to move to either next chunk or next bucket
		if (packedSize + block.Size > chunkSize)
		{
			packedSize = 0;
			chunkIndex++;

			// No more free chunks in this bucket, move to next (smaller) one
			if (chunkIndex == datResourceInfo::GetMaxChunkCountInBucket(bucket))
			{
				bucket++;
				chunkIndex = 0;
				chunkSize /= 2;
			}
		}

		// Fitting in buckets with specific size makes this task much more complicated to do optimally
		// Here's the deal: We get initial bucket / size shift from largest memory block,
		// assuming that other blocks will be at least twice smaller to fit in remaining buckets
		// This is not true in some cases (for example when we need to pack two large memory blocks of the same size)
		// Our solution is to simply increment size shift, which will double chunk size and try to pack again
		if (block.Size > chunkSize)
		{
			AM_DEBUG("pgRscPacker::TryPack() -> Chunk is too small... retrying with next size shift");
			return TryPack(startBucket, sizeShift + 1);
		}

		// We have to make sure that we don't put small blocks in large chunks,
		// causing unreasonably large internal fragmentation (unused space inside chunk).
		// To prevent that, we check if we can fit remaining blocks in next (smaller) bucket
		u8 oldBucket = bucket;
		while (true)
		{
			if (bucket + 1 == PG_MAX_BUCKETS)
				break;

			u32 remainingSize = GetTotalBlockSize(bucket, i);
			u32 nextChunkSize = chunkSize / 2;

			// Though chunks with size smaller that minimum will be parsed and allocated properly, it won't make any difference
			if (nextChunkSize < PG_MIN_CHUNK_SIZE)
				break;

			// We simply can't fit remaining blocks in next chunk.
			// While some of blocks may fit separately, it's too expensive to calculate.
			if (remainingSize > nextChunkSize)
				break;

			// Is it even rational to move to next bucket? If we can fit everything into current chunk,
			// we better do that, because every new chunk is one more allocation
			// To make it reasonable, amount of fragmentation in current bucket chunk must be higher than in next (smaller) one.
			u32 currentBucketLeftSpace = chunkSize - (packedSize + remainingSize);
			u32 nextBucketLeftSpace = nextChunkSize - remainingSize;
			if (currentBucketLeftSpace <= nextBucketLeftSpace)
				break;

			bucket++;
			chunkIndex = 0;
			chunkSize /= 2;
		}

		// See 'move to next bucket' optimization in while loop above (the last one)
		if (bucket != oldBucket)
			AM_DEBUGF("pgRscPacker::TryPack() -> Moving by %u buckets to reduce fragmentation...", bucket - oldBucket);

		packedSize += block.Size;
		m_Buckets[bucket][chunkIndex].Add(block.Index);
		m_BucketsDirty = true;

		// TODO: We need to account max chunk count from other packer
		// We have total maximum of 128 chunks (both virtual & physical), so same as with block size not fitting in chunk,
		// we have to increase size shift and try again
		u32 chunkCount = GetTotalChunkCount();
		if (chunkCount > 128)
		{
			AM_DEBUGF("pgRscPacker::TryPack() -> Out of chunks... (%u) retrying with next size shift", chunkCount);
			return TryPack(startBucket, sizeShift + 1);
		}

		// Well, we can't really do anything here, maximum allowed streamed chunk size is 100MB
		if (chunkSize >= PG_MAX_CHUNK_SIZE)
			return 255;
	}
	return sizeShift;
}

rage::pgRscPacker::pgRscPacker(const pgSnapshotAllocator& allocator, u32 reservedChunks) : m_SortedBlocks(0)
{
	m_ReservedChunks = reservedChunks;

	CopyAndSortBlocks(allocator);
}

bool rage::pgRscPacker::Pack(datPackedChunks& outPack)
{
	outPack.Buckets = m_Buckets;
	outPack.SizeShift = PG_MIN_SIZE_SHIFT;
	outPack.IsEmpty = true;

	// Physical allocator often has nothing to pack whatsoever, handle that properly
	if (!m_SortedBlocks.Any())
		return true;

	outPack.IsEmpty = false;

	// Packing strategy explained in details in ::TryPack
	u8	startBucket;
	u8	startShift;
	u32 largestChunk = datResourceInfo::GetChunkSize(m_LargestBlock);

	// Maximum allowed by pgStreamer size is 100MB
	// Actual maximum size, though, is defined by size of buddy allocator
	if (!AM_VERIFY(largestChunk < PG_MAX_CHUNK_SIZE, "pgRscPacker::Pack() -> Block size %u is too large and exceeds 100MB limit", m_LargestBlock))
		return false;

	GetInitialBucketAndSizeShift(m_LargestBlock, startBucket, startShift);

	AM_DEBUGF("pgRscPacker::Pack() -> Starting at bucket %u with size shift %u (%#x)",
		startBucket, startShift, PG_MIN_CHUNK_SIZE << startShift);

	// Do packing and calculate number of chunks in every bucket
	u8 resultSizeShift = TryPack(startBucket, startShift);

	if (!AM_VERIFY(resultSizeShift != 255, "pgRscPacker::Pack() -> Resource cannot be packed because largest chunk size exceeds 100MB limit."))
		return false;

	outPack.SizeShift = resultSizeShift;

	CalculateChunkCountInBuckets(outPack);

	AM_DEBUGF("pgRscPacker::Pack() -> Packed with size shift: %u (%x)",
		outPack.SizeShift, PG_MIN_CHUNK_SIZE << outPack.SizeShift);

	PrintPackedInfo(outPack);
	return true;
}
