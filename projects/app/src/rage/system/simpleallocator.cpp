#include "simpleallocator.h"

#include <new.h>

#include "memory.h"
#include "am/file/fileutils.h"

#include "common/logger.h"

#include "am/system/asserts.h"
#include "am/system/errordisplay.h"
#include "am/system/exception/stacktrace.h"

#include "helpers/bits.h"
#include "helpers/align.h"
#include "helpers/ranges.h"
#include "helpers/compiler.h"

#include "rage/file/stream.h"
#include "helpers/format.h"
#include "rage/file/device/local.h"

rage::sysMemSimpleAllocator::Node::Node(u32 size)
{
	Guard = CreateGuard();
	BlockSize = size;
}

u32 rage::sysMemSimpleAllocator::Node::CreateGuard() const
{
	// 'reinterpret_cast': truncation
#pragma warning( push )
#pragma warning( disable : 4311)
#pragma warning( disable : 4302)
	return reinterpret_cast<u32>(this); // NOLINT(clang-diagnostic-pointer-to-int-cast)
#pragma warning( pop ) 
}

void rage::sysMemSimpleAllocator::Node::AssertGuard() const
{
	AM_ASSERT(VerifyGuard(), "SimpleAllocator::Node::AssertGuard() -> Guard word was trashed.");
}

void rage::sysMemSimpleAllocator::InitHeap(pVoid heap, u64 size, bool useSmallocator)
{
	AM_ASSERT(size <= SYS_GENERAL_MAX_HEAP_SIZE, "SimpleAllocator::InitHeap() -> Size %llu is larger than allowed maximum %llu",
		size, SYS_GENERAL_MAX_HEAP_SIZE);

	m_HeapBase = heap;
	m_HeapSize = size;

	if (useSmallocator && size >= SMALLOCATOR_MIN_SIZE)
	{
		m_UseSmallocator = true;
		m_Smallocator.Init(this);
	}

	u64 heapAddr = reinterpret_cast<u64>(heap);

	// To calculate root node size
	Node* endNode = reinterpret_cast<Node*>(heapAddr + size);

	// Having root node aligned to 32 will allow us to make first
	// allocation without extra aligning
	Node* root = PlaceNode(reinterpret_cast<pVoid>(ALIGN_32(heapAddr)));
	root->SetNewNextNode(endNode);

	m_MainHeapSize = root->BlockSize + HEADER_SIZE;
	m_MainBlock = root;

	m_AvailableMemoryHigh = root->BlockSize;
	m_AvailableMemoryLow = m_AvailableMemoryHigh;

	AddToFreeList(root);
	PrintState();
}

rage::sysMemSimpleAllocator::Node* rage::sysMemSimpleAllocator::GetNextNodeInMemory(const Node* node, eGetNodeHint hint) const
{
	Node* next = reinterpret_cast<Node*>(node->GetBlock() + node->BlockSize);

	if (hint != GET_NODE_ALLOW_END && !IsNotEndNode(next))
		return nullptr;

	if (hint == GET_NODE_FREE)
	{
		next->AssertGuard();
		if (next->GetIsAllocated())
			return nullptr;
	}
	return next;
}

rage::sysMemSimpleAllocator::Node* rage::sysMemSimpleAllocator::GetPreviousNodeInMemory(const Node* node) const
{
	if (node->PrevNodeOffset == 0)
		return nullptr;

	return reinterpret_cast<Node*>(reinterpret_cast<u64>(node) - node->PrevNodeOffset);
}

rage::sysMemSimpleAllocator::Node* rage::sysMemSimpleAllocator::PlaceNode(pVoid where)
{
	ALLOC_LOG("SimpleAllocator::PlaceNode(%p) - Allocating %llu bytes for node", where, HEADER_SIZE);

	AddToMemoryUsed(HEADER_SIZE);

	return new (where) Node();
}

void rage::sysMemSimpleAllocator::DeleteNode(Node* node)
{
	ALLOC_LOG("SimpleAllocator::DeleteNode(%p) -> Brought back %llu bytes back", (pVoid)node, HEADER_SIZE);

	AddToMemoryAvailable(HEADER_SIZE);

	// To prevent anyone from passing pointer on this block.
	memset(node, 0, sizeof Node);
}

bool rage::sysMemSimpleAllocator::IsNotEndNode(const Node* node) const
{
	return reinterpret_cast<u64>(node) < reinterpret_cast<u64>(m_MainBlock) + m_MainHeapSize;
}

