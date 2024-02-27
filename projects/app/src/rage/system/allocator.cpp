#include "allocator.h"

#ifdef AM_INTEGRATED
#include "am/integration/memory/address.h"
#include "tls.h"

namespace
{
	std::mutex                                s_Mutex;
	bool                                      s_Initialized = false;
	rage::sysMemAllocator*					  s_TheAllocator;
	thread_local bool						  tl_UseGameAllocators = false;
	rage::ThreadLocal<rage::sysMemAllocator*> tl_Current;
	rage::ThreadLocal<rage::sysMemAllocator*> tl_Master;
	rage::ThreadLocal<rage::sysMemAllocator*> tl_Container;
}

void ScanTlsOffsetsAndTheAllocator()
{
	std::unique_lock lock(s_Mutex);
	if (s_Initialized)
		return;

	// sysMemAllocator_sm_Current;
	// sysMemAllocator_sm_Master;
	// sysMemAllocator_sm_Container;
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	gmAddress addr = gmAddress::Scan("48 8B C8 E8 ?? ?? ?? ?? BA FB 07 00 00").GetAt(-207);
	int current = *addr.GetAt(1).To<int*>();
	int master = *addr.GetAt(37 + 1).To<int*>();
	// SetContainer call was not inlined
	int container = *addr.GetAt(81).GetCall().GetAt(43 + 1).To<int*>();
	// Shift to theAllocator
	addr = addr.GetAt(26);
#elif APP_BUILD_2699_16_RELEASE
	static gmAddress addr = gmAddress::Scan("48 8D 2D ?? ?? ?? ?? A8 10");
	int current = *addr.GetAt(605).GetAt(1).To<int*>();
	int master = *addr.GetAt(605 + 9).GetAt(1).To<int*>();
	int container = *addr.GetAt(605 + 9 + 9).GetAt(1).To<int*>();
#else
	static gmAddress addr = gmAddress::Scan("48 89 1D ?? ?? ?? ?? 48 89 1C 10");
	int current = *addr.GetAt(-5).GetAt(1).To<int*>();
	int master = *addr.GetAt(11).GetAt(1).To<int*>();
	int container = *addr.GetAt(20).GetAt(1).To<int*>();
#endif
	s_TheAllocator = addr.GetRef(3).To<rage::sysMemAllocator*>();

	tl_Current.SetOffset(current);
	tl_Master.SetOffset(master);
	tl_Container.SetOffset(container);

	s_Initialized = true;
}

void rage::sysMemUseGameAllocators(bool toggle)
{
	if (toggle)
		sysMemInitCurrentThread();

	tl_UseGameAllocators = toggle;
}

bool rage::sysMemIsUsingGameAllocators()
{
	return tl_UseGameAllocators;
}

void rage::sysMemInitCurrentThread()
{
	ScanTlsOffsetsAndTheAllocator();

	// Prevent overwriting existing values
	thread_local bool isSet = false;
	if(!isSet)
	{
		// Set master allocator everywhere as default
		tl_Current.Set(s_TheAllocator);
		tl_Master.Set(s_TheAllocator);
		tl_Container.Set(s_TheAllocator);
		isSet = true;
	}
}

rage::sysMemAllocator* rage::sysMemGetMaster()
{
	ScanTlsOffsetsAndTheAllocator();
	return s_TheAllocator;
}
#endif
