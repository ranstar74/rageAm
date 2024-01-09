// ReSharper disable CppClangTidyClangDiagnosticImplicitExceptionSpecMismatch
// ReSharper disable CppParameterNamesMismatch

#include "new.h"

#pragma warning( push )
#pragma warning( disable : 28251) // Inconsistent SAL annotation

#ifdef AM_USE_SYS_ALLOCATORS

void* operator new(size_t size, size_t align, rage::eAllocatorType type)
{
	return GetMultiAllocator()->Allocate(size, align, type);
}

void* operator new(size_t size)
{
	return GetAllocator()->Allocate(size);
}

void* operator new(size_t size, size_t align)
{
	return GetAllocator()->Allocate(size, align);
}

void* operator new [](size_t size)
{
	return GetAllocator()->Allocate(size);
}

void* operator new [](size_t size, size_t align)
{
	return GetAllocator()->Allocate(size, align);
}

void operator delete(void* block)
{
	if (!block)
		return;

	// README:
	// We must ensure that our allocators are either the last item
	// dynamically destructed or we simply don't allow global variables with allocations
	// Life is not simple and some libraries (such as lunasvg) declare static std containers
	// within the functions, this is totally awful
	// Which leaves us only one option to ignore deletion if there's no active allocator set
	rage::sysMemAllocator* allocator = GetAllocator();
	if (allocator) allocator->Free(block);
}

void operator delete [](void* block)
{
	operator delete(block);
}

#endif
#pragma warning( pop )
