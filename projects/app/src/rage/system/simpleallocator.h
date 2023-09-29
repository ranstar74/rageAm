#pragma once

#include "allocator.h"
#include "smallocator.h"
#include "helpers/ranges.h"

#include "ipc/criticalsection.h"

namespace rage
{
	class fiStream;

	/*
	 *         Not linked, we use block size to navigate (Node)
	 *             ┌──────────────────────────────┐
	 * Heap        │                              │
	 * ┌─┬────────┬┴┬─────────┬─┬────────────────┬┴┬──────────┬─┬─────────────┐
	 * │_│        │#│         │_│                │#│          │_│             │
	 * │_│        │#│         │_│                │#│          │_│             │
	 * │_│        │#│         │_│                │#│          │_│             │
	 * │_│        │#│         │_│                │#│          │_│             │
	 * └┬┴────────┴─┴─────────┴┬┴────────────────┴─┴──────────┴┬┴─────────────┘
	 *  │                      │                               │    End of heap
	 *  └──────────────────────┴───────────────────────────────┘
	 *                   Linked List (FreeNode)
	 *
	 *
	 *  Every block has preamble, which also known as header and represented
	 *  by two structures - Node (#) and FreeNode (-)
	 *
	 *  FreeBlocks are linked between each other and grouped by buckets.
	 *  Size buckets are assigned by lowest exponent of two.
	 *  Bucket - { Size set }
	 *   1     - { 2, 3 }
	 *   2     - { 4, 5, 6, 7 }
	 *   3     - { 8, 9, 10, 11, 12, 13, 14, 15 }
	 *   ...
	 *   31    - { 2^31 to 2^32-1 }
	 *
	 *  Splitting size on buckets allows to quickly find node of required size when allocating.
	 *
	 *	Interesting functions to look at:
	 *	 - sysMemSimpleAllocator::DoAllocate
	 *	 - sysMemSimpleAllocator::DoFree
	 *	 - sysMemSimpleAllocator::DoResize
	 */

	 /**
	  * \brief General purpose allocator.
	  *	\n NOTE: Maxiumum size of allocator heap is 1GB!
	  *	Otherwise game will eventually overrun chunk map in smallocator.
	  */
	class sysMemSimpleAllocator : public sysMemAllocator
	{
		/**
		 * \brief Memory block header (or 'preamble').
		 */
		struct Node
		{
			static constexpr u32 NODE_ALLOCATED_SHIFT = 27;
			static constexpr u32 NODE_BUCKET_SHIFT = 28;
			static constexpr u32 NODE_BUCKET_MAX_VALUE = 0xF;

			static constexpr u32 NODE_BUCKET_MASK = NODE_BUCKET_MAX_VALUE << NODE_BUCKET_SHIFT; // High 4 bits
			static constexpr u32 NODE_ALLOC_ID_MASK = (1 << 26) - 1; // Low 25 bits (0x3FF FFFF)

			// Guard word is truncated memory address (in this implementation, but can be any constant),
			// allows to ensure that node (which is placed behind allocated memory blocks) is valid.
			u32 Guard = 0;

			u32 BlockSize;
			u32 PrevNodeOffset; // Previous block size + size of node

			// Node::Data bit layout
			// 
			//         ┌─ Is Allocated
			//         │
			//         │  ┌─ Unused          ┌─ AllocID, 25 bits (0 - 67 108 863)
			//         │  │                  │
			// ┌────┐  │  │  ┌───────────────┴────────────────┐
			// │0000│  0  0  │00 0000 0000 0000 0000 0000 0000│
			// └┬───┘        └────────────────────────────────┘
			//  │
			//  └── Memory bucket index (0 - 15)
			u32 Data = 0;

			Node(u32 size = 0);

			u32 CreateGuard() const;
			void AssertGuard() const;
			bool VerifyGuard() const { return Guard == CreateGuard(); }

			void SetNewNextNode(Node* next) { BlockSize = (u32)DISTANCE(GetBlock(), next); }
			void SetNewPreviousNode(Node* previous) { PrevNodeOffset = (u32)DISTANCE(previous, this); }

