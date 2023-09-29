#include "multiallocator.h"

#include "am/system/asserts.h"

void rage::sysMemMultiAllocator::Free(pVoid block)
{
	if (!block)
		return;

	sysMemAllocator* owner = GetPointerOwner(block);
	AM_ASSERT(owner, "MultiAllocator::Free() -> Pointer is not valid!");
	owner->Free(block);
}

void rage::sysMemMultiAllocator::Resize(pVoid block, u64 newSize)
{
	if (!block)
		return;

	sysMemAllocator* owner = GetPointerOwner(block);
	AM_ASSERT(owner, "MultiAllocator::Resize() -> Pointer is not valid!");
	owner->Resize(block, newSize);
}

rage::sysMemAllocator* rage::sysMemMultiAllocator::GetPointerOwner(pVoid block)
{
	for (sysMemAllocator* allocator : m_Allocators)
	{
		if (!allocator)
			continue;

		if (!allocator->IsValidPointer(block))
			continue;

		return allocator;
	}
	return nullptr;
}

u64 rage::sysMemMultiAllocator::GetSize(pVoid block)
{
	sysMemAllocator* owner = GetPointerOwner(block);
	if (owner)
		return owner->GetSize(block);
	return 0;
}

u64 rage::sysMemMultiAllocator::GetMemoryUsed(u8 memoryBucket)
{
	u64 total = 0;
	for (sysMemAllocator* allocator : m_Allocators)
		total += allocator->GetMemoryUsed(memoryBucket);
	return total;
}

u64 rage::sysMemMultiAllocator::GetMemoryAvailable()
{
	u64 total = 0;
	for (sysMemAllocator* allocator : m_Allocators)
		total += allocator->GetMemoryAvailable();
	return total;
}

u64 rage::sysMemMultiAllocator::GetLargestAvailableBlock()
{
	// It wouldn't make any sense if we return largest block
	// from random allocator without returning it's index.
	// Probably was worth to add out param, but we're not going to
	// alter native implementation.

	return m_Allocators[0]->GetLargestAvailableBlock();
}

void rage::sysMemMultiAllocator::SanityCheck()
{
	for (sysMemAllocator* allocator : m_Allocators)
		allocator->SanityCheck();
}

bool rage::sysMemMultiAllocator::IsValidPointer(pVoid block)
{
	for (sysMemAllocator* allocator : m_Allocators)
	{
		if (allocator->IsValidPointer(block))
			return true;
	}
	return false;
}

u64 rage::sysMemMultiAllocator::GetSizeWithOverhead(pVoid block)
{
	for (sysMemAllocator* allocator : m_Allocators)
	{
		if (allocator->IsValidPointer(block))
			return allocator->GetSizeWithOverhead(block);
	}
	return false;
}
