#include "protect_new.h"

#ifdef AM_USE_SYS_OVERRUN_DETECTION

#include <Windows.h>
#include "helpers/align.h"

DWORD PageSize = 0;

void InitPageSize()
{
	if (PageSize)
		return;

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	PageSize = sysInfo.dwPageSize;
}

void* operator new(size_t size, size_t align)
{
	// We have to ignore alignment
	return operator new(size);
}

void* operator new(size_t size)
{
	InitPageSize();

	// We allocate two pages and put allocated block at the end of first page,
	// padding is distance between beginning of page and allocated block.
	size_t allocSize = ALIGN(size, PageSize);
	size_t leftPadding = allocSize - size;

	allocSize += PageSize; // Reserve protected page

	char* block = static_cast<char*>(VirtualAlloc(
		NULL,
		allocSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE));

	// Next adjacent block that we allocate is protected from reading / writing,
	// which will help us to detect any buffer overrun
	DWORD dwOldProtect;
	VirtualProtect(block + allocSize - PageSize, PageSize, PAGE_NOACCESS, &dwOldProtect);

	return block + leftPadding;
}

void* operator new[](size_t size)
{
	return operator new(size);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void operator delete(void* block)
{
	MEMORY_BASIC_INFORMATION mbi;
	DWORD dwOldProtect;

	VirtualQuery(block, &mbi, sizeof mbi);
	// Leave pages in reserved state, but free the physical memory
	VirtualFree(mbi.AllocationBase, 0, MEM_DECOMMIT);
	// Protect the address space, so no one can access those pages
	VirtualProtect(mbi.AllocationBase, mbi.RegionSize, PAGE_NOACCESS, &dwOldProtect);
}

void operator delete [](void* block)
{
	operator delete(block);
}
#endif
