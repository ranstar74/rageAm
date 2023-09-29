#pragma once

#include "common/types.h"

#include "multiallocator.h"

// Mostly used for testing, allows vs memory debugging
#define USE_OS_ALLOCATOR

namespace rage
{
	class SystemHeap
	{
		// This includes large expenses on resource compiler
		static constexpr u64 GENERAL_ALLOCATOR_SIZE = 600ull * 1024ull * 1024ull;	// 600MB
		static constexpr u64 VIRTUAL_ALLOCATOR_SIZE = 60ull * 1024ull * 1024ull;	// 60MB
		static constexpr u64 PHYSICAL_ALLOCATOR_SIZE = 90ull * 1024ull * 1024ull;	// 90MB
		 
		static constexpr u64 MIN_BUDDY_SIZE = 0x2000;

		static sysMemMultiAllocator* sm_MultiAllocator;
	public:
		static void Init();
		static void Shutdown();

		static sysMemMultiAllocator* GetAllocator() { return sm_MultiAllocator; }
	};
}

/**
 * \brief Gets multi allocator instance with general, virtual and physical allocators.
 */
rage::sysMemAllocator* GetMultiAllocator();
rage::sysMemAllocator* GetAllocator(rage::eAllocatorType type);
rage::sysMemAllocator* GetAllocator();
