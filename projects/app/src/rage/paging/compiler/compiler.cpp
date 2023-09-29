#include "compiler.h"
#include "rage/paging/paging.h"

void rage::pgRscCompiler::FixupReferences(const datPackedChunks& pack, const pgSnapshotAllocator& allocator) const
{
	if (pack.IsEmpty)
		return;

	// We accumulate file offset while iterating through all packed blocks
	u32 fileOffset = 0;
	u32 chunkSize = PG_MIN_CHUNK_SIZE << pack.SizeShift;
	for (u8 i = 0; i < PG_MAX_BUCKETS; i++)
	{
		for (const auto& chunk : pack.Buckets[i])
		{
			if (!chunk.Any())
				continue;

			u32 inChunkOffset = 0;
			for (u16 blockIndex : chunk)
			{
				u64 newAddress = allocator.GetBaseAddress() | static_cast<u64>(fileOffset + inChunkOffset);
				allocator.FixupBlockReferences(blockIndex, newAddress);

				inChunkOffset += allocator.GetBlockSize(blockIndex);
			}

			fileOffset += chunkSize;
		}
		chunkSize /= 2;
	}
}
