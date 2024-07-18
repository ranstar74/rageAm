//
// File: array.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "am/system/asserts.h"
#include "rage/system/new.h"
#include "helpers/align.h"
#include "rage/paging/resource.h"
#include "rage/paging/compiler/compilerhelper.h"

#include <algorithm>
#include <functional>
#include <new>
#include <vector>

namespace rage
{
	struct datResource;

	template<typename T, typename TSize>
	struct atArrayAllocate
	{
		// This exists because pgArray handles allocation differently during resource compiling, for more details see paging/pgArray.h

		static void Allocate(T** ppItems, TSize capacity)
		{
			// By default, C++ invoke constructor and destructor on new/delete for every item
			// of the array, we pre-allocate memory without constructing items, so we have
			// to delete them without destructing too, casting to void* does the job
			*ppItems = static_cast<T*>(operator new[](capacity * sizeof T));
		}

		static void Free(T* pItems)
		{
			// Just like in Allocate, we have to cast pointer to void* to prevent destructor call
			operator delete[](static_cast<pVoid>(pItems));
		}
	};

	/**
	 * \brief Array with dynamic memory allocation.
	 */
	template<typename T, typename TSize = u16, typename TAllocate = atArrayAllocate<T, TSize>>
	class atArray
	{
	protected:
		T* m_Items;
		TSize m_Size;
		TSize m_Capacity;

		// If current buffer size is smaller than given, allocates larger one.
		void VerifyBufferCanFitOrGrow(TSize size)
		{
			if (size <= m_Capacity)
				return;

			// We can't allocate size smaller than 16
			if (size < 16)
			{
				size = 16;
			}
			else
			{
				// In native implementation it's defined by allocation policy,
				// but we're not launching rockets here...
				// Growing twice in size will work fine in most cases.
				size = NEXT_POWER_OF_TWO_32(size);
			}

			Reserve(size);
		}
	public:
		atArray(TSize size)
		{
			m_Items = nullptr;
			m_Capacity = 0;
			m_Size = 0;

			Resize(size);
		}
		atArray()
		{
			// Deserializing resource, we shouldn't touch anything and let pgArray fixup things
			if (datResource::GetCurrent())
				return;

			m_Items = nullptr;
			m_Capacity = 0;
			m_Size = 0;
		}
		atArray(const atArray& other)
		{
			m_Capacity = 0;
			m_Size = 0;
			m_Items = nullptr;

			if (other.m_Size == 0)
				return;

			Reserve(other.m_Size);
			m_Size = other.m_Size;

			// Copy all items
			for (u32 i = 0; i < m_Size; i++)
			{
				// Special case for resource compiler:
				// We can't copy some structs (such as pgPtr / pgUPtr) for various reasons:
				// - It will mess up reference counter
				// - Unique pointer can't be even copied
				// As solution those classes must implement 'Snapshot' method
				constexpr bool canSnapshot = requires(T t) { t.Snapshot(t); };
				constexpr bool canCopy = requires(T lhs, T rhs) { lhs = rhs; };
				if constexpr (canSnapshot)
				{
					if (IsResourceCompiling())
					{
						m_Items[i].Snapshot(other.m_Items[i]);
						continue;
					}
					// Otherwise we do regular copy
				}

				if constexpr (canCopy)
				{
					new (&m_Items[i]) T(other.m_Items[i]);
				}
				else
				{
					// This is a terrible way to handle it but otherwise code won't just compile
					// for pgUPtr that can't be copied but can be snapshot.
					// I'm open for better ideas how to handle this.
					AM_UNREACHABLE("atArray::Copy() -> Type %s can't be copied.", typeid(T).name());
				}
			}
		}
		atArray(atArray&& other) noexcept : atArray(0)
		{
			std::swap(m_Items, other.m_Items);
			std::swap(m_Size, other.m_Size);
			std::swap(m_Capacity, other.m_Capacity);
		}
		atArray(std::vector<T> vec) : atArray(vec.begin(), vec.end()) {}
		template<typename TIterator>
		atArray(TIterator start, TIterator end) : atArray(0)
		{
			TSize size = end - start;
			VerifyBufferCanFitOrGrow(size);

			for (TSize i = 0; start < end; ++start, ++i)
				m_Items[i] = *start;
			m_Size = size;
		}
		atArray(std::initializer_list<T> list) : atArray(list.begin(), list.end()) {}

		// ReSharper disable once CppPossiblyUninitializedMember
		atArray(const datResource& rsc)
		{
			// Moved to pgArray, the only purpose of this constructor
			// is to prevent members from overwriting with default values
		}

		~atArray()
		{
			Destroy();
		}

		// Hack used to initialize temp array duruing resource building
		void InitFromResourceConstructor()
		{
			m_Items = nullptr;
			m_Capacity = 0;
			m_Size = 0;
		}

		/*
		 *	------------------ Initializers / Destructors ------------------
		 */

		 // WARNING: To use only when it is required to get internal array buffer without copying!
		 // Buffer must be deleted manually or memory will be leaked!
		T* MoveBuffer()
		{
			T* buffer = m_Items;

			m_Items = nullptr;
			m_Capacity = 0;
			m_Size = 0;

			return buffer;
		}

		T*& GetBufferRef()
		{
			return m_Items;
		}

		void Destroy()
		{
			if (!m_Items)
				return;

			Clear();

			TAllocate::Free(m_Items);

			m_Items = nullptr;
			m_Capacity = 0;
		}

