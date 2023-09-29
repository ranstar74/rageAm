#include "buddyallocator.h"

#include "am/system/asserts.h"

#include "memory.h"

#include "helpers/align.h"
#include "helpers/bits.h"
#include "helpers/ranges.h"

rage::sysCriticalSectionToken rage::sysMemBuddyAllocator::sm_CriticalSection;

void rage::sysBuddyHeap::AddToFreeList(u32 index, u8 level)
{
	ALLOC_LOG("BuddyHeap::AddToFreeList(index: %u, level: %u (buddy size: %u))",
		index, level, GetBuddySize(level));

	// This buddy now links to root buddy in free list
	sysBuddy& buddy = m_Buddies[index];
	buddy.PreviousIndex = sysBuddy::INDEX_NULL;
	buddy.NextIndex = m_FreeList[level];

	buddy.SetLevel(level);

	// Set this buddy as previous for root one, if there's any
	u32 rootIndex = m_FreeList[level];
	if (rootIndex != sysBuddy::INDEX_NULL)
		m_Buddies[rootIndex].PreviousIndex = index;

	m_LevelSize[level]++;
	m_FreeList[level] = index;
}

void rage::sysBuddyHeap::RemoveFromFreeList(u32 index, u8 level)
{
	ALLOC_LOG("BuddyHeap::RemoveFromFreeList(index: %u, level: %u (buddy size: %u))",
		index, level, GetBuddySize(level));

	sysBuddy& buddy = m_Buddies[index];

	// Connect next node with previous from left
	if (buddy.PreviousIndex == sysBuddy::INDEX_NULL)
		m_FreeList[level] = buddy.NextIndex;
	else
		m_Buddies[buddy.PreviousIndex].NextIndex = buddy.NextIndex;

	// Connect next node with previous from right
	if (buddy.NextIndex != sysBuddy::INDEX_NULL)
		m_Buddies[buddy.NextIndex].PreviousIndex = buddy.PreviousIndex;

	buddy.PreviousIndex = sysBuddy::INDEX_OUT_OF_TREE;
	buddy.NextIndex = sysBuddy::INDEX_OUT_OF_TREE;

	m_LevelSize[level]--;
}

u32 rage::sysBuddyHeap::AllocateRecurse(u8 level)
{
	// # - Allocated block
	// 
	// Initial state of heap (Note that 512 and two 256 blocks form one 1024 block)
	// ┌──────────┐  ┌───┐ ┌───┐  ┌───────────────────────┐
	// │   512    │  │256│ │256│  │       1024            │
	// │          │  │###│ │###│  │                       │
	// └──────────┘  └───┘ └───┘  └───────────────────────┘
	// 
	// Allocation of block with size of 256:
	//  1) We have no free blocks on 256 level, move one level up
	//  2) 512 block fits our requirements, return it
	//  3) Block is larger than our requirement, we'll have to split it on two buddies,
	//      replacing one 512 block on two blocks with size twice as small as 512
	// 
	//  State of heap after allocation
	// ┌───┐ ┌───┐  ┌───┐ ┌───┐  ┌───────────────────────┐
	// │256│ │256│  │256│ │256│  │       1024            │
	// │###│ │   │  │###│ │###│  │                       │
	// └───┘ └───┘  └───┘ └───┘  └───────────────────────┘
	// 
	// On de-allocation we'll have to do the same process but in reverse,
	// merge adjacent buddies to bring them in the initial state
	// 
	// For allocating next block of 256 we'll have to split 1024 block twice,
	// to get two 512 blocks first, then split 512 block on two blocks on 256
	// ┌───┐ ┌───┐  ┌───┐ ┌───┐  ┌───┐ ┌───┐  ┌──────────┐
	// │256│ │256│  │256│ │256│  │256│ │256│  │   512    │
	// │###│ │   │  │###│ │###│  │###│ │   │  │          │
	// └───┘ └───┘  └───┘ └───┘  └───┘ └───┘  └──────────┘

	ALLOC_LOG("BuddyHeap::AllocateRecurse(level: %u (buddy size: %u))",
		level, GetBuddySize(level));

	if (level >= BUDDY_MAX_LEVEL)
		return sysBuddy::INDEX_NULL;

	// Step #1

	// Check if there's any buddy in free list
	u32 index = m_FreeList[level];
	if (index != sysBuddy::INDEX_NULL)
	{
		RemoveFromFreeList(index, level);
		return index; // Step #2
	}

	// If level has no empty slot, we'll have to check next (larger) level
	index = AllocateRecurse(level + 1);
	if (index == sysBuddy::INDEX_NULL) // Out of memory.
		return sysBuddy::INDEX_NULL;

	// We've got here from Step #2 if we were unlucky to find free block of exactly requested size

	// Step #3

	// Now we found some larger free buddy, we have to split it on two buddies
	// return half and add remaining bit in the free list.
	AddToFreeList(GetBuddysBud(index, level), level);

	return index;
}

