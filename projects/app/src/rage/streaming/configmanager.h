#pragma once

#include "common/logger.h"
#include "common/types.h"
#include "rage/atl/hashstring.h"

namespace rage
{
	class fwConfigManager
	{
		static inline fwConfigManager* sm_Instance = nullptr;

	public:
		static fwConfigManager* GetInstance() { return sm_Instance; }

		u32 GetSizeOfPool(const atHashString& name, u32 defaultSize) const
		{
			AM_ERRF("fwConfigManager::GetSizeOfPool(name: %s) -> Not Implemented!", name.GetCStr());
			return 256;
		}
	};
}