u8 rage::sysMemSimpleAllocator::GetBucket(u64 size) const
{
	return BitScanR64(size);
}

rage::sysMemSimpleAllocator::Node* rage::sysMemSimpleAllocator::GetBlockNode(pVoid block) const
{
	u64 addr = reinterpret_cast<u64>(block);
	return reinterpret_cast<Node*>(addr - HEADER_SIZE);
}

rage::sysMemSimpleAllocator::FreeNode* rage::sysMemSimpleAllocator::GetFreeListNode(u8 bucket, u64 size, u64 align) const
{
	FreeNode* node = m_FreeList[bucket];
	while (node)
	{
		node->AssertGuard();

		// Align current block address and see if after that we have enough memory
		// Also make sure that after aligning blocks are still in the same order

		u64 addr = node->GetBlockAddress();

		// See ::AlignBlock() for header overlap fix explanation.
		addr += sizeof(FreeNode);

		u64 addrAligned = ALIGN(addr, align);
		u64 nextAddr = node->GetBlockAddress() + node->BlockSize;
		u64 remainSize = DISTANCE(addrAligned, nextAddr);

		if (addrAligned < nextAddr && remainSize >= size)
			break;

		node = node->NextLinked;
	}
	return node;
}

rage::sysMemSimpleAllocator::FreeNode* rage::sysMemSimpleAllocator::GetFreeListNode(u64 size, u64 align) const
{
	u8 bucket = GetBucket(size);

	FreeNode* node = nullptr;
	while (bucket < MAX_BUCKETS)
	{
		node = GetFreeListNode(bucket++, size, align);
		if (node)
			break;
	}
	return node;
}

void rage::sysMemSimpleAllocator::RemoveFromFreeList(Node* node)
{
	FreeNode* freeNode = reinterpret_cast<FreeNode*>(node);

	FreeNode* next = freeNode->NextLinked;
	FreeNode* previous = freeNode->PreviousLinked;

	// If node have previous node, link previous node with next node.
	// Otherwise add next node as root node to free list
	if (previous)
		previous->NextLinked = next;
	else
		m_FreeList[GetBucket(node->BlockSize)] = next;

	// Link next node with previous node
	if (next)
		next->PreviousLinked = previous;
}

rage::sysMemSimpleAllocator::Node* rage::sysMemSimpleAllocator::AlignBlock(Node* node, u64 align)
{
	AM_ASSERT(!node->GetIsAllocated(),
		"SimpleAllocator::AlignBlock -> Can't align allocated block.");

	Node* nextNode = GetNextNodeInMemory(node, GET_NODE_ALLOW_END);

	// [       BLOCK MEMORY          ]
	// [    GAP     | ALIGNED MEMORY ]

	u64 block = node->GetBlockAddress();

	// NOTE: Somehow rage implementation doesn't take into account that FreeNode struct is 32 byte in length, not 16...
	// And in result NextLinked and PreviousLinked pointers are overwritten by allocated block
	// Header aligned to 16 and block aligned to 32 will cause this issue.
	// To fix that we need to make sure that beginning of next block is at least 32 bytes apart,
	// which is (sizeof(FreeNode) - sizeof(Node)) = memory required for linked list and + sizeof(Node) - memory for next header,
	// simplifies to sizeof(FreeNode).
	block += sizeof(FreeNode);

	u64 alignedBlock = ALIGN(block, align);
	u32 alignedGap = static_cast<u32>(alignedBlock - block);

	node->BlockSize += alignedGap;

	Node* alignedNode = PlaceNode(reinterpret_cast<pVoid>(alignedBlock - HEADER_SIZE));
	alignedNode->SetNewPreviousNode(node);
	alignedNode->SetNewNextNode(nextNode);

	if (IsNotEndNode(nextNode))
		nextNode->SetNewPreviousNode(alignedNode);

	node->SetNewNextNode(alignedNode);

	AddToFreeList(node);

	ALLOC_LOG("SimpleAllocator::AlignBlock() -> Done aligning with 2 blocks with sizes of %u and %u",
		node->BlockSize, alignedNode->BlockSize);

	return alignedNode;
}

