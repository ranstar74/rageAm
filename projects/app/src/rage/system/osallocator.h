#pragma once

#include "allocator.h"

// #define ENABLE_SYS_MEM_NODES

namespace rage
{
	// Note: This is not part of rage.

	/**
	 * \brief Uses system calls (malloc, free) to allocate memory, used for testing purposes.
	 */
	class sysMemOsAllocator : public sysMemAllocator
	{
#ifdef ENABLE_SYS_MEM_NODES
		// We hold linked list to detect if block pointer is valid (needed for virtual / physical allocator)

		struct Node
		{
			static constexpr u32 GUARD = MAKEFOURCC('R', 'A', 'G', 'E');

			u32 Guard;
			Node* Prev;
			Node* Next;
			pVoid Block;

			void DeleteSelfAndChildren()
			{
				if (Next)
					Next->DeleteSelfAndChildren();
				free(this);
			}
		};
		Node* m_RootNode = nullptr;

		Node* FindNode(pVoid block) const
		{
			Node* node = m_RootNode;
			while (node)
			{
				AM_ASSERT(node->Guard == Node::GUARD, "sysMemOsAllocator::FindNode() -> Guard was trashed.");
				if (node->Block == block)
					return node;
				node = node->Next;
			}
			return nullptr;
		}
#endif

	public:
		~sysMemOsAllocator() override
		{
#ifdef ENABLE_SYS_MEM_NODES
			// If any left then we've got memory leaks
			m_RootNode->DeleteSelfAndChildren();
#endif
		}

		pVoid Allocate(u64 size, u64 align, u32 type) override
		{
			pVoid block = _aligned_malloc(size, align);

#ifdef ENABLE_SYS_MEM_NODES
			// Insert block in linked list
			Node* blockNode = (Node*)_aligned_malloc(sizeof Node, 16);
			memset(blockNode, 0, sizeof Node);
			blockNode->Block = block;
			blockNode->Next = m_RootNode;
			blockNode->Guard = Node::GUARD;
			if (m_RootNode) m_RootNode->Prev = blockNode;
			m_RootNode = blockNode;
#endif

			return block;
		}

		pVoid TryAllocate(u64 size, u64 align, u32 type) override
		{
			return Allocate(size, align, type);
		}

		void Free(pVoid block) override
		{
#ifdef ENABLE_SYS_MEM_NODES
			// Remove block from linked list
			Node* blockNode = FindNode(block);

			if (blockNode->Next) blockNode->Next->Prev = blockNode->Prev;
			if (blockNode->Prev) blockNode->Prev->Next = blockNode->Next;
			if (m_RootNode == blockNode)
				m_RootNode = blockNode->Next;

			_aligned_free(blockNode);
#endif
			_aligned_free(block);
		}

#ifdef ENABLE_SYS_MEM_NODES
		bool IsValidPointer(pVoid block) override
		{
			return FindNode(block) != nullptr;
		}
#endif

		u64 GetMemoryUsed(u8 memoryBucket) override { return 0; }
		u64 GetMemoryAvailable() override { return 0; }
	};
}
