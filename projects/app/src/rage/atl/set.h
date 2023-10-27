//
// File: set.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "map.h"
#include "rage/paging/compiler/compiler.h"

namespace rage
{
	template<typename TValue, typename THashFn>
	class atSetIterator;

	/**
	 * \brief Unique set of values.
	 */
	template<typename TValue, typename THashFn = atMapHashFn<TValue>>
	class atSet
	{
		using Iterator = atSetIterator<TValue, THashFn>;

		friend class Iterator;

		// Chosen as smallest non-prime number in (atHashNextSize / atHashSize) terms
		static constexpr u32 DEFAULT_CAPACITY = 10;

		struct Node
		{
			u32	HashKey;
			TValue Value;
			Node* Next = nullptr;
		};

		Node** m_Buckets;

		u16		m_BucketCount;
		u16		m_UsedSlotCount;
		u8		m_UnusedC[3]{};	// I doubt that those are debug-related booleans but they're just here, unused
		bool	m_AllowGrowing;

		Node** AllocateBuckets(u16 size)
		{
			// Same as below, allocate manually without invoking constructors
			size_t allocSize = static_cast<size_t>(size) * sizeof(Node*); // NOLINT(bugprone-sizeof-expression)
			Node** buckets = static_cast<Node**>(operator new(allocSize));
			memset(buckets, 0, allocSize);
			return buckets;
		}

		void DeleteBuckets(Node** buckets)
		{
			// Same as below, allocate manually without invoking destructor
			operator delete(buckets);
		}

		Node* AllocateNode()
		{
			// Allocate without invoking default TValue constructor
			Node* node = static_cast<Node*>(operator new(sizeof Node));
			memset(node, 0, sizeof Node);
			return node;
		}

		void DeleteNode(Node* node)
		{
			node->Value.~TValue();

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
			Node* it = m_Buckets[bucket];

			if (m_Buckets[bucket] == node)
			{
				// Special case - linked list starts with given node
				m_Buckets[bucket] = node->Next;
			}
			else
			{
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
		atSet()
		{
			m_Buckets = nullptr;
			m_BucketCount = 0;
			m_UsedSlotCount = 0;
			m_AllowGrowing = false;
		}
		atSet(const std::initializer_list<TValue>& list) : atSet()
		{
			InitAndAllocate(list.size());
			for (const TValue& value : list)
				Insert(value);
		}
		atSet(const atSet& other) : atSet()
		{
			CopyFrom(other);
		}
		// ReSharper disable once CppPossiblyUninitializedMember
		// ReSharper disable CppObjectMemberMightNotBeInitialized
		atSet(const datResource& rsc)
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
		atSet(atSet&& other) noexcept : atSet()
		{
			Swap(other);
		}
		~atSet()
		{
			Destruct();
		}

		/*
		 *	------------------ Initializers / Destructors ------------------
		 */

		void InitAndAllocate(u16 bucketCountHint, bool allowGrowing = true)
		{
			Destruct();

			u16 bucketCount = atHashSize(bucketCountHint);

			m_Buckets = AllocateBuckets(bucketCount);
			m_BucketCount = bucketCount;
			m_AllowGrowing = allowGrowing;
		}

		void Swap(atSet& other)
		{
			std::swap(m_BucketCount, other.m_BucketCount);
			std::swap(m_AllowGrowing, other.m_AllowGrowing);
			std::swap(m_UsedSlotCount, other.m_UsedSlotCount);
			std::swap(m_Buckets, other.m_Buckets);
		}

		void CopyFrom(const atSet& other)
		{
			Destruct();

			InitAndAllocate(other.m_BucketCount, other.m_AllowGrowing);

			// Insert all items from other map
			for (u16 bucket = 0; bucket < other.m_BucketCount; bucket++)
			{
				// Iterate separate chaining linked list in this bucket...
				Node* otherNode = other.m_Buckets[bucket];
				while (otherNode)
				{
					Node& copyNode = AllocateNodeInLinkedList(otherNode->HashKey);

					// Perform new placement via copy constructor
					void* where = &copyNode.Value;
					new (where) TValue(otherNode->Value);

					otherNode = otherNode->Next;
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

		void Destruct()
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

			DeleteBuckets(m_Buckets);
			m_Buckets = nullptr;
			m_BucketCount = 0;
			m_UsedSlotCount = 0;
		}

		void Clear()
		{
			Destruct(); // Yes...
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

			DeleteBuckets(m_Buckets);
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
			AM_ASSERT(node, "atSet::Remove() -> Slot with key hash %u is not allocated.", hash);

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
			AM_ASSERT(value, "atSet::Get() -> Value with hash %u is not present.", hash);
			return *value;
		}

		/**
		 * \brief Performs comparison by stored values.
		 */
		bool Equals(const atSet& other) const
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

		atSet& operator=(const std::initializer_list<TValue>& list)
		{
			Resize(list.size());
			Clear();
			for (const TValue& value : list)
				Insert(value);
			return *this;
		}

		atSet& operator=(const atSet& other) // NOLINT(bugprone-unhandled-self-assignment)
		{
			CopyFrom(other);
			return *this;
		}

		atSet& operator=(atSet&& other) noexcept
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

		bool operator==(const atSet& other) const { return Equals(other); }

		Iterator begin() const { return Iterator(this); }
		Iterator end() const { return Iterator::GetEnd(); }
	};

	/**
	 * \brief Allows to iterate through allocated slots in atSet.
	 */
	template<typename TValue, typename THashFn = atMapHashFn<TValue>>
	class atSetIterator
	{
		using Set = atSet<TValue, THashFn>;
		using Node = typename Set::Node;

		friend class Set;

		const Set* m_Set;
		s32 m_BucketIndex = 0;
		Node* m_Node = nullptr; // Pointer at current node in bucket's linked list

	public:
		atSetIterator(const Set* map) : m_Set(map)
		{
			if (!Next())
				m_Set = nullptr;
		}

		atSetIterator(const atSetIterator& other)
		{
			m_Set = other.m_Set;
			m_BucketIndex = other.m_BucketIndex;
			m_Node = other.m_Node;
		}

		atSetIterator(const Set* set, u16 bucketIndex, Node* node)
		{
			m_Set = set;
			m_BucketIndex = bucketIndex;
			m_Node = node;
		}

		static atSetIterator GetEnd() { return atSetIterator(nullptr); }

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

		atSetIterator operator++()
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

		bool operator==(const atSetIterator& other) const
		{
			if (m_Set == nullptr && other.m_Set == nullptr)
				return true;

			return m_Set == other.m_Set &&
				m_BucketIndex == other.m_BucketIndex &&
				m_Node == other.m_Node;
		}
	};

	// Implementation of functions in atSet that depend on atSetIterator

	template <typename TValue, typename THashFn>
	void atSet<TValue, THashFn>::RemoveAtIterator(const Iterator& it)
	{
		AM_ASSERT(it != end(), "atSet::RemoveAtIterator() -> Invalid iterator!");

		RemoveNodeFromLinkedListAndFree(it.m_Node, it.m_BucketIndex);
	}

	template <typename TValue, typename THashFn>
	typename atSet<TValue, THashFn>::Iterator atSet<TValue, THashFn>::FindByHash(u32 hash)
	{
		u16 bucket;
		Node* node = TryGetNode(hash, &bucket);
		if (!node)
			return end();

		return Iterator(this, bucket, node);
	}

	template <typename TValue, typename THashFn>
	atArray<TValue> atSet<TValue, THashFn>::ToArray() const
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
