#pragma once

#include "allocator.h"

namespace rage
{
	// To be fully honest here this is totally over-engineered, most functions
	// are either not implemented or redirected to the first slot.

	class sysMemMultiAllocator : public sysMemAllocator
	{
		// GTA V Layout:
		// ID Name						Purpose				Size
		//
		// 0: sysMemSimpleAllocator		General				488MB
		// 1: sysMemGrowBuddyAllocator	Virtual				1GB
		// 2: sysMemGrowBuddyAllocator	Reference to #3
		// 3: sysMemGrowBuddyAllocator	Physical			268MB
		// 4: sysMemSimpleAllocator		Reference to #0
		// 5: Reserved
		// 6: Reserved
		// 7: Reserved
		// Virtual / Physical ones are resource allocators
		sysMemAllocator* m_Allocators[8]{};

		u32 m_AllocatorCount = 0;
	public:
		pVoid Allocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override
		{
			return m_Allocators[type]->Allocate(size, align, type);
		}

		pVoid TryAllocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) override
		{
			return m_Allocators[type]->TryAllocate(size, align, type);
		}

		void Free(pVoid block) override;
		void Resize(pVoid block, u64 newSize) override;

		sysMemAllocator* GetAllocator(u32 type) override { return m_Allocators[type]; }
		const sysMemAllocator* GetAllocator(u32 type) const override { return m_Allocators[type]; }

		sysMemAllocator* GetPointerOwner(pVoid block) override;

		u64 GetSize(pVoid block) override;
		u64 GetMemoryUsed(u8 memoryBucket = SYS_MEM_INVALID_BUCKET) override;
		u64 GetMemoryAvailable() override;
		u64 GetLargestAvailableBlock() override;

		u64 GetLowWaterMark(bool updateMeasure) override { return 0; }
		u64 GetHighWaterMark(bool updateMeasure) override { return 0; }

		void UpdateMemorySnapshots() override { }
		u64 GetMemorySnapshot(u8 memoryBucket) override { return 0; }

		bool IsTailed() override { return true; }

		u64 BeginLayer() override { return m_Allocators[0]->BeginLayer(); }
		void EndLayer(const char* layerName, const char* logPath) override
		{
			return m_Allocators[0]->EndLayer(layerName, logPath);
		}

		void BeginMemoryLog(const char* fileName, bool traceStack) override
		{
			m_Allocators[0]->BeginMemoryLog(fileName, traceStack);
		}
		void EndMemoryLog() override { m_Allocators[0]->EndMemoryLog(); }

		bool IsBuildingResource() override { return false; }
		bool HasMemoryBuckets() override { return false; }

		void SanityCheck() override;

		bool IsValidPointer(pVoid block) override;

		u64 GetSizeWithOverhead(pVoid block) override;

		virtual u32 GetAllocatorCount() { return m_AllocatorCount; }

		pVoid GetHeapBase() override { return nullptr; }
		u64 GetHeapSize() override { return 0; }

		void AddAllocator(sysMemAllocator* that)
		{
			m_Allocators[m_AllocatorCount++] = that;
		}
	};
}