void rage::sysMemSimpleAllocator::SplitBlock(Node* node, u64 size, Node** left, Node** right)
{
	Node* nextNode = GetNextNodeInMemory(node, GET_NODE_ALLOW_END);

	// [        BLOCK MEMORY        ]
	// [ USED MEMORY | SPLIT MEMORY ]

	Node* leftover = PlaceNode(node->GetBlock() + size);
	leftover->SetNewNextNode(nextNode);
	leftover->SetNewPreviousNode(node);

	if (IsNotEndNode(nextNode))
		nextNode->SetNewPreviousNode(leftover);

	node->SetNewNextNode(leftover);

	ALLOC_LOG("SimpleAllocator::SplitBlock() -> Splitted block on two with sizes of %u and %u",
		node->BlockSize, leftover->BlockSize);

	if (left) *left = node;
	if (right) *right = leftover;
}

void rage::sysMemSimpleAllocator::MergeBlocks(Node* left, FreeNode* right)
{
	Node* nextNode = GetNextNodeInMemory(right, GET_NODE_ALLOW_END);
	left->SetNewNextNode(nextNode);

	// Fixup reference on this node for next node
	if (nextNode && IsNotEndNode(nextNode))
		nextNode->SetNewPreviousNode(left);

	RemoveFromFreeList(right);
	DeleteNode(right);

	ALLOC_LOG("SimpleAllocator::MergeBlocks() -> Merged 2 blocks into one with size of %u",
		left->BlockSize);
}

rage::sysMemSimpleAllocator::FreeNode* rage::sysMemSimpleAllocator::AddToFreeList(Node* node)
{
	u8 bucket = GetBucket(node->BlockSize);

	FreeNode* freeNode = reinterpret_cast<FreeNode*>(node);
	FreeNode* rootNode = m_FreeList[bucket];

	freeNode->NextLinked = rootNode;
	if (rootNode)
		rootNode->PreviousLinked = freeNode;
	freeNode->PreviousLinked = nullptr;

	m_FreeList[bucket] = freeNode;

	return freeNode;
}

void rage::sysMemSimpleAllocator::MarkNodeAsAllocated(Node* node)
{
	u8 memoryBucket = GetCurrentMemoryBucket();

	m_MemoryBuckets[memoryBucket] += node->BlockSize;

	AddToMemoryUsed(node->BlockSize);

	node->SetAllocated(true);
	node->SetAllocID(++m_CurrentAllocID);
	node->SetMemoryBucket(memoryBucket);
}

rage::sysMemSimpleAllocator::FreeNode* rage::sysMemSimpleAllocator::MarkNodeAsFree(Node* node)
{
	m_MemoryBuckets[node->GetMemoryBucket()] -= node->BlockSize;

	AddToMemoryAvailable(node->BlockSize);

	node->SetAllocated(false);

	FreeNode* freeNode = reinterpret_cast<FreeNode*>(node);

	// This is important if we don't want to get pointers on garbage
	freeNode->NextLinked = nullptr;
	freeNode->PreviousLinked = nullptr;

	return freeNode;
}

rage::sysMemSimpleAllocator::FreeNode* rage::sysMemSimpleAllocator::GetAsFreeNode(Node* node) const
{
	if (!node)
		return nullptr;

	node->AssertGuard();
	if (node->GetIsAllocated())
		return nullptr;

	return reinterpret_cast<FreeNode*>(node);
}

void rage::sysMemSimpleAllocator::AddToMemoryUsed(u64 value)
{
	ALLOC_LOG("SimpleAllocator::AddToMemoryUsed(%llu)", value);

	m_UsedMemory += value;
	m_AvailableMemoryHigh -= value;
}

void rage::sysMemSimpleAllocator::AddToMemoryAvailable(u64 value)
{
	ALLOC_LOG("SimpleAllocator::AddToMemoryAvailable(%llu)", value);

	m_UsedMemory -= value;
	m_AvailableMemoryHigh += value;
}

void rage::sysMemSimpleAllocator::PrintState() const
{
#ifdef ALLOC_PRINT_STATE
	ALLOC_LOG("SimpleAllocator::PrintState() -> ");

	ALLOC_LOG(" Used Memory: %llu", m_UsedMemory);
	ALLOC_LOG(" Available Memory: %llu", m_AvailableMemoryHigh);

	for (u8 i = 0; i < MAX_BUCKETS; i++)
	{
		const FreeNode* node = m_FreeList[i];
		if (!node)
			continue; // Skip empty bucket

		ALLOC_LOG(" Bucket: %i (%i to %i), Free block (size, address):", i, 1 << i, 1 << (i + 1));
		while (node)
		{
			ALLOC_LOG(" - %i (%p)", node->BlockSize, (pVoid)node);

			node = node->NextLinked;
		}
	}
#endif
}