			// A bit of alien code to appreciate rockstar ability to pack universe in 32 bits.
			// Might help: https://stackoverflow.com/a/47990

			bool GetIsAllocated() const { return Data >> NODE_ALLOCATED_SHIFT & 1; }
			void SetAllocated(bool toggle)
			{
				Data &= ~(1 << NODE_ALLOCATED_SHIFT);
				Data |= toggle << NODE_ALLOCATED_SHIFT;
			}

			u8 GetMemoryBucket() const { return (u8)(Data >> NODE_BUCKET_SHIFT & NODE_BUCKET_MAX_VALUE); }
			void SetMemoryBucket(u8 bucket)
			{
				Data &= ~NODE_BUCKET_MASK;
				Data |= bucket << NODE_BUCKET_SHIFT;
			}

			u32 GetAllocID() const { return Data & NODE_ALLOC_ID_MASK; }
			void SetAllocID(u32 id)
			{
				Data &= ~NODE_ALLOC_ID_MASK;
				Data |= id;
			}

			u64 GetBlockAddress() const { return reinterpret_cast<u64>(this) + sizeof(Node); }
			char* GetBlock() const { return reinterpret_cast<char*>(GetBlockAddress()); }
		};

		struct FreeNode : Node
		{
			FreeNode* PreviousLinked = nullptr;
			FreeNode* NextLinked = nullptr;

			using Node::Node;
		};

		// ~1MB
		static constexpr u64 SMALLOCATOR_MIN_SIZE = 0x100000;

		static constexpr u64 HEADER_SIZE = sizeof Node;
		static constexpr u64 MAX_BUCKETS = 32;

		Node* m_MainBlock;
		pVoid m_HeapBase;

		// Accelerator structure to quickly find free blocks of memory
		FreeNode* m_FreeList[MAX_BUCKETS]{};

		u64 m_MainHeapSize = 0; // Distance from the main (root) node to the end of heap
		u64 m_HeapSize = 0; // Size of initial heap
		u64 m_UsedMemory = 0;

		u64 m_MemoryBuckets[SYS_MEM_MAX_MEMORY_BUCKETS]{};
		u64 m_MemorySnapshots[SYS_MEM_MAX_MEMORY_BUCKETS]{};

		u64 m_AvailableMemoryHigh;
		u64 m_AvailableMemoryLow;

		u32 m_Layers[8]{};
		u32 m_LayerCount = 0;

		u8 m_Unused264; // NOLINT(clang-diagnostic-unused-private-field)

		bool m_TraceStack = false; // For operation logging
		bool m_bOwnHeap;
		bool m_UseSmallocator;

		fiStream* m_LogStream = nullptr;

		// Used as AllocID (based on leftover debug strings in GTA IV in EndLayer function).
		//  'Leak allocid=%u, bucket=%u, size=%u bytes'
		// Value is never altered during lifetime in native implementation,
		// most likely cut with other logging things in release build.
		u32 m_CurrentAllocID;

		sysSmallocator m_Smallocator;

		u32 m_Unused2308; // NOLINT(clang-diagnostic-unused-private-field)

		bool m_QuitOnFail = true;

		sysCriticalSectionToken m_CriticalSection;

		enum eGetNodeHint
		{
			GET_NODE_DEFAULT,
			// Allows node that is on the end of heap range, used only to get block size.
			GET_NODE_ALLOW_END,
			GET_NODE_FREE,
		};

		// Sets up heap and allocates root node.
		void InitHeap(pVoid heap, u64 size, bool useSmallocator);

		// Gets next node that is located right after this node.
		// NOTE: It's not the same thing as NextLinked in FreeNode.
		Node* GetNextNodeInMemory(const Node* node, eGetNodeHint hint = GET_NODE_DEFAULT) const;

		// Gets previous node that is located right before this node.
		// NOTE: It's not the same thing as NextLinked in FreeNode.
		Node* GetPreviousNodeInMemory(const Node* node) const;

		// Performs placement new of node header at given address and adds that to memory used.
		Node* PlaceNode(pVoid where);

