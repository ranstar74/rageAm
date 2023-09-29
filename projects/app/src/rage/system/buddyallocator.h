#pragma once

#include "allocator.h"

#include "ipc/criticalsection.h"

namespace rage
{
	// Good read on this topic:
	// https://bitsquid.blogspot.com/2015/08/allocation-adventures-3-buddy-allocator.html
	//
	// In rage this allocator is used for resource memory chunks.
	// Biggest con of buddy allocator is that it can only allocate blocks with size of power-of-two,
	// but even this can be more or less avoided by packing chunks carefully (see paging/compiler/pgRscPacker.h)
	// Other than this, it has very low memory fragmentation, which is just perfect for
	// efficient and long-running streaming system of open world game.

	// Maximum size constraint for single buddy.
	static constexpr u32 BUDDY_MAX_LEVEL = 24;

#pragma pack(push, 1) // Size is 13 bytes
	struct sysBuddy
	{
		// An invalid index.
		static constexpr u32 INDEX_NULL = -1;
		// Index that assigned to buds that are allocated (and removed from linked list)
		static constexpr u32 INDEX_OUT_OF_TREE = -2;

		static constexpr u16 LEVEL_MASK = 0x1F; // Low 5 bits

		static constexpr u16 LOCK_SHIFT = 5;
		static constexpr u16 LOCK_SIZE = 0x7F; // 7 bits
		static constexpr u16 LOCK_MASK = LOCK_SIZE < LOCK_SHIFT;

		static constexpr u16 BUCKET_SHIFT = 12;
		static constexpr u16 BUCKET_SIZE = 0xF; // 4 bits
		static constexpr u16 BUCKET_MASK = BUCKET_SIZE < BUCKET_SHIFT; // Top 4 bits

		u32 PreviousIndex = INDEX_OUT_OF_TREE;
		u32 NextIndex = INDEX_OUT_OF_TREE;

		// sysBuddy::Data bit layout
		// 
		//          ┌─ Lock Data (0 - 127) (Not researched)
		//          │
		//          │           ┌── Level (0 - 31)
		//          │           │
		// ┌────┐ ┌─┴──────┐ ┌──┴───┐
		// │0000│ │0000 000│ │0 0000│
		// └─┬──┘ └────────┘ └──────┘
		//   │
		//   └─ Memory bucket index (0 - 15)
		//
		u16 Data = 0;

		// 24 Bits, Purpose is unknown

		u16 UserDataHigh = -1;
		u8 UserDataLow = -1;

		sysBuddy()
		{
			SetLevel(BUDDY_MAX_LEVEL); // Invalid level
		}

		bool GetIsAllocated() const { return NextIndex == INDEX_OUT_OF_TREE; }

		u8 GetLevel() const { return Data & LEVEL_MASK; }
		void SetLevel(u8 level)
		{
			Data &= ~LEVEL_MASK;
			Data |= level & LEVEL_MASK;
		}

		u8 GetMemoryBucket() const { return Data >> BUCKET_SHIFT & BUCKET_SIZE; }
		void SetMemoryBucket(u8 bucket)
		{
			Data &= ~BUCKET_MASK;
			Data |= bucket << BUCKET_SHIFT;
		}

		u8 GetLockData() const { return Data >> LOCK_SHIFT & LOCK_SIZE; }
		void SetLockData(u8 lock)
		{
			Data &= ~LOCK_MASK;
			Data |= lock << LOCK_SHIFT;
		}

		void ResetLock() { SetLockData(1); }
		void ResetLevel() { SetLevel(BUDDY_MAX_LEVEL); }
		void ResetUserData()
		{
			UserDataHigh = -1;
			UserDataLow = -1;
		}
	};
#pragma pack(pop)

	/**
	 * \brief Manages buddies of unit size.
	 */
	class sysBuddyHeap
	{
		// Pre-allocated pool of buddies.
		sysBuddy* m_Buddies = nullptr;

		// I've no idea why it's stored as mask, we have to do all checks with <= and >= because of it.
		// The only place where it's used as an actual mask is default implementation of Init function
		// where it was used to check if buddy size at level was smaller than total num of buds.
		// Why, Rockstar?
		u32 m_BuddyCountMask = 0;

		u64 m_UsedMemory = 0;

		u64 m_MemoryBuckets[SYS_MEM_MAX_MEMORY_BUCKETS]{};

