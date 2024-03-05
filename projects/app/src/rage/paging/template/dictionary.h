//
// File: dictionary.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/template/array.h"
#include "rage/paging/resource.h"
#include "rage/paging/base.h"
#include "rage/paging/place.h"
#include "rage/paging/ref.h"
#include "rage/atl/hashstring.h"

namespace rage
{
	template<typename T>
	struct pgKeyPair
	{
		u32	Key;
		T* Value;
	};

	/**
	 * \brief Hash set for small sized collections.
	 * \n Note:
	 * Dictionary holds pointers of specified type!
	 * All pointers are destroyed with dictionary.
	 */
	template<typename T>
	class pgDictionary : public pgBase
	{
		static inline pgDictionary* sm_Current = nullptr;

		// For pgTextureDictionary - parent from CTxdRelationship
		pgPtr<pgDictionary> m_Parent;
		u32					m_RefCount;
		pgArray<u32>		m_Keys;
		pgPtrArray<T>		m_Items;

		// Instead of sorting dictionary after adding new item we
		// simply insert it at right position
		u16 FindInsertIndex(u32 key)
		{
			auto it = std::ranges::lower_bound(m_Keys, key);
			return static_cast<u16>(std::distance(m_Keys.begin(), it));
		}

	public:
		pgDictionary() { m_RefCount = 1; }
		pgDictionary(u16 size) : m_Keys(size), m_Items(size) { m_RefCount = 1; }
		pgDictionary(const pgDictionary& other) : pgBase(other), m_Keys(other.m_Keys), m_Items(other.m_Items)
		{
			if (IsResourceCompiling())
				m_RefCount = other.m_RefCount;
			else
				m_RefCount = 1;
		}
		pgDictionary(pgDictionary&& other) noexcept : pgDictionary(0)
		{
			std::swap(m_Parent, other.m_Parent);
			std::swap(m_RefCount, other.m_RefCount);
			std::swap(m_Keys, other.m_Keys);
			std::swap(m_Items, other.m_Items);
		}
		// ReSharper disable once CppPossiblyUninitializedMember
		pgDictionary(const datResource& rsc) : m_Keys(rsc), m_Items(rsc)
		{
			// Nothing to do here, everything is resolved by pgArray
		}
		// Pushes dictionary on top of the stack, see PopDictionary & GetCurrent
		static void PushDictionary(pgDictionary* dict)
		{
			AM_ASSERT(dict != nullptr, "pgDictionary::PushDictionary() -> Dict was NULL.");
			AM_ASSERT(dict->m_Parent.Get() == nullptr,
				"pgDictionary::PushDictionary() -> Dictionary already has parent and linked somewhere else!");

			dict->m_Parent.Set(sm_Current);
			sm_Current = dict;
		}
		// Pops dictionary from top of the stack
		static void PopDictionary()
		{
			AM_ASSERT(sm_Current != nullptr, "pgDictionary::PopDictionary() -> Stack is empty!");

			pgDictionary* oldCurrent = sm_Current->m_Parent.Get();
			sm_Current->m_Parent.Set(nullptr);
			sm_Current = oldCurrent;
		}
		// Gets dictionary from top of the stack, may be NULL
		static pgDictionary* GetCurrent() { return sm_Current; }
		// Gets linked parent dictionary
		const pgPtr<pgDictionary>& GetParent() { return m_Parent; }
		// Parenting is used for linking multiple TXDs together
		void SetParent(const pgPtr<pgDictionary>& newParent) { m_Parent = newParent; }
		// Total number of added items
		u16 GetSize() const { return m_Items.GetSize(); }
		// Gets size of internal array buffers
		u16 GetCapacity() const { return m_Items.GetCapacity(); }
		// Destroys all existing items
		void Clear() { m_Items.Clear(); m_Keys.Clear(); }
		// NOTE: This is not the same as Destroy, Destroy is pgBase function!
		void DestroyDictionary() { m_Items.Destroy(); m_Keys.Destroy(); }
		// If key is in the set
		bool Contains(u32 key) const { return m_Keys.Find(key) != -1; }
		bool Contains(ConstString key) const { return Contains(atStringHash(key)); }
		// Returns NULL if element is not in the set
		T* Find(u32 key)
		{
			int index = m_Keys.Find(key);
			if (index == -1) return nullptr;
			return m_Items[index].Get();
		}
		T* Find(ConstString key) { return Find(atStringHash(key)); }
		// Gets key-value pair at given index (meant to be used with GetSize for iterating dictionary)
		pgKeyPair<T> GetAt(u16 index) { return { m_Keys[index], m_Items[index].Get() }; }
		// Gets just value at index, use with GetSize
		T* GetValueAt(u16 index) { return m_Items[index].Get(); }
		const pgPtr<T>& GetValueRefAt(u16 index) { return m_Items[index]; }
		// Gets index from element hash key
		s32 IndexOf(u32 key) const { return m_Keys.Find(key); }
		s32 IndexOf(ConstString key) const { return IndexOf(atStringHash(key)); }
		// If value is already set for given key, it will be replaced
		// NOTE: Added element pointer ownership goes to dictionary!
		T* Insert(u32 key, T* item)
		{
			int existingIndex = IndexOf(key);
			if (existingIndex != -1)
			{
				m_Items[existingIndex] = pgPtr<T>(item);
				return m_Items[existingIndex].Get();
			}

			u16 index = FindInsertIndex(key);
			m_Keys.Insert(index, key);
			m_Items.EmplaceAt(index, pgPtr<T>(item));
			return m_Items[index].Get();
		}
		T* Insert(ConstString key, T* item) { return Insert(atStringHash(key), item); }
		// Removes the key and value if exists, otherwise does nothing
		void Remove(u32 key)
		{
			int index = IndexOf(key);
			if (index != -1)
			{
				m_Keys.RemoveAt(index);
				m_Items.RemoveAt(index);
			}
		}
		void Remove(ConstString key) { Remove(atStringHash(key)); }
		// Transfers ownership of the value to caller and removes the item from collection
		T* MoveFrom(s32 index)
		{
			AM_ASSERTS(index != -1, index < GetSize());
			m_Keys.RemoveAt(index);
			T* item = m_Items[index].Get();
			m_Items[index].Set(nullptr); // Force set pointer to NULL without deleting
			m_Items.RemoveAt(index);
			return item;
		}
		T* Move(u32 key)
		{
			int index = IndexOf(key);
			return MoveFrom(index);
		}
		T* Move(ConstString key) { return Move(atStringHash(key)); }
		T* MoveIfExists(u32 key)
		{
			int index = IndexOf(key);
			if (index != -1) return MoveFrom(index);
			return nullptr;
		}
		T* MoveIfExists(ConstString key) { return MoveIfExists(atStringHash(key)); }

		IMPLEMENT_REF_COUNTER(pgDictionary);
	};
}
