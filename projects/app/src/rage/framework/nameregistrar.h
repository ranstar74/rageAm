//
// File: nameregistrar.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "rage/atl/map.h"

namespace rage
{
	template<typename TValue>
	struct fwNameRegistrarKeyPair
	{
		u32 Key;
		TValue* Value;
	};

	template<typename TKey, typename TValue>
	struct fwNameRegistrarInitializerKeyPair
	{
		TKey Key;
		TValue Value;
	};

	// Forward declaration for class friend

	template<typename TKey, typename TValue, typename THashFn>
	class fwNameRegistrarIterator;

	/**
	 * \brief Hash table with separate chaining and mod index function, similar to .NET Dictionary;
	 * \n You can read on separate chaining & mod here: https://en.wikipedia.org/wiki/Hash_table
	 * or just refer to actual implementation.
	 */
	template<typename TKey, typename TValue, typename THashFn = atMapHashFn<TKey>>
	class fwNameRegistrar
	{
		using TIterator = fwNameRegistrarIterator<TKey, TValue, THashFn>;

		friend class TIterator;

		struct Node
		{
			u32		HashKey;
			TValue	Value;
			s32		NextIndex;
		};

		u32 GetHash(const TKey& key) const
		{
			THashFn fn{};
			return fn(key);
		}

		u32 GetBucket(const u32 hash) const
		{
			return hash % m_BucketCount;
		}

		Node* m_NodePool = nullptr;     // Node pool
		s32*  m_BucketToNode = nullptr; // Maps hash to node index using (hash % bucketCount) function
		u32   m_BucketCount = 0;        // Number of buckets
		s32   m_FreeIndex = 0;          // Index of head node in free linked list
		u32   m_UsedSlotCount = 0;      // Number of allocated (used) slots in map

		// Gets next node in linked list if given node is not tail; Otherwise null.
		Node* GetNextNode(Node* node) const
		{
			if (node->NextIndex == -1)
				return nullptr;
			return &m_NodePool[node->NextIndex];
		}

		Node* TryGetNodeAndIndexByHash(u32 hash, s32* outIndex = nullptr) const
		{
			if (outIndex) *outIndex = -1;

			u32 bucket = GetBucket(hash);
			s32 index = m_BucketToNode[bucket];
			if (index == -1)
				return nullptr;

			// Separate chaining - there is always multiple hashes that map to the same bucket (index),
			// that is solved using linked list with actual hash stored in node
			Node* node = &m_NodePool[index];
			while (node)
			{
				if (node->HashKey == hash)
				{
					if (outIndex) *outIndex = index;
					return node;
				}

				index = node->NextIndex;
				node = GetNextNode(node);
			}
			return nullptr;
		}

		Node* TryGetNodeAndIndexByKey(const TKey& key, s32* outIndex = nullptr)
		{
			u32 hash = GetHash(key);
			return TryGetNodeAndIndexByHash(hash, outIndex);
		}

		Node& AllocateNodeByHash(u32 hash)
		{
			Node* existingNode = TryGetNodeAndIndexByHash(hash);
			if (existingNode) return *existingNode;

			AM_ASSERT(m_FreeIndex != -1, "fwNameRegistrar::AllocateNodeByHash() -> Out of slots!");

			u32 bucket = GetBucket(hash);
			s32 index = m_FreeIndex;

			Node& node = m_NodePool[index];
			m_FreeIndex = node.NextIndex; // Remove node from free list

			// Get current head in the bucket linked list
			s32 bucketHead = m_BucketToNode[bucket];
			// Insert new head in bucket linked list
			m_BucketToNode[bucket] = index;
			m_UsedSlotCount++;

			node.HashKey = hash;
			node.NextIndex = bucketHead; // Link old list head with new head

			return node;
		}

		Node& AllocateNodeByKey(const TKey& key)
		{
			u32 hash = GetHash(key);
			return AllocateNodeByHash(hash);
		}

		void BuildLinkedList()
		{
			for (u32 i = 0; i < m_BucketCount; i++)
			{
				m_BucketToNode[i] = -1;

				Node& node = m_NodePool[i];
				node.NextIndex = i + 1;
			}
			m_NodePool[m_BucketCount - 1].NextIndex = -1; // Last node points to void
		}
	public:
		fwNameRegistrar() = default;
		fwNameRegistrar(const fwNameRegistrar& other)
		{
			CopyFrom(other);
		}
		fwNameRegistrar(fwNameRegistrar&& other) noexcept
		{
			Swap(other);
		}
		fwNameRegistrar(std::initializer_list<fwNameRegistrarInitializerKeyPair<TKey, TValue>> list)
		{
			InitAndAllocate(list);
			for (const auto& pair : list)
			{
				Insert(pair.Key, pair.Value);
			}
		}
		~fwNameRegistrar()
		{
			Destroy();
		}