		void SetItems(T* items, TSize size, TSize capacity)
		{
			Destroy();
			m_Items = items;
			m_Size = size;
			m_Capacity = capacity;
		}

		/*
		 *	------------------ Adding / Removing items ------------------
		 */

		void AddRange(const atArray& other)
		{
			Reserve(m_Size + other.m_Size);
			for (const T& element : other)
				Add(element);
		}

		void AddRange(atArray&& other)
		{
			Reserve(m_Size + other.m_Size);
			for (u16 i = 0; i < other.m_Size; i++)
				Emplace(std::move(other.m_Items[i]));
		}

		/**
		 * \brief Appends item copy in the end of array.
		 * \remarks
		 * - For complex/large structures or classes use atArray::Emplace with move constructor.
		 * - Also known as push_back
		 */
		T& Add(const T& item)
		{
			VerifyBufferCanFitOrGrow(m_Size + 1);

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

			AM_ASSERT(index < m_Size, "atArray::Insert(%i) -> Index is out of range.", index);

			VerifyBufferCanFitOrGrow(m_Size + 1);

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

			AM_ASSERT(index < m_Size, "atArray::EmplaceAt(%i) -> Index is out of range.", index);

			VerifyBufferCanFitOrGrow(m_Size + 1);

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
			VerifyBufferCanFitOrGrow(m_Size + 1);

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
			VerifyBufferCanFitOrGrow(m_Size + 1);
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
			RemoveAt(m_Size - 1);
		}

		/**
		 * \brief Removes item at given index. Other elements will be shifted to fill the gap.
		 */
		void RemoveAt(TSize index)
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atArray::Remove(%u) -> Index was out of range.", index);

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

			if (newSize < m_Size)
			{
				for (TSize i = m_Size; i > newSize; --i)
					m_Items[i - 1].~T();

				m_Size = newSize;
				return;
			}

			Reserve(newSize);

			// We must ensure that accessible items are constructed
			for (TSize i = m_Size; i < newSize; ++i)
			{
				pVoid where = (pVoid)(m_Items + i);
				new (where) T();
			}
			m_Size = newSize;
		}

		/**
		 * \brief Resizes array buffer without re-allocation to actually used size.
		 * \remarks Works only for size greater than 128, because sizes <= 128 are handled by rage::sysSmallocator, which does not support resizing.
		 */
		void Shrink()
		{
			if (m_Capacity <= 128)
				return;

			GetMultiAllocator()->Resize(m_Items, m_Size);
		}

		/**
		 * \brief Reserves memory in array buffer without constructing items (see Resize).
		 * \remarks If new capacity is smaller or equal to current one, nothing is done.
		 * \n By default function aligns given capacity to multiple of 16, using forceExactSize
		 * it can be forced to exact given value without aligning applied
		 */
		void Reserve(TSize capacity, bool forceExactSize = false)
		{
			if (!forceExactSize)
			{
				capacity = ALIGN_16(capacity);
				if (capacity <= m_Capacity)
					return;
			}

			m_Capacity = capacity;

			// Allocate new buffer and simply copy all items to it
			T* oldItems = m_Items;
			TAllocate::Allocate(&m_Items, m_Capacity);
			if (!oldItems)
				return;

			u32 copySize = m_Size * sizeof(T);
			memcpy_s(m_Items, copySize, oldItems, copySize);

			TAllocate::Free(oldItems);
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
		T& Last() { return m_Items[m_Size - 1]; }

		/**
		 * \brief Gets first element in the array.
		 */
		T& First() { return m_Items[0]; }

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
			AM_ASSERT(index >= 0 && index < m_Size, "atArray::Get(%u) -> Index was out of range.", index);

			return m_Items[index];
		}

		/**
		 * \brief Gets const item reference at given index.
		 */
		const T& Get(s32 index) const
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atArray::Get(%u) -> Index was out of range.", index);

			return m_Items[index];
		}

		/**
		 * \brief Sets value at given index.
		 */
		T& Set(s32 index, const T& value) const
		{
			AM_ASSERT(index >= 0 && index < m_Size, "atArray::Set(%u) -> Index was out of range.", index);

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
				"atArray::IndexFromPtr() -> Item at %p is not in range of internal buffer %p to %p. "
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
		TSize GetCapacity() const { return m_Capacity; }

		/**
		 * \brief Compares two arrays by value and items order.
		 * \returns True if all items are equal value and their order wise, otherwise false.
		 */
		bool operator==(const atArray& other) const
		{
			if (m_Size != other.m_Size)
				return false;

			for (TSize i = 0; i < m_Size; ++i)
			{
				if (m_Items[i] != other.m_Items[i])
					return false;
			}
			return true;
		}

		atArray& operator=(const atArray& other) // NOLINT(bugprone-unhandled-self-assignment)
		{
			Clear();
			Reserve(other.m_Capacity);
			memcpy(m_Items, other.m_Items, other.m_Size * sizeof(T));
			m_Size = other.m_Size;

			return *this;
		}

		atArray& operator=(atArray&& other) noexcept
		{
			TAllocate::Free(m_Items);
			m_Items = nullptr;

			m_Items = other.m_Items;
			m_Size = other.m_Size;
			m_Capacity = other.m_Capacity;

			other.m_Items = nullptr;

			return *this;
		}

		T* begin() const { return m_Items; }
		T* end() const { return m_Items + m_Size; }

		T& operator [](TSize index) { return Get(index); }
		const T& operator [] (TSize index) const { return Get(index); }
	};
}
