//
// File: array.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <new.h>

#include "rage/atl/array.h"
#include "rage/paging/compiler/compiler.h"
#include "rage/paging/ref.h"
#include "rage/paging/resource.h"

namespace rage
{
	template<typename T, typename TSize>
	struct pgArrayAllocate
	{
		static void Allocate(T** ppItems, TSize capacity)
		{
			pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
			if (!pAllocator) // We're doing regular allocation, resource compiler isn't active
			{
				atArrayAllocate<T, TSize>::Allocate(ppItems, capacity);
				return;
			}

			// Resource is being compiled...
			// Note that we used size here instead of capacity,
			// we don't want extra unused memory to be in resource.
			pAllocator->AllocateRefArray(*ppItems, capacity);
		}

		static void Free(T* pItems)
		{
			pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
			if (!pAllocator)
				atArrayAllocate<T, TSize>::Free(pItems);

			// We don't need to free memory in datAllocator because it will be destroyed after compilation is done
		}
	};

	// Re-allocates and moves all array items to snapshot allocator (if resource is being compiled).
	template<typename T>
	void pgSnapshotArray(T* items, u16 size)
	{
		if constexpr (pgCanDeserialize<T>)
		{
			pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
			if (!pAllocator)
				return;

			// atArray will copy items but won't snapshot them, we have to do it for every item manually
			for (int i = 0; i < size; i++)
			{
				auto& item = items[i];

				// As usual, allocate memory for resource struct on virtual allocator
				// and copy structure memory via placement new and copy constructor
				pVoid where = pAllocator->Allocate(sizeof(T));
				item = new (where) T(std::move(item));

				pAllocator->AddRef(item);
			}
		}
	}

	// Fixes up items pointer and places all array items.
	template<typename T>
	void pgPlaceArray(const datResource& rsc, T*& items, u16 size, bool placeItems)
	{
		rsc.Fixup(items);

		// Raw pointers must be used for testing only!
		if constexpr (std::is_pointer<T>())
		{
			for (u16 i = 0; i < size; i++)
				rsc.Fixup(items[i]);
		}

		if (!placeItems)
			return;

		if constexpr (pgHasRscConstructor<T>)
		{
			for (u16 i = 0; i < size; i++)
			{
				// We don't have to fixup structs, only place (construct)
				// Special case is pgPtr / pgUPtr and they have fix-up inside them, as it should be
				T* pItem = &items[i];
				rsc.Place(pItem);
			}
		}
	}

	/**
	 * \brief Array that supports paged serialization.
	 */
	template<typename T, bool bPlaceItems = true>
	class pgArray : public atArray<T, u16, pgArrayAllocate<T, u16>>
	{
		using Array = atArray<T, u16, pgArrayAllocate<T, u16>>;
	public:
		using Array::atArray;

		pgArray()
		{
			// Perform placement if we're building resource
			datResource* rsc = datResource::GetCurrent();
			if (rsc)
				pgPlaceArray(*rsc, Array::m_Items, Array::m_Size, bPlaceItems);
		}

		pgArray(const pgArray& other) : Array(other)
		{
			pgSnapshotArray(Array::m_Items, Array::m_Size);

			if (pgRscCompiler::GetCurrent())
			{
				// NOTE: We clamp capacity to size because tools like CodeWalker use it instead of
				// size and crash with null reference exception
				Array::m_Capacity = Array::m_Size;
			}
		}

		pgArray(const datResource& rsc) : Array(rsc)
		{
			pgPlaceArray(rsc, Array::m_Items, Array::m_Size, bPlaceItems);
		}
	};

	// Array of shared pointers.
	template<typename T, bool bPlaceItems = true>
	using pgPtrArray = pgArray<pgPtr<T>, bPlaceItems>;
	// Array of unique pointers.
	template<typename T, bool bPlaceItems = true>
	using pgUPtrArray = pgArray<pgUPtr<T>, bPlaceItems>;
}