		/*
		 *	------------------ Initializers / Destructors ------------------
		 */

		 /**
		  * \brief Has to be called once and before using.
		  * \param sizeHint Minimum size to allocate, in reality it's rounded to greater prime number.
		  * \remarks To get actual (allocated) size, use GetSize();
		  */
		void InitAndAllocate(u16 sizeHint)
		{
			if (m_BucketCount)
				Destroy();

			m_BucketCount = static_cast<u32>(atHashSize(sizeHint));
			m_BucketToNode = new s32[m_BucketCount];

			// Allocate without invoking default TValue constructor for each node
			size_t nodePoolAllocSize = static_cast<size_t>(m_BucketCount) * sizeof Node;
			m_NodePool = static_cast<Node*>(operator new(nodePoolAllocSize));
			memset(m_NodePool, 0, nodePoolAllocSize);  // NOLINT(bugprone-undefined-memory-manipulation)

			BuildLinkedList();
		}

		void Swap(fwNameRegistrar& other)
		{
			std::swap(m_NodePool, other.m_NodePool);
			std::swap(m_BucketToNode, other.m_BucketToNode);
			std::swap(m_BucketCount, other.m_BucketCount);
			std::swap(m_FreeIndex, other.m_FreeIndex);
			std::swap(m_UsedSlotCount, other.m_UsedSlotCount);
		}

		void CopyFrom(const fwNameRegistrar& other)
		{
			Destroy();

			m_BucketCount = other.m_BucketCount;
			m_FreeIndex = other.m_FreeIndex;
			m_UsedSlotCount = other.m_UsedSlotCount;

			InitAndAllocate(other.m_BucketCount);

			for (u32 i = 0; i < m_BucketCount; i++)
			{
				m_BucketToNode[i] = other.m_BucketToNode[i];
				m_NodePool[i] = other.m_NodePool[i];
			}
		}

		void Clear()
		{
			if (m_BucketCount == 0)
				return;

			// Destruct still allocated items
			for (u32 i = 0; i < m_BucketCount; i++) // This is basically unrolled version of fwNameRegistrarIterator
			{
				s32 index = m_BucketToNode[i];
				if (index == -1)
					continue; // Bucket list is empty

				// Loop through whole linked list in this bucket and call destructor for node values
				Node* node = &m_NodePool[index];
				while (true)
				{
					node->Value.~TValue();

					if (node->NextIndex == -1)
						break; // No more nodes

					node = &m_NodePool[node->NextIndex];
				}
			}

			// TODO: This is called on Destroy too, although it's unneeded!
			BuildLinkedList();
		}

		void Destroy()
		{
			Clear();

			delete m_BucketToNode;
			// Delete without invoking TValue destructor, because otherwise destructor will be called for 
			// items that weren't even constructed
			operator delete(m_NodePool);

			m_BucketToNode = nullptr;
			m_NodePool = nullptr;

			m_BucketCount = 0;
			m_UsedSlotCount = 0;
		}

		/*
		 *	------------------ Adding / Removing items ------------------
		 */

		TValue& InsertAt(u32 hash, const TValue& value)
		{
			Node& node = AllocateNodeByHash(hash);
			node.Value = value;
			return node.Value;
		}

		TValue& Insert(const TKey& key, const TValue& value)
		{
			u32 hash = GetHash(key);
			return InsertAt(hash, value);
		}

		template<typename... TArgs>
		TValue& ConstructAt(u32 hash, TArgs... args)
		{
			Node& node = AllocateNodeByHash(hash);
			pVoid where = &node.Value;
			new (where) TValue(args...);
			return node.Value;
		}

		template<typename... TArgs>
		TValue& Construct(const TKey& key, TArgs... args)
		{
			u32 hash = GetHash(key);
			return ConstructAt(hash, args...);
		}

		void RemoveAt(u32 hash)
		{
			s32 index;
			Node* node = TryGetNodeAndIndexByHash(hash, &index);
			AM_ASSERT(node, "fwNameRegistrar::Remove() -> Slot with key hash %u is not allocated.", hash);

			// Remove node from bucket linked list
			u32 bucket = GetBucket(hash);
			m_BucketToNode[bucket] = node->NextIndex;

			// Insert node back in free linked list
			node->NextIndex = m_FreeIndex;
			m_FreeIndex = index;

			m_UsedSlotCount--;
		}

