#include "interlocked.h"
#include "am/system/asserts.h"

#include <intrin0.inl.h>

u32 rage::sysInterlockedIncrement(volatile u32* destination)
{
	AM_ASSERTS(*destination < INT32_MAX);
	return _InterlockedIncrement((volatile long*) destination);
}

s32 rage::sysInterlockedIncrement(volatile s32* destination)
{
	return _InterlockedIncrement((volatile long*) destination);
}

u16 rage::sysInterlockedIncrement(volatile u16* destination)
{
	AM_ASSERTS(*destination < INT16_MAX);
	return _InterlockedIncrement16((volatile short*) destination);
}

s16 rage::sysInterlockedIncrement(volatile s16* destination)
{
	return _InterlockedIncrement16(destination);
}

u32 rage::sysInterlockedDecrement(volatile u32* destination)
{
	AM_ASSERTS(*destination > 0);
	return _InterlockedDecrement((volatile long*) destination);
}

s32 rage::sysInterlockedDecrement(volatile s32* destination)
{
	return _InterlockedDecrement((volatile long*) destination);
}

u16 rage::sysInterlockedDecrement(volatile u16* destination)
{
	AM_ASSERTS(*destination > 0);
	return _InterlockedDecrement16((volatile short*) destination);
}

s16 rage::sysInterlockedDecrement(volatile s16* destination)
{
	return _InterlockedDecrement16(destination);
}
