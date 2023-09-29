#include "pool.h"
#include "am/system/asserts.h"

void rage::fwBasePool::AllocStorage()
{
	u64 slotsSize = static_cast<u64>(m_ItemSize) * m_Size;

	m_Slots = new char[slotsSize];
	m_SlotInfos = new fwPoolInfo[m_Size];

	Reset();
	SetPoolAllocated(true);
}

rage::fwBasePool::fwBasePool(u32 itemSize)
{
	m_ItemSize = itemSize;
}

rage::fwBasePool::~fwBasePool()
{
	Destroy();
}

void rage::fwBasePool::InitAndAllocate(u32 size)
{
	m_Size = size;
	AllocStorage();
}

void rage::fwBasePool::Destroy()
{
	delete m_Slots;
	delete m_SlotInfos;

	m_Slots = nullptr;
	m_SlotInfos = nullptr;
	m_Size = 0;

	SetPoolAllocated(false);
	SetUsedSlotsCount(0);
}

pVoid rage::fwBasePool::GetSlotFromJustIndex(s32 index) const
{
	fwPoolInfo& info = m_SlotInfos[index];
	if (info.GetIsFree())
		return nullptr;

	return GetSlotPtr(index);
}

rage::fwGuid rage::fwBasePool::GetIndex(pVoid slot) const
{
	s32 index = GetJustIndex(slot);
	fwGuid id(m_SlotInfos[index], index);
	return id;
}

pVoid rage::fwBasePool::NewAt(fwGuid id)
{
	s32 index = id.GetIndex();

	fwPoolInfo& slotInfo = m_SlotInfos[index];
	AM_ASSERT(slotInfo.GetIsFree(), "fwBasePool::NewAt(idx: %i) -> Slot is allocated!", id.GetIndex());
	slotInfo.SetIsFree(false);
	slotInfo.SetCounter(id.GetInfo().GetCounter());

	FreeSlot* freeSlot = GetFreeSlotPtr(index);

	// We have to remove node from free linked list
	FreeSlot* prevSlot = GetFreeSlotPtr(m_HeadIndex);
	while (true)
	{
		// Find slot that points to current index
		if (prevSlot->NextIndex == index)
		{
			prevSlot->NextIndex = freeSlot->NextIndex;
			// In case if we remove tail slot, we have to set tail to previous one
			if (m_TailIndex == index)
				m_TailIndex = GetJustIndex(prevSlot);

			break;
		}

		AM_ASSERT(prevSlot->NextIndex != -1,
			"fwBasePool::NewAt(idx: %i) -> Free node is not in free list, state is corruped.", id.GetIndex());
		prevSlot = GetFreeSlotPtr(prevSlot->NextIndex);
	}

	SetUsedSlotsCount(GetUsedSlotsCount() + 1);

	return freeSlot;
}

pVoid rage::fwBasePool::New()
{
	s32 index = m_HeadIndex;
	AM_ASSERT(index != -1, "fwBasePool::New() -> Pool is full! Size: %u", m_Size);

	FreeSlot* freeSlot = GetFreeSlotPtr(index);

	// Head is now next free slot
	m_HeadIndex = freeSlot->NextIndex;

	// If linked list is empty, set tail to null too
	if (m_HeadIndex == -1)
		m_TailIndex = -1;

	// Mark as allocated
	fwPoolInfo& slotInfo = m_SlotInfos[index];
	slotInfo.SetIsFree(false);

	// See fwPoolInfo comment for explanation on GUID
	// GUID Counter has to be only incremented in New! NewAt sets counter from fwGuid
	u8 newCounter = slotInfo.GetCounter() + 1;
	if (newCounter >= fwPoolInfo::COUNTER_MAX_VALUE) // Counter stored in 7 bits, reset to 1 when overflowing
		newCounter = 1;
	slotInfo.SetCounter(newCounter);

	SetUsedSlotsCount(GetUsedSlotsCount() + 1);

	return freeSlot;
}

void rage::fwBasePool::Delete(pVoid slot)
{
	s32 index = GetJustIndex(slot);

	FreeSlot* freeSlot = GetFreeSlotPtr(index);
	freeSlot->NextIndex = -1; // We insert it in the end of linked linked list, end points to nothing
	if (m_HeadIndex == -1)
		m_HeadIndex = index; // Linked list was empty, set given slot as head

	// Insert slot in the end of linked list
	if (m_TailIndex != -1)
		GetFreeSlotPtr(m_TailIndex)->NextIndex = index;
	m_TailIndex = index;

	fwPoolInfo& info = m_SlotInfos[index];
	info.SetIsFree(true);

	SetUsedSlotsCount(GetUsedSlotsCount() - 1);
}

bool rage::fwBasePool::IsValidPtr(pVoid slot) const
{
	if (slot < m_Slots)
		return false;

	// Pointer is out of range
	pVoid maxSlot = GetSlotPtr(m_Size - 1);
	if (slot > maxSlot)
		return false;

	// Item offset must be multiple of item size
	u32 offset = GetOffset(slot);
	if (offset % m_ItemSize != 0)
		return false;

	// Slot has to be allocated
	u32 index = offset / m_ItemSize;
	if (m_SlotInfos[index].GetIsFree())
		return false;

	return true;
}

void rage::fwBasePool::Reset()
{
	s32 size = static_cast<s32>(m_Size);

	m_HeadIndex = 0;
	m_TailIndex = size - 1;

	// Build linked list
	for (s32 i = 0; i < size; i++)
	{
		m_SlotInfos[i].SetIsFree(true);
		m_SlotInfos[i].SetCounter(1);

		bool isEnd = i + 1 == size;

		FreeSlot* freeSlot = GetFreeSlotPtr(i);
		freeSlot->NextIndex = isEnd ? -1 : i + 1;
	}
}
