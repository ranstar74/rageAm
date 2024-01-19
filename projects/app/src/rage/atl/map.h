//
// File: map.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/compiler/compiler.h"
#include "hashstring.h"
#include "string.h"

namespace rage
{
	// See https://cs.stackexchange.com/questions/11029/why-is-it-best-to-use-a-prime-number-as-a-mod-in-a-hashing-function

	// Gets next prime number for given size if size is not prime number already.
	inline u16 atHashSize(u16 size)
	{
		if (size > 65167) return 65521;
		if (size > 38351) return 65167;
		if (size > 22571) return 38351;
		if (size > 13297) return 22571;
		if (size > 7841) return 13297;
		if (size > 4621) return 7841;
		if (size > 2729) return 4621;
		if (size > 1609) return 2729;
		if (size > 953) return 1609;
		if (size > 563) return 953;
		if (size > 331) return 563;
		if (size > 191) return 331;
		if (size > 107) return 191;
		if (size > 59) return 107;
		if (size > 29) return 59;
		if (size > 11) return 29;
		return 11;
	}

	// Gets next prime number for given size.
	inline u16 atHashNextSize(u16 size)
	{
		if (size >= 65167) return 65521;
		if (size >= 38351) return 65167;
		if (size >= 22571) return 38351;
		if (size >= 13297) return 22571;
		if (size >= 7841) return 13297;
		if (size >= 4621) return 7841;
		if (size >= 2729) return 4621;
		if (size >= 1609) return 2729;
		if (size >= 953) return 1609;
		if (size >= 563) return 953;
		if (size >= 331) return 563;
		if (size >= 191) return 331;
		if (size >= 107) return 191;
		if (size >= 59) return 107;
		if (size >= 29) return 59;
		if (size >= 11) return 29;
		return 11;
	}

	template<typename T>
	struct atMapHashFn
	{
		u32 operator()(const T&) = delete;
	};

	// For default integer types we just use initial value, 64 bit values we wrap into 32 bits (though it's not good way to do it)

	template<> inline u32 atMapHashFn<u8>::operator()(const u8& value) { return value; }
	template<> inline u32 atMapHashFn<s8>::operator()(const s8& value) { return static_cast<u32>(value); }
	template<> inline u32 atMapHashFn<u16>::operator()(const u16& value) { return value; }
	template<> inline u32 atMapHashFn<s16>::operator()(const s16& value) { return static_cast<u32>(value); }
	template<> inline u32 atMapHashFn<u32>::operator()(const u32& value) { return value; }
	template<> inline u32 atMapHashFn<s32>::operator()(const s32& value) { return static_cast<u32>(value); }
	template<> inline u32 atMapHashFn<u64>::operator()(const u64& value) { return static_cast<u32>(value % UINT_MAX); }
	template<> inline u32 atMapHashFn<s64>::operator()(const s64& value) { return static_cast<u32>(value % UINT_MAX); }
	template<> inline u32 atMapHashFn<ConstString>::operator()(const ConstString& str) { return atStringHash(str); }
	template<> inline u32 atMapHashFn<ConstWString>::operator()(const ConstWString& str) { return atStringHash(str); }
	template<> inline u32 atMapHashFn<atString>::operator()(const atString& str) { return atStringHash(str); }
	template<> inline u32 atMapHashFn<atWideString>::operator()(const atWideString& str) { return atStringHash(str); }

	template<typename TValue, typename THashFn>
	class atMapIterator;

	/**
	 * \brief Hash table with separate chaining and mod index function, similar to .NET Dictionary;
	 * \n You can read on separate chaining & mod here: https://en.wikipedia.org/wiki/Hash_table
	 * or just refer to actual implementation.
	 */
	template<typename TValue, typename THashFn = atMapHashFn<TValue>>
	class atMap
	{
		using Iterator = atMapIterator<TValue, THashFn>;

		friend class Iterator;

		// Chosen as smallest non-prime number in (atHashNextSize / atHashSize) terms
		static constexpr u32 DEFAULT_CAPACITY = 10;

		struct Node
		{
			u32    HashKey;
			TValue Value;
			Node*  Next = nullptr;
		};

