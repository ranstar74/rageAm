//
// File: configmanager.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/logger.h"
#include "common/types.h"
#include "rage/atl/hashstring.h"

namespace rage
{
	/**
	 * \brief Manages loading and access for gameconfig.xml
	 */
	class fwConfigManager
	{
		static inline fwConfigManager* sm_Instance = nullptr;

	public:
		static fwConfigManager* GetInstance() { return sm_Instance; }

		u32 GetSizeOfPool(atHashValue name, u32 defaultSize) const
		{
			AM_ERRF("fwConfigManager::GetSizeOfPool(name: %s) -> Not Implemented!", atHashString(name).TryGetCStr());
			return 256;
		}
	};
}
