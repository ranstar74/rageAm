#pragma once

#include "allocator.h"

#include "helpers/bitset.h"

namespace rage
{
	class sysMemSimpleAllocator;

	/**
	 * \brief 'Slab' - like allocator, performs allocation of blocks with sizes from 16 to 128.
	 * \n NOTE: This allocator is made for internal use with SimpleAllocator and has no fool protection on public functions.
	 */
	class sysSmallocator
	{
		struct Bucket;

		/**
		 * \brief When unallocated, pool memory block is one-way linked list.
		 */
		struct FreeNode
		{
			FreeNode* NextLinked;
		};

		/**
		 * \brief A pool of memory blocks of fixed size.
		 */
		struct Chunk
		{
			Chunk* PreviousLinked = nullptr;
			Chunk* NextLinked = nullptr;

			u32 FreeBlockCount;

			FreeNode* MainBlock;

			Bucket* BucketParent;

			// Not related to ::Bucket! See allocator.h -> ::GetCurrentMemoryBucket
			u8 MemoryBucket;

			u8 mUnused2C[16]{};

			Chunk(Bucket* bucket);

			// Gets address of first free block.
			char* GetBlock() const;

			// Gets block as FreeNode, used for building linked list.
			FreeNode* GetBlockAsFreeBlock() const;

			// Allocates free block and removes it from linked list.
			FreeNode* AllocateBlock();

			// Inserts node at beginning of linked list and returns as FreeNode.
			FreeNode* FreeBlock(pVoid block);

			// Gets number of currently allocated blocks in this chunk.
			u32 GetAllocatedBlockCount() const;
		};

		/**
		 * \brief Bucket contains array of memory pools, separated by allocation size.
		 */
		struct Bucket
		{
			Chunk* MainChunk;

			u16 BlockSize;
			u16 BlockCount;

			// Adds chunk in beginning of linked list.
			void InsertChunk(Chunk* chunk);

			// Removes chunk from linked list.
			void DeleteChunk(const Chunk* chunk) const;
		};

		// Explained in comment for m_ChunkStates below.

		static constexpr u64 CHUNK_ALIGN_SHIFT = 14;
		static constexpr u64 CHUNK_ALIGN = 1 << CHUNK_ALIGN_SHIFT;
		static constexpr u64 CHUNK_MASK = CHUNK_ALIGN - 1; // Low 13 bits.

		// Full size of allocated ::Chunk structure, 16 is SimpleAllocator header size.
		// This is full available memory range before 'overflowing' into next multiple if CHUNK_ALIGN.
		static constexpr u64 CHUNK_ALLOC_SIZE = CHUNK_ALIGN - 16;

		// Memory available for allocating blocks.
		static constexpr u64 CHUNK_POOL_SIZE = CHUNK_ALLOC_SIZE - sizeof(Chunk);

		static constexpr u32 BUCKET_COUNT = 8;

		static constexpr u32 MIN_BLOCK_SIZE_SHIFT = 4;
		static constexpr u32 MIN_BLOCK_SIZE = 1 << MIN_BLOCK_SIZE_SHIFT;
		static constexpr u8 MAX_BLOCK_SIZE = MIN_BLOCK_SIZE * BUCKET_COUNT;

		static constexpr u32 BITFIELD_ARRAY_SIZE = SYS_GENERAL_MAX_HEAP_SIZE >> CHUNK_ALIGN_SHIFT >> 5;

		// Every bucket has list of chunks, whose can be seen as memory pools.
		// 
		// Once there is not enough memory in chunk,
		//  another one is inserted at the beginning of bucket's chunk linked list.
		// 
		// Size of pool (allocation unit count) in each chunk
		//  is defined by size of the chunk (16304) divided by allocation unit size.
		// 
		//                  ┌─────┐  ┌─────┐  ┌─────┐
		//  Bucket 0 (16)   │Chunk├─►│Chunk├─►│Chunk│
		//                  └─────┘  └─────┘  └─────┘
		// 
		//                  ┌─────┐  ┌─────┐
		//  Bucket 1 (32)   │Chunk├─►│Chunk│
		//                  └─────┘  └─────┘
		// 
		//                  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
		//  Bucket 2 (48)   │Chunk├─►│Chunk├─►│Chunk├─►│Chunk│
		//                  └─────┘  └─────┘  └─────┘  └─────┘
		Bucket m_Buckets[BUCKET_COUNT]{};

