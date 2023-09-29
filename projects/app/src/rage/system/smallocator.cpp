#include "smallocator.h"

#include <new.h>

#include "common/logger.h"
#include "helpers/align.h"
#include "simpleallocator.h"

rage::sysSmallocator::Chunk::Chunk(Bucket* bucket)
{
	MainBlock = GetBlockAsFreeBlock();
	FreeBlockCount = bucket->BlockCount;
	BucketParent = bucket;
	MemoryBucket = GetCurrentMemoryBucket();

	// Build one way linked list.

	FreeNode* block = GetBlockAsFreeBlock();
	for (u32 i = 1; i < FreeBlockCount; i++) 
	{
		FreeNode* nextBlock = reinterpret_cast<FreeNode*>((char*)block + bucket->BlockSize);

		block->NextLinked = nextBlock;
		block = nextBlock;
	}
	block->NextLinked = nullptr;
}

char* rage::sysSmallocator::Chunk::GetBlock() const
{
	return (char*)this + sizeof(Chunk); // NOLINT(clang-diagnostic-cast-qual)
}

rage::sysSmallocator::FreeNode* rage::sysSmallocator::Chunk::GetBlockAsFreeBlock() const
{
	return reinterpret_cast<FreeNode*>(GetBlock());
}

rage::sysSmallocator::FreeNode* rage::sysSmallocator::Chunk::AllocateBlock()
{
	// Remove first block from linked list and return it.

	FreeNode* block = MainBlock;
	MainBlock = block->NextLinked;

	FreeBlockCount--;
	return block;
}

rage::sysSmallocator::FreeNode* rage::sysSmallocator::Chunk::FreeBlock(pVoid block)
{
	FreeNode* node = static_cast<FreeNode*>(block);

	node->NextLinked = MainBlock;
	MainBlock = node;
	FreeBlockCount++;

	return node;
}

u32 rage::sysSmallocator::Chunk::GetAllocatedBlockCount() const
{
	return BucketParent->BlockCount - FreeBlockCount;
}

void rage::sysSmallocator::Bucket::InsertChunk(Chunk* chunk)
{
	if (MainChunk)
		MainChunk->PreviousLinked = chunk;

	chunk->NextLinked = MainChunk;
	MainChunk = chunk;
}

void rage::sysSmallocator::Bucket::DeleteChunk(const Chunk* chunk) const
{
	Chunk* nextChunk = chunk->NextLinked;
	Chunk* prevChunk = chunk->PreviousLinked;

	// Link next chunk with previous one from behind
	if (nextChunk)
		nextChunk->PreviousLinked = chunk->PreviousLinked;

	// If there's previous chunk, link it with next chunk.
	// Otherwise next chunk as root one
	if (prevChunk)
		prevChunk->NextLinked = nextChunk;
	else
		chunk->BucketParent->MainChunk = nextChunk;
}

u8 rage::sysSmallocator::GetBucket(u64 size) const
{
	return static_cast<u8>(((size >> MIN_BLOCK_SIZE_SHIFT) - 1));
}

u16 rage::sysSmallocator::GetChunkIndex(pVoid block) const
{
	u64 addr = reinterpret_cast<u64>(block);
	u64 offset = addr - m_ChunkBase;

	offset >>= CHUNK_ALIGN_SHIFT;
	offset >>= 5;

	return static_cast<u16>(offset);
}

u8 rage::sysSmallocator::GetChunkBit(pVoid block) const
{
	u64 addr = reinterpret_cast<u64>(block);
	u64 offset = addr - m_ChunkBase;

	return offset >> CHUNK_ALIGN_SHIFT & 31;
}

rage::sysSmallocator::Chunk* rage::sysSmallocator::GetChunkFromBlock(pVoid block) const
{
	return reinterpret_cast<Chunk*>((u64)block & ~CHUNK_MASK);
}

rage::sysSmallocator::Chunk* rage::sysSmallocator::AllocateNewChunk(u8 bucketIndex, sysMemSimpleAllocator* allocator)
{
	Bucket& bucket = m_Buckets[bucketIndex];

	pVoid chunkBlock = allocator->Allocate(CHUNK_ALLOC_SIZE, CHUNK_ALIGN);
	if (!chunkBlock)
		return nullptr;

	Chunk* chunk = new (chunkBlock) Chunk(&bucket);
	bucket.InsertChunk(chunk);

	u16 chunkIndex = GetChunkIndex(chunk);
	u8 chunkBit = GetChunkBit(chunk);
	m_ChunkStates[chunkIndex].SetBit(chunkBit, true);

	ALLOC_LOG("Smallocator::AllocateNewChunk() -> Inserthing chunk at %u at position: %u",
		chunkIndex, chunkBit);

	return chunk;
}