		Node** m_Buckets;
		u16    m_BucketCount;
		u16    m_UsedSlotCount;
		u8     m_Pad[3] = {};
		bool   m_AllowGrowing;

		Node** AllocateBuckets(u16 size)
		{
			// Same as below, allocate manually without invoking constructors
			size_t allocSize = static_cast<size_t>(size) * sizeof(Node*); // NOLINT(bugprone-sizeof-expression)
			Node** buckets = static_cast<Node**>(rage_malloc(allocSize));
			memset(buckets, 0, allocSize);
			return buckets;
		}

		Node* AllocateNode()
		{
			// Allocate without invoking default TValue constructor
			Node* node = static_cast<Node*>(rage_malloc(sizeof Node));
			memset(node, 0, sizeof Node);
			return node;
		}

		void DeleteNode(Node* node)
		{
			node->Value.~TValue();
			memset(node, 0, sizeof Node);

			// Delete without invoking TValue destructor, because otherwise destructor will be called for 
			// item that wasn't even constructed
			operator delete(node);
		}

		u32 GetHash(const TValue& value) const
		{
			THashFn fn{};
			return fn(value);
		}

		u16 GetBucket(const u32 hash) const
		{
			return static_cast<u16>(hash % m_BucketCount);
		}

		// Allocates new node and inserts it in the beginning of linked list
		// This function is used directly only for copying because we know that there cannot be node with given hash
		Node& AllocateNodeInLinkedList(u32 hash)
		{
			// Initialize if it wasn't
			if (m_BucketCount == 0)
				InitAndAllocate(DEFAULT_CAPACITY, true);

			// We can live without re-allocating because we use separate chaining
			// to resolve collision but eventually there will be too many collisions and it will slow down things
			m_UsedSlotCount++;
			if (m_AllowGrowing && m_UsedSlotCount > m_BucketCount)
				Resize(atHashSize(m_UsedSlotCount));

			// Allocate new node and insert it in the beginning of linked list
			Node* newNode = AllocateNode();
			newNode->HashKey = hash;

			u16 bucket = GetBucket(hash);
			newNode->Next = m_Buckets[bucket];
			m_Buckets[bucket] = newNode;

			return *newNode;
		}

		// First tries to find if there's already node with given hash and return it, if there's not -
		// allocates new node and inserts it in the beginning of linked list
		Node& AllocateNodeInLinkedListOrFindExisting(u32 hash)
		{
			// Check if slot already exists and return it if so
			Node* existingNode = TryGetNode(hash);
			if (existingNode)
			{
				existingNode->Value.~TValue();
				return *existingNode;
			}

			return AllocateNodeInLinkedList(hash);
		}

		// Tries to find node with given hash in separate chaining linked list
		// Additionally accepts out parameter with bucket where node was found (only if return node is not null)
		Node* TryGetNode(u32 hash, u16* outBucket = nullptr) const
		{
			if (m_BucketCount == 0) return nullptr; // Not even initialized

			// Find exact node in bucket linked list, if there is... (separate chaining)
			u16 bucket = GetBucket(hash);
			Node* node = m_Buckets[bucket];
			while (node)
			{
				if (node->HashKey == hash)
				{
					if (outBucket) *outBucket = bucket;
					return node;
				}
				node = node->Next;
			}
			return nullptr;
		}