u32 rage::sysBuddyHeap::GetBuddysBud(u32 buddyIndex, u8 level) const
{
	// L                           R
	// ┌─────────────────────────┐ ┌─────────────────────────┐ Level 2: 2 nodes
	// └─────────────────────────┘ └─────────────────────────┘ Node size : 4
	// 0                           1
	// 
	// L             R             L             R
	// ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐ Level 1: 4 nodes
	// └───────────┘ └───────────┘ └───────────┘ └───────────┘ Node size : 2
	// 0             1             2             3
	// 
	// L      R      L      R      L      R      L      R
	// ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ Level 0: 8 nodes
	// └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ Node size: 1
	// 0      1      2      3      4      5      6      7
	// 
	// Each node has adjacent buddy, for left one it is the right one,
	// for right one - the left one.
	// 
	// In order to get the right buddy, we simply add node size to node index
	// Getting left node is the same process but reversed.
	// 
	// But how do we tell if node is left or right in the first place?
	// One interesting observation we can make is that left node is always even.
	// 
	// We get equation from this:
	// BuddyIndex = Index + (IsEven(Index) ? -BuddySize : BuddySize)
	// 
	// This is one way, to show idea behind this. Let's look at how it's actually implemented:
	// BuddyIndex = Index ^ BuddySize
	// 
	// Just as reminder, XOR works like toggle. If we XOR left buddy with buddy size, we simply
	// add buddy size, but if we XOR right buddy, we subtract it, because all bits are set.
	return buddyIndex ^ GetBuddySize(level);
}

rage::sysBuddy* rage::sysBuddyHeap::GetBuddy(u32 index) const
{
	if (index == sysBuddy::INDEX_NULL ||
		index == sysBuddy::INDEX_OUT_OF_TREE)
		return nullptr;

	return &m_Buddies[index];
}

void rage::sysBuddyHeap::PrintState() const
{
#ifdef ALLOC_PRINT_STATE
	ALLOC_LOG("BuddyHeap::PrintState");
	ALLOC_LOG("Used Memory: %llu", m_UsedMemory);
	ALLOC_LOG("Free list:");
	u8 numLevels = 0;
	for (u8 i = 0; i < BUDDY_MAX_LEVEL; i++)
	{
		sysBuddy* buddy = GetBuddy(m_FreeList[i]);
		if (!buddy)
			continue;

		u32 length = 0;
		while ((buddy = GetBuddy(buddy->NextIndex)))
			length++;

		numLevels++;
		ALLOC_LOG(" - 2^%u (buddy size: %u, length: %u)", i, GetBuddySize(i), length + 1);
	}

	if (numLevels == 0)
		ALLOC_LOG(" Free list is empty!");
#endif
}

void rage::sysBuddyHeap::Init(u32 buddyCountMask, sysBuddy* buddies)
{
	m_Buddies = buddies;
	m_BuddyCountMask = buddyCountMask;

	for (u32& i : m_FreeList)
		i = sysBuddy::INDEX_NULL;

	for (u32 i = 0; i < m_BuddyCountMask; i++)
		m_Buddies[i] = sysBuddy();

	// Reset all root buddy indices
	for (u32& index : m_FreeList)
		index = sysBuddy::INDEX_NULL;

	// Insert default buddies
	u8 level = BUDDY_MAX_LEVEL;
	u32 index = 0;
	u32 buddySize = GetBuddySize(level);
	while (buddySize > 0) // Brute force buddy size until it fits constraint
	{
		level--;

		// In original implementation: (buddySize & m_BuddyCountMask) != 0
		if (buddySize <= GetBuddyCount())
		{
			// This is native implementation which adds buddies of
			// every available size in free list by default,
			// this is more optimal for real usage, because
			// there's more chance that we will allocate smaller chunk
			// rather than chunk of heap size

			// AddToFreeList(index, level);
			// index += buddySize;

			// This is alternate (default) implementation, where only root
			// node is added, and then it's split if needed when allocating
			AddToFreeList(index, level + 1);
			break;
		}
		buddySize /= 2;
	}
	PrintState();
}

