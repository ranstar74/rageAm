// ReSharper disable CppClangTidyClangDiagnosticInlineNewDelete
// ReSharper disable CppClangTidyClangDiagnosticImplicitExceptionSpecMismatch
// ReSharper disable CppParameterNamesMismatch
// ReSharper disable CppClangTidyReadabilityRedundantDeclaration
#pragma once

#pragma warning( push )
#pragma warning( disable : 28251 ) // Inconsistent SAL annotation

#include "rage/system/systemheap.h"
#include "am/system/protect_new.h"

#ifndef AM_USE_SYS_OVERRUN_DETECTION // We can't use both overrun detection and system allocator
// Un-define this to use external memory debugging tools
#define AM_USE_SYS_ALLOCATORS
#endif

#ifdef AM_USE_SYS_ALLOCATORS
void* operator new(size_t size, size_t align, rage::eAllocatorType type);

void* operator new(size_t size, size_t align);
void* operator new(size_t size);

void* operator new[](size_t size);
void* operator new[](size_t size, size_t align);

void operator delete(void* block);
void operator delete[](void* block);
#endif

inline void* rage_malloc(size_t size) { return operator new(size, 16); }
inline void* rage_malloc(size_t size, size_t align) { return operator new(size, align); }
inline void rage_free(void* block) { return operator delete(block); }

// Allocates memory on virtual allocator.
#define virtual_new		new(16, rage::ALLOC_TYPE_VIRTUAL)
// Allocates memory on physical allocator.
#define physical_new	new(16, rage::ALLOC_TYPE_PHYSICAL)

#pragma warning( pop ) 
