#pragma once

#include <Windows.h>
#include "common/types.h"

namespace rage
{
	class TlsManager
	{
		static u64 GetBase()
		{
			// Dirty hack to get slot 0 of current thread block
			return *(u64*)((u64)NtCurrentTeb() + 0x58);
		}
	public:
		template<typename T>
		static T& GetAt(u32 offset) { return *(T*)(GetBase() + offset); }
	};

	template<typename T>
	class ThreadLocal
	{
		// TODO: ...
	};
}