		// Pops node from bucket linked list, releases node memory and decrements used slots counter.
		void RemoveNodeFromLinkedListAndFree(Node* node, u16 bucket)
		{
			if (m_Buckets[bucket] == node)
			{
				// Special case - linked list starts with given node
				m_Buckets[bucket] = node->Next;
			}
			else
			{
				Node* it = m_Buckets[bucket];

				AM_ASSERT(it != nullptr, "atMap::RemoveNodeFromLinkedListAndFree() -> Chain is empty!");

				// Search for node that points to one that we have to remove in linked list
				while (it)
				{
					if (it->Next == node)
					{
						// Remove node from linked list
						it->Next = node->Next;
						break;
					}
					it = it->Next;
				}
			}

			DeleteNode(node);

			m_UsedSlotCount--;
		}
	public:
		atMap()
		{
			m_Buckets = nullptr;
			m_BucketCount = 0;
			m_UsedSlotCount = 0;
			m_AllowGrowing = false;
		}
		atMap(const std::initializer_list<TValue>& list) : atMap()
		{
			InitAndAllocate(list.size());
			for (const TValue& value : list)
				Insert(value);
		}
		atMap(const atMap& other) : atMap()
		{
			CopyFrom(other);
		}
		// ReSharper disable once CppPossiblyUninitializedMember
		// ReSharper disable CppObjectMemberMightNotBeInitialized
		atMap(const datResource& rsc)
		{
			if (!m_Buckets)
				return;

			rsc.Fixup(m_Buckets);
			for (u16 i = 0; i < m_BucketCount; i++)
			{
				rsc.Fixup(m_Buckets[i]);

				Node* node = m_Buckets[i];
				while (node)
				{
					rsc.Fixup(node->Next);
					node = node->Next;
				}
			}
		}
		// ReSharper restore CppObjectMemberMightNotBeInitialized
		atMap(atMap&& other) noexcept : atMap()
		{
			Swap(other);
		}
		~atMap()
		{
			Destroy();
		}

		/*
		 *	------------------ Initializers / Destructors ------------------
		 */

		void InitAndAllocate(u16 bucketCountHint, bool allowGrowing = true)
		{
			Destroy();

			u16 bucketCount = atHashSize(bucketCountHint);

			m_Buckets = AllocateBuckets(bucketCount);
			m_BucketCount = bucketCount;
			m_AllowGrowing = allowGrowing;
		}

		void Swap(atMap& other)
		{
			std::swap(m_BucketCount, other.m_BucketCount);
			std::swap(m_AllowGrowing, other.m_AllowGrowing);
			std::swap(m_UsedSlotCount, other.m_UsedSlotCount);
			std::swap(m_Buckets, other.m_Buckets);
		}

		void CopyFrom(const atMap& other)
		{
			Destroy();

			InitAndAllocate(other.m_BucketCount, other.m_AllowGrowing);

			m_UsedSlotCount = other.m_UsedSlotCount;

			// Insert all items from other map
			if (m_UsedSlotCount > 0)
			{
				for (u16 bucket = 0; bucket < other.m_BucketCount; bucket++)
				{
					// Iterate separate chaining linked list in this bucket...
					Node* otherNode = other.m_Buckets[bucket];
					while (otherNode)
					{
						Node& copyNode = AllocateNodeInLinkedList(otherNode->HashKey);

						// Perform new placement via copy constructor
						void* where = &copyNode.Value;
						new(where) TValue(otherNode->Value);

						otherNode = otherNode->Next;
					}
				}
			}

			// Snapshot linked list if compiling resource
			pgSnapshotAllocator* compilerAllocator = pgRscCompiler::GetVirtualAllocator();
			if (compilerAllocator)
			{
				compilerAllocator->AddRef(m_Buckets);

				for (u16 bucket = 0; bucket < other.m_BucketCount; bucket++)
				{
					Node* node = other.m_Buckets[bucket];

					if (!node)
						continue;

					compilerAllocator->AddRef(other.m_Buckets[bucket]);

					while (node)
					{
						if (node->Next)
							compilerAllocator->AddRef(node->Next);

						node = node->Next;
					}
				}
			}
		}

		void Destroy()
		{
			if (!m_Buckets)
				return;

			// Delete nodes and call destructor, not any different from atMap one
			for (u16 i = 0; i < m_BucketCount; i++)
			{
				Node* node = m_Buckets[i];
				while (node)
				{
					Node* nodeToDelete = node;
					node = node->Next;
					DeleteNode(nodeToDelete);
				}
			}

			rage_free(m_Buckets);
			m_Buckets = nullptr;
			m_BucketCount = 0;
			m_UsedSlotCount = 0;
		}

		void Clear()
		{
			Destroy(); // Yes...
		}

		/*
		 *	------------------ Altering size ------------------
		 */