u32 rage::sysBuddyHeap::Allocate(u64 size)
{
	u8 level = BitScanR64(size);
	if (!IS_POWER_OF_TWO(size))
		level++;

	u32 index = AllocateRecurse(level);
	if (index == sysBuddy::INDEX_NULL) // Out of memory
		return index;

	u32 buddySize = GetBuddySize(level);
	u8 currentBucket = GetCurrentMemoryBucket();

	m_UsedMemory += buddySize;
	m_AllocCount[level]++;
	m_MemoryBuckets[currentBucket] += buddySize;

	sysBuddy& buddy = m_Buddies[index];
	buddy.ResetUserData();
	buddy.ResetLock();
	buddy.SetMemoryBucket(currentBucket);

	// Set level for all 'children' buddies,
	// we'll need that on de-allocation to merge them
	for (u32 i = index; i < index + buddySize; i++)
		m_Buddies[i].SetLevel(level);

	PrintState();
	return index;
}

void rage::sysBuddyHeap::Free(u32 index)
{
	ALLOC_LOG("BuddyHeap::Free(index: %u)", index);

	sysBuddy* node = GetBuddy(index);

	// Rockstar were lazy? with resources so buddy allocator is allowed
	// to delete pointer twice (first in resource destructor then in pgBase::FreeMemory)
	if (!node->GetIsAllocated())
		return;

	u8 level = node->GetLevel();
	u8 bucket = node->GetMemoryBucket();
	u32 buddySize = GetBuddySize(level);

	m_AllocCount[level]--;
	m_UsedMemory -= buddySize;
	m_MemoryBuckets[bucket] -= buddySize;

	node->SetMemoryBucket(0);
	node->ResetUserData();

	// Reset all children buddy levels
	node->ResetLevel();
	for (u32 i = index; i < index + buddySize; i++)
		m_Buddies[i].ResetLevel();

	// See also ::AllocateRecurse;
	// L
	// ┌───────────┐
	// │           ├──── Single buddy after merging
	// └───────────┘
	//
	// L      R
	// ┌────┐ ┌────┐
	// │    ├─┤    ├──── Adjacent buddy
	// └──┬─┘ └────┘
	//    │
	//    │
	//    └───── Buddy that we
	//           are de-allocating
	//
	// When allocating, we split buddies on two. On freeing process is reversed,
	// we check if two adjacent buddies are free and merge them into one, until
	// reaching top level or stopping due to either of two nodes not being free.
	// Similar merging process is shown in sysMemSimpleAllocator.
	while (true)
	{
		ALLOC_LOG("BuddyHeap::Free() -> Looking for buddy at level %u (buddy size: %u)", level, GetBuddySize(level));

		u32 buddyIndex = GetBuddysBud(index, level);

		// Only happens if we are on root level where we have only one buddy node
		if (buddyIndex >= m_BuddyCountMask)
			break;

		// We can merge only free buddies
		if (m_Buddies[buddyIndex].GetIsAllocated())
			break;

		// Our buddy was split on two or more smaller buddies, but we can join only
		// buddies of the same size
		if (m_Buddies[buddyIndex].GetLevel() != level)
			break;

		ALLOC_LOG("BuddyHeap::Free() -> Joining %u buddy at %u", index, buddyIndex);

		RemoveFromFreeList(buddyIndex, level);

		// When merging buddies, we merge right into left
		// So simply flip them if current buddy is right one
		if (buddyIndex < index)
		{
			// Same as swap(index, BuddyIndex)
			buddyIndex = index;
			index = GetBuddysBud(index, level);
		}
		sysBuddy& buddy = m_Buddies[buddyIndex];
		buddy.ResetLevel();
		buddy.PreviousIndex = sysBuddy::INDEX_OUT_OF_TREE;
		buddy.NextIndex = sysBuddy::INDEX_OUT_OF_TREE;

		// Now we can go on level deeper and see if we can merge any buddy there
		level++;
	}
	AddToFreeList(index, level); // Add buddy to it's new level after merging
	PrintState();
}