AM_NOINLINE void rage::sysMemSimpleAllocator::PrintLogFor(const char* operation, const Node* node) const
{
#ifdef ENABLE_ALLOCATOR_OPERATION_LOG
	if (!m_LogStream)
		return;

	// op,ptr,allocId,size,bucket,stack
	m_LogStream->WriteLinef("%s, 0x%p, %s, %u, %u\n",
		operation, (pVoid)node->GetBlockAddress(), FormatAllocID(node->GetAllocID()),
		node->BlockSize, node->GetMemoryBucket());

	if (m_TraceStack)
	{
		static char buffer[rageam::STACKTRACE_BUFFER_SIZE];
		rageam::StackTracer::CaptureAndWriteTo(buffer, rageam::STACKTRACE_BUFFER_SIZE, 1 /* PrintLogFor */);
		m_LogStream->WriteLine(buffer);
	}

	m_LogStream->WriteLine("\n");
#endif
}

const char* rage::sysMemSimpleAllocator::FormatAllocID(u32 id) const
{
	static char buffer[32];
	sprintf_s(buffer, 32, "A_ID_%u", id);
	return buffer;
}

pVoid rage::sysMemSimpleAllocator::DoAllocate(u64 size, u64 align)
{
	/*
	 * # - Allocated memory.
	 *
	 *                        ┌── New block at the end of
	 * Heap                   │   allocated memory.
	 * ┌─┬────────┬─┬────────┬┴┬────────────────┬─┐
	 * │_│        │_│########│_│                │x│
	 * │_│        │_│########│_│                │x│
	 * └┬┴────────┴┬┴────────┴─┴────────────────┴┬┘
	 *  │          │                             │
	 *  │          │                             └── 'End' node
	 *  │          └─ Block that we've                Exists only as pointer,
	 *  │             got after aligning.             used to calculate block size.
	 *  │
	 *  └─ Free block that we found
	 *     as suitable for allocation.
	 *
	 * In diagram shown scenario when allocation
	 * required aligning and free block was larger than requested.
	 *
	 * In result we've got one free block before allocation (at index #0),
	 * allocated block (#1) and block with remaining memory after allocation (#2)
	 *
	 * Algorithm:
	 *
	 *  1) Find free memory block that satisfies size and alignment constraints.
	 *  2) If block is not aligned, create new block at aligned address.
	 *  3) If block is larger than size constraint, split it on two blocks.
	 */

	align = MAX(align, 16);

	size = MAX(size, sizeof(FreeNode));
	size = ALIGN_16(size);

	ALLOC_LOG("SimpleAllocator::DoAllocate(size: %llu, align: %llu)", size, align);

	Node* node = GetFreeListNode(size, align);
	if (!node)
	{
		AM_ERR("SimpleAllocator::DoAllocate() -> Unable to find free node for requested size, allocator is out of memory...");
		PrintState();

		return nullptr;
	}
	ALLOC_LOG("SimpleAllocator::DoAllocate() -> Found free node with size of %u", node->BlockSize);

	RemoveFromFreeList(node);

	u64 block = node->GetBlockAddress();

	if (!IS_ALIGNED(block, align))
	{
		ALLOC_LOG("SimpleAllocator::DoAllocate() -> Block is not aligned");

		node = AlignBlock(node, align);
	}

	// If block is larger than our requirement, we'll have to split it on two blocks.
	if (node->BlockSize > size)
	{
		ALLOC_LOG("SimpleAllocator::DoAllocate() -> Block size %u is larger than required %llu, splitting...", node->BlockSize, size);

		Node* right;
		SplitBlock(node, size, &node, &right);
		AddToFreeList(right);
	}

	MarkNodeAsAllocated(node);

	m_AvailableMemoryLow = MIN(m_AvailableMemoryLow, m_AvailableMemoryHigh);

	ALLOC_LOG("SimpleAllocator::DoAllocate() -> Allocated block located at %p", (pVoid)node->GetBlock());

	PrintState();

	return node->GetBlock();
}

