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
	template<typename T, u32 Capacity, typename TSize = int>
	class atFixedArray
	{
		// Use union to prevent destructor calls on elements
		union
		{
			T		m_Items[Capacity] = {};
			char	m_Dummy;
		};
		TSize		m_Size;

		void VerifyBufferCanFitNext() const
		{
			AM_ASSERT(m_Size < Capacity, "atFixedArray::VerifyBufferCanFitNext() -> %u/%u - Full", m_Size, Capacity);
		}

	public:
		atFixedArray() = default;
		atFixedArray(TSize size)
		{
			m_Size = 0;
			Resize(size);
		}
		~atFixedArray()
		{
			Resize(0);
		}

		/*
		 *	------------------ Adding / Removing items ------------------
		 */

		void AddRange(const atFixedArray& other)
		{
			Reserve(m_Size + other.m_Size);
			for (const T& element : other)
				Add(element);
		}

		void AddRange(atFixedArray&& other)
		{
			Reserve(m_Size + other.m_Size);
			for (u16 i = 0; i < other.m_Size; i++)
				Emplace(std::move(other.m_Items[i]));
		}

		/**
		 * \brief Appends item copy in the end of array.
		 * \remarks
		 * - For complex/large structures or classes use atFixedArray::Emplace with move constructor.
		 * - Also known as push_back
		 */
		T& Add(const T& item)
		{
			VerifyBufferCanFitNext();

			pVoid where = m_Items + m_Size;
			++m_Size;
			return *new (where) T(item); // Copy-place
		}

		/**
		 * \brief Inserts item copy at given index of array, index value must be within array range.
		 * \remarks If index is equal to Size, item is added at the end of array.
		 */
		T& Insert(TSize index, const T& item)
		{
			if (index == m_Size)
				return Add(item);

			AM_ASSERT(index < m_Size, "atFixedArray::Insert(%i) -> Index is out of range.", index);

			VerifyBufferCanFitNext();

			// Shift items on indices >= requested
			memmove(
				m_Items + index + 1,
				m_Items + index,
				(size_t)(m_Size - index) * sizeof(T));

			++m_Size;

			pVoid where = m_Items + index;
			return *new(where) T(item); // Copy-place
		}

		/**
		 * \brief Inserts item at given index of array, index value must be within array range.
		 * \remarks If index is equal to Size, item is added at the end of array.
		 */
		T& EmplaceAt(TSize index, T&& item)
		{
			if (index == m_Size)
				return Emplace(std::move(item));

			AM_ASSERT(index < m_Size, "atFixedArray::Emplace(%i) -> Index is out of range.", index);

			VerifyBufferCanFitNext();

			// Shift items on indices >= requested
			memmove(
				m_Items + index + 1,
				m_Items + index,
				(size_t)(m_Size - index) * sizeof(T));

			++m_Size;

			pVoid where = m_Items + index;

			// Move-place
			T* slot = new (where) T();
			*slot = std::move(item);
			return *slot;
		}

		/**
		 * \brief Moves item in the end of array.
		 * \param item Item to move, use std::move.
		 */
		T& Emplace(T&& item)
		{
			VerifyBufferCanFitNext();

			pVoid where = m_Items + m_Size;
			++m_Size;

			// Move-place
			T* slot = new (where) T();
			*slot = std::move(item);
			return *slot;
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
			return *new (where) T(args...); // Construct in-place
		}

		/**
		 * \brief Removes element from the beginning of the array (first added).
		 * \remarks Also known as remove_front
		 */
		void RemoveFirst()
		{
			AM_ASSERT(m_Size != 0, "atFixedArray::RemoveFirst() -> Array is empty!");
			RemoveAt(0);
		}

		/**
		 * \brief Removes element from the end of the array (last added).
		 * \remarks Also known as remove_back
		 */
		void RemoveLast()
		{
			AM_ASSERT(m_Size != 0, "atFixedArray::RemoveEnd() -> Array is empty!");
			RemoveAt(m_Size - 1);
		}

		/**
		 * \brief Removes item at given index. Other elements will be shifted to fill the gap.
		 */
		void RemoveAt(TSize index)
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atFixedArray::Remove(%u) -> Index was out of range.", index);

			m_Items[index].~T();

			// Is there anything to shift?
			if (index + 1 < m_Size)
			{
				T* dst = m_Items + index;
				T* src = dst + 1;
				u32 size = (m_Size - (index + 1)) * sizeof T;

				memmove(dst, src, size);
			}

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
			// Destruct all items
			for (TSize i = 0; i < m_Size; ++i)
				m_Items[i].~T();

			m_Size = 0;
		}

		/*
		 *	------------------ Altering array size ------------------
		 */

		 /**
		  * \brief Resizes internal array buffer capacity to given size.
		  */
		void Resize(TSize newSize)
		{
			if (m_Size == newSize)
				return;

			AM_ASSERT(newSize <= Capacity, 
				"atFixedArray::Resize() -> Requested size %u is larger than capacity %u.", newSize, Capacity);

			for (TSize i = m_Size; i > newSize; --i)
				m_Items[i - 1].~T();

			m_Size = newSize;
		}

		/*
		 *	------------------ Getters / Operators ------------------
		 */

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
		void Sort(std::function<bool(const T& lhs, const T& rhs)> predicate)
		{
			std::sort(m_Items, m_Items + m_Size, predicate);
		}

		/**
		 * \brief Gets last element in the array.
		 */
		T& Last() { AM_ASSERTS(m_Size != 0); return m_Items[m_Size - 1]; }

		/**
		 * \brief Gets first element in the array.
		 */
		T& First() { AM_ASSERTS(m_Size != 0); return m_Items[0]; }

		/**
		 * \brief Gets pointer to underlying buffer array, with size of GetCapacity() and usable range of GetSize().
		 */
		const T* GetItems() const { return m_Items; }

		/**
		 * \brief Gets const pointer to underlying buffer array, with size of GetCapacity() and usable range of GetSize().
		 */
		T* GetItems() { return m_Items; }

		/**
		 * \brief Gets item reference at given index.
		 */
		T& Get(s32 index)
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atFixedArray::Get(%u) -> Index was out of range.", index);

			return m_Items[index];
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
		 * \brief Sets value at given index.
		 */
		T& Set(s32 index, const T& value) const
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atFixedArray::Set(%u) -> Index was out of range.", index);

			return m_Items[index] = value;
		}

		/**
		 * \brief Returns index of pointer, -1 if pointer is nullptr or out of range.
		 */
		s32 IndexOf(T& item) const
		{
			for (TSize i = 0; i < m_Size; ++i)
			{
				if (m_Items[i] == item)
					return i;
			}
			return -1;
		}

		/**
		 * \brief Returns index of iterator.
		 */
		s32 IndexFromPtr(T* pItem) const
		{
			if (pItem == nullptr)
				return -1;

			AM_ASSERT(pItem > begin() && pItem < end(),
				"atFixedArray::IndexFromPtr() -> Item at %p is not in range of internal buffer %p to %p. "
				"Most likely array was resized and pointer became invalid.",
				pItem, begin(), end());

			return (pItem - m_Items) / sizeof T;
		}

		/**
		 * \brief Performs binary search to find given value index.
		 * \remarks Array has to be sorted in ascending order.
		 */
		s32 Find(const T& value) const
		{
			auto it = std::lower_bound(begin(), end(), value);
			if (it == end() || *it != value)
			{
				return -1;
			}
			return std::distance(begin(), it);
		}

		/**
		 * \brief Gets whether array contains given item, type must have comparison operator implemented.
		 */
		bool Contains(const T& item) const
		{
			for (TSize i = 0; i < m_Size; ++i)
			{
				if (m_Items[i] == item)
					return true;
			}
			return false;
		}

		/**
		 * \brief Gets whether array has at least one item.
		 */
		bool Any() const { return m_Size != 0; }

		TSize GetSize() const { return m_Size; }
		TSize GetCapacity() const { return Capacity; }

		T* begin() { return m_Items; }
		T* end() { return m_Items + m_Size; }
		const T* begin() const { return m_Items; }
		const T* end() const { return m_Items + m_Size; }

		T& operator [](TSize index) { return Get(index); }
		const T& operator [] (TSize index) const { return Get(index); }
	};
}
