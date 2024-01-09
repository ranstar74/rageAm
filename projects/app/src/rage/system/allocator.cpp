#include "allocator.h"

#ifdef AM_INTEGRATED
#include "am/integration/memory/address.h"
#include "tls.h"
#endif

void rage::sysMemInitCurrentThread()
{
#ifdef AM_INTEGRATED
	// sysMemAllocator_sm_Current;
	// sysMemAllocator_sm_Master;
	// sysMemAllocator_sm_Container;
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	static gmAddress addr = gmAddress::Scan("48 8B C8 E8 ?? ?? ?? ?? BA FB 07 00 00").GetAt(-207);
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
	sysMemAllocator* theAllocator = *addr.GetRef(3).To<sysMemAllocator**>();

	ThreadLocal<sysMemAllocator*> tl_Current(current);
	ThreadLocal<sysMemAllocator*> tl_Master(master);
	ThreadLocal<sysMemAllocator*> tl_Container(container);

	tl_Current.Set(theAllocator);
	tl_Master.Set(theAllocator);
	tl_Container.Set(theAllocator);
#endif
}