void rage::sysMemSimpleAllocator::DoFree(pVoid block)
{
	/*
	 * _ - Free memory.
	 *
	 * # - Allocated memory.
	 *
	 *                        ┌─ Next free block.
	 * Heap                   │
	 * ┌─┬────────┬─┬────────┬┴┬────────────────┬─┐
	 * │_│        │#│########│_│                │x│
	 * │_│        │#│########│_│                │x│
	 * └┬┴────────┴┬┴────────┴─┴────────────────┴─┘
	 *  │          │
	 *  │          └─ Allocated block that
	 *  │             has to be freed.
	 *  │
	 *  └─ Previous free block.
	 *
	 *
	 * Heap
	 * ┌─┬──────────────────────────────────────┬─┐
	 * │_│                                      │x│
	 * │_│                                      │x│
	 * └┬┴──────────────────────────────────────┴─┘
	 *  │
	 *  │
	 *  └─ Merged block.
	 *
	 * In diagram shown scenario when we want to free
	 * allocated block and both adjacent blocks are free.
	 *
	 * Memory de-allocation is done via setting allocated bit
	 * in Node::Data field to false and joining adjacent
	 * blocks to reduce memory fragmentation.
	 */

	AM_ASSERT(IsValidPointer(block), "SimpleAllocator::DoFree() -> Pointer is not valid.");

	Node* node = GetBlockNode(block);
	node->AssertGuard();

	AM_ASSERT(node->GetIsAllocated(), "SimpleAllocator::DoFree() -> Called on a free block.");

	ALLOC_LOG("SimpleAllocator::DoFree() -> Freeing block at %p with size of %u", block, node->BlockSize);

	// We could just end up on this but it would be good to join adjacent blocks to reduce memory fragmentation
	FreeNode* freeNode = MarkNodeAsFree(node);
	FreeNode* nextNode = GetAsFreeNode(GetNextNodeInMemory(node));
	FreeNode* prevNode = GetAsFreeNode(GetPreviousNodeInMemory(node));

	// If next block is free, we can merge
	if (nextNode)
	{
		ALLOC_LOG("SimpleAllocator::DoFree() -> Next block is free and has size of %u, joining...", nextNode->BlockSize);

		MergeBlocks(node, nextNode);
	}

	// Same as above, we can merge previous block
	if (prevNode)
	{
		ALLOC_LOG("SimpleAllocator::DoFree() -> Previous block is free and has size of %u, joining...", prevNode->BlockSize);

		RemoveFromFreeList(prevNode);
		MergeBlocks(prevNode, freeNode);
		node = prevNode;
	}
	AddToFreeList(node);
	PrintState();

	ALLOC_LOG("SimpleAllocator::DoFree() -> Added block with size of %u in free list", node->BlockSize);
}

void rage::sysMemSimpleAllocator::DoResize(pVoid block, u64 newSize)
{
	/*
	 * - Block resizing (shrinking)
	 *
	 * Before:
	 *
	 * # - Allocated memory.
	 *
	 *             ┌── New block that will be inserted
	 * Heap        │   after shrinking.
	 * ┌─┬────────┬┴┬───────┬─┬────────┬─┬────────┐
	 * │#│########│#│#######│_│        │_│        │
	 * │#│########│#│#######│_│        │_│        │
	 * └┬┴────────┴─┴───────┴┬┴────────┴┬┴────────┘
	 *  │                    │          │
	 *  │                    │          └───────── Next free block.
	 *  └─ Block that we     └──── Current end of
	 *     we want to shrink.      the block.
	 *
	 * After:
	 *
	 *             ┌── New end of the block.
	 * Heap        │
	 * ┌─┬────────┬┴┬──────────────────┬─┬────────┐
	 * │#│########│_│ Merged remaining │_│        │
	 * │#│########│_│ block with next  │_│        │
	 * │#│########│_│ free block.      │_│        │
	 * └┬┴────────┴─┴──────────────────┴─┴────────┘
	 *  │
	 *  │
	 *  └─ Start of the block.
	 *
	 * In diagram shown scenario when we want to resize (shrink) a block.
	 *
	 * Shrinking is done by inserting a new block at (block start + new size)
	 * Additionally, just like when we free block, we want to check if next
	 * to inserted block is free, so it can be merged.
	 */

	AM_ASSERT(IsValidPointer(block), "SimpleAllocator::DoResize() -> Pointer is not valid.");

	Node* node = GetBlockNode(block);
	node->AssertGuard();

	AM_ASSERT(node->GetIsAllocated(), "SimpleAllocator::DoResize() -> Called on a free block.");

	u32 currentSize = node->BlockSize;
	u32 size = ALIGN_16(newSize);
	u32 freeMemory = currentSize - size;

	ALLOC_LOG("SimpleAllocator::DoResize() -> Resizing block at %p with size: %u to new size: %u",
		block, currentSize, size);

	if (size < sizeof(FreeNode))
	{
		ALLOC_LOG("SimpleAllocator::DoResize() -> Free block can't be fit in remaining space, skipping...");
		return;
	}

	if (size == currentSize)
	{
		ALLOC_LOG("SimpleAllocator::DoResize() -> Current block size is equal to new size, skipping...");
		return;
	}

	AM_ASSERT(size < currentSize, "SimpleAllocator::DoResize() -> Cannot expand block! Only shrink is allowed");

	Node* leftover;
	SplitBlock(node, size, nullptr, &leftover);
	AddToMemoryAvailable(freeMemory);

	ALLOC_LOG("SimpleAllocator::DoResize() -> Brought %u bytes back to available memory", leftover->BlockSize);

	FreeNode* nextNode = GetAsFreeNode(GetNextNodeInMemory(leftover, GET_NODE_FREE));
	if (nextNode)
	{
		ALLOC_LOG("SimpleAllocator::DoResize() -> Next node is free, joining %u bytes", nextNode->BlockSize);

		MergeBlocks(leftover, nextNode);
	}
	AddToFreeList(leftover);

	PrintState();
}

