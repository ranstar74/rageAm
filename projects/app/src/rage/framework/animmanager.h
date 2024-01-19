//
// File: animmanager.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/streaming/streamingdefs.h"

namespace rage
{
	class fwAnimManager
	{
	public:
		// Looks up slot in ClipDictionaryStore from #cd name hash
		static strLocalIndex FindSlotFromHashKey(u32 clipDictNameHash);
	};
}
