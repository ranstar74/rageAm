#pragma once

#include "allocator.h"
#include "am/system/errordisplay.h"
#include "helpers/align.h"
#include "helpers/ranges.h"

namespace rage
{
	struct sysMemContainerData
	{
		pVoid	Block; // Has to be allocated on heap
		u32		Size;
	};

	/**
	 * \brief The most simple linear allocator,
	 * used to minimize fragmentation by putting small sub-allocations inside single memory block.
	 */
	class sysMemContainer : public sysMemAllocator
	{
		u32 m_Offset = 0;
		sysMemContainerData& m_Data;
		sysMemAllocator* m_PreviousAllocator = nullptr;

	public:
		sysMemContainer(sysMemContainerData& data) : m_Data(data)
		{

		}

		void Bind()
		{
			m_PreviousAllocator = GetCurrent();
			SetCurrent(this);
		}

		void UnBind()
		{
			SetCurrent(m_PreviousAllocator);
			m_PreviousAllocator = nullptr;
		}

		void FreeHeap() const
		{
			rage_free(m_Data.Block);
			m_Data.Block = nullptr;
			m_Data.Size = 0;
		}

		pVoid Allocate(u64 size, u64 align, u32 type) override
		{
			// There's crazy batshit code that calls malloc(0) and expects result to be valid
			size = MAX(size, 1);

			// Check if allocated block is within the heap
			if (m_Offset + size > m_Data.Size)
			{
				rageam::ErrorDisplay::OutOfMemory(this, size, align);
				std::exit(-1);
			}

			u32 allocOffset = ALIGN(m_Offset, align);

			// Set new offset after allocated block
			m_Offset = allocOffset + static_cast<u32>(size);

			return static_cast<char*>(m_Data.Block) + allocOffset;
		}

		pVoid TryAllocate(u64 size, u64 align, u32 type) override { return Allocate(size, align, type); }

		void Free(pVoid block) override
		{
			if (!block)
				return;

			u64 blockAddr = reinterpret_cast<u64>(block);
			u64 heapAddr = reinterpret_cast<u64>(m_Data.Block);

			// Block is within container heap, we do nothing because
			// all memory will be freed once in ::UnUse
			if (blockAddr >= heapAddr && blockAddr < heapAddr + m_Data.Size)
				return;

			// If block doesn't belong to container, resolve it in regular allocator
			m_PreviousAllocator->Free(block);
		}

		u64 GetMemoryUsed(u8 memoryBucket) override
		{
			return m_Offset;
		}

		u64 GetMemoryAvailable() override
		{
			return m_Data.Size - m_Offset;
		}

		u32 GetOffset() const { return m_Offset; }
	};
}