		void Resize(u16 newSizeHint)
		{
			u16 newSize = atHashSize(newSizeHint);
			if (m_BucketCount == newSize)
				return;

			// We need to save it to iterate through old buckets
			u16 oldBucketCount = m_BucketCount;
			// It has to be set before we use GetBucket() to relocate nodes
			m_BucketCount = newSize;

			Node** newBuckets = AllocateBuckets(newSize);

			// Move existing lists to new array
			for (u16 i = 0; i < oldBucketCount; i++)
			{
				// We have to 'reallocate' each node now because with new number of buckets
				// old function 'hash % size' is not valid anymore
				Node* node = m_Buckets[i];
				while (node)
				{
					u16 newBucket = GetBucket(node->HashKey);

					Node* next = node->Next;

					// Insert node in new bucket list
					node->Next = newBuckets[newBucket];
					newBuckets[newBucket] = node;

					node = next;
				}
			}

			rage_free(m_Buckets);
			m_Buckets = newBuckets;
		}

		/*
		 *	------------------ Adding / Removing items ------------------
		 */

		TValue& InsertAt(u32 hash, const TValue& value)
		{
			Node& node = AllocateNodeInLinkedListOrFindExisting(hash);
			new (&node.Value) TValue(value); // Placement new
			return node.Value;
		}

		TValue& Insert(const TValue& value)
		{
			u32 hash = GetHash(value);
			return InsertAt(hash, value);
		}

		template<typename... TArgs>
		TValue& ConstructAt(u32 hash, TArgs... args)
		{
			Node& node = AllocateNodeInLinkedListOrFindExisting(hash);
			new (&node.Value) TValue(args...); // Placement new
			return node.Value;
		}

		TValue& EmplaceAt(u32 hash, TValue&& value)
		{
			Node& node = AllocateNodeInLinkedListOrFindExisting(hash);
			new (&node.Value) TValue(std::move(value));
			return node.Value;
		}

		TValue& Emplace(TValue&& value)
		{
			u32 hash = GetHash(value);
			return EmplaceAt(hash, std::move(value));
		}

		void RemoveAtIterator(const Iterator& it);

		void RemoveAt(u32 hash)
		{
			u16 bucket;
			Node* node = TryGetNode(hash, &bucket);
			AM_ASSERT(node, "atMap::Remove() -> Slot with key hash %u is not allocated.", hash);

			RemoveNodeFromLinkedListAndFree(node, bucket);
		}

		void Remove(const TValue& value)
		{
			u32 hash = GetHash(value);
			RemoveAt(hash);
		}

		/*
		 *	------------------ Getters / Operators ------------------
		 */

		Iterator FindByHash(u32 hash);

		Iterator Find(const TValue& value)
		{
			u32 hash = GetHash(value);
			return FindByHash(hash);
		}

		TValue* TryGetAt(u32 hash) const
		{
			Node* node = TryGetNode(hash);
			if (!node)
				return nullptr;
			return &node->Value;
		}

		TValue& GetAt(u32 hash) const
		{
			TValue* value = TryGetAt(hash);
			AM_ASSERT(value, "atMap::Get() -> Value with hash %u is not present.", hash);
			return *value;
		}

		/**
		 * \brief Performs comparison by stored values.
		 */
		bool Equals(const atMap& other) const
		{
			// Collections of different size can't be equal
			if (m_UsedSlotCount != other.m_UsedSlotCount)
				return false;

			// Loop through every value and try to find it in other set
			for (u16 i = 0; i < m_BucketCount; i++)
			{
				Node* node = m_Buckets[i];
				while (node)
				{
					if (!other.ContainsAt(node->HashKey))
						return false;

					node = node->Next;
				}
			}
			return true;
		}

		atArray<TValue> ToArray() const;

		bool Any() const { return m_UsedSlotCount > 0; }
		bool Contains(const TValue& value) const { return TryGetNode(GetHash(value)) != nullptr; }
		bool ContainsAt(u32 hash) const { return TryGetNode(hash) != nullptr; }
		u32	GetBucketCount() const { return m_BucketCount; }
		u32 GetNumUsedSlots() const { return m_UsedSlotCount; }

