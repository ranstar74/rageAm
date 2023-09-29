#pragma once

#include "memoryapi.h"

#include "common/logger.h"
#include "common/types.h"

#include "helpers/align.h"

namespace rage
{
	inline void* sysMemVirtualAlloc(u64 size)
	{
		// 2699.16 implementation also have virtual protect and watch
		// flags but I decided to cut it out as unneeded.
		// I suppose this was used during debugging to find code that
		// overrans allocated memory, in addition to guard words.

		size = ALIGN_4096(size);

		DWORD allocType = MEM_COMMIT | MEM_RESERVE;
		return VirtualAlloc(NULL, size, allocType, PAGE_READWRITE);
	}

	inline void sysMemVirtualFree(void* block)
	{
		VirtualFree(block, 0, MEM_RELEASE);
	}
}