pVoid rage::sysSmallocator::DoAllocate(u8 bucketIndex, sysMemSimpleAllocator* allocator)
{
	ALLOC_LOG("Smallocator::DoAllocate() -> Allocating in bucket %i (%i)", bucketIndex, 1 << bucketIndex);

	Bucket& bucket = m_Buckets[bucketIndex];

	u32 chunkIndex = 0;

	Chunk* chunk = bucket.MainChunk;
	while (chunk)
	{
		// Get first available block in this chunk.
		// If no free block available, go to next chunk.

		ALLOC_LOG("Smallocator::DoAllocate() -> %u free slots left", chunk->FreeBlockCount);
		if (chunk->FreeBlockCount > 0)
		{
			ALLOC_LOG("Smallocator::DoAllocate() -> Found free slot in chunk pool at index %u", chunkIndex);

			return chunk->AllocateBlock();
		}

		chunk = chunk->NextLinked;
		chunkIndex++;
	}

	// We've got no free chunks left, allocate new one.

	ALLOC_LOG("Smallocator::DoAllocate() -> Pools are full, allocating new one...");

	chunk = AllocateNewChunk(bucketIndex, allocator);
	if (!chunk)
	{
		AM_ERR("Smallocator::DoAllocate() -> Failed to allocate new pool chunk");
		return nullptr;
	}
	return chunk->AllocateBlock();
}

void rage::sysSmallocator::Destroy(sysMemSimpleAllocator* allocator)
{
	// TODO: Print out leaks
	return;

	// We shouldn't have any chunks left if there's no memory leaks.
	bool hasLeaks = false;
	for (Bucket& bucket : m_Buckets)
	{
		Chunk* chunk = bucket.MainChunk;
		while (chunk)
		{
			if (!hasLeaks)
			{
				AM_ERRF("Smallocator::Destroy() -> Leaks detected.");
				hasLeaks = true;
			}

			Chunk* next = chunk->NextLinked;
			allocator->Free(chunk);
			chunk = next;
		}
	}

	if (!hasLeaks)
		ALLOC_LOG("Smallocator::Destroy() -> No leaks detected.");
}

void rage::sysSmallocator::Init(sysMemSimpleAllocator* allocator)
{
	m_ChunkBase = reinterpret_cast<u64>(allocator->GetHeapBase()) & ~CHUNK_MASK;

	// Calculate number of blocks in every pool
	u16 blockSize = 0;
	for (Bucket& bucket : m_Buckets)
	{
		// Hence block sizes are multiple of 16
		blockSize += MIN_BLOCK_SIZE;

		bucket.BlockSize = blockSize;
		bucket.BlockCount = CHUNK_POOL_SIZE / blockSize;
	}
}

bool rage::sysSmallocator::CanAllocate(u64 size, u64 align) const
{
	return size <= MAX_BLOCK_SIZE && align == 16;
}

pVoid rage::sysSmallocator::Allocate(u64 size, u64 align, sysMemSimpleAllocator* allocator)
{
	// Buckets are power of 16 (see ::Init)
	size = ALIGN_16(size);

	return DoAllocate(GetBucket(size), allocator);
}

void rage::sysSmallocator::Free(pVoid block, sysMemSimpleAllocator* allocator)
{
	Chunk* chunk = GetChunkFromBlock(block);
	chunk->FreeBlock(block);

	// Free chunk if it has no allocated blocks
	if (chunk->GetAllocatedBlockCount() != 0)
		return;

	chunk->BucketParent->DeleteChunk(chunk);

	// Remove chunk from allocated list
	u16 chunkIndex = GetChunkIndex(chunk);
	u8 chunkBit = GetChunkBit(chunk);
	m_ChunkStates[chunkIndex].SetBit(chunkBit, false);

	allocator->Free(chunk);
}

bool rage::sysSmallocator::IsPointerOwner(pVoid block) const
{
	u64 addr = reinterpret_cast<u64>(block);

	// Check whether address is in chunk memory range,
	// mark out clearly invalid addresses.
	// This is not really necessary, chance of this being true is very small.
	if ((addr & CHUNK_MASK) >= CHUNK_ALLOC_SIZE)
		return false;

	// Explained in comment for m_ChunkStates
	u16 chunkIndex = GetChunkIndex(block);
	u8 chunkBit = GetChunkBit(block);
	return m_ChunkStates[chunkIndex].IsBitSet(chunkBit);
}

u64 rage::sysSmallocator::GetSize(pVoid block) const
{
	return GetChunkFromBlock(block)->BucketParent->BlockSize;
}
