#include "resource.h"
#include "common/logger.h"

#include <algorithm>

rage::datResource::datResource(datResourceMap& map, ConstString name)
{
	Map = &map;
	DebugName = name;
	ChunkCount = map.GetChunkCount();

	for (u8 i = 0; i < ChunkCount; i++)
	{
		const datResourceChunk& chunk = map.Chunks[i];

		SrcChunks[i] = datResourceSortedChunk(chunk.SrcAddr, chunk.Size, i);
		DestChunks[i] = datResourceSortedChunk(chunk.DestAddr, chunk.Size, i);
	}

	// Chunks has to be sorted so we can quickly find chunk from address
	auto compareFn = [](const datResourceSortedChunk& lhs, const datResourceSortedChunk& rhs)
	{
		return lhs.Address < rhs.Address;
	};
	std::sort(SrcChunks, SrcChunks + ChunkCount, compareFn);
	std::sort(DestChunks, DestChunks + ChunkCount, compareFn);

	tl_CurrentResource = this;
}

rage::datResource::~datResource()
{
	tl_CurrentResource = nullptr;
}

rage::datResourceChunk* rage::datResource::GetChunk(const datResourceSortedChunk* sortedChunk) const
{
	return &Map->Chunks[sortedChunk->GetChunkIndex()];
}

const rage::datResourceSortedChunk* rage::datResource::Find(const datResourceSortedChunk* chunks, u64 address) const
{
	const datResourceSortedChunk* chunkIt = chunks;
	u8 index = ChunkCount;

	// Binary search to find page which range contains address
	while (index > 0)
	{
		u8 mid = index / 2;
		const datResourceSortedChunk& chunk = chunkIt[mid];

		if (chunk.GetEndAddress() > address)
		{
			index = mid;
		}
		else
		{
			chunkIt += mid + 1;
			index -= mid + 1;
		}
	}

	const datResourceSortedChunk* end = &chunks[ChunkCount];
	if (chunkIt != end && chunkIt->ContainsThisAddress(address))
		return chunkIt;
	return nullptr;
}

bool rage::datResource::ContainsThisAddress(u64 address) const
{
	return Find(DestChunks, address) != nullptr;
}

u64 rage::datResource::GetFixup(u64 resourceOffset) const
{
	if (const datResourceSortedChunk* chunk = Find(SrcChunks, resourceOffset))
		return GetChunk(chunk)->GetFixup();

	// ERR_SYS_INVALIDRESOURCE_5
	AM_ERRF("rage::datResource::GetFixup() -> "
		"Invalid fixup, address %llu is neither virtual nor physical.", resourceOffset);

	return 0;
}
