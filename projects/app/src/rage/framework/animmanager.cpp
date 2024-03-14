#include "animmanager.h"

#include "am/integration/memory/address.h"

rage::strLocalIndex rage::fwAnimManager::FindSlotFromHashKey(u32 clipDictNameHash)
{
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"44 8B DA 33 D2 4C 8B D1", "rage::fwAnimManager::FindSlotFromHashKey")
#else
		"44 89 5C 24 10", "rage::fwAnimManager::FindSlotFromHashKey+0x94")
		.GetAt(-0x94)
#endif
		.ToFunc<void(strLocalIndex*, u32)>();

	strLocalIndex outIndex;
	fn(&outIndex, clipDictNameHash);
	return outIndex;
}