		atMap& operator=(const std::initializer_list<TValue>& list)
		{
			Resize(list.size());
			Clear();
			for (const TValue& value : list)
				Insert(value);
			return *this;
		}

		atMap& operator=(const atMap& other) // NOLINT(bugprone-unhandled-self-assignment)
		{
			CopyFrom(other);
			return *this;
		}

		atMap& operator=(atMap&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		bool operator==(const std::initializer_list<TValue>& list)
		{
			// We can't compare by used slot count at this point because
			// initializer list may contain multiple unique values and it will give false results
			for (const TValue& value : list)
			{
				if (!Contains(value))
					return false;
			}
			return true;
		}

		bool operator==(const atMap& other) const { return Equals(other); }

		Iterator begin() const { return Iterator(this); }
		Iterator end() const { return Iterator::GetEnd(); }
	};

	/**
	 * \brief Allows to iterate through allocated slots in atMap.
	 */
	template<typename TValue, typename THashFn = atMapHashFn<TValue>>
	class atMapIterator
	{
		using Set = atMap<TValue, THashFn>;
		using Node = typename Set::Node;

		friend class Set;

		const Set* m_Set;
		s32 m_BucketIndex = 0;
		Node* m_Node = nullptr; // Pointer at current node in bucket's linked list

	public:
		atMapIterator(const Set* map) : m_Set(map)
		{
			if (!Next())
				m_Set = nullptr;
		}

		atMapIterator(const atMapIterator& other)
		{
			m_Set = other.m_Set;
			m_BucketIndex = other.m_BucketIndex;
			m_Node = other.m_Node;
		}

		atMapIterator(const Set* set, u16 bucketIndex, Node* node)
		{
			m_Set = set;
			m_BucketIndex = bucketIndex;
			m_Node = node;
		}

		static atMapIterator GetEnd() { return atMapIterator(nullptr); }

		bool Next()
		{
			if (!m_Set)
				return false;

			// Keep iterating linked list until we reach tail
			if (m_Node)
			{
				m_Node = m_Node->Next;
				if (m_Node)
				{
					return true;
				}
				// Keep iterating next bucket...
			}

			// We're out of map slots, this is over
			if (m_BucketIndex >= m_Set->m_BucketCount)
				return false;

			// Check if there's linked list in bucket at index
			m_Node = m_Set->m_Buckets[m_BucketIndex++];
			if (m_Node == nullptr)
				return Next(); // No linked list, search for next one

			// Begin iterating linked list
			return true;
		}

		atMapIterator operator++()
		{
			if (!Next())
			{
				m_Set = nullptr;
				return GetEnd();
			}
			return *this;
		}

		TValue& operator*() const { return m_Node->Value; }
		TValue& GetValue() const { return m_Node->Value; }
		bool HasValue() const { return m_Node != nullptr; }

		bool operator==(const atMapIterator& other) const
		{
			if (m_Set == nullptr && other.m_Set == nullptr)
				return true;

			return m_Set == other.m_Set &&
				m_BucketIndex == other.m_BucketIndex &&
				m_Node == other.m_Node;
		}
	};

	// Implementation of functions in atMap that depend on atMapIterator

	template <typename TValue, typename THashFn>
	void atMap<TValue, THashFn>::RemoveAtIterator(const Iterator& it)
	{
		AM_ASSERT(it != end(), "atMap::RemoveAtIterator() -> Invalid iterator!");

		RemoveNodeFromLinkedListAndFree(it.m_Node, it.m_BucketIndex);
	}

	template <typename TValue, typename THashFn>
	typename atMap<TValue, THashFn>::Iterator atMap<TValue, THashFn>::FindByHash(u32 hash)
	{
		u16 bucket;
		Node* node = TryGetNode(hash, &bucket);
		if (!node)
			return end();

		return Iterator(this, bucket, node);
	}

	template <typename TValue, typename THashFn>
	atArray<TValue> atMap<TValue, THashFn>::ToArray() const
	{
		atArray<TValue> result;
		result.Reserve(GetNumUsedSlots());

		for (const TValue& value : *this)
		{
			result.Add(value);
		}

		return result;
	}
}
