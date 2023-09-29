//
// File: pool.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/macro.h"

namespace rage
{
	/**
	 * \brief Stores whether slot is free and 'counter'.
	 * \n Counter gets incremented in fwBasePool::New method when slot is set to new value.
	 * This was made to reduce chance of two different allocations obtaining the same GUID.
	 * (fwPoolInfo in combination with slot index produce fwScriptGuid, which is returned from CreateVehicle, CreatePed etc)
	 */
	struct fwPoolInfo
	{
		static constexpr u8 IS_FREE_MASK = 1 << 7;
		static constexpr u8 COUNTER_MASK = (1 << 7) - 1;
		static constexpr u8 COUNTER_MAX_VALUE = 1 << 7;

		// fwPoolInfo::Data bit layout
		// 
		// ┌─┐ ┌────────┐
		// │0│ │000 0000├── GUID Counter (0-127), 7 bits
		// └┬┘ └────────┘
		//  └─ IsFree
		//
		u8 Data;

		bool GetIsFree() const { return Data & IS_FREE_MASK; }
		void SetIsFree(bool toggle)
		{
			Data &= ~IS_FREE_MASK;
			if (toggle) Data |= IS_FREE_MASK;
		}

		u8 GetCounter() const { return Data & COUNTER_MASK; }
		void SetCounter(u8 value)
		{
			Data &= ~COUNTER_MASK;
			Data |= value & COUNTER_MASK;
		}
	};

	/**
	 * \brief A pair of slot index and allocation info from fwBasePool, identical to fwScriptGuid. See rage::fwPoolInfo;
	 */
	struct fwGuid
	{
		static constexpr u32 INDEX_SHIFT = 8;
		static constexpr u32 INFO_MASK = 0xFF;

		// Low 8 bits - info, the rest 24 upper bits  - index.
		u32 InfoAndIndex;

		fwGuid(fwPoolInfo info, s32 index)
		{
			InfoAndIndex = info.Data | index << INDEX_SHIFT;
		}

		s32 GetIndex() const { return (s32)(InfoAndIndex >> INDEX_SHIFT); }
		fwPoolInfo GetInfo() const { return fwPoolInfo(InfoAndIndex & INFO_MASK); }
	};
	// Casts u32 guid to fwGuid
	#define POOL_TO_GUID(guid) (*(rage::fwGuid*)&(guid))

	class fwBasePool
	{
		struct FreeSlot
		{
			s32 NextIndex;
		};

		char* m_Slots = nullptr;
		fwPoolInfo* m_SlotInfos = nullptr;

		u32 m_Size = 0;
		u32 m_ItemSize;

		// # - Allocated
		// @ - Free
		// 
		// In order to maintain constant new/free time in pool, we use linked list.
		// 
		// On allocation we pop head and set it to next free slot, on
		// free we insert given slot in the end of linked list (tail)
		// 
		// This is how pool linked list looks like after initialization:
		//            ┌─┐  ┌─┐  ┌─┐  ┌─┐
		//     Head──►│@├─►│@├─►│@├─►│@│◄──Tail
		//            └─┘  └─┘  └─┘  └─┘
		// After allocating first slot:
		//            ┌─┐  ┌─┐  ┌─┐  ┌─┐
		//     Head   │#│  │@├─►│@├─►│@│◄──Tail
		//        │   └─┘  └▲┘  └─┘  └─┘
		//        └─────────┘
		// As you can see head was popped and became next free slot.
		// Pool state after we allocate 3 more slots:
		//           ┌─┐  ┌─┐  ┌─┐  ┌─┐
		//           │#│  │#│  │#│  │#│
		//           └─┘  └─┘  └─┘  └─┘
		// Well pool is now full (don't let it overflow!), linked list is empty.
		// Let's free slot at index 2:
		//           ┌─┐  ┌─┐  ┌─┐  ┌─┐
		//           │#│  │#│  │@│  │#│
		//           └─┘  └─┘  └▲┘  └─┘
		//                 Head─┴─Tail
		// Linked list now contains only one node, so head is equal to tail.