		u32 m_AllocCount[BUDDY_MAX_LEVEL]{}; // Count of allocated buddies per every level.
		u32 m_LevelSize[BUDDY_MAX_LEVEL]{}; // Count of buddy nodes per every level.
		u32 m_FreeList[BUDDY_MAX_LEVEL]{}; // Index of root buddy node at each level. See sysMemSimpleAllocator::m_FreeList;

		u32 GetBuddyCount() const { return m_BuddyCountMask + 1; /* Read comment for m_BuddyCountMask; */ }

		// Inserts a buddy in the beginning of linked list at given level.
		void AddToFreeList(u32 index, u8 level);

		// Removes buddy from linked list at given level.
		void RemoveFromFreeList(u32 index, u8 level);

		// Buddy size is how much 'indices' it covers.
		// For example if we've got only one buddy it will cover every single node in m_Buddies list.
		u32 GetBuddySize(u8 level) const { return 1 << level; }

		// Allocates a new buddy at given level.
		// Returns: sysBuddy index if allocated; Otherwise INDEX_NULL.
		u32 AllocateRecurse(u8 level);

		// Gets adjacent buddy of the given buddy (right one if buddy is 'left one', otherwise left one).
		u32 GetBuddysBud(u32 buddyIndex, u8 level) const;

		// Gets buddy pointer at given index, if index is valid (not INDEX_NULL or INDEX_OUT_OF_TREE).
		sysBuddy* GetBuddy(u32 index) const;

		// Prints out all free blocks and their sizes in allocator log stream.
		void PrintState() const;
	public:
		void Init(u32 buddyCountMask, sysBuddy* buddies);

		// Returns allocated buddy index, if successful; Otherwise sysBuddy::INDEX_NULL;
		u32 Allocate(u64 size);
		void Free(u32 index);

		u64 GetSize(u32 index) const;
		u64 GetMemoryUsed(u8 memoryBucket) const;
		u64 GetAvailableMemory() const;
		u64 GetLargestAvailableBlock() const;
	};

	class sysMemGrowBuddyAllocator;

	/**
	 * \brief Allocates memory in power-of-two sized chunks.
	 * \n This allocator is used for paged resources.
	 * \n NOTE: This allocator does not support aligning.
	 */
	class sysMemBuddyAllocator : public sysMemAllocator
	{
		friend class sysMemGrowBuddyAllocator; // To access sync token and private empty constructor
		static sysCriticalSectionToken sm_CriticalSection;

		pVoid m_Heap;

		sysBuddy* m_Buddies;

		u64 m_Size;
		u64 m_MinBuddySize;

		sysBuddyHeap m_BuddyHeap;

		// Buddy heap manages buddies with size of 1, while allocator have specified
		// by user minimum buddy block size. We have to convert units to exchange information.

		u64 ToBuddyHeapSpace(u64 value) const { return value /= m_MinBuddySize; }
		u64 ToAllocatorSpace(u64 value) const { return value *= m_MinBuddySize; }

		// Gets offset of memory address relative to heap.
		u64 GetOffset(pVoid block) const;

		// Gets address of buddy block, if index is valid; Otherwise nullptr.
		pVoid GetBuddyAddress(u32 index) const;

		// Returns index of buddy in tree from memory address,
		// if address is valid; Otherwise INDEX_NULL;
		u32 GetBuddyIndex(pVoid block) const;

		bool MayBeValid(pVoid block) const;

		sysMemBuddyAllocator(); // Reserved for GrowBuddy
	public:
		sysMemBuddyAllocator(pVoid heap, u64 minBuddySize, u32 buddyCount, sysBuddy* buddies);

		// NOTE: Aligning is not performed in buddy allocator.
		pVoid Allocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override;
		pVoid TryAllocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override;

		void Free(pVoid block) override;

		void Resize(pVoid block, u64 newSize) override { /* We can't shrink buddy in any way... */ }

		u64 GetSize(pVoid block) override;
		u64 GetMemoryUsed(u8 memoryBucket = SYS_MEM_INVALID_BUCKET) override;
		u64 GetMemoryAvailable() override;
		u64 GetLargestAvailableBlock() override;

		// Following functions are not implemented because they're used only on simple allocator
		// For the same reason sysMemBuddyAllocator is not inherited from sysMemAllocator in RDR3 anymore

		u64 GetLowWaterMark(bool updateMeasure) override { return 0; }
		u64 GetHighWaterMark(bool updateMeasure) override { return 0; }

		void UpdateMemorySnapshots() override {}
		u64 GetMemorySnapshot(u8 memoryBucket) override { return 0; }

		bool IsTailed() override { return true; }