		void Remove(const TKey& key)
		{
			u32 hash = GetHash(key);
			return RemoveAt(hash);
		}

		/*
		 *	------------------ Getters / Operators ------------------
		 */

		TValue* TryGetAt(u32 hash) const
		{
			Node* node = TryGetNodeAndIndexByHash(hash);
			if (node)
				return &node->Value;
			return nullptr;
		}

		TValue* TryGet(const TKey& key) const
		{
			u32 hash = GetHash(key);
			return TryGetAt(hash);
		}

		TValue& GetAt(u32 hash) const
		{
			TValue* value = TryGetAt(hash);
			AM_ASSERT(value != nullptr, "fwNameRegistrar::Get() -> Slot with hash %u doesn't  exist.", hash);
			return *value;
		}

		TValue& Get(const TKey& key) const
		{
			u32 hash = GetHash(key);
			return GetAt(hash);
		}

		bool Contains(const TKey& key) const { return TryGet(key) != nullptr; }
		bool ContainsAt(u32 hash) const { return TryGetAt(hash) != nullptr; }
		u32	GetSize() const { return m_BucketCount; }
		u32 GetNumUsedSlots() const { return m_UsedSlotCount; }

		[[nodiscard]] TValue& operator[](const TKey& key)
		{
			TValue* existingValue = TryGet(key);
			if (existingValue)
				return *existingValue;
			return Insert(key, {});
		}

		fwNameRegistrar& operator=(const fwNameRegistrar& other) // NOLINT(bugprone-unhandled-self-assignment)
		{
			CopyFrom(other);
			return *this;
		}

		fwNameRegistrar& operator=(fwNameRegistrar&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		TIterator begin() const { return TIterator(this); }
		TIterator end() const { return TIterator::GetEnd(); }
	};

	/**
	 * \brief Allows to iterate through allocated slots in fwNameRegistrar.
	 */
	template<typename TKey, typename TValue, typename THashFn = atMapHashFn<TKey>>
	class fwNameRegistrarIterator
	{
		using TMap = fwNameRegistrar<TKey, TValue, THashFn>;
		using TPair = fwNameRegistrarKeyPair<TValue>;
		using TNode = typename TMap::Node;

		friend class TMap;

		const TMap* m_Map;
		s32 m_BucketIndex = -1;
		TNode* m_Node = nullptr; // Pointer at current node in bucket's linked list
		TPair m_Pair;

	public:
		fwNameRegistrarIterator(const TMap* map) : m_Map(map)
		{
			if (!Next())
				m_Map = nullptr;
		}

		fwNameRegistrarIterator(const fwNameRegistrarIterator& other)
		{
			m_Map = other.m_Map;
			m_BucketIndex = other.m_BucketIndex;
			m_Node = other.m_Node;
			m_Pair = other.m_Pair;
		}

		static fwNameRegistrarIterator GetEnd() { return fwNameRegistrarIterator(nullptr); }

		bool Next()
		{
			if (!m_Map)
				return false;

			// Keep iterating linked list until we reach tail
			if (m_Node)
			{
				m_Node = m_Map->GetNextNode(m_Node);
				if (m_Node)
				{
					m_Pair.Key = m_Node->HashKey;
					m_Pair.Value = &m_Node->Value;
				}
				// Keep iterating next bucket...
			}

			m_BucketIndex++;

			// We're out of map slots, this is over
			if (m_BucketIndex >= m_Map->m_BucketCount)
				return false;

			// Check if there's linked list in bucket at index
			s32 nodeIndex = m_Map->m_BucketToNode[m_BucketIndex];
			if (nodeIndex == -1)
				return Next(); // No linked list, search for next one

			// Begin iterating linked list
			m_Node = &m_Map->m_NodePool[nodeIndex];
			m_Pair.Key = m_Node->HashKey;
			m_Pair.Value = &m_Node->Value;
			return true;
		}

		fwNameRegistrarIterator operator++()
		{
			if (!Next())
			{
				m_Map = nullptr;
				return GetEnd();
			}
			return *this;
		}

		TPair& operator*() { return m_Pair; }

		bool operator==(const fwNameRegistrarIterator& other) const
		{
			if (m_Map == nullptr && other.m_Map == nullptr)
				return true;

			return
				m_Map == other.m_Map &&
				m_BucketIndex == other.m_BucketIndex &&
				m_Node == other.m_Node;
		}
	};
}