		s32 m_HeadIndex = -1;
		s32 m_TailIndex = -1;

		static constexpr u32 USED_SLOTS_COUNT_MASK = (1 << 30) - 1;
		static constexpr u32 IS_ALLOCATED_SHIFT = 30;
		static constexpr u32 IS_ALLOCATED_MASK = 1 << IS_ALLOCATED_SHIFT;

		// fwBasePool::m_Data bit layout
		// 
		//  ┌─ Unused  ┌──── Used slot count (30 bits)
		//  │          │
		// ┌┴┐ ┌─┐ ┌───┴───┐ ┌─────────┐ ┌─────────┐ ┌─────────┐
		// │0│ │0│ │00 0000├─┤0000 0000├─┤0000 0000├─┤0000 0000│
		// └─┘ └┬┘ └───────┘ └─────────┘ └─────────┘ └─────────┘
		//      │
		//      └─ IsPoolAllocated
		// 
		u32 m_Data = 0;

		u32 GetOffset(pVoid slot) const { return (u32)((u64)slot - (u64)m_Slots); }
		char* GetSlotPtr(u32 index) const { return m_Slots + (u64)(index * m_ItemSize); }
		FreeSlot* GetFreeSlotPtr(u32 index) const { return reinterpret_cast<FreeSlot*>(GetSlotPtr(index)); }

		u32 GetUsedSlotsCount() const { return m_Data & USED_SLOTS_COUNT_MASK; }
		void SetUsedSlotsCount(u32 count)
		{
			m_Data &= ~USED_SLOTS_COUNT_MASK;
			m_Data |= count & USED_SLOTS_COUNT_MASK;
		}

		bool GetPoolAllocated() const { return m_Data >> IS_ALLOCATED_SHIFT & 0x1; }
		void SetPoolAllocated(bool toggle)
		{
			m_Data &= ~IS_ALLOCATED_MASK;
			if (toggle) m_Data |= IS_ALLOCATED_MASK;
		}

		void AllocStorage();
	public:
		fwBasePool(u32 itemSize);
		~fwBasePool();

		// Allocates pool and resets slots, this function has to be called before pool is used.
		void InitAndAllocate(u32 size);
		void Destroy();

		u32 GetNumUsedSlots() const { return GetUsedSlotsCount(); }
		u32 GetSize() const { return m_Size; }

		// Gets slot pointer if allocated, otherwise null.
		pVoid GetSlotFromJustIndex(s32 index) const;
		pVoid GetSlot(fwGuid id) const { return GetSlotFromJustIndex(id.GetIndex()); }

		s32 GetJustIndex(pVoid slot) const { return (s32)(GetOffset(slot) / m_ItemSize); }
		fwGuid GetIndex(pVoid slot) const; // Actually GetGuid but we're keeping legacy name.

		pVoid NewAt(fwGuid id);
		pVoid New();
		void Delete(pVoid slot);

		bool IsValidPtr(pVoid slot) const;
		void Reset();
	};

	template<typename T>
	class fwPool : public fwBasePool
	{
	public:
		fwPool() : fwBasePool(sizeof T) {}

		T* NewAt(fwGuid id)
		{
			return static_cast<T*>(fwBasePool::NewAt(id));
		}

		T* New()
		{
			return static_cast<T*>(fwBasePool::New());
		}

		T* GetSlot(u32 index)
		{
			return static_cast<T*>(fwBasePool::GetSlotFromJustIndex(index));
		}

		void Delete(T* item)
		{
			fwBasePool::Delete(item);
		}
	};

#define IMPLEMENT_POOL_NEW(pool, itemType)												\
	pVoid operator new(size_t) { return (poolName).New(); }								\
	void operator delete(pVoid item) { return (poolName).Delete((itemType)item); }		\
	MACRO_END
}
