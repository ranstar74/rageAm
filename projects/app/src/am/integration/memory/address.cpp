#include "address.h"

#include "common/logger.h"
#include "am/system/asserts.h"
#include "am/system/timer.h"

#include <cstdlib>
#include <mutex>

gmAddress gmAddress::Scan(const char* patternStr, const char* debugName)
{
	static constexpr u64 MAX_SANE_SCAN_MS = 25;

	rageam::Timer timer = rageam::Timer::StartNew();

	static u64 moduleSize;
	static u64 moduleBase;
	static bool initialized = false;
	static std::mutex initMutex;
	if (!initialized && initMutex.try_lock())
	{
		GetModuleBaseAndSize(&moduleBase, &moduleSize);
		initialized = true;
		initMutex.unlock();
	}

	// Convert string pattern into byte array form
	s16 pattern[256];
	u8 patternSize = 0;
	for (size_t i = 0; i < strlen(patternStr); i += 3)
	{
		const char* cursor = patternStr + i;

		if (cursor[0] == '?')
			pattern[patternSize] = -1;
		else
			pattern[patternSize] = static_cast<s16>(strtol(cursor, nullptr, 16));

		// Support single '?' (we're incrementing by 3 expecting ?? and space, but with ? we must increment by 2)
		if (cursor[1] == ' ')
			i--;

		patternSize++;
	}

	// In two-end comparison we approach from both sides (left & right) so size is twice smaller
	u8 scanSize = patternSize;
	if (scanSize % 2 == 0)
		scanSize /= 2;
	else
		scanSize = patternSize / 2 + 1;

	// Search for string through whole module
	// We use two-end comparison, nothing fancy but better than just brute force
	for (u64 i = 0; i < moduleSize; i += 1)
	{
		const u8* modulePos = (u8*)(moduleBase + i);
		for (u8 j = 0; j < scanSize; j++)
		{
			s16 lExpected = pattern[j];
			s16 lActual = modulePos[j];

			if (lExpected != -1 && lActual != lExpected) goto miss;

			s16 rExpected = pattern[patternSize - j - 1];
			s16 rActual = modulePos[patternSize - j - 1];

			if (rExpected != -1 && rActual != rExpected) goto miss;
		}
		timer.Stop();
		if (timer.GetElapsedMilliseconds() > MAX_SANE_SCAN_MS)
			AM_WARNINGF("Pattern '%s' ('%s') took %llu ms! Too much.", patternStr, debugName, timer.GetElapsedMilliseconds());
		return { moduleBase + i };
	miss:;
	}
	AM_UNREACHABLE("gmAddress::Scan(%s) -> Failed to find pattern %s", debugName, patternStr);
}