void rage::sysMemSimpleAllocator::DoSanityCheck() const
{
	// Original implementation was lost

#ifdef ENABLE_ALLOCATOR_SANITY_CHECK
	// Verify every allocator node
	Node* node = m_MainBlock;
	while (node)
	{
		node->AssertGuard();
		node = GetNextNodeInMemory(node);
	}

	// Verify free linked list
	for (FreeNode* freeNode : m_FreeList)
	{
		FreeNode* prevNode = nullptr; // For root bucket node previous node is always null
		while (freeNode)
		{
			// If guard is invalid, there's two main options:
			// 1) It was overwritten by faulty code
			// 2) Node was overwritten
			freeNode->AssertGuard();

			AM_ASSERT(!freeNode->GetIsAllocated(),
				"SimpleAllocator::DoSanityCheck() -> Allocated node found in free list, data is corrupted.");

			AM_ASSERT(freeNode->PreviousLinked == prevNode,
				"SimpleAllocator::DoSanityCheck() -> Previous node link was trashed, data is corrupted.");

			prevNode = freeNode;
			freeNode = freeNode->NextLinked;
		}
	}
#endif
}

rage::sysMemSimpleAllocator::sysMemSimpleAllocator(u64 size, u8 allocIdSeed, bool useSmallocator)
{
	m_bOwnHeap = true;

	// 'Seed' appears to be in range from 0 to 8, guessed from leftover debug in GTA IV for
	// leak detection in EndLayer function.
	m_CurrentAllocID = (allocIdSeed << 23) + 1;

	void* heap = sysMemVirtualAlloc(size);
	AM_ASSERT(heap, "Failed to allocate heap for simple allocator.");

	InitHeap(heap, size, useSmallocator);
}

rage::sysMemSimpleAllocator::~sysMemSimpleAllocator()
{
	m_Smallocator.Destroy(this);

	if (m_bOwnHeap)
		sysMemVirtualFree(m_HeapBase);

	if (m_LogStream)
		m_LogStream->Close();
}

bool rage::sysMemSimpleAllocator::SetQuitOnFail(bool toggle)
{
	bool oldValue = m_QuitOnFail;
	m_QuitOnFail = toggle;

	return oldValue;
}

pVoid rage::sysMemSimpleAllocator::Allocate(u64 size, u64 align, u32 type)
{
	sysCriticalSectionLock lock(m_CriticalSection);

	ALLOC_LOG("");
	ALLOC_LOG("SimpleAllocator::Allocate(size: %llu, align: %llu)", size, align);

	const char* opType = "large alloc";

	void* block;
	if (m_UseSmallocator && m_Smallocator.CanAllocate(size, align))
	{
		ALLOC_LOG("SimpleAllocator::Allocate() -> Doing small allocation");

		opType = "small alloc";
		block = m_Smallocator.Allocate(size, align, this);
	}
	else
	{
		ALLOC_LOG("SimpleAllocator::Allocate() -> Doing large allocation");

		block = DoAllocate(size, align);
	}

	if (!block && m_QuitOnFail)
	{
		rageam::ErrorDisplay::OutOfMemory(this, size, align);
		std::exit(-1);
	}

	PrintLogFor(opType, GetBlockNode(block));
	DoSanityCheck();

	return block;
}