		u64 BeginLayer() override { return 0; }
		void EndLayer(const char* layerName, const char* logPath) override {}

		void BeginMemoryLog(const char* fileName, bool traceStack) override {}
		void EndMemoryLog() override {}

		bool IsBuildingResource() override { return false; }
		bool HasMemoryBuckets() override { return false; }

		void SanityCheck() override {} // TODO: Implement
		bool IsValidPointer(pVoid block) override;

		u64 GetSizeWithOverhead(pVoid block) override
		{
			// Buddy allocator has no block header or anything
			return GetSize(block);
		}

		u64 GetHeapSize() override { return m_Size; }
		pVoid GetHeapBase() override { return m_Heap; }
		void SetHeapBase(pVoid newBase) override { m_Heap = newBase; }

		// TODO: Vftable is not complete
	};

	/**
	 * \brief Similar to MultiAllocator, manages multiple buddy allocators.
	 */
	class sysMemGrowBuddyAllocator : public sysMemAllocator /* In V it's mistakenly inherited from buddy allocator */
	{
		static constexpr s32 GROW_BUDDY_MAX_ALLOCATORS = 32;

		static constexpr u32 GROW_BUDDY_MIN_SIZE = 0x1000000; // 16MB~

		// Used as suitable available size to use or grow instead
		static constexpr u32 GROW_BUDDY_LARGE_BLOCK_SIZE = 0x400000;

		// Because of R* dev mistake this class inherits from sysMemBuddyAllocator, meaning that all fields are copied and never used
		// I leave it as padding to preserve field offsets
		char m_LimboBuddy[472]{}; // NOLINT(clang-diagnostic-unused-private-field)

		u64 m_Size;
		u64 m_MinBuddySize;

		s32 m_ActiveAllocatorIndex = -1;
		s32 m_AllocatorCount = 0;
		sysMemBuddyAllocator m_Allocators[GROW_BUDDY_MAX_ALLOCATORS]{};

		// Heaps & Buddies for every allocator

		sysBuddy* m_BuddysPool[GROW_BUDDY_MAX_ALLOCATORS]{};
		pVoid m_MemoryPool[GROW_BUDDY_MAX_ALLOCATORS]{};

		// Gets whether allocator at given index was constructed can be used.
		bool IsAllocatorValid(u32 slot) const { return m_MemoryPool[slot] != nullptr; }

		// Tries to allocate new buddy heap with specified size.
		// Returns index if successfully; Otherwise -1
		s32 AllocateNewBuddy(u64 size);

		// Tries to allocate new buddy heap with sizes from m_Size to GROW_BUDDY_MIN_SIZE
		// (While on modern PC it's hard to run out of system memory, for X-box 360 / PS3 it was the different case)
		// Returns index if allocated successfully; Otherwise -1
		s32 DoGrow();
	public:
		sysMemGrowBuddyAllocator(u64 minBuddySize, u64 size);
		~sysMemGrowBuddyAllocator() override;

		pVoid Allocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override;
		pVoid TryAllocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override
		{
			return Allocate(size, align, type);
		}

		void Free(pVoid block) override;

		void Resize(pVoid block, u64 newSize) override { /* Buddy can't be resized in any way. */ }

		u64 GetSize(pVoid block) override;
		u64 GetMemoryUsed(u8 memoryBucket = SYS_MEM_INVALID_BUCKET) override;
		u64 GetMemoryAvailable() override;
		u64 GetLargestAvailableBlock() override;

		// Just like in BuddyAllocator, following functions are not implemented

		u64 GetLowWaterMark(bool updateMeasure) override { return 0; }
		u64 GetHighWaterMark(bool updateMeasure) override { return 0; }

		void UpdateMemorySnapshots() override {}
		u64 GetMemorySnapshot(u8 memoryBucket) override { return 0; }

		bool IsTailed() override { return true; }

		u64 BeginLayer() override { return 0; }
		void EndLayer(const char* layerName, const char* logPath) override {}

		void BeginMemoryLog(const char* fileName, bool traceStack) override {}
		void EndMemoryLog() override {}

		bool IsBuildingResource() override { return false; }
		bool HasMemoryBuckets() override { return false; }

		void SanityCheck() override;
		bool IsValidPointer(pVoid block) override;

		u64 GetSizeWithOverhead(pVoid block) override { return GetSize(block); /* No overhead */ }

		u64 GetHeapSize() override;
		pVoid GetHeapBase() override { return m_Allocators[0].GetHeapBase(); /* Extremely useful */ }
	};
}
