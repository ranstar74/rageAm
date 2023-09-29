//
// File: dictionary.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/crypto/joaat.h"
#include "rage/paging/template/array.h"
#include "rage/paging/resource.h"
#include "rage/paging/base.h"
#include "rage/paging/place.h"
#include "rage/paging/ref.h"

namespace rage
{
	template<typename T>
	struct pgKeyPair
	{
		u32	Key;
		T* Value;
	};

	/**
	 * \brief Dictionary with hash keys.
	 */
	template<typename T>
	class pgDictionary : public pgBase
	{
		static inline pgDictionary* sm_Current = nullptr; // TODO: TLS Hook

		// For pgTextureDictionary - parent from CTxdRelationship
		pgPtr<pgDictionary> m_Parent;

		u32 m_RefCount;
		u32 m_Unused1C = 0;

		pgArray<u32>	m_Keys;
		pgUPtrArray<T>	m_Items;

		void Sort()
		{
			u16 size = m_Keys.GetSize();

			atArray oldItems = std::move(m_Items);
			atArray oldKeys = m_Keys;

			m_Items.Resize(size);

			m_Keys.Sort();
			for (u16 i = 0; i < size; i++)
			{
				u16 newIndex = m_Keys.Find(oldKeys[i]);
				m_Items[newIndex] = std::move(oldItems[i]);
			}
		}
	public:
		pgDictionary(u16 size) : m_Keys(size), m_Items(size)
		{
			m_RefCount = 1;
		}
		pgDictionary()
		{
			m_RefCount = 1;
		}
		pgDictionary(const pgDictionary& other)
			: pgBase(other), m_Keys(other.m_Keys), m_Items(other.m_Items)
		{
			m_RefCount = 1;
		}
		pgDictionary(pgDictionary&& other) noexcept : pgDictionary(0)
		{
			std::swap(m_RefCount, other.m_RefCount);
			std::swap(m_Keys, other.m_Keys);
			std::swap(m_Items, other.m_Items);
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		pgDictionary(const datResource& rsc) : m_Keys(rsc), m_Items(rsc)
		{
			// Nothing to do here, everything is resolved by pgArray
		}

		static void PushDictionary(pgDictionary* dict)
		{
			AM_ASSERT(dict != nullptr, "pgDictionary::PushDictionary() -> Dict was NULL.");
			AM_ASSERT(dict->m_Parent.Get() == nullptr,
				"pgDictionary::PushDictionary() -> Dictionary already has parent and linked somewhere else!");

			dict->m_Parent.Set(sm_Current);
			sm_Current = dict;
		}

		static void PopDictionary()
		{
			AM_ASSERT(sm_Current != nullptr, "pgDictionary::PopDictionary() -> Stack is empty!");

			pgDictionary* oldCurrent = sm_Current->m_Parent.Get();
			sm_Current->m_Parent.Set(nullptr);
			sm_Current = oldCurrent;
		}

		pgPtr<pgDictionary>& GetParent() { return m_Parent; }
		void SetParent(const pgPtr<pgDictionary>& newParent) { m_Parent = newParent; }

		const pgArray<u32>& GetKeys() const { return m_Keys; }
		const pgArray<u32>& GetItems() const { return m_Items; }

		int  GetSize() const { return m_Items.GetSize(); }
		void Clear() { m_Items.Clear(); m_Keys.Clear(); }
		bool ContainsKey(u32 key) const { return m_Keys.Find(key); }
		bool Find(u32 key, pgUPtr<T>** outItem)
		{
			*outItem = nullptr;
			s32 slot = m_Keys.Find(key);
			if (slot == -1) return false;

			*outItem = &m_Items[slot];
			return true;
		}

		pgKeyPair<T> GetAt(u16 index) { return { m_Keys[index], m_Items[index].Get() }; }
		T* GetValueAt(u16 index) { return m_Items[index].Get(); }

		/**
		 * \brief Gets index of item by hash key.
		 * \return Index if item present in dictionary; Otherwise -1;
		 */
		s32 GetIndexOf(u32 key) const
		{
			return m_Keys.Find(key);
		}

		s32 GetIndexOf(ConstString key) const { return GetIndexOf(joaat(key)); }

		template<typename ...Args>
		T* Construct(ConstString key, Args&&... args)
		{
			return Construct(joaat(key), std::forward<Args>(args)...);
		}

		template<typename ...Args>
		T* Construct(u32 key, Args&&... args)
		{
			T* item;

			s32 index = GetIndexOf(key);
			if (index != -1)
			{
				T* ptr = new T(std::forward<Args>(args)...);
				m_Items[index] = pgUPtr<T>(ptr);
				item = m_Items[index].Get();
			}
			else
			{
				m_Keys.Add(key);
				m_Items.Construct(new T(args...));
				item = m_Items.Last().Get();
			}

			// Binary search requires array to be sorted
			// TODO: This can be improved by 'inserting' new item without messing with order
			Sort();

			return item;
		}

		/**
		 * \brief If item with given key is not present in dictionary, item is inserted;
		 * Otherwise existing item is destructed and replaced.
		 * \remarks Item must be allocated on heap and dictionary will take care of it.
		 */
		T* Add(u32 key, T* item)
		{
			T* addedItem;

			s32 index = GetIndexOf(key);
			if (index != -1)
			{
				m_Items[index] = item;
				addedItem = m_Items[index].Get();
			}
			else
			{
				m_Keys.Add(key);
				addedItem = m_Items.Emplace(pgUPtr<T>(item)).Get();
			}

			// We have to sort it now, otherwise Find will be broken.
			// TODO: This can be improved by 'inserting' new item without messing with order
			Sort();

			return addedItem;
		}

		T* Add(ConstString key, T* item)
		{
			return Add(joaat(key), item);
		}

		/**
		 * \brief Gets existing item if exists, otherwise creates new.
		 * \returns Reference to item pointer.
		 * \remarks If key was not present and returned value was not assigned, exception is thrown.
		 */
		[[nodiscard]] T* operator [](u32 key)
		{
			s32 index = GetIndexOf(key);
			if (index != -1) return m_Items[index].Get();
			AM_UNREACHABLE("pgDitionary::operator []() -> Item with key %u is not present dictionary.", key);
		}

		[[nodiscard]] T* operator [](ConstString key)
		{
			return operator[](joaat(key));
		}

		IMPLEMENT_REF_COUNTER(pgDictionary);
	};
}
