//
// File: interlocked.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

namespace rage
{
	u32 sysInterlockedIncrement(volatile u32* destination);
	s32 sysInterlockedIncrement(volatile s32* destination);
	u16 sysInterlockedIncrement(volatile u16* destination);
	s16 sysInterlockedIncrement(volatile s16* destination);

	u32 sysInterlockedDecrement(volatile u32* destination);
	s32 sysInterlockedDecrement(volatile s32* destination);
	u16 sysInterlockedDecrement(volatile u16* destination);
	s16 sysInterlockedDecrement(volatile s16* destination);
}