		u64 m_Unused2E8 = 0; // NOLINT(clang-diagnostic-unused-private-field)
		u64 m_Unused2F0 = 0; // NOLINT(clang-diagnostic-unused-private-field)

		// This can be seen as address of the first chunk, used to calculate chunk offset.
		// For more details, see comment for m_ChunkStates
		u64 m_ChunkBase = 0;

		// This allocator has no block header, so how do we get parent chunk from arbitrary pointer?
		// 
		// In buddy allocator we know that every pointer is multiple of the smallest
		// block that we can allocate, and given that we can map every address to small table.
		// 
		// In this case, every address is multiple of 16 and gives us very large map size.
		// So how do we solve this problem with minimum memory footprint?
		// 
		// If we align chunk address to multiple of 2, let N: exponent, we get all low bits set to 0.
		// If it's unclear why, let's look at bit representation of some power-of-two numbers:
		// 16 - 0001 0000
		// 32 - 0010 0000
		// 64 - 0100 0000
		// 
		// Given that, for chunk aligned to 2^N, it leaves us N-1-size of (chunk) memory pool.
		// In this implementation, N was chosen to be 14, which gives 16 384 bytes.
		// 
		// And to get chunk from pointer, we simply clear all low 13 bits. We can do that with simple bit tick:
		// Let mask: 1 << 14 - 1, which will look like: 0011 1111 1111 1111.
		// We have to invert it to clear unwanted bits (~mask)
		// 
		// Chunk address = pointer & ~mask
		// 
		// Alright, we've got chunk from pointer. But how do we tell that it is actual chunk?
		// And not some unrelated memory address.
		// 
		// One way is to use some kind of checksum or constant field
		//  (It's actually way more simpler approach comparing to actual rage implementation)
		// 
		// What we do is we calculate chunk offset from base heap address where chunk is allocated
		// and divide that by 2^N. This will give us index that we can use in metadata array to verify chunk.
		// Index = (Chunk address - Heap address) >> N
		// 
		// But what is the maximum index? It depends on heap size, default general allocator in GTA V uses 500MB.
		// which is 524 288 000 byte range. Rockstar chosen size of 0x4000'0000 or 1 073 741 824 bytes (~1GB) to be absolute maximum size.
		// (It was derived from reversed process of getting index 2048 << 14 << 5)
		// 
		// 524 288 000 >> 14 = 65 536. Seems to not that bad on the first glance.
		// 
		// Let's think what do we store in this metadata array to verify that chunk is indeed valid? Single bit is enough.
		// But we can't have array of bits! Even with array of 1 byte ( u8[64536]) we will waste a lot of memory!
		// 
		// This gives us another idea: Let's use bit field! If we pick common bitfield size of 32 bits, it gives us
		// 65 536 / 32 (or 65 536 >> 5) = 2048. Only 2KB with absolutely no wasted memory.
		// 
		// Now we have to define 2 functions to obtain bitfield index and bit index:
		// 
		// Index = offset >> 14 >> 5
		// Bit   = (offset >> 14) % 32 (since 32 is power of two that can be also written as offset >> 14 & 31)
		// 
		// They're implemented in GetChunkIndex and GetChunkBit functions.
		BitSet32 m_ChunkStates[BITFIELD_ARRAY_SIZE]{};

		// Size has to be multiple of 16.
		u8 GetBucket(u64 size) const;

		// Gets block index in bitfield chunk state array.
		u16 GetChunkIndex(pVoid block) const;

		// Gets block bit in chunk state bitfield.
		u8 GetChunkBit(pVoid block) const;

		// Gets chunk from it's block, does not perform any checks.
		Chunk* GetChunkFromBlock(pVoid block) const;

		// Allocates new chunk in given bucket.
		Chunk* AllocateNewChunk(u8 bucketIndex, sysMemSimpleAllocator* allocator);

		pVoid DoAllocate(u8 bucketIndex, sysMemSimpleAllocator* allocator);
	public:
		void Destroy(sysMemSimpleAllocator* allocator);
		void Init(sysMemSimpleAllocator* allocator);

		bool CanAllocate(u64 size, u64 align) const;

		pVoid Allocate(u64 size, u64 align, sysMemSimpleAllocator* allocator);

		void Free(pVoid block, sysMemSimpleAllocator* allocator);

		bool IsPointerOwner(pVoid block) const;

		u64 GetSize(pVoid block) const;
	};
}
