#include "systemheap.h"

#include "simpleallocator.h"
#include "buddyallocator.h"
#include "osallocator.h"
#include "am/system/asserts.h"

namespace
{
	bool		s_Initialized = false;
	std::mutex	s_InitMutex;
}

rage::sysMemMultiAllocator* rage::SystemHeap::sm_MultiAllocator;

void rage::SystemHeap::Init()
{
#ifndef USE_OS_ALLOCATOR
	static sysMemSimpleAllocator		s_General(GENERAL_ALLOCATOR_SIZE);
#else
	static sysMemOsAllocator			s_General;
#endif
	static sysMemGrowBuddyAllocator		s_Virtual(MIN_BUDDY_SIZE, VIRTUAL_ALLOCATOR_SIZE);
	static sysMemGrowBuddyAllocator		s_Physical(MIN_BUDDY_SIZE, PHYSICAL_ALLOCATOR_SIZE);
	static sysMemMultiAllocator			s_Multi;

	s_Multi.AddAllocator(&s_General);
	s_Multi.AddAllocator(&s_Virtual);
	s_Multi.AddAllocator(&s_Physical);
	s_Multi.AddAllocator(&s_Physical);
	s_Multi.AddAllocator(&s_General);

#ifndef AM_UNIT_TESTS
	s_Multi.BeginLayer();
	s_Multi.BeginMemoryLog("game_heap_log", true);
#endif

	sm_MultiAllocator = &s_Multi;

	sysMemAllocator::SetCurrentFallback(sm_MultiAllocator);
}

void rage::SystemHeap::Shutdown()
{
	if (!sm_MultiAllocator)
		return;

#ifndef AM_UNIT_TESTS
	sm_MultiAllocator->EndMemoryLog();
	sm_MultiAllocator->EndLayer("Global", "global_heap_leaks");
#endif
	sm_MultiAllocator = nullptr;

	sysMemAllocator::SetCurrentFallback(nullptr);
}

static void EnsureHeapInitialized()
{
	std::unique_lock lock(s_InitMutex);
	if (!s_Initialized)
	{
		rage::SystemHeap::Init();
		s_Initialized = true;
	}
}

rage::sysMemAllocator* GetMultiAllocator()
{
	EnsureHeapInitialized();

	rage::sysMemMultiAllocator* multi = rage::SystemHeap::GetAllocator();
	AM_ASSERT(multi,
		"GetAllocator() -> Allocator was destructed! Make sure to clean up all memory in rageam::System::Destroy!");
	return multi;
}

rage::sysMemAllocator* GetAllocator(rage::eAllocatorType type)
{
	return GetMultiAllocator()->GetAllocator(type);
}

rage::sysMemAllocator* GetAllocator()
{
	EnsureHeapInitialized();

	return rage::sysMemAllocator::GetCurrent();
}