u64 rage::sysBuddyHeap::GetSize(u32 index) const
{
	if (index >= GetBuddyCount())
		return 0;

	sysBuddy& buddy = m_Buddies[index];
	if (buddy.GetIsAllocated())
		return GetBuddySize(buddy.GetLevel());

	return 0;
}

u64 rage::sysBuddyHeap::GetMemoryUsed(u8 memoryBucket) const
{
	if (memoryBucket < SYS_MEM_MAX_MEMORY_BUCKETS)
		return m_MemoryBuckets[memoryBucket];
	return m_UsedMemory;
}

u64 rage::sysBuddyHeap::GetAvailableMemory() const
{
	return GetBuddyCount() - m_UsedMemory;
}

u64 rage::sysBuddyHeap::GetLargestAvailableBlock() const
{
	u8 level = BUDDY_MAX_LEVEL;
	while (true)
	{
		if (--level == 0)
			return 0;

		if (m_FreeList[level] != sysBuddy::INDEX_NULL)
			return GetBuddySize(level);
	}
}

u64 rage::sysMemBuddyAllocator::GetOffset(pVoid block) const
{
	u64 address = reinterpret_cast<u64>(block);
	u64 heap = reinterpret_cast<u64>(m_Heap);

	return address - heap;
}

pVoid rage::sysMemBuddyAllocator::GetBuddyAddress(u32 index) const
{
	if (index == sysBuddy::INDEX_NULL)
		return nullptr;

	return static_cast<char*>(m_Heap) + ToAllocatorSpace(index);
}

u32 rage::sysMemBuddyAllocator::GetBuddyIndex(pVoid block) const
{
	return static_cast<u32>(ToBuddyHeapSpace(GetOffset(block)));
}

bool rage::sysMemBuddyAllocator::MayBeValid(pVoid block) const
{
	if (!block)
		return false;

	u64 address = reinterpret_cast<u64>(block);
	u64 heap = reinterpret_cast<u64>(m_Heap);

	u64 offset = address - heap;

	// Range
	if (address < heap || offset >= m_Size)
		return false;

	// Since all memory blocks are power of two and greater or equal to minimum buddy block size,
	// buddy address must be a multiple of minimum buddy size, hence it should divide without remainder.
	// - Additionally this 'trick' allows us to get rid of block headers and use mini hash map
	// for allocated buddies by using [offset / minBlockSize] as hash function.
	return offset % m_MinBuddySize == 0;
}

rage::sysMemBuddyAllocator::sysMemBuddyAllocator()
{
	// Empty constructor for GrowBuddy array, shouldn't be used anywhere else.

	m_MinBuddySize = 0;
	m_Size = 0;
	m_Heap = nullptr;
	m_Buddies = nullptr;
}

rage::sysMemBuddyAllocator::sysMemBuddyAllocator(pVoid heap, u64 minBuddySize, u32 buddyCount, sysBuddy* buddies)
{
	AM_ASSERT(heap, "BuddyAllocator() -> Memory heap was NULL.");

	m_MinBuddySize = minBuddySize;
	m_Buddies = buddies;

	m_Heap = heap;
	m_Size = buddyCount * m_MinBuddySize;

	u32 buddyCountMask = buddyCount - 1;
	m_BuddyHeap.Init(buddyCountMask, m_Buddies);
}

pVoid rage::sysMemBuddyAllocator::Allocate(u64 size, u64 align, u32 type)
{
	sysCriticalSectionLock lock(sm_CriticalSection);

	ALLOC_LOG("BuddyAllocator::Allocate(%llu)", size);

	// Allocation can't be smaller than smallest block
	size = MAX(size, m_MinBuddySize);

	u64 newSize = ALIGN_POWER_OF_TWO_64(size);
	if (size != newSize)
		ALLOC_LOG("BuddyAllocator::Allocate() -> Allocation of size %llu is not power of two, rounded up to %llu, losing %llu",
			size, newSize, newSize - size);

	u32 slot = m_BuddyHeap.Allocate(ToBuddyHeapSpace(newSize));
	pVoid block = GetBuddyAddress(slot);

	if (block)
		ALLOC_LOG("BuddyAllocator::Allocate() -> Allocated at %p", block);
	else
		ALLOC_LOG("BuddyAllocator::Allocate() -> Failed to allocate block with size of %llu", newSize);

	return block;
}