pVoid rage::sysMemSimpleAllocator::TryAllocate(u64 size, u64 align, u32 type)
{
	sysCriticalSectionLock lock(m_CriticalSection);

	bool oldValue = SetQuitOnFail(false);
	void* block = Allocate(size, align, type);
	SetQuitOnFail(oldValue);

	return block;
}

void rage::sysMemSimpleAllocator::Free(pVoid block)
{
	sysCriticalSectionLock lock(m_CriticalSection);

	ALLOC_LOG("");

	// TODO: Actual implementation handles invalid pointer differently.
	AM_ASSERT(IsValidPointer(block), "SimpleAllocator::Free() -> Pointer is not valid.");

	PrintLogFor("free", GetBlockNode(block));

	if (m_Smallocator.IsPointerOwner(block))
		m_Smallocator.Free(block, this);
	else
		DoFree(block);

	DoSanityCheck();
}

void rage::sysMemSimpleAllocator::Resize(pVoid block, u64 newSize)
{
	sysCriticalSectionLock lock(m_CriticalSection);

	ALLOC_LOG("");

	PrintLogFor("resize", GetBlockNode(block));

	if (m_Smallocator.IsPointerOwner(block))
	{
		ALLOC_LOG("SimpleAllocator::Reserve() -> Can't resize small block.");
		return;
	}

	DoResize(block, newSize);
	DoSanityCheck();
}

u64 rage::sysMemSimpleAllocator::GetSize(pVoid block)
{
	if (!IsValidPointer(block))
		return 0;

	if (m_Smallocator.IsPointerOwner(block))
		return m_Smallocator.GetSize(block);

	Node* node = GetBlockNode(block);
	if (node->VerifyGuard() && node->GetIsAllocated())
		return node->BlockSize;

	return 0;
}

u64 rage::sysMemSimpleAllocator::GetMemoryUsed(u8 memoryBucket)
{
	if (memoryBucket < SYS_MEM_MAX_MEMORY_BUCKETS)
		return m_MemoryBuckets[memoryBucket];
	return m_UsedMemory;
}

u64 rage::sysMemSimpleAllocator::GetMemoryAvailable()
{
	return m_AvailableMemoryHigh;
}

u64 rage::sysMemSimpleAllocator::GetLargestAvailableBlock()
{
	sysCriticalSectionLock lock(m_CriticalSection);

	FreeNode* node = nullptr;
	u64 maxSize = 0;

	for (u8 i = MAX_BUCKETS - 1; i > 0; i--)
	{
		node = m_FreeList[i];
		if (node)
			break;
	}

	while (node)
	{
		maxSize = MAX(maxSize, node->BlockSize);
		node = node->NextLinked;
	}
	return maxSize;
}

u64 rage::sysMemSimpleAllocator::GetLowWaterMark(bool updateMeasure)
{
	u64 low = m_AvailableMemoryLow;
	if (updateMeasure)
		m_AvailableMemoryLow = m_AvailableMemoryHigh;
	return low;
}

u64 rage::sysMemSimpleAllocator::GetHighWaterMark(bool updateMeasure)
{
	return GetHeapSize() - GetLowWaterMark(updateMeasure);
}

void rage::sysMemSimpleAllocator::UpdateMemorySnapshots()
{
	memcpy(m_MemorySnapshots, m_MemoryBuckets, sizeof m_MemorySnapshots);
}

u64 rage::sysMemSimpleAllocator::GetMemorySnapshot(u8 memoryBucket)
{
	AM_ASSERT(memoryBucket < SYS_MEM_MAX_MEMORY_BUCKETS,
		"SimpleAllocator::GetMemorySnapshot() -> Memory bucket must be in range from 0 to 15.");

	if (memoryBucket < SYS_MEM_MAX_MEMORY_BUCKETS) // To satisfy compiler
		return m_MemorySnapshots[memoryBucket];
	return 0;
}

