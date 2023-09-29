//
// File: fixedarray.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <algorithm>
#include <functional>

#include "am/system/asserts.h"
#include "common/types.h"

namespace rage
{
	/**
	 * \brief Non-resizable static array.
	 */
	template<typename T, s32 Capacity, typename TSize = s32>
	class atFixedArray
	{
		T		m_Items[Capacity] = {};
		TSize	m_Size = 0;

		void VerifyBufferCanFitNext() const
		{
			AM_ASSERT(m_Size < Capacity, "atFixedArray::VerifyBufferCanFitNext() -> %u/%u - Full", m_Size, Capacity);
		}

		void PrepareForInsert(TSize index)
		{
			AM_ASSERT(index < m_Size, "atFixedArray::Insert(%i) -> Index is out of range.", index);

			VerifyBufferCanFitNext();

			memmove(
				m_Items + index + 1,
				m_Items + index,
				(size_t)(m_Size - index) * sizeof(T));

			++m_Size;
		}
	public:
		TSize GetSize() const { return m_Size; }
		TSize GetCapacity() const { return Capacity; }

		/*
		 *	------------------ Adding / Removing items ------------------
		 */

		 /**
		  * \brief Appends item copy in the end of array.
		  * \remarks
		  * - For complex/large structures or classes use atArray::Emplace with move constructor.
		  * - Also known as push_back
		  */
		T& Add(const T& item)
		{
			VerifyBufferCanFitNext();
			return m_Items[m_Size++] = item;
		}

		/**
		 * \brief Inserts item copy at given index of array, index value must be within array range.
		 * \remarks If index is equal to Size, item is added at the end of array.
		 */
		T& Insert(TSize index, const T& item)
		{
			if (index == m_Size)
				return Add(item);

			PrepareForInsert(index);

			return m_Items[index] = item;
		}

		/**
		 * \brief Constructs a new item in place with given params.
		 * \remarks No copying is done, neither move constructor is required.
		 */
		template<typename ...Args>
		T& Construct(Args... args)
		{
			VerifyBufferCanFitNext();

			pVoid where = (pVoid)(m_Items + m_Size);
			++m_Size;
			return *new (where) T(args...);
		}

		/**
		 * \brief Constructs and inserts item in place at given index of array, index value must be within array range.
		 * \remarks If index is equal to Size, item is added at the end of array.
		 */
		template<typename ...Args>
		T& ConstructAndInsert(u32 index, Args... args)
		{
			if (index == m_Size)
				return Construct(args...);

			PrepareForInsert(index);

			pVoid where = (pVoid)(m_Items + index);
			return *new (where) T(args...);
		}

		/**
		 * \brief Inserts given item at the end of array using move semantics.
		 * \param item Item to move, use std::move.
		 */
		T& Emplace(T&& item)
		{
			VerifyBufferCanFitNext();

			pVoid where = m_Items + m_Size;
			++m_Size;

			T* slot = new (where) T();
			*slot = std::move(item);
			return *slot;
		}

		/**
		 * \brief Inserts given item at given index using move semantics (std::move);
		 */
		T& EmplaceInsert(TSize index, T&& item)
		{
			VerifyBufferCanFitNext();

			PrepareForInsert(index);

			pVoid where = m_Items + index;
			T* slot = new (where) T();
			*slot = std::move(item);
			return *slot;
		}

		/**
		 * \brief Removes element from the beginning of the array (first added).
		 * \remarks Also known as remove_front
		 */
		void RemoveFirst()
		{
			AM_ASSERT(m_Size != 0, "atArray::RemoveFirst() -> Array is empty!");
			RemoveAt(0);
		}

		/**
		 * \brief Removes element from the end of the array (last added).
		 * \remarks Also known as remove_back
		 */
		void RemoveLast()
		{
			AM_ASSERT(m_Size != 0, "atArray::RemoveEnd() -> Array is empty!");
			m_Items[--m_Size].~T();
		}

		/**
		 * \brief Removes item at given index. Other elements will be shifted to fill the gap.
		 */
		void RemoveAt(s32 index)
		{
			m_Items[index].~T();

			memmove(m_Items + index, m_Items + index + 1,
				(size_t)(m_Size - index - 1) * sizeof(T));
			--m_Size;
		}

		/**
		 * \brief Performs linear search to find given item and destructs first found slot.
		 */
		void Remove(const T& item)
		{
			for (TSize i = 0; i < m_Size; ++i)
			{
				if (m_Items[i] != item)
					continue;

				RemoveAt(i);
				return;
			}
		}

		/**
		 * \brief Resets array size, destructing existing items.
		 */
		void Clear()
		{
			for (TSize i = 0; i < m_Size; i++)
				m_Items[i].~T();
			m_Size = 0;
		}

		/**
		 * \brief Sorts array in ascending order using default predicate.
		 */
		void Sort()
		{
			std::sort(m_Items, m_Items + m_Size);
		}

		/**
		 * \brief Sorts array using given predicate and std::sort;
		 */
		template<typename TPredicate>
		void Sort(TPredicate predicate)
		{
			std::sort(m_Items, m_Items + m_Size, predicate);
		}

		/*
		 *	Altering array size
		 */

		 /**
		  * \brief Sets accessible array range.
		  * This is slightly misleading because array is not being actually resized (in terms of internal capacity)
		  * but only items are constructed / destructed.
		  *
		  * \n Example of use:
		  * \n atFixedArray<int, 50> values;
		  * \n values[5] = 0; // Index of out range
		  * \n This happens because we maintain internal size counter and item at index 5 was not constructed yet.
		  * \n values.Resize(50);
		  * \n values[5] = 0; // Works as intended
		  */
		void Resize(TSize newSize)
		{
			if (m_Size == newSize)
				return;

			if (newSize < m_Size)
			{
				for (TSize i = m_Size; i > newSize; --i)
					m_Items[i - 1].~T();

				m_Size = newSize;
				return;
			}
			m_Size = newSize;

			// We must ensure that accessible items are constructed
			for (TSize i = 0; i < m_Size; ++i)
			{
				pVoid where = (pVoid)(m_Items + i);
				new (where) T();
			}
		}

		/*
		 *	------------------ Getters / Operators ------------------
		 */

		/**
		 * \brief Gets last element in the array.
		 */
		T& Last()
		{
			AM_ASSERT(m_Size != 0, "atFixedArray::Last() -> No items in array.");
			return m_Items[m_Size - 1];
		}

		/**
		 * \brief Gets first element in the array.
		 */
		T& First()
		{
			AM_ASSERT(m_Size != 0, "atFixedArray::First() -> No items in array.");
			return m_Items[0];
		}

		 /**
		  * \brief Gets const item reference at given index.
		  */
		const T& Get(s32 index) const
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atFixedArray::Get(%u) -> Index was out of range.", index);

			return m_Items[index];
		}

		/**
		 * \brief Gets item reference at given index.
		 */
		T& Get(s32 index)
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atFixedArray::Get(%u) -> Index was out of range.", index);

			return m_Items[index];
		}

		bool Any() const { return m_Size > 0; }

		T* begin() { return m_Items; }
		T* end() { return m_Items + m_Size; }
		const T* begin() const { return m_Items; }
		const T* end() const { return m_Items + m_Size; }

		T& operator [](TSize index) { return Get(index); }
		const T& operator [] (TSize index) const { return Get(index); }
	};
}