pVoid rage::sysMemBuddyAllocator::TryAllocate(u64 size, u64 align, u32 type)
{
	// Buddy allocator does not throw if allocation was unsuccessful, simply call allocate.
	return Allocate(size, align, type);
}

void rage::sysMemBuddyAllocator::Free(pVoid block)
{
	if (!MayBeValid(block))
		return;

	sysCriticalSectionLock lock(sm_CriticalSection);

	m_BuddyHeap.Free(GetBuddyIndex(block));
}

u64 rage::sysMemBuddyAllocator::GetSize(pVoid block)
{
	if (!MayBeValid(block))
		return 0;

	sysCriticalSectionLock lock(sm_CriticalSection);

	return ToAllocatorSpace(m_BuddyHeap.GetSize(GetBuddyIndex(block)));
}

u64 rage::sysMemBuddyAllocator::GetMemoryUsed(u8 memoryBucket)
{
	return ToAllocatorSpace(m_BuddyHeap.GetMemoryUsed(memoryBucket));
}

u64 rage::sysMemBuddyAllocator::GetMemoryAvailable()
{
	return ToAllocatorSpace(m_BuddyHeap.GetAvailableMemory());
}

u64 rage::sysMemBuddyAllocator::GetLargestAvailableBlock()
{
	return ToAllocatorSpace(m_BuddyHeap.GetLargestAvailableBlock());
}

bool rage::sysMemBuddyAllocator::IsValidPointer(pVoid block)
{
	if (!block)
		return false;

	u64 address = reinterpret_cast<u64>(block);
	u64 heap = reinterpret_cast<u64>(m_Heap);
	u64 offset = address - heap;

	return address >= heap && offset < m_Size;
}

s32 rage::sysMemGrowBuddyAllocator::AllocateNewBuddy(u64 size)
{
	s32 index = m_AllocatorCount;

	u32 buddyCount = static_cast<u32>(size / m_MinBuddySize);
	u64 budsSize = sizeof sysBuddy * buddyCount;

	sysBuddy* buddies = static_cast<sysBuddy*>(sysMemVirtualAlloc(budsSize));

	if (!buddies)
	{
		AM_ERRF("GrowBuddy::DoGrow() -> Unable to allocate %llu bytes for buddies", budsSize);
		return -1;
	}

	pVoid heap = sysMemVirtualAlloc(size);
	if (!heap)
	{
		sysMemVirtualFree(buddies);

		AM_ERRF("GrowBuddy::DoGrow() -> Unable to allocate %llu bytes for heap", size);
		return -1;
	}

	m_BuddysPool[index] = buddies;
	m_MemoryPool[index] = heap;
	m_Allocators[index] = sysMemBuddyAllocator(heap, m_MinBuddySize, buddyCount, buddies);
	m_AllocatorCount++;

	return index;
}

s32 rage::sysMemGrowBuddyAllocator::DoGrow()
{
	if (m_AllocatorCount >= GROW_BUDDY_MAX_ALLOCATORS)
	{
		AM_ERR("GrowBuddy::DoGrow() -> All slots are used.");
		return -1;
	}

	// Try to allocate until we succeed or reach minimum size limit
	// As said in header, this was actual only on old-generation consoles
	// and not issue for modern PC.

	u64 size = m_Size;
	do
	{
		s32 index = AllocateNewBuddy(size);
		if (index != -1)
		{
			ALLOC_LOG("GrowBuddy::DoGrow() -> Allocated at slot %i", index);
			return index;
		}

		size /= 2;
	} while (size >= GROW_BUDDY_MIN_SIZE);
	AM_ERR("GrowBuddy::DoGrow() -> System is out of virtual memory");
	return -1;
}

rage::sysMemGrowBuddyAllocator::sysMemGrowBuddyAllocator(u64 minBuddySize, u64 size)
{
	m_MinBuddySize = minBuddySize;
	m_Size = size;

	m_ActiveAllocatorIndex = DoGrow();
}

rage::sysMemGrowBuddyAllocator::~sysMemGrowBuddyAllocator()
{
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		sysMemVirtualFree(m_MemoryPool[i]);
		sysMemVirtualFree(m_BuddysPool[i]);
	}
}