		// Cleans up memory used by node and sets memory to zero's.
		void DeleteNode(Node* node);

		// Checks whether address is in allocator heap range.
		bool IsNotEndNode(const Node* node) const;

		// Gets bucket index (or simply array index) for given size, explained in comment for m_FreeList field.
		u8 GetBucket(u64 size) const;

		// Gets node located right behind given block address, does not perform guard word check.
		Node* GetBlockNode(pVoid block) const;

		// Gets first node of requested size and aligning in given bucket.
		FreeNode* GetFreeListNode(u8 bucket, u64 size, u64 align) const;

		// Gets first node of requested size and aligning.
		FreeNode* GetFreeListNode(u64 size, u64 align) const;

		// Removes node from linked list.
		void RemoveFromFreeList(Node* node);

		// Aligns block and creates a new node to fill created gap in memory.
		Node* AlignBlock(Node* node, u64 align);

		// Splits block on two blocks (left and right) with given size constraint.
		// NOTE: Remaining block is not added to free list!
		// If right is not null, sets it to the right part of the block.
		// If left is not null, sets it to the left of the part block.
		void SplitBlock(Node* node, u64 size, Node** left = nullptr, Node** right = nullptr);

		// Joins left block with right one.
		void MergeBlocks(Node* left, FreeNode* right);

		// Inserts node at the beginning of free linked list.
		// Returns: inserted node as free node.
		FreeNode* AddToFreeList(Node* node);

		// Updates available memory stats, sets allocated flag.
		void MarkNodeAsAllocated(Node* node);

		// Updates available memory stats, resets allocated flag and link pointers.
		FreeNode* MarkNodeAsFree(Node* node);

		// If node is free, returns free node. Otherwise nullptr.
		FreeNode* GetAsFreeNode(Node* node) const;

		// Updates available & used memory stats.
		void AddToMemoryUsed(u64 value);

		// Updates available & used memory stats.
		void AddToMemoryAvailable(u64 value);

		// Prints out all free blocks and their sizes in allocator log stream.
		void PrintState() const;

		// Prints operation log if it was started using BeginMemoryLog.
		void PrintLogFor(const char* operation, const Node* node) const;

		// For easier search inside logs, format each alloc id as unique identifier.
		const char* FormatAllocID(u32 id) const;

		pVoid DoAllocate(u64 size, u64 align);
		void DoFree(pVoid block);
		void DoResize(pVoid block, u64 newSize);
		void DoSanityCheck() const;
	public:
		// For mode details, see allocator.h

		sysMemSimpleAllocator(u64 size, u8 allocIdSeed = 0, bool useSmallocator = true);
		~sysMemSimpleAllocator() override;

		bool SetQuitOnFail(bool toggle) override;

		pVoid Allocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override;
		pVoid TryAllocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override;

		void Free(pVoid block) override;

		void Resize(pVoid block, u64 newSize) override;

		u64 GetSize(pVoid block) override;
		u64 GetMemoryUsed(u8 memoryBucket = SYS_MEM_INVALID_BUCKET) override;
		u64 GetMemoryAvailable() override;
		u64 GetLargestAvailableBlock() override;

		u64 GetLowWaterMark(bool updateMeasure) override;
		u64 GetHighWaterMark(bool updateMeasure) override;

		void UpdateMemorySnapshots() override;
		u64 GetMemorySnapshot(u8 memoryBucket) override;

		bool IsTailed() override { return true; }

		u64 BeginLayer() override;
		void EndLayer(const char* layerName, const char* logName) override;

		void BeginMemoryLog(const char* logName, bool traceStack) override;
		void EndMemoryLog() override;

		bool IsBuildingResource() override { return false; }
		bool HasMemoryBuckets() override { return true; }

		void SanityCheck() override;

		bool IsValidPointer(pVoid block) override;

		u64 GetSizeWithOverhead(pVoid block) override;

		u64 GetHeapSize() override { return m_MainHeapSize; }
		pVoid GetHeapBase() override { return m_MainBlock; }
	};
}
