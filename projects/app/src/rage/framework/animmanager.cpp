#include "animmanager.h"

#include "am/integration/memory/address.h"

rage::strLocalIndex rage::fwAnimManager::FindSlotFromHashKey(u32 clipDictNameHash)
{
	static auto fn = gmAddress::Scan("44 8B DA 33 D2 4C 8B D1").ToFunc<void(strLocalIndex*, u32)>();

	strLocalIndex outIndex;
	fn(&outIndex, clipDictNameHash);
	return outIndex;
}