u64 rage::sysMemSimpleAllocator::BeginLayer()
{
	m_Layers[m_LayerCount++] = m_CurrentAllocID;
	return m_CurrentAllocID;
}

void rage::sysMemSimpleAllocator::EndLayer(const char* layerName, const char* logName)
{
	sysCriticalSectionLock lock(m_CriticalSection);

	rageam::Logger* memoryLogger = GetMemoryLogger();
	rageam::Logger layerLogger(logName, LOG_OPTION_NO_PREFIX | LOG_OPTION_FILE_ONLY);

	bool hasLeaks = false;
	u32 leakCount = 0;

	// All layer allocation must have id greater than this one
	u32 baseAllocID = m_Layers[--m_LayerCount];

	Node* node = m_MainBlock;
	while (node)
	{
		if (node->GetIsAllocated())
		{
			u32 allocID = node->GetAllocID();
			if (allocID > baseAllocID) // If block is created within the layer
			{
				if (!hasLeaks)
				{
					memoryLogger->LogFormat(LOG_WARNING, "Memory leaks in layer '%s'", layerName);
					hasLeaks = true;
				}

				u32 size = node->BlockSize;
				u32 bucket = node->GetMemoryBucket();

				ConstString sAllocID = FormatAllocID(allocID);

				memoryLogger->GetOptions().Set(LOG_OPTION_NO_PREFIX, true);
				memoryLogger->LogFormat(LOG_WARNING, " - Leak AllocID=%s, Bucket=%u, Size=%u bytes", sAllocID, bucket, size);
				memoryLogger->GetOptions().Set(LOG_OPTION_NO_PREFIX, false);

				layerLogger.LogFormat(LOG_WARNING, "%s", sAllocID);

				leakCount++;
			}
		}
		node = GetNextNodeInMemory(node);
	}

	if (leakCount > 0)
		memoryLogger->LogFormat(LOG_WARNING, "%u leak(s) total", leakCount);
}

void rage::sysMemSimpleAllocator::BeginMemoryLog(const char* logName, bool traceStack)
{
#ifdef ENABLE_ALLOCATOR_OPERATION_LOG
	// Original implementation doesn't support unicode paths but DataManager use absolute path so that's an requirement
	// I added fiStream::FromHandle for that purpose
	rageam::file::WPath logPath = rageam::Logger::GetLogsDirectory() / String::ToWideTemp(logName);
	HANDLE handle = CreateNew(logPath);
	m_LogStream = fiStream::FromHandle(handle, fiDeviceLocal::GetInstance());
	if (!m_LogStream)
	{
		AM_ERRF("SimpleAllocator::BeginMemoryLog(%s) -> Failed to open file for writing.", logName);
		return;
	}

	m_TraceStack = traceStack;
	if (m_TraceStack)
		rageam::StackTracer::RequestSymbols();

	m_LogStream->WriteLine("op,ptr,allocId,size,bucket,stack...\r\n");
#endif
}

void rage::sysMemSimpleAllocator::EndMemoryLog()
{
#ifdef ENABLE_ALLOCATOR_OPERATION_LOG
	if (!m_LogStream)
		return;

	if (m_TraceStack)
		rageam::StackTracer::FreeSymbols();

	m_LogStream->Close();
	m_LogStream = nullptr;
#endif
}

void rage::sysMemSimpleAllocator::SanityCheck()
{
	sysCriticalSectionLock lock(m_CriticalSection);

	DoSanityCheck();
}

bool rage::sysMemSimpleAllocator::IsValidPointer(pVoid block)
{
	u64 addr = reinterpret_cast<u64>(block);
	u64 heap = reinterpret_cast<u64>(m_HeapBase);

	if (addr < heap + HEADER_SIZE)
		return false;

	if (addr >= heap + m_MainHeapSize)
		return false;

	return true;
}

u64 rage::sysMemSimpleAllocator::GetSizeWithOverhead(pVoid block)
{
	// The only difference from GetSize is + HEADER_SIZE

	if (!IsValidPointer(block))
		return 0;

	if (m_Smallocator.IsPointerOwner(block))
		return m_Smallocator.GetSize(block); // Smallocator has no overhead

	Node* node = GetBlockNode(block);
	if (node->VerifyGuard() && node->GetIsAllocated())
		return node->BlockSize + HEADER_SIZE;

	return 0;
}
