#pragma once
#include "common/types.h"

namespace rage
{
	// Whether any resource is compiling in current thread.
	bool IsResourceCompiling();

	pVoid pgAlloc(u32 size);
	void pgFree(pVoid block);
}
