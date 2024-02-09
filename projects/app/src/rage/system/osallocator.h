#pragma once

#include "allocator.h"
#include "helpers/align.h"

namespace rage
{
	// Note: This is not part of rage.

	/**
	 * \brief Uses system calls (malloc, free) to allocate memory, used for testing purposes.
	 */
	class sysMemOsAllocator : public sysMemAllocator
	{
		DWORD m_PageSize = 0;

		void InitPageSize()
		{
			if (m_PageSize)
				return;

			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			m_PageSize = sysInfo.dwPageSize;
		}

	public:
		pVoid Allocate(u64 size, u64 align, u32 type) override
		{
			InitPageSize();

			// We allocate two pages and put allocated block at the end of first page,
			// padding is distance between beginning of page and allocated block.
			size_t allocSize = ALIGN(size, m_PageSize);
			size_t leftPadding = allocSize - size;

			allocSize += m_PageSize; // Reserve protected page

			char* block = static_cast<char*>(VirtualAlloc(
				NULL,
				allocSize,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE));

			// Next adjacent block that we allocate is protected from reading / writing,
			// which will help us to detect any buffer overrun
			DWORD dwOldProtect;
			VirtualProtect(block + allocSize - m_PageSize, m_PageSize, PAGE_NOACCESS, &dwOldProtect);

			return block + leftPadding;
		}

		pVoid TryAllocate(u64 size, u64 align, u32 type) override
		{
			return Allocate(size, align, type);
		}

		void Free(pVoid block) override
		{
			MEMORY_BASIC_INFORMATION mbi;
			DWORD dwOldProtect;

			VirtualQuery(block, &mbi, sizeof mbi);
			// Leave pages in reserved state, but free the physical memory
			VirtualFree(mbi.AllocationBase, 0, MEM_DECOMMIT);
			// Protect the address space, so no one can access those pages
			VirtualProtect(mbi.AllocationBase, mbi.RegionSize, PAGE_NOACCESS, &dwOldProtect);
		}

		u64 GetMemoryUsed(u8 memoryBucket) override { return 0; }
		u64 GetMemoryAvailable() override { return 0; }
	};
}