pVoid rage::sysMemGrowBuddyAllocator::Allocate(u64 size, u64 align, u32 type)
{
	sysCriticalSectionLock lock(sysMemBuddyAllocator::sm_CriticalSection);

	pVoid block;

	sysMemAllocator& activeAllocator = m_Allocators[m_ActiveAllocatorIndex];
	block = activeAllocator.Allocate(size);

	if (!block) // Active allocator is full, try to find other allocator
	{
		for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
		{
			if (!IsAllocatorValid(i))
				continue;

			block = m_Allocators[i].Allocate(size);
			if (block)
			{
				m_ActiveAllocatorIndex = i;
				break;
			}
		}
		// All allocators are full, we're out of memory

		// Native implementation doesn't throw in any scenario most likely because
		// it's used only for resources with real-time streaming, this need more investigation.
		return nullptr;
	}
	return block;
}

void rage::sysMemGrowBuddyAllocator::Free(pVoid block)
{
	// Similarly to Allocate, this function doesn't throw if block is invalid

	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (!IsAllocatorValid(i))
			continue;

		sysMemBuddyAllocator& allocator = m_Allocators[i];
		if (!allocator.IsValidPointer(block))
			continue;

		allocator.Free(block);
		return;
	}
}

u64 rage::sysMemGrowBuddyAllocator::GetSize(pVoid block)
{
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (!IsAllocatorValid(i))
			continue;

		sysMemBuddyAllocator& allocator = m_Allocators[i];
		if (!allocator.IsValidPointer(block))
			continue;

		return allocator.GetSize(block);
	}
	return 0;
}

u64 rage::sysMemGrowBuddyAllocator::GetMemoryUsed(u8 memoryBucket)
{
	u64 total = 0;
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (IsAllocatorValid(i))
			total += m_Allocators[i].GetMemoryUsed(memoryBucket);
	}
	return total;
}

u64 rage::sysMemGrowBuddyAllocator::GetMemoryAvailable()
{
	// We can't really get actual available memory because
	//  this is a grow allocator, when there's not enough memory
	//  in existing allocators - we add one more.
	// So if we don't have any created allocator, we theoretically have
	//  no memory available, but it's not right.
	// 
	// What we do instead (or well, rockstar do, let's be honest here)
	//  is we get total available memory from all allocators and check
	//  if it matches some arbitrary lowest minimum.
	u64 available;
	do
	{
		u64 heapSize = 0;
		for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
		{
			if (!IsAllocatorValid(i))
				continue;

			heapSize += m_Allocators[i].GetHeapSize();
		}

		// Check if we've got fine amount of available memory
		available = heapSize - GetMemoryUsed();
		if (available >= GROW_BUDDY_LARGE_BLOCK_SIZE)
			return available;
	} while (DoGrow() != -1); // Grow until we get enough memory or until we've out of memory

	return available;
}

u64 rage::sysMemGrowBuddyAllocator::GetLargestAvailableBlock()
{
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (!IsAllocatorValid(i))
			continue;

		u64 size = m_Allocators[i].GetLargestAvailableBlock();
		if (size >= GROW_BUDDY_LARGE_BLOCK_SIZE)
			return size;
	}

	// Largest available block was not large enough...
	// Construct new allocator and get largest block from it
	s32 newAllocator = DoGrow();
	if (newAllocator != -1)
		return m_Allocators[newAllocator].GetLargestAvailableBlock();

	return 0; // Who did install 5000 HQ vehicle mods again?
}

void rage::sysMemGrowBuddyAllocator::SanityCheck()
{
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (IsAllocatorValid(i))
			m_Allocators[i].SanityCheck();
	}
}

bool rage::sysMemGrowBuddyAllocator::IsValidPointer(pVoid block)
{
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (!IsAllocatorValid(i))
			continue;

		if (m_Allocators[i].IsValidPointer(block))
			return true;
	}
	return false;
}

u64 rage::sysMemGrowBuddyAllocator::GetHeapSize()
{
	u64 total = 0;
	for (s32 i = 0; i < GROW_BUDDY_MAX_ALLOCATORS; i++)
	{
		if (IsAllocatorValid(i))
			total += m_Allocators[i].GetHeapSize();
	}
	return total;
}
